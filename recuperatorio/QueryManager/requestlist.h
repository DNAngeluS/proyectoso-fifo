#ifndef _REQUESTLIST_H
#define	_REQUESTLIST_H

#include "qmanager.h"

#ifndef _IRC_H
	#define DESCRIPTORID_LEN 16
#endif

struct request {
		char idQP[MAX_ID_QP];
		SOCKET sockQP;
    SOCKET sockCliente;
    char descIDQP[DESCRIPTORID_LEN];
    char descIDCliente[DESCRIPTORID_LEN];
};

typedef struct nodoListaRequest {
    struct request info;
    struct nodoListaRequest *sgte;
} NodoListaRequest;

typedef NodoListaRequest *ptrListaRequest;

int listaVaciaRequest (ptrListaRequest ptr);
int AgregarRequest (ptrListaRequest *ptr, struct request info);
int EliminarRequest (ptrListaRequest *ptr, ptrListaRequest ptrEliminar);
unsigned cantidadRequestsLista (ptrListaRequest ptr);

#endif	/* _REQUESTLIST_H */
