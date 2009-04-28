#include "http.h"
#include <string.h>

int EnviarBloque(SOCKET sockfd, DWORD bAEnviar, char *bloque) 
{

	DWORD bHastaAhora, bEnviados;	
	int error = 0;

	bHastaAhora = bEnviados = 0;
 	while (!error && bEnviados < bAEnviar) 
	{
		if ((bHastaAhora = send(sockfd, bloque+bEnviados, bAEnviar, 0)) == -1) 
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

int RecibirNBloque(SOCKET sockfd, char *bloque, DWORD nBytes)
{
	int bRecibidos = 0;
	int aRecibir = nBytes;
	int bHastaAhora = 0;

	do
	{
		if ((bHastaAhora = recv(sockfd, bloque+bRecibidos, aRecibir, 0)) == -1) 
		{
			printf("Error en la funcion recv.\n\n");
			return -1;
		}
		aRecibir -= bHastaAhora;
		bRecibidos += bHastaAhora;
	} while (aRecibir > 0);
	
	return bRecibidos;
}


int RecibirBloque(SOCKET sockfd, char *bloque) {
	
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
		if ((bHastaAhora = recv(sockfd, bloque+bRecibidos, BUF_SIZE, 0)) == -1 && GetLastError() != WSAEWOULDBLOCK) 
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
	char buffer[BUF_SIZE], *ptr, *nextT;
	char header[4];
	int bytesRecv, error = 0;

	if ((bytesRecv = RecibirBloque(sockfd, buffer)) == -1)
		error = 1;
	else
	{
		buffer[bytesRecv+1] = '\0';
		
		lstrcpy(header, strtok_s(buffer, " ", &nextT));

		if(!lstrcmp(header, "GET"))
		{
			char *filename = getInfo->filename;
			
			lstrcpy(filename, strtok_s(NULL, " ", &nextT));
			filename[strlen(filename)] = '\0';
			filename;
			lstrcpy(getInfo->filename,filename);
			
			ptr = strtok_s(NULL, ".", &nextT);
			getInfo->protocolo = atoi(strtok_s(NULL, "\r\n", &nextT));

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

DWORD getFileSize(const char *nombre)
{	
	DWORD size;
	HANDLE fileHandle = CreateFile(nombre, GENERIC_READ, FILE_SHARE_READ, 
						NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_READONLY, NULL);
	size = GetFileSize(fileHandle, NULL);
	CloseHandle(fileHandle);

	return size;
}

int getFileType(const char *nombre)
{
	char *ptr;

	ptr = strrchr(nombre, '.');
	
	ptr++;

	if (!lstrcmp(ptr, "jpg"))
		return JPG;
	if (!lstrcmp(ptr, "txt"))
		return TXT;
	if (!lstrcmp(ptr, "pdf"))
		return PDF;
	if (!lstrcmp(ptr, "exe"))
		return EXE;
	if (!lstrcmp(ptr, "zip"))
		return ZIP;
	if (!lstrcmp(ptr, "html"))
		return HTML;
	if (!lstrcmp(ptr, "doc"))
		return DOC;
	if (!lstrcmp(ptr, "xls"))
		return XLS;
	if (!lstrcmp(ptr, "ppt"))
		return PPT;
	if (!lstrcmp(ptr, "gif"))
		return GIF;
	if (!lstrcmp(ptr, "png"))
		return PNG;
	if (!lstrcmp(ptr, "jpeg"))
		return JPEG;
	if (!lstrcmp(ptr, "php"))
		return PHP;
	return ARCHIVO;
}

int httpOk_send(SOCKET sockfd, msgGet getInfo)
{
	
	char buffer[BUF_SIZE];
	char tipoArchivo[50];
	int bytesSend = 0, error = 0;
	int fileType;
	DWORD fileSize;

	ZeroMemory(buffer, BUF_SIZE);
	
	fileSize = getFileSize(getInfo.filename);
	fileType = getFileType(getInfo.filename);
	switch (fileType)
	{
	case HTML:
		lstrcpy(tipoArchivo, "text/html");
		break;
	case TXT:
		lstrcpy(tipoArchivo, "text/txt");
		break;
	case PHP:
		lstrcpy(tipoArchivo, "text/php");
		break;
	case JPEG:
		lstrcpy(tipoArchivo, "image/jpeg");
		break;
	case JPG:
		lstrcpy(tipoArchivo, "image/jpg");
		break;
	case PNG:
		lstrcpy(tipoArchivo, "image/png");
		break;
	case GIF:
		lstrcpy(tipoArchivo, "image/gif");
		break;
	case EXE:
		lstrcpy(tipoArchivo, "application/octet-stream");
		break;
	case ZIP:
		lstrcpy(tipoArchivo, "application/zip");
		break;
	case DOC:
		lstrcpy(tipoArchivo, "application/msword");
		break;
	case XLS:
		lstrcpy(tipoArchivo, "application/vnd.ms-excel");
		break;
	case PDF:
		lstrcpy(tipoArchivo, "application/pdf");
		break;
	case PPT:
		lstrcpy(tipoArchivo, "application/vnd.ms-powerpoint");
		break;
	case ARCHIVO:
		lstrcpy(tipoArchivo, "application/force-download");
		break;
	}

	if (fileType > JPEG) /*No es ni foto ni texto*/
	{
		/*Crea el Buffer con el protocolo*/
		sprintf_s(buffer, sizeof(buffer), "HTTP/1.%d 200 OK\nContent-type: %s\n"
									"Content-Disposition: attachment; filename=\"%s\"\n"
									"Content-Transfer-Encoding: binary\n"
									"Accept-Ranges: bytes\n"
									"Cache-Control: private\n"
									"Pragma: private\n"
									"Expires: Mon, 26 Jul 1997 05:00:00 GMT\n"
									"Content-Length: %ld\n\n", getInfo.protocolo, tipoArchivo, 
									getFilename(getInfo.filename), fileSize);
	
	}
	else
	{
		sprintf_s(buffer, sizeof(buffer), "HTTP/1.%d 200 OK\nContent-type: %s\n\n", getInfo.protocolo, tipoArchivo);
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


int EnviarArchivo(SOCKET sockRemoto, char *fileBuscado) 
{
	
	char buf[BUF_SIZE];
	DWORD fileSize, bAEnviar = 0;
	HANDLE fileHandle;
	int bEnviadosAux, bEnviadosTot = 0;
	int lenALeer = 0, error = 0, neof = 1;
	DWORD valor;
	DWORD dato1, dato2;

	fileHandle = CreateFile(fileBuscado, GENERIC_READ, FILE_SHARE_READ, 
						NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_READONLY, NULL);
	
	fileSize = GetFileSize(fileHandle, NULL);

	dato1 = fileSize / BUF_SIZE;
	dato2 = fileSize % BUF_SIZE;

	SetFilePointer(fileHandle, 0, NULL, FILE_BEGIN);
	do 
	{
		ZeroMemory(buf, BUF_SIZE);
		if (FALSE == ReadFile(fileHandle, buf, BUF_SIZE, &bAEnviar, NULL))
		{
			printf("Error en ReadFile");
			error = 1;	
		}
		if (bAEnviar > 0)
		{
			if ((bEnviadosAux = EnviarBloque(sockRemoto, bAEnviar, buf)) == -1) 
				error = 1;
			bEnviadosTot += bEnviadosAux;				
		}
		
		valor = SetFilePointer(fileHandle, 0, NULL, FILE_CURRENT);
	} while(bAEnviar > 0);

	printf("\nTama�o de archivo: %d, Enviado: %d\n", fileSize, bEnviadosTot);
	
	CloseHandle(fileHandle);

	if (fileSize != bEnviadosTot)
		error = 1;
	return error == 0? bEnviadosTot: -1;
}

int BuscarArchivo(char *filename)
{
	WIN32_FIND_DATA findData;
	HANDLE hFind;

	hFind = FindFirstFile(filename, &findData);

	if (hFind == INVALID_HANDLE_VALUE)
		return 0;
	FindClose(hFind);

	return 1;
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
