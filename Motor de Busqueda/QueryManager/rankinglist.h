/* 
 * File:   rankinglist.h
 * Author: marianoyfer
 *
 * Created on 1 de julio de 2009, 10:15
 */

#ifndef _RANKINGLIST_H
#define	_RANKINGLIST_H

#include "qmanager.h"

/*Estructuras para Rankings de Palabras y Recursos*/
struct ranking {
    char name[MAX_PATH];
    unsigned long busquedas;
};

typedef struct nodoListaRanking {
    struct ranking info;
    struct nodoListaRanking *sgte;
} NodoListaRanking;

typedef NodoListaRanking *ptrListaRanking;

void imprimeListaRanking (ptrListaRanking ptr);
int listaVaciaRanking (ptrListaRanking ptr);
int AgregarRanking (ptrListaRanking *ptr, struct ranking info);
int EliminarRanking (ptrListaRanking *ptr, ptrListaRanking ptrEliminar);
unsigned cantidadRankingsLista (ptrListaRanking ptr);
int incrementarRanking(ptrListaRanking *lista, char *name);

#endif	/* _RANKINGLIST_H */

