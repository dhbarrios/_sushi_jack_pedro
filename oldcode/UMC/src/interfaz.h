/*
 * interfaz.h
 *
 *  Created on: 29/5/2016
 *      Author: utnso
 */

#ifndef INTERFAZ_H_
#define INTERFAZ_H_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <mllibs/log/logger.h>
#include "memoria.h"

int inicializar_programa(char*);
int leer_pagina(char*, char**);
int escribir_pagina(char*);
int finalizar_programa(char*);
void nuevo_pid(char*);

#endif /* INTERFAZ_H_ */
