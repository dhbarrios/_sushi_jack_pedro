/*
 * funciones.c
 *
 *  Created on: 21/04/2016
 *      Author: hernan
 */

#include "CPU.h"
#include "configuration.h"
#include "primitivas.h"

AnSISOP_funciones functions = {
		.AnSISOP_definirVariable		= ml_definirVariable,
		.AnSISOP_obtenerPosicionVariable= ml_obtenerPosicionVariable,
		.AnSISOP_dereferenciar			= ml_dereferenciar,
		.AnSISOP_asignar				= ml_asignar,
		.AnSISOP_obtenerValorCompartida = ml_obtenerValorCompartida,
		.AnSISOP_asignarValorCompartida = ml_asignarValorCompartida,
		.AnSISOP_irAlLabel				= ml_irAlLabel,
		.AnSISOP_llamarSinRetorno		= ml_llamarSinRetorno,
		.AnSISOP_llamarConRetorno		= ml_llamarConRetorno,
		.AnSISOP_finalizar				= ml_finalizar,
		.AnSISOP_retornar				= ml_retornar,
		.AnSISOP_imprimir				= ml_imprimir,
		.AnSISOP_imprimirTexto			= ml_imprimirTexto,
		.AnSISOP_entradaSalida			= ml_entradaSalida,
};

AnSISOP_kernel kernel_functions = {
		.AnSISOP_wait	= ml_wait,
		.AnSISOP_signal = ml_signal,
};



void conectarConUMC(void* arguments){
	arg_struct *args = arguments;
	int socket;		/* descriptor de conexión con el servidor */
	Package* package;

	/* Se abre una conexión con el servidor */
	socket = abrirConexionInetConServer(args->config->ip_umc, args->config->puerto_umc);
	if (socket < 1){
		logError("Me han cerrado la conexión.");
		exit(-1);
	}

	socketUMC = socket;

	logDebug("Realizando handshake con UMC");
	package = createPackage();
	if(recieve_and_deserialize(package, socket) > 0) {
		if(package->msgCode==HANDSHAKE_UMC){
			if(package->msgCode==HANDSHAKE_UMC){
				size_pagina = atoi(package->message);//recibo el tamanio de pagina
				logDebug("Conexion con UMC confirmada, tamanio de pagina: %d",size_pagina);
			}
		}
	}
	destroyPackage(package);

	//Le aviso a la UMC que soy un nucleo
	enviarMensajeSocket(socket,HANDSHAKE_CPU,"");
	logDebug("Handshake con UMC exitoso!!");

	definir_socket_umc(&socketUMC);
}

void iniciarEjecucionCPU(void* arguments){
	arg_struct *args = arguments;
	int buffer;		/* buffer de lectura de datos procedentes del servidor */
	int error;		/* error de lectura por el socket */

	conectarConUMC(args);

	//inicializacion de flags
	proceso_fue_bloqueado = 0;
	programa_finalizado = 0;
	end_signal_received = 0;
	end_cpu = 0;
	esperando_mensaje = 0;
	hubo_exception = 0;

	/* Se abre una conexión con el servidor */
	socketNucleo = abrirConexionInetConServer(args->config->ip_nucleo, args->config->puerto_nucleo);

	/* Se lee el número de cliente, dato que nos da el servidor. Se escribe
	 * dicho número en pantalla.*/
	error = leerSocketClient(socketNucleo, (char *)&buffer, sizeof(int));

	/* Si ha habido error de lectura lo indicamos y salimos */
	if (error < 1)
	{
		logError("Me han cerrado la conexión.");
		exit(-1);
	}

	logDebug("Realizando handshake con Nucleo");
	Package* package = createPackage();
	if(recieve_and_deserialize(package, socketNucleo) > 0) {
		if(package->msgCode==HANDSHAKE_CPU_NUCLEO){
			if(package->msgCode==HANDSHAKE_CPU_NUCLEO){
				size_stack = atoi(package->message);//recibo el tamanio de stack
				logDebug("Conexion con Nucleo confirmada, tamanio de stack: %d",size_stack);
			}
		}
	}
	destroyPackage(package);

	/* Se escribe el número de cliente que nos ha enviado el servidor */
	logDebug("Soy el CPU %d", buffer);


	while (!end_cpu)
	{
		Package* package = malloc(sizeof(Package));
		esperando_mensaje = 1;
		if(recieve_and_deserialize(package,socketNucleo) > 0){
			//logDebug("Nucleo envía [message code]: %d, [Mensaje]: %s", package->msgCode, package->message);
			esperando_mensaje = 0;
			analizarMensaje(package,args);
		}
		destroyPackage(package);
	}
}

void analizarMensaje(Package* package, arg_struct *args){
	if(package->msgCode==EXEC_NEW_PROCESS){
		cargarContextoPCB(package);
		ejecutarProceso(args);
	} else if(package->msgCode==QUANTUM_SLEEP_CPU){
		quantumSleep(args,atoi(package->message));
	} else if(package->msgCode==CONTINUE_EXECUTION){
		ejecutarProceso(args);
	} else if(package->msgCode==ABORT_EXECUTION){
		abortarProceso(args);
	} else if(package->msgCode==CONTEXT_SWITCH){
		contextSwitch();
	}
}

void contextSwitch(){
	logTrace("Cambiando contexto proceso PID:%d",pcbActual->processID);
	logTrace("Informando al Nucleo que el CPU se encuentra libre");
	informarNucleoContextSwitchFinished(socketNucleo,pcbActual);
	destroyPCB(pcbActual);
}

void contextSwitch_semBlocked(){
	logTrace("El proceso PID:%d fue bloqueado por un semaforo",pcbActual->processID);
	informarNucleoContextSwitchFinished(socketNucleo,pcbActual);
	destroyPCB(pcbActual);
	proceso_fue_bloqueado = 1;
}

void cargarContextoPCB(Package* package){
	pcbActual = deserializar_PCB(package->message);
	logTrace("Contexto de proceso cargado PID:%d...",pcbActual->processID);
	nuevo_pid(pcbActual->processID);
}

void ejecutarProceso(arg_struct *args){
	logTrace("Ejecutando instruccion del proceso PID:%d...",pcbActual->processID);

	ejecutarInstruccion();

	logTrace("Informando al Nucleo que finalizo la ejecucion de 1 Quantum. (PC:%d/%d)",pcbActual->programCounter,pcbActual->codeIndex->instrucciones_size);
	enviarMensajeSocket(socketNucleo,EXECUTION_FINISHED,"");
}

void quantumSleep(arg_struct *args, int milisegundos){
	logTrace("Sleeping %d miliseconds (-.-) ...zzzZZZ",milisegundos);

	usleep(milisegundos*1000);//convierto micro en milisegundos

	if(proceso_fue_bloqueado){
		if (end_signal_received){
			logInfo("*** Se ha recibido la signal de desconexion de CPU ***");
			end_cpu = 1;
		} else {
			informarNucleoCPUlibre(socketNucleo);
			logTrace("CPU se encuentra libre");
			proceso_fue_bloqueado = 0;
		}
	} else if(hubo_exception){
		informarNucleoCPUlibre(socketNucleo);
		logTrace("CPU se encuentra libre");
		hubo_exception = 0;
	} else {
		if (end_signal_received){
			logInfo("*** Se ha recibido la signal de desconexion de CPU ***");
			informarNucleoCPUdisconnectedBySignal(socketNucleo,pcbActual);
			destroyPCB(pcbActual);
			end_cpu = 1;
		} else if(programa_finalizado){
			programa_finalizado = 0;
			finalizarPrograma();
		} else {
			informarNucleoQuantumFinished(socketNucleo,pcbActual);
		}
	}

}

//verifica si se terminaron las instrucciones del programa
bool programaFinalizado(){
	return pcbActual->codeIndex->instrucciones_size - pcbActual->programCounter <= 0;
}

void abortarProceso(arg_struct *args){
	logTrace("Abortando proceso PID:%d",pcbActual->processID);
	logTrace("Informando al Nucleo que el CPU se encuentra libre");
	informarNucleoCPUlibre(socketNucleo);
	destroyPCB(pcbActual);
}

void ejecutarInstruccion(){
	char* instruccion = getSiguienteInstruccion();

	if(!hubo_exception){
		//incremento el PC antes de ejecutar la instruccion por si se bloquea
		pcbActual->programCounter++;

		logDebug("=================================");
		logDebug("Ejecutando '%s'", instruccion);
		analizadorLinea(instruccion, &functions, &kernel_functions);
		logDebug("=================================");
	}
	if(instruccion!=NULL){
		free(instruccion);
	}
}

int getSocketUMC(){
	return socketUMC;
}


char* getInstruccion(char* codigo, int offset, int length){
	char* instruccion = malloc(sizeof(char)*length+1);
	if(instruccion!=NULL){
		memcpy(instruccion,codigo+offset,length);
		instruccion[length]='\0';
	}
	return instruccion;
}

char* getSiguienteInstruccion(){
	int offset = pcbActual->codeIndex->instrucciones_serializado[pcbActual->programCounter].start;
	int length = pcbActual->codeIndex->instrucciones_serializado[pcbActual->programCounter].offset;
	//return getInstruccion(pcbActual->programa,offset,length);
	return getInstruccionFromUMC(offset,length);
}

char* getInstruccionFromUMC(int offset, int length){
	char* buffer = NULL;
	int buffer_len = 0;
	int pagina_num = offset/size_pagina;
	int pagina_offset = offset%size_pagina;
	int restante = length;
	int size_pedido;
	while(pagina_offset + restante > size_pagina){
		size_pedido = size_pagina - pagina_offset;
		buffer = realloc(buffer,buffer_len+size_pedido);
		char* pedido = pedirCodigoUMC(pagina_num,pagina_offset,size_pedido);
		if(hubo_exception){
			return NULL;
		}
		memcpy(buffer+buffer_len,pedido,size_pedido);
		buffer_len += size_pedido;
		restante -= size_pedido;
		pagina_offset += size_pedido;
		if(pagina_offset>=size_pagina){
			pagina_num++;
			pagina_offset -= size_pagina;
		}
		free(pedido);
	}
	if(restante>0){
		buffer = realloc(buffer,buffer_len+restante);
		char* pedido = pedirCodigoUMC(pagina_num,pagina_offset,restante);
		if(hubo_exception){
			return NULL;
		}
		memcpy(buffer+buffer_len,pedido,restante);
		free(pedido);
		buffer_len += restante;
	}
	buffer = realloc(buffer,buffer_len+1);
	buffer[buffer_len]='\0';//lo convierto en string
	return buffer;
}

char* pedirCodigoUMC(uint32_t pagina, uint32_t offset, uint32_t size){
	char* contenido = NULL;
	int resultado = leer_pagina(pagina,offset,size,&contenido);
	logTrace("Resultado Lectura en UMC: %d",resultado);
	if(resultado<=0){
		informarException();
	}
	return contenido;
}

void ejecutarOperacionIO(char* io_id, uint32_t cant_operaciones){
	logTrace("Enviando al Nucleo ejecucion de operacion I/O");
	informarNucleoEjecutarOperacionIO(socketNucleo, pcbActual, io_id, cant_operaciones);
	destroyPCB(pcbActual);
	proceso_fue_bloqueado = 1;
}

void finalizarPrograma(){
	//envia el PCB de nuevo al Nucleo
	informarNucleoFinPrograma(socketNucleo,pcbActual);
	destroyPCB(pcbActual);
	logDebug("CPU se encuentra libre");
}

void execute_wait(char* sem_id){
	if(sem_id[strlen(sem_id)-1]=='\n'){
		sem_id[strlen(sem_id)-1]='\0';
	}
	char* serialized = serializar_semaforo(pcbActual->processID,sem_id);
	uint32_t length = getLong_semaforo(sem_id);

	Package* package = createPackage();

	enviarMensajeSocketConLongitud(socketNucleo,SEM_WAIT,serialized,length);

	if(recieve_and_deserialize(package,socketNucleo) > 0)
	{
		if(package->msgCode==CONTEXT_SWITCH_SEM_BLOCKED){
			contextSwitch_semBlocked();
		}
	}
	destroyPackage(package);
	free(serialized);
}

void execute_signal(char* sem_id){
	if(sem_id[strlen(sem_id)-1]=='\n'){
		sem_id[strlen(sem_id)-1]='\0';
	}
	char* serialized = serializar_semaforo(pcbActual->processID,sem_id);
	uint32_t length = getLong_semaforo(sem_id);
	enviarMensajeSocketConLongitud(socketNucleo,SEM_SIGNAL,serialized,length);
	free(serialized);
}

void informarStackOverflow(){
	logTrace("Informando Nucleo StackOverflow");
	char* tmp_str = string_itoa(pcbActual->processID);
	enviarMensajeSocket(socketNucleo,STACK_OVERFLOW_EXCEPTION,tmp_str);
	hubo_exception = 1;
	free(tmp_str);
}

void informarException(){
	logTrace("Informando Nucleo excepcion de la UMC");
	char* tmp_str = string_itoa(pcbActual->processID);
	enviarMensajeSocket(socketNucleo,GENERIC_EXCEPTION,tmp_str);
	hubo_exception = 1;
	free(tmp_str);
}
