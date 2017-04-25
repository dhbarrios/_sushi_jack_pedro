/*
 * funciones.h
 *
 *  Created on: 8/5/2016
 *      Author: utnso
 */

#ifndef FUNCIONES_H_
#define FUNCIONES_H_

#include <mllibs/sockets/server.h>
#include <mllibs/sockets/client.h>
#include <mllibs/sockets/package.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include "interfaz.h"
#include "configuration.h"
#include "memoria.h"

#define MAX_CLIENTES 10 //cantidad maxima de conexiones por socket (CPUs)

//codigos de operaciones de la UMC
//esto tenemos que ver bien despues que decidimos hacer para codificarlas
#define INIT_PROGRAM 1
#define SOLICITAR_BYTES_PAGINA 2
#define ALMACENAR_BYTES_PAGINA 3
#define END_PROGRAM 4
#define HANDSHAKE_UMC 5
#define RESULTADO_OPERACION 5
#define SWITCH_PROCESS 6
//---------------------
//operaciones SWAP
#define SOLICITAR_PAGINA_SWAP 20
#define ALMACENAR_PAGINA_SWAP 21
#define NUEVO_PROGRAMA_SWAP 22
#define ELIMINAR_PROGRAMA_SWAP 23
//---------------------
//codigos de operacion con el Nucleo
#define HANDSHAKE_NUCLEO 12
//---------------------
//codigos de operacion con los CPU's
#define HANDSHAKE_CPU 5

typedef struct arg_thread_cpu {
	int socket_cpu;
} t_arg_thread_cpu;

typedef struct arg_thread_nucleo {
	int socket_nucleo;
	Configuration* config;
} t_arg_thread_nucleo;

pthread_mutex_t retardo_mutex;

int socket_swap;						//Esto deberia ser privado de interfazSwap.c pero por
pthread_mutex_t comunicacion_swap_mutex,//ahora hay funciones en funciones.c que lo necesitan
				socket_swap_mutex;

void handleClients();
void comunicarSWAP(int);
int conectarConSwap();
void inicializarUMC();
void ejecutarRetardoMemoria();
void handle_cpu(t_arg_thread_cpu*);
void handleNucleo(t_arg_thread_nucleo*);
void setear_pid(uint32_t);
uint32_t obtener_pid();
void crear_key_pid();
void borrar_key_pid();

#endif /* FUNCIONES_H_ */
