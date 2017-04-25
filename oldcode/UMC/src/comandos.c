/*
 * comandos.c
 *
 *  Created on: 5/6/2016
 *      Author: utnso
 */

#include "comandos.h"

void dump (uint32_t pid, int screen_log, int reporte_memoria, int reporte_tabla){
	FILE *reporte = fopen ("reporte.txt", "a+");
	//si el pid es 0 imprimo todos los procesos

	generar_reporte(reporte, pid, reporte_memoria, reporte_tabla, screen_log);

	fclose(reporte);
}


void retardo (int segundos){
	pthread_mutex_lock(&retardo_mutex);
	config->retraso = segundos;
	pthread_mutex_unlock(&retardo_mutex);
}

void print_retardo(){
	pthread_mutex_lock(&retardo_mutex);
	printf("Retardo actual: %d\n",config->retraso);
	pthread_mutex_unlock(&retardo_mutex);
}


void error_comando(char* comando)
{
	printf("Comando inexistente %s\n", comando);
}

void limpiar_pantalla(){

	system("clear");

}

void fin_programa(){

	/*Se cambia la variable "continua" a 0 para que
	 * todos los threads sepa que se ejecuto el comando exit
	 */

	continua = 0;

}

int parsear_comando(char * comando, char *** comando_parseado_p){

	int i=0,						//Indice para recorrer el string
		contador = 0,				//Contador de palabras parseadas
		letras = 0;					//Conador de letras de la palara a parsear
	char *tmp = comando,			//Puntero al comienzo de la proxima palabra a parsear
		 ** comando_parseado=NULL;	//Puntero temporal

	while(comando[i] != '\0'){
		if(comando[i] == 32){
			if(letras != 0){
				comando_parseado=realloc(comando_parseado,sizeof(char*)*(contador+1));
				comando_parseado[contador] = malloc(sizeof(char)*letras+1);
				strncpy(comando_parseado[contador],tmp,letras);
				comando_parseado[contador][letras] = '\0';
				contador ++;
			}
			tmp=tmp+letras+1;
			letras = 0;
		}else letras++;
		i++;
	}


	comando_parseado=realloc(comando_parseado,sizeof(char*)*(contador+1));
	comando_parseado[contador]= malloc(sizeof(char)*letras+1);
	strncpy(comando_parseado[contador],tmp,letras+1);

	*comando_parseado_p = comando_parseado;

	return contador+1;
}

void intepretarComando(char* comando){

	char **comando_parseado;
	int cantidad, i=0;

	cantidad=parsear_comando(comando, &comando_parseado);

	if(!strcmp(*comando_parseado,"dump")){
		switch(cantidad){
			case 1:
				dump(0,1,1,1);
				break;

			case 2:
				dump(atoi(comando_parseado[1]),1,1,1);
				break;

			case 3:
				dump(atoi(comando_parseado[1]),comando_parseado[2][0]-'0',comando_parseado[2][1]-'0',comando_parseado[2][2]-'0');
				break;

			default:
				error_comando(comando);

		}

	}else if(!strcmp(*comando_parseado,"flush") && (cantidad == 2))

			if(!strcmp(*(comando_parseado+1),"tlb")) flush_tlb();
			else if (!strcmp(*(comando_parseado+1),"memory")) flush_memory();
				 else error_comando(comando);

		 else if(!strcmp(*comando_parseado,"retardo"))

			if(cantidad == 2) retardo(atoi(*(comando_parseado + 1)));
			else print_retardo();

		 else if(!strcmp(*comando_parseado,"clear")) limpiar_pantalla();
				 else if(!strcmp(*comando_parseado,"exit")) fin_programa();
				 	  else error_comando(comando);

	//Libero la memoria de los strings
	for(i=0; i<cantidad; i++){
		free(comando_parseado[i]);
	}

	//Libero la memoria para los punteros a string
	free(comando_parseado);
}

void handleComandos(){

	char * comando;
	size_t size_buff=0;

	//Mutex para manejar el flag de ejecucion
	pthread_mutex_init(&continua_mutex,NULL);

	pthread_mutex_lock(&continua_mutex);
	while(continua){

		comando = NULL;
		printf("ml-umc>");
		getline(&comando,&size_buff,stdin);
		comando[strlen(comando)-1]='\0'; //getline tambien guarda el \n y hay que eliminarlo para poder comparar despues
		intepretarComando(comando);

		free(comando);
	}
	pthread_mutex_unlock(&continua_mutex);
}
