/*
 * interfaz.c
 *
 *  Created on: 19/5/2016
 *      Author: utnso
 */

#include "interfaz.h"

//----------------------------------PRIVADO---------------------------------------

/* El objetivo de esta variable es versatilidad.
 * Los proyectos que usen esta biblioteca no tienen
 * que estar pendientes de como se llama la variable
 * que contiene el id del socket con la umc.
 */
static int* socket_umc;

static char* serializar_parametros(int cant_parametros, ...){

	char* buffer=NULL;
	va_list valist;
	int i,
		offset=0,
		tamanio;
	void* tmp;

	va_start(valist, cant_parametros);

	for(i=0; i<cant_parametros; i++){

		tamanio=va_arg(valist, int);
		buffer=(char*)realloc((char*)buffer,offset+tamanio);

		tmp=va_arg(valist, void*);
		memcpy(buffer+offset,tmp,tamanio);

		offset +=tamanio;
	}

	va_end(valist);
	return buffer;
}

//----------------------------------PUBLICO---------------------------------------

void definir_socket_umc(int* id){

	logDebug("Se define el socket %d para comunicacion con umc",*id);
	socket_umc = id;
}

int inicializar_programa(uint32_t pid, uint32_t paginas, char* contenido){

	char* parametros_serializados;
	Package* package = createPackage();
	uint32_t resultado=-1;
	int tamanio_uint32=sizeof(uint32_t);
	uint32_t size_contenido = strlen(contenido)+1;


	logDebug("Se ha solicitado inicializar el programa %d con %d paginas.",pid,paginas);
	parametros_serializados=serializar_parametros(4, tamanio_uint32, (void*)&pid, tamanio_uint32, (void*)&paginas, tamanio_uint32, (void*)&size_contenido, size_contenido, (void*) contenido);

	enviarMensajeSocketConLongitud(*socket_umc,INICIALIZAR_PROGRAMA_UMC,parametros_serializados,tamanio_uint32*3+size_contenido);
	if(recieve_and_deserialize(package,*socket_umc) > 0)
	{
		if(package->msgCode==RESULTADO_OPERACION){
			memcpy(&resultado,package->message,tamanio_uint32);
		}
	}

	free(parametros_serializados);
	destroyPackage(package);

	return resultado;
}

int leer_pagina(uint32_t pagina, uint32_t offset, uint32_t tamanio, char** contenido){

	char *parametros_serializados;
	Package* package = createPackage();
	int tamanio_uint32=sizeof(uint32_t), resultado;

	logDebug("Se solicito leer %d bytes de la pagina %d",tamanio,pagina);
	parametros_serializados=serializar_parametros(3, tamanio_uint32, (void*)&pagina, tamanio_uint32, (void*) &offset, tamanio_uint32, (void*)&tamanio);

	enviarMensajeSocketConLongitud(*socket_umc,LEER_PAGINA_UMC,parametros_serializados,tamanio_uint32*3);
	if(recieve_and_deserialize(package,*socket_umc) > 0)
	{
		if(package->msgCode==RESULTADO_OPERACION){
			memcpy(&resultado,package->message,tamanio_uint32);

			if(resultado > 0){
				if(!*contenido) *contenido=(char*)malloc(tamanio);
				memcpy(*contenido,package->message + tamanio_uint32,tamanio);
			}
		}
	}

	free(parametros_serializados);
	destroyPackage(package);

	return resultado;
}

int escribir_pagina(uint32_t pagina, uint32_t offset, uint32_t tamanio, char* buffer){

	char *parametros_serializados, *buffer_tmp;
	Package* package = createPackage();
	uint32_t resultado=-1;
	int tamanio_uint32=sizeof(uint32_t);

	buffer_tmp = stream_a_string(buffer,tamanio);
	logDebug("Se solicito escribir %d bytes en la pagina %d. Contenido: %s",tamanio,pagina,buffer_tmp);
	free(buffer_tmp);

	parametros_serializados=serializar_parametros(4, tamanio_uint32, (void*)&pagina, tamanio_uint32, (void*)&offset, tamanio_uint32, (void*)&tamanio, tamanio, (void*)buffer);

	enviarMensajeSocketConLongitud(*socket_umc,ESCRIBIR_PAGINA_UMC,parametros_serializados,tamanio_uint32*3+tamanio);
	if(recieve_and_deserialize(package,*socket_umc) > 0)
	{
		if(package->msgCode==RESULTADO_OPERACION){
			memcpy(&resultado,package->message,tamanio_uint32);
		}
	}

	free(parametros_serializados);
	destroyPackage(package);

	return resultado;
}

int finalizar_programa(uint32_t pid){

	char* parametros_serializados;
	Package* package = createPackage();
	uint32_t resultado=-1;
	int tamanio_uint32=sizeof(uint32_t);

	logDebug("Se solicito finalizar el programa %d",pid);
	parametros_serializados=serializar_parametros(1, tamanio_uint32, (void*)&pid);

	enviarMensajeSocketConLongitud(*socket_umc,FINALIZAR_PROGRAMA_UMC,parametros_serializados,tamanio_uint32*1);
	if(recieve_and_deserialize(package,*socket_umc) > 0)
	{
		if(package->msgCode==RESULTADO_OPERACION){
			memcpy(&resultado,package->message,tamanio_uint32);
		}
	}

	free(parametros_serializados);
	destroyPackage(package);

	return resultado;
}

void nuevo_pid(uint32_t pid){
	int tamanio_uint32=sizeof(uint32_t);

	logDebug("Se solicito al UMC cambiar al Proceso %d",pid);
	char* parametros_serializados=serializar_parametros(1, tamanio_uint32, (void*)&pid);
	enviarMensajeSocketConLongitud(*socket_umc,SWITCH_PROCESS,parametros_serializados,tamanio_uint32*1);
	free(parametros_serializados);
	//sin respuesta
}
