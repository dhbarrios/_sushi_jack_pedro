/*
 * memoria.c
 *
 */

#include "memoria.h"

//----------------------------------PRIVADO---------------------------------------

static t_memoria_principal memoria_principal;
static t_list *tablas_de_paginas;
static t_tabla_tlb *tlb;
static t_dictionary *pids_en_uso;

void log_reporte( FILE *fp, char screen_log, char* message_template, ...){

	char* message;

	va_list arguments;
	va_start(arguments, message_template);
	message = string_from_vformat(message_template, arguments);

	if(screen_log) printf(message); //No termino de entender porque tira estos dos warnings, igual funciona
	fprintf(fp,message);

	free(message);
	va_end(arguments);
}

int compareTimestamps(timestamp* t1, timestamp* t2){
	if(t1->tv_sec == t2->tv_sec){
		return t1->tv_usec - t2->tv_usec;
	} else {
		return t1->tv_sec - t2->tv_sec;
	}
}

//para convertir el timestamp a string, por si lo quieren usar para hacer algun reporte de la tlb
char* timestamp_to_string(timestamp* ts){
	time_t nowtime;
	struct tm *nowtm;
	char* tmbuf = malloc(sizeof(char)*20);
	char* buf = malloc(sizeof(char)*27);

	gettimeofday(ts, NULL);
	nowtime = ts->tv_sec;
	nowtm = localtime(&nowtime);
	strftime(tmbuf, 20, "%Y-%m-%d %H:%M:%S", nowtm);
	snprintf(buf, 27, "%s.%06d", tmbuf, (int)ts->tv_usec);
	free(tmbuf);
	return buf;
}

timestamp *time_stamp(){
	timestamp* ts = malloc(sizeof(timestamp));
	gettimeofday(ts, NULL);
	return ts;
}

void destructor_tabla(void* tabla){
	t_tabla *tmp;

	void eliminar_fila(void *fila){
		free(fila);
	}

	tmp=(t_tabla*) tabla;

	list_destroy_and_destroy_elements(tmp->filas,eliminar_fila);

	free(tmp);
}

void destructor_fila_tlb(void* fila){
	t_fila_tlb *tmp=(t_fila_tlb*) fila;
	free(tmp->timeStamp);
	free(tmp);
}

t_tabla* obtener_tabla(uint32_t pid){
	t_tabla *tabla_buscada;


	bool tabla_valida(void* tabla){
		return ((t_tabla*) tabla)->pid==pid;
	}

	return tabla_buscada=list_find(tablas_de_paginas,tabla_valida);
}

uint32_t obtener_numero_marco_tlb(uint32_t numero_pagina){
	t_fila_tlb *fila_buscada;

	bool fila_valida(void* fila){
		t_fila_tlb *tmp=(t_fila_tlb*) fila;

		return 	tmp->numero_pagina==numero_pagina &&
				tmp->pid==obtener_pid();
	}

	fila_buscada=list_find(tlb->filas,fila_valida);

	if(fila_buscada){
		free(fila_buscada->timeStamp);
		fila_buscada->timeStamp = time_stamp();//hago refresh del timestamp
		logDebug("Acierto TLB: (PID:%d,Pag:%d,Marco:%d)",obtener_pid(),fila_buscada->numero_pagina,fila_buscada->numero_marco);
	}

	return fila_buscada ? fila_buscada->numero_marco : -1;
}

uint32_t obtener_numero_marco_tabla(uint32_t numero_pagina){

	ejecutarRetardoMemoria();//retardo por acceder a la tabla de paginas

	t_tabla *tabla_buscada;
	t_fila_tabla *fila_buscada;

	bool fila_valida(void* fila){
		return ((t_fila_tabla*) fila)->numero_pagina==numero_pagina;
	}

	tabla_buscada=obtener_tabla(obtener_pid());
	if(tabla_buscada==NULL){
		logDebug("No se encontro la tabla");
		return -3;	//No existe la tabla del proceso, estamos hasta las manos
	}
	if(numero_pagina > tabla_buscada->tamanio){
		logDebug("Quiere entrar a una posicion que no le pertenece");
		return -2; //Quiere entrar a una posicion que no le pertenece
	}
	fila_buscada=list_find(tabla_buscada->filas,fila_valida);
	if(fila_buscada==NULL){
		logDebug("No se encontro la fila por lo que la pagina no esta en memoria");
		return -1;	//No se encontro la fila, por lo que la pagina no esta en memoria
	}

	logDebug("Numero de marco encontrado: %d", fila_buscada->numero_marco);
	return fila_buscada->numero_marco;
}

void cargar_dir_tabla(uint32_t numero_pagina, uint32_t numero_marco){

	t_tabla *tabla;
	t_fila_tabla *fila;

	bool fila_valida(void* fila){
		return ((t_fila_tabla*) fila)->numero_pagina==numero_pagina;
	}

	tabla=obtener_tabla(obtener_pid());	//La tabla a esta altura tiene que existir

	if(!(fila=list_find(tabla->filas,fila_valida))){
		//La fila puede no existir, la tabla se crea vacia
		fila=malloc(sizeof(t_fila_tabla));
		list_add_in_index(tabla->filas,tabla->puntero,(void*)fila);
		tabla->puntero++;
		tabla->puntero %= tabla->filas->elements_count;
	}

	fila->numero_pagina=numero_pagina;
	fila->numero_marco=numero_marco;
	fila->accedido=1;
	fila->modificado=0;
}

void borrar_dir_tabla(t_tabla *tabla_prc, uint32_t numero_marco){

	void eliminar(void* fila){
		free(fila);
	}

	bool fila_valida(void* fila){
		return ((t_fila_tabla*)fila)->numero_marco == numero_marco;
	}


	list_remove_and_destroy_by_condition(tabla_prc->filas, fila_valida, eliminar);
}

void agregar_fila_tlb(t_list* filas, t_fila_tlb* filaAgregar)
{
	list_add(tlb->filas,filaAgregar);
}


t_fila_tlb* algoritmoLRU (t_list* filas)
{
	t_fila_tlb* filaVictima;
	filaVictima = list_get(filas,0);


	void esMayor(void* aux)
	{
		t_fila_tlb* fila = (t_fila_tlb *) aux;

		if(compareTimestamps(fila->timeStamp,filaVictima->timeStamp) < 0)
		{
			filaVictima = fila;
		}
	}
	list_iterate(filas, esMayor);

	return filaVictima;
}

void remover_fila_tlb(t_list* filas, t_fila_tlb* filaQuitar)
{
	bool misma_entrada(void* aux)
	{
		t_fila_tlb* fila = (t_fila_tlb *) aux;

		return ((filaQuitar->pid == fila->pid) && (filaQuitar->numero_marco == fila->numero_marco) && (filaQuitar->numero_pagina == fila->numero_pagina));
	}

	list_remove_and_destroy_by_condition(filas,misma_entrada,destructor_fila_tlb);
}

void copiar_pagina_a_tlb(uint32_t numero_pagina, uint32_t numero_marco)
{
	if(config->tamanio_tlb<=0){
		return;
	}
	logDebug("Copiando entrada a TLB: (PID:%d,Pag:%d,Marco:%d)",obtener_pid(),numero_pagina,numero_marco);
	t_fila_tlb* filaQuitar;
	t_fila_tlb* filaAgregar;
	filaAgregar = (t_fila_tlb*)malloc(sizeof(t_fila_tlb));
	filaAgregar ->timeStamp = time_stamp();
	filaAgregar ->numero_marco = numero_marco;
	filaAgregar ->numero_pagina = numero_pagina;
	filaAgregar->pid = obtener_pid();

	pthread_mutex_lock(&tlb_mutex);


	if (list_size(tlb->filas) < config->tamanio_tlb)
	{

		agregar_fila_tlb(tlb->filas, filaAgregar);

	}
	else
	{
		filaQuitar = algoritmoLRU(tlb->filas);
		logDebug("LRU ejecutado, removiendo (PID:%d,Pag:%d,Marco:%d)",filaQuitar->pid,filaQuitar->numero_pagina,filaQuitar->numero_marco);
		remover_fila_tlb(tlb->filas,filaQuitar);
		agregar_fila_tlb(tlb->filas, filaAgregar);
	}

	pthread_mutex_unlock(&tlb_mutex);

}

void borrar_dir_tlb(uint32_t numero_marco, uint32_t numero_pagina){
	t_fila_tlb* fila = (t_fila_tlb*)malloc(sizeof(t_fila_tlb));
	fila->numero_marco = numero_marco;
	fila->numero_pagina = numero_pagina;
	fila->pid = obtener_pid();
	pthread_mutex_lock(&tlb_mutex);
	remover_fila_tlb(tlb->filas,fila);
	pthread_mutex_unlock(&tlb_mutex);
	free(fila);
}

uint32_t swap_marco(uint32_t numero_pagina, t_tabla *tabla_prc){
	int tamanio_pagina = config->size_pagina,
		pid = obtener_pid();
	logDebug("Ejecutando algoritmo de reemplazo de pagina");

	t_list* filas = tabla_prc->filas;
	uint32_t marco_elegido;
	t_fila_tabla* fila;
	bool marco_encontrado = false;

	if(config->algoritmo==CLOCK){

		while(!marco_encontrado){
			fila = list_get(filas,tabla_prc->puntero);
			if(fila->accedido){
				fila->accedido = 0;
				tabla_prc->puntero++;
				tabla_prc->puntero %= filas->elements_count;
			} else {
				marco_encontrado = true;
			}
		}
		marco_elegido = fila->numero_marco;
	} else {
		int contador_vuelta = 0;
		while(!marco_encontrado){
			//Paso 1
			while(!marco_encontrado && contador_vuelta < filas->elements_count){
				fila = list_get(filas,tabla_prc->puntero);
				if(!fila->accedido && !fila->modificado){
					marco_encontrado = true;
				} else {
					tabla_prc->puntero++;
					tabla_prc->puntero %= filas->elements_count;
				}
				contador_vuelta++;
			}
			contador_vuelta = 0;
			//Paso 2
			while(!marco_encontrado && contador_vuelta < filas->elements_count){
				fila = list_get(filas,tabla_prc->puntero);
				if(!fila->accedido && fila->modificado){
					marco_encontrado = true;
				} else {
					fila->accedido = 0;
					tabla_prc->puntero++;
					tabla_prc->puntero %= filas->elements_count;
				}
				contador_vuelta++;
			}
			contador_vuelta = 0;
		}
		marco_elegido = fila->numero_marco;
	}

	uint32_t dir_fisica = marco_elegido * tamanio_pagina;

	if(fila->modificado){
		logDebug("Se realiza swap de pagina para PID: %d", fila->numero_pagina, pid);
		escribirPaginaSwap(pid, fila->numero_pagina, tamanio_pagina,memoria_principal.memoria + dir_fisica);
	} else {
		logDebug("No fue necesario realizar swap de pagina %d para PID: %d", fila->numero_pagina, pid);
	}

	borrar_dir_tlb(marco_elegido,fila->numero_pagina);
	borrar_dir_tabla(tabla_prc,marco_elegido);

	return marco_elegido;
}

uint32_t obtener_marco_libre(){

	int i=0;
	char bit_no_encontrado=1;

	while(i < config->cantidad_paginas && bit_no_encontrado){
		if(bitarray_test_bit(memoria_principal.bitmap,i)==0) bit_no_encontrado = 0;
		else i++;
	}

	return bit_no_encontrado ? -1 : i;
}

int agregar_pagina_a_memoria(uint32_t pid, uint32_t numero_pagina, char* pagina){

	int marco_libre;
	t_tabla *tabla_buscada = obtener_tabla(obtener_pid());

	/* Se puede dar que no exista la tabla si se cae un CPU mientras
	 * se esta esperando la respuesta de swap por lo que se devuelve -3
	 */
	if(!tabla_buscada) return -3;

	pthread_mutex_lock(&bitMap_mutex);//lockeo el mutex para buscar un marco libre

	if(tabla_buscada->filas->elements_count < config->marcos_x_proc){
		marco_libre	= obtener_marco_libre();
	} else {
		logDebug("Limite marcos x proceso alcanzado PID: %d",pid);
		marco_libre = -1;
	}

	uint32_t dir_fisica;

	if(marco_libre < 0){

		pthread_mutex_unlock(&bitMap_mutex);//desbloqueo si no se encontro

		if(list_is_empty(tabla_buscada->filas)){
			logDebug("No hay marcos libres ni tiene marcos para hacer swap [PID: %d]",pid);
			return -1;//Si no se encontro marco libre y el proceso no tiene ningun marco asignado para swapear
		}
		marco_libre = swap_marco(numero_pagina,tabla_buscada);	//Si es -1 hago swap, swap no deberia fallar nunca
	} else {
		bitarray_set_bit(memoria_principal.bitmap,marco_libre);
		pthread_mutex_unlock(&bitMap_mutex);//desbloqueo despues de setear en ocupado el marco asignado
	}
	dir_fisica = marco_libre * config->size_pagina;

	logDebug("Se agrega pagina %d en marco %d",numero_pagina, marco_libre);
	memcpy(memoria_principal.memoria+dir_fisica,pagina,config->size_pagina);

	cargar_dir_tabla(numero_pagina, marco_libre);
	return marco_libre;
}



int copiar_pagina_a_memoria(uint32_t numero_pagina){

	uint32_t pid = obtener_pid();
	char* pagina = leerPaginaSwap(pid, numero_pagina);

	if(!pagina) return -1;	//Todo ver de poner un error copado

	int result = agregar_pagina_a_memoria(pid,numero_pagina,pagina);

	free(pagina);
	return result;
}

int obtener_numero_marco(uint32_t numero_pagina){
	uint32_t nro_marco;

	if((nro_marco=obtener_numero_marco_tlb(numero_pagina)) == -1){
		if((nro_marco=obtener_numero_marco_tabla(numero_pagina)) == -1){

			if(copiar_pagina_a_memoria(numero_pagina) < 0) return -1;

			nro_marco=obtener_numero_marco_tabla(numero_pagina);
		}
		copiar_pagina_a_tlb(numero_pagina,nro_marco);

	}
	return nro_marco;
}

t_fila_tabla* get_fila_tabla(uint32_t numero_pagina){
	t_tabla *tabla_buscada;
	t_fila_tabla *fila_buscada;

	bool fila_valida(void* fila){
		return ((t_fila_tabla*) fila)->numero_pagina==numero_pagina;
	}

	tabla_buscada=obtener_tabla(obtener_pid());
	fila_buscada=list_find(tabla_buscada->filas,fila_valida);
	return fila_buscada;
}

void marcar_pagina_accedida(uint32_t numero_pagina){
	t_fila_tabla *fila = get_fila_tabla(numero_pagina);
	fila->accedido=1;
}

void marcar_pagina_modificada(uint32_t numero_pagina){
	t_fila_tabla *fila = get_fila_tabla(numero_pagina);
	fila->modificado=1;
}

void genearar_reporte_memoria(FILE *reporte, uint32_t pid, int screen_print){

	char *buff=NULL, *tiempo;
	t_list *lista_tmp;

	tiempo = temporal_get_string_time();
	log_reporte(reporte,screen_print, "Reporte de memoria - %s\n",tiempo);
	free(tiempo);

	bool mismo_pid (void *tabla){
		return pid ? ((t_tabla *) tabla)->pid == pid : 1;
	}

	void imprimir_memoria_fila(void *fila){
		t_fila_tabla *fila_tmp = (t_fila_tabla*) fila;

		log_reporte(reporte,screen_print,"Marco: %d\n",fila_tmp->numero_marco);
		log_reporte(reporte,screen_print,"Contenido:\n");
		buff = stream_a_string(memoria_principal.memoria + fila_tmp->numero_marco * config->size_pagina, config->size_pagina);
		log_reporte(reporte,screen_print,buff);
		free(buff);

		log_reporte(reporte,screen_print,"\n\n");
	}

	void imprimir_memoria(void *tabla){
		t_tabla *tabla_tmp = (t_tabla*)tabla;

		log_reporte(reporte,screen_print,"Proceso: %d\n",tabla_tmp->pid);
		list_iterate(tabla_tmp->filas,imprimir_memoria_fila);
		log_reporte(reporte,screen_print,"\n");
	}

	lista_tmp=list_filter(tablas_de_paginas,mismo_pid);

	list_iterate(lista_tmp,imprimir_memoria);
	list_destroy(lista_tmp);

}

void generar_reporte_tablas(FILE *reporte, uint32_t pid, int screen_print){
	t_list *lista_tmp;
	char *tiempo = NULL;

		bool mismo_pid (void *tabla){
			return pid ? ((t_tabla *) tabla)->pid == pid : 1;
		}

		void generar_reporte_fila(void *fila){
			t_fila_tabla *fila_tmp = (t_fila_tabla*) fila;
			log_reporte(reporte,screen_print,"┠─────┼──────┼─┼─┨\n");
			log_reporte(reporte,screen_print,"┃%5d|%6d|%d|%d┃\n",fila_tmp->numero_marco,fila_tmp->numero_pagina,fila_tmp->accedido,fila_tmp->modificado);
		}

		void imprimirTabla(void* aux){
			t_tabla* tabla = (t_tabla *) aux;

			log_reporte(reporte,screen_print, "Proceso: %d\n", tabla->pid);
			log_reporte(reporte,screen_print, "Cantidad de paginas en memoria: %d\n",list_size(tabla->filas));
			log_reporte(reporte,screen_print,"┏━━━━━┯━━━━━━┯━┯━┓\n");
			log_reporte(reporte,screen_print,"┃Marco|Pagina|A|M┃\n");
			list_iterate(tabla->filas,generar_reporte_fila);
			log_reporte(reporte,screen_print,"┗━━━━━┷━━━━━━┷━┷━┛\n");
		}

		tiempo = temporal_get_string_time();
		log_reporte(reporte,screen_print, "Reporte de tabla de paginas - %s\n",tiempo);
		free(tiempo);

		lista_tmp=list_filter(tablas_de_paginas,mismo_pid);

		list_iterate(lista_tmp,imprimirTabla);
		list_destroy(lista_tmp);

		log_reporte(reporte,screen_print,"\n");

}

pthread_mutex_t* obtener_mutex_pid(uint32_t pid){
	pthread_mutex_t *mutex;
	char *pid_str = string_itoa(pid);

	pthread_mutex_lock(&pid_mutex);
	mutex = dictionary_get(pids_en_uso,pid_str);
	pthread_mutex_unlock(&pid_mutex);

	free(pid_str);

	return mutex;
}

void lock_pid(uint32_t pid){
	pthread_mutex_t *mutex;

	if((mutex = obtener_mutex_pid(pid))) pthread_mutex_lock(mutex);

}

void unlock_pid(uint32_t pid){
	pthread_mutex_t *mutex;

	if((mutex = obtener_mutex_pid(pid))) pthread_mutex_unlock(mutex);
}

//----------------------------------PUBLICO---------------------------------------

void crearMemoriaPrincipal(int cantidad_paginas, int size_pagina){

	char *bits_bitMap = malloc(sizeof(char)*cantidad_paginas);
	int i;

	logDebug("Creando memoria principal de tamanio %d\n", cantidad_paginas*size_pagina);

	for(i=0;i<cantidad_paginas;i++){
		bits_bitMap[i]=0;
	}

	memoria_principal.memoria = malloc(cantidad_paginas*size_pagina);
	memoria_principal.bitmap = bitarray_create(bits_bitMap,cantidad_paginas);
}

void crear_tabla_de_paginas(uint32_t pid, uint32_t cant_paginas){

	t_tabla *nueva_tabla = malloc(sizeof(t_tabla));

	nueva_tabla->pid = pid;
	nueva_tabla->tamanio = cant_paginas;//TODO esto me parece que no sirve
	nueva_tabla->filas = list_create();
	nueva_tabla->puntero = 0;

	list_add(tablas_de_paginas,(void*) nueva_tabla);

}

void eliminar_tabla_de_paginas(uint32_t pid){

	lock_pid(pid);

	bool tiene_igual_pid (void* elemento){
		return ((t_tabla*) elemento)->pid==pid;
	}

	list_remove_and_destroy_by_condition(tablas_de_paginas,tiene_igual_pid,destructor_tabla);

	unlock_pid(pid);
}

void crear_tlb(uint32_t tamanio){

	tlb = malloc(sizeof(t_tabla_tlb));

	tlb->tamanio=tamanio;
	tlb->filas=list_create();

	logDebug("Nueva TLB creada");
}

void elimina_tlb(t_tabla_tlb *tabla){

	void eliminar_fila(void *fila){
		free(fila);
	}

	list_destroy_and_destroy_elements(tabla->filas, eliminar_fila);
}

int obtener_contenido_memoria(char** contenido, uint32_t numero_pagina, uint32_t offset, uint32_t tamanio){

	int numero_marco,
		dir_fisica;
	memoria mem;

	lock_pid(obtener_pid());

	numero_marco = obtener_numero_marco(numero_pagina);

	//Si no se pudo encontrar la direccion fisica devuelvo el codigo de error
	if(numero_marco<0) return numero_marco;

	 dir_fisica = numero_marco*config->size_pagina;
	 mem = memoria_principal.memoria+offset+dir_fisica;


	//Pueden mandarme un puntero a NULL o un puntero a espacio ya reservado (actualmente solo pasa la primera)
	if(*contenido==NULL) *contenido=malloc(tamanio);

	memcpy(*contenido,mem,tamanio);

	marcar_pagina_accedida(numero_pagina);

	unlock_pid(obtener_pid());
	return tamanio;	//Si salio bien devuelvo el la cantidad de bytes que lei
}

int escribir_contenido_memoria(uint32_t numero_pagina, uint32_t offset, uint32_t tamanio, char* contenido){

	uint32_t numero_marco,
			 dir_fisica;
	memoria mem;

	lock_pid(obtener_pid());

	numero_marco = obtener_numero_marco(numero_pagina);

	//Si no se pudo encontrar la direccion fisica devuelvo el codigo de error
	if(numero_marco < 0) return numero_marco;

	dir_fisica = numero_marco * config->size_pagina;
	mem = memoria_principal.memoria+offset+dir_fisica;

	memcpy(mem,contenido,tamanio);

	marcar_pagina_accedida(numero_pagina);
	marcar_pagina_modificada(numero_pagina);

	unlock_pid(obtener_pid());

	return tamanio;	//Si salio bien devuelvo el la cantidad de bytes que escribi
}

int hayMarcosLibres(){
	int marco_libre = -1;
	pthread_mutex_lock(&bitMap_mutex);
	marco_libre = obtener_marco_libre();
	pthread_mutex_unlock(&bitMap_mutex);
	return marco_libre;
}

void liberar_memoria(uint32_t pid){

	t_tabla *tabla_buscada;

	lock_pid(pid);
	tabla_buscada=obtener_tabla(pid);

	void liberar_memoria_con_fila(void* fila){
		int nro_marco;

		nro_marco=((t_fila_tabla *) fila)->numero_marco;
		bitarray_clean_bit(memoria_principal.bitmap,nro_marco);
	}
	pthread_mutex_lock(&bitMap_mutex);
	list_iterate(tabla_buscada->filas,liberar_memoria_con_fila);
	pthread_mutex_unlock(&bitMap_mutex);

	unlock_pid(pid);
}

void liberar_entradas_tlb(uint32_t pid){

	t_fila_tlb* aux = NULL;

	lock_pid(pid);

	bool tiene_igual_pid (void* elemento){
		return ((t_fila_tlb*) elemento)->pid==pid;
	}
	pthread_mutex_lock(&tlb_mutex);
	while((aux = list_remove_by_condition(tlb->filas,tiene_igual_pid))!=NULL){
		destructor_fila_tlb(aux);
	}
	pthread_mutex_unlock(&tlb_mutex);

	unlock_pid(pid);
}

void flush_tlb(){
	logDebug("Ejecutando Flush TLB");
	//Limpia el contenido de la TLB
	pthread_mutex_lock(&tlb_mutex);
	list_clean_and_destroy_elements(tlb->filas, destructor_fila_tlb);
	pthread_mutex_unlock(&tlb_mutex);
}

void flush_memory(){
	logDebug("Ejecutando Flush Memoria");
	//Marca todas las paginas como modificadas
	void flushFila(void* aux){
		t_fila_tabla* fila = (t_fila_tabla *) aux;
		fila->modificado = 1;
	}

	void flushTabla(void* aux){
		t_tabla* tabla = (t_tabla *) aux;
		list_iterate(tabla->filas,flushFila);
	}

	list_iterate(tablas_de_paginas,flushTabla);
}

void crearListaDeTablas(){

	tablas_de_paginas = list_create();
}

void generar_reporte(FILE *reporte, uint32_t pid, int reporte_memoria, int reporte_tabla, int screen_print){

	//printf("Generando reporte, pid: %d logMemoria: %d, logTabla: %d, scree_print: %d\n",pid, reporte_memoria, reporte_tabla, screen_print);
	if(reporte_memoria) genearar_reporte_memoria(reporte, pid, screen_print);
	if(reporte_tabla) generar_reporte_tablas(reporte, pid, screen_print);
}

void crear_lista_pids_en_uso(){
	pids_en_uso = dictionary_create();
}

void agregar_mutex_pid(uint32_t pid){
	pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
	char *pid_str = string_itoa(pid);

	pthread_mutex_init(mutex,NULL);

	pthread_mutex_lock(&pid_mutex);
	dictionary_put(pids_en_uso,pid_str,mutex);
	pthread_mutex_unlock(&pid_mutex);

	logDebug("Mutex creado para pid: %d",pid);
}

void eliminar_mutex_pid(uint32_t pid){
	char *pid_str = string_itoa(pid);

	void destructor_mutex(void *mutex){
		pthread_mutex_destroy(mutex);
		free(mutex);
	}

	pthread_mutex_lock(&pid_mutex);
	dictionary_remove_and_destroy(pids_en_uso,pid_str,destructor_mutex);
	pthread_mutex_unlock(&pid_mutex);

	free(pid_str);
}
