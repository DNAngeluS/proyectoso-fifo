/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "5500"  /* the port users will be connecting to*/

#define BACKLOG 10	 /* how many pending connections queue will hold*/

int establecerConexion();

void *get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(void)
{
	int sockfd;  /* listen on sock_fd, new connection on new_fd*/	
	
	sockfd = -1;
	sockfd = establecerConexion();
	
	while(1){
		int new_fd;
		struct sockaddr_storage their_addr; /* connector's address information*/
		socklen_t sin_size;
		char s[INET6_ADDRSTRLEN];

		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
		printf("server: new connection accepted from, %s\n", s);
	
	}


	return 0;
}

int establecerConexion(){
	int sockfd;
	struct addrinfo hints, *servinfo, *p;

	char yes='1';
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; /* use my IP*/

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	/* loop through all the results and bind to the first we can*/
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); /* all done with this structure*/

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		return 3;
	}

	printf("server: waiting for connections...\n");

	return sockfd;
}

