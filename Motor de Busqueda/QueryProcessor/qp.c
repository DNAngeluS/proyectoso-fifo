/*
 * File:   qp.c
 * Author: Lucifer
 *
 * Created on 2 de mayo de 2009, 23:15
 */

#include "qp.h"
#include "config.h"
#include "irc.h"
#include "http.h"


void rutinaDeError(char *string);
SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort);

headerIRC recibirPedido(SOCKET sockCli);
void* consultarLDAP(headerIRC buscar);
headerIRC construirIRC(void *objeto);
void enviarPedido(headerIRC irc);


int main(){
  SOCKET sockQP, sockCli;
  SOCKADDR_IN dirQP, dirCliente; //Address info del cliente
  configuracion config;  
  headerIRC irc, buscar;
  void *objeto;

  /*Lectura de Archivo de Configuracion*/
  if (leerArchivoConfiguracion(&config) != 0)
      rutinaDeError("Lectura Archivo de configuracion");

  /*Se establece conexion con el puerto de escucha*/
  if ((sockQP = establecerConexionQP(config.ip, config.puerto, (SOCKADDR *)&dirQP)) == INVALID_SOCKET)
      rutinaDeError("Socket invalido");

  while(1){
      int nAddrSize = sizeof(dirCliente);

      /*Acepta la conexion entrante. Ahora FrontEnd*/
      sockCli = accept(sockQP, (SOCKADDR *) &dirCliente, &nAddrSize);
      
      if (sockCli != INVALID_SOCKET)
          continue;
      
      printf ("Conexion aceptada de %s:%d.\r\n\r\n", inet_ntoa(dirCliente.sin_addr),
                                                        ntohs(dirCliente.sin_port));
      /*Recibe las palabras a buscar*/
      buscar = recibirPedido(sockCli);
      /*Consulta en el Directorio openDS las palabras*/
      objeto = consultarLDAP(buscar);
      /*Construye el IRC para mandar los datos encontrados*/
      irc = construirIRC(objeto);
      /*envia el IRC con los datos encontrados*/
      enviarPedido(irc);

  }
  /*cierra las conexiones*/
  close(sockQP);
  close(sockCli);
  
  return (EXIT_SUCCESS);
}

headerIRC recibirPedido(SOCKET sockCli){
    headerIRC buscar;
 
    return buscar;
}
void* consultarLDAP(headerIRC buscar){
    void *ptr;

    return ptr;
}
headerIRC construirIRC(void *objeto){
    headerIRC irc;

    return irc;
}
void enviarPedido(headerIRC irc){

    return;
}

/*Descripcion: Establece la conexion del servidor web a la escucha en un puerto y direccion ip.
Ultima modificacion: Moya, Lucio
Recibe: Direccion ip y puerto a escuchar.
Devuelve: ok? Socket del servidor: socket invalido.
*/
SOCKET establecerConexionEscucha(in_addr_t nDireccionIP, in_port_t nPort){
  SOCKET sockfd;
  /*Obtiene un socket*/
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd != INVALID_SOCKET){
      SOCKADDR_IN addrQP; /*Address del Web server*/
      char yes = 1;
      int NonBlock = 1;

      /*Impide el error "addres already in use" y setea non blocking el socket*/
      if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1)
          rutinaDeError("Setsockopt");
      /*if (ioctlsocket(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR){
       * rutinaDeError("ioctlsocket");
       * }*/
      addrQP.sin_family = AF_INET;
      addrQP.sin_addr.s_addr = nDireccionIP;
      addrQP.sin_port = nPort;
      memset((&addrQP.sin_zero), '\0', 8);
      /*Liga socket al puerto y direccion*/
      if ((bind (sockfd, (SOCKADDR *) &addrQP, sizeof(SOCKADDR_IN))) != SOCKET_ERROR){
          /*Pone el puerto a la escucha de conexiones entrantes*/
          if (listen(sockfd, SOMAXCONN) == -1)
              rutinaDeError("Listen");
          else
              return sockfd;
      }else
          rutinaDeError("Bind");
  }
  return INVALID_SOCKET;
}
