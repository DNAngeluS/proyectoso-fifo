/*
 * File:   qp.c
 * Author: Lucifer
 *
 * Created on 2 de mayo de 2009, 23:15
 */

#include "qp.h"
#include "config.h"
#include "irc.h"
#include "LdapWrapper.h"
#include "mldap.h"
#include "http.h"

void rutinaDeError(char *string);
SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort);
int atenderConsulta(SOCKET sockfd, ldapObj ldap);

int main()
{
    SOCKET sockQP;
    
    configuracion config;
    ldapObj ldap;
    PLDAP_RESULT_SET resultSet;
    
    fd_set fdMaestro;
	fd_set fdLectura;	
	struct timeval timeout;
    int fdMax;
    
    /*Lectura de Archivo de Configuracion*/
    if (leerArchivoConfiguracion(&config) != 0)
      rutinaDeError("Lectura Archivo de configuracion");

    if(establecerConexionLDAP(&ldap, config)!=0)
      rutinaDeError("No se pudo establecer la conexion LDAP.");	
    printf("Conexion establecida con LDAP\n");
    
    if ((sockQP = establecerConexionEscucha(INADDR_ANY, config.puerto)) != 0)
      rutinaDeError("Establecer conexion de escucha");
    printf("Conexion establecida. Se comienzan a escuchar pedidos.\n\n");
    
    FD_ZERO (&fdMaestro);
    FD_SET(sockQP, &fdMaestro);
	fdMax = sockQP;
    
    while (1)
    {
        int rc, desc_ready, int cli;
        
        /*timeout.tv_sec = 0;
		timeout.tv_usec = 0;*/
        
        FD_ZERO(&fdLectura);
		memcpy(&fdLectura, &fdMaestro, sizeof(fdMaestro));
        	
		rc = select(fdMax+1, &fdLectura, NULL, NULL, NULL);
    
        if (rc < 0)
			rutinaDeError("select");			
		if (rc == 0) 
        {
			/*SELECT TIMEOUT*/
		}
		desc_ready = rc;
		for (cli = 0; cli < fdMax+1 && desc_ready > 0; cli++) 
		{			
			if (FD_ISSET(cli, &fdLectura)) 
			{
				desc_ready--;
				if (cli == sockQP)
				/*Nueva conexion detectada -> Thread en front end*/
				{
                    /*Cliente*/
                    SOCKET sockCliente;
                    SOCKADDR_IN dirCliente;
                    int nAddrSize = sizeof(dirCliente);
                    
                    memset(descriptorID, '\0', DESCRIPTORID_LEN);
                    
                    /*Acepta la conexion entrante. Thread en el front end*/
                    sockCliente = accept(sockfd, (SOCKADDR *) &dirCliente, &nAddrSize);            
                    if (sockCliente != INVALID_SOCKET)
                        continue;            
                    
                    /*Agrega cliente y actualiza max*/
                    FD_SET (sockCliente, &fdMaestro);
					if (sockCliente > fdMax)			
						fdMax = sockCliente;
					
                    printf ("Conexion aceptada de %s:%d.\n\n", inet_ntoa(dirCliente.sin_addr),
                                                                    ntohs(dirCliente.sin_port));
                }
                else
                /*Cliente detectado -> esta enviando consultas*/
                {
                    if (atenderConsulta(cli, ldap) < 0)
                       perror("atender Consulta ldap");
                    
                    /*Eliminar cliente y actualizar nuevo maximo*/
                    close(cli);
					FD_CLR(cli, &fdMaestro);
					if (cli == fdMax) 
						while (FD_ISSET(fdMax, &fdMaestro) == 0)
							fdMax--;
                }		
    }

    /*Cierra conexiones de Sockets*/
    for (cli = 0; cli < fdMax+1; ++cli) /*cerrar descriptores*/
		close(cli);    

    /*Cierra conexiones LDAP*/
    ldap.sessionOp->endSession(ldap.session);
    
    /*Libero los objetos de operaciones*/
    freeLDAPSession(ldap.session);
    freeLDAPContext(ldap.context);
    freeLDAPContextOperations(ldap.ctxOp);
    freeLDAPSessionOperations(ldap.sessionOp);

    return 0;
}

/*      
Descripcion: Atiende el cliente, recibe busca en LDAP y responde.
Ultima modificacion: Moya, Lucio
Recibe: Socket y estructura LDAP.
Devuelve: ok? Socket del servidor: socket invalido.
*/

int atenderConsulta(SOCKET sockfd, ldapObj ldap)
{ 
    /*IRC/IPC*/
    char queryString[BUF_SIZE];
    char descriptorID[DESCRIPTORID_LEN];
    int mode;  
    /*Crea las estructuras para enviar el IRC*/
    void *resultados = NULL;
    unsigned long len;  
    PLDAP_RESULT_SET resultSet; 
     
    /*Recibe las palabras a buscar*/
    if (ircRequest_recv (sockCliente, (void *) &getInfo, descriptorID, &mode) < 0)
    {
        perror("consultarLDAP");
        return -1;
    }
    
    /*Indentifica el tipo de busqueda*/
    if (mode == IRC_REQUEST_HTML) len = (sizeof(so_URL_HTML));
    if (mode == IRC_REQUEST_ARCHIVOS) len = (sizeof(so_URL_Archivos));
    
    /*Realiza la busqueda*/        
    if ((resultSet = consultarLDAP(ldap, getInfo->palabras, getInfo->searchType))!=NULL)
    {
        perror("consultarLDAP");
        return -1;
    }
    
    /*Prepara la informacion a enviar por el IRC*/
    if ((resultados = armarPayload(resultSet, getInfo->searchType)) != NULL)
    {
        perror("consultarLDAP");
        return -1;
    }
    
    /*envia el IRC con los datos encontrados*/
    if (ircResponse_send(sockCliente, descriptorID, resultados, len, mode) < 0)
    {
        perror("consultarLDAP");
        return -1;
    }
    
    return 0;
}

/*      
Descripcion: Establece la conexion del servidor web a la escucha en un puerto y direccion ip.
Ultima modificacion: Scheinkman, Mariano
Recibe: Direccion ip y puerto a escuchar.
Devuelve: ok? Socket del servidor: socket invalido.
*/
SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort)
{
    SOCKET sockfd;

    /*Obtiene un socket*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd != INVALID_SOCKET)
    {
        SOCKADDR_IN addrServidorWeb; /*Address del Web server*/
        char yes = '1';
        int NonBlock = 1;

        /*Impide el error "addres already in use" y setea non blocking el socket*/
        if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1)
            rutinaDeError("Setsockopt");

        /*if (ioctlsocket(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
        {
        rutinaDeError("ioctlsocket");
        }*/

        addrServidorWeb.sin_family = AF_INET;
        addrServidorWeb.sin_addr.s_addr = nDireccionIP;
        addrServidorWeb.sin_port = nPort;
        memset((&addrServidorWeb.sin_zero), '\0', 8);

        /*Liga socket al puerto y direccion*/
        if ((bind (sockfd, (SOCKADDR *) &addrServidorWeb, sizeof(SOCKADDR_IN))) != SOCKET_ERROR)
        {
            /*Pone el puerto a la escucha de conexiones entrantes*/
            if (listen(sockfd, SOMAXCONN) == -1)
                rutinaDeError("Listen");
            else
            return sockfd;
        }
        else
            rutinaDeError("Bind");
    }

    return INVALID_SOCKET;
}

void rutinaDeError(char* error)
{
    printf("\r\nError: %s\r\n", error);
    printf("Codigo de Error: %d\r\n\r\n", errno);
    perror(error);
    exit(EXIT_FAILURE);
}
