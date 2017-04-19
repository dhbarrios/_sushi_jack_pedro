int cantidadPaginasPorPrograma(int iTamProg, int iTamPag){
	int iCant = iTamProg/iTamPag;
	if(iTamProg%iTamPag!=0){
		iCant++;//agrego una pagina mas para lo que resta
	}
	return iCant;
}
