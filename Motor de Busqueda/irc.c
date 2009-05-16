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
    if (EnviarBloque(sock, len, (void *) &header) != len)
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

int ircRequest_recv (SOCKET sock, void *bloque, char *descriptorID, int *mode)
{
    headerIRC header;
    unsigned long len = sizeof(headerIRC);

    if (RecibirNBloque(sock, header, len) != len)
    {
        printf("Error en irc request recv header\n");
        return -1;
    }
    memcpy(header.descriptorID, descriptorID, DESCRIPTORID_LEN);

    len = header.payloadLen;
    if (RecibirNBloque(sock, bloque, len) != len)
    {
        printf("Error en irc request recv header\n");
        return -1;
    }

    return 0;
}

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
    if (EnviarBloque(sock, len, header) != len)
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
int ircResponse_recv (SOCKET sock, void *bloque, char *descriptorID, 
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
        
        realloc(bloque, len);

        if (RecibirNBloque(sock, bloque, len) != len)
        {
            printf("Error en irc request recv header\n");
            return -1;
        }

        return 0;
    }
    return -1;
}

/*******************************************
Función: GenerarID()
Propósito: genera un id aleatorio
Devuelve: -
Fecha última modificación: 12/09/2008 15:13
Último en modificar: Mariano Scheinkman
*******************************************/
void GenerarID(char *cadenaOUT){

	int i;

	srand (time (NULL));
	for(i=0; i<DESCRIPTORID_LEN; i++)
	{
		cadenaOUT[i] = 'A'+ ( rand() % ('Z' - 'A') );
	}

	cadenaOUT[i] = '\0';

}
