/*
 * interfazSwap.c
 *
 *  Created on: 17/5/2016
 *      Author: utnso
 */

#include "interfazSwap.h"

static int comunicar_con_swap(char accion, char* buffer, uint32_t longitud, Package** respuesta){

	int result=-1;
	char comunicacion_no_exitosa=1,
		 *tmp_buf;
	Package *paquete_recibido=malloc(sizeof(Package));	//recieve_and_deserialize espera un puntero a Package con espacio suficiente

	pthread_mutex_lock(&comunicacion_swap_mutex);
	pthread_mutex_lock(&socket_swap_mutex);

	while(comunicacion_no_exitosa){

		enviarMensajeSocketConLongitud(socket_swap,accion,buffer,longitud);

		if(recieve_and_deserialize(paquete_recibido,socket_swap) > 0){

			tmp_buf = stream_a_string(paquete_recibido->message,paquete_recibido->message_long);
			logDebug("Swap envía [message code]: %d, [Mensaje]: %s", paquete_recibido->msgCode, tmp_buf);
			free(tmp_buf);

			/* En un programa posta esto traeria muchos problemas porque si se
			 * corta la comunicacion entre el send y el receive voy a mandar
			 * otro send y reservar el doble de memoria para el mismo programa.
			 *
			 * La forma de solucionarlo seria que el Swap devuelva un codigo
			 * que represente el pedido y el resultado se guarde en el Swap.
			 * Despues la UMC tiene que, con el codigo, pedir el resultado de
			 * la operacion, de esta manera si se corta la comunicacion y luego
			 * se reestablece solo pedis el codigo en vez de volver a pedir memoria.
			 */
			comunicacion_no_exitosa = 0;
			result = 0;
		}else{
			conectarConSwap();
			destroyPackage(paquete_recibido);
		}
	}

	*respuesta = paquete_recibido;

	pthread_mutex_unlock(&socket_swap_mutex);
	pthread_mutex_unlock(&comunicacion_swap_mutex);

	return result;
}

char* serializar_NuevoPrograma(uint32_t pid, uint32_t cantPags){
	//mensaje: pid + cantPags
	char *serializedPackage = malloc(sizeof(uint32_t)*2);

	int offset = 0;
	int size_to_send;

	size_to_send = sizeof(uint32_t);
	memcpy(serializedPackage + offset, &pid, size_to_send);
	offset += size_to_send;

	size_to_send = sizeof(uint32_t);
	memcpy(serializedPackage + offset, &cantPags, size_to_send);

	return serializedPackage;
}

int getLong_NuevoPrograma(){
	return sizeof(uint32_t)*2;
}


char* serializar_EscribirPagina(uint32_t pid, uint32_t numero_de_pagina, pagina pagina, int size_pagina){
	//mensaje: pid + numero_de_pagina + pagina
	char *serializedPackage = malloc(sizeof(uint32_t)*2+size_pagina);

	int offset = 0;
	int size_to_send;

	size_to_send = sizeof(uint32_t);
	memcpy(serializedPackage + offset, &pid, size_to_send);
	offset += size_to_send;

	size_to_send = sizeof(uint32_t);
	memcpy(serializedPackage + offset, &numero_de_pagina, size_to_send);
	offset += size_to_send;

	size_to_send = size_pagina;
	memcpy(serializedPackage + offset, pagina, size_to_send);
	offset += size_to_send;

	return serializedPackage;
}

int getLong_EscribirPagina(int size_pagina){
	return sizeof(uint32_t)*2+size_pagina;
}


char* serializar_SolicitarPagina(uint32_t pid, uint32_t numero_de_pagina){
	//mensaje: pid + numero_de_pagina
	char *serializedPackage = malloc(sizeof(uint32_t)*2);

	int offset = 0;
	int size_to_send;

	size_to_send = sizeof(uint32_t);
	memcpy(serializedPackage + offset, &pid, size_to_send);
	offset += size_to_send;

	size_to_send = sizeof(uint32_t);
	memcpy(serializedPackage + offset, &numero_de_pagina, size_to_send);

	return serializedPackage;
}

int getLong_SolicitarPagina(){
	return sizeof(uint32_t)*2;
}


char* serializar_EliminarPrograma(uint32_t pid){
	char *serializedPackage = malloc(sizeof(uint32_t));

	int size_to_send;

	size_to_send = sizeof(uint32_t);
	memcpy(serializedPackage, &pid, size_to_send);

	return serializedPackage;
}

int getLong_EliminarPrograma(){
	return sizeof(uint32_t);
}

int comunicarSWAPNuevoPrograma(uint32_t pid, uint32_t cant_paginas){

	char* buffer;
	int longitud,
		result,
		int_respuesta;
	Package **respuesta = malloc(sizeof(Package**));

	buffer = serializar_NuevoPrograma(pid,cant_paginas);
	longitud = getLong_NuevoPrograma();

	result = comunicar_con_swap(NUEVO_PROGRAMA_SWAP,buffer,longitud,respuesta);
	free(buffer);

	int_respuesta = atoi((*respuesta)->message);

	destroyPackage(*respuesta);
	free(respuesta);

	return result == 0 ? int_respuesta : -1; //-1 es error de comunicacion

}

char* leerPaginaSwap(uint32_t pid, uint32_t nro_pagina){

	char* buffer;
	int longitud,
		result;
	Package **respuesta = malloc(sizeof(Package*));

	buffer = serializar_SolicitarPagina(pid,nro_pagina);
	longitud = getLong_SolicitarPagina();

	result = comunicar_con_swap(SOLICITAR_PAGINA_SWAP,buffer,longitud,respuesta);
	free(buffer);

	char* respuesta_lectura = malloc(config->size_pagina);
	memcpy(respuesta_lectura,(*respuesta)->message,config->size_pagina);

	if(result==0){
		destroyPackage(*respuesta);
	}
	free(respuesta);

	return result == 0 ? respuesta_lectura : NULL;	//Todo averiguar como hago para saber si me mando una pagina o un error
}

int escribirPaginaSwap(uint32_t pid, uint32_t nro_pagina, uint32_t tamanio, char* pagina){

	char* buffer;
	int longitud,
		result;
	Package **respuesta = malloc(sizeof(Package*));

	buffer = serializar_EscribirPagina(pid,nro_pagina,pagina,tamanio);
	longitud = getLong_EscribirPagina(tamanio);

	result = comunicar_con_swap(ALMACENAR_PAGINA_SWAP,buffer,longitud,respuesta);
	free(buffer);

	int resultado_escritura = atoi((*respuesta)->message);

	if(result==0){
		destroyPackage(*respuesta);
	}
	free(respuesta);

	return result == 0 ? resultado_escritura : -1;

}

int finalizarProgramaSwap(uint32_t pid){

	char* buffer;
	int longitud,
		result;
	Package **respuesta = malloc(sizeof(Package*));

	buffer = serializar_EliminarPrograma(pid);
	longitud = getLong_EliminarPrograma();

	result = comunicar_con_swap(ELIMINAR_PROGRAMA_SWAP,buffer,longitud,respuesta);
	free(buffer);

	int resultado_finalizar = atoi((*respuesta)->message);

	if(result==0){
		destroyPackage(*respuesta);
	}
	free(respuesta);

	return result == 0 ? resultado_finalizar : -1;

}

//TODO: borrar (solo para probar)
//hace un relleno de pagina con la letra pasada en "cad"
pagina llenarPagina(char* cad, int size){
	pagina page = malloc(sizeof(char)*size);
	int j;
	page[0] = '\0';
	for(j=0; j<size; j++){
		strcat(page,cad);
	}
	page[j-1] = '\0';
	return page;
}
