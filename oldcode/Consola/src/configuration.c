/*
 * configuration.c
 *
 *  Created on: 26/4/2016
 *      Author: utnso
 */

#include "configuration.h"

Configuration* configurar(char* archConf){

	Configuration* config = malloc(sizeof(Configuration));

	t_config* nConfig = config_create(archConf);
	if(nConfig==NULL){
		logError("No se encontro el archivo de configuracion en: %s",archConf);
		exit (EXIT_FAILURE);
	}
	config->puerto_nucleo=config_get_int_value(nConfig,PUERTO_NUCLEO);
	config->ip_nucleo = strdup(config_get_string_value(nConfig,IP_NUCLEO));
	config->puerto_nucleo=config_get_int_value(nConfig,PUERTO_NUCLEO);

	config->log_level = strdup(config_get_string_value(nConfig,LOG_LEVEL));
	config->log_file = strdup(config_get_string_value(nConfig,LOG_FILE));
	config->log_program_name = strdup(config_get_string_value(nConfig,LOG_PROGRAM_NAME));
	config->log_print_console = config_get_int_value(nConfig,LOG_PRINT_CONSOLE);

	logDebug("Arhivo log: %s",config->log_file);

	config_destroy(nConfig);
	return config;
}

void mostrar_ayuda(){

	puts("Uso: Consola [-f] programa [-c] \"archivo de configuracion\"\n");

	puts("Opciones:");
	puts("-h\t: muestra este mensaje de ayuda y termina la ejecución del programa.");

	puts("Archivos:");
	puts("-f\t: programa ansisop.");
	puts("-c\t: archivo de configuracion. De no especificarse se el estandar.");
	puts("(Si los archivos se colocan en este orden no hace falta poner las opciones.)");
	puts("(El archivo de configuracion default puede ser espesificado a travez de la variable de entorno CONF_CONSOLA)");

	exit (EXIT_SUCCESS);
}

void argumento_invalido(char* arg){

	if (arg != NULL)
		logError("'%s' no es un argumento válido.\n",arg);
	else
		logError("Faltan argumentos.\n");

	exit(EXIT_FAILURE);
}

Parameters* interpretar_parametros(int argc, char* argv[]){

	Parameters* parametros = malloc(sizeof(Parameters));
	parametros->config = NULL;
	parametros->programa = NULL;
	char* arch_conf_default = getenv(CONSOLA_CONFIG_PATH);
	int i;

	/*
	 *
	 * Uso: Consola [-f] programa [-c] "archivo de configuracion"
	 *
	 * Opciones:
	 *  -h: Muestra ayuda.
	 *
	 * Archivos:
	 *  -f: Programa ansisop.
	 *  -c: Archivo de configuracion.
	 *
	 */

	/*
	 * Esto tiene 2 bugs:
	 *
	 * 1: Si ejecuto algo asi: "Consola ./programa.ansisop -c ../otroPrograma.ansisop" voy a estar pisando en la configuracion el primer programa con el segundo.
	 * 2: Si ejecuto algo asi: "Consola ./programa.ansisop -f" voy a estar leyendo una posicion de memoria que no me pertenece.
	 *
	 * Hay que ver si vale la pena corregirlos
	 *
	 */

	//Leo los argumentos, evito el 0 porque es el path de ejecucion
	for(i=1;i<argc;i++){
		if(*(argv[i])=='-' && strlen(argv[i]) == 2){
			//Si el argumento empieza con "-" y su tamaño es 2 tiene que ser una opcion
			switch(*(argv[i]+1)){
				case 'c':
					parametros->config = configurar(argv[++i]);
					break;
				case 'f':
					parametros->programa = (argv[++i]);
					break;
				case 'h':
					mostrar_ayuda();
					break;
				default:
					//Si no es ninguno de los ateriores el argumento es invalido
					argumento_invalido(argv[i]);
			}
		} else {
			//En caso contrario se asume que es el path del programa o del archivo de configuracion en ese orden.
			if(parametros->programa == NULL) parametros->programa = argv[i];
			else if(parametros->config == NULL) parametros->config = configurar(argv[i]);
		}
	}

	//Si no esta espesificado el archivo de configuracion se toma el default.
	if(parametros->config == NULL){
		if(arch_conf_default == NULL){
			liberar_parametros(parametros);
			argumento_invalido(NULL);
		}
		parametros->config = configurar(arch_conf_default);
	}

	return parametros;
}

void liberar_parametros(Parameters* parametros){
	//Libero la memoria tomada por la estructura de parametros interpretados
	if(parametros != NULL){
		if(parametros->config != NULL){
			free(parametros->config->ip_nucleo);
			free(parametros->config->log_file);
			free(parametros->config->log_level);
			free(parametros->config->log_program_name);
			free(parametros->config);
		}
		free(parametros);
	}
}
