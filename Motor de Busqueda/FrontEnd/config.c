/* 
 * File:   config.c
 * Author: marianoyfer
 *
 * Created on 2 de mayo de 2009, 20:08
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>

int leerArchivoConfiguracion (configuracion *config)
{
    /*HACER*/
    config->ipQP = inet_addr("127.0.0.1");
    config->puertoQP = htons(5500);
    config->puertoL = htons(4444);
    
    return 0;
}
int asignarDatos (configuracion *config, char *key, char *value)
{
    /*HACER*/
}
