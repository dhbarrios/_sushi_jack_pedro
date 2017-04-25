/*
 * interfazSwap.h
 *
 *  Created on: 17/5/2016
 *      Author: utnso
 */

#ifndef INTERFAZSWAP_H_
#define INTERFAZSWAP_H_

#include <commons/string.h>
#include <mllibs/string/convertir.h>
#include "funciones.h"
#include "configuration.h"

typedef char* pagina;

char* serializar_NuevoPrograma(uint32_t, uint32_t);
int getLong_NuevoPrograma();
char* serializar_EscribirPagina(uint32_t pid, uint32_t numero_de_pagina, pagina pagina, int size_pagina);
int getLong_EscribirPagina(int size_pagina);
char* serializar_SolicitarPagina(uint32_t pid, uint32_t numero_de_pagina);
int getLong_SolicitarPagina();
char* serializar_EliminarPrograma(uint32_t pid);
int getLong_EliminarPrograma();
pagina llenarPagina(char* cad, int size);
int comunicarSWAPNuevoPrograma(uint32_t, uint32_t);
char* leerPaginaSwap(uint32_t, uint32_t);
int escribirPaginaSwap(uint32_t, uint32_t, uint32_t, char*);
int finalizarProgramaSwap(uint32_t);

#endif /* INTERFAZSWAP_H_ */
