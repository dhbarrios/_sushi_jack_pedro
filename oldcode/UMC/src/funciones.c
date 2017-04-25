/*
 * funciones.c
 */

#include "funciones.h"
#include "interfazSwap.h"

//----------------------------------PRIVADO---------------------------------------

static pthread_key_t key_pid;

//----------------------------------PUBLICO---------------------------------------

void handleClients(){


	int socketServidor,							//Descriptor del socket servidor
		socket_cliente;							//Se usa para recivir al conexion y se envia al thread que la va a manejar
	char *tmp_str;

	pthread_t thread_tmp;						//Variable Temporal para crear threads;
	t_arg_thread_cpu* arg_thread_cpu;			//Arguemntos para el thread del nuevo cpu
	t_arg_thread_nucleo* arg_thread_nucleo;		//Argumentos para el thread del nuevo Nucleo
	pthread_attr_t thread_detached_attr;		//Atributos para crear socket detached

	Package* package;

	//Mutex para comunicacion con swap
	pthread_mutex_init(&comunicacion_swap_mutex,NULL);
	pthread_mutex_init(&socket_swap_mutex,NULL);

	//Mutex para bitmaps de memoria
	pthread_mutex_init(&bitMap_mutex,NULL);
	pthread_mutex_init(&activo_mutex,NULL);
	pthread_mutex_init(&modificacion_mutex,NULL);

	pthread_mutex_init(&pid_mutex,NULL);

	//Completo los atributos de thread para que sea detached. Se va a usar para los thread de CPU
	pthread_attr_init(&thread_detached_attr);
	pthread_attr_setdetachstate(&thread_detached_attr,PTHREAD_CREATE_DETACHED);

	//Creo la key para los pids de cpus
	pthread_key_create(&key_pid, NULL);

	/* Se abre el socket servidor, avisando por pantalla y saliendo si hay
	 * algún problema */
	socketServidor = abrirSocketInetServer(config->ip_umc,config->puerto_umc);
	if (socketServidor == -1)
	{
		perror ("Error al abrir servidor");
		exit (-1);
	}

	//Conecto el socket swap
	conectarConSwap();

	crear_lista_pids_en_uso();

	/* Bucle infinito.
	 * Se atiende a si hay más clientes para conectar y a los mensajes enviados
	 * por los clientes ya conectados
	 */
	while (1){

		//Acepto las conexiones y las mando a diferentes threads
		socket_cliente = aceptarConexionCliente(socketServidor);

		//Comienzo el handshake, enviando el tamanio de pagina
		tmp_str = string_itoa(config->size_pagina);
		enviarMensajeSocket(socket_cliente,HANDSHAKE_UMC, tmp_str);
		free(tmp_str);

		package = createPackage();
		//Espero respuesta y creo thread correspondiente
		if(recieve_and_deserialize(package,socket_cliente) > 0){
			logDebug("Mensaje recibido de %d",socket_cliente);
			switch(package->msgCode){
				case -1:
					logDebug("Se desconeccto el cliente %d",socket_cliente);
					break;
				case HANDSHAKE_CPU:
					logDebug("Cliente %d es un CPU",socket_cliente);

					//Se borra al final del handleCPU.
					//Argumentos para el thread de la CPU. Mando una estructura porque es mas facil de modificar en un futuro.
					arg_thread_cpu = malloc(sizeof(t_arg_thread_cpu));
					arg_thread_cpu->socket_cpu=socket_cliente;

					/*TODO Entiendo para este punto deberia estar conectado el nucleo, sino no deberia poder
					 * conectar CPU's. ¿O no es necesario?
					 */
					pthread_create(&thread_tmp,&thread_detached_attr,(void*) handle_cpu,(void*) arg_thread_cpu);
					break;

				case HANDSHAKE_NUCLEO:
					logDebug("Cliente %d es un Nucleo",socket_cliente);
					//Se borra al final del handleNucleo.
					arg_thread_nucleo = malloc(sizeof(t_arg_thread_nucleo));
					arg_thread_nucleo->socket_nucleo=socket_cliente;
					pthread_create(&thread_tmp,NULL,(void*) handleNucleo,(void*) arg_thread_nucleo);
					break;

				default:
					logDebug("El cliente %d no se identifico",socket_cliente);
					close(socket_cliente);
			}
		}
		destroyPackage(package);
	}

	pthread_key_delete(key_pid);
	pthread_attr_destroy(&thread_detached_attr);
}

int conectarConSwap(){

	int socket;		/* descriptor de conexión con el servidor */
	int buffer;		/* buffer de lectura de datos procedentes del servidor */
	int error;		/* error de lectura por el socket */

	pthread_mutex_lock(&socket_swap_mutex);

	/* Se abre una conexión con el servidor */
	socket = abrirConexionInetConServer(config->ip_swap, config->puerto_swap);

	/* Se lee el número de cliente, dato que nos da el servidor.*/
	error = leerSocketClient(socket, (char *)&buffer, sizeof(int));

	/* Si ha habido error de lectura lo indicamos y salimos */
	if (error < 1)
		logDebug("SWAP se encuentra desconectada.");
	else {
		logDebug("Conexion con SWAP satisfactoria.");
		socket_swap=socket;
	}

	pthread_mutex_unlock(&socket_swap_mutex);
	return socket;
}

void handle_cpu(t_arg_thread_cpu* argumentos){
	int sigue = 1,								//Flag para el while, cuando muere el socket se pasa a 0
		*socket_cpu = &argumentos->socket_cpu,	//Lo guardo en variables para que sea mas comodo de usar
		result;
	Package *package_receive;
	char* contenido_lectura=NULL, *result_serializado=NULL;

	crear_key_pid();

	while(sigue){
		package_receive = createPackage();
		if(recieve_and_deserialize(package_receive,*socket_cpu) > 0){
			logDebug("CPU envía [message code]: %d", package_receive->msgCode);

			switch(package_receive->msgCode){

				case SOLICITAR_BYTES_PAGINA:

					logDebug("Se ha solicitado la lectura de Bytes en pagina.");

					contenido_lectura=NULL;
					result = leer_pagina(package_receive->message,&contenido_lectura);

					if(*socket_cpu > 0){
						//Si la operacion salio bien result es el tamanio de contenido leido
						if(result > 0){

							//Creo un buffer y serializo el resultado + el contenido leido
							result_serializado=(char*)malloc(sizeof(uint32_t)+result);
							memcpy(result_serializado,&result,sizeof(uint32_t));
							memcpy(result_serializado+sizeof(uint32_t),contenido_lectura,result);

							enviarMensajeSocketConLongitud(*socket_cpu,RESULTADO_OPERACION,result_serializado,sizeof(uint32_t)+result);
							free(result_serializado);

						}else
							enviarMensajeSocketConLongitud(*socket_cpu,RESULTADO_OPERACION,(char*)&result,sizeof(uint32_t));

					}
						free(contenido_lectura);

					break;

				case ALMACENAR_BYTES_PAGINA:

					logDebug("Se ha solicitado la escritura de Bytes en pagina.");

					result = escribir_pagina(package_receive->message);
					if(*socket_cpu > 0) enviarMensajeSocketConLongitud(*socket_cpu,RESULTADO_OPERACION,(char*)&result,sizeof(uint32_t));

					break;

				case SWITCH_PROCESS:

					logDebug("CPU ha solicitado el cambio de Proceso");
					nuevo_pid(package_receive->message);

			}

		} else {
			//Si el cliente cerro la conexion se termino el thread
			sigue=0;
			logInfo("CPU ha cerrado la conexión, cerrando thread");
		}
		destroyPackage(package_receive);
	}
	logInfo("Fin thread CPU pid %d",obtener_pid());
	free(argumentos);
	borrar_key_pid();
}

void crear_key_pid(){
	uint32_t *pid=malloc(sizeof(uint32_t));
	pthread_setspecific(key_pid,(void*) pid);
}

void borrar_key_pid(){
	free(pthread_getspecific(key_pid));
}

void setear_pid(uint32_t pid){
	uint32_t *p_pid;

	p_pid=pthread_getspecific(key_pid);
	*p_pid=pid;
}

uint32_t obtener_pid(){
	uint32_t *pid;
	pid=pthread_getspecific(key_pid);

	if(pid) return *pid;

	return 0;
}

void handleNucleo(t_arg_thread_nucleo* args){
	int sigue = 1,
		*socket_nucleo = &args->socket_nucleo,	//Lo guardo en variables para que sea mas comodo de usar
		result;
	Package* package;

	while(sigue){
		package = createPackage();
		if(recieve_and_deserialize(package,*socket_nucleo) > 0){
			logDebug("Nucleo envía [message code]: %d", package->msgCode);

			switch(package->msgCode){

				case INIT_PROGRAM:

					logDebug("Se ha solicitado la inicializacion de un nuevo programa.");

					result = inicializar_programa(package->message);
					enviarMensajeSocketConLongitud(*socket_nucleo,RESULTADO_OPERACION,(char*)&result,sizeof(uint32_t));

					break;

				case END_PROGRAM:
					logDebug("Se ha solicitado la finalizacion de un programa.");

					result = finalizar_programa(package->message);
					enviarMensajeSocketConLongitud(*socket_nucleo,RESULTADO_OPERACION,(char*)&result,sizeof(uint32_t));

			}

		} else {
			//Si el cliente cerro la conexion se termino el thread
			sigue=0;
			logInfo("Nucleo ha cerrado la conexión, cerrando thread");
		}
		destroyPackage(package);
	}
	logInfo("Fin thread Nucleo");
	free(args);
}

void inicializarUMC(){

	logDebug("Inicializando la UMC");

	crear_tlb(config->tamanio_tlb);
	crearMemoriaPrincipal(config->cantidad_paginas, config->size_pagina);
	crearListaDeTablas();
}

void ejecutarRetardoMemoria(){
	logDebug("Ejecutando retardo de %d ms", config->retraso);
	pthread_mutex_lock(&retardo_mutex);
	usleep(config->retraso * 1000);
	pthread_mutex_unlock(&retardo_mutex);
	logDebug("Fin del retardo");
}

