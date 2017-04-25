/*
 * comandos.h
 *
 *  Created on: 5/6/2016
 *      Author: utnso
 */

#ifndef COMANDOS_H_
#define COMANDOS_H_

#include <pthread.h>
#include "funciones.h"
#include "UMC.h"

pthread_mutex_t continua_mutex;

int continua;

void handleComandos();

#endif /* COMANDOS_H_ */
