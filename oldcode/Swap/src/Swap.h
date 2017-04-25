/*
 * Swap.h
 *
 *  Created on: 22/04/2016
 *      Author: hernan
 */

#ifndef SWAP_H_
#define SWAP_H_

#include "configuration.h"
#include "fileHandler.h"

//operaciones SWAP
#define SOLICITAR_PAGINA_SWAP 20
#define ALMACENAR_PAGINA_SWAP 21
#define NUEVO_PROGRAMA_SWAP 22
#define ELIMINAR_PROGRAMA_SWAP 23

void handleUMCRequests(Configuration* config);
void analizarMensaje(Package* package, int socketUMC, Configuration* config);
void ejecutarRetardo(int miliseconds);
uint32_t getProcessID_NuevoPrograma(char* str);
uint32_t getCantidadPaginas_NuevoPrograma(char* str);
uint32_t getProcessID_EscribirPagina(char* str);
uint32_t getNumeroPagina_EscribirPagina(char* str);
pagina getPagina_EscribirPagina(char* str, int size);
uint32_t getProcessID_SolicitarPagina(char* str);
uint32_t getProcessID_EliminarPrograma(char* str);
uint32_t getNumeroPagina_SolicitarPagina(char* str);



#endif /* SWAP_H_ */
