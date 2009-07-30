#include "requestlist.h"

/*Devuelve: vacia? 1: 0*/
int listaVaciaRequest (ptrListaRequest ptr)
{
    return ptr == NULL;
}

/*Agrega al final de la lista un nuevo Request Processor*/
int AgregarRequest (ptrListaRequest *ptr, struct request info)
{
    ptrListaRequest nuevo, actual, anterior;

    if ((nuevo = malloc(sizeof(NodoListaRequest))) == NULL)
    {
        printf("No hay memoria disponible para agrega Request.\n\n");
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

/*Elimina un Request Processor indicado por el puntero que lo apunta*/
int EliminarRequest (ptrListaRequest *ptr, ptrListaRequest ptrEliminar)
{
    ptrListaRequest ptrAnt, ptrAct, tempPtr;

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
unsigned cantidadRequestsLista (ptrListaRequest ptr)
{
    ptrListaRequest ptrAux;
    int cantidad = 0;

    for (ptrAux = ptr; ptrAux != NULL; ptrAux = ptrAux->sgte)
        cantidad++;

    return cantidad;
}

