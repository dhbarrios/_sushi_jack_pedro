/*
 ============================================================================
 Name        : Nucleo.c
 Author      : Andres Michel
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "Nucleo.h"

int main(int argc, char *argv[])
{
	configurar(argc > 1 ? argv[1] : NULL);
	Configuration* config = getConfiguration();
	//creo el log
	initLogMutex(config->log_file, config->log_program_name, config->log_print_console, log_level_from_string(config->log_level));

	handleClients(config);

	return 0;
}

