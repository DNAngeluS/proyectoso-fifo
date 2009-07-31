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
    
    config->log = 0;

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
    
	/*Se crea el archivo log*/
    if ((config->log = open("log.txt", O_CREAT | O_TRUNC | O_WRONLY, modeOpen)) < 0)
        return -1;

    
    return 0;
}

int asignarDatos (configuracion *config, char *key, char *value)
{
    if (strcmp(key, "IP") == 0)
        config->ip = inet_addr(value);

    else if (strcmp(key, "PUERTO") == 0)
        config->puerto = htons(atoi(value));

    else if (strcmp(key, "IP_QUERYMANAGER") == 0)
        config->ipQM = inet_addr(value);

    else if (strcmp(key, "PUERTO_QUERYMANAGER") == 0)
        config->puertoQM = htons(atoi(value));

    else if (strcmp(key, "IP_PUERTO_LDAP") == 0)
        strcpy(config->ipPortLDAP, value);
    
    else if (strcmp(key, "CLAVE_LDAP") == 0)
        strcpy(config->claveLDAP,value);

    else if (strcmp(key, "TIPO_RECURSO") == 0)
        config->tipoRecurso = atoi(value);

    else if (strcmp(key, "CANTIDAD_CONEXIONES") == 0)
        config->cantidadConexiones = atoi(value);

    else if (strcmp(key, "TIEMPO_DEMORA") == 0)
        config->tiempoDemora = atoi(value);

    else
    {
        printf("Archivo de configuracion: Opcion inexistente ('%s')\n", key);
        return -1;
    }
    return 0;
}

