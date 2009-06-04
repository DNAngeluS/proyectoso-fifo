#ifndef HTTP
#define HTTP

#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#define BUF_SIZE 1024

enum filetype_t {HTML, TXT, PHP, JPG, GIF, PNG, JPEG, PDF, ARCHIVO, EXE, ZIP, DOC, XLS, PPT };

typedef struct {
	char filename[MAX_PATH];
	int protocolo;
} msgGet;

int EnviarBloque		(SOCKET sockfd, unsigned long len, void *buffer);
int RecibirBloque		(SOCKET sockfd, char *bloque);
int RecibirNBloque		(SOCKET socket, void *buffer, unsigned long length);
int EnviarArchivo		(SOCKET sockRemoto, char *fileBuscado);
int BuscarArchivo		(char *filename);

int httpOk_send			(SOCKET sockfd, msgGet getInfo);
int httpGet_recv		(SOCKET sockfd, msgGet *getInfo);
int httpTimeout_send	(SOCKET sockfd, msgGet getInfo);
int httpNotFound_send	(SOCKET sockfd, msgGet getInfo);

char *pathUnixToWin		(const char *dir, char *path);
int getFileType			(const char *nombre);
char *getFilename		(const char *path);
DWORD getFileSize		(const char *nombre);

#endif