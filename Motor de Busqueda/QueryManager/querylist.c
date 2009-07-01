#include "querylist.h"

/*Devuelve: vacia? 1: 0*/
int listaVacia (ptrListaQuery ptr)
{
    return ptr == NULL;
}

/*Agrega al final de la lista un nuevo Query Processor*/
int AgregarQuery (ptrListaQuery *ptr, struct query info)
{
    ptrListaQuery nuevo, actual, anterior;

    if ((nuevo = malloc(sizeof(NodoListaQuery))) == NULL)
    {
        printf("No hay memoria disponible para agrega Query Processor.\n\n");
        return -1;
    }

    nuevo->info = info;
    nuevo->sgte = NULL;

    anterior = NULL;
    actual = *ptr;

    while (actual != NULL)
    {
            anterior = actual;
            actual = actual->sgte;
    }

    if (anterior == NULL)
    {
            nuevo->sgte = *ptr;
            *ptr = nuevo;
    }
    else
    {
            anterior->sgte = nuevo;
            nuevo->sgte = actual;
    }

    return 0;
};

/*Elimina un Query Processor indicado por el puntero que lo apunta*/
int EliminarQuery (ptrListaQuery *ptr, ptrListaQuery ptrEliminar)
{
    ptrListaQuery ptrAnt, ptrAct, tempPtr;

    if (ptrEliminar == *ptr)
    {
        tempPtr = *ptr;
        *ptr = (*ptr)->sgte;
        free(tempPtr);
    }
    else
    {
        ptrAnt = *ptr;
        ptrAct = (*ptr)->sgte;

        while (ptrAct != NULL && ptrAct != ptrEliminar)
        {
            ptrAnt = ptrAct;
            ptrAct = ptrAct->sgte;
        }

        if (ptrAct != NULL)
        {
            tempPtr = ptrAct;
            ptrAnt->sgte = ptrAct->sgte;
            free (tempPtr);
        }
    }
    return 0;
}

/*Devuelve la cantidad de querys en la lista*/
unsigned cantidadQuerysLista (ptrListaQuery ptr)
{
    ptrListaQuery ptrAux;
    int cantidad = 0;

    for (ptrAux = ptr; ptrAux != NULL; ptrAux = ptrAux->sgte)
        cantidad++;

    return cantidad;
}

void imprimeLista (ptrListaQuery ptr, int *i)
{
    ptrListaQuery ptrAux;

    printf("Query Processors:\n");
    for(ptrAux = listaHtml; ptrAux != NULL; *i++, ptrAux = ptrAux->sgte)
    {
        char recurso[20];

        memset(recurso,'\0', sizeof(recurso));

        if (ptrAux->info.tipoRecurso == RECURSO_WEB)
            strcpy(recurso, "Web");
        else if (ptrAux->info.tipoRecurso == RECURSO_ARCHIVOS)
            strcpy(recurso, "Archivos");

        printf("%d-\tIP: %s\n\tPuerto: %d\n\tRecurso: %s\n"
                "\tConsultas exitosas: %d\n\tConsultas sin exito: %d\n",
                *i+1, inet_ntoa(*(IN_ADDR *) &ptrAux->info.ip),
                ntohs(ptrAux->info.puerto), recurso,
                ptrAux->info.consultasExito, ptrAux->info.consultasFracaso);

    }
}
