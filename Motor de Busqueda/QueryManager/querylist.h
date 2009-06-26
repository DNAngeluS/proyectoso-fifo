/* 
 * File:   querylist.h
 * Author: marianoyfer
 *
 * Created on 19 de junio de 2009, 19:46
 */

#ifndef _QUERYLIST_H
#define	_QUERYLIST_H

#ifndef _QMANAGER_H
    #define SOCKET int
    #define SOCKADDR_IN struct sockaddr_in
#endif

struct query {
    SOCKET socket;
    SOCKADDR_IN direccion;
    int tipoRecurso;
    unsigned int consultasExito;
    unsigned int consultasFracaso;
};

typedef struct nodoListaQuery {
    struct query info;
    struct nodoQuery *sgte;
} NodoListaQuery;

typedef NodoListaQuery *ptrListaQuery;

void imprimeLista (ptrListaQuery ptr);
int listaVacia (ptrListaQuery ptr);
int AgregarQuery (ptrListaQuery *ptr, struct query info);
int EliminarQuery (ptrListaQuery *ptr, SOCKET socket);
unsigned cantidadQuerysLista (ptrListaQuery ptr);

#endif	/* _QUERYLIST_H */

