/*
Obtener cantidad de paginas por programa
*/
int cantidadPaginasPorPrograma(int iTamProg, int iTamPag){
	// Obtengo la cantidad de paginas que debera tener el programa
	int iCant = iTamProg/iTamPag;
	if(iTamProg%iTamPag!=0){
		iCant++;
	}
	return iCant;
}

/*
Inicializar la Memoria Principal
*/
void inicializarMemoriaPrincipal(int iCantPag, int iTamPag){

	char *bits_bitMap = malloc(sizeof(char)*iCantPag);
	int i;

	logDebug("== Inicializando Memoria Principal: %d bytes\n", iCantPag*iTamPag);

	for(i=0;i<iCantPag;i++){
		bits_bitMap[i]=0;
	}

	memoria_principal.memoria = malloc(iCantPag*iTamPag);
	memoria_principal.bitmap = bitarray_create(bits_bitMap,iCantPag);
}
