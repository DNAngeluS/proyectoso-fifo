/* 
 * File:   irc.c
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:08
 */

#include "irc.h"
#include <stdio.h>
#include <stdlib.h>

int ircRequest_send(SOCKET sock, void *bloque, unsigned long bloqueLen)
{
    headerIRC header;
    unsigned long len;

    header.descriptorID = generarID();
    header.payloadDesc = IRC_REQUEST;
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
    retun 0;
}
int ircRequest_recv (SOCKET sock, void *bloque, char *descriptorID)
{
    headerIRC header;
    unsigned long len = sizeof(headerIRC);

    if (RecibirNBloque(sock, header, len) != len)
    {
        printf("Error en irc request recv header\n");
        return -1;
    }
    if (header.payloadDesc != IRC_REQUEST)
        return -1;
    memcpy(header.descriptorID, descriptorID);

    len = header.payloadLen;
    if (RecibirNBloque(sock, bloque, len) != len)
    {
        printf("Error en irc request recv header\n");
        return -1;
    }

    return 0;
}

int ircResponse_send (SOCKET sock, char *descriptorID, void *bloque, unsigned long bloqueLen)
{
    headerIRC header;
    unsigned long len = sizeof(headerIRC);

    header.descriptorID = descriptorID;
    header.payloadDesc = IRC_RESPONSE;
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
    retun 0;

    return 0;
}
int ircResponse_recv (SOCKET sock, void *bloque, unsigned long bloqueLen, char *descriptorID)
{
    headerIRC header;
    unsigned long len = sizeof(headerIRC);
    unsigned int elementosArray;

    if (RecibirNBloque(sock, header, len) != len)
    {
        printf("Error en irc request recv header\n");
        return -1;
    }
    if (header.payloadDesc != IRC_REQUEST)
        return -1;
    if (!memcmp(header.descriptorID, descriptorID, DESCRIPTORID_LEN))
    {
        int i;
        
        len = header.payloadLen;
        elementosArray = len / bloqueLen;
        bloque = malloc(bloqueLen);

        for (i=0; i< elementosArray; i++)
        {
            if (RecibirNBloque(sock, bloque, len) != len)
            {
                printf("Error en irc request recv header\n");
                return -1;
            }
            if (i+1 < elementosArray)
            {
                realloc(bloque, bloqueLen);
            }
        }
        return 0;
    }
    return -1;
}
