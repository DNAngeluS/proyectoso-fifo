/* 
 * File:   config.c
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:08
 */

#define BUF_SIZE 1024

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h> /* inet_addr */
#include "config.h"

int leerArchivoConfiguracion (configuracion *config)
{
    char buf[BUF_SIZE];
    char *key, *value, *act, *primero, *lim;
    int buf_len;
    int fd;

    if( (fd = open("config.cfg", O_RDONLY)) < 0) {
            perror("No existe el archivo de configuracion\n");
            return -1;
    }
    if ( (buf_len = read(fd, buf, BUF_SIZE)) < 0) {
            perror("No se pudo leer el archivo de configuracion\n");
            return -1;
    }
    close(fd);

    /* Comenzamos a parsear el archivo de configuracion */
    key = value = NULL;
    lim = buf + buf_len;
    for (primero = act = buf; act < lim; act++) {
            switch (*act) {
            case '=':
                    *act = '\0';
                    key = primero;
                    value = act + 1;
                    break;

            case '\n':
                    primero = act + 1;
            case ' ':
            case '\t':
            if (key != NULL && value != NULL) {
                    *act = '\0';
                    if (asignarDatos(config, key ,value) < 0)
                            return -1;
            }
            /* En cualquier caso, reseteamos los punteros key y value */
            case '#':
                    key = value = NULL;
                    break;
            }
    }
    return 0;
}

int asignarDatos (configuracion *config, char *key, char *value)
{
    if (strcmp(key, "IP_QUERY_PROCESSOR") == 0)
        config->ipQP = inet_addr(value);

    else if (strcmp(key, "PUERTO_LOCAL") == 0)
        config->puertoL = htons(atoi(value));

    else if (strcmp(key, "PUERTO_QUERY_PROCESSOR") == 0)
        config->puertoQP = htons(atoi(value));
    
    else
    {
        printf("Archivo de configuracion: Opcion inexistente ('%s')\n", key);
        return -1;
    }
    return 0;
}
