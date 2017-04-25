/*
 * server.c
 *
 *  Created on: 20/4/2016
 *      Author: utnso
 */

#include "server.h"

/*
 * Crea un nuevo socket cliente.
 * Se le pasa el socket servidor y el array de clientes, con el número de
 * clientes ya conectados.
 */
void nuevoCliente (int servidor, int *clientes, int *nClientes, int max_clientes)
{
	/* Acepta la conexión con el cliente, guardándola en el array */
	clientes[*nClientes] = aceptarConexionCliente(servidor);
	(*nClientes)++;

	/* Si se ha superado el maximo de clientes, se cierra la conexión,
	 * se deja como estaba y se vuelve. */
	if ((*nClientes) >= max_clientes)
	{
		close (clientes[(*nClientes) -1]);
		(*nClientes)--;
		return;
	}

	/* Envía su número de cliente al cliente */
	escribirSocketServer(clientes[(*nClientes)-1], (char *)nClientes, sizeof(int));

	/* Escribe en pantalla que ha aceptado al cliente y vuelve */
	//printf ("Aceptado cliente %d\n", *nClientes);
	return;
}

/*
 * Función que devuelve el valor máximo en la tabla.
 * Supone que los valores válidos de la tabla son positivos y mayores que 0.
 * Devuelve 0 si n es 0 o la tabla es NULL */
int dameMaximo (int *tabla, int n)
{
	int i;
	int max;

	if ((tabla == NULL) || (n<1))
		return 0;

	max = tabla[0];
	for (i=0; i<n; i++)
		if (tabla[i] > max)
			max = tabla[i];

	return max;
}

/*
 * Busca en array todas las posiciones con -1 y las elimina, copiando encima
 * las posiciones siguientes.
 * Ejemplo, si la entrada es (3, -1, 2, -1, 4) con *n=5
 * a la salida tendremos (3, 2, 4) con *n=3
 */
void compactaClaves (int *tabla, int *n)
{
	int i,j;

	if ((tabla == NULL) || ((*n) == 0))
		return;

	j=0;
	for (i=0; i<(*n); i++)
	{
		if (tabla[i] != -1)
		{
			tabla[j] = tabla[i];
			j++;
		}
	}

	*n = j;
}


int abrirSocketInetServer(const char* ip, int port)
{
	/*
	* Se rellenan los campos de la estructura Direccion, necesaria
	* para la llamada a la funcion bind()
	*/
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr(ip);
	direccionServidor.sin_port = htons(port);

	//se abre el socket
	int servidorFD = socket(AF_INET, SOCK_STREAM, 0);

	int activado = 1;
	setsockopt(servidorFD, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

	if (bind(servidorFD, (void*) &direccionServidor, sizeof(direccionServidor)) != 0)
	{
		perror("Fallo al hacer el bind (Server)\n");
		return 1;
	}
	//printf("Estoy escuchando\n");
	/*
	* Se avisa al sistema que comience a atender llamadas de clientes
	*/
	if (listen(servidorFD, 100) == -1)
	{
		close (servidorFD);
		return -1;
	}
	/*
	* Se devuelve el descriptor del socket servidor
	*/
	return servidorFD;
}

/*
* Lee datos del socket. Supone que se le pasa un buffer con hueco
*	suficiente para los datos. Devuelve el numero de bytes leidos o
* 0 si se cierra fichero o -1 si hay error.
*/
int leerSocketServer(int fd, char *datos, int longitud) {
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
int escribirSocketServer(int fd, char *datos, int longitud) {
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

/*
* Se le pasa un socket de servidor y acepta en el una conexion de cliente.
* devuelve el descriptor del socket del cliente o -1 si hay problemas.
* Esta funcion vale para socket AF_INET o AF_UNIX.
*/
int aceptarConexionCliente (int descriptor)
{
	socklen_t longitudCliente;
	struct sockaddr cliente;
	int hijo;

	/*
	* La llamada a la funcion accept requiere que el parametro
	* Longitud_Cliente contenga inicialmente el tamano de la
	* estructura Cliente que se le pase. A la vuelta de la
	* funcion, esta variable contiene la longitud de la informacion
	* util devuelta en Cliente
	*/
	longitudCliente = sizeof (cliente);
	hijo = accept (descriptor, &cliente, &longitudCliente);
	if (hijo == -1)
		return -1;

	/*
	* Se devuelve el descriptor en el que esta "enchufado" el cliente.
	*/
	return hijo;
}

//recibe el paquete y lo deserializa
int recieve_and_deserialize(Package *package, int socketCliente){

	int leidos = 0;
	int aux = 0;

	int buffer_size;
	char *buffer = malloc(buffer_size = sizeof(uint32_t));

	aux = recv(socketCliente, buffer, sizeof(package->msgCode), 0);
	memcpy(&(package->msgCode), buffer, buffer_size);
	if (!aux) return 0;
	leidos+=aux;

	aux = recv(socketCliente, buffer, sizeof(package->message_long), 0);
	memcpy(&(package->message_long), buffer, buffer_size);
	if (!aux) return 0;
	leidos+=aux;

	package->message = malloc(sizeof(char)*package->message_long);
	aux = recv(socketCliente, package->message, package->message_long, 0);
	if (!aux) return 0;
	leidos+=aux;

	free(buffer);

	return leidos;
}

int validarLectura(int aux){
	if (aux >= 0)
	{
		return aux;
	}
	else
	{
		if (aux == -1)
		{
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
	return 1;
}
