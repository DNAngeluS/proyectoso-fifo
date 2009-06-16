
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <windows.h>
#include <winsock2.h>




typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;


/*#define SOCKET int
#define SOCKADDR struct sockaddr
#define SOCKADDR_IN struct sockaddr_in
#define INVALID_SOCKET -1
#define MAX_PATH 256
#define SOCKET_ERROR -1*/
#define BUF_SIZE 1024
#define MAX_UUID 35
#define DESCRIPTORID_LEN 16
#define MAX_HTMLCODE 4096
#define MAX_FORMATO 5

#define IRC_CRAWLER_ALTA_HTML 0x30
#define IRC_CRAWLER_ALTA_ARCHIVOS 0x31
#define IRC_CRAWLER_MODIFICACION_HTML 0x32
#define IRC_CRAWLER_MODIFICACION_ARCHIVOS 0x33
#define IRC_CRAWLER_HOST 0x34
#define IRC_CRAWLER_CREATE 0x35
#define IRC_CRAWLER_CONNECT 0x36
#define IRC_CRAWLER_OK 0x37
#define IRC_CRAWLER_FAIL 0x38
#define IRC_CRAWLER_HANDSHAKE_CONNECT "SOOGLE CRAWLER CONNECT/1.0\n\n"
#define IRC_CRAWLER_HANDSHAKE_OK "CRAWLER OK\n\n"
#define IRC_CRAWLER_HANDSHAKE_FAIL "CRAWLER FAIL\n\n"

typedef struct {
    char descriptorID [DESCRIPTORID_LEN];
    int payloadDesc;
    long payloadLen;
    void *payload;
} headerIRC;

typedef struct {
    char URL            [MAX_PATH];
    char **palabras;
    char titulo         [MAX_PATH];
    char descripcion    [MAX_PATH];
    char htmlCode       [MAX_HTMLCODE];
    char tipo           [2];
    char length         [20];
    char formato        [MAX_FORMATO];
} crawler_URL;

typedef struct {
    char UUID [MAX_UUID];
    char html [MAX_PATH];
} hostsCodigo;

typedef struct {
    char host [MAX_PATH];
    long uts;
} hostsExpiracion;

int EnviarCrawlerCreate(in_addr_t nDireccion, in_port_t nPort);
SOCKET establecerConexionServidorWeb(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr);
void rutinaDeError(char *error);

int RecibirNBloque(SOCKET socket, void *buffer, unsigned long length);
int ircRequest_recv (SOCKET sock, void *bloque, char *descriptorID, int *mode);
int ircRequest_send(SOCKET sock, void *bloque, unsigned long bloqueLen, char *descriptorID, int mode);
void GenerarID(char *cadenaOUT);
int EnviarBloque                        (SOCKET sockfd, unsigned long bAEnviar, void *bloque);

int main()
{
    WSADATA wsaData;					/*Version del socket*/
    /*Inicializacion de los sockets*/
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		rutinaDeError("WSAStartup");
    
    /*Se envia pedido de creacion de Crawler al primer WebServer conocido*/
    if (EnviarCrawlerCreate(inet_addr("127.0.0.1"), htons(4445)) < 0)
        rutinaDeError("Enviando primer Crawler");
        
    
    WSACleanup();
    
    return 0;
}

int EnviarCrawlerCreate(in_addr_t nDireccion, in_port_t nPort)
{
    SOCKET sockWebServer;
    SOCKADDR_IN dirServidorWeb;
    char descriptorID[DESCRIPTORID_LEN];
    char buf[BUF_SIZE];
    int mode = 0x00;

    /*Se levanta conexion con el Web Server*/
    if ((sockWebServer = establecerConexionServidorWeb(nDireccion, nPort, &dirServidorWeb)) < 0)
        return -1;
    printf("Se establecio conexion con WebServer en %s.\n", inet_ntoa(dirServidorWeb.sin_addr));

    /*Se envia mensaje de instanciacion de un Crawler*/
/*    if (ircRequest_send(sockWebServer, NULL, 0, descriptorID, IRC_CRAWLER_CREATE) < 0)
    {
        close(sockWebServer);
        return -1;
    }*/
    if (ircRequest_send(sockWebServer, NULL, 0, descriptorID, IRC_CRAWLER_CONNECT) < 0)
    {
        close(sockWebServer);
        return -1;
    }
    printf("Crawler disparado a %s.\n\n", inet_ntoa(dirServidorWeb.sin_addr));

    if (ircRequest_recv (sockWebServer, buf, descriptorID, &mode) < 0)
       return -1;
    
    if (mode == IRC_CRAWLER_OK)
       printf("Crawler a migrado safisfactoriamente a %s.\n\n", inet_ntoa(dirServidorWeb.sin_addr));
    else if (mode == IRC_CRAWLER_FAIL)
       printf("Crawler a sido rechazado de %s.\n\n", inet_ntoa(dirServidorWeb.sin_addr));
    else
       printf("Error en mensaje IRC, se descarta.\n\n");

    /*close(sockWebServer);*/
    closesocket(sockWebServer);
    
    return 0;
}

SOCKET establecerConexionServidorWeb(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr)
{
    SOCKET sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        rutinaDeError("socket");

    their_addr->sin_family = AF_INET;
    their_addr->sin_port = nPort;
    their_addr->sin_addr.s_addr = nDireccionIP;
    memset(&(their_addr->sin_zero),'\0',8);

    /*if (connect(sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1)
        rutinaDeError("connect");*/

    while ( connect (sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1 && errno != WSAEISCONN )
        if ( errno != EINTR )
            rutinaDeError("connect");
    
    
    return sockfd;
}

int RecibirNBloque(SOCKET socket, void *buffer, unsigned long length)
{
    int bRecibidos = 0;
    int bHastaAhora = 0;

    do {
        if ((bHastaAhora = recv(socket, buffer, length, 0)) == -1) {
                break;
        }
        if (bHastaAhora == 0)
                return 0;
        bRecibidos += bHastaAhora;
    } while (bRecibidos != length);

    return bHastaAhora == -1? -1: bRecibidos;
}

int ircRequest_recv (SOCKET sock, void *bloque, char *descriptorID, int *mode)
{
    headerIRC header;
    unsigned long len;

    len = sizeof(headerIRC);

    if (RecibirNBloque(sock, &header, len) != len)
    {
        printf("Error en irc request recv header\n");
        return -1;
    }

    memcpy(descriptorID, header.descriptorID, DESCRIPTORID_LEN);
    if (*mode == 0x00) 
		*mode = header.payloadDesc;
	else
		if (header.payloadDesc != *mode)
			return -1;
    len = header.payloadLen;
	if (len != 0)
	{
		if (RecibirNBloque(sock, bloque, len) != len)
		{
			printf("Error en irc request recv header\n");
			return -1;
		}
	}

    return 0;
}

/*
Descripcion: Genera un descriptorID aleatorio
Ultima modificacion: Scheinkman, Mariano
Recibe: cadena vacia.
Devuelve: cadena con el nuevo descriptorID
*/
void GenerarID(char *cadenaOUT)
{

	int i;

	srand (time (NULL));
	for(i=0; i<DESCRIPTORID_LEN-1; i++)
	{
		cadenaOUT[i] = 'A'+ ( rand() % ('Z' - 'A') );
	}

	cadenaOUT[i] = '\0';
}


/*
Descripcion: Envia un bloque de datos
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, longitud del bloque, bloque
Devuelve: ok? bytes enviados: -1
*/
int EnviarBloque(SOCKET sockfd, unsigned long len, void *buffer)
{
    int bHastaAhora = 0;
    int bytesEnviados = 0;

    do {
        if ((bHastaAhora = send(sockfd, buffer, len, 0)) == -1){
                break;
        }
        bytesEnviados += bHastaAhora;
    } while (bytesEnviados != len);

    return bHastaAhora == -1? -1: bytesEnviados;
}

int ircRequest_send(SOCKET sock, void *bloque, unsigned long bloqueLen,
                    char *descriptorID, int mode)
{
    headerIRC header;
    unsigned long len;

    GenerarID(header.descriptorID);
    strcpy(descriptorID, header.descriptorID);
    header.payloadDesc = mode;
    header.payloadLen = bloqueLen;
    header.payload = bloque;

    len = sizeof(headerIRC);
    if (EnviarBloque(sock, len, &header) != len)
    {
        printf("Error en irc request send header\n");
        return -1;
    }

    if ((len = bloqueLen) != 0)
    {
        if (EnviarBloque(sock, len, bloque) != len)
        {
            printf("Error en irc request send bloque\n");
            return -1;
        }
    }
    return 0;
}

void rutinaDeError (char* error) 
{
	printf("\r\nError: %s\r\n", error);
	printf("Codigo de Error: %d\r\n\r\n", GetLastError());
	exit(1);
}
