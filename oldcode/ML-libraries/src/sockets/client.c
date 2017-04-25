/*
 * client.c
 *
 *  Created on: 20/4/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include "client.h"

/*
* Conecta con un servidor remoto a traves de socket INET
*/
int abrirConexionInetConServer(const char* ip, int port)
{
	int socketFD;
	struct sockaddr_in serv_addr;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

	socketFD = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketFD == -1)
		return -1;

	if (connect(socketFD, (void*) &serv_addr, sizeof(serv_addr)) == -1) {
		perror("No se pudo conectar con el socket server.");
		return -1;
	}

	return socketFD;
}

int leerSocketClient(int fd, char *datos, int longitud)
{
	int leido = 0;
	int aux = 0;

	/*
	* Comprobacion de que los parametros de entrada son correctos
	*/
	if ((fd == -1) || (datos == NULL) || (longitud < 1))
		return -1;

	/*
	* Mientras no hayamos leido todos los datos solicitados
	*/
	while (leido < longitud)
	{
		aux = read (fd, datos + leido, longitud - leido);
		if (aux > 0)
		{
			/*
			* Si hemos conseguido leer datos, incrementamos la variable
			* que contiene los datos leidos hasta el momento
			*/
			leido = leido + aux;
		}
		else
		{
			/*
			* Si read devuelve 0, es que se ha cerrado el socket. Devolvemos
			* los caracteres leidos hasta ese momento
			*/
			if (aux == 0)
				return leido;
			if (aux == -1)
			{
				/*
				* En caso de error, la variable errno nos indica el tipo
				* de error.
				* El error EINTR se produce si ha habido alguna
				* interrupcion del sistema antes de leer ningun dato. No
				* es un error realmente.
				* El error EGAIN significa que el socket no esta disponible
				* de momento, que lo intentemos dentro de un rato.
				* Ambos errores se tratan con una espera de 100 microsegundos
				* y se vuelve a intentar.
				* El resto de los posibles errores provocan que salgamos de
				* la funcion con error.
				*/
				switch (errno)
				{
					case EINTR:
					case EAGAIN:
						usleep (100);
						break;
					default:
						return -1;
				}
			}
		}
	}

	/*
	* Se devuelve el total de los caracteres leidos
	*/
	return leido;
}


/*
* Escribe dato en el socket cliente. Devuelve numero de bytes escritos,
* o -1 si hay error.
*/
int escribirSocketClient(int fd, char *datos, int longitud)
{
	int escrito = 0;
	int aux = 0;

	/*
	* Comprobacion de los parametros de entrada
	*/
	if ((fd == -1) || (datos == NULL) || (longitud < 1))
		return -1;

	/*
	* Bucle hasta que hayamos escrito todos los caracteres que nos han
	* indicado.
	*/
	while (escrito < longitud)
	{
		aux = write (fd, datos + escrito, longitud - escrito);
		if (aux > 0)
		{
			/*
			* Si hemos conseguido escribir caracteres, se actualiza la
			* variable escrito
			*/
			escrito = escrito + aux;
		}
		else
		{
			/*
			* Si se ha cerrado el socket, devolvemos el numero de caracteres
			* leidos.
			* Si ha habido error, devolvemos -1
			*/
			if (aux == 0)
				return escrito;
			else
				return -1;
		}
	}

	/*
	* Devolvemos el total de caracteres leidos
	*/
	return escrito;
}


char* serializarMensaje(Package *package){

	char *serializedPackage = calloc(1,getLongitudPackage(package));

	int offset = 0;
	int size_to_send;

	size_to_send =  sizeof(package->msgCode);
	memcpy(serializedPackage + offset, &(package->msgCode), size_to_send);
	offset += size_to_send;

	size_to_send =  sizeof(package->message_long);
	memcpy(serializedPackage + offset, &(package->message_long), size_to_send);
	offset += size_to_send;

	size_to_send =  package->message_long;
	memcpy(serializedPackage + offset, package->message, size_to_send);

	return serializedPackage;
}

Package* fillPackage(uint32_t msgCode, char* message, uint32_t message_long){
	Package	*package = malloc(sizeof(Package));
	package->message = malloc(sizeof(char)*message_long);
	memcpy(package->message,message,message_long);
	package->message_long = message_long;
	package->msgCode = msgCode;
	return package;
}

Package* createPackage(){
	Package* tmp;

	tmp=malloc(sizeof(Package));
	tmp->message=NULL;

	return tmp;
}

void destroyPackage(Package* package){
	if(package!=NULL){
		free(package->message);
	}
	free(package);
}

int getLongitudPackage(Package *package){
	return sizeof(package->msgCode)+sizeof(package->message_long)+(sizeof(char)*package->message_long);
}

void enviarMensajeSocket(int socket, uint32_t accion, char* mensaje){
	enviarMensajeSocketConLongitud(socket,accion,mensaje,strlen(mensaje)+sizeof(char));
}

int enviarMensajeSocketConLongitud(int socket, uint32_t accion, char* mensaje, uint32_t longitud){
	Package* package = fillPackage(accion,mensaje,longitud);
	char* serializedPkg = serializarMensaje(package);
	int resultado;

	resultado = escribirSocketClient(socket, (char *)serializedPkg, getLongitudPackage(package));

	free(serializedPkg);
	destroyPackage(package);
	return resultado;
}

