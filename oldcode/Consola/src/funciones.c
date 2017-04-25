/*
 * funciones.c
 *
 *  Created on: 22/04/2016
 *      Author: hernan
 */

#include "funciones.h"

void comunicacionConNucleo(Configuration* config, char* arch_programa){

	logDebug("Iniciando comunicacion con Nucleo.");

	int resp;
	int socket;
	int buffer;

	socket = abrirConexionInetConServer(config->ip_nucleo,config->puerto_nucleo);
	resp = leerSocketClient(socket, (char *)&buffer, sizeof(int));

	if (resp < 1)
	{
			printf ("Me han cerrado la conexión\n");
			exit(-1);
	}

	logInfo("Soy la consola %d\n",buffer);


	//handshake con Nucleo
	logDebug("Iniciando Handshake con Nucleo.");
	handshake(socket);

	//iniciar programa
	logDebug("Iniciando programa AnSISOP.");
	iniciarProgramaAnsisop(socket,arch_programa);

	int continua = 1;
	while (continua)
	{
		Package* package = createPackage();
		if(recieve_and_deserialize(package,socket) > 0){
			if(package->msgCode==PROGRAMA_FINALIZADO){
				continua = 0;
				logDebug("Nucleo me informa que finalizo mi programa");
			} else if(package->msgCode==GENERIC_EXCEPTION){
				continua = 0;
				logDebug("Nucleo me informa: %s",package->message);
			} else if(package->msgCode==PRINT_VARIABLE){
				int valor = deserializar_imprimirVariable_consola(package->message);
				printf("print> %d\n",valor);
			} else if(package->msgCode==PRINT_TEXT){
				printf("print> %s\n",package->message);
			}
		}
		destroyPackage(package);
	}

	close(socket);
}

void handshake(int serverSocket){
	enviarMensajeSocket(serverSocket,HANDSHAKE,"2000");
}

void iniciarProgramaAnsisop(int serverSocket,char* arch_programa){
	char* programa = obtener_programa(arch_programa);
	enviarMensajeSocket(serverSocket,NEW_ANSISOP_PROGRAM,programa);
	free(programa);
}

char* obtener_programa(char* arch_programa){
	FILE *fp;
	char* programa;
	long fsize;

	if((fp=fopen(arch_programa,"r"))==NULL){
		logError("Error al abrir el programa %s",arch_programa);
		exit (EXIT_FAILURE);
	}

	logDebug("Leyendo programa ansisop %s\n",arch_programa);

	//Obtengo el tamanio del archivo
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	//Cargo el archivo en el buffer
	programa = malloc(fsize + 1);
	fread(programa, fsize, 1, fp);
	fclose(fp);

	//Agrego el caracter de fin al buffer
	programa[fsize] = 0;

	logDebug("Programa leido: \n%s",programa);

	return programa;
}

/* Return 1 if the difference is negative, otherwise 0.  */
int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1)
{
    long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
    result->tv_sec = diff / 1000000;
    result->tv_usec = diff % 1000000;

    return (diff<0);
}

void timeval_print(struct timeval *tv)
{
    char buffer[30];
    time_t curtime;

    printf("%ld.%06ld", tv->tv_sec, tv->tv_usec);
    curtime = tv->tv_sec;
    strftime(buffer, 30, "%m-%d-%Y  %T", localtime(&curtime));
    printf(" = %s.%06ld\n", buffer, tv->tv_usec);
}
