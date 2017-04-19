int cantidadPaginasPorPrograma(int iTamProg, int iTamPag){
	// Obtengo la cantidad de paginas que debera tener el programa
	int iCant = iTamProg/iTamPag;
	if(iTamProg%iTamPag!=0){
		iCant++;
	}
	return iCant;
}
