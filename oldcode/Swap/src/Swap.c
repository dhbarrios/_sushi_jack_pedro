/*
 ============================================================================
 Name        : Swap.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "Swap.h"

pagina escribir(char* cad, int size);
void imprimirBitmap();
void imprimirTablaPaginas();
void crearProgramaTest(int,int,int);

int main(int argc, char *argv[]) {

	Configuration* config = configurar(argc > 1 ? argv[1] : NULL);

	//creo el log
	initLogMutex(config->log_file, config->log_program_name, config->log_print_console, log_level_from_string(config->log_level));

	inicializarSwap(config);

	/*
	crearProgramaTest(1,3,config->size_pagina);
	crearProgramaTest(2,2,config->size_pagina);
	crearProgramaTest(3,2,config->size_pagina);
	crearProgramaTest(4,2,config->size_pagina);
	crearProgramaTest(5,3,config->size_pagina);

	eliminarPrograma(2);
	eliminarPrograma(4);

	imprimirBitmap();
	imprimirTablaPaginas();

	crearProgramaTest(6,4,config->size_pagina);

	imprimirBitmap();
	imprimirTablaPaginas();


	pagina page3 = escribir("z",config->size_pagina);
	escribirPaginaDeProceso(5,2,page3);

	//sobreescribo una pagina
	pagina page2 = leerPaginaDeProceso(5,2);
	printf("\nPagina %d leida\n%s\n",2,page2);

	pagina page4 = leerPaginaDeProceso(1,0);
	printf("\nPagina %d leida\n%s\n",0,page4);
	*/

	handleUMCRequests(config);

	destroyTabla();
	cerrarArchivoSwap();

	logDestroy();

	return EXIT_SUCCESS;
}

//hace un relleno de pagina con la letra pasada en "cad" (para probar)
pagina escribir(char* cad, int size){
	pagina page = malloc(sizeof(char)*size);
	int j;
	page[0] = '\0';
	for(j=0; j<size; j++){
		strcat(page,cad);
	}
	page[j-1] = '\0';
	return page;
}

void imprimirBitmap(){
	//imprimo en consola el contenido del bitmap
	t_bitarray* bitMap = getBitMap();
	int i;
	printf("\nBitMap\n");
	printf("Frame:\t");

	for(i=0; i<bitMap->size; i++){
		printf("%d ",i);
	}
	printf("\nOcup:\t");
	for(i=0; i<bitMap->size; i++){
		printf("%d ",bitarray_test_bit(bitMap,i));
		if(i>=10)
			printf(" ");
		if(i>=100)
			printf(" ");
	}
}

void imprimirTablaPaginas(){
	//imprimo en consola el contenido de la tabla de paginas
	t_bitarray* bitMap = getBitMap();
	tableRow* tabla = getTablaDePaginas();
	int i;
	printf("\n\nTabla de paginas\n");
	printf("Frame:\t");
	for(i=0; i<bitMap->size; i++){
		printf("%d   ",i);
	}
	printf("\nPID:\t");
	for(i=0; i<bitMap->size; i++){
		printf("%d  ",tabla[i].pid);
		if(tabla[i].pid>=0)
			printf(" ");
		if(i>=10)
			printf(" ");
		if(i>=100)
			printf(" ");
	}
	printf("\nPage:\t");
	for(i=0; i<bitMap->size; i++){
		printf("%d  ",tabla[i].page);
		if(tabla[i].page>=0)
			printf(" ");
		if(i>=10)
			printf(" ");
		if(i>=100)
			printf(" ");
	}
	printf("\n\n");
}

void crearProgramaTest(int pid, int cantidadPaginas, int size_pagina){
	//Simulo la creacion de un programa
	pagina* paginas = malloc(sizeof(pagina)*cantidadPaginas);
	int i;
	for(i=0; i<cantidadPaginas; i++){
		paginas[i] = escribir("a",size_pagina);
	}

	int espacio = getFirstAvailableBlock(cantidadPaginas);
	if(espacio==-2){
		compactarSwap();
		imprimirBitmap();
		imprimirTablaPaginas();
		espacio = getFirstAvailableBlock(cantidadPaginas);
	}
	printf("Primer espacio libre: %d\n",espacio);

	nuevoPrograma(espacio,pid,cantidadPaginas);

	//ya fueron reservadas las paginas, ahora escribo las paginas que forman el nuevo programa
	for(i=0; i<cantidadPaginas; i++){
		escribirPaginaDeProceso(pid,i,paginas[i]);
	}
	//libero
	for(i=0; i<cantidadPaginas; i++){
		free(paginas[i]);
	}
	free(paginas);
}
