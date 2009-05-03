/* 
 * File:   http.c
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:08
 */

#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int EnviarBloque(SOCKET sockfd, unsigned long bAEnviar, char *bloque)
{

    unsigned long bHastaAhora, bEnviados;
    int error = 0;
    bHastaAhora = bEnviados = 0;
    
    while (!error && bEnviados < bAEnviar)
    {
        if ((bHastaAhora = send(sockfd, bloque+bEnviados, bAEnviar, 0)) == -1)
        {
            printf("Error en la funcion send.\n\n");
            error = 1;
            break;
        }
        else
        {
            bEnviados += bHastaAhora;
            bAEnviar -= bHastaAhora;
        }
    }

    return error == 0? bEnviados: -1;
}

int RecibirNBloque(SOCKET sockfd, char *bloque, unsigned long nBytes)
{
    int bRecibidos = 0;
    int aRecibir = nBytes;
    int bHastaAhora = 0;

    do
    {
        if ((bHastaAhora = recv(sockfd, bloque+bRecibidos, aRecibir, 0)) == -1)
        {
                printf("Error en la funcion recv.\n\n");
                return -1;
        }
        aRecibir -= bHastaAhora;
        bRecibidos += bHastaAhora;
    } while (aRecibir > 0);

    return bRecibidos;
}


int RecibirBloque(SOCKET sockfd, char *bloque) {

    int bRecibidos = 0;
    int bHastaAhora = 0;
    int NonBlock = 1;

    do
    {
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

int httpGet_recv(SOCKET sockfd, msgGet *getInfo)
{
    char buffer[BUF_SIZE], *ptr;
    char header[4];
    int bytesRecv, error = 0;

    if ((bytesRecv = RecibirBloque(sockfd, buffer)) == -1)
            error = 1;
    else
    {
        buffer[bytesRecv+1] = '\0';

        strcpy(header, strtok(buffer, " "));

        if(!lstrcmp(header, "GET"))
        {
            char *palabras = getInfo->palabras;

            strcpy(palabras, strtok(NULL, " "));
            palabras[strlen(palabras)] = '\0';
            palabras;
            strcpy(getInfo->palabras,palabras);

            ptr = strtok(NULL, ".");
            getInfo->protocolo = atoi(strtok(NULL, "\r\n"));

            if (getInfo->protocolo != 0 && getInfo->protocolo != 1)
                error = 1;
        }
        else
            error = 1;
    }
    if (error)
    {
        getInfo->protocolo = -1;
        strcpy(getInfo->palabras, "");
        return -1;
    }
    else return 0;
}
int enviarFormularioHtml(SOCKET sockCliente)
{
    /*
     * HACER
     * info en : http://xmlsoft.org/
     * usar bibliotecas "libxml2"     
     */
}
