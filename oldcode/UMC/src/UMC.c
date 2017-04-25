/*
 ============================================================================
 Name        : UMC.c
 Author      : Andres Michel
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "UMC.h"
/*
 * Programa principal.
 * Crea un socket servidor y se mete en un select() a la espera de clientes.
 * Cuando un cliente se conecta, le atiende y lo añade al select() y vuelta
 * a empezar.
 */

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

	/*TODO estaria bueno que el handle clients
	 * tambien use la variable continua para
	 * termina bien y en vez de hacer un cancel
	 * hacer un join
	 */
    pthread_cancel(hilo1);

	logDestroy();

	return EXIT_SUCCESS;
}


