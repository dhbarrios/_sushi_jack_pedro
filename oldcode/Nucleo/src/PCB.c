/*
 * PCB.c
 *
 *  Created on: 26/4/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "PCB.h"
#include "Nucleo.h"
#include <commons/string.h>
#include <mllibs/sockets/server.h>
#include <mllibs/sockets/package.h>
#include <mllibs/sockets/client.h>
#include <mllibs/umc/interfaz.h>
#include <mllibs/log/logger.h>
#include <mllibs/stack/stack.h>
#include <semaphore.h>
#include "planificador.h"
#include "Nucleo.h"

//Estructura que contiene las colas/listas del diagrama de Estados
Estados* estados;

int pidActual = 1;
t_list* consolas_desconectadas;

//mutex de colas/listas de estados
pthread_mutex_t executeMutex;
pthread_mutex_t exitMutex;
pthread_mutex_t newMutex;
pthread_mutex_t readyMutex;

//lista consolas desconectadas mutex
pthread_mutex_t discConsoleMutex;

//array de mutex para cada dispositivo de entrada/salida (para ejecutar la instruccion IO)
pthread_mutex_t* io_mutex_array;
//array de mutex para cada dispositivo de entrada/salida (para encolar/desencolar en bloqueados)
pthread_mutex_t* io_mutex_queues;
//array de semaforos para cada dispositivo de entrada/salida
sem_t* io_sem_array;


//array de valores de semaforos para las primitivas signal y wait de Ansisop
int* sem_values;
//array de colas de procesos bloqueados en semaforos
t_queue** sem_blocked;


//array de valores de variables compartidas
int* shared_vars_values;

//probando
PCB* buildNewPCB(int consolaFD, char* programa){
	PCB *new = malloc(sizeof(PCB));
	new->processID = getNextPID();
	new->consolaFD = consolaFD;
	new->stackFirstPage = 0;
	new->stackOffset = 0;
	new->programCounter = 0;
	new->executedQuantums = 0;
	new->consolaActiva = true;
	new->codeIndex = metadata_desde_literal(programa);

	new->context_len = 0;
	new->stackIndex = NULL;
	crearNuevoContexto(new);//inicializo el contexto del "main"
	new->programa = strdup(programa);//TODO: borrar
	logTrace("Creado PCB [PID:%d, ConsolaFD:%d, QuantumsExec:%d]",new->processID,new->consolaFD,new->executedQuantums);
	return new;
}

int getNextPID(){
	return pidActual++;
}

void inicializarEstados(){
	logTrace("Inicializando Estados del Planificador");
	estados = malloc(sizeof(Estados));
	//inicializo colas de bloqueados (1 por dispositivo)
	estados->block = malloc(sizeof(t_queue*)*config->io_length);
	int i;
	for(i=0; i<config->io_length; i++){
		estados->block[i] = queue_create();
	}

	estados->execute = list_create();
	estados->exit = queue_create();
	estados->new = queue_create();
	estados->ready = queue_create();

	io_mutex_queues = malloc(sizeof(pthread_mutex_t)*config->io_length);
	//inicializo los mutex
	for(i=0; i<config->io_length; i++){
		pthread_mutex_init(&io_mutex_queues[i],NULL);
	}
	pthread_mutex_init(&executeMutex,NULL);
	pthread_mutex_init(&exitMutex,NULL);
	pthread_mutex_init(&newMutex,NULL);
	pthread_mutex_init(&readyMutex,NULL);
	pthread_mutex_init(&discConsoleMutex,NULL);

	//inicializo lista de consolas (socket FD) que se desconectaron
	//cuando el programa entro en bloqueo por solicitud IO
	consolas_desconectadas = list_create();
}

void destroyEstados(){
	int i;
	for(i=0; i<config->io_length; i++){
		queue_destroy_and_destroy_elements(estados->block[i],(void*)destroy_solicitud_io);
	}
	list_destroy_and_destroy_elements(estados->execute,(void*)destroyPCB);
	queue_destroy_and_destroy_elements(estados->exit,(void*)destroyPCB);
	queue_destroy_and_destroy_elements(estados->new,(void*)destroyPCB);
	queue_destroy_and_destroy_elements(estados->ready,(void*)destroyPCB);
	for(i=0; i<config->io_length; i++){
		pthread_mutex_destroy(&io_mutex_queues[i]);
	}
	pthread_mutex_destroy(&executeMutex);
	pthread_mutex_destroy(&exitMutex);
	pthread_mutex_destroy(&newMutex);
	pthread_mutex_destroy(&readyMutex);
}

void destroy_solicitud_io(solicitud_io* self){
	destroyPCB(self->pcb);
	free(self->io_id);
	free(self);
}

void sendToNEW(PCB* pcb){
	pthread_mutex_lock(&newMutex);
	queue_push(estados->new,pcb);
	pthread_mutex_unlock(&newMutex);
	logTrace("Plan: PCB:%d / -> NEW",pcb->processID);
}

PCB* getNextFromNEW(){
	pthread_mutex_lock(&newMutex);
	PCB* pcb = queue_pop(estados->new);
	pthread_mutex_unlock(&newMutex);
	logTrace("Plan: PCB:%d / NEW -> next",pcb->processID);
	return pcb;
}

solicitud_io* getNextFromBlock(int io_index){
	pthread_mutex_lock(&io_mutex_queues[io_index]);
	solicitud_io* sol = queue_pop(estados->block[io_index]);
	pthread_mutex_unlock(&io_mutex_queues[io_index]);
	logTrace("Plan: PCB:%d / Block [%s] -> next",sol->pcb->processID,sol->io_id);
	return sol;
}

PCB* getNextFromREADY(){
	pthread_mutex_lock(&readyMutex);
	PCB* pcb = queue_pop(estados->ready);
	pthread_mutex_unlock(&readyMutex);
	logTrace("Plan: PCB:%d / READY -> next",pcb->processID);
	return pcb;
}

void sendToREADY(PCB* pcb){
	pthread_mutex_lock(&readyMutex);
	queue_push(estados->ready,pcb);
	pthread_mutex_unlock(&readyMutex);
	logTrace("Plan: PCB:%d / -> READY",pcb->processID);
}

void sendToEXEC(PCB* pcb){
	pthread_mutex_lock(&executeMutex);
	list_add(estados->execute,pcb);
	pthread_mutex_unlock(&executeMutex);
	logTrace("Plan: PCB:%d / -> EXEC",pcb->processID);
}
//TODO: ver si hay que organizar distintas colas de bloqueados
void sendToBLOCK(solicitud_io* solicitud, int io_index){
	pthread_mutex_lock(&io_mutex_queues[io_index]);
	queue_push(estados->block[io_index],solicitud);
	pthread_mutex_unlock(&io_mutex_queues[io_index]);
	logTrace("Plan: PCB:%d / -> BLOCK",solicitud->pcb->processID);
}

void sendToEXIT(PCB* pcb){
	pthread_mutex_lock(&exitMutex);
	queue_push(estados->exit,pcb);
	pthread_mutex_unlock(&exitMutex);
	logTrace("Plan: PCB:%d / -> EXIT",pcb->processID);
}

void sendFromEXECtoREADY(int pid){
	PCB* proceso = removeFromEXEC(pid);
	if(proceso!=NULL){
		sendToREADY(proceso);
	}
}

void abortFromREADY(int index){
	PCB* pcb = list_remove(estados->ready->elements,index);
	logTrace("Plan: PCB:%d / READY -> abort",pcb->processID);
	sendToEXIT(pcb);
}

void abortFromBLOCK(int index, int io_index){
	solicitud_io* solicitud = list_remove(estados->block[io_index]->elements,index);
	logTrace("Plan: PCB:%d / BLOCK -> abort",solicitud->pcb->processID);
	sendToEXIT(solicitud->pcb);
	free_solicitud_io(solicitud);
}

void abortFromNEW(int index){
	PCB* pcb = list_remove(estados->new->elements,index);
	logTrace("Plan: PCB:%d / NEW -> abort",pcb->processID);
	sendToEXIT(pcb);
}

void abortFromEXEC(int pid){
	PCB* pcb = removeFromEXEC(pid);
	logTrace("Plan: PCB:%d / EXEC -> abort",pcb->processID);
	sendToEXIT(pcb);
}

PCB* removeFromEXEC(int pid){
	int i;
	bool encontrado = false;
	pthread_mutex_lock(&executeMutex);
	t_list* enEjecucion = estados->execute;
	PCB* proceso = NULL;
	for( i=0; i < enEjecucion->elements_count; i++){
		proceso = list_get(enEjecucion,i);
		//saca de la lista y retorna el proceso cuando lo encuentra por el ID
		if(proceso->processID==pid){
			list_remove(enEjecucion,i);
			encontrado = true;
			break;
		}
	}
	pthread_mutex_unlock(&executeMutex);
	logTrace("Plan: PCB:%d / EXEC ->",pid);
	if(encontrado){
		return proceso;
	} else {
		return NULL;
	}
}

PCB* removeNextFromEXIT(){
	pthread_mutex_lock(&exitMutex);
	PCB* pcb = queue_pop(estados->exit);
	pthread_mutex_unlock(&exitMutex);
	if(pcb!=NULL){
		logTrace("Plan: PCB:%d / EXIT -> fin",pcb->processID);
	}
	return pcb;
}

PCB* getFromEXEC(int pid){
	int i;
	bool encontrado = false;
	pthread_mutex_lock(&executeMutex);
	t_list* enEjecucion = estados->execute;
	PCB* proceso = NULL;
	for( i=0; i < enEjecucion->elements_count; i++){
		proceso = list_get(enEjecucion,i);
		//retorna el proceso cuando lo encuentra por el ID
		if(proceso->processID==pid){
			encontrado = true;
			break;
		}
	}
	pthread_mutex_unlock(&executeMutex);
	if(encontrado){
		return proceso;
	} else {
		return NULL;
	}
}

bool hayProcesosEnREADY(){
	return (estados->ready->elements->elements_count)>0;
}

int addQuantumToExecProcess(PCB* proceso, int quantum){
	//le suma 1 a los quantums ejecutados
	proceso->executedQuantums++;
	logTrace("Plan: PCB:%d / Ejecutado 1 Quantum / Actual: %d/%d",proceso->processID,proceso->executedQuantums,quantum);
	//retorna la cantidad que le quedan por ejecutar
	return quantum - proceso->executedQuantums;
}

void quantumFinishedCallback(int pid, int quantum, int socketCPU){
	PCB* proceso = getFromEXEC(pid);
	if(proceso!=NULL){
		if(proceso->consolaActiva){
			//si se le terminaron los quantums al proceso
			if(addQuantumToExecProcess(proceso,quantum)<=0){
				reiniciarQuantumsEjecutados(proceso);//reinicio los quantums ejecutados
				switchProcess(pid,socketCPU);
			} else {
				continueExec(socketCPU,proceso);
			}
		} else {
			//entra por aca si la consola cerro la conexion con el Nucleo
			logTrace("Consola estaba inactiva.");
			abortProcess(pid,socketCPU);
		}
	}
}

void reiniciarQuantumsEjecutados(PCB* pcb){
	pcb->executedQuantums = 0;
}

void contextSwitchFinishedCallback(PCB* pcbActualizado){
	PCB* proceso = removeFromEXEC(pcbActualizado->processID);
	if(proceso!=NULL){
		actualizarPCB(proceso,pcbActualizado);
		reiniciarQuantumsEjecutados(proceso);
		logTrace("Context Switch finished callback: PCB:%d / PC: %d/%d",proceso->processID,proceso->programCounter,proceso->codeIndex->instrucciones_size);
		notifyProcessREADY(proceso);
	}
}

void notifyProcessREADY(PCB* pcb){
	sendToREADY(pcb);
	logTrace("Informando Planificador [Program READY]");
	informarPlanificador(PROGRAM_READY,pcb->processID);
}

void finalizarPrograma(PCB* pcbActualizado, int socketCPU){
	PCB* proceso = removeFromEXEC(pcbActualizado->processID);
	actualizarPCB(proceso,pcbActualizado);
	destroyPCB(pcbActualizado);
	sendToEXIT(proceso);
	informarPlanificadorFinalizarPrograma();
}

void actualizarPCB(PCB* local, PCB* actualizado){
	local->programCounter = actualizado->programCounter;
	local->stackOffset = actualizado->stackOffset;
	contexto* contexto_aux = local->stackIndex;
	uint32_t context_len_aux = local->context_len;
	local->context_len = actualizado->context_len;
	local->stackIndex = actualizado->stackIndex;
	actualizado->stackIndex = contexto_aux;
	actualizado->context_len = context_len_aux;
}

void switchProcess(int pid, int socketCPU){
	logTrace("Informando CPU [Switch process]");
	informarCPU(socketCPU,CONTEXT_SWITCH,pid);
}

void abortProcess(int pid, int socketCPU){
	abortFromEXEC(pid);
	informarCPU(socketCPU,ABORT_EXECUTION,pid);
	informarPlanificadorFinalizarPrograma();
}

void continueExec(int socketCPU, PCB* pcb){
	logTrace("Informando CPU [Continue process execution]");
	continuarEjecucionProcesoCPU(socketCPU);
}

void startExec(CPU* cpu){
	PCB* proceso = getNextFromREADY(estados);
	sendToEXEC(proceso);
	cpu->pid = proceso->processID;
	logTrace("Informando CPU [Execute new process]");
	ejecutarNuevoProcesoCPU(cpu->cpuFD,proceso);
}

void abortExecutingProcess(CPU* cpu){
	bool mismoPid(void* tmp){
		PCB* aux = (PCB*) tmp;
		return aux->processID == cpu->pid;
	}

	pthread_mutex_lock(&executeMutex);
	PCB* pcb = list_remove_by_condition(estados->execute,mismoPid);
	pthread_mutex_unlock(&executeMutex);
	if(pcb!=NULL){
		logTrace("Plan: PCB:%d / EXEC -> abort",pcb->processID);
		sendToEXIT(pcb);
		informarPlanificadorFinalizarPrograma();
	}
}

/*
void informarEjecucionCPU(int socketCPU, int accion, PCB* pcb){
	char* instruccion = getSiguienteInstruccion(pcb);
	char* serialized = serializar_EjecutarInstruccion(pcb->processID,instruccion);
	int longitud = getLong_EjecutarInstruccion(instruccion);
	enviarMensajeSocketConLongitud(socketCPU,EXEC_NEW_PROCESS,serialized,longitud);
	free(serialized);
}*/

void informarCPU(int socketCPU, int accion, int pid){
	char* tmp = string_itoa(pid);
	enviarMensajeSocket(socketCPU,accion,tmp);
	free(tmp);
}

void iniciarPrograma(int consolaFD, char* programa){
	logTrace("Iniciando nuevo Programa Consola");
	PCB* nuevo = buildNewPCB(consolaFD,programa);
	sendToNEW(nuevo);

	//esto es para pedirle a la UMC que reserve espacio para el programa
	//int socketUMC = getSocketUMC();
	int size_pagina = getConfiguration()->size_pagina;
	int stack_size = getConfiguration()->stack_size;
	int pagsNecesarias = getCantidadPaginasNecesarias(programa,size_pagina,stack_size);
	nuevo-> stackFirstPage = (pagsNecesarias-stack_size);
	logDebug("Se necesitan %d paginas para almacenar el programa",pagsNecesarias);

	logDebug("Enviado inicio de programa a UMC");
	int resultado = inicializar_programa(nuevo->processID,pagsNecesarias,programa);
	nuevo = getNextFromNEW(estados);
	if(resultado<0){//ERROR
		logDebug("Ha ocurrido un error en la inicializacion del Proceso %d en UMC",nuevo->processID);
		if(nuevo->consolaActiva){
			logDebug("Informando Consola %d que el programa no se pudo inicializar",consolaFD);
			enviarMensajeSocket(consolaFD,GENERIC_EXCEPTION,INIT_EXCEPTION_MESSAGE);
		}
		logTrace("Destruyendo PCB [PID:%d]",nuevo->processID);
		destroyPCB(nuevo);
	} else {
		if(nuevo->consolaActiva){
			notifyProcessREADY(nuevo);
		} else {
			sendToEXIT(nuevo);
			informarPlanificadorFinalizarPrograma();
		}
	}
}

void abortarPrograma(int consolaFD){
	logTrace("Finalizando Programa Consola");
	//primero lo busca en los estados distintos de EXEC(ejecutandose)
	bool encontrado = findAndExitPCBnotExecuting(consolaFD);
	if(encontrado){
		informarPlanificadorFinalizarPrograma();
	} else {
		//si no lo encuentra, lo busca en la lista de EXEC
		encontrado = findAndExitPCBexecuting(consolaFD);
		if(!encontrado){
			encontrado = findAndExitPCBblockedInSemaphore(consolaFD);
			if(encontrado){
				informarPlanificadorFinalizarPrograma();
			} else {
				//deprecado pero lo dejo por las dudas
				//...porque esta bloqueado en un semaforo
				pthread_mutex_lock(&discConsoleMutex);
				int* consola_p = malloc(sizeof(int));
				*consola_p = consolaFD;
				list_add(consolas_desconectadas,consola_p);
				pthread_mutex_unlock(&discConsoleMutex);
			}
		}
	}

}

bool findAndExitPCBnotExecuting(int consolaFD){
	int i;
	PCB* pcb;
	bool encontrado = false;

	//busco en ready
	pthread_mutex_lock(&readyMutex);
	for(i=0; i<estados->ready->elements->elements_count; i++){
		pcb = list_get(estados->ready->elements,i);
		if(pcb->consolaFD==consolaFD){
			pcb->consolaActiva = false;
			abortFromREADY(i);
			encontrado = true;
			break;
		}
	}
	pthread_mutex_unlock(&readyMutex);

	int j=0;
	while(!encontrado && j<config->io_length){
		//busco en block
		pthread_mutex_lock(&io_mutex_queues[j]);
		for(i=0; i<estados->block[j]->elements->elements_count; i++){
			solicitud_io* solicitud = list_get(estados->block[j]->elements,i);
			if(solicitud->pcb->consolaFD==consolaFD){
				solicitud->pcb->consolaActiva = false;
				abortFromBLOCK(i,j);
				encontrado = true;
				break;
			}
		}
		pthread_mutex_unlock(&io_mutex_queues[j]);
		j++;
	}

	if(!encontrado){
		//busco en new
		pthread_mutex_lock(&newMutex);
		for(i=0; i<estados->new->elements->elements_count; i++){
			pcb = list_get(estados->new->elements,i);
			if(pcb->consolaFD==consolaFD){
				pcb->consolaActiva = false;
				abortFromNEW(i);
				encontrado = true;
				break;
			}
		}
		pthread_mutex_unlock(&newMutex);
	}

	return encontrado;
}

bool findAndExitPCBexecuting(int consolaFD){
	int i;
	bool encontrado = false;
	PCB* pcb;
	//busco en EXEC
	pthread_mutex_lock(&executeMutex);
	for(i=0; i<estados->execute->elements_count; i++){
		pcb = list_get(estados->execute,i);
		if(pcb->consolaFD==consolaFD){
			pcb->consolaActiva = false;
			//otro metodo se encarga de validar este flag
			//para abortarlo cuando termine la ejecucion actual
			encontrado = true;
			break;
		}
	}
	pthread_mutex_unlock(&executeMutex);

	return encontrado;
}

//esto es bastante feo pero bueno... hay que poder finalizar el programa y liberar memoria en umc
bool findAndExitPCBblockedInSemaphore(int consolaFD){
	bool encontrado = false;
	int j=0, i;
	//busco en colas de semaforos
	while(!encontrado && j<config->sem_length){

		for(i=0; i<sem_blocked[j]->elements->elements_count; i++){
			PCB* pcb = list_get(sem_blocked[j]->elements,i);
			if(pcb->consolaFD==consolaFD){
				pcb->consolaActiva = false;
				logTrace("Removiendo proceso %d bloqueado en semaforo %s",pcb->processID,config->sem_ids[j]);
				list_remove(sem_blocked[j]->elements,i);
				sendToEXIT(pcb);
				encontrado = true;
				sem_values[j]++;//tengo que aumentar 1 el semaforo ya que estoy sacando un proceso de la cola
				break;
			}
		}
		j++;
	}

	return encontrado;
}

void informarPlanificador(int accion, int pid){
	char* tmp = string_itoa(pid);
	enviarMensajeSocket(socketPlanificador,accion,tmp);
	free(tmp);
}

int getCantidadPaginasPrograma(char* programa, int size_pagina){
	int length = strlen(programa);
	int cantidad = length/size_pagina;
	if(length%size_pagina!=0){
		cantidad++;//agrego una pagina mas para lo que resta
	}
	return cantidad;
}

int getCantidadPaginasNecesarias(char* programa, int size_pagina, int stack_size){
	int cantidad = getCantidadPaginasPrograma(programa,size_pagina);
	cantidad += stack_size;
	return cantidad;
}

pagina* getPaginasFromPrograma(char* programa, int size_pagina){
	int cantPags = getCantidadPaginasPrograma(programa,size_pagina);
	int offset = 0;
	pagina* pags = malloc(sizeof(pagina)*cantPags);
	int i;
	for(i=0; i<cantPags; i++){
		pags[i] = malloc(sizeof(char)*size_pagina);
		memcpy(pags[i],programa+offset,size_pagina);
		offset+=size_pagina;
	}
	return pags;
}

void destroyPaginas(pagina* paginas, int cantidad){
	int i;
	for(i=0; i<cantidad; i++){
		free(paginas[i]);
	}
	free(paginas);
}

void launch_IO_threads(){
	pthread_t thread_io;

	io_sem_array = malloc(sizeof(sem_t)*config->io_length);
	io_mutex_array = malloc(sizeof(pthread_mutex_t)*config->io_length);

	int i;
	for(i=0; i<config->io_length; i++){
		io_arg_struct *args = malloc(sizeof(io_arg_struct));
		args->estados = estados;
		args->io_index = i;

		//inicializo el semaforo en 0 (vacio)
		sem_init(&io_sem_array[i],0,0);
		//inicializo mutex
		pthread_mutex_init(&io_mutex_array[i],NULL);

		pthread_create(&thread_io,NULL,(void*) ejecutarIO,(void*) args);
		logTrace("Creado thread para atender dispositivo: %s",config->io_ids[i]);
	}
}

void ejecutarIO(void* arguments){
	io_arg_struct *args = arguments;
	int io_index = args->io_index;

	while(1){

		sem_wait(&io_sem_array[io_index]);//consumo 1, se suspende el hilo aca si no hay ninguno esperando en cola de bloqueados para este dispositivo

		//operacion critica de I/O
		pthread_mutex_lock(&io_mutex_array[io_index]);

		solicitud_io* solicitud = getNextFromBlock(io_index);

		logDebug("Ejecutando %d operaciones de %s (%d ms sleep)",solicitud->cant_operaciones,solicitud->io_id,config->io_sleep[io_index]);
		//hago un sleep del tiempo del dispositivo por la cantidad de operaciones
		usleep(config->io_sleep[io_index]*solicitud->cant_operaciones*1000);//paso de micro a milisegundos (*1000)

		pthread_mutex_unlock(&io_mutex_array[io_index]);

		logDebug("Ejecucion IO de %s finalizada (%d ms) PCB:%d",solicitud->io_id,config->io_sleep[io_index]*solicitud->cant_operaciones,solicitud->pcb->processID);

		if(consola_desconectada(solicitud->pcb->consolaFD)){
			sendToEXIT(solicitud->pcb);
			informarPlanificadorFinalizarPrograma();
		} else {
			//mando el proceso a ready
			notifyProcessREADY(solicitud->pcb);
		}

		//libero la solicitud
		free_solicitud_io(solicitud);
	}

	destroy_io_arg_struct(args);
}

void atenderSolicitudDispositivoIO(solicitud_io* solicitud){

	int io_index = getPosicionDispositivo(config->io_ids,config->io_length,solicitud->io_id);
	PCB* pcb = removeFromEXEC(solicitud->pcb->processID);
	actualizarPCB(pcb,solicitud->pcb);
	reiniciarQuantumsEjecutados(pcb);

	destroyPCB(solicitud->pcb);

	solicitud->pcb = pcb;

	sendToBLOCK(solicitud,io_index);

	sem_post(&io_sem_array[io_index]);//incremento el semaforo
}

void destroy_io_arg_struct(io_arg_struct *args){
	free(args);
}

int getPosicionDispositivo(char** lista_ids, int len, char* io_id){
	int i=0;
	while(i<len && !string_equals_ignore_case(lista_ids[i],io_id)){
		i++;
	}
	return i;
}

void free_solicitud_io(solicitud_io* solicitud){
	free(solicitud->io_id);
	free(solicitud);
}

bool consola_desconectada(int consoleFD){
	bool encontrado = false;
	int i;
	int* consola_aux;
	pthread_mutex_lock(&discConsoleMutex);
	for(i=0; i<consolas_desconectadas->elements_count; i++){
		consola_aux = list_get(consolas_desconectadas,i);
		if(*consola_aux==consoleFD){
			list_remove(consolas_desconectadas,i);
			encontrado = true;
			break;
		}
	}
	pthread_mutex_unlock(&discConsoleMutex);
	return encontrado;
}

void inicializarSemaforos(){
	sem_values = malloc(sizeof(int)*config->sem_length);
	sem_blocked = malloc(sizeof(t_queue*)*config->sem_length);
	int i;
	for(i=0; i<config->sem_length; i++){
		sem_values[i] = config->sem_init[i];
		sem_blocked[i] = queue_create();
	}
}

int getPosicionSemaforo(char* sem_id){
	int i=0;
	while(i<config->sem_length && !string_equals_ignore_case(config->sem_ids[i],sem_id)){
		i++;
	}
	return i;
}

void bloquearEnSemaforo(PCB* pcb, char* sem_id){
	int sem_pos = getPosicionSemaforo(sem_id);
	logDebug("Bloqueado en Semaforo %s, PID: %d",sem_id,pcb->processID);
	queue_push(sem_blocked[sem_pos],pcb);
}

PCB* getNextFromSemaforo(int sem_pos){
	PCB* pcb = queue_pop(sem_blocked[sem_pos]);
	return pcb;
}

int execute_wait(char* sem_id){
	int bloquear = 0;
	int pos = getPosicionSemaforo(sem_id);
	sem_values[pos]--;
	logDebug("[WAIT] Semaforo %s. Valor actual: %d",sem_id,sem_values[pos]);
	if(sem_values[pos]<0){
		bloquear = 1;
	}
	return bloquear;
}

void execute_signal(char* sem_id){
	int pos = getPosicionSemaforo(sem_id);
	sem_values[pos]++;
	logDebug("[SIGNAL] Semaforo %s. Valor actual: %d",sem_id,sem_values[pos]);
	if(sem_values[pos]<=0){
		PCB* pcb = getNextFromSemaforo(pos);
		logDebug("Desbloqueado de Semaforo %s, PID: %d",sem_id,pcb->processID);
		if(pcb!=NULL){
			if(consola_desconectada(pcb->consolaFD)){
				sendToEXIT(pcb);
				informarPlanificadorFinalizarPrograma();
			} else {
				notifyProcessREADY(pcb);
			}
		}
	}
}


void inicializarVariablesCompartidas(){
	shared_vars_values = malloc(sizeof(int)*config->shared_vars_length);
	int i;
	for(i=0; i<config->shared_vars_length; i++){
		shared_vars_values[i] = 0;//inicializadas en cero
	}
}

int getPosicionVariableCompartida(char* var_id){
	int i=0;
	while(i<config->shared_vars_length && !string_equals_ignore_case(config->shared_vars[i],var_id)){
		i++;
	}
	return i;
}

int getValorVariableCompartida(char* var_id){
	int pos = getPosicionVariableCompartida(var_id);
	logTrace("[GET] Variable compartida %s. Valor: %d",var_id,shared_vars_values[pos]);
	return shared_vars_values[pos];
}

void setValorVariableCompartida(char* var_id, int valor){
	int pos = getPosicionVariableCompartida(var_id);
	logTrace("[SET] Variable compartida %s. Valor: %d",var_id,valor);
	shared_vars_values[pos] = valor;
}

void informarPlanificadorFinalizarPrograma(){
	enviarMensajeSocket(socketPlanificador,FINALIZAR_PROGRAMA,"");
}
