/*
 * server.h
 *
 *  Created on: 20/4/2016
 *      Author: utnso
 */

#ifndef SOCKETS_SERVER_SERVER_H_
#define SOCKETS_SERVER_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include "package.h"

void nuevoCliente (int servidor, int *clientes, int *nClientes, int max_clientes);
int dameMaximo (int *tabla, int n);
void compactaClaves (int *tabla, int *n);
int abrirSocketInetServer(const char* ip, int port);
int leerSocketServer(int fd, char *datos, int longitud);
int escribirSocketServer(int fd, char *datos, int longitud);
int aceptarConexionCliente (int descriptor);
int recieve_and_deserialize(Package *package, int socketCliente);
int validarLectura(int aux);

#endif /* SOCKETS_SERVER_SERVER_H_ */
