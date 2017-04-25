/*
 * client.h
 *
 *  Created on: 20/4/2016
 *      Author: utnso
 */

#include "package.h"
#ifndef CLIENT_CLIENT_H_
#define CLIENT_CLIENT_H_

int abrirConexionInetConServer(const char* ip, int port);
int leerSocketClient(int fd, char *datos, int longitud);
int escribirSocketClient(int fd, char *datos, int longitud);
Package* fillPackage(uint32_t msgCode, char* message, uint32_t message_long);
Package* createPackage();
void destroyPackage(Package* package);
int getLongitudPackage(Package *package);
void enviarMensajeSocket(int socket, uint32_t accion, char* mensaje);
int enviarMensajeSocketConLongitud(int socket, uint32_t accion, char* mensaje, uint32_t longitud);

#endif /* CLIENT_CLIENT_H_ */
