/*
 * Nucleo.h
 *
 *  Created on: 21/04/2016
 *      Author: hernan
 */

#ifndef NUCLEO_H_
#define NUCLEO_H_

#include "configuration.h"
#include <mllibs/nucleoCpu/interfaz.h>

#define MAX_CPUS 20	//cantidad de CPUs que se pueden conectar
#define MAX_CONSOLAS 20	//cantidad de Consolas que se pueden conectar

//codigos de operaciones de la Consola
//esto tenemos que ver bien despues que decidimos hacer para codificarlas
#define NEW_ANSISOP_PROGRAM 10
#define ANSISOP_PROGRAM 11
#define HANDSHAKE_NUCLEO 12
#define PROGRAMA_FINALIZADO 13
#define HANDSHAKE_CPU_NUCLEO 101
//---------------------
//codigos de operacion con la UMC
#define HANDSHAKE_UMC 5
//---------------------

//estructura de argumentos para pasarle a un thread
typedef struct arg_struct {
    int cpuSockets[MAX_CPUS];
    int consolaSockets[MAX_CONSOLAS];
    int socketServerCPU;
    int socketServerConsola;
    int socketServerPlanificador;
    t_list* listaCPUs;
    Configuration* config;
} arg_struct;

//contiene el estado de un CPU conectado al Nucleo
typedef struct CPU {
    int cpuFD;	//file descriptor del socket del cpu
    int pid;	//PID del proceso que esta ejecutando actualmente
    int libre;	// 0:ocupado / 1:libre
} CPU;

int socketUMC;
int socketPlanificador;

int quantum;
pthread_mutex_t quantum_mutex;
int quantum_sleep;
pthread_mutex_t quantum_sleep_mutex;

//prototipos de funciones
void handleClients(Configuration* config);
void handleConsolas();
void handleCPUs();
int elegirRandomCPU(int cpuSockets[]);
void comunicarCPU(int cpuSockets[]);
void enviarMensaje(int socket, int accion, char* message);
void imprimirArraySockets(int sockets[], int len);
void inicializarArraySockets(arg_struct* args);
int conectarConUMC(Configuration* config);
void nuevoCPU(t_list* listaCPUs, int socketCPU);
void destroyCPU(CPU* self);
void liberarCPU(CPU* cpu);
void liberarCPUporSocketFD(int socketCPU, arg_struct *args);
CPU* buscarCPUporSocketFD(int socketCPU, t_list* listaCPUs);
void eliminarCPU(t_list* listaCPUs,int socketCPU);
int conectarConPlanificador(char* ip, int puerto);
void analizarMensajeCPU(int socketCPU , Package* package, arg_struct *args);
int getSocketUMC();
void borrarSocketConsola(arg_struct *args, int socketConsola);
void setQuantum(int valor);
int getQuantum();
void setQuantumSleep(int valor);
int getQuantumSleep();
void handleInotify(void* arguments);
void finalizarProgramaException(PCB* pcb, char* mensaje);

#endif /* NUCLEO_H_ */
