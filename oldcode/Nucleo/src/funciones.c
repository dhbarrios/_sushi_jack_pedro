/*
 * funciones.c
 *
 *  Created on: 21/04/2016
 *      Author: hernan
 */

#include "Nucleo.h"
#include "planificador.h"
#include "PCB.h"

void handleClients(Configuration* config){

	arg_struct args;
	args.config = config;
	args.listaCPUs = list_create();

	//inicializo variables globales de sockets
	socketUMC = -1;
	socketPlanificador = -1;

	inicializarArraySockets(&args);
	inicializarSemaforos();
	inicializarVariablesCompartidas();
	inicializarEstados();

	pthread_mutex_init(&quantum_mutex,NULL);
	setQuantum(config->quantum);
	pthread_mutex_init(&quantum_sleep_mutex,NULL);
	setQuantumSleep(config->quantum_sleep);


	//abrir server para escuchar CPUs
	args.socketServerCPU = abrirSocketInetServer(config->ip_nucleo,config->puerto_nucleo_cpu);
	if (args.socketServerCPU == -1)
	{
		perror ("Error al abrir servidor para CPUs");
		exit (-1);
	}
	//abrir server para escuchar Consolas
	args.socketServerConsola = abrirSocketInetServer(config->ip_nucleo,config->puerto_nucleo_prog);
	if (args.socketServerConsola == -1)
	{
		perror ("Error al abrir servidor para Consolas");
		exit (-1);
	}
	//abrir server para escuchar mensajes enviados por los Threads anteriores
	args.socketServerPlanificador = abrirSocketInetServer(PLANIFICADOR_IP,PLANIFICADOR_PORT);
	if (args.socketServerPlanificador == -1)
	{
		perror ("Error al abrir servidor para Threads (Planificador)");
		exit (-1);
	}

	pthread_t hilo1;
	pthread_create(&hilo1,NULL,(void*)handleConsolas,(void *)&args);
	pthread_t hilo2;
	pthread_create(&hilo2,NULL,(void*)handleCPUs,(void *)&args);
	pthread_t hilo3;
	pthread_create(&hilo3,NULL,(void*)planificar,(void *)&args);
	pthread_t hilo4;
	pthread_create(&hilo4,NULL,(void*)handleInotify,NULL);

	pthread_join(hilo1,NULL);
	pthread_join(hilo2,NULL);
	pthread_join(hilo3,NULL);
	pthread_join(hilo4,NULL);
}

void handleConsolas(void* arguments){
	arg_struct *args = arguments;
	int socketServidor;				/* Descriptor del socket servidor */
	int *socketCliente = args->consolaSockets;/* Descriptores de sockets con clientes */
	int numeroClientes = 0;			/* Número clientes conectados */
	fd_set descriptoresLectura;	/* Descriptores de interes para select() */
	//int buffer;							/* Buffer para leer de los socket */
	int maximo;							/* Número de descriptor más grande */
	int i;								/* Para bubles */

	socketServidor = args->socketServerConsola;

	/* Bucle infinito.
	 * Se atiende a si hay más clientes para conectar y a los mensajes enviados
	 * por los clientes ya conectados */
	while (1)
	{
		if(socketUMC==-1){
			socketUMC = conectarConUMC(args->config);
		}
		if(socketPlanificador==-1){
			socketPlanificador = conectarConPlanificador(PLANIFICADOR_IP,PLANIFICADOR_PORT);
			if(socketPlanificador!=-1){
				launch_IO_threads();
			}
		}
		/* Cuando un cliente cierre la conexión, se pondrá un -1 en su descriptor
		 * de socket dentro del array socketCliente. La función compactaClaves()
		 * eliminará dichos -1 de la tabla, haciéndola más pequeña.
		 *
		 * Se eliminan todos los clientes que hayan cerrado la conexión */
		compactaClaves (socketCliente, &numeroClientes);

		/* Se inicializa descriptoresLectura */
		FD_ZERO (&descriptoresLectura);

		/* Se añade para select() el socket servidor */
		FD_SET (socketServidor, &descriptoresLectura);

		/* Se añaden para select() los sockets con los clientes ya conectados */
		for (i=0; i<numeroClientes; i++)
			FD_SET (socketCliente[i], &descriptoresLectura);

		/* Se el valor del descriptor más grande. Si no hay ningún cliente,
		 * devolverá 0 */
		maximo = dameMaximo (socketCliente, numeroClientes);

		if (maximo < socketServidor)
			maximo = socketServidor;

		/* Espera indefinida hasta que alguno de los descriptores tenga algo
		 * que decir: un nuevo cliente o un cliente ya conectado que envía un
		 * mensaje */
		select (maximo + 1, &descriptoresLectura, NULL, NULL, NULL);
		/* Se comprueba si algún cliente ya conectado ha enviado algo */
		for (i=0; i<numeroClientes; i++)
		{
			if (FD_ISSET (socketCliente[i], &descriptoresLectura))
			{
				Package* package = createPackage();
				/* Se lee lo enviado por el cliente y se escribe en pantalla */
				if(recieve_and_deserialize(package,socketCliente[i]) > 0){
					//logDebug("Consola %d envía [message code]: %d, [Mensaje]: %s", i+1, package->msgCode, package->message);
					if(package->msgCode==NEW_ANSISOP_PROGRAM){
						logDebug("Consola %d solicito el inicio de un nuevo programa.",i+1);
						iniciarPrograma(socketCliente[i],package->message);
					}

				}
				else
				{
					/* Se indica que el cliente ha cerrado la conexión y se
					 * marca con -1 el descriptor para que compactaClaves() lo
					 * elimine */
					logInfo("Consola %d ha cerrado la conexión.", i+1);
					abortarPrograma(socketCliente[i]);
					socketCliente[i] = -1;
				}
				destroyPackage(package);
			}
		}

		/* Se comprueba si algún cliente nuevo desea conectarse y se le
		 * admite */
		if (FD_ISSET (socketServidor, &descriptoresLectura)){
			nuevoCliente (socketServidor, socketCliente, &numeroClientes, MAX_CONSOLAS);
		}

	}
}

void handleCPUs(void* arguments){
	arg_struct *args = arguments;
	t_list* listaCPUs = args->listaCPUs;
	int socketServidor;				/* Descriptor del socket servidor */
	int *socketCliente = args->cpuSockets;/* Descriptores de sockets con clientes */
	int numeroClientes = 0;			/* Número clientes conectados */
	fd_set descriptoresLectura;	/* Descriptores de interes para select() */
	//int buffer;							/* Buffer para leer de los socket */
	int maximo;							/* Número de descriptor más grande */
	int i;								/* Para bubles */

	socketServidor = args->socketServerCPU;

	/* Bucle infinito.
	 * Se atiende a si hay más clientes para conectar y a los mensajes enviados
	 * por los clientes ya conectados */
	while (1)
	{
		/* Cuando un cliente cierre la conexión, se pondrá un -1 en su descriptor
		 * de socket dentro del array socketCliente. La función compactaClaves()
		 * eliminará dichos -1 de la tabla, haciéndola más pequeña.
		 *
		 * Se eliminan todos los clientes que hayan cerrado la conexión */
		compactaClaves (socketCliente, &numeroClientes);

		/* Se inicializa descriptoresLectura */
		FD_ZERO (&descriptoresLectura);

		/* Se añade para select() el socket servidor */
		FD_SET (socketServidor, &descriptoresLectura);

		/* Se añaden para select() los sockets con los clientes ya conectados */
		for (i=0; i<numeroClientes; i++)
			FD_SET (socketCliente[i], &descriptoresLectura);

		/* Se el valor del descriptor más grande. Si no hay ningún cliente,
		 * devolverá 0 */
		maximo = dameMaximo (socketCliente, numeroClientes);

		if (maximo < socketServidor)
			maximo = socketServidor;

		/* Espera indefinida hasta que alguno de los descriptores tenga algo
		 * que decir: un nuevo cliente o un cliente ya conectado que envía un
		 * mensaje */
		select (maximo + 1, &descriptoresLectura, NULL, NULL, NULL);

		/* Se comprueba si algún cliente ya conectado ha enviado algo */
		for (i=0; i<numeroClientes; i++)
		{
			if (FD_ISSET (socketCliente[i], &descriptoresLectura))
			{
				Package* package = createPackage();
				/* Se lee lo enviado por el cliente y se escribe en pantalla */
				//if ((leerSocket (socketCliente[i], (char *)&buffer, sizeof(int)) > 0))
				if(recieve_and_deserialize(package,socketCliente[i]) > 0){
					//logDebug("CPU %d envía [message code]: %d, [Mensaje]: %s", i+1, package->msgCode, package->message);
					analizarMensajeCPU(socketCliente[i],package,args);
				}
				else
				{
					/* Se indica que el cliente ha cerrado la conexión y se
					 * marca con -1 el descriptor para que compactaClaves() lo
					 * elimine */
					logInfo("CPU %d ha cerrado la conexión", i+1);
					eliminarCPU(listaCPUs,socketCliente[i]);
					socketCliente[i] = -1;
				}
				destroyPackage(package);
			}
		}

		/* Se comprueba si algún cliente nuevo desea conectarse y se le
		 * admite */
		if (FD_ISSET (socketServidor, &descriptoresLectura)){
			int numAnterior = numeroClientes;
			nuevoCliente (socketServidor, socketCliente, &numeroClientes, MAX_CPUS);
			if(numeroClientes > numAnterior){//nuevo CPU aceptado
				nuevoCPU(listaCPUs,socketCliente[numeroClientes-1]);
			}
		}
	}
}

int elegirRandomCPU(int cpuSockets[]){
	srand(time(NULL));
	int aux[MAX_CPUS];
	int i,j=0;
	for(i=0;i<MAX_CPUS;i++){
		if(cpuSockets[i]!=-1){
			aux[j]=cpuSockets[i];
			j++;
		}
	}
	if(j>0){//para evitar la division por 0 si no hay CPUs
		int randNum = rand() % j;
		return aux[randNum];
	} else {
		return -1;
	}
}

void comunicarCPU(int cpuSockets[]){
	int socketCPU = elegirRandomCPU(cpuSockets);
	if(socketCPU!=-1){
		enviarMensajeSocket(socketCPU,NEW_ANSISOP_PROGRAM,"3000");
	}
}

void imprimirArraySockets(int sockets[], int len){
	int i;
	printf("Sockets -> ");
	for(i=0;i<len;i++){
		printf("%d,",sockets[i]);
	}
	printf("\n");
}

void inicializarArraySockets(arg_struct* args){
	int i;
	for(i=0;i<MAX_CONSOLAS;i++){
		args->consolaSockets[i]=-1;
	}
	for(i=0;i<MAX_CPUS;i++){
		args->cpuSockets[i]=-1;
	}
}

int conectarConUMC(Configuration* config){

	int socket;		/* descriptor de conexión con el servidor */
	Package *package;

	/* Se abre una conexión con el servidor */
	socket = abrirConexionInetConServer(config->ip_umc, config->puerto_umc);
	if (socket < 1) {
		logDebug("UMC se encuentra desconectada.");
		return -1;
	} else {
		logDebug("Conexion con UMC satisfactoria.");
	}

	//Se espera el handshake de la UMC para confirmar conexion
	package = createPackage();
	if(recieve_and_deserialize(package, socket) > 0) {
		if(package->msgCode==HANDSHAKE_UMC){
			config->size_pagina = atoi(package->message);//recibo el tamanio de pagina
			logDebug("Conexion con UMC confirmada, tamanio de pagina: %d",config->size_pagina);
		}
	}
	destroyPackage(package);

	//Le aviso a la UMC que soy un nucleo
	enviarMensajeSocket(socket,HANDSHAKE_NUCLEO,"");

	int* socket_umc = malloc(sizeof(int));
	*socket_umc = socket;
	definir_socket_umc(socket_umc);

	return socket;
}

void nuevoCPU(t_list* listaCPUs, int socketCPU){
	CPU* nuevo = malloc(sizeof(CPU));
	nuevo->cpuFD = socketCPU;
	list_add(listaCPUs,nuevo);
	logTrace("Creado nuevo CPU: %d", socketCPU);
	char* tmp_str = string_itoa(config->stack_size);
	enviarMensajeSocket(socketCPU,HANDSHAKE_CPU_NUCLEO, tmp_str);
	free(tmp_str);
	liberarCPU(nuevo);
}

void destroyCPU(CPU* self){
	free(self);
}

//si se tiene el CPU
void liberarCPU(CPU* cpu){
	cpu->libre = 1;	//true
	cpu->pid = 0;
	logTrace("Informando Planificador(%d) [CPU LIBRE]",socketPlanificador);
	informarPlanificador(CPU_LIBRE,cpu->cpuFD);
}

//si solo se tiene el socket del CPU
void liberarCPUporSocketFD(int socketCPU, arg_struct *args){
	CPU* cpu = buscarCPUporSocketFD(socketCPU,args->listaCPUs);
	if(cpu!=NULL){
		liberarCPU(cpu);
	}
}

CPU* buscarCPUporSocketFD(int socketCPU, t_list* listaCPUs){
	CPU* cpu;
	int i;
	for(i=0; i<listaCPUs->elements_count; i++){
		cpu = list_get(listaCPUs,i);
		if(cpu->cpuFD==socketCPU){
			return cpu;
		}
	}
	return NULL;
}

void eliminarCPU(t_list* listaCPUs,int socketCPU){

	int i;
	CPU* aEliminar = NULL;
	for(i=0;i<listaCPUs->elements_count;i++){
		aEliminar = list_get(listaCPUs,i);
		if(aEliminar->cpuFD==socketCPU){
			abortExecutingProcess(aEliminar);
			list_remove_and_destroy_element(listaCPUs,i,(void*)destroyCPU);
		}
	}
}

int conectarConPlanificador(char* ip, int puerto){

	int socket;		/* descriptor de conexión con el servidor */
	int buffer;		/* buffer de lectura de datos procedentes del servidor */
	int error;		/* error de lectura por el socket */
	/* Se abre una conexión con el servidor */
	socket = abrirConexionInetConServer(ip, puerto);
	/* Se lee el número de cliente, dato que nos da el servidor.*/
	error = leerSocketClient(socket, (char *)&buffer, sizeof(int));

	/* Si ha habido error de lectura lo indicamos y salimos */
	if (error < 1)
	{
		logDebug("Planificador se encuentra desconectado.");
	} else {
		logDebug("Conexion con Planificador satisfactoria.");
	}
	return socket;
}

void analizarMensajeCPU(int socketCPU , Package* package, arg_struct *args){
	if(package->msgCode==EXECUTION_FINISHED){
		logTrace("CPU %d me informa que finalizo de ejecutar la instruccion",socketCPU);
		logTrace("Enviando al CPU la orden de Quantum Sleep");
		char* tmp = string_itoa(getQuantumSleep());
		enviarMensajeSocket(socketCPU,QUANTUM_SLEEP_CPU,tmp);
		free(tmp);
	} else if(package->msgCode==QUANTUM_FINISHED){
		logTrace("CPU %d informa que finalizo 1 Quantum",socketCPU);
		quantumFinishedCallback(atoi(package->message),getQuantum(),socketCPU);
	} else if(package->msgCode==PROGRAM_FINISHED){
		liberarCPUporSocketFD(socketCPU,args);
		PCB* pcbActualizado = deserializar_PCB(package->message);
		int socketConsola = getFromEXEC(pcbActualizado->processID)->consolaFD;
		finalizarPrograma(pcbActualizado,socketCPU);
		borrarSocketConsola(args,socketConsola);
	} else if(package->msgCode==CONTEXT_SWITCH_FINISHED){
		liberarCPUporSocketFD(socketCPU,args);
		PCB* pcbActualizado = deserializar_PCB(package->message);
		contextSwitchFinishedCallback(pcbActualizado);
		destroyPCB(pcbActualizado);
	} else if(package->msgCode==CPU_LIBRE){
		logTrace("CPU %d informa que esta Libre",socketCPU);
		liberarCPUporSocketFD(socketCPU,args);
	} else if(package->msgCode==STACK_OVERFLOW_EXCEPTION){
		liberarCPUporSocketFD(socketCPU,args);
		PCB* pcb = removeFromEXEC(atoi(package->message));
		if(pcb!=NULL){
			finalizarProgramaException(pcb,STACK_OVERFLOW_EXCEPTION_MESSAGE);
		}
	} else if(package->msgCode==GENERIC_EXCEPTION){
		liberarCPUporSocketFD(socketCPU,args);
		PCB* pcb = removeFromEXEC(atoi(package->message));
		if(pcb!=NULL){
			finalizarProgramaException(pcb,GENERIC_EXCEPTION_MESSAGE);
		}
	} else if(package->msgCode==EXEC_IO_OPERATION){
		logTrace("Solicitada operacion I/O");
		solicitud_io* solicitud = deserializar_ejecutarOperacionIO(package->message);
		atenderSolicitudDispositivoIO(solicitud);
	} else if(package->msgCode==PRINT_VARIABLE){
		print_var* print = deserializar_imprimirVariable(package->message);
		int socketConsola = getFromEXEC(print->pid)->consolaFD;
		char* serialized = serializar_imprimirVariable_consola(print->valor);
		enviarMensajeSocketConLongitud(socketConsola,PRINT_VARIABLE,serialized,sizeof(uint32_t));
		destroy_print_var(print);
		free(serialized);
	} else if(package->msgCode==PRINT_TEXT){
		print_text* print = deserializar_imprimirTexto(package->message);
		int socketConsola = getFromEXEC(print->pid)->consolaFD;
		enviarMensajeSocket(socketConsola,PRINT_TEXT,print->text);
		destroy_print_text(print);
	} else if(package->msgCode==CPU_SIGNAL_DISCONNECTED){
		PCB* pcbActualizado = deserializar_PCB(package->message);
		contextSwitchFinishedCallback(pcbActualizado);
		destroyPCB(pcbActualizado);
	} else if(package->msgCode==GET_SHARED_VAR){
		int valor = getValorVariableCompartida(package->message);
		informarValorVariableCompartida(socketCPU,valor);
	} else if(package->msgCode==SET_SHARED_VAR){
		shared_var* var = deserializar_shared_var(package->message);
		setValorVariableCompartida(var->var_id,var->value);
		destroy_shared_var(var);
	} else if(package->msgCode==SEM_WAIT){
		sem_action* action = deserializar_semaforo(package->message);
		if(execute_wait(action->sem_id)){
			PCB* pcbActualizado = informarCPUbloqueoSemaforo(socketCPU);
			PCB* pcbLocal = removeFromEXEC(action->pid);
			actualizarPCB(pcbLocal,pcbActualizado);
			reiniciarQuantumsEjecutados(pcbLocal);
			bloquearEnSemaforo(pcbLocal,action->sem_id);
			destroyPCB(pcbActualizado);
		} else {
			continuarEjecucionProcesoCPU(socketCPU);
		}
		destroy_sem_action(action);
	} else if(package->msgCode==SEM_SIGNAL){
		sem_action* action = deserializar_semaforo(package->message);
		execute_signal(action->sem_id);
		destroy_sem_action(action);
	}
}

int getSocketUMC(){
	return socketUMC;
}

void borrarSocketConsola(arg_struct *args, int socketConsola){
	int i;
	for(i=0; i<MAX_CONSOLAS; i++){
		if(args->consolaSockets[i]==socketConsola){
			args->consolaSockets[i]=-1;
		}
	}
}

void setQuantum(int valor){
	pthread_mutex_lock(&quantum_mutex);
	quantum = valor;
	logInfo("Modificado Quantum: %d",valor);
	pthread_mutex_unlock(&quantum_mutex);
}

int getQuantum(){
	int valor;
	pthread_mutex_lock(&quantum_mutex);
	valor = quantum;
	pthread_mutex_unlock(&quantum_mutex);
	return valor;
}

void setQuantumSleep(int valor){
	pthread_mutex_lock(&quantum_sleep_mutex);
	quantum_sleep = valor;
	logInfo("Modificado Quantum Sleep: %d ms",valor);
	pthread_mutex_unlock(&quantum_sleep_mutex);
}

int getQuantumSleep(){
	int valor;
	pthread_mutex_lock(&quantum_sleep_mutex);
	valor = quantum_sleep;
	pthread_mutex_unlock(&quantum_sleep_mutex);
	return valor;
}

void handleInotify(void* arguments){

	char buffer[BUF_LEN];

	// Al inicializar inotify este nos devuelve un descriptor de archivo
	int file_descriptor = inotify_init();
	if (file_descriptor < 0) {
		perror("inotify_init");
	}

	// Creamos un monitor sobre un path indicando que eventos queremos escuchar
	int watch_descriptor = inotify_add_watch(file_descriptor, config_dir, IN_MODIFY);

	// El file descriptor creado por inotify, es el que recibe la información sobre los eventos ocurridos
	// para leer esta información el descriptor se lee como si fuera un archivo comun y corriente pero
	// la diferencia esta en que lo que leemos no es el contenido de un archivo sino la información
	// referente a los eventos ocurridos
	while(1){
		int length = read(file_descriptor, buffer, BUF_LEN);
		if (length < 0) {
			perror("read");
		}

		int offset = 0;

		// Luego del read buffer es un array de n posiciones donde cada posición contiene
		// un eventos ( inotify_event ) junto con el nombre de este.
		while (offset < length) {

			// El buffer es de tipo array de char, o array de bytes. Esto es porque como los
			// nombres pueden tener nombres mas cortos que 24 caracteres el tamaño va a ser menor
			// a sizeof( struct inotify_event ) + 24.
			struct inotify_event *event = (struct inotify_event *) &buffer[offset];

			// El campo "len" nos indica la longitud del tamaño del nombre
			if (event->len) {
				// Dentro de "mask" tenemos el evento que ocurrio y sobre donde ocurrio
				// sea un archivo o un directorio
				if (event->mask & IN_MODIFY) {
					if (event->mask & IN_ISDIR) {
						//printf("The directory %s was modified.\n", event->name);
					} else {
						if(string_equals_ignore_case(event->name,config_file_name)){
							t_config* tConfig = config_create(config_file_name);
							if(tConfig!=NULL){
								char* valor = config_get_string_value(tConfig,QUANTUM);
								if(valor!=NULL){
									setQuantum(atoi(valor));
								}
								char* valor2 = config_get_string_value(tConfig,QUANTUM_SLEEP);
								if(valor2!=NULL){
									setQuantumSleep(atoi(valor2));
								}
								free(tConfig);
							}
						}
					}
				}
			}
			offset += sizeof (struct inotify_event) + event->len;
		}
	}


	inotify_rm_watch(file_descriptor, watch_descriptor);
	close(file_descriptor);

}

void finalizarProgramaException(PCB* pcb, char* mensaje){
	logDebug("%s en Proceso %d",mensaje,pcb->processID);
	if(pcb->consolaActiva){
		logDebug("Informando Consola %d: %s",pcb->consolaFD,mensaje);
		enviarMensajeSocket(pcb->consolaFD,GENERIC_EXCEPTION,mensaje);
	}
	int resultado = finalizar_programa(pcb->processID);
	logTrace("Solicitar UMC finalizar el programa PID:%d, resultado:%d",pcb->processID,resultado);
	logTrace("Destruyendo PCB [PID:%d]",pcb->processID);
	destroyPCB(pcb);
}

