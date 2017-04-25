/*
 * funciones.h
 *
 *  Created on: 5/5/2016
 *      Author: utnso
 */

#ifndef FUNCIONES_H_
#define FUNCIONES_H_

#include <mllibs/sockets/client.h>
#include <mllibs/sockets/server.h>
#include <mllibs/sockets/package.h>
#include <mllibs/nucleoCpu/interfaz.h>
#include <sys/time.h>
#include <math.h>
#include "configuration.h"

//codigos de operaciones de la Consola
//esto tenemos que ver bien despues que decidimos hacer para codificarlas
#define NEW_ANSISOP_PROGRAM 10
#define ANSISOP_PROGRAM 11
#define HANDSHAKE 12
#define PROGRAMA_FINALIZADO 13
//---------------------

//prototipos de funciones
void handshake(int);
void comunicacionConNucleo(Configuration*, char*);
void iniciarProgramaAnsisop(int, char*);
char* obtener_programa(char*);

//para tomar el tiempo
int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1);
void timeval_print(struct timeval *tv);

#endif /* FUNCIONES_H_ */
