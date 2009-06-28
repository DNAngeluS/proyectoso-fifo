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
int atenderFrontEnd (SOCKET sockCliente, ptrListaQuery listaHtml, ptrListaQuery listaArchivos);
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
    if (leerArchivoConfiguracion(&config) != 0)
       rutinaDeError("Lectura Archivo de configuracion");
    printf("Archivo de configuracion leido correctamente.\n");

    /*Se establece conexion a puerto de escucha*/
    if ((sockQM = establecerConexionEscucha(INADDR_ANY, config.puertoL)) == INVALID_SOCKET)
       rutinaDeError("Establecer conexion de escucha");
    printf("Conexion de escucha establecida.\n");

    do
    {
        printf("Esperando conexion del primer Query Processor para conectarse "
                "al Front-end.\n");

        /*Acepta la conexion entrante del primer QP, condicionante para estar operativo*/
        sockPrimerQP = accept(sockQM, (SOCKADDR *) &dirPrimerQP, &nAddrSize);
        if (sockPrimerQP == INVALID_SOCKET)
            perror("No se pudo conectar al QP");
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
    if (conectarFrontEnd(config.ipFrontEnd, config.puertoFrontEnd) < 0)
       rutinaDeError("Conexion a Front-End");
    printf("Conexion al Front-end satisfactoria. ");
    printf("Se comienzan a escuchar requests.\n\n");

    while (1)
    {
        int rc, desc_ready;

        timeout.tv_sec = 60*3;
        timeout.tv_usec = 0;

        FD_ZERO(&fdLectura);
        memcpy(&fdLectura, &fdMaestro, sizeof(fdMaestro));

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
                        char buffer[BUF_SIZE];
                        char descID[DESCRIPTORID_LEN];
                        int control;
                        int mode = 0x00;

                        memset(buffer, '\0', sizeof(buffer));
                        memset(descID, '\0', sizeof(descID));

                        /*Recibir HandShake*/
                        if ((control = ircRequest_recv (cli, (void **)&buffer, descID, &mode)) < 0)
                            printf("Error al recibir mensaje IRC");
                        else
                        {
                            if (mode == IRC_HANDSHAKE_QP)
                            /*Si es QP -> colgar de lista segun recurso atendido (payload)*/
                            {
                                int control;
                                struct query info;

                                info.ip = inet_addr(strtok(buffer, ":"));
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
                                if (atenderFrontEnd(cli, listaHtml, listaArchivos) < 0)
                                    printf("Hubo un error al atender al Front-End. Se descarta conexion.\n\n");
                            }
                            
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


int atenderFrontEnd(SOCKET sockCliente, ptrListaQuery listaHtml, ptrListaQuery listaArchivos)
{
    SOCKET sockQP;
    int modeQP = 0x00;
    char descriptorID[DESCRIPTORID_LEN];
    void *datos = NULL;
    unsigned long respuestaLen = 0;
    int mode = 0x00;
    ptrListaQuery ptrAux = NULL;
    int control;
    int esUnicoQP;

    /*Recibe las palabras a buscar del Front-End*/
    if ((control = ircRequest_recv (sockCliente, &datos, descriptorID, &mode)) < 0 || control == 1)
    {
        perror("ircRequest_recv");
        return -1;
    }

    esUnicoQP = cantidadQuerysLista(listaHtml) + cantidadQuerysLista(listaArchivos) == 1;
    
    if (esUnicoQP)
    {
        ptrAux = listaHtml == NULL? listaArchivos: listaHtml;
        mode = IRC_REQUEST_UNICOQP;
    }
    else
    {
        modeQP = mode;
        if (mode == IRC_REQUEST_HTML)
        {
            ptrAux = listaHtml;
            mode = IRC_RESPONSE_HTML;
        }
        else if (mode == IRC_REQUEST_ARCHIVOS)
        {
            ptrAux = listaArchivos;
            mode = IRC_RESPONSE_ARCHIVOS;
        }
        else
        {
            printf("IRC: Mode incompatible. Se descarta request.\n\n");
            return -1;
        }
    }

    /*Busco hasta el final o hasta que el primero responda que puede atenderme*/
    while (ptrAux != NULL)
    {
        /*Actualizo las estructuras para la proxima peticion*/
        char descriptorID[DESCRIPTORID_LEN];
        int modeQPHandshake = IRC_REQUEST_POSIBLE;
        sockQP = establecerConexionServidor(ptrAux->info.ip, ptrAux->info.puerto);
        int rtaLen = 0;

        if (sockQP == INVALID_SOCKET)
        {
            printf("Error al conectar a QP.\n\n");
            continue;
        }

        /*Re-Envia las palabras a buscar al QP*/
        if (ircRequest_send(sockQP, NULL, 0, descriptorID, modeQPHandshake) < 0)
        {
            printf("Error al enviar consulta a QP.\n\n");
            close(sockQP);
            continue;
        }

        modeQP = 0x00;

        /*Recibir respuesta del QP*/
        if (ircResponse_recv(sockQP, NULL, descriptorID, &rtaLen, &modeQPHandshake) < 0)
        {
            printf("Error al enviar consulta a QP.\n\n");
            close(sockQP);
            continue;
        }

        if (modeQPHandshake == IRC_RESPONSE_POSIBLE)
            break;

        close(sockQP);
        ptrAux = ptrAux->sgte;

    }

    /*Ninguno pudo atenderme*/
    if (ptrAux == NULL)
    {
        datos = NULL;
        mode = IRC_RESPONSE_ERROR;
    }
    /*Si alguno pudo atenderme*/
    else
    {
       /*Re-Envia las palabras a buscar al QP*/
        if (ircRequest_send(sockQP, datos, sizeof(*datos), descriptorID, modeQP) < 0)
        {
            printf("Error al enviar consulta a QP.\n\n");
            close(sockQP);
            return -1;
        }

        /*Libero y limpio estructuras para recibir*/
        free(datos);
        datos = NULL;
        mode = 0x00;

        /*Recibir respuesta del QP*/
        if (ircResponse_recv(sockQP, &datos, descriptorID, &respuestaLen, &modeQP) < 0)
        {
            printf("Error al enviar consulta a QP.\n\n");
            close(sockQP);
            return -1;
        }

        close(sockQP);
    }

    /*Re-Envio la respuesta al Cliente en Front-End*/
    if (ircResponse_send(sockCliente, descriptorID, datos, respuestaLen, mode) < 0)
    {
      perror("ircResponse_send");
      return -1;
    }

    /*Libero estructura*/
    free(datos);

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
