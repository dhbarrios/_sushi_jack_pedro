void inicializarMemoriaPrincipal(int iCantPag, int iTamPag){

	char *bits_bitMap = malloc(sizeof(char)*iCantPag);
	int i;

	logDebug("Creando memoria principal de tamanio %d\n", iCantPag*iTamPag);

	for(i=0;i<iCantPag;i++){
		bits_bitMap[i]=0;
	}

	memoria_principal.memoria = malloc(iCantPag*iTamPag);
	memoria_principal.bitmap = bitarray_create(bits_bitMap,iCantPag);
}
