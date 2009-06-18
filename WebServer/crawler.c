#include "crawler.h"

static int forAllFiles(char *directorio, int (*funcion) (WIN32_FIND_DATA));
static int generarPaqueteArchivos(const char *filename, crawler_URL *paquete);
static int rutinaTrabajoCrawler(WIN32_FIND_DATA filedata);
static int comprobarCondicionesMigracion(unsigned esperaCrawler);

extern configuracion config;
extern int crawPresence;
extern DWORD crawTimeStamp;
extern struct hashManager hashman;
extern HANDLE logMutex;
extern HANDLE crawMutex;
extern int codop;

static int comprobarCondicionesMigracion(unsigned esperaCrawler)
{
	return  ((GetTickCount() - crawTimeStamp) >= esperaCrawler) 
			&& (crawPresence <= 0);		
}

void rutinaAtencionCrawler (LPVOID args)
{
	int pvez = (crawPresence == -1);
	int error = 0;
	char logMsg[BUF_SIZE];
    int logMsgSize, bytesWritten, i;
    SYSTEMTIME time;
    DWORD idProcess;
	DWORD idThread;
	DWORD inicio = GetTickCount();

    idProcess = GetProcessId(GetCurrentProcess());
	idThread = GetCurrentThreadId();

    GetLocalTime(&time);
	logMsgSize = sprintf_s(logMsg, BUF_SIZE, "%d/%d/%d %02d:%02d:%02d Web Crawler %d-%d: Ingreso al sistema.\r\n\r\n",
                                        time.wDay, time.wMonth, time.wYear,
										time.wHour, time.wMinute, time.wSecond, idProcess, idThread);

    /* Para escribir el archivo log exigimos mutual exception */
    WaitForSingleObject(logMutex, INFINITE);
    WriteFile(config.archivoLog, logMsg, logMsgSize, &bytesWritten, NULL);
    ReleaseMutex(logMutex);
	
	_CrtDumpMemoryLeaks();

	/*Mutua exclusion para la variable global de presencia de Crawler*/
	WaitForSingleObject(crawMutex, INFINITE);
	crawPresence = 1;
	ReleaseMutex(crawMutex);
			
	/*Procesar los archivos del directorio*/
	if ((error = forAllFiles(config.directorioFiles, rutinaTrabajoCrawler)) < 0)
		printf("Crawler: Error al procesar archivos. Se descarta Crawler.\r\n\r\n");	
	
	/*Deja consistente el estado del Hash Manager*/
	for (i = 0; i < HASHSIZE; i++)
		if (hashman.flag[i] == RECIENTEMENTE_ACCEDIDO)
			hashman.flag[i] = LLENO;

	printf("Analisis del Web Crawler a finalizado %s.\r\n\r\n", error<0? "con error.":"satisfactoriamente");
	
	logMsgSize = sprintf_s(logMsg, BUF_SIZE, "%d/%d/%d %02d:%02d:%02d Web Crawler %d-%d: finalizado el analisis de documentos en %d milisegundos.\r\n\r\n",
                                        time.wDay, time.wMonth, time.wYear,
										time.wHour, time.wMinute, time.wSecond, 
										idProcess, idThread, GetTickCount() - inicio);

	WaitForSingleObject(crawMutex, INFINITE);
	WriteFile(config.archivoLog, logMsg, logMsgSize, &bytesWritten, NULL);
	crawTimeStamp = GetTickCount();
	crawPresence = 0;
	ReleaseMutex(crawMutex);

	_endthreadex(0);
}

static int forAllFiles(char *directorio, int (*funcion) (WIN32_FIND_DATA))
{
	WIN32_FIND_DATA ffd;
	char szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	int archivosProcesados = 0;

	/*Chequea la longuitud del directorio*/
	length_of_arg = lstrlen(directorio);

	if (length_of_arg > (MAX_PATH - 2))
	{
		printf("Path del Directorio muy largo.\r\n");
		return -1;
	}

	/*Agrega \* al nombre del directorio*/
	lstrcpy(szDir, directorio);
	lstrcat(szDir, "\\*");

	/*Obtiene el primer archivo del directorio*/
	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind) 
	{
		printf("Error.\r\n");
		printf("FindFirstFile: %d\r\n\\r\n", GetLastError());
		return -1;
	} 

	/*Recorre todos los archivos del directorio y sus subdirectorios y les aplica la funcion a cada uno*/
	do
	{
		if (!strcmp(ffd.cFileName, ".") || !strcmp(ffd.cFileName, ".."))
			continue;
		if (ffd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			char newDir[MAX_PATH];

			wsprintf(newDir, "%s\\%s", directorio, ffd.cFileName);
			if (forAllFiles(newDir, funcion) < 0)
				return -1;
		}
		else
		{
			int control;

			if ((control = funcion(ffd)) < 0)
				return -1;
			if (control != ELIMINA_NO_PUBLICO)
				archivosProcesados++;
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if (archivosProcesados != hashman.ocupados)
	{
		int i;

		for (i = 0; i < HASHSIZE; i++)
			if (hashman.hashtab[i] != NULL && hashman.flag[i] == LLENO)
				if (hashClean(hashman.hashtab[i]->file) < 0)
					return -1;
	}

	if (GetLastError() != ERROR_NO_MORE_FILES)
	{
		printf("Error.\r\n");
		printf("FindFirstFile: %d\r\n\\r\n", GetLastError());
		return -1;
	}

	FindClose(hFind);
	return 0;
}

static int rutinaTrabajoCrawler(WIN32_FIND_DATA filedata)
{
	DWORD attr = filedata.dwFileAttributes;
	struct nlist *np = hashLookup(filedata.cFileName);
	char *filename = filedata.cFileName;
	char type[MAX_FORMATO];
	char md5[MAX_PATH];
	crawler_URL paquete;
	int mode;
	int tipoProceso = ARCHIVO_NO_SUFRIO_CAMBIOS;

	hashMD5(filename, config.directorioFiles, md5);

	if (np != NULL && attr == FILE_ATTRIBUTE_READONLY)
	{
		if (hashClean(filename) < 0)
			return -1;
		tipoProceso = ELIMINA_NO_PUBLICO;
	}
	else if ((np == NULL && attr != FILE_ATTRIBUTE_READONLY) || 
			(np != NULL && attr != FILE_ATTRIBUTE_READONLY && strcmp(np->md5, md5)))
	{
		int palabrasLen = 0;

		if (getFileType(filename, type) == HTML)
		{
			if (np == NULL && attr != FILE_ATTRIBUTE_READONLY)
				mode = IRC_CRAWLER_ALTA_HTML;
			else
				mode = IRC_CRAWLER_MODIFICACION_HTML;

			if (parsearHtml(filename, &paquete) < 0)
				return -1;
		}
		else
		{
			if (np == NULL && attr != FILE_ATTRIBUTE_READONLY)
				mode = IRC_CRAWLER_ALTA_ARCHIVOS;
			else
				mode = IRC_CRAWLER_MODIFICACION_ARCHIVOS;

			if (generarPaqueteArchivos(filename, &paquete) < 0)
				return -1;
		}

		/*Se envia el paquete al Web Store*/
		if (enviarPaquete(config.ipWebStore, config.puertoWebStore, &paquete, mode, (int) (strlen(paquete.palabras)+1)) < 0)
			printf("Error en el envio de Paquete al Web Store para %s.\r\n\r\n", filename);

		tipoProceso = ATIENDE_NUEVO_O_MODIFICADO;
		if (hashInstall(filename, md5) < 0)
			return -1;
	}
	else
		if (np != NULL)
			hashman.flag[hash(np->file)] = RECIENTEMENTE_ACCEDIDO;

	_CrtDumpMemoryLeaks();

	return tipoProceso;
}


static int generarPaqueteArchivos(const char *filename, crawler_URL *paquete)
{   
    int ntype;
	DWORD size;
    char type[5];
	char filedir[MAX_PATH];	

    ntype = getFileType(filename, type);
	if (ntype < 0) return -1;

	lstrcpy(paquete->formato, type);
	if (ntype == JPG || ntype == GIF || ntype == PNG || ntype == JPEG)
		lstrcpy(paquete->tipo, "1");
	else
		lstrcpy(paquete->tipo, "2");

	wsprintf(filedir, "%s\\%s", config.directorioFiles, filename);
	size = getFileSize(filedir);
	if (size < 0) return -1;
	wsprintf(paquete->length, "%ld", size);

	if (getKeywords(filename, &paquete->palabras) < 0)
		return -1;
	wsprintf(paquete->URL, "http://%s:%d/%s", inet_ntoa(*(IN_ADDR *)&config.ip), ntohs(config.puerto), filename);

    printf("Size: %s\r\n", paquete->length);
	printf("Type: %s\r\n", paquete->tipo);
	printf("URL: %s\r\n", paquete->URL);
	printf("Formato: %s\r\n", paquete->formato);
	printf("Palabras: %s\r\n", paquete->palabras);
	printf("\r\n");

	return 0;
}


int EnviarCrawler(in_addr_t nDireccion, in_port_t nPort)
{
    SOCKET sockWebServer;
    SOCKADDR_IN dirServidorWeb;
    char descriptorID[DESCRIPTORID_LEN];
    char buf[BUF_SIZE];
    int mode = 0x00;

    /*Se levanta conexion con el Web Server*/
    if ((sockWebServer = establecerConexionServidor(nDireccion, nPort, &dirServidorWeb)) < 0)
        return -1;
    printf("Se establecio conexion con WebServer en %s.\n", inet_ntoa(dirServidorWeb.sin_addr));

    /*Se envia mensaje de instanciacion de un Crawler*/
    if (ircRequest_send(sockWebServer, NULL, 0, descriptorID, IRC_CRAWLER_CONNECT) < 0)
    {
        closesocket(sockWebServer);
        return -1;
    }
    printf("Crawler disparado a %s.\n\n", inet_ntoa(dirServidorWeb.sin_addr));

    if (ircRequest_recv (sockWebServer, buf, descriptorID, &mode) < 0)
       return -1;

    if (mode == IRC_CRAWLER_OK)
       printf("Crawler a migrado safisfactoriamente a %s.\n\n", inet_ntoa(dirServidorWeb.sin_addr));
    else if (mode == IRC_CRAWLER_FAIL)
       printf("Crawler a sido rechazado de %s.\n\n", inet_ntoa(dirServidorWeb.sin_addr));
    else
       printf("Error en mensaje IRC, se descarta.\n\n");

    closesocket(sockWebServer);
    
    return 0;
}

/*Devuelve ok? socket, -1 si error, -2 si outofservice, -3 si no se cumplen condiciones de migracion*/
int rutinaConexionCrawler(SOCKET sockWebServer)
{
	SOCKET sockCrawler;					/*Socket del cliente remoto*/
	SOCKADDR_IN dirCrawler;				/*Direccion de la conexion entrante*/

	int nAddrSize = sizeof(dirCrawler);

	/*Acepta la conexion entrante*/
	sockCrawler = accept(sockWebServer, (SOCKADDR *) &dirCrawler, &nAddrSize);
	
	/*Si el servidor No esta fuera de servicio puede atender Crawlers*/
	if (sockCrawler != INVALID_SOCKET) 
	{
		if (codop != OUTOFSERVICE)
		{
			int mode = 0x00;
			int comprobacion;
			char descID[DESCRIPTORID_LEN];

			/*Recibir el msg IRC del WebStore*/
			if (ircRequest_recv (sockCrawler, NULL, descID, &mode) < 0)
			{
				printf("Error al recibir IRC Crawler Create. Se cierra conexion.\r\n\r\n");
				closesocket(sockCrawler);
				return (-1);
			}			

			if (mode == IRC_CRAWLER_CONNECT)
			{
				printf ("A migrado un Web Crawler. Se comprobaran Condiciones de Creacion...\r\n");
				
				/*Se comprueban las condiciones para que haya una migracion*/
				if ((comprobacion = comprobarCondicionesMigracion(config.esperaCrawler)) == 1)
				/*Si se puede migrar, envia un Crawler Ok y crea thread*/
				{	
					HANDLE threadHandle;
					DWORD threadID;
					
					ircResponse_send(sockCrawler, descID, IRC_CRAWLER_HANDSHAKE_OK, 
									sizeof(IRC_CRAWLER_HANDSHAKE_OK), IRC_CRAWLER_OK);	
					printf("Migracion de Web Crawler satisfactoria.\r\n\r\n");
					closesocket(sockCrawler);

					/*Se crea el thread Crawler*/
					threadHandle = (HANDLE) _beginthreadex (NULL, 1, (void *) rutinaAtencionCrawler, (LPVOID) NULL, 0, &threadID);
					if (threadHandle == 0)
					{
						printf("No se pudo crear el thread Crawler\r\n\r\n");
					}
					else
					{					
						/*Se modifica el Thread Priority Level*/
						SetThreadPriority(threadHandle, THREAD_PRIORITY_HIGHEST);
					}

					return 0;
				}
				else
				/*Si no, envia un Crawler Fail*/
				{
					ircResponse_send(sockCrawler, descID,IRC_CRAWLER_HANDSHAKE_FAIL, 
									sizeof(IRC_CRAWLER_HANDSHAKE_FAIL), IRC_CRAWLER_FAIL);
					printf("Migracion de Web Crawler a sido rechazada.\r\n\r\n");
					closesocket(sockCrawler);
					return (-3);
				}
			}
		}
		closesocket(sockCrawler);
		return (-2);
	}
	else
	{
		printf("Error en Accept: %d", GetLastError());
		return (-1);
	}
	return 0;
}