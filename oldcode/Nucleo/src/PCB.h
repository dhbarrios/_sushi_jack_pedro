/*
 * PCB.h
 *
 *  Created on: 26/4/2016
 *      Author: utnso
 */

#ifndef PCB_H_
#define PCB_H_

#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <parser/metadata_program.h>
#include <mllibs/umc/interfaz.h>
#include <mllibs/nucleoCpu/interfaz.h>
#include "Nucleo.h"


/*
 * Estructura con las colas de Estados
 */
typedef struct Estados {
	t_queue* new;
	t_queue* ready;
	t_list* execute;
	t_queue** block;//array de colas de solicitud_io
	t_queue* exit;
} Estados;

typedef struct io_arg_struct {
    Estados* estados;
    int io_index;//posicion del dispositivo en los arrays
} io_arg_struct;


//funciones
PCB* buildNewPCB(int consolaFD, char* programa);
int getNextPID();
void inicializarEstados();
void destroyEstados();
void destroy_solicitud_io(solicitud_io* self);
void sendToNEW(PCB* pcb);
PCB* getNextFromNEW();
solicitud_io* getNextFromBlock(int io_index);
PCB* getNextFromREADY();
void sendToREADY(PCB* pcb);
void sendToEXEC(PCB* pcb);
void sendToBLOCK(solicitud_io* solicitud, int io_index);
void sendToEXIT(PCB* pcb);
void sendFromEXECtoREADY(int pid);
void abortFromREADY(int index);
void abortFromBLOCK(int index, int io_index);
void abortFromNEW(int index);
void abortFromEXEC(int pid);
PCB* removeFromEXEC(int pid);
PCB* removeNextFromEXIT();
PCB* getFromEXEC(int pid);
bool hayProcesosEnREADY();
int addQuantumToExecProcess(PCB* proceso, int quantum);
void quantumFinishedCallback(int pid, int quantum, int socketCPU);
void reiniciarQuantumsEjecutados(PCB* pcb);
void contextSwitchFinishedCallback(PCB* pcbActualizado);
void notifyProcessREADY(PCB* pcb);
void finalizarPrograma(PCB* pcb, int socketCPU);
void actualizarPCB(PCB* local, PCB* actualizado);
void switchProcess(int pid, int socketCPU);
void abortProcess(int pid, int socketCPU);
void continueExec(int socketCPU, PCB* pcb);
void startExec(CPU* cpu);
void abortExecutingProcess(CPU* cpu);
void informarEjecucionCPU(int socketCPU, int accion, PCB* pcb);
void informarCPU(int socketCPU, int accion, int pid);
void iniciarPrograma(int consolaFD, char* programa);
void abortarPrograma(int consolaFD);
bool findAndExitPCBnotExecuting(int consolaFD);
bool findAndExitPCBexecuting(int consolaFD);
bool findAndExitPCBblockedInSemaphore(int consolaFD);
void informarPlanificador(int accion, int pid);
void getCodeIndex(PCB* pcb, char* programa);
int esInstruccionValida(char* str, int offset, int length);
int getCantidadPaginasPrograma(char* programa, int size_pagina);
int getCantidadPaginasNecesarias(char* programa, int size_pagina, int stack_size);
pagina* getPaginasFromPrograma(char* programa, int size_pagina);
void destroyPaginas(pagina* paginas, int cantidad);
void launch_IO_threads();
void ejecutarIO(void* arguments);
void atenderSolicitudDispositivoIO(solicitud_io* solicitud);
void destroy_io_arg_struct(io_arg_struct *args);
int getPosicionDispositivo(char** lista_ids, int len, char* io_id);
void free_solicitud_io(solicitud_io* solicitud);
bool consola_desconectada(int consoleFD);
void inicializarSemaforos();
int getPosicionSemaforo(char* sem_id);
void bloquearEnSemaforo(PCB* pcb, char* sem_id);
PCB* getNextFromSemaforo(int sem_pos);
int execute_wait(char* sem_id);
void execute_signal(char* sem_id);
void inicializarVariablesCompartidas();
int getPosicionVariableCompartida(char* var_id);
int getValorVariableCompartida(char* var_id);
void setValorVariableCompartida(char* var_id, int valor);
void informarPlanificadorFinalizarPrograma();

#endif /* PCB_H_ */
