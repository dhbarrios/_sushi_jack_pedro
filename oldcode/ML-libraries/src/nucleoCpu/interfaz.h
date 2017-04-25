/*
 * interfaz.h
 *
 *  Created on: 26/5/2016
 *      Author: utnso
 */

#ifndef NUCLEOCPU_INTERFAZ_H_
#define NUCLEOCPU_INTERFAZ_H_

#include <stdio.h>
#include <stdlib.h>
#include "../sockets/server.h"
#include "../sockets/client.h"
#include "../sockets/package.h"
#include "../log/logger.h"
#include "../stack/stack.h"
#include <stdint.h>
#include <parser/metadata_program.h>

//codigos de operaciones entre CPU/Nucleo
#define CONTINUE_EXECUTION 70
#define ABORT_EXECUTION 71
#define EXEC_NEW_PROCESS 72
#define EXECUTION_FINISHED 73
#define QUANTUM_SLEEP_CPU 74
#define QUANTUM_FINISHED 75
#define PROGRAM_FINISHED 76
#define CONTEXT_SWITCH 77
#define CONTEXT_SWITCH_FINISHED 78
#define EXEC_IO_OPERATION 79
#define PRINT_VARIABLE 80
#define PRINT_TEXT 81
#define CPU_SIGNAL_DISCONNECTED 82
#define GET_SHARED_VAR 83
#define SET_SHARED_VAR 84
#define SEM_WAIT 85
#define SEM_SIGNAL 86
#define CONTEXT_SWITCH_SEM_BLOCKED 87
#define CPU_LIBRE 90

//Exceptions
#define GENERIC_EXCEPTION 200
#define STACK_OVERFLOW_EXCEPTION 201
#define INIT_EXCEPTION 202
#define GENERIC_EXCEPTION_MESSAGE "Generic UMC Exception"
#define STACK_OVERFLOW_EXCEPTION_MESSAGE "Stack overflow Exception"
#define INIT_EXCEPTION_MESSAGE "Initialization Exception"
//-------------------------------

typedef struct dir_memoria {
	uint32_t pagina;
	uint32_t offset;
	uint32_t size;
} dir_memoria;

typedef struct variable {
	dir_memoria direccion;
	char nombre;
} variable;

typedef struct contexto {
	variable* argumentos;
	uint32_t arg_len;
	variable* variables;
	uint32_t var_len;
    t_puntero_instruccion retPos;
	dir_memoria retVar;
} contexto;

typedef struct PCB {
	uint32_t processID;		//identificador unico del proceso
	int consolaFD;		//file descriptor del socket de la consola que inicio el programa
	uint32_t programCounter;	//contador de programa
	uint32_t stackFirstPage;		//numero de pagina de inicio del stack en la UMC
	uint32_t stackOffset;	//offset actual donde agregar variables en el stack
	int executedQuantums;	//cantidad de quantums ya ejecutados
	t_metadata_program* codeIndex;		//indice de codigo
	contexto* stackIndex;		//array con elementos "contexto"
	uint32_t context_len;		//largo del array stackIndex
	bool consolaActiva;	//indica si la consola esta activa o si cerro la conexion
	char* programa;	//codigo del programa TODO: borrar
} PCB;

//wrapper que se crea con cada solicitud de I/O de un proceso
//va a la cola de bloqueados correspondiente al dispositivo (io_id)
typedef struct solicitud_io {
    PCB* pcb;
    char* io_id;
	int cant_operaciones;
} solicitud_io;

typedef struct print_var {
	uint32_t pid;
	uint32_t valor;
} print_var;

typedef struct print_text {
	uint32_t pid;
	char* text;
} print_text;

typedef struct sem_action {
	uint32_t pid;
	char* sem_id;
} sem_action;

typedef struct shared_var {
	char* var_id;
	uint32_t value;
} shared_var;

//serializacion PCB

void destroyPCB(PCB* self);
void serializarDato(char* buffer, void* dato, int size_to_send, int* offset);
void deserializarDato(void* dato, char* buffer, int size, int* offset);
char* serializarPCB(PCB* pcb);
uint32_t getLong_PCB(PCB* pcb);
char* serializar_metadata_program(t_metadata_program* metadata);
uint32_t getLong_metadata_program(t_metadata_program* metadata);
PCB* deserializar_PCB(char* serialized);
t_metadata_program* deserializar_metadata_program(char* serialized);
char* serializar_ejecutarOperacionIO(PCB* pcb, char* io_id, uint32_t cant_operaciones);
uint32_t getLong_ejecutarOperacionIO(PCB* pcb, char* io_id, uint32_t cant_operaciones);
solicitud_io* deserializar_ejecutarOperacionIO(char* serialized);
char* serializar_contexto(contexto* contexto);
uint32_t getLong_contexto(contexto* contexto);
contexto* deserializar_contexto(char* serialized);
char* serializar_array_variables(variable** variables, uint32_t len);
variable* deserializar_array_variables(char* serialized, uint32_t len);
char* serializar_stack(contexto** contextos, uint32_t contextos_length);
uint32_t getLong_stack(contexto* contextos, uint32_t contextos_length);
contexto* deserializar_stack(char* serialized,uint32_t contextos_length);
void crearNuevoContexto(PCB* pcb);
void destruirContextoActual(PCB* pcb, int size_pagina);
void destroy_stackIndex(contexto* contexto, uint32_t context_len);
void destroy_dir_memoria(dir_memoria* dir);
char* serializar_imprimirVariable(uint32_t pid, uint32_t valor);
print_var* deserializar_imprimirVariable(char* serialized);
char* serializar_imprimirTexto(uint32_t pid, char* texto);
print_text* deserializar_imprimirTexto(char* serialized);
void destroy_print_text(print_text* self);
void destroy_print_var(print_var* self);
char* serializar_imprimirVariable_consola(uint32_t valor);
uint32_t deserializar_imprimirVariable_consola(char* serialized);
char* serializar_semaforo(uint32_t pid, char* sem_id);
sem_action* deserializar_semaforo(char* serialized);
uint32_t getLong_semaforo(char* sem_id);
void destroy_sem_action(sem_action* self);
char* serializar_shared_var(uint32_t valor, char* var_id);
shared_var* deserializar_shared_var(char* serialized);
uint32_t getLong_shared_var(char* var_id);
void destroy_shared_var(shared_var* self);


//funciones interfaz CPU a Nucleo

void informarNucleoFinPrograma(int socketNucleo, PCB* pcb);
void informarNucleoQuantumFinished(int socketNucleo, PCB* pcb);
void informarNucleoContextSwitchFinished(int socketNucleo, PCB* pcb);
void informarNucleoCPUdisconnectedBySignal(int socketNucleo, PCB* pcb);
void informarNucleoCPUlibre(int socketNucleo);
void informarNucleoEjecutarOperacionIO(int socketNucleo, PCB* pcb, char* io_id, uint32_t cant_operaciones);
void informarNucleoImprimirVariable(int socketNucleo, uint32_t pid, t_valor_variable valor);
void informarNucleoImprimirTexto(int socketNucleo, uint32_t pid, char* texto);
uint32_t getValorCompartida(int socketNucleo, char* var_id);
void setValorCompartida(int socketNucleo, char* var_id, uint32_t valor);

//funciones interfaz Nucleo a CPU

void ejecutarNuevoProcesoCPU(int socketCPU, PCB* pcb);
void continuarEjecucionProcesoCPU(int socketCPU);
PCB* informarCPUbloqueoSemaforo(int socketCPU);
void informarValorVariableCompartida(int socketCPU, uint32_t valor);

#endif /* NUCLEOCPU_INTERFAZ_H_ */
