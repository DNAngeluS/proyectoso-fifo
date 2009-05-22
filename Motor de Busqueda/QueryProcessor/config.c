/* 
 * File:   config.c
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:08
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int leerArchivoConfiguracion (configuracion *config)
{
    config->ip = inet_addr("127.0.0.1");
    config->puerto = htons(5500);
    strcpy(config->ipPortLDAP,"ldap://127.0.0.1:1389");
    strcpy(config->claveLDAP, "marianovic");
    
    return 0;
}
int asignarDatos (configuracion *config, char *key, char *value)
{
    /*HACER*/
}
