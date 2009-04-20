#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#define HTTP_TIMEOUT_0 "HTTP/1.0 408 Request Timeout\n\n"
#define HTTP_TIMEOUT_1 "HTTP/1.1 408 Request Timeout\n\n"


#define BUF_SIZE 4096

typedef struct {
	char filename[MAX_PATH];
	int protocolo;
} msgGet;

int EnviarBloque(SOCKET sockfd, DWORD bAEnviar, LPVOID bloque);
int RecibirBloque(SOCKET sockfd, LPVOID bloque, DWORD bARecibir);

int httpGet_recv(SOCKET sockfd, msgGet *getInfo);