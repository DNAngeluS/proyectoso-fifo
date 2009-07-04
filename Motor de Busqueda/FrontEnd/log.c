/* 
 * File:   log.c
 * Author: mariano
 *
 * Created on 4 de julio de 2009, 14:05
 */
#include "log.h"

int WriteLog(int log, char *proceso, pid_t pid, thread_t thread, char *descripcion, char *tipo)
{
    char buffer[BUF_SIZE];
    int error = 0, n;
    char fecha[20];
    time_t tiempo = time(NULL);

    memset(buffer, '\0', sizeof(buffer));
    memset(fecha, '\0', sizeof(fecha));

    strftime(fecha, sizeof(fecha), "[%H:%M:%S]", localtime(&tiempo));

    sprintf(buffer, "%s - %s [%d][%d]:%s: %s\n", fecha, proceso, pid, thread, tipo, descripcion);

    lseek(log, 0L, 2);

    if ((n = write(log, buffer, strlen(buffer))) < 0)
        error = 1;

    printf("%s.%s", descripcion, strcmp(tipo, "INFO") == 0? " ":"\n");

    return error? -1: 0;
}

