/*
 * config.h
 *
 *  Created on: 18/4/2016
 *      Author: utnso
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <time.h>
#include <mllibs/sockets/package.h>
#include <mllibs/sockets/server.h>
#include <mllibs/sockets/client.h>
#include <mllibs/log/logger.h>
#include <commons/config.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <commons/log.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/inotify.h>

#define NUCLEO_CONFIG_PATH "../nucleo.conf"
#define NUCLEO_CONFIG_PATH_ECLIPSE "nucleo.conf"
#define PUERTO_PROG "PUERTO_PROG"
#define PUERTO_CPU "PUERTO_CPU"
#define PUERTO_UMC "PUERTO_UMC"
#define IP_UMC "IP_UMC"
#define IP_NUCLEO "IP_NUCLEO"
#define QUANTUM "QUANTUM"
#define QUANTUM_SLEEP "QUANTUM_SLEEP"
#define SEM_IDS "SEM_IDS"
#define SEM_INIT "SEM_INIT"
#define IO_IDS "IO_IDS"
#define IO_SLEEP "IO_SLEEP"
#define SHARED_VARS "SHARED_VARS"
#define STACK_SIZE "STACK_SIZE"

//para inotify
#define EVENT_SIZE  ( sizeof (struct inotify_event) + 24 )
#define BUF_LEN     ( 1024 * EVENT_SIZE )

typedef struct Configuration {
	int puerto_nucleo_cpu;
	int puerto_nucleo_prog;
	char* ip_nucleo;
	char* ip_umc;
	int puerto_umc;
	int quantum;
	int quantum_sleep;
	int stack_size;
	char* log_level;
	char* log_file;
	char* log_program_name;
	int log_print_console;
	int size_pagina;
	int io_length;
	char** io_ids;
	int* io_sleep;
	int sem_length;
	char** sem_ids;
	int* sem_init;
	int shared_vars_length;
	char** shared_vars;
} Configuration;

Configuration* config;
char* config_dir;
char* config_file_name;

void configurar(char*);
Configuration* getConfiguration();

#endif /* CONFIGURATION_H_ */
