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

/*Elimina un Query Processor indicado por su socket*/
int EliminarQuery (ptrListaQuery *ptr, SOCKET socket)
{
    ptrListaQuery ptrAnt, ptrAct, tempPtr;

    if (socket == ptr->info.socket)
    {
        tempPtr = *ptr;
        *ptr = (*ptr)->sgte;
        free(tempPtr);
    }
    else
    {
        ptrAnt = *ptr;
        ptrAct = (*ptr)->sgte;

        while (ptrAct != NULL && ptrAct->info.socket != socket)
        {
            ptrAnt = ptrAct;
            ptrAct = ptrAct->sgte;
        }

        if (ptrActual != NULL)
        {
            tempPtr = ptrActual;
            ptrAnterior->ptrSgte = ptrActual->ptrSgte;
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

