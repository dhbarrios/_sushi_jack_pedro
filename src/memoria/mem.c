/*
 ============================================================================

 ============================================================================
 */

#include "mem.h"

int main(int argc, char *argv[]){

	pthread_t hilo1;

	//Leo la configuracion del archivo y lo guardo en una variable global (config)
	configurar(argc > 1 ? argv[1] : NULL);

	//creo el log
	initLogMutex(config->log_file, config->log_program_name, config->log_print_console, log_level_from_string(config->log_level));

	//Mutex para manejar el retardo de config
	pthread_mutex_init(&retardo_mutex,NULL);

	inicializarUMC(config);

	//No uso mutex porque todavia no hay threads
	continua=1;

	pthread_create(&hilo1,NULL,(void*)handleClients,NULL);

	handleComandos();

	pthread_cancel(hilo1);

	logDestroy();

	return EXIT_SUCCESS;
}
