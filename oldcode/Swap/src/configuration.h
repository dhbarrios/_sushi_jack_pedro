/*
 * configuration.h
 *
 *  Created on: 22/04/2016
 *      Author: hernan
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <commons/config.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <mllibs/sockets/client.h>
#include <mllibs/sockets/package.h>
#include <commons/log.h>
#include <mllibs/log/logger.h>
#include <commons/bitarray.h>

#define SWAP_CONFIG_PATH "../swap.conf"
#define SWAP_CONFIG_PATH_ECLIPSE "swap.conf"

#define PUERTO_SWAP "PUERTO_SWAP"
#define IP_SWAP "IP_SWAP"
#define NOMBRE_SWAP "NOMBRE_SWAP"
#define CANTIDAD_PAGINAS "CANTIDAD_PAGINAS"
#define TAMANIO_PAGINA "TAMANIO_PAGINA"
#define RETARDO_ACCESO "RETARDO_ACCESO"
#define RETARDO_COMPACTACION "RETARDO_COMPACTACION"


typedef struct Configuration {
	int puerto_swap;
	char* ip_swap;
	char* nombre_swap;
	int cantidad_paginas;
	int size_pagina;
	int retardo_acceso;
	int retardo_compact;
	char* log_level;
	char* log_file;
	char* log_program_name;
	int log_print_console;
} Configuration;

Configuration* configurar(char*);

#endif /* CONFIGURATION_H_ */
