/*
 * configuration.h
 *
 *  Created on: 19/4/2016
 *      Author: utnso
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <mllibs/log/logger.h>

//CONF_CONSOLA es una variable de entorno definida con el path absoluto al archivo de configuracion default
#define CONSOLA_CONFIG_PATH "CONF_CONSOLA"

//Necesario para interpretar el archivo de configuracion
#define IP_NUCLEO "IP_NUCLEO"
#define PUERTO_NUCLEO "PUERTO_NUCLEO"

//Estructura de configuracion para coneccion por socket al nucleo
typedef struct Configuration{
	int puerto_nucleo;
	char* ip_nucleo;
	char* log_level;
	char* log_file;
	char* log_program_name;
	int log_print_console;
} Configuration;

//Estructura de los parametros de entrada del proceso Consola
typedef struct Parameters{
	Configuration* config;
	char* programa;
} Parameters;

void mostrar_ayuda();
void argumento_invalido(char*);
void liberar_parametros(Parameters*);
Configuration* configurar (char*);
Parameters* interpretar_parametros(int, char*[]);

#endif /* CONFIGURATION_H_ */
