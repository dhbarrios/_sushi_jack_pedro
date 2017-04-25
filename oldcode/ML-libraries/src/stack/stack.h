/*
 * stack.h
 *
 *  Created on: 31/5/2016
 *      Author: utnso
 */

#ifndef STACK_STACK_H_
#define STACK_STACK_H_

#include <commons/collections/list.h>

	typedef struct {
		t_list* elements;
	} t_stack;

	/**
	* @NAME: stack_create
	* @DESC: Crea y devuelve un puntero a una pila
	*/
	t_stack *stack_create();

	/**
	* @NAME: stack_destroy
	* @DESC: Destruye una pila.
	*/
	void stack_destroy(t_stack *);

	/**
	* @NAME: stack_destroy_and_destroy_elements
	* @DESC: Destruye una pila, recibiendo como argumento el metodo encargado de liberar cada
	* 		elemento de la pila.
	*/
	void stack_destroy_and_destroy_elements(t_stack*, void(*element_destroyer)(void*));

	/**
	* @NAME: stack_push
	* @DESC: Agrega un elemento al final de la pila
	*/
	void stack_push(t_stack *, void *element);

	/**
	* @NAME: stack_pop
	* @DESC: quita un elemento del final de la pila
	*/
	void *stack_pop(t_stack *);

	/**
	* @NAME: stack_peek
	* @DESC: Devuelve el primer elemento de la pila sin extraerlo
	*/
	void *stack_peek(t_stack *);

	/**
	* @NAME: stack_clean
	* @DESC: Elimina todos los elementos de la pila.
	*/
	void stack_clean(t_stack *);

	/**
	* @NAME: stack_clean_and_destroy_elements
	* @DESC: Elimina y destruye todos los elementos de la pila.
	*/
	void stack_clean_and_destroy_elements(t_stack *, void(*element_destroyer)(void*));

	/**
	* @NAME: stack_size
	* @DESC: Devuelve la cantidad de elementos de la pila
	*/
	int stack_size(t_stack *);

	/**
	* @NAME: stack_is_empty
	* @DESC: Verifica si la pila esta vacía
	*/
	int stack_is_empty(t_stack *);

#endif /* STACK_STACK_H_ */
