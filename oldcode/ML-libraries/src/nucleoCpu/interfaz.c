/*
 * interfaz.c
 *
 *  Created on: 26/5/2016
 *      Author: utnso
 */

#include "interfaz.h"

//funciones serializacion del PCB

void serializarDato(char* buffer, void* dato, int size_to_send, int* offset){
	memcpy(buffer + *offset, dato, size_to_send);
	*offset += size_to_send;
}

void deserializarDato(void* dato, char* buffer, int size, int* offset){
	memcpy(dato,buffer + *offset,size);
	*offset += size;
}

char* serializarPCB(PCB* pcb){
	uint32_t total_size = getLong_PCB(pcb);

	char *serializedPackage = malloc(total_size);

	int offset = 0;
	int size_to_send;

	serializarDato(serializedPackage,&(pcb->processID),sizeof(uint32_t),&offset);
	serializarDato(serializedPackage,&(pcb->programCounter),sizeof(uint32_t),&offset);
	serializarDato(serializedPackage,&(pcb->stackFirstPage),sizeof(uint32_t),&offset);
	serializarDato(serializedPackage,&(pcb->stackOffset),sizeof(uint32_t),&offset);

	//serializar code index
	uint32_t size_metadata_program = getLong_metadata_program(pcb->codeIndex);
	serializarDato(serializedPackage,&(size_metadata_program),sizeof(uint32_t),&offset);

	char* serialized_metadata = serializar_metadata_program(pcb->codeIndex);
	serializarDato(serializedPackage,serialized_metadata,sizeof(char)*size_metadata_program,&offset);
	free(serialized_metadata);

	//cantidad de contextos
	serializarDato(serializedPackage,&(pcb->context_len),sizeof(uint32_t),&offset);

	//serializar stack
	uint32_t size_stack = getLong_stack(pcb->stackIndex,pcb->context_len);
	serializarDato(serializedPackage,&(size_stack),sizeof(uint32_t),&offset);

	char* serialized_stack = serializar_stack(&(pcb->stackIndex),pcb->context_len);
	serializarDato(serializedPackage,serialized_stack,sizeof(char)*size_stack,&offset);
	free(serialized_stack);

	//TODO: borrar (solo para probar al principio le mando el programa hasta que funcione la UMC)
	size_to_send = strlen(pcb->programa)+1;
	memcpy(serializedPackage + offset, pcb->programa, size_to_send);

	return serializedPackage;
}

uint32_t getLong_PCB(PCB* pcb){
	uint32_t total_size = 0;
	uint32_t size_metadata_program = getLong_metadata_program(pcb->codeIndex);

	total_size += sizeof(uint32_t)*4;//PID + PC + stackFirstPage + stackOffset
	total_size += sizeof(uint32_t);//campo size_metadata_program
	total_size += size_metadata_program;

	uint32_t size_stack = getLong_stack(pcb->stackIndex,pcb->context_len);
	total_size += sizeof(uint32_t)*2;//campo context_len y size_stack
	total_size += size_stack;


	//TODO: borrar (solo para probar al principio le mando el programa hasta que funcione la UMC)
	total_size += strlen(pcb->programa)+1;

	return total_size;
}

PCB* deserializar_PCB(char* serialized){
	PCB* pcb = malloc(sizeof(PCB));
	int offset = 0;

	deserializarDato(&(pcb->processID),serialized,sizeof(uint32_t),&offset);
	deserializarDato(&(pcb->programCounter),serialized,sizeof(uint32_t),&offset);
	deserializarDato(&(pcb->stackFirstPage),serialized,sizeof(uint32_t),&offset);
	deserializarDato(&(pcb->stackOffset),serialized,sizeof(uint32_t),&offset);

	uint32_t size_metadata_program;
	deserializarDato(&size_metadata_program,serialized,sizeof(uint32_t),&offset);

	char* serialized_metadata = malloc(sizeof(char)*size_metadata_program);
	deserializarDato(serialized_metadata,serialized,size_metadata_program,&offset);
	pcb->codeIndex = deserializar_metadata_program(serialized_metadata);
	free(serialized_metadata);

	//cantidad de contextos
	deserializarDato(&(pcb->context_len),serialized,sizeof(uint32_t),&offset);

	uint32_t size_stack;
	deserializarDato(&size_stack,serialized,sizeof(uint32_t),&offset);

	char* serialized_stack = malloc(sizeof(char)*size_stack);
	deserializarDato(serialized_stack,serialized,size_stack,&offset);
	pcb->stackIndex = deserializar_stack(serialized_stack,pcb->context_len);
	free(serialized_stack);

	//TODO: borrar (solo para probar al principio le mando el programa hasta que funcione la UMC)
	pcb->programa = strdup(serialized+offset);

	return pcb;
}

char* serializar_metadata_program(t_metadata_program* metadata){
	int total_size = getLong_metadata_program(metadata);

	char *serialized = malloc(total_size);

	int offset = 0;
	int size_to_send;

	//campo instruccion_inicio
	serializarDato(serialized,&(metadata->instruccion_inicio),sizeof(metadata->instruccion_inicio),&offset);
	//campo instrucciones_size
	serializarDato(serialized,&(metadata->instrucciones_size),sizeof(metadata->instrucciones_size),&offset);
	//array instrucciones_serializado
	int i;
	for(i=0; i < metadata->instrucciones_size; i++){
		serializarDato(serialized,&(metadata->instrucciones_serializado[i].start),sizeof(metadata->instrucciones_serializado[i].start),&offset);
		serializarDato(serialized,&(metadata->instrucciones_serializado[i].offset),sizeof(metadata->instrucciones_serializado[i].offset),&offset);
	}
	//campo etiquetas_size
	serializarDato(serialized,&(metadata->etiquetas_size),sizeof(metadata->etiquetas_size),&offset);
	//campo string etiquetas
	serializarDato(serialized,metadata->etiquetas,sizeof(char)*metadata->etiquetas_size,&offset);
	//campo cantidad_de_funciones
	serializarDato(serialized,&(metadata->cantidad_de_funciones),sizeof(metadata->cantidad_de_funciones),&offset);
	//campo cantidad_de_etiquetas
	serializarDato(serialized,&(metadata->cantidad_de_etiquetas),sizeof(metadata->cantidad_de_etiquetas),&offset);

	return serialized;
}

uint32_t getLong_metadata_program(t_metadata_program* metadata){
	uint32_t total_size = 0;
	total_size += sizeof(metadata->instruccion_inicio);//inst_inicio
	total_size += sizeof(metadata->instrucciones_size);//size int32
	total_size += sizeof(t_intructions)*metadata->instrucciones_size;//size indice de codigo
	total_size += sizeof(metadata->etiquetas_size);//size etiquetas
	total_size += sizeof(char)*metadata->etiquetas_size;//string de etiquetas serializado
	total_size += sizeof(int)*2;//cant funciones y etiquetas
	return total_size;
}

t_metadata_program* deserializar_metadata_program(char* serialized){
	t_metadata_program* metadata = malloc(sizeof(t_metadata_program));
	int offset = 0;
	int size_aux;

	size_aux = sizeof(metadata->instruccion_inicio);
	memcpy(&(metadata->instruccion_inicio),serialized+offset,size_aux);
	offset+=size_aux;
	size_aux = sizeof(metadata->instrucciones_size);
	memcpy(&(metadata->instrucciones_size),serialized+offset,size_aux);
	offset+=size_aux;

	metadata->instrucciones_serializado = malloc(sizeof(t_intructions)*metadata->instrucciones_size);
	int i;
	for(i=0; i < metadata->instrucciones_size; i++){
		size_aux = sizeof(metadata->instrucciones_serializado[i].start);
		memcpy(&(metadata->instrucciones_serializado[i].start),serialized+offset,size_aux);
		offset+=size_aux;
		size_aux = sizeof(metadata->instrucciones_serializado[i].offset);
		memcpy(&(metadata->instrucciones_serializado[i].offset),serialized+offset,size_aux);
		offset+=size_aux;
	}

	size_aux = sizeof(metadata->etiquetas_size);
	memcpy(&(metadata->etiquetas_size),serialized+offset,size_aux);
	offset+=size_aux;

	metadata->etiquetas = malloc(sizeof(char)*metadata->etiquetas_size);
	memcpy(metadata->etiquetas,serialized+offset,sizeof(char)*metadata->etiquetas_size);

	size_aux = sizeof(metadata->cantidad_de_funciones);
	memcpy(&(metadata->cantidad_de_funciones),serialized+offset,size_aux);
	offset+=size_aux;
	size_aux = sizeof(metadata->cantidad_de_etiquetas);
	memcpy(&(metadata->cantidad_de_etiquetas),serialized+offset,size_aux);
	offset+=size_aux;

	return metadata;
}

//envio la solicitud IO mas el PCB entero para hacer el context switch y que se bloquee en el Nucleo
char* serializar_ejecutarOperacionIO(PCB* pcb, char* io_id, uint32_t cant_operaciones){
	uint32_t size_pcb = getLong_PCB(pcb);
	uint32_t total_size = getLong_ejecutarOperacionIO(pcb,io_id,cant_operaciones);

	char *serializedPackage = malloc(total_size);

	int offset = 0;
	int size_to_send;

	uint32_t io_id_length = strlen(io_id)+1;
	serializarDato(serializedPackage,&(io_id_length),sizeof(uint32_t),&offset);
	serializarDato(serializedPackage,io_id,io_id_length,&offset);
	serializarDato(serializedPackage,&(cant_operaciones),sizeof(uint32_t),&offset);
	serializarDato(serializedPackage,&(size_pcb),sizeof(uint32_t),&offset);

	char* serialized_pcb = serializarPCB(pcb);
	serializarDato(serializedPackage,serialized_pcb,sizeof(char)*size_pcb,&offset);
	free(serialized_pcb);

	return serializedPackage;
}

uint32_t getLong_ejecutarOperacionIO(PCB* pcb, char* io_id, uint32_t cant_operaciones){
	return strlen(io_id)+1 + sizeof(uint32_t)*3 + getLong_PCB(pcb);
}

solicitud_io* deserializar_ejecutarOperacionIO(char* serialized){
	solicitud_io* sol = malloc(sizeof(solicitud_io));
	int offset = 0;

	uint32_t io_id_length;
	memcpy(&(io_id_length),serialized+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	sol->io_id = strdup(serialized+offset);
	offset+=io_id_length;
	memcpy(&(sol->cant_operaciones),serialized+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	int size_pcb;
	memcpy(&size_pcb,serialized+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	char* serialized_pcb = malloc(sizeof(char)*size_pcb);
	memcpy(serialized_pcb,serialized+offset,size_pcb);
	offset+=size_pcb;

	sol->pcb = deserializar_PCB(serialized_pcb);
	free(serialized_pcb);

	return sol;
}


char* serializar_contexto(contexto* contexto){
	uint32_t total_size = getLong_contexto(contexto);
	char *serializedPackage = malloc(sizeof(char)*total_size);

	int offset = 0;

	//cantidad de argumentos
	serializarDato(serializedPackage,&(contexto->arg_len),sizeof(uint32_t),&offset);

	//serializar array de argumentos
	char* serialized_dictionary = serializar_array_variables(&(contexto->argumentos),contexto->arg_len);
	serializarDato(serializedPackage,serialized_dictionary,sizeof(variable)*contexto->arg_len,&offset);
	free(serialized_dictionary);

	//cantidad de variables
	serializarDato(serializedPackage,&(contexto->var_len),sizeof(uint32_t),&offset);

	//serializar array de variables
	serialized_dictionary = serializar_array_variables(&(contexto->variables),contexto->var_len);
	serializarDato(serializedPackage,serialized_dictionary,sizeof(variable)*contexto->var_len,&offset);
	free(serialized_dictionary);

	//posicion de retorno
	serializarDato(serializedPackage,&(contexto->retPos),sizeof(t_puntero_instruccion),&offset);

	//direccion de variable de retorno
	serializarDato(serializedPackage,&(contexto->retVar.pagina),sizeof(uint32_t),&offset);
	serializarDato(serializedPackage,&(contexto->retVar.offset),sizeof(uint32_t),&offset);
	serializarDato(serializedPackage,&(contexto->retVar.size),sizeof(uint32_t),&offset);

	return serializedPackage;
}

uint32_t getLong_contexto(contexto* contexto){
	uint32_t longitud = 0;
	longitud += sizeof(uint32_t);
	longitud += sizeof(variable)*contexto->arg_len;
	longitud += sizeof(uint32_t);
	longitud += sizeof(variable)*contexto->var_len;
	longitud += sizeof(t_puntero_instruccion);
	longitud += sizeof(dir_memoria);
	return longitud;
}

contexto* deserializar_contexto(char* serialized){
	contexto* context = malloc(sizeof(contexto));
	int offset = 0;

	//cantidad de argumentos
	deserializarDato(&(context->arg_len),serialized,sizeof(uint32_t),&offset);

	char* serialized_dictionary = malloc((sizeof(variable))*context->arg_len);
	deserializarDato(serialized_dictionary,serialized,(sizeof(variable))*context->arg_len,&offset);

	context->argumentos = deserializar_array_variables(serialized_dictionary,context->arg_len);
	free(serialized_dictionary);

	//cantidad de variables
	deserializarDato(&(context->var_len),serialized,sizeof(uint32_t),&offset);

	serialized_dictionary = malloc((sizeof(variable))*context->var_len);
	deserializarDato(serialized_dictionary,serialized,(sizeof(variable))*context->var_len,&offset);

	context->variables = deserializar_array_variables(serialized_dictionary,context->var_len);
	free(serialized_dictionary);

	deserializarDato(&(context->retPos),serialized,sizeof(t_puntero_instruccion),&offset);

	deserializarDato(&(context->retVar.pagina),serialized,sizeof(uint32_t),&offset);
	deserializarDato(&(context->retVar.offset),serialized,sizeof(uint32_t),&offset);
	deserializarDato(&(context->retVar.size),serialized,sizeof(uint32_t),&offset);

	return context;
}

char* serializar_array_variables(variable** variables, uint32_t len){
	char *serializedPackage = malloc(sizeof(variable)*len);
	variable* aux_variables = *variables;

	int offset = 0;

	int i;
	for (i = 0; i < len; i++) {
		serializarDato(serializedPackage,&(aux_variables[i].nombre),sizeof(char),&offset);
		serializarDato(serializedPackage,&(aux_variables[i].direccion.pagina),sizeof(uint32_t),&offset);
		serializarDato(serializedPackage,&(aux_variables[i].direccion.offset),sizeof(uint32_t),&offset);
		serializarDato(serializedPackage,&(aux_variables[i].direccion.size),sizeof(uint32_t),&offset);
	}
	return serializedPackage;
}

variable* deserializar_array_variables(char* serialized, uint32_t len){
	int offset = 0;

	variable* variables = NULL;
	int i;
	for(i=0; i<len; i++){
		variables = realloc(variables,sizeof(variable)*(i+1));
		deserializarDato(&(variables[i].nombre),serialized,sizeof(char),&offset);
		deserializarDato(&(variables[i].direccion.pagina),serialized,sizeof(uint32_t),&offset);
		deserializarDato(&(variables[i].direccion.offset),serialized,sizeof(uint32_t),&offset);
		deserializarDato(&(variables[i].direccion.size),serialized,sizeof(uint32_t),&offset);
	}
	return variables;
}

char* serializar_stack(contexto** contextos, uint32_t contextos_length){
	contexto* aux_contextos = *contextos;
	uint32_t total_size = getLong_stack(aux_contextos, contextos_length);
	char *serializedPackage = malloc(sizeof(char)*total_size);

	int offset = 0;

	int i;
	for (i = 0; i < contextos_length; i++) {
		char* serialized_contexto = serializar_contexto(&aux_contextos[i]);//TODO: ver como pasarle el puntero como parametro
		uint32_t size_contexto = getLong_contexto(&aux_contextos[i]);
		serializarDato(serializedPackage,&(size_contexto),sizeof(uint32_t),&offset);//size contexto
		serializarDato(serializedPackage,serialized_contexto,sizeof(char)*size_contexto,&offset);//contexto
		free(serialized_contexto);
	}
	return serializedPackage;
}

uint32_t getLong_stack(contexto* contextos, uint32_t contextos_length){
	uint32_t total = 0;
	int i;
	for(i=0; i<contextos_length; i++){
		total += sizeof(uint32_t);
		total += getLong_contexto(&(contextos[i]));
	}
	return total;
}

contexto* deserializar_stack(char* serialized, uint32_t contextos_length){
	int offset = 0;

	//contexto* contextos = NULL;
	contexto* contextos = malloc(sizeof(contexto)*contextos_length);
	int i;
	for (i = 0; i < contextos_length; i++) {
		uint32_t size_contexto;
		deserializarDato(&(size_contexto),serialized,sizeof(uint32_t),&offset);
		char* serialized_contexto = malloc(sizeof(char)*size_contexto);
		//contextos = realloc(contextos,sizeof(contexto)*(i+1));
		deserializarDato(serialized_contexto,serialized,size_contexto,&offset);
		contexto* aux = deserializar_contexto(serialized_contexto);
		contextos[i] = *(aux);
		free(aux);
		free(serialized_contexto);
	}
	return contextos;
}


void crearNuevoContexto(PCB* pcb){
	pcb->stackIndex = realloc(pcb->stackIndex,sizeof(contexto)*(pcb->context_len+1));
	pcb->stackIndex[pcb->context_len].arg_len = 0;
	pcb->stackIndex[pcb->context_len].argumentos = NULL;
	pcb->stackIndex[pcb->context_len].var_len = 0;
	pcb->stackIndex[pcb->context_len].variables = NULL;
	pcb->context_len++;
}

void destruirContextoActual(PCB* pcb, int size_pagina){
	pcb->context_len--;
	contexto* contextoActual = &(pcb->stackIndex[pcb->context_len]);
	//pongo el program counter en la direccion de retorno de la funcion
	pcb->programCounter = contextoActual->retPos;
	//libero variables y argumentos
	if(contextoActual->variables!=NULL){
		free(contextoActual->variables);
	}
	if(contextoActual->argumentos!=NULL){
		free(contextoActual->argumentos);
	}
	//reestablezco el valor del stack offset para que se pueda volver a usar el espacio para crear variables
	pcb->stackOffset -= (contextoActual->var_len*sizeof(uint32_t));
	pcb->stackOffset -= (contextoActual->arg_len*sizeof(uint32_t));
	//libero el ultimo contexto
	pcb->stackIndex = realloc(pcb->stackIndex,sizeof(contexto)*(pcb->context_len));
}

void destroy_stackIndex(contexto* contexto, uint32_t context_len){
	int i;
	for(i=0; i<context_len; i++){
		if(contexto[i].variables!=NULL && contexto[i].var_len>0){
			free(contexto[i].variables);
		}
		if(contexto[i].argumentos!=NULL && contexto[i].arg_len>0){
			free(contexto[i].argumentos);
		}
	}
	free(contexto);
}

void destroyPCB(PCB* self){
	if(self!=NULL){
		free(self->programa);
		metadata_destruir(self->codeIndex);
		destroy_stackIndex(self->stackIndex,self->context_len);
		free(self);
		self=NULL;
	}
}


char* serializar_imprimirVariable(uint32_t pid, uint32_t valor){
	char *serializedPackage = malloc(sizeof(uint32_t)*2);
	int offset = 0;
	serializarDato(serializedPackage,&(pid),sizeof(uint32_t),&offset);
	serializarDato(serializedPackage,&(valor),sizeof(uint32_t),&offset);
	return serializedPackage;
}

print_var* deserializar_imprimirVariable(char* serialized){
	print_var* print = malloc(sizeof(print_var));
	int offset = 0;
	deserializarDato(&(print->pid),serialized,sizeof(uint32_t),&offset);
	deserializarDato(&(print->valor),serialized,sizeof(uint32_t),&offset);
	return print;
}

char* serializar_imprimirTexto(uint32_t pid, char* texto){
	char *serializedPackage = malloc(sizeof(uint32_t)+strlen(texto)+sizeof(char));
	int offset = 0;
	serializarDato(serializedPackage,&(pid),sizeof(uint32_t),&offset);
	serializarDato(serializedPackage,texto,strlen(texto)+sizeof(char),&offset);
	return serializedPackage;
}

print_text* deserializar_imprimirTexto(char* serialized){
	print_text* print = malloc(sizeof(print_text));
	int offset = 0;
	deserializarDato(&(print->pid),serialized,sizeof(uint32_t),&offset);
	print->text = strdup(serialized+offset);
	return print;
}

void destroy_print_text(print_text* self){
	free(self->text);
	free(self);
}

void destroy_print_var(print_var* self){
	free(self);
}

char* serializar_imprimirVariable_consola(uint32_t valor){
	char *serializedPackage = malloc(sizeof(uint32_t));
	int offset = 0;
	serializarDato(serializedPackage,&(valor),sizeof(uint32_t),&offset);
	return serializedPackage;
}

uint32_t deserializar_imprimirVariable_consola(char* serialized){
	int offset = 0;
	uint32_t valor;
	deserializarDato(&(valor),serialized,sizeof(uint32_t),&offset);
	return valor;
}

char* serializar_semaforo(uint32_t pid, char* sem_id){
	char *serializedPackage = malloc(sizeof(uint32_t)+strlen(sem_id)+sizeof(char));
	int offset = 0;
	serializarDato(serializedPackage,&(pid),sizeof(uint32_t),&offset);
	serializarDato(serializedPackage,sem_id,strlen(sem_id)+sizeof(char),&offset);
	return serializedPackage;
}

sem_action* deserializar_semaforo(char* serialized){
	sem_action* sem = malloc(sizeof(sem_action));
	int offset = 0;
	deserializarDato(&(sem->pid),serialized,sizeof(uint32_t),&offset);
	sem->sem_id = strdup(serialized+offset);
	return sem;
}

uint32_t getLong_semaforo(char* sem_id){
	return sizeof(uint32_t)+strlen(sem_id)+sizeof(char);
}

void destroy_sem_action(sem_action* self){
	free(self->sem_id);
	free(self);
}

char* serializar_shared_var(uint32_t valor, char* var_id){
	char *serializedPackage = malloc(sizeof(uint32_t)+strlen(var_id)+sizeof(char));
	int offset = 0;
	serializarDato(serializedPackage,&(valor),sizeof(uint32_t),&offset);
	serializarDato(serializedPackage,var_id,strlen(var_id)+sizeof(char),&offset);
	return serializedPackage;
}

shared_var* deserializar_shared_var(char* serialized){
	shared_var* var = malloc(sizeof(shared_var));
	int offset = 0;
	deserializarDato(&(var->value),serialized,sizeof(uint32_t),&offset);
	var->var_id = strdup(serialized+offset);
	return var;
}

uint32_t getLong_shared_var(char* var_id){
	return sizeof(uint32_t)+strlen(var_id)+sizeof(char);
}

void destroy_shared_var(shared_var* self){
	free(self->var_id);
	free(self);
}






//funciones interfaz CPU a Nucleo

void informarNucleoFinPrograma(int socketNucleo, PCB* pcb){
	char* serialized = serializarPCB(pcb);
	uint32_t longitud = getLong_PCB(pcb);
	enviarMensajeSocketConLongitud(socketNucleo,PROGRAM_FINISHED,serialized,longitud);
	free(serialized);
}

void informarNucleoQuantumFinished(int socketNucleo, PCB* pcb){
	char* tmp = string_itoa(pcb->processID);
	enviarMensajeSocket(socketNucleo,QUANTUM_FINISHED,tmp);
	free(tmp);
}

void informarNucleoContextSwitchFinished(int socketNucleo, PCB* pcb){
	char* serialized = serializarPCB(pcb);
	uint32_t longitud = getLong_PCB(pcb);
	enviarMensajeSocketConLongitud(socketNucleo,CONTEXT_SWITCH_FINISHED,serialized,longitud);
	free(serialized);
}

void informarNucleoCPUdisconnectedBySignal(int socketNucleo, PCB* pcb){
	char* serialized = serializarPCB(pcb);
	uint32_t longitud = getLong_PCB(pcb);
	enviarMensajeSocketConLongitud(socketNucleo,CPU_SIGNAL_DISCONNECTED,serialized,longitud);
	free(serialized);
}

void informarNucleoCPUlibre(int socketNucleo){
	enviarMensajeSocket(socketNucleo,CPU_LIBRE,"");
}

void informarNucleoEjecutarOperacionIO(int socketNucleo, PCB* pcb, char* io_id, uint32_t cant_operaciones){
	char* serialized = serializar_ejecutarOperacionIO(pcb,io_id,cant_operaciones);
	uint32_t longitud = getLong_ejecutarOperacionIO(pcb,io_id,cant_operaciones);
	enviarMensajeSocketConLongitud(socketNucleo,EXEC_IO_OPERATION,serialized,longitud);
	free(serialized);
}

void informarNucleoImprimirVariable(int socketNucleo, uint32_t pid, t_valor_variable valor){
	char* serialized = serializar_imprimirVariable(pid,valor);
	enviarMensajeSocketConLongitud(socketNucleo,PRINT_VARIABLE,serialized,sizeof(uint32_t)*2);
	free(serialized);
}

void informarNucleoImprimirTexto(int socketNucleo, uint32_t pid, char* texto){
	char* serialized = serializar_imprimirTexto(pid,texto);
	enviarMensajeSocketConLongitud(socketNucleo,PRINT_TEXT,serialized,sizeof(uint32_t)+strlen(texto)+sizeof(char));
	free(serialized);
}

uint32_t getValorCompartida(int socketNucleo, char* var_id){
	if(var_id[strlen(var_id)-1]=='\n'){
		var_id[strlen(var_id)-1]='\0';
	}
	Package* package = createPackage();
	uint32_t valor;
	enviarMensajeSocket(socketNucleo,GET_SHARED_VAR,var_id);
	if(recieve_and_deserialize(package,socketNucleo) > 0)
	{
		if(package->msgCode==GET_SHARED_VAR){
			memcpy(&valor,package->message,sizeof(uint32_t));
		}
	}
	destroyPackage(package);
	return valor;
}

void setValorCompartida(int socketNucleo, char* var_id, uint32_t valor){
	if(var_id[strlen(var_id)-1]=='\n'){
		var_id[strlen(var_id)-1]='\0';
	}
	char* serialized = serializar_shared_var(valor,var_id);
	uint32_t length = getLong_shared_var(var_id);
	enviarMensajeSocketConLongitud(socketNucleo,SET_SHARED_VAR,serialized,length);
	free(serialized);
}


//funciones interfaz Nucleo a CPU

void ejecutarNuevoProcesoCPU(int socketCPU, PCB* pcb){
	char* serialized = serializarPCB(pcb);
	int longitud = getLong_PCB(pcb);
	enviarMensajeSocketConLongitud(socketCPU,EXEC_NEW_PROCESS,serialized,longitud);
	free(serialized);
}

void continuarEjecucionProcesoCPU(int socketCPU){
	enviarMensajeSocket(socketCPU,CONTINUE_EXECUTION,"");
}

PCB* informarCPUbloqueoSemaforo(int socketCPU){
	Package* package = createPackage();
	PCB* pcb = NULL;

	enviarMensajeSocket(socketCPU,CONTEXT_SWITCH_SEM_BLOCKED,"");

	if(recieve_and_deserialize(package,socketCPU) > 0)
	{
		if(package->msgCode==CONTEXT_SWITCH_FINISHED){
			pcb = deserializar_PCB(package->message);
		}
	}
	destroyPackage(package);
	return pcb;
}

void informarValorVariableCompartida(int socketCPU, uint32_t valor){
	char* serialized = malloc(sizeof(uint32_t));
	memcpy(serialized,&valor,sizeof(uint32_t));
	enviarMensajeSocketConLongitud(socketCPU,GET_SHARED_VAR,serialized,sizeof(uint32_t));
	free(serialized);
}
