/*
    ** client.c -- Ejemplo de cliente de sockets de flujo
    */

#include "craw.h"
#include "irc.h"
 
#define PORT 8888 // puerto al que vamos a conectar 

#define MAXDATASIZE 100 // máximo número de bytes que se pueden leer de una vez 

int main(int argc, char *argv[])
{
    int sockfd, numbytes; 
	char descID[16];
    crawler_URL paquete;
    struct hostent *he;
    struct sockaddr_in their_addr; // información de la dirección de destino 

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    if ((he=gethostbyname(argv[1])) == NULL) {  // obtener información de máquina 
        perror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET;    // Ordenación de bytes de la máquina 
    their_addr.sin_port = htons(PORT);  // short, Ordenación de bytes de la red 
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(their_addr.sin_zero), '\0', 8);  // poner a cero el resto de la estructura 

    if (connect(sockfd, (struct sockaddr *)&their_addr,
                                          sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

	memset(&paquete, '\0', sizeof(crawler_URL));
	strcpy(paquete.URL, "http://2:3/hola.html");
	strcpy(paquete.titulo, "Pagina ppal Hola");
	strcpy(paquete.descripcion, "pagina con 2 holas");
	strcpy(paquete.htmlCode, "hola hola");
	paquete.palabras = NULL;

	ircRequest_send(sockfd, (void *) &paquete, sizeof(crawler_URL), descID, IRC_CRAWLER_ALTA_HTML);

    
	sleep(5);

	close(sockfd);

    return 0;
}








