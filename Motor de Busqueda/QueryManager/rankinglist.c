/* 
 * File:   rankinglist.c
 * Author: marianoyfer
 *
 * Created on 1 de julio de 2009, 10:15
 */

#include "rankinglist.h"

/*Devuelve: vacia? 1: 0*/
int listaVaciaRanking (ptrListaRanking ptr)
{
    return ptr == NULL;
}

/*Agrega al final de la lista un nuevo Ranking*/
int AgregarRanking (ptrListaRanking *ptr, struct ranking info)
{
    ptrListaRanking nuevo, actual, anterior;

    if ((nuevo = malloc(sizeof(NodoListaRanking))) == NULL)
    {
        printf("No hay memoria disponible para agrega Ranking.\n\n");
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

/*Elimina un Ranking indicado por el puntero que lo apunta*/
int EliminarRanking (ptrListaRanking *ptr, ptrListaRanking ptrEliminar)
{
    ptrListaRanking ptrAnt, ptrAct, tempPtr;

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

/*Devuelve la cantidad de rankings en la lista*/
unsigned cantidadRankingsLista (ptrListaRanking ptr)
{
    ptrListaRanking ptrAux;
    int cantidad = 0;

    for (ptrAux = ptr; ptrAux != NULL; ptrAux = ptrAux->sgte)
        cantidad++;

    return cantidad;
}

int incrementarRanking(ptrListaRanking *lista, char *name)
{
    ptrListaRanking ptr, ptrAux, ptrAnt;

    ptr = ptrAux = ptrAnt  = *lista;

    /*Se busca en la lista*/
    while (ptr != NULL && strcmp(ptr->info.name, name) != 0)
    {
        ptrAnt = ptr;
        ptr = ptr->sgte;
    }
    
    /*Si no esta lo agrega*/
    if (ptr == NULL)
    {
        struct ranking info;
        
        memset(&info, '\0', sizeof(info));
        strcpy(info.name, name);
        info.busquedas = 0;
        
        if (AgregarRanking(lista, info) < 0)
            return -1;
    }
    
    /*Si ya esta lo incremento y reordeno*/
    else
    {
        /*Se incrementa la busqueda y puentea*/
        ptr->info.busquedas++;
        ptrAnt->sgte = ptr->sgte;

        /*Se busca su nueva posicion para reordenarse*/
        ptrAnt = *lista;
        while (ptrAux != NULL && ptrAux->info.busquedas > ptr->info.busquedas)
        {
            ptrAnt = ptrAux;
            ptrAux = ptrAux->sgte;
        }

        /*Se mete en el medio*/
        ptr->sgte = ptrAux;

        if (ptrAux == *lista)
            *lista = ptr;
        else
            ptrAux = ptr;
    }

    return 0;
}

void imprimeListaRanking (ptrListaRanking lista)
{
    ptrListaRanking ptrAux = NULL;
    int cant = cantidadRankingsLista(lista);
    
    printf("Ranking:\n");

    for (ptrAux = lista, i=0; i < cant; i++, ptrAux = ptrAux->sgte)
            printf("%d-\t%s\n\tCantidad: %ld\n", i+1, ptrAux->info.name, ptrAux->info.busquedas);
    putchar('\n');
}


