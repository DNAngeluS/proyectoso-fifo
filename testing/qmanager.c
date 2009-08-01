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
#include "rankinglist.h"
#include "log.h"
#include "requestlist.h"

void rutinaDeError (char* error, int log);
int atenderFrontEnd(SOCKET sockCliente, void *datos, unsigned long sizeDatos, char *descriptorID, 
										char *descriptorIDQP, int mode, ptrListaQuery *lstHtml, 
										ptrListaQuery *lstArchivos, ptrListaRanking *listaPalabras, 
										ptrListaRequest listaRequest, char *idQP);
SOCKET establecerConexionEscucha (in_addr_t nDireccionIP, in_port_t nPort);
SOCKET establecerConexionServidor (in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr);
int conectarFrontEnd (in_addr_t nDireccionIP, in_port_t nPort);
int chequeoDeVida (in_addr_t ip, in_port_t puerto);
int eliminarQueryProcessor(ptrListaQuery *lstHtml, ptrListaQuery *lstArchivos, char *id);
int atenderQP (SOCKET sockCliente, void *datos, unsigned long respuestaLen, char *descriptorID, int mode, 
										ptrListaRanking *listaRecursos);
int computarEstadisticas(ptrListaQuery listaHtml, ptrListaQuery listaArchivos, char *id, int exito);
void GenerarIDQP(char *cadenaOUT);

configuracion config;
mutex_t logMutex;
int log;

int main(int argc, char** argv)
{
    SOCKET sockQM;
    int mensajeEsperando = 0;

    SOCKET sockPrimerQP;
    SOCKADDR_IN dirPrimerQP;
    int nAddrSize = sizeof(dirPrimerQP);
    
    ptrListaQuery listaHtml = NULL;
    ptrListaQuery listaArchivos = NULL;
    ptrListaRanking listaPalabras = NULL;
    ptrListaRanking listaRecursos = NULL;
    ptrListaRequest listaRequest = NULL;

    fd_set fdMaestro;
    fd_set fdLectura;
    struct timeval timeout;
    int fdMax, cli;
    mode_t modeOpen = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    fdMax = cli = 0;
    char text[60];

    /*Se inicializa el mutex*/
    mutex_init(&logMutex, USYNC_THREAD, NULL);

    /*Se crea el archivo log*/
    if ((log = open("log.txt", O_CREAT | O_TRUNC | O_WRONLY, modeOpen)) < 0)
        rutinaDeError("Crear archivo Log", log);

    WriteLog(log, "Query Manager", getpid(), thr_self(), "Inicio de ejecucion", "INFO");  

    /*Lectura de Archivo de Configuracion*/
    WriteLog(log, "Query Manager", getpid(), thr_self(), "Se leera archivo de configuracion", "INFO");
    if (leerArchivoConfiguracion(&config) != 0)
       rutinaDeError("Lectura Archivo de configuracion", log);
    WriteLog(log, "Query Manager", getpid(), thr_self(), "Leido OK", "INFOFIN");

    /*Se establece conexion a puerto de escucha*/
    WriteLog(log, "Query Manager", getpid(), thr_self(), "Se establecera conexion de escucha", "INFO");
    if ((sockQM = establecerConexionEscucha(INADDR_ANY, config.puertoL)) == INVALID_SOCKET)
       rutinaDeError("Socket invalido", log);
    WriteLog(log, "Query Manager", getpid(), thr_self(), "Establecido OK", "INFOFIN");

    do
    {
        WriteLog(log, "Query Manager", getpid(), thr_self(), "Esperando primer Query Processor para estar operativo...", "INFOFIN");
        putchar('\n');

        /*Acepta la conexion entrante del primer QP, condicionante para estar operativo*/
        sockPrimerQP = accept(sockQM, (SOCKADDR *) &dirPrimerQP, &nAddrSize);
        if (sockPrimerQP == INVALID_SOCKET)
            WriteLog(log, "Query Manager", getpid(), thr_self(), "No se pudo conectar al Query Processor", "ERROR");
    } while (sockPrimerQP == INVALID_SOCKET);

    sprintf(text, "Query Processor en %s conectado.\n", inet_ntoa(dirPrimerQP.sin_addr));
    WriteLog(log, "Query Manager", getpid(), thr_self(), text, "INFOFIN");

    /*Se agregan los sockets abiertos y se asigna el maximo actual*/
    FD_ZERO (&fdMaestro);
    FD_SET(0, &fdMaestro); /*Añado stdin*/
    FD_SET(sockQM, &fdMaestro);
    FD_SET(sockPrimerQP, &fdMaestro);
    fdMax = sockQM > sockPrimerQP? sockQM: sockPrimerQP;

   /*
    * Ya se recibio la conexion del primer QP, ya se puede establecer la conexion con el Front-End.
    * Se conecta al Front-End, se le envia la IP y Puerto local, y se cierra conexion.
    */
    WriteLog(log, "Query Manager", getpid(), thr_self(), "Se establecera conexion con Front-end", "INFO");
    if (conectarFrontEnd(config.ipFrontEnd, config.puertoFrontEnd) < 0)
       rutinaDeError("Conexion a Front-End", log);
    WriteLog(log, "Query Manager", getpid(), thr_self(), "Conexion establecida OK", "INFOFIN");

    while (1)
    {
        int rc, desc_ready;

        timeout.tv_sec = 20;
        timeout.tv_usec = 0;

        FD_ZERO(&fdLectura);
        memcpy(&fdLectura, &fdMaestro, sizeof(fdMaestro));

        if (!mensajeEsperando)
        {
            mensajeEsperando = 1;
            WriteLog(log, "Query Manager", getpid(), thr_self(), "Esperando nuevas peticiones", "INFOFIN");
        }

        rc = select(fdMax+1, &fdLectura, NULL, NULL, &timeout);

        if (rc < 0)
            rutinaDeError("select", log);
        else if (rc == 0)
        {
            /*SELECT TIMEOUT*/
            
        }
        else if (rc > 0)
        {
            mensajeEsperando = 0;
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
                            WriteLog(log, "Query Manager", getpid(), thr_self(), "No se pudo conectar cliente", "ERROR");
                            continue;
                        }

                        /*Agrega cliente (Front-End o QP) y actualiza max*/
                        FD_SET (sockCliente, &fdMaestro);
                        if (sockCliente > fdMax)
                            fdMax = sockCliente;

                        sprintf (text, "Conexion aceptada de %s", inet_ntoa(dirCliente.sin_addr));
                        WriteLog(log, "Query Manager", getpid(), thr_self(), text, "INFOFIN");
                    }
                    else if (cli == 0) /*stdin*/
                    {
                        /*ATENDER INPUT CONSOLA*/
                        char input[MAX_PATH];
                        int c, i = 0;

                        memset(input, '\0', sizeof(input));

                        while ((c = getchar()) != EOF && c != '\n')
                            input[i++] = c;
                        input[i] = '\0';

                        fflush(stdin);

                        if(strcmp(input, "-qpstatus") == 0)
                        {
                            int contador = 0;
                            imprimeLista(listaHtml, &contador);
                            imprimeLista(listaArchivos, &contador);
                            putchar('\n');
                        }

                        else if(strcmp(input, "-ranking-palabras") == 0)
                        {
                            imprimeListaRanking(listaPalabras);
                            putchar('\n');
                        }

                        else if(strcmp(input, "-ranking-recursos") == 0)
                        {
                            imprimeListaRanking(listaRecursos);
                            putchar('\n');
                        }

                        else if(strcmp(input, "-help") == 0)
                        {
                            printf("%s", STR_MSG_HELP);
                        }
                        else
                            printf("%s", STR_MSG_INVALID_INPUT);
                    }
                    
                    
                    
                    else
                    /*Cliente detectado -> Si es QP colgar de lista, si es Front-End satisfacer pedido*/
                    {
                        void *buffer = NULL;
                        char descID[DESCRIPTORID_LEN];
                        char descQP[DESCRIPTORID_LEN];
                        unsigned long rtaLen = 0;
                        int control;
                        int socketQP = 0;
                        int mode = 0x00;
                        ptrListaRequest ptrAux = NULL;

                        /*memset(buffer, '\0', sizeof(buffer));*/
                        memset(descID, '\0', sizeof(descID));

                        /*Recibir HandShake*/
                        WriteLog(log, "Query Manager", getpid(), thr_self(), "Se recibira informacion entrante", "INFOFIN");
                        if ((control = ircRequest_recv (cli, (void **)&buffer, &rtaLen, descID, &mode)) < 0)
                        {
                            WriteLog(log, "Query Manager", getpid(), thr_self(), "Error", "ERROR");
                            putchar('\n');
                        }
                        
                        /*Se detecta desconexion*/
                        else if (control == 1)
                        {
                        		/*Se busca en la lista de Requests si el socket detectado es Front-end*/
                        		ptrAux = listaRequest;
                        		while (ptrAux != NULL && ptrAux->info.sockCliente != cli)
                        				ptrAux = ptrAux->sgte;
                        		
                        		/*Si es frontend-> se elimina de la lista*/
                        		if (ptrAux != NULL)
                        				EliminarRequest(&listaRequest, ptrAux);
                        		else
                        		{
                        				/*Sino, se busca en la lista de requests si es un QP*/
                        				ptrAux = listaRequest;
                        				while (ptrAux != NULL && ptrAux->info.sockQP != cli)
                        						ptrAux = ptrAux->sgte;
                        						
                    						/*Si lo encuentra*/
                        				if (ptrAux != NULL)
                        				{
                        						char idQP[MAX_ID_QP];
                        						
                        						strcpy(idQP, ptrAux->info.idQP);

																		/*Re-Envio la respuesta fallida al Cliente en Front-End*/
																		WriteLog(log, "Query Manager", getpid(), thr_self(), "A fallado conexion con Query Processor. Se enviara Http 500 Internal Service Error", "INFO");
																		if (ircResponse_send(ptrAux->info.sockCliente, ptrAux->info.descIDCliente, NULL, 0, IRC_RESPONSE_ERROR) < 0)
																		{
																			printf("Error.\n");
																			return -1;
																		}
																		WriteLog(log, "Query Manager", getpid(), thr_self(), "Enviado OK", "INFOFIN");
                        						
                        						/*Se elimina de la lista de requests*/
                        						EliminarRequest(&listaRequest, ptrAux);
                        						
                        						/*Busco en la lista de qps por id y lo elimino*/
                        						eliminarQueryProcessor(&listaHtml, &listaArchivos, idQP);
                        				}
                        				/*Sino*/
                        				else
                        				{
                        						/*Inconsistencia*/
                        				}
                        		}
                        		/*Eliminar cliente y actualizar nuevo maximo*/
		                          close(cli);
		                          FD_CLR(cli, &fdMaestro);
		                          if (cli == fdMax)
		                              while (FD_ISSET(fdMax, &fdMaestro) == 0)
		                                  fdMax--;
		                          putchar('\n');
		                          mensajeEsperando = 1;
                        }
                        else
                        {
                            WriteLog(log, "Query Manager", getpid(), thr_self(), "Handshake recibido OK", "INFOFIN");
                            if (mode == IRC_HANDSHAKE_QP)
                            /*Si es QP -> colgar de lista segun recurso atendido (payload)*/
                            {
                                int control;
                                struct query info;

                                WriteLog(log, "Query Manager", getpid(), thr_self(), "Se atendera nuevo Query Processor", "INFOFIN");

                                info.ip = inet_addr(strtok((char *)buffer, ":"));
                                info.puerto = htons(atoi(strtok(NULL, "-")));
                                info.tipoRecurso = atoi(strtok(NULL, "+"));
                                info.conexionesMaxima = atoi(strtok(NULL, ""));
                                info.consultasExito = 0;
                                info.consultasFracaso = 0;
                                GenerarIDQP(info.id);

                                WriteLog(log, "Query Manager", getpid(), thr_self(), "Se agregara a la lista", "INFO");
                                if (info.tipoRecurso == RECURSO_WEB)
                                {
                                    if ((control = AgregarQuery (&listaHtml, info)) < 0)
                                        WriteLog(log, "Query Manager", getpid(), thr_self(), "Error al agregar a lista Html", "ERROR");
                                }
                                else
                                {
                                    if ((control = AgregarQuery (&listaArchivos, info)) < 0)
                                        WriteLog(log, "Query Manager", getpid(), thr_self(), "Error al agregar a lista Archivos", "ERROR");
                                }
                                
                                if (control == 0)
                                    WriteLog(log, "Query Manager", getpid(), thr_self(), "Agregado a lista OK", "INFOFIN");
                                printf("Atencion de conexion de Query Processor finalizada.\n\n");
                            }
                            else if (mode == IRC_REQUEST_HTML || mode == IRC_REQUEST_ARCHIVOS || mode == IRC_REQUEST_CACHE)
                            /*Si es Front-End atender*/
                            {
                                char idQP[MAX_ID_QP];
                                WriteLog(log, "Query Manager", getpid(), thr_self(), "Se atendera Front-end", "INFOFIN");
                                socketQP = atenderFrontEnd(cli, buffer, rtaLen, descID, descQP, mode, &listaHtml, &listaArchivos, &listaPalabras, listaRequest, idQP);
                                sprintf(text, "Request del Front-end recibido%s", socketQP < 0? " con Error": "");
                                WriteLog(log, "Query Manager", getpid(), thr_self(), text, socketQP<0? "ERROR": "INFOFIN");
                                if (socketQP > 0)
                                {
                                
                                /*AGREGAR A LA LISTA EL SOCKET DEL CLIENTE (cli),
                                	EL SOCKET DEL QP (socketQP),
                                	EL DESCRIPTORID DEL QP (descQP),
                                	Y EL DESCRIPTORID DEL CLIENTE (descID).
                                	PARA DEVOLVER UN MENSAJE SE BUSCA PRIMERO POR SOCKET QP
                                	Y SEGUNDO POR DESCRIPTORID DEL QP
                                */
                                	struct request info;
                                	
                                	info.sockCliente = cli;
                                	info.sockQP = socketQP;
                                	strcpy(info.idQP, idQP);
                                	strcpy(info.descIDQP, descQP);
                                	strcpy(info.descIDCliente, descID);
                                	
                                	if (AgregarRequest(&listaRequest, info) < 0)
                                		WriteLog(log, "Query Manager", getpid(), thr_self(), "Error al agregar request a lista", "ERROR");
                                	putchar('\n');
																	mensajeEsperando = 0;
                                }
                            }
											/*++++++else if EL QP ME ESTA RESPONDIENDO+++++++*/
														else if (mode == IRC_RESPONSE_HTML || mode == IRC_RESPONSE_ARCHIVOS || 
																			mode == IRC_RESPONSE_CACHE)
														{
																int control;
																ptrListaRequest ptrAux = listaRequest;
																
																WriteLog(log, "Query Manager", getpid(), thr_self(), "Se atendera Query Processor", "INFOFIN");
																
																/*BUSCAR EL QP SEGUN DESCID DEL QP (descID)*/
																while (ptrAux != NULL && strcmp(ptrAux->info.descIDQP, descID) != 0)
																		ptrAux = ptrAux->sgte;
																	
																if (ptrAux == NULL)
																{
																		WriteLog(log, "Query Manager", getpid(), thr_self(), "Consulta inconsistente, Se descarta request", "ERROR");
																}
																else
																{
																		char text[100];
																		
																		WriteLog(log, "Query Manager", getpid(), thr_self(), "Se han obtenido respuestas de un QP", "INFOFIN");
																		control = atenderQP(ptrAux->info.sockCliente, buffer, rtaLen, ptrAux->info.descIDCliente, mode, &listaRecursos);
																		if (control < 0)
																		{
																				/*ERROR AL ENVIAR INFO AL CLIENTE*/
																		}
																		
																		/*Busco el qp segun id para computar ranking*/
																		else
																		{
																				computarEstadisticas(listaHtml, listaArchivos, ptrAux->info.idQP, rtaLen != 0? 1: 0);
																		}
																		
																		/*Actualizamos el archivo maestro*/
																		FD_CLR(ptrAux->info.sockCliente, &fdMaestro);
								                    if (ptrAux->info.sockCliente == fdMax)
								                        while (FD_ISSET(fdMax, &fdMaestro) == 0)
								                            fdMax--;
								                            
								                    /*Eliminamos de la lista de Requests*/
																		EliminarRequest(&listaRequest, ptrAux);
																		sprintf(text, "Atencion del Front-end finalizada%s", control < 0? " con Error": "");
																		WriteLog(log, "Query Manager", getpid(), thr_self(), text, control<0? "ERROR": "INFOFIN");
																		putchar('\n');
																		mensajeEsperando = 0;
																}					
														}

                            free(buffer);
                            
														if (socketQP == 0)
														{
		                          /*Eliminar cliente y actualizar nuevo maximo*/
		                          close(cli);
		                          FD_CLR(cli, &fdMaestro);
		                          if (cli == fdMax)
		                              while (FD_ISSET(fdMax, &fdMaestro) == 0)
		                                  fdMax--;
		                          putchar('\n');
		                          mensajeEsperando = 1;
														}
														else
														{
																/*Agrega cliente (Front-End o QP) y actualiza max*/
								                FD_SET (socketQP, &fdMaestro);
								                if (socketQP > fdMax)
								                    fdMax = socketQP;
														}
                        }
                    }
                }
            }
        }
    }

    /*Cierra conexiones de Sockets*/
    for (cli = 0; cli < fdMax+1; ++cli) /*cerrar descriptores*/
        close(cli);

    /*Libera la memoria de la lista de Query Processors*/
    while (listaHtml != NULL)
        EliminarQuery(&listaHtml, listaHtml);
    while (listaArchivos != NULL)
        EliminarQuery(&listaArchivos, listaArchivos);
    while (listaPalabras != NULL)
        EliminarRanking(&listaPalabras, listaPalabras);
    while (listaRecursos != NULL)
        EliminarRanking(&listaRecursos, listaRecursos);

		/*Finalizo el mutex*/
    mutex_destroy(&logMutex);
		close(log);

    return (EXIT_SUCCESS);
}

int atenderFrontEnd(SOCKET sockCliente, void *datos, unsigned long sizeDatos, char *descriptorID, 
										char *descriptorIDQP, int mode,ptrListaQuery *lstHtml, 
										ptrListaQuery *lstArchivos, ptrListaRanking *listaPalabras, ptrListaRequest listaRequest, char *idQP)
{
    SOCKET sockQP;
    char descIDQP[DESCRIPTORID_LEN];
    int modeQP = 0x00;
    unsigned long respuestaLen = 0;
    ptrListaQuery listaHtml = *lstHtml;
    ptrListaQuery listaArchivos = *lstArchivos;
    ptrListaQuery ptrAux = NULL;
    int esUnicoQP;

    esUnicoQP = cantidadQuerysLista(listaHtml) + cantidadQuerysLista(listaArchivos) == 1;

    if (esUnicoQP)
    {
        ptrAux = listaHtml == NULL? listaArchivos: listaHtml;
        modeQP = IRC_REQUEST_UNICOQP;
    }
    else
    {
        modeQP = mode;

        if (mode == IRC_REQUEST_HTML || mode == IRC_REQUEST_CACHE)
            ptrAux = listaHtml;
        else if (mode == IRC_REQUEST_ARCHIVOS)
            ptrAux = listaArchivos;
    }

    /*Computo en ranking de palabras el querystring buscado*/
		if (mode != IRC_REQUEST_CACHE)
    {
        char palabras[MAX_PATH];
				memset(palabras, '\0', sizeof(palabras));

        strcpy(palabras, ((msgGet *) datos)->palabras);

        WriteLog(log, "Query Manager", getpid(), thr_self(), "Se computara el querystring en el ranking", "INFO");
        if (incrementarRanking(listaPalabras, palabras) < 0)
            WriteLog(log, "Query Manager", getpid(), thr_self(), "Error: no hay memoria", "ERROR");
        else
            WriteLog(log, "Query Manager", getpid(), thr_self(), "Computado OK", "INFOFIN");
    }

    /*Busco hasta el final o hasta que el primero responda que puede atenderme*/
    while (ptrAux != NULL)
    {
        /*Actualizo las estructuras para la proxima peticion*/
        char descIDQP[DESCRIPTORID_LEN];
        void *buff = NULL;
        int modeQPHandshake = IRC_REQUEST_POSIBLE;
        SOCKADDR_IN their_addr;
        unsigned long rtaLen = 0;
        
        /*Verificamos que el qp a pedir permiso tenga disponibilidad de conexiones*/
        int conexionesActuales = 0;
        ptrListaRequest ptrRequest = listaRequest;
        
        while (ptrRequest != NULL)
        		if (strcmp(ptrRequest->info.idQP, ptrAux->info.id) == 0)
        				conexionesActuales++;
        if (conexionesActuales >= ptrAux->info.conexionesMaxima)
        {
        		ptrAux = ptrAux->sgte;
        		continue;
        }
        
				sockQP = establecerConexionServidor(ptrAux->info.ip, ptrAux->info.puerto, &their_addr);
        if (sockQP == INVALID_SOCKET)
        {
            ptrListaQuery ptrEliminar = ptrAux;
            
            ptrAux = ptrAux->sgte;
            EliminarQuery(ptrEliminar->info.tipoRecurso == RECURSO_WEB? lstHtml: lstArchivos, ptrEliminar);
            WriteLog(log, "Query Manager", getpid(), thr_self(), "Error: conectar Query Processor", "ERROR");
            WriteLog(log, "Query Manager", getpid(), thr_self(), "Query Processor sin vida se a eliminado de la lista de Html", "INFOFIN");
        }
        else
        {
						/*Envia al QP el permiso de peticion*/
						if (ircRequest_send(sockQP, NULL, 0, descIDQP, modeQPHandshake) < 0)
						{
						    WriteLog(log, "Query Manager", getpid(), thr_self(), "Error: enviar permiso a Query Processor", "ERROR");
						}
						else
						{
							modeQPHandshake = 0x00;

							/*Recibir respuesta del permiso por parte del QP*/
							if (ircResponse_recv(sockQP, &buff, descIDQP, &rtaLen, &modeQPHandshake) < 0)
							{
									WriteLog(log, "Query Manager", getpid(), thr_self(), "Error: recibir permiso de Query Processor", "ERROR");
							}
							else
							{
								/*Si es posible la atencion en ese servidor, continuar pedido*/
								if (modeQPHandshake == IRC_RESPONSE_POSIBLE)
										break;
							}
						}

						/*Sino cerrar conexion y probar con el proximo*/
						close(sockQP);
						if (buff != NULL)
							free(buff);
				
						ptrAux = ptrAux->sgte;
				}
    }
		
    /*Ninguno pudo atenderme*/
    if (ptrAux == NULL)
    {
        WriteLog(log, "Query Manager", getpid(), thr_self(), "No se encontraron Query Processors disponibles. Se enviara Internal Service Error", "INFOFIN");
        void *vacio = NULL;
        respuestaLen = 0;
        mode = IRC_RESPONSE_ERROR;

				/*Re-Envio la respuesta al Cliente en Front-End*/
				WriteLog(log, "Query Manager", getpid(), thr_self(), "Se envia respuesta al Front-end", "INFO");
				if (ircResponse_send(sockCliente, descriptorID, vacio, respuestaLen, mode) < 0)
				{
				  printf("Error.\n");
				  return -1;
				}
				WriteLog(log, "Query Manager", getpid(), thr_self(), "Respuesta enviada satisfactoriamente", "INFOFIN");

				strcpy(descriptorIDQP, "");
				strcpy(idQP, "");

				return 0;
    }
    /*Si me pueden atender*/
		else
		{
				WriteLog(log, "Query Manager", getpid(), thr_self(), "Se a conectado a un Query Processor", "INFOFIN");

        /*Re-Envia las palabras a buscar al QP*/
        WriteLog(log, "Query Manager", getpid(), thr_self(), "Se enviara peticion", "INFO");
        if (ircRequest_send(sockQP, datos, sizeDatos, descIDQP, modeQP) < 0)
        {
            WriteLog(log, "Query Manager", getpid(), thr_self(), "Error: enviar consulta a Query Processor", "ERROR");
            close(sockQP);
            return -1;
        }
        WriteLog(log, "Query Manager", getpid(), thr_self(), "Peticion enviada OK", "INFOFIN");
				
				strcpy(descriptorIDQP, descIDQP);
				strcpy(idQP, ptrAux->info.id);
				return sockQP;
		}
}


int computarEstadisticas(ptrListaQuery listaHtml, ptrListaQuery listaArchivos, char *id, int exito)
{
		ptrListaQuery ptrAux = listaHtml;
		while (ptrAux != NULL && strcmp(ptrAux->info.id, id) != 0)
				ptrAux = ptrAux->sgte;
				
		if (ptrAux ==  NULL)
		{
				ptrListaQuery ptrAux = listaArchivos;
				while (ptrAux != NULL && strcmp(ptrAux->info.id, id) != 0)
						ptrAux = ptrAux->sgte;
		}
		
		if (ptrAux != NULL)
		{
				/*Actualizo valores estadisticos*/
				if (exito)
						ptrAux->info.consultasExito++;
				else
						ptrAux->info.consultasFracaso++;
		}
		else
		{
				/*FRULA*/
		}
		return 0;
}

int eliminarQueryProcessor(ptrListaQuery *lstHtml, ptrListaQuery *lstArchivos, char *id)
{
		ptrListaQuery listaHtml = *lstHtml;
		ptrListaQuery listaArchivos = *lstArchivos;
		ptrListaQuery ptrAux = listaHtml;

    /*Busco hasta el final o hasta encontrar el id*/
    while (ptrAux != NULL && strcmp(ptrAux->info.id, id) != 0)
    		ptrAux = ptrAux->sgte;
    
    /*Si encontro*/
    if (ptrAux != NULL)
    {
    		EliminarQuery(&listaHtml, ptrAux);
        WriteLog(log, "Query Manager", getpid(), thr_self(), "Query Processor sin vida se a eliminado de la lista de Html", "INFOFIN");
    }
    
    /*Si no encontro*/
    else
    {
    		ptrAux = listaArchivos;
    		
    		/*Busco hasta el final o hasta encontrar el id*/
				while (ptrAux != NULL && strcmp(ptrAux->info.id, id) != 0)
						ptrAux = ptrAux->sgte;
						
				/*Si encontro*/
				if (ptrAux != NULL)
				{
						EliminarQuery(&listaArchivos, ptrAux);
				    WriteLog(log, "Query Manager", getpid(), thr_self(), "Query Processor sin vida se a eliminado de la lista de Archivos", "INFOFIN");
				}
				else
				{
						/*FRULA*/
				}
    }
    
    return 0;
}



int atenderQP (SOCKET sockCliente, void *datos, unsigned long respuestaLen, char *descriptorID, int mode, 
										ptrListaRanking *listaRecursos)
{   
		
    if (mode == IRC_RESPONSE_HTML || mode == IRC_RESPONSE_ARCHIVOS)
    {
        int cantidadRespuestas = 0;
				int i, rankingError = 0;

        if (mode == IRC_RESPONSE_HTML)
            cantidadRespuestas = respuestaLen / sizeof(so_URL_HTML);
        else  if (mode == IRC_RESPONSE_ARCHIVOS)
            cantidadRespuestas = respuestaLen / sizeof(so_URL_Archivos);

        /*Computo en ranking de recursos los recursos encontrados*/
				WriteLog(log, "Query Manager", getpid(), thr_self(), "Se computaran el recursos en el ranking", "INFO");
        for (i = 0; i < cantidadRespuestas; i++)
        {
            char palabras[MAX_PATH];

            if (mode == IRC_RESPONSE_HTML)
                strcpy(palabras, (((so_URL_HTML *) datos)[i]).URL);
            else  if (mode == IRC_RESPONSE_ARCHIVOS)
                strcpy(palabras, (((so_URL_Archivos *) datos)[i]).URL);
            
            if (incrementarRanking(listaRecursos, palabras) < 0)
						{
							rankingError = 1;
							break;
						}
        }
				if (rankingError)
					WriteLog(log, "Query Manager", getpid(), thr_self(), "Error: no hay memoria, computar rakings incompleto", "ERROR");
				else
					WriteLog(log, "Query Manager", getpid(), thr_self(), "Computados OK", "INFOFIN");
    }

    /*Re-Envio la respuesta al Cliente en Front-End*/
    WriteLog(log, "Query Manager", getpid(), thr_self(), "Se reenviaran respuestas al Front-end", "INFO");
    if (ircResponse_send(sockCliente, descriptorID, datos, respuestaLen, mode) < 0)
    {
      printf("Error.\n");
      return -1;
    }

    WriteLog(log, "Query Manager", getpid(), thr_self(), "Respuesta enviada satisfactoriamente", "INFOFIN");

    return 0;    
}

int chequeoDeVida(in_addr_t ip, in_port_t puerto)
{
    /*Actualizo las estructuras para la proxima peticion*/
    char descIDQP[DESCRIPTORID_LEN];
    void *buff = NULL;
    int modeQPHandshake = IRC_REQUEST_IS_ALIVE;
    SOCKADDR_IN their_addr;
    int sockQP = establecerConexionServidor(ip, puerto, &their_addr);
    unsigned long rtaLen = 0;

    if (sockQP == INVALID_SOCKET)
    {
        WriteLog(log, "Query Manager", getpid(), thr_self(), "Error: conectar con el Query Processor", "ERROR");
        close(sockQP);
        return -1;
    }

    /*Envia al QP el permiso de peticion*/
    if (ircRequest_send(sockQP, NULL, 0, descIDQP, modeQPHandshake) < 0)
    {
        WriteLog(log, "Query Manager", getpid(), thr_self(), "Error: enviar consulta de vida al Query Processor", "ERROR");
        close(sockQP);
        return -1;
    }

    modeQPHandshake = 0x00;

    /*Recibir respuesta del permiso por parte del QP*/
    if (ircResponse_recv(sockQP, &buff, descIDQP, &rtaLen, &modeQPHandshake) < 0)
    {
        WriteLog(log, "Query Manager", getpid(), thr_self(), "Error: recibir respuesta de vida del Query Processor", "ERROR");
        close(sockQP);
        return -1;
    }

    close(sockQP);
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
        WriteLog(log, "Query Manager", getpid(), thr_self(), "Error: recibir mensaje del Front-end al tratar de conectarse", "ERROR");
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
        return -1;

    their_addr->sin_family = AF_INET;
    their_addr->sin_port = nPort;
    their_addr->sin_addr.s_addr = nDireccionIP;
    memset(&(their_addr->sin_zero),'\0',8);

    /*if (connect(sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1)
        return -1;*/

    while ( connect (sockfd, (SOCKADDR *)their_addr, sizeof(SOCKADDR)) == -1 && errno != EISCONN )
        if ( errno != EINTR )
        {
            close(sockfd);
            return -1;
    	}

    return sockfd;
}

void GenerarIDQP(char *cadenaOUT)
{

	int i;

	srand (time (NULL));
	for(i=0; i<MAX_ID_QP; i++)
	{
		cadenaOUT[i] = 'A'+ ( rand() % ('Z' - 'A') );
	}

	cadenaOUT[i] = '\0';
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
            return -1;

        /*if (ioctlsocket(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
        {
        return -1;
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
                return -1;
            else
            return sockfd;
        }
        else
            return -1;
    }

    return INVALID_SOCKET;
}



/*
Descripcion: Atiende error cada vez que exista en alguna funcion y finaliza proceso.
Ultima modificacion: Scheinkman, Mariano
Recibe: Error detectado.
Devuelve: -
*/
void rutinaDeError(char* error, int log)
{
    printf("\r\nError: %s\r\n", error);
    printf("Codigo de Error: %d\r\n\r\n", errno);
    perror(error);

    /*Mutua exclusion*/
    mutex_lock(&logMutex);
    WriteLog(log, "Front-end", getpid(), thr_self(), error, "ERROR");
    mutex_unlock(&logMutex);

	close(log);

    exit(EXIT_FAILURE);
}
