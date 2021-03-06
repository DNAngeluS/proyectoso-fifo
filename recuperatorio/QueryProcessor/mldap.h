/* 
 * File:   ldap.h
 * Author: lucio
 *
 * Created on 9 de mayo de 2009, 4:53
 */

#ifndef _MLDAP_H
#define	_MLDAP_H

#include <stdio.h>
#include <stdlib.h>
#include "LdapWrapper.h"
#include "config.h"
#include "irc.h"

typedef struct {
    PLDAP_SESSION session;
    PLDAP_CONTEXT context;
    PLDAP_CONTEXT_OP ctxOp;
    PLDAP_SESSION_OP sessionOp;
    PLDAP_ENTRY_OP entryOp;
    PLDAP_ATTRIBUTE_OP attribOp;
} ldapObj;

int establecerConexionLDAP(ldapObj *ldap, configuracion config);
PLDAP_RESULT_SET consultarLDAP(ldapObj *ldap, char *palabras);
int armarPayload(PLDAP_RESULT_SET resultSet, void **resultados, int mode, unsigned int *cantBloques);
void ldapFreeResultSet(PLDAP_RESULT_SET resultSet);

#endif	/* _MLDAP_H */
