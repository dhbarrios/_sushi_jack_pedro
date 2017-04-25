/*
 * fileHandler.c
 *
 *  Created on: 5/5/2016
 *      Author: utnso
 */

#include "fileHandler.h"

t_bitarray* bitMap;
tableRow* tabla;
FILE* file;
Configuration* config;

void inicializarSwap(Configuration* conf){
	config = conf;
	logDebug("Inicializando estructuras %d marcos",config->cantidad_paginas);
	crearBitMap();
	crearTablaDePaginas();
	crearArchivoSwap();
}

void crearBitMap()
{
	char* bits = malloc(sizeof(char)*config->cantidad_paginas);
	int i;
	for(i=0; i<config->cantidad_paginas; i++){
		bits[i]=0;
	}
	bitMap = bitarray_create(bits,config->cantidad_paginas);
}

int getFirstAvailableBlock(int cantPaginas)
{
	//total(espacio libre fragmentado), cont(contador de espacio contiguo)
	int total = 0, cont = 0, i;

	for(i=0; i<bitMap->size; i++){
		//si encuentro un bit en 0 (espacio libre) incremento, si no reseteo el contiguo
		if(!bitarray_test_bit(bitMap,i))
		{
			cont++;
			total++;
		} else {
			cont = 0;
		}
		//si encuentro un espacio contiguo de la cantidad de paginas pedidas, corto el for
		if(cont>=cantPaginas)
		{
			i++;
			break;
		}
	}
	int retorna = -1;
	if(cont>=cantPaginas){//si corte porque hay un espacio, devuelvo la posicion a partir de la cual esta el espacio para empezar a escribir
		retorna = i-cantPaginas;
	} else if(total>=cantPaginas){//si hay lugar pero fragmentado, se devuelve -2 que es el codigo para saber si se puede defragmentar
		retorna = -2;
	} else {//devuelve -1 si no hay espacio
		retorna = -1;
	}
	return retorna;
}

int escribirPaginaEnFrame(int frame, pagina pag){
	if(fseek(file,frame*config->size_pagina,SEEK_SET)){
		return -1;//error
	}
	if(fwrite(pag,config->size_pagina,1,file)>0){
		return 0;
	} else {
		return -1;
	}
}

void escribirPaginasEnFrame(int frame, pagina* paginas, int cantPaginas)
{
	int i;
	for(i=0; i<cantPaginas; i++)
	{
		escribirPaginaEnFrame(frame+i,paginas[i]);
	}
}

int getFileSize(){
	fseek(file, 0, SEEK_END); // seek to end of file
	int size = ftell(file)/config->size_pagina; // get current file pointer
	fseek(file, 0, SEEK_SET);
	return size;
}


void cerrarArchivoSwap(){
	fclose(file);
}


pagina leerPaginaFromFrame(int frame){
	pagina pag = malloc(sizeof(char)*config->size_pagina);
	fseek(file,frame*config->size_pagina,SEEK_SET);
	fread(pag,config->size_pagina,1,file);
	return pag;
}

void crearArchivoSwap(){
	file = fopen(config->nombre_swap,"w+b");
}

void crearTablaDePaginas(){
	tabla = malloc(sizeof(tableRow)*config->cantidad_paginas);
	int i;
	for(i=0; i<config->cantidad_paginas; i++){
		tabla[i].pid = -1;
		tabla[i].page = -1;
	}
}

void destroyTabla(){
	free(tabla);
}

t_bitarray* getBitMap(){
	return bitMap;
}

tableRow* getTablaDePaginas(){
	return tabla;
}

void nuevoPrograma(int frame, int pid, int cantPaginas)
{
	int i,j=0;
	for(i=frame; i<(cantPaginas+frame); i++)
	{
		bitarray_set_bit(bitMap,i);
		tabla[i].pid = pid;
		tabla[i].page = j;
		j++;
	}
}

void eliminarPrograma(int pid)
{
	int i;
	for(i=0; i<config->cantidad_paginas; i++)
	{
		//si encuentra un registro con el mismo processID, hace la baja logica y libera el espacio en el bitmap
		if(tabla[i].pid==pid)
		{
			tabla[i].pid=-1;
			tabla[i].page=-1;
			bitarray_clean_bit(bitMap,i);
		}
	}
}

int buscarFramePorPagina(int pid, int pagina){
	int i;
	for(i=0; i<config->cantidad_paginas; i++){
		if(tabla[i].pid==pid && tabla[i].page==pagina){
			return i;//retorna la posicion en swap (frame)
		}
	}
	return -1;//retorna -1 si no encuentra la pagina del proceso en la tabla
}

int escribirPaginaDeProceso(int pid, int paginaNro, pagina pag){
	int frame = buscarFramePorPagina(pid,paginaNro);
	logDebug("Escritura: [PID: %d, Page: %d, F: %d]",pid,paginaNro,frame);
	if(frame>=0){
		return escribirPaginaEnFrame(frame,pag);
	}
}
pagina leerPaginaDeProceso(int pid, int paginaNro)
{
	int frame = buscarFramePorPagina(pid,paginaNro);
	if(frame>=0){
		logDebug("Lectura: [PID: %d, Page: %d, F: %d]",pid,paginaNro,frame);
		return leerPaginaFromFrame(frame);
	} else {
		return NULL;
	}
}

int obtener_primer_disponible(int offset)
{
	//contado de espacio contiguo
	int i;
	for(i=offset; i<bitMap->size; i++){
		//a partir del primer 0 espacio disponible
		if(!bitarray_test_bit(bitMap,i)){
			break;
		}
	}
	return i;
}

int ultimo_disponible(int primero)
{
	//contado de espacio contiguo
	int  i;

	for(i=primero; i<bitMap->size; i++){
		//corta si encuentra uno ocupado
		if(bitarray_test_bit(bitMap,i)){
			i--;
			break;
		}
	}
	return i;
}

void moverFrame(int frame_origen, int frame_destino)
{
	pagina pag;
	//setea el frame donde se va mover en 1
	bitarray_set_bit(bitMap,frame_destino);
	//trae la pagina del frame origen y la escribe en el frame destino
	pag = leerPaginaFromFrame(frame_origen);
	escribirPaginaEnFrame(frame_destino,pag);
	free(pag);

	tabla[frame_destino].page = tabla[frame_origen].page;
	tabla[frame_destino].pid = tabla[frame_origen].pid;

	tabla[frame_origen].page = -1;
	tabla[frame_origen].pid = -1;

	bitarray_clean_bit(bitMap,frame_origen);
}

void compactarSwap()
{
	logInfo("Realizando compactacion de espacio: %d ms",config->retardo_compact);
	usleep(config->retardo_compact*1000);   //convierto micro en milisegundos
	int ultimo_frame_disponible = 0;
	int j=0;
	while(j<config->cantidad_paginas){
		int i;
		//primer bloque disponible para empezar a compactar
		int primer_frame_disponible = obtener_primer_disponible(ultimo_frame_disponible);
		//busco ultimo disponible empezando desde el primero disponible
		ultimo_frame_disponible = ultimo_disponible(primer_frame_disponible);

		i=primer_frame_disponible;
		j=ultimo_frame_disponible+1;
		//Voy acomodando desde el primero disponible hasta el ultimo de ese bloque todos los ocupados que
		//encuentro a partir de ahi
		while(i <= ultimo_frame_disponible && j<config->cantidad_paginas)
		{
			if(bitarray_test_bit(bitMap,j))
			{
				moverFrame(j, i);
				i++;
			}
			j++;

		}
	}
	logInfo("Compactacion finalizada");
}
