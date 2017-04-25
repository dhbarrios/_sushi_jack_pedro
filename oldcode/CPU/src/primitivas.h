/*
 * primitivas.h
 *
 *  Created on: 19/5/2016
 *      Author: utnso
 */

#ifndef PRIMITIVAS_H_
#define PRIMITIVAS_H_

#include <parser/parser.h>
#include <parser/metadata_program.h>
#include "configuration.h"
#include <stdio.h>
#include <ctype.h>

bool verificarStackOverflow();

t_puntero ml_definirVariable(t_nombre_variable variable);
t_puntero ml_obtenerPosicionVariable(t_nombre_variable variable);
t_valor_variable ml_dereferenciar(t_puntero puntero);
void ml_asignar(t_puntero puntero, t_valor_variable variable);
t_valor_variable ml_obtenerValorCompartida(t_nombre_compartida variable);
t_valor_variable ml_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
void ml_irAlLabel(t_nombre_etiqueta t_nombre_etiqueta);
void ml_llamarSinRetorno(t_nombre_etiqueta etiqueta);
void ml_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void ml_finalizar(void);
void ml_retornar(t_valor_variable retorno);
void ml_imprimir(t_valor_variable valor);
void ml_imprimirTexto(char* texto);
void ml_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);
void ml_wait(t_nombre_semaforo identificador_semaforo);
void ml_signal(t_nombre_semaforo identificador_semaforo);

#endif /* PRIMITIVAS_H_ */
