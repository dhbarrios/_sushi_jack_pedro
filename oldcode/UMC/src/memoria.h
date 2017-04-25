/*
 * memoria.h
 *
 *  Created on: 5/6/2016
 *      Author: utnso
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <commons/temporal.h>
#include <mllibs/log/logger.h>
#include "configuration.h"
#include "funciones.h"
#include "interfazSwap.h"

typedef char* memoria;
typedef struct timeval timestamp;

typedef struct memoria_principal{
	memoria memoria;
	t_bitarray *bitmap;
} t_memoria_principal;

typedef struct fila_tabla{
	uint16_t numero_pagina;
	uint16_t numero_marco;
	char accedido;//para clock y mejorado
	char modificado;//solo para clock mejorado
} t_fila_tabla;

typedef struct tabla{
	t_list* filas;
	uint32_t pid;
	uint32_t tamanio;
	uint32_t puntero;
} t_tabla;

typedef struct fila_tlb{
	uint32_t numero_pagina;
	uint32_t numero_marco;
	uint32_t pid;
	timestamp* timeStamp;
} t_fila_tlb;

typedef struct tabla_tlb{
	uint32_t tamanio;
	t_list* filas;
} t_tabla_tlb;

pthread_mutex_t bitMap_mutex,
				modificacion_mutex,
				activo_mutex,
				tlb_mutex,
				pid_mutex;

void crearMemoriaPrincipal(int, int);
void crear_tabla_de_paginas(uint32_t,uint32_t);
void crear_tlb(uint32_t tamanio);
void eliminar_tabla_de_paginas(uint32_t);
void eliminar_tabla(t_tabla*, t_tabla**, int);
int obtener_contenido_memoria(char**,uint32_t,uint32_t,uint32_t);
int escribir_contenido_memoria(uint32_t,uint32_t,uint32_t,char*);
int hayMarcosLibres();
void liberar_memoria(uint32_t);
void liberar_entradas_tlb(uint32_t pid);
void flush_tlb();
void flush_memory();
void crearListaDeTablas();
void generar_reporte(FILE*,uint32_t,int,int,int);
void crear_lista_pids_en_uso();
void agregar_mutex_pid(uint32_t);
void eliminar_mutex_pid(uint32_t);

#endif /* MEMORIA_H_ */
