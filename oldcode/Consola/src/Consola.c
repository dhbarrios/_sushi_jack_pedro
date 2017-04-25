/*
 ============================================================================
 Name        : Consola.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "consola.h"

int main(int argc, char* argv[]){

	Parameters* parametros;

	struct timeval tvBegin, tvEnd, tvDiff;//para tomar el tiempo

	initLogMutex(DEFAULT_LOG_FILE,"ELESTAC",true,LOG_LEVEL_DEBUG);

	//Cargo la configuracion
	parametros = interpretar_parametros(argc, argv);

	//Si hay un archivo log por configuracion uso el nuevo
	if(strlen(parametros->config->log_file) != 0 && !strcmp(parametros->config->log_file,DEFAULT_LOG_FILE)){
		logDestroy();
		initLogMutex(parametros->config->log_file,parametros->config->log_program_name,parametros->config->log_print_console,log_level_from_string(parametros->config->log_level));
	}

	logInfo("Consola iniciada");

	if(parametros->programa == NULL){
		logDebug("No fue espesificado un programa, en este momento la consola deberia permitir usar linea de comandos para acceder al archivo");
		return EXIT_FAILURE;
	}

	// begin
	gettimeofday(&tvBegin, NULL);

	logInfo("Ejecutando: %s\n",parametros->programa);

	comunicacionConNucleo(parametros->config, parametros->programa);

	//end
	gettimeofday(&tvEnd, NULL);
	// diff
	timeval_subtract(&tvDiff, &tvEnd, &tvBegin);
	logInfo("Fin programa - Tiempo: %ld.%3ld segundos\n", tvDiff.tv_sec, tvDiff.tv_usec);
	printf("Fin programa - Tiempo: %ld.%3ld segundos\n", tvDiff.tv_sec, tvDiff.tv_usec);

	//Libero la memoria
	liberar_parametros(parametros);

	//Cierro el logger
	logDestroy();

	return EXIT_SUCCESS;
}
