/*
 * configuration.c
 *
 *  Created on: 19/5/2016
 *      Author: utnso
 */

#include "configuration.h"

void configurar(char *config_file){

	config = malloc(sizeof(Configuration));

	config_file_name = strdup(config_file ? config_file : NUCLEO_CONFIG_PATH);

	t_config* nConfig = config_create(config_file_name);
	if(nConfig==NULL){
		//para debuggear desde eclipse
		free(config_file_name);
		config_file_name = strdup(NUCLEO_CONFIG_PATH_ECLIPSE);
		nConfig = config_create(config_file_name);
		if(nConfig==NULL){
			printf("No se encontro el archivo de configuracion.\n");
			exit (1);
		} else {
			config_dir = strdup(".");
		}
	} else {
		config_dir = strdup(".");
	}
	config->puerto_nucleo_cpu = config_get_int_value(nConfig,PUERTO_CPU);
	config->puerto_nucleo_prog = config_get_int_value(nConfig,PUERTO_PROG);
	config->ip_nucleo = strdup(config_get_string_value(nConfig,IP_NUCLEO));
	config->ip_umc = strdup(config_get_string_value(nConfig,IP_UMC));
	config->puerto_umc = config_get_int_value(nConfig,PUERTO_UMC);

	//planificador
	config->quantum = config_get_int_value(nConfig,QUANTUM);
	config->quantum_sleep = config_get_int_value(nConfig,QUANTUM_SLEEP);

	config->stack_size = config_get_int_value(nConfig,STACK_SIZE);

	//configuracion de log
	config->log_level = strdup(config_get_string_value(nConfig,LOG_LEVEL));
	config->log_file = strdup(config_get_string_value(nConfig,LOG_FILE));
	config->log_program_name = strdup(config_get_string_value(nConfig,LOG_PROGRAM_NAME));
	config->log_print_console = config_get_int_value(nConfig,LOG_PRINT_CONSOLE);

	//levanto los dispositivos de entrada/salida
	config->io_ids = config_get_array_value(nConfig,IO_IDS);
	int len=0;
	while(config->io_ids[len]!=NULL){
		len++;
	}
	config->io_length = len;
	char** io_sleep_aux = config_get_array_value(nConfig,IO_SLEEP);
	config->io_sleep = malloc(sizeof(int)*len);
	int i;
	for(i=0; i<len; i++){
		config->io_sleep[i] = atoi(io_sleep_aux[i]);
		free(io_sleep_aux[i]);
	}
	free(io_sleep_aux);

	//levanto los semaforos
	config->sem_ids = config_get_array_value(nConfig,SEM_IDS);
	len=0;
	while(config->sem_ids[len]!=NULL){
		len++;
	}
	config->sem_length = len;
	char** sem_init_aux = config_get_array_value(nConfig,SEM_INIT);
	config->sem_init = malloc(sizeof(int)*len);
	for(i=0; i<len; i++){
		config->sem_init[i] = atoi(sem_init_aux[i]);
		free(sem_init_aux[i]);
	}
	free(sem_init_aux);

	//levanto variables compartidas
	config->shared_vars = config_get_array_value(nConfig,SHARED_VARS);
	len=0;
	while(config->shared_vars[len]!=NULL){
		len++;
	}
	config->shared_vars_length = len;

	config_destroy(nConfig);
}

Configuration* getConfiguration(){
	return config;
}
