/*
 * convertir.c
 *
 *  Created on: 20/6/2016
 *      Author: utnso
 */

#include "convertir.h"

char* stream_a_string(char* stream, int tamanio_stream){
	char* string = malloc(sizeof(char) * tamanio_stream + sizeof(char));

	memcpy(string, stream, tamanio_stream);
	string[tamanio_stream]='\0';

	return string;
}
