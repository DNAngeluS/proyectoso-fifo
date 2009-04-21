#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#define BUF_SIZE 4096

typedef struct {
	char filename[MAX_PATH];
	int protocolo;
} msgGet;

int EnviarBloque		(SOCKET sockfd, DWORD bAEnviar, LPVOID bloque);
int RecibirBloque		(SOCKET sockfd, LPVOID bloque);
int RecibirNBloque		(SOCKET sockfd, LPVOID bloque, DWORD nBytes);
int EnviarArchivo		(SOCKET sockRemoto, HANDLE fileHandle);
HANDLE BuscarArchivo	(char *filename);

int httpOk_send			(SOCKET sockfd, msgGet getInfo);
int httpGet_recv		(SOCKET sockfd, msgGet *getInfo);
int httpTimeout_send	(SOCKET sockfd, int protocolo);
int httpNotFound_send	(SOCKET sockfd, msgGet getInfo);

char *pathUnixToWin		(const char *dir, char *path);
