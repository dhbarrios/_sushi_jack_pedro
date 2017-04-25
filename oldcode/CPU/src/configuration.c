/*
 * configuration.c
 *
 *  Created on: 19/5/2016
 *      Author: utnso
 */

#include "configuration.h"

Configuration* configurar(char *config_file){

	Configuration* config = malloc(sizeof(Configuration));

	t_config* nConfig = config_create(config_file ? config_file : CPU_CONFIG_PATH);
	if(nConfig==NULL){
		//para debuggear desde eclipse
		nConfig = config_create(CPU_CONFIG_PATH_ECLIPSE);
		if(nConfig==NULL){
			printf("No se encontro el archivo de configuracion.\n");
			exit (1);
		}
	}
	config->puerto_nucleo=config_get_int_value(nConfig,PUERTO_NUCLEO);
	config->ip_nucleo = strdup(config_get_string_value(nConfig,IP_NUCLEO));
	config->puerto_umc=config_get_int_value(nConfig,PUERTO_UMC);
	config->ip_umc = strdup(config_get_string_value(nConfig,IP_UMC));
	//configuracion de log
	config->log_level = strdup(config_get_string_value(nConfig,LOG_LEVEL));
	config->log_file = strdup(config_get_string_value(nConfig,LOG_FILE));
	config->log_program_name = strdup(config_get_string_value(nConfig,LOG_PROGRAM_NAME));
	config->log_print_console = config_get_int_value(nConfig,LOG_PRINT_CONSOLE);

	config_destroy(nConfig);

	return config;
}

