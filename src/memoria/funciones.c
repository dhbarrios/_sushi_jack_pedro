int getCantidadPaginasPrograma(int size_programa, int size_pagina){
	int cantidad = size_programa/size_pagina;
	if(size_programa%size_pagina!=0){
		cantidad++;//agrego una pagina mas para lo que resta
	}
	return cantidad;
}
