#ifndef CRAWLER
#define CRAWLER

#ifndef WEBSERVER
	#include "webserver.h"
#endif
#ifndef CONFIG
	#include "config.h"
#endif
#ifndef HASH
	#include "hash.h"
#endif
#ifndef CONNECTION
	#include "connection.h"
#endif

#include "htmlParser.h"

int rutinaConexionCrawler(SOCKET sockWebServer);
void rutinaAtencionCrawler(LPVOID args);
int EnviarCrawler(in_addr_t nDireccion, in_port_t nPort);

#endif