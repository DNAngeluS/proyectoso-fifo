#include "http.h"
#include <string.h>

int EnviarBloque(SOCKET sockfd, DWORD bAEnviar, LPVOID bloque) 
{

	int bHastaAhora,bEnviados, error = 0;

	bHastaAhora = bEnviados = 0;
 	while (!error && bAEnviar != 0) 
	{
		if ((bHastaAhora = send(sockfd, bloque, bAEnviar, 0)) == -1) 
		{
			printf("Error en la funcion send.\n\n");
			error = 1;
			break;
		}
		else 
		{
			bEnviados += bHastaAhora;			
			bAEnviar -= bHastaAhora;
		}
	}
	return error == 0? bEnviados: -1;
}

int RecibirNBloque(SOCKET sockfd, LPVOID bloque, DWORD nBytes)
{
	int bRecibidos = 0;
	int aRecibir = nBytes;
	int bHastaAhora = 0;

	do
	{
		if ((bHastaAhora = recv(sockfd, bloque, aRecibir, 0)) == -1) 
		{
			printf("Error en la funcion recv.\n\n");
			return -1;
		}
		aRecibir -= bHastaAhora;
		bRecibidos += bHastaAhora;
	} while (aRecibir > 0);
	
	return bRecibidos;
}

int RecibirBloque(SOCKET sockfd, LPVOID bloque) {
	
	int bRecibidos = 0;
	int bHastaAhora = 0;
	int NonBlock = 1;

	do
	{
		if (bRecibidos != 0)
		{
			/*Se pone el socket como no bloqueante*/
			if (ioctlsocket(sockfd, FIONBIO, &NonBlock) == SOCKET_ERROR)
			{
				printf("ioctlsocket");
				return -1;
			}
		}
		if ((bHastaAhora = recv(sockfd, bloque, BUF_SIZE, 0)) == -1 && GetLastError() != WSAEWOULDBLOCK) 
		{
			printf("Error en la funcion recv.\n\n");
			return -1;
		}
		if (GetLastError() == WSAEWOULDBLOCK)
			break;
		bRecibidos += bHastaAhora;
	} while (1);
	
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
	char buffer[BUF_SIZE], *ptr;
	char header[4];
	int bytesRecv, error = 0;

	if ((bytesRecv = RecibirBloque(sockfd, buffer)) == -1)
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
			filename;
			lstrcpy(getInfo->filename,filename);
			
			ptr = strtok(NULL, ".");
			getInfo->protocolo = atoi(strtok(NULL, "\r\n"));

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

int getFileType(const char *nombre)
{
	char *ptr;

	ptr = strrchr(nombre, '.');
	
	ptr++;

	if (!lstrcmp(ptr, "jpg") || !lstrcmp(ptr, "jpeg") || !lstrcmp(ptr, "gif") || !lstrcmp(ptr, "ttf") || !lstrcmp(ptr, "bmp"))
		return IMAGEN;
	else if (!lstrcmp(ptr, "html"))
		return HTML;
	else
		return ARCHIVO;
}

int httpOk_send(SOCKET sockfd, msgGet getInfo){
	
	char buffer[BUF_SIZE];
	char tipoArchivo[50];
	char filename[30];
	int bytesSend = 0, error = 0;
	int fileType;

	ZeroMemory(buffer, BUF_SIZE);
	
	fileType = getFileType(getInfo.filename);

	switch (fileType)
	{
	case HTML:
		lstrcpy(tipoArchivo, "text/html");
		break;
	case IMAGEN:
		lstrcpy(tipoArchivo, "image/jpeg");
		break;
	case ARCHIVO:
		lstrcpy(tipoArchivo, "application/octet-stream");
		break;
	}
	
	/*Crea el Buffer con el protocolo*/
	sprintf_s(buffer, sizeof(buffer), "HTTP/1.%d 200 OK\nContent-type: %s\n", getInfo.protocolo, tipoArchivo);
	
	if (fileType == ARCHIVO || fileType == IMAGEN)
	{
		lstrcat(buffer, "Content-Disposition: attachment; filename=\"");
		lstrcat(buffer, getFilename(getInfo.filename));
		lstrcat(buffer, "\"\n");
	}

	/*Enviamos el buffer como stream (sin el \0)*/
	if (EnviarBloque(sockfd, lstrlen(buffer), buffer) == -1)
		error = 1;

	return error? -1: 0;
}

int httpNotFound_send(SOCKET sockfd, msgGet getInfo){
	char buffer[BUF_SIZE];
	
	sprintf_s(buffer, sizeof(buffer), "HTTP/1.%d 404 Not Found\n\n<b>ERROR</b>: File %s was not found\n", getInfo.protocolo, getInfo.filename);
	
	
	/*Enviamos el buffer como stream (sin el \0)*/
	if (EnviarBloque(sockfd, lstrlen(buffer), buffer) == -1 )
		return -1;
	else
		return 0;
}

int httpTimeout_send(SOCKET sockfd, msgGet getInfo)
{
	char buffer[BUF_SIZE];

	sprintf_s(buffer, sizeof(buffer), "HTTP/1.%d 408 Request Timeout\r\n", getInfo.protocolo);
	
	/*Enviamos el buffer como stream (sin el \0)*/
	if (EnviarBloque(sockfd, lstrlen(buffer), buffer) == -1)
		return -1;
	else
		return 0;
}

int EnviarArchivo(SOCKET sockRemoto, HANDLE fileHandle) 
{
	
	char buf[BUF_SIZE];
	DWORD fileSize;

	int cantBloques, bUltBloque, i;
	int bEnviadosAux, bEnviadosBloque, bAEnviar, bEnviadosTot;
	int lenALeer, error = 0;
 
	fileSize = GetFileSize(fileHandle, NULL);
	cantBloques = fileSize / BUF_SIZE;
	bUltBloque = fileSize % BUF_SIZE;
	
	for (i=bEnviadosTot=0; !error && i<=cantBloques; i++) 
	{	 	
	  	if (i<cantBloques)
	 		lenALeer = BUF_SIZE;
	 	else
	 		lenALeer = bUltBloque;	
	 	
	 	bEnviadosAux = bAEnviar = bEnviadosBloque = 0;
	 	
	 	while (!error && lenALeer != 0) 
		{
	 		bEnviadosAux = bAEnviar = 0;
			if (ReadFile(fileHandle, buf, lenALeer, &bAEnviar, NULL) == -1) 
			{
				printf("Error en ReadFile");
				error = 1;
				break;
			}
			else 
			{
				if ((bEnviadosAux = EnviarBloque(sockRemoto, bAEnviar, buf)) == -1) 
					error = 1;
				bEnviadosBloque += bEnviadosAux;
				lenALeer -= bEnviadosAux;
			}
	 	}
	 	bEnviadosTot+=bEnviadosBloque;
	}
		
	printf("\nTamaño de archivo: %d, Enviado: %d\n", fileSize, bEnviadosTot);
	CloseHandle(fileHandle);

	if (fileSize != bEnviadosTot)
		error = 1;
	return error == 0? bEnviadosTot: -1;
}

HANDLE BuscarArchivo(char *filename)
{
	WIN32_FIND_DATA findData;
	HANDLE hFind;
	HANDLE file ;

	hFind = FindFirstFile(filename, &findData);

	if (hFind == INVALID_HANDLE_VALUE)
		return hFind;
	
	file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 
						NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	FindClose(hFind);
	return file;
}

char *pathUnixToWin(const char *dir, char *path)
{
    char filename[MAX_PATH];
    char *i, *j, *lim = path + lstrlen(path);
	int k;

    /* Buscamos el ultimo nombre, el del archivo */
	j = path;
	i = path;

	for(k = 0; k < lstrlen(path); ++k)
		if (i[k]=='/')
			j[k] = '\\';
		else			
			j[k] = i[k];

    strcpy_s(filename, MAX_PATH, j);
    sprintf_s(path, MAX_PATH, "%s%s", dir, filename);
	
    return path;
}

char *getFilename(const char *path)
{    
	char *filename = strrchr(path, '\\');
	
	return ++filename;
}
