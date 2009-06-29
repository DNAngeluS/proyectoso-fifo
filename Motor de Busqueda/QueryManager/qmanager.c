/* 
 * File:   querymanager.c
 * Author: marianoyfer
 *
 * Created on 19 de junio de 2009, 11:49
 */

#include "qmanager.h"
#include "config.h"
#include "querylist.h"
#include "irc.h"

void rutinaDeError (char* error);
int atenderFrontEnd(SOCKET sockCliente, void *datos, unsigned long sizeDatos, char *descriptorID, int mode, ptrListaQuery listaHtml, ptrListaQuery listaArchivos);
SOCKET establecerConexionEscucha (in_addr_t nDireccionIP, in_port_t nPort);
SOCKET establecerConexionServidor (in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr);
int conectarFrontEnd (in_addr_t nDireccionIP, in_port_t nPort);

configuracion config;

int main(int argc, char** argv)
{
    SOCKET sockQM;

    SOCKET sockPrimerQP;
    SOCKADDR_IN dirPrimerQP;
    int nAddrSize = sizeof(dirPrimerQP);
    
    ptrListaQuery listaHtml = NULL;
    ptrListaQuery listaArchivos = NULL;

    fd_set fdMaestro;
    fd_set fdLectura;
    struct timeval timeout;
    int fdMax, cli;

    fdMax = cli = 0;

    /*Lectura de Archivo de Configuracion*/
    printf("Se leera archivo de configuracion. ");
    if (leerArchivoConfiguracion(&config) != 0)
       rutinaDeError("Lectura Archivo de configuracion");
    printf("Leido OK.\n");

    /*Se establece conexion a puerto de escucha*/
    printf("Se establecera conexion de escucha. ");
    if ((sockQM = establecerConexionEscucha(INADDR_ANY, config.puertoL)) == INVALID_SOCKET)
       rutinaDeError("Socket invalido");
    printf("Establecida OK.\n");

    do
    {
        printf("Esperando primer Query Processor para estar operativo...\n");

        /*Acepta la conexion entrante del primer QP, condicionante para estar operativo*/
        sockPrimerQP = accept(sockQM, (SOCKADDR *) &dirPrimerQP, &nAddrSize);
        if (sockPrimerQP == INVALID_SOCKET)
            printf("No se pudo conectar al QP.\n");
    } while (sockPrimerQP == INVALID_SOCKET);

    printf("Query Processor en %s conectado.\n", inet_ntoa(dirPrimerQP.sin_addr));

    /*Se agregan los sockets abiertos y se asigna el maximo actual*/
    FD_ZERO (&fdMaestro);
    FD_SET(sockQM, &fdMaestro);
    FD_SET(sockPrimerQP, &fdMaestro);
    fdMax = sockQM > sockPrimerQP? sockQM: sockPrimerQP;

   /*
    * Ya se recibio la conexion del primer QP, ya se puede establecer la conexion con el Front-End.
    * Se conecta al Front-End, se le envia la IP y Puerto local, y se cierra conexion.
    */
    printf("Se establecera conexion con Front-end. ");
    if (conectarFrontEnd(config.ipFrontEnd, config.puertoFrontEnd) < 0)
       rutinaDeError("Conexion a Front-End");
    printf("Conexion establecida OK.\n\n");

    while (1)
    {
        int rc, desc_ready;

        timeout.tv_sec = 60*3;
        timeout.tv_usec = 0;

        FD_ZERO(&fdLectura);
        memcpy(&fdLectura, &fdMaestro, sizeof(fdMaestro));

        printf("Esperando nuevas peticiones...\n");

        rc = select(fdMax+1, &fdLectura, NULL, NULL, &timeout);

        if (rc < 0)
            rutinaDeError("select");
        if (rc == 0)
        {
        /*SELECT TIMEOUT*/
        }
        if (rc > 0)
        {
            desc_ready = rc;
            for (cli = 0; cli < fdMax+1 && desc_ready > 0; cli++)
            {
                if (FD_ISSET(cli, &fdLectura))
                {
                    desc_ready--;
                    if (cli == sockQM)
                    /*Nueva conexion detectada*/
                    {
                        /*Cliente*/
                        SOCKET sockCliente;
                        SOCKADDR_IN dirCliente;
                        int nAddrSize = sizeof(dirCliente);

                        /*Acepta la conexion entrante*/
                        sockCliente = accept(sockQM, (SOCKADDR *) &dirCliente, &nAddrSize);
                        if (sockCliente == INVALID_SOCKET)
                        {
                            perror("No se pudo conectar cliente");
                            continue;
                        }

                        /*Agrega cliente (Front-End o QP) y actualiza max*/
                        FD_SET (sockCliente, &fdMaestro);
                        if (sockCliente > fdMax)
                            fdMax = sockCliente;

                        printf ("Conexion aceptada de %s.\n", inet_ntoa(dirCliente.sin_addr));
                    }
                    else if (cli == 0) /*stdin*/
                    {
                        /*ATENDER INPUT CONSOLA*/
                    }
                    else
                    /*Cliente detectado -> Si es QP colgar de lista, si es Front-End satisfacer pedido*/
                    {
                        void *buffer = NULL;
                        char descID[DESCRIPTORID_LEN];
                        unsigned long rtaLen = 0;
                        int control;
                        int mode = 0x00;

                        /*memset(buffer, '\0', sizeof(buffer));*/
                        memset(descID, '\0', sizeof(descID));

                        /*Recibir HandShake*/
                        if ((control = ircRequest_recv (cli, (void **)&buffer, &rtaLen, descID, &mode)) < 0)
                            printf("Error al recibir mensaje IRC");
                        else
                        {
                            if (mode == IRC_HANDSHAKE_QP)
                            /*Si es QP -> colgar de lista segun recurso atendido (payload)*/
                            {
                                int control;
                                struct query info;

                                info.ip = inet_addr(strtok((char *)buffer, ":"));
                                info.puerto = htons(atoi(strtok(NULL, "-")));
                                info.tipoRecurso = atoi(strtok(NULL, ""));
                                info.consultasExito = 0;
                                info.consultasFracaso = 0;

                                if (info.tipoRecurso == RECURSO_WEB)
                                {
                                    if ((control = AgregarQuery (&listaHtml, info)) < 0)
                                        printf("Error al agregar Query Processor a lista Htm.l\n\n");
                                }
                                else
                                {
                                    if ((control = AgregarQuery (&listaArchivos, info)) < 0)
                                        printf("Error al agregar Query Processor a lista Archivos.\n\n");
                                }
                                
                                if (control == 0)
                                    printf("Query Processor a sido agregado a una lista.\n\n");
                            }
                            else if (mode == IRC_REQUEST_HTML || mode == IRC_REQUEST_ARCHIVOS)
                            /*Si es Front-End atender*/
                            {
                                if (atenderFrontEnd(cli, buffer, rtaLen, descID, mode, listaHtml, listaArchivos) < 0)
                                    printf("Hubo un error al atender al Front-End. Se descarta conexion.\n\n");
                            }

                            free(buffer);
                            
                            /*Eliminar cliente y actualizar nuevo maximo*/
                            close(cli);
                            FD_CLR(cli, &fdMaestro);
                            if (cli == fdMax)
                                while (FD_ISSET(fdMax, &fdMaestro) == 0)
                                    fdMax--;
                        }
                    }
                }
            }
        }
    }

    /*Cierra conexiones de Sockets*/
    for (cli = 0; cli < fdMax+1; ++cli) /*cerrar descriptores*/
        close(cli);

    return (EXIT_SUCCESS);
}


int atenderFrontEnd(SOCKET sockCliente, void *datos, unsigned long sizeDatos, char *descriptorID, int mode, ptrListaQuery listaHtml, ptrListaQuery listaArchivos)
{
    SOCKET sockQP;
    char descIDQP[DESCRIPTORID_LEN];
    int modeQP = 0x00;
    unsigned long respuestaLen = 0;
    ptrListaQuery ptrAux = NULL;
    int esUnicoQP;

    esUnicoQP = cantidadQuerysLista(listaHtml) + cantidadQuerysLista(listaArchivos) == 1;

    if (mode == IRC_REQUEST_HTML)
        mode = IRC_RESPONSE_HTML;
    else if (mode == IRC_REQUEST_ARCHIVOS)
        mode = IRC_RESPONSE_ARCHIVOS;
    else
    {
        printf("IRC: Mode incompatible. Se descarta request.\n\n");
        return -1;
    }

    if (esUnicoQP)
    {
        ptrAux = listaHtml == NULL? listaArchivos: listaHtml;
        modeQP = IRC_REQUEST_UNICOQP;
    }
    else
    {
        modeQP = mode;

        if (mode == IRC_REQUEST_HTML)
            ptrAux = listaHtml;
        else if (mode == IRC_REQUEST_ARCHIVOS)
            ptrAux = listaArchivos;
    }

    /*Busco hasta el final o hasta que el primero responda que puede atenderme*/
    while (ptrAux != NULL)
    {
        /*Actualizo las estructuras para la proxima peticion*/
        char descIDQP[DESCRIPTORID_LEN];
        void *buff = NULL;
        int modeQPHandshake = IRC_REQUEST_POSIBLE;
        SOCKADDR_IN their_addr;
        sockQP = establecerConexionServidor(ptrAux->info.ip, ptrAux->info.puerto, &their_addr);
        unsigned long rtaLen = 0;

        if (sockQP == INVALID_SOCKET)
        {
            printf("Error al conectar a QP.\n\n");
            continue;
        }

        /*Envia al QP el permiso de peticion*/
        if (ircRequest_send(sockQP, NULL, 0, descIDQP, modeQPHandshake) < 0)
        {
            printf("Error al enviar consulta a QP.\n\n");
            close(sockQP);
            continue;
        }

        modeQPHandshake = 0x00;

        /*Recibir respuesta del permiso por parte del QP*/
        if (ircResponse_recv(sockQP, &buff, descIDQP, &rtaLen, &modeQPHandshake) < 0)
        {
            printf("Error al enviar consulta a QP.\n\n");
            close(sockQP);
            continue;
        }

        /*Si es posible la atencion en ese servidor, continuar pedido*/
        if (modeQPHandshake == IRC_RESPONSE_POSIBLE)
            break;

        /*Sino cerrar conexion y probar con el proximo*/
        close(sockQP);
        ptrAux = ptrAux->sgte;

    }

    /*Ninguno pudo atenderme*/
    if (ptrAux == NULL)
    {
        printf("No se encontraron Query Processors disponibles. Se enviara Internal Service Error.\n");
        datos = NULL;
        mode = IRC_RESPONSE_ERROR;
    }
    /*Si alguno pudo atenderme*/
    else
    {
        printf("Se a conectado a un Query Processor.\nSe enviara peticion. ");

        /*Re-Envia las palabras a buscar al QP*/
        if (ircRequest_send(sockQP, datos, sizeDatos, descIDQP, modeQP) < 0)
        {
            printf("Error al enviar consulta a QP.\n\n");
            close(sockQP);
            return -1;
        }

        /*Libero y limpio estructuras para recibir*/
        free(datos);
        datos = NULL;
        modeQP = 0x00;

        /*Recibir respuesta del QP*/
        if (ircResponse_recv(sockQP, &datos, descIDQP, &respuestaLen, &modeQP) < 0)
        {
            printf("Error al enviar consulta a QP.\n\n");
            close(sockQP);
            return -1;
        }

        close(sockQP);

        printf("Se han obtenido respuestas.\n");
    }

    printf("Se reenviaran respuestas al Front-end. ");

    /*Re-Envio la respuesta al Cliente en Front-End*/
    if (ircResponse_send(sockCliente, descriptorID, datos, respuestaLen, mode) < 0)
    {
      perror("ircResponse_send");
      return -1;
    }

    /*Libero estructura*/
    free(datos);

    printf("Respuesta enviadas satisfactoriamente.\n\n");

    return 0;    
}

int conectarFrontEnd(in_addr_t nDireccionIP, in_port_t nPort)
{
    SOCKET sockFrontEnd;
    SOCKADDR_IN their_addr;
    char buffer[BUF_SIZE];
    char descID[DESCRIPTORID_LEN];

    memset(buffer, '\0', sizeof(buffer));

    sockFrontEnd = establecerConexionServidor(nDireccionIP, nPort, &their_addr);

    if (sockFrontEnd == INVALID_SOCKET)
        return -1;

    sprintf(buffer, "%s:%d", inet_ntoa(*(IN_ADDR *) &config.ipL), ntohs(config.puertoL));

    if (ircRequest_send(sockFrontEnd, buffer, sizeof(buffer), descID, IRC_HANDSHAKE_QM) < 0)
    {
        perror("IRC request send");
        close(sockFrontEnd);
        return -1;
    }

    close(sockFrontEnd);

    return 0;
}

SOCKET establecerConexionServidor(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr)
{
    SOCKET sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        rutinaDeError("socket");

    their_addr->sin_family = AF_INET;
    their_addr->sin_port = nPort;
    their_addr->sin_addr.s_addr = nDireccionIP;
    memset(&(their_addr->sin_zero),'\0',8);

    /*if (connect(sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1)
        rutinaDeError("connect");*/

    while ( connect (sockfd, (SOCKADDR *)their_addr, sizeof(SOCKADDR)) == -1 && errno != EISCONN )
        if ( errno != EINTR )
        {
            perror("connect");
            close(sockfd);
            return -1;
    	}

    return sockfd;
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
        char yes = 1;
        /*int NonBlock = 1;*/

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

/*
Descripcion: Atiende error cada vez que exista en alguna funcion y finaliza proceso.
Ultima modificacion: Scheinkman, Mariano
Recibe: Error detectado.
Devuelve: -
*/
void rutinaDeError(char* error)
{
    printf("\r\nError: %s\r\n", error);
    printf("Codigo de Error: %d\r\n\r\n", errno);
    perror(error);
    exit(EXIT_FAILURE);
}
