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

/*Incrementa y reordena un ranking o si no existe lo crea y agrega*/
int incrementarRanking(ptrListaRanking *lista, char *name)
{
    ptrListaRanking ptr, ptrAnt = NULL;
    struct ranking info;

    ptr = *lista;

    /*Se busca en la lista*/
    while (ptr != NULL && strcmp(ptr->info.name, name) != 0)
    {
        ptrAnt = ptr;
        ptr = ptr->sgte;
    }

    memset(&info, '\0', sizeof(info));
    strcpy(info.name, name);

    /*Si no esta lo agrega*/
    if (ptr == NULL)
        info.busquedas = 1;
    else
	{
        info.busquedas = ptr->info.busquedas + 1;
		if (ptrAnt != NULL)
        	ptrAnt->sgte = ptr->sgte;
	}
    
    if (insertaNodoRanking(lista, info, &ptr) < 0)
        return -1;

    return 0;
}

/*Inserta el nodo nuevo, creandolo si es NULL, de forma ordenada*/  
int insertaNodoRanking (ptrListaRanking *lista, struct ranking info, ptrListaRanking *nuevo)
{
    if (*lista == NULL || (*lista)->info.busquedas < info.busquedas)
    {
        int i;

		if (*nuevo == NULL)
		{
			if ((*nuevo = malloc(sizeof(NodoListaRanking))) == NULL)
    		{
				printf("No hay memoria disponible para agrega Ranking.\n\n");
				return -1;
    		}
			(*nuevo)->sgte = NULL;
		}
		else if (*nuevo != *lista)
			(*nuevo)->sgte = *lista;
		
        (*nuevo)->info = info;
        for (i=0; ((*nuevo)->info.name)[i] != '\0'; i++) /*Convertimos todo a minusculas*/
            ((*nuevo)->info.name)[i] = tolower(((*nuevo)->info.name)[i]);
        *lista = (*nuevo);
    }
    else
        if (insertaNodoRanking(&((*lista)->sgte), info, nuevo) < 0)
            return -1;

    return 0;
}

void imprimeListaRanking (ptrListaRanking lista)
{
    ptrListaRanking ptrAux = NULL;
	int i;
    int cant;
    
    printf("Ranking:\n");

    if(lista == NULL)
		printf("No hay resultados.\n");
	else
	{
		cant = cantidadRankingsLista(lista);
	    for (ptrAux = lista, i=0; i < cant; i++, ptrAux = ptrAux->sgte)
        	printf("%d-\t%s\n\tCantidad: %ld\n", i+1, ptrAux->info.name, ptrAux->info.busquedas);
	}
}


