/* 
 * File:   irc.c
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:08
 */

#include "irc.h"
#include <stdio.h>
#include <stdlib.h>

void GenerarID(char *cadenaOUT);


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


/*
Descripcion: Recibe n bytes de un bloque de datos
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, bloque, longitud del bloque
Devuelve: ok? bytes recibidos: -1
*/
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



/*
Descripcion: Recibe un bloque de datos que no se sabe la longitud
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, bloque
Devuelve: ok? bytes recibidos: -1
*/
int RecibirBloque(SOCKET sockfd, char *bloque) {

    int bRecibidos = 0;
    int bHastaAhora = 0;
    int NonBlock = 1;

    errno = 0;

    do
    {
        /*Si no es la primera vez lo desbloquea*/
        if (bRecibidos != 0)
        {
            /*Se pone el socket como no bloqueante*/
            if (ioctl(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
            {
                printf("ioctl");
                return -1;
            }
        }
        if ((bHastaAhora = recv(sockfd, bloque+bRecibidos, BUF_SIZE, 0)) == -1 && errno != EWOULDBLOCK)
        {
            printf("Error en la funcion recv.\n\n");
            return -1;
        }
        if (errno == EWOULDBLOCK)
            break;
        bRecibidos += bHastaAhora;
    } while (1);

    /*Se vuelve a poner como bloqueante*/
    NonBlock = 0;
    if (ioctl(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
    {
        printf("ioctl");
        return -1;
    }

    return bRecibidos;
}

/*
Descripcion: Envia un mensaje Request del protocolo IRC atraves de un  socket
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, bloque a enviar, su tamaño, un descriptorID del mensaje vacio,
 *      payload descriptor del mensaje.
Devuelve: ok? 0: -1. descriptorID utilizado en el mensaje
*/
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

    len = bloqueLen;
    if (EnviarBloque(sock, len, bloque) != len)
    {
        printf("Error en irc request send bloque\n");
        return -1;
    }
    return 0;
}


/*
Descripcion: Recibe un mensaje Request del protocolo IRC atraves de un  socket
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, bloque donde recibir vacio, un descriptorID del mensaje,
 *      payload descriptor del mensaje vacio.
Devuelve: ok? 0: -1. bloque lleno y payload descriptor del mensaje recibido.
*/
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
    *mode = header.payloadDesc;

    len = header.payloadLen;
    if (RecibirNBloque(sock, bloque, len) != len)
    {
        printf("Error en irc request recv header\n");
        return -1;
    }

    return 0;
}


/*
Descripcion: Envia un mensaje Response del protocolo IRC atraves de un  socket
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, un descriptorID del mensaje, bloque a enviar, su tamaño,
 *      payload descriptor del mensaje.
Devuelve: ok? 0: -1
*/
int ircResponse_send (SOCKET sock, char *descriptorID, void *bloque,
                        unsigned long bloqueLen, int mode)
{
    headerIRC header;
    unsigned long len = sizeof(headerIRC);

    strcpy(header.descriptorID, descriptorID);
    header.payloadDesc = mode;
    header.payloadLen = bloqueLen;
    header.payload = bloque;

    len = sizeof(headerIRC);
    if (EnviarBloque(sock, len, &header) != len)
    {
        printf("Error en irc request send header\n");
        return -1;
    }

    len = bloqueLen;
    if (EnviarBloque(sock, len, bloque) != len)
    {
        printf("Error en irc request send bloque\n");
        return -1;
    }
    return 0;
}


/*
Descripcion: Recibe un mensaje Response del protocolo IRC atraves de un  socket
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, bloque vacio donde recibir, un descriptorID del mensaje,
 *      tamaño de la respuesta, payload descriptor del mensaje.
Devuelve: ok? 0: -1. tamaño de la respuesta, bloque con la respuesta
*/
int ircResponse_recv (SOCKET sock, void **bloque, char *descriptorID,
                    unsigned long *respuestaLen, int mode)
{
    headerIRC header;
    unsigned long len = sizeof(headerIRC);

    if (RecibirNBloque(sock, (void *) &header, len) != len)
    {
        printf("Error en irc request recv header\n");
        return -1;
    }
    if (header.payloadDesc != mode)
        return -1;
    if (!memcmp(header.descriptorID, descriptorID, DESCRIPTORID_LEN))
    {
        int i;

        len = header.payloadLen;
        *respuestaLen = header.payloadLen;
        
        *bloque = realloc(*bloque, len);

        if (RecibirNBloque(sock, *bloque, len) != len)
        {
            printf("Error en irc request recv header\n");
            return -1;
        }

        return 0;
    }
    return -1;
}

/*
Descripcion: Genera un descriptorID aleatorio
Ultima modificacion: Scheinkman, Mariano
Recibe: cadena vacia.
Devuelve: cadena con el nuevo descriptorID
*/
void GenerarID(char *cadenaOUT){

	int i;

	srand (time (NULL));
	for(i=0; i<DESCRIPTORID_LEN-1; i++)
	{
		cadenaOUT[i] = 'A'+ ( rand() % ('Z' - 'A') );
	}

	cadenaOUT[i] = '\0';

}
