#include "http.h"

int EnviarBloque(SOCKET sockfd, DWORD bAEnviar, LPVOID bloque) {

	int bHastaAhora,bEnviados, error = 0;
	
	bHastaAhora = bEnviados = 0;
 	while (!error && bAEnviar != 0) {
		if ((bHastaAhora = send(sockfd, bloque, bAEnviar, 0)) == -1) {
			printf("Error en la funcion send.\n\n");
			error = 1;
			break;
		}
		else {
			bEnviados += bHastaAhora;			
			bAEnviar -= bHastaAhora;
		}
	}
	return error == 0? bEnviados: -1;
}

int RecibirBloque(SOCKET sockfd, LPVOID bloque, DWORD bARecibir) {
	
	int bRecibidos = 0;
	int aRecibir = bARecibir;
	int bHastaAhora = 0;
	int NonBlock;

	do
	{
		/*Se pone el socket como no bloqueante*/
		int NonBlock = 1;
		if (ioctlsocket(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
		{
			printf("ioctlsocket");
			return -1;
		}
		if ((bHastaAhora = recv(sockfd, bloque, aRecibir, 0)) == -1 && GetLastError() != WSAEWOULDBLOCK) 
		{
			printf("Error en la funcion recv.\n\n");
			return -1;
		}
		if (GetLastError() == WSAEWOULDBLOCK)
			break;
		aRecibir -= bHastaAhora;
		bRecibidos += bHastaAhora;
	} while (bRecibidos != bARecibir);
	
	/*Se vuelve a poner como bloqueante*/
	NonBlock = 0;
	if (ioctlsocket(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
	{
		printf("ioctlsocket");
		return -1;
	}
	
	return bRecibidos;
}

int httpGet_recv(SOCKET sockfd, msgGet *getInfo)
{
	char buffer[BUF_SIZE], *ptr, *ptr2;
	char header[4];
	int bytesRecv, error = 0;

	

	if ((bytesRecv = RecibirBloque(sockfd, buffer, sizeof(buffer))) == -1)
		error = 1;
	else
	{
		buffer[bytesRecv+1] = '\0';
		
		lstrcpy(header, strtok(buffer, " "));

		if(!lstrcmp(header, "GET"))
		{
			char *filename = getInfo->filename;
			
			lstrcpy(filename, strtok(NULL, " "));
			filename[strlen(filename)] = '\0';
			filename++;
			lstrcpy(getInfo->filename,filename);
			
			ptr = strtok(NULL, ".");
			getInfo->protocolo = (ptr2 = strtok(NULL, "\r\n"))[0] - '0';

			if (getInfo->protocolo != 0 && getInfo->protocolo != 1)
				error = 1;
		}
		else
			error = 1;
	}
	if (error)
	{
		getInfo->protocolo = -1;
		lstrcpy(getInfo->filename, "");
		return -1;
	}
	else
		return 0;
}