/*
 * stack.c
 *
 *  Created on: 31/5/2016
 *      Author: utnso
 */

#include <stdlib.h>
#include "stack.h"


t_stack *stack_create() {
	t_stack* stack = malloc(sizeof(t_stack));
	t_list* elements = list_create();
	stack->elements = elements;
	return stack;
}

void stack_clean(t_stack *self) {
	list_clean(self->elements);
}

void stack_clean_and_destroy_elements(t_stack *self, void(*element_destroyer)(void*)) {
	list_clean_and_destroy_elements(self->elements, element_destroyer);
}

void stack_destroy(t_stack *self) {
	list_destroy(self->elements);
	free(self);
}

void stack_destroy_and_destroy_elements(t_stack *self, void(*element_destroyer)(void*)) {
	list_destroy_and_destroy_elements(self->elements, element_destroyer);
	free(self);
}

void stack_push(t_stack *self, void *element) {
	list_add(self->elements, element);
}

void *stack_pop(t_stack *self) {
	return list_remove(self->elements, self->elements->elements_count-1);
}

void *stack_peek(t_stack *self) {
	return list_get(self->elements, self->elements->elements_count-1);
}

int stack_size(t_stack* self) {
	return list_size(self->elements);
}

int stack_is_empty(t_stack *self) {
	return list_is_empty(self->elements);
}
