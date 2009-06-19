#include "http.h"
#include <string.h>

/*
Descripcion: Envia un bloque de datos
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, longitud del bloque, bloque
Devuelve: ok? bytes enviados: -1
*/
int EnviarBloque(SOCKET sockfd, unsigned long len, void *buffer)
{
    int bHastaAhora = 0;
    int bytesEnviados = 0;

    do {
        if ((bHastaAhora = send(sockfd, buffer, len, 0)) == -1){
                break;
        }
        bytesEnviados += bHastaAhora;
    } while (bytesEnviados != len);

    return bHastaAhora == -1? -1: bytesEnviados;
}


/*
Descripcion: Recibe n bytes de un bloque de datos
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, bloque, longitud del bloque
Devuelve: ok? bytes recibidos: -1
*/
int RecibirNBloque(SOCKET socket, void *buffer, unsigned long length)
{
    int bRecibidos = 0;
    int bHastaAhora = 0;

    do {
        if ((bHastaAhora = recv(socket, buffer, length, 0)) == -1) {
                break;
        }
        if (bHastaAhora == 0)
                return 0;
        bRecibidos += bHastaAhora;
    } while (bRecibidos != length);

    return bHastaAhora == -1? -1: bRecibidos;
}



/*
Descripcion: Recibe un bloque de datos que no se sabe la longitud
Ultima modificacion: Scheinkman, Mariano
Recibe: socket, bloque
Devuelve: ok? bytes recibidos: -1
*/
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
    char buffer[BUF_SIZE], *ptr;
    int bytesRecv=-1, error = 0;

    memset(buffer, '\0', BUF_SIZE);

    if ((bytesRecv = RecibirBloque(sockfd, buffer)) == -1)
        error = 1;
    else
    {
        int error = 1;
        char *palabras;

        ptr = buffer;
        for (;*ptr != '\0';ptr++)
        {
            if (!memcmp(ptr,"GET ", strlen("GET ")))
            {
                error = 0;
                ptr = ptr + strlen("GET ");
                palabras = ptr;
            }
            if (!error && *ptr == ' ')
            {
				memcpy(getInfo->filename, palabras, ptr - palabras);
                getInfo->filename[ptr - palabras] = '\0';
            }

            if (!error && !memcmp(ptr,"HTTP/1.", strlen("HTTP/1.")))
            {
                ptr = ptr + strlen("HTTP/1.");
                getInfo->protocolo = *ptr - '0';
                break;
            }
        }

        if (getInfo->protocolo != 0 && getInfo->protocolo != 1)
            error = 1;
    }
    if (error)
    {
        getInfo->protocolo = -1;
        lstrcpy(getInfo->filename, "");
        return -1;
    }
    else return 0;
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

int getFileType(const char *nombre, char *type)
{
	char *ptr;

	ptr = strrchr(nombre, '.');
	
	ptr++;
	lstrcpy(type, ptr);

	if (!strcmp(ptr, "jpg"))
		return JPG;
	if (!strcmp(ptr, "txt"))
		return TXT;
	if (!strcmp(ptr, "pdf"))
		return PDF;
	if (!strcmp(ptr, "exe"))
		return EXE;
	if (!strcmp(ptr, "zip"))
		return ZIP;
	if (!strcmp(ptr, "html"))
		return HTML;
	if (!strcmp(ptr, "doc"))
		return DOC;
	if (!strcmp(ptr, "xls"))
		return XLS;
	if (!strcmp(ptr, "ppt"))
		return PPT;
	if (!strcmp(ptr, "gif"))
		return GIF;
	if (!strcmp(ptr, "png"))
		return PNG;
	if (!strcmp(ptr, "jpeg"))
		return JPEG;
	if (!strcmp(ptr, "php"))
		return PHP;
	return ARCHIVO;
}

int httpOk_send(SOCKET sockfd, msgGet getInfo)
{
	
	char buffer[BUF_SIZE];
	char tipoArchivo[50];
	char type[MAX_FORMATO];
	int bytesSend = 0, error = 0;
	int fileType = -1;
	DWORD fileSize = -1;

	memset(buffer, '\0', sizeof(buffer));
	memset(tipoArchivo, '\0', sizeof(tipoArchivo));
	memset(type, '\0', sizeof(type));
	
	fileSize = getFileSize(getInfo.filename);
	fileType = getFileType(getInfo.filename, type);
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
	HANDLE fileHandle = INVALID_HANDLE_VALUE;
	int bEnviadosAux, bEnviadosTot = 0;
	int lenALeer = 0, error = 0, neof = 1;
	DWORD valor = 0;
	DWORD dato1 = 0, dato2 = 0;

	/*fileHandle = CreateFile(fileBuscado, GENERIC_READ, FILE_SHARE_READ, 
						NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_READONLY, NULL);*/	
	/*fileHandle = CreateFile(fileBuscado, GENERIC_READ, FILE_SHARE_READ, 
						NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_OVERLAPPED, NULL);*/

	memset(buf, '\0', sizeof(buf));

	fileHandle = CreateFile(fileBuscado, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 
						NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	fileSize = GetFileSize(fileHandle, NULL);

	dato1 = fileSize / BUF_SIZE;
	dato2 = fileSize % BUF_SIZE;

	SetFilePointer(fileHandle, 0, NULL, FILE_BEGIN);
	do 
	{
		memset(buf, '\0', sizeof(buf));
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

	printf("\nTamaño de archivo: %d, Enviado: %d\n", fileSize, bEnviadosTot);
	
	CloseHandle(fileHandle);

	if (fileSize != bEnviadosTot)
		error = 1;
	return error == 0? bEnviadosTot: -1;
}

int BuscarArchivo(char *filename)
{
	WIN32_FIND_DATA findData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	int encontro = 0;

	memset(&findData, '\0', sizeof(WIN32_FIND_DATA));

	hFind = FindFirstFile(filename, &findData);

	if (hFind != INVALID_HANDLE_VALUE)
		encontro = 1;
	else if (hFind == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
		encontro = 0;
	else
		encontro = -1;

	return encontro;
}

void pathUnixToWin(const char *dir, char *path, char *pathBuscado)
{
    char filename[MAX_PATH];
    char *i, *j, *lim = path + lstrlen(path);
	int k;

	memset(filename, '\0', sizeof(filename));

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
	
	lstrcpy(pathBuscado, path);
    return;
}

char *getFilename(const char *path)
{    
	char *filename = strrchr(path, '\\');
	
	return ++filename;
}

int getKeywords (const char *filename, char **palabras)
{
    char nombre[MAX_PATH], *word, *ptr, *lim;
    int i=0;
    
    char *vp = NULL;
    
    if ( (vp = HeapAlloc(GetProcessHeap(), 0, sizeof(char))) == NULL)
       return -1;
    
    lstrcpy(nombre, filename);
    lim = strchr(nombre, '\0');
    
    for(ptr = word = nombre; ptr != lim; ptr++)
	{		
       if (*ptr == '_' || *ptr == '.')
       {
			char keyword[PALABRA_SIZE];

			*ptr = '\0';
			wsprintf(keyword, "%s,", word);

			if ((vp = HeapReAlloc(GetProcessHeap(), 0, vp, strlen(vp) + strlen(keyword)+1)) == NULL)
				return -1;

			lstrcat(vp, keyword);
			word = ptr+1;
       }
    }

    *palabras = vp;
        
    return 0;
}
