/*
 * CPU.h
 *
 *  Created on: 19/4/2016
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include "configuration.h"
#include <mllibs/nucleoCpu/interfaz.h>
#include <mllibs/umc/interfaz.h>

//codigos de operaciones de la UMC
//esto tenemos que ver bien despues que decidimos hacer para codificarlas
#define INIT_PROGRAM 1
#define SOLICITAR_BYTES_PAGINA 2
#define ALMACENAR_BYTES_PAGINA 3
#define END_PROGRAM 4
#define HANDSHAKE_CPU 5
#define SWITCH_PROCESS 6
#define HANDSHAKE_CPU_NUCLEO 101
//---------------------
//codigos de operaciones con UMC
#define HANDSHAKE_UMC 5
//----------------------
#define NEW_ANSISOP_PROGRAM 10

typedef struct arg_struct {
	Configuration* config;
} arg_struct;


int socketUMC;
int socketNucleo;

int proceso_fue_bloqueado;
int programa_finalizado;
int end_signal_received;
int end_cpu;
int esperando_mensaje;
int hubo_exception;

int size_pagina;
int size_stack;

PCB* pcbActual;//pcb del proceso que se esta ejecutando actualmente en el CPU

//prototipos de funciones
void conectarConUMC(void*);
void iniciarEjecucionCPU(void*);
Configuration* configurar();
void comunicarUMC(int socketUMC, int accion);
void enviarMensaje(int socket, int accion, char* message);
void analizarMensaje(Package* package, arg_struct *args);
void contextSwitch();
void contextSwitch_semBlocked();
void cargarContextoPCB(Package* package);
void ejecutarProceso(arg_struct *args);
void quantumSleep(arg_struct *args, int milisegundos);
bool programaFinalizado();
void abortarProceso(arg_struct *args);
void ejecutarInstruccion();
int getSocketUMC();
char* getInstruccion(char* codigo, int offset, int length);
char* getSiguienteInstruccion();
char* getInstruccionFromUMC(int offset, int length);
char* pedirCodigoUMC(uint32_t pagina, uint32_t offset, uint32_t size);
void ejecutarOperacionIO(char* io_id, uint32_t cant_operaciones);
void finalizarPrograma();
void execute_wait(char* sem_id);
void execute_signal(char* sem_id);
void informarStackOverflow();
void informarException();


#endif /* CPU_H_ */
