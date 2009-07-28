#ifndef CONNECTION
#define CONNECTION

#ifndef WEBSERVER
	#include "webserver.h"
#endif
#ifndef IRC
	#include "irc.h"
#endif
#ifndef HTTP
	#include "http.h"
#endif
#ifndef CONFIG
	#include "config.h"
#endif
#ifndef THREADLIST
	#include "threadlist.h"
#endif

SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort);
SOCKET establecerConexionServidor(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr);
int enviarPaquete(in_addr_t nDireccion, in_port_t nPort, crawler_URL *paquete, int mode, int palabrasLen);
void rutinaAtencionCliente(LPVOID args);
SOCKET rutinaConexionCliente(SOCKET sockWebServer, unsigned maxClientes);
int rutinaCrearThread(void (*funcion)(LPVOID), SOCKET sockfd);

#endif