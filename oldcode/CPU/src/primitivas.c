/*
 * primitivas.c
 *
 *  Created on: 19/5/2016
 *      Author: utnso
 */

#include "primitivas.h"
#include "CPU.h"

contexto* getContextoActual(){
	return &(pcbActual->stackIndex[pcbActual->context_len-1]);
}

void crearVariable(t_nombre_variable variable_nom){
	contexto* contexto = getContextoActual();
	contexto->variables = realloc(contexto->variables,sizeof(variable)*(contexto->var_len+1));
	contexto->variables[contexto->var_len].nombre = variable_nom;
	contexto->variables[contexto->var_len].direccion.pagina = pcbActual->stackFirstPage;
	contexto->variables[contexto->var_len].direccion.offset = pcbActual->stackOffset;
	while(contexto->variables[contexto->var_len].direccion.offset + sizeof(uint32_t) > size_pagina){
		contexto->variables[contexto->var_len].direccion.pagina++;
		contexto->variables[contexto->var_len].direccion.offset -= size_pagina;
	}
	contexto->variables[contexto->var_len].direccion.size = sizeof(uint32_t);
	contexto->var_len++;
	pcbActual->stackOffset += sizeof(uint32_t);
}

void crearArgumento(t_nombre_variable variable_nom){
	contexto* contexto = getContextoActual();
	contexto->argumentos = realloc(contexto->argumentos,sizeof(variable)*(contexto->arg_len+1));
	contexto->argumentos[contexto->arg_len].nombre = variable_nom;
	contexto->argumentos[contexto->arg_len].direccion.pagina = pcbActual->stackFirstPage;
	contexto->argumentos[contexto->arg_len].direccion.offset = pcbActual->stackOffset;
	while(contexto->argumentos[contexto->arg_len].direccion.offset + sizeof(uint32_t) > size_pagina){
		contexto->argumentos[contexto->arg_len].direccion.pagina++;
		contexto->argumentos[contexto->arg_len].direccion.offset -= size_pagina;
	}
	contexto->argumentos[contexto->arg_len].direccion.size = sizeof(uint32_t);
	contexto->arg_len++;
	pcbActual->stackOffset += sizeof(uint32_t);
}

bool verificarStackOverflow(){
	bool hayOverflow = false;
	int aux = pcbActual->stackOffset;
	aux = aux/size_pagina;
	if(aux>=size_stack){
		hayOverflow = true;
		if(!hubo_exception){
			informarStackOverflow();
		}
	}
	return hayOverflow;
}

dir_memoria* puntero_a_direccion_logica(t_puntero puntero){
	dir_memoria* dir = malloc(sizeof(dir_memoria));
	int pagina_num = puntero/size_pagina;
	int offset = puntero%size_pagina;
	dir->pagina = pcbActual->stackFirstPage + pagina_num;
	dir->offset = offset;
	dir->size = sizeof(uint32_t);
	return dir;
}

t_puntero direccion_logica_a_puntero(dir_memoria* dir){
	t_puntero puntero = 0;
	puntero += (dir->pagina - pcbActual->stackFirstPage)*size_pagina;
	puntero += dir->offset;
	return puntero;
}



variable* obtener_variable(t_nombre_variable variable_nom){
	variable* var = NULL;
	contexto* contexto = getContextoActual();
	int i;
	for(i=0; i<contexto->var_len; i++){
		if(contexto->variables[i].nombre==variable_nom){
			var = &(contexto->variables[i]);
		}
	}
	return var;
}

variable* obtener_argumento(t_nombre_variable variable_nom){
	variable* var = NULL;
	contexto* contexto = getContextoActual();
	int i;
	for(i=0; i<contexto->arg_len; i++){
		if(contexto->argumentos[i].nombre==variable_nom){
			var = &(contexto->argumentos[i]);
		}
	}
	return var;
}


t_puntero_instruccion obtenerIndiceInstruccion(char* serialized, char* label, t_size size){
	int offset = 0;
	t_puntero_instruccion pos = -1;

	if(label[strlen(label)-1]=='\n'){
		label[strlen(label)-1]='\0';//esto es porque las etiquetas vienen con el \n al final
	}

	while(offset<size && strcmp(serialized+offset,label)!=0){
		offset += strlen(serialized+offset) + 1 + sizeof(t_puntero_instruccion);
	}

	if(offset<size){
		memcpy(&pos,serialized+offset+strlen(serialized+offset)+1,sizeof(t_puntero_instruccion));

	}
	return pos;
}

int escribir_variable_en_umc(uint32_t variable, dir_memoria* dir){
	char* buffer = malloc(sizeof(char)*sizeof(uint32_t));
	memcpy(buffer,&variable,sizeof(uint32_t));
	int resultado = escribir_pagina(dir->pagina,dir->offset,dir->size,buffer);
	logTrace("Resultado Escritura en UMC: %d",resultado);
	if(resultado<=0){
		informarException();
	}
	free(buffer);
	return resultado;
}


//************************************************************
//						PRIMITIVAS
//************************************************************

t_puntero ml_definirVariable(t_nombre_variable variable_nom) {
	t_puntero puntero = -1;
	if(!verificarStackOverflow()){
		logTrace("Definir la variable %c", variable_nom);

		contexto* contexto = getContextoActual();

		dir_memoria dir;
		if(isdigit(variable_nom)){
			crearArgumento(variable_nom);
			logTrace("Argumento definido: Nom: %c",contexto->argumentos[contexto->arg_len-1].nombre);
			puntero = direccion_logica_a_puntero(&(contexto->argumentos[contexto->arg_len-1].direccion));
			dir = contexto->argumentos[contexto->arg_len-1].direccion;
		} else {
			crearVariable(variable_nom);
			logTrace("Variable definida: Nom: %c",contexto->variables[contexto->var_len-1].nombre);
			puntero = direccion_logica_a_puntero(&(contexto->variables[contexto->var_len-1].direccion));
			dir = contexto->variables[contexto->var_len-1].direccion;
		}
		logTrace("Direccion logica a puntero: Pag:%d,Off:%d,Size:%d, puntero:%d",dir.pagina,dir.offset,dir.size,puntero);
	}
	return puntero;
}

t_puntero ml_obtenerPosicionVariable(t_nombre_variable variable_nom) {
	t_puntero puntero = -1;
	if(!hubo_exception){
		logTrace("Obtener posicion de %c", variable_nom);

		variable* variable = NULL;
		if(isdigit(variable_nom)){
			variable = obtener_argumento(variable_nom);
		} else {
			variable = obtener_variable(variable_nom);
		}

		dir_memoria* dir = &(variable->direccion);
		puntero = direccion_logica_a_puntero(dir);
		logTrace("Direccion logica a puntero: Pag:%d,Off:%d,Size:%d, puntero:%d",dir->pagina,dir->offset,dir->size,puntero);
	}
	return puntero;
}

t_valor_variable ml_dereferenciar(t_puntero puntero) {
	t_valor_variable valor = -1;
	if(!hubo_exception){
		dir_memoria* dir = puntero_a_direccion_logica(puntero);
		logTrace("Puntero a Direccion logica: puntero:%d, Pag:%d,Off:%d,Size:%d",puntero,dir->pagina,dir->offset,dir->size);
		char* contenido = NULL;
		int resultado = leer_pagina(dir->pagina,dir->offset,dir->size,&contenido);
		logTrace("Resultado Lectura en UMC: %d",resultado);


		if(resultado>0){
			memcpy(&valor,contenido,sizeof(uint32_t));
			free(contenido);
		} else {
			informarException();
		}
		free(dir);

		logTrace("Dereferenciar %d y su valor es: %d", puntero, valor);
	}
	return valor;
}

void ml_asignar(t_puntero puntero, t_valor_variable variable) {
	if(!hubo_exception){
		logTrace("Asignando en %d el valor %d", puntero, variable);

		dir_memoria* dir = puntero_a_direccion_logica(puntero);
		logTrace("Puntero a Direccion logica: puntero:%d, Pag:%d,Off:%d,Size:%d",puntero,dir->pagina,dir->offset,dir->size);

		escribir_variable_en_umc(variable,dir);

		free(dir);
	}
}

void ml_imprimir(t_valor_variable valor) {
	logTrace("Imprimir %d", valor);
	informarNucleoImprimirVariable(socketNucleo,pcbActual->processID,valor);
}

void ml_imprimirTexto(char* texto) {
	logTrace("ImprimirTexto: %s", texto);
	informarNucleoImprimirTexto(socketNucleo,pcbActual->processID,texto);
}

t_valor_variable ml_obtenerValorCompartida(t_nombre_compartida variable){
	logTrace("Obteniendo Valor Variable compartida: %s",variable);
	t_valor_variable valor = getValorCompartida(socketNucleo,variable);
	return valor;
}

t_valor_variable ml_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	logTrace("Asignando Valor Variable compartida: %s, valor: %d",variable,valor);
	setValorCompartida(socketNucleo,variable,valor);
	return valor;
}

void ml_irAlLabel(t_nombre_etiqueta nombre_etiqueta){
	logTrace("Ejecutando Ir a Label: %s",nombre_etiqueta);
	t_puntero_instruccion  pos = obtenerIndiceInstruccion(pcbActual->codeIndex->etiquetas, nombre_etiqueta, pcbActual->codeIndex->etiquetas_size);
	logTrace("Ir a direccion: %d",pos);
	pcbActual->programCounter = pos;
}

void ml_llamarSinRetorno(t_nombre_etiqueta etiqueta){
	logTrace("Ejecutando Llamar sin retorno a funcion: %s (No implementado)",etiqueta);
}

void ml_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	logTrace("Ejecutando Llamar con retorno a funcion: %s, Retorno: %d",etiqueta,donde_retornar);
	t_puntero_instruccion  pos = obtenerIndiceInstruccion(pcbActual->codeIndex->etiquetas, etiqueta, pcbActual->codeIndex->etiquetas_size);
	logTrace("Indice de la funcion: %d",pos);
	crearNuevoContexto(pcbActual);
	contexto* contexto = getContextoActual();
	contexto->retPos = pcbActual->programCounter;
	pcbActual->programCounter = pos;
	dir_memoria* dir = puntero_a_direccion_logica(donde_retornar);
	contexto->retVar.pagina = dir->pagina;
	contexto->retVar.offset = dir->offset;
	contexto->retVar.size = dir->size;
	free(dir);
}

void ml_finalizar(void){
	logTrace("Ejecutando Finalizar");
	programa_finalizado = 1;
}

void ml_retornar(t_valor_variable retorno){
	logTrace("Ejecutando Retornar");
	dir_memoria* dir = &(getContextoActual()->retVar);
	logTrace("Escribir en variable (%d,%d,%d) el valor de retorno: %d",dir->pagina,dir->offset,dir->size,retorno);

	escribir_variable_en_umc(retorno,dir);

	destruirContextoActual(pcbActual,size_pagina);
}

void ml_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo){
	logTrace("Ejecutando Entrada-Salida [Dispositivo: %s, Tiempo: %d]",dispositivo,tiempo);
	ejecutarOperacionIO(dispositivo,tiempo);
}

void ml_wait(t_nombre_semaforo identificador_semaforo){
	logTrace("Ejecutando Wait de semaforo: %s",identificador_semaforo);
	execute_wait(identificador_semaforo);
}

void ml_signal(t_nombre_semaforo identificador_semaforo){
	logTrace("Ejecutando Signal de semaforo: %s",identificador_semaforo);
	execute_signal(identificador_semaforo);
}
