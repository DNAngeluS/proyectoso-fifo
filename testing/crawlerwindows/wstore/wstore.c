
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <winsock2.h>
#include <windows.h>





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
    char *palabras;
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
SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort);

int RecibirNBloque(SOCKET socket, void *buffer, unsigned long length);
int ircPaquete_recv (SOCKET sock, void *bloque, char *descriptorID, int *mode);
int ircRequest_send(SOCKET sock, void *bloque, unsigned long bloqueLen, char *descriptorID, int mode);
void GenerarID(char *cadenaOUT);
int EnviarBloque                        (SOCKET sockfd, unsigned long bAEnviar, void *bloque);

int main()
{
    WSADATA wsaData;					/*Version del socket*/
    SOCKET sockWebStore;
    SOCKET sockCliente;					/*Socket del cliente remoto*/
	SOCKADDR_IN dirCliente;				/*Direccion de la conexion entrante*/
    int nAddrSize = sizeof(dirCliente);
    crawler_URL paquete;
    char descID[DESCRIPTORID_LEN];
    int mode = 0x00;
    
    /*Inicializacion de los sockets*/
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		rutinaDeError("WSAStartup");
    
    sockWebStore = establecerConexionEscucha(inet_addr("127.0.0.1"), htons(9999));

	/*Acepta la conexion entrante*/
	sockCliente = accept(sockWebStore, (SOCKADDR *) &dirCliente, &nAddrSize);
 
    ircPaquete_recv (sockCliente, &paquete, descID, &mode);
    
	free(paquete.palabras);

    WSACleanup();
    
    return 0;
}

/*      
Descripcion: Establece la conexion del servidor web a la escucha en un puerto y direccion ip.
Ultima modificacion: Scheinkman, Mariano
Recibe: Direccion ip y puerto a escuchar.
Devuelve: ok? Socket del servidor: socket invalido.
*/
SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort)
{
    SOCKET sockfd;

    /*Obtiene un socket*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd != INVALID_SOCKET)
    {
        SOCKADDR_IN addrServidorWeb; /*Address del Web server*/
        char yes = 1;
        /*int NonBlock = 1;*/

        /*Impide el error "addres already in use" y setea non blocking el socket*/
        if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1)
            rutinaDeError("Setsockopt");

        /*if (ioctlsocket(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
        {
        rutinaDeError("ioctlsocket");
        }*/

        addrServidorWeb.sin_family = AF_INET;
        addrServidorWeb.sin_addr.s_addr = nDireccionIP;
        addrServidorWeb.sin_port = nPort;
        memset((&addrServidorWeb.sin_zero), '\0', 8);

        /*Liga socket al puerto y direccion*/
        if ((bind (sockfd, (SOCKADDR *) &addrServidorWeb, sizeof(SOCKADDR_IN))) != SOCKET_ERROR)
        {
            /*Pone el puerto a la escucha de conexiones entrantes*/
            if (listen(sockfd, SOMAXCONN) == -1)
                rutinaDeError("Listen");
            else
            return sockfd;
        }
        else
            rutinaDeError("Bind");
    }

    return INVALID_SOCKET;
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

int ircPaquete_recv (SOCKET sock, crawler_URL *paquete, char *descriptorID, int *mode)
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
    len = sizeof(crawler_URL);
	header.payload = NULL;

	if (len != 0)
	{
		if (RecibirNBloque(sock, paquete, len) != len)
		{
			printf("Error en irc request recv header\n");
			return -1;
		}
	}

	len = header.payloadLen - sizeof(crawler_URL);
	paquete->palabras = malloc(len);
	if (RecibirNBloque(sock, (void *)paquete->palabras, len) != len)
		{
			printf("Error en irc request recv header\n");
			return -1;
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
