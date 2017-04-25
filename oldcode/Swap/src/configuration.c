/*
 * configuration.c
 *
 *  Created on: 6/5/2016
 *      Author: utnso
 */

#include "configuration.h"

Configuration* configurar(char *config_file){

	Configuration* config = malloc(sizeof(Configuration));

	t_config* nConfig = config_create(config_file ? config_file :SWAP_CONFIG_PATH);
	if(nConfig==NULL){
		//para debuggear desde eclipse
		nConfig = config_create(SWAP_CONFIG_PATH_ECLIPSE);
		if(nConfig==NULL){
			printf("No se encontro el archivo de configuracion.\n");
			exit (1);
		}
	}
	config->puerto_swap = config_get_int_value(nConfig,PUERTO_SWAP);
	config->ip_swap = strdup(config_get_string_value(nConfig,IP_SWAP));
	config->nombre_swap = strdup(config_get_string_value(nConfig,NOMBRE_SWAP));
	config->cantidad_paginas = config_get_int_value(nConfig,CANTIDAD_PAGINAS);
	config->size_pagina = config_get_int_value(nConfig,TAMANIO_PAGINA);
	config->retardo_acceso = config_get_int_value(nConfig,RETARDO_ACCESO);
	config->retardo_compact = config_get_int_value(nConfig,RETARDO_COMPACTACION);
	//configuracion de log
	config->log_level = strdup(config_get_string_value(nConfig,LOG_LEVEL));
	config->log_file = strdup(config_get_string_value(nConfig,LOG_FILE));
	config->log_program_name = strdup(config_get_string_value(nConfig,LOG_PROGRAM_NAME));
	config->log_print_console = config_get_int_value(nConfig,LOG_PRINT_CONSOLE);

	config_destroy(nConfig);

	return config;
}

