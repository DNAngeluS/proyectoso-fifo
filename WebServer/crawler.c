#include "crawler.h"

static int forAllFiles(char *directorio, int (*funcion) (WIN32_FIND_DATA));
static int generarPaqueteArchivos(const char *filename, DWORD size, crawler_URL *paquete);
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
    int archivosProcesados = 0, error = 0;
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
	if ((archivosProcesados = forAllFiles(config.directorioFiles, rutinaTrabajoCrawler)) < 0)
		printf("Crawler: Error al procesar archivos. Se descarta Crawler.\r\n\r\n");	
	
	if (archivosProcesados != hashman.ocupados)
	{
		int i;
		
		/*Se eliminan los archivos que no fueron procesados, ya queno existen mas en el directorio*/
		for (i = 0; i < HASHSIZE; i++)
			if (hashman.hashtab[i] != NULL && hashman.flag[i] == LLENO)
				if (hashClean(hashman.hashtab[i]->file) < 0)
					error = -1;
	}

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

	SetCurrentDirectory(directorio);

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
		if (ffd.dwFileAttributes == FILE_ATTRIBUTE_HIDDEN)
			continue;
		if (ffd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			char newDir[MAX_PATH];

			wsprintf(newDir, "%s\\%s", directorio, ffd.cFileName);
			if ((archivosProcesados += forAllFiles(newDir, funcion)) < 0)
				return -1;
			SetCurrentDirectory("..");
		}
		else
		{
			int control = 0;

			if ((control = funcion(ffd)) < 0)
				return -1;
			if (control != ELIMINA_NO_PUBLICO && control != ATIENDE_MODIFICADO)
				archivosProcesados++;
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES)
	{
		printf("Error.\r\n");
		printf("FindFirstFile: %d\r\n\\r\n", GetLastError());
		return -1;
	}

	FindClose(hFind);
	return archivosProcesados;
}

static int rutinaTrabajoCrawler(WIN32_FIND_DATA filedata)
{
	DWORD attr = filedata.dwFileAttributes;
	struct nlist *np = hashLookup(filedata.cFileName);
	char *filename = filedata.cFileName;
	char type[MAX_FORMATO];
	char md5[MAX_PATH];
	crawler_URL paquete;
	int mode = 0x00;
	int tipoProceso = ARCHIVO_NO_SUFRIO_CAMBIOS;
	int palabrasSize = 0;

	memset(&paquete, '\0', sizeof(paquete));
	memset(type, '\0', sizeof(type));
	memset(md5, '\0', sizeof(md5));

	hashMD5(filename, config.directorioFiles, md5);

	/*Si encontro y es solo lectura -> Eliminarlo*/
	if (np != NULL && attr == FILE_ATTRIBUTE_READONLY)
	{
		if (hashClean(filename) < 0)
			return -1;
		tipoProceso = ELIMINA_NO_PUBLICO;
	}

	/*
	 *Si no encontro y no es solo lectura -> Agregar
	 *Si encontro y no es solo lectura y cambio el md5 -> Modificar
	 */
	else if ((np == NULL && attr != FILE_ATTRIBUTE_READONLY) || 
			(np != NULL && attr != FILE_ATTRIBUTE_READONLY && strcmp(np->md5, md5) != 0))
	{
		int palabrasLen = 0;

		if (getFileType(filename, type) == HTML)
		{
			if (np == NULL && attr != FILE_ATTRIBUTE_READONLY)
			{
				mode = IRC_CRAWLER_ALTA_HTML;
				tipoProceso = ATIENDE_NUEVO;
			}
			else
			{
				mode = IRC_CRAWLER_MODIFICACION_HTML;
				tipoProceso = ATIENDE_MODIFICADO;
			}

			if (parsearHtml(filename, &paquete) < 0)
				return -1;
		}
		else
		{
			if (np == NULL && attr != FILE_ATTRIBUTE_READONLY)
			{
				mode = IRC_CRAWLER_ALTA_ARCHIVOS;
				tipoProceso = ATIENDE_NUEVO;
			}
			else
			{
				mode = IRC_CRAWLER_MODIFICACION_ARCHIVOS;
				tipoProceso = ATIENDE_MODIFICADO;
			}

			if (generarPaqueteArchivos(filename, filedata.nFileSizeLow, &paquete) < 0)
				return -1;
		}

		if (paquete.palabras == NULL)
			palabrasSize = 0;
		else
			palabrasSize = lstrlen(paquete.palabras)+1;

		/*Se envia el paquete al Web Store*/
		if (enviarPaquete(config.ipWebStore, config.puertoWebStore, &paquete, mode, palabrasSize) < 0)
			printf("Error en el envio de Paquete al Web Store para %s.\r\n\r\n", filename);
		
		if (hashInstall(filename, md5) < 0)
			return -1;
	}
	else
		if (np != NULL)
			hashman.flag[hash(np->file)] = RECIENTEMENTE_ACCEDIDO;

	_CrtDumpMemoryLeaks();

	return tipoProceso;
}


static int generarPaqueteArchivos(const char *filename, DWORD size, crawler_URL *paquete)
{   
    int ntype, control = 0;
	char *dirBegin = NULL;
	char dirLocal[MAX_PATH];
    char type[5];
	char filedir[MAX_PATH];	

    ntype = getFileType(filename, type);
	if (ntype < 0) return -1;

	lstrcpy(paquete->formato, type);
	if (ntype == JPG || ntype == GIF || ntype == PNG || ntype == JPEG)
		lstrcpy(paquete->tipo, "1");
	else
		lstrcpy(paquete->tipo, "2");

	GetCurrentDirectory(sizeof(dirLocal), dirLocal);
	if (strcmp(dirLocal, config.directorioFiles) != 0)
	{
		control = 1;	
		dirBegin = dirLocal;
		dirBegin += lstrlen(config.directorioFiles);
		dirBegin++;
		wsprintf(filedir, "%s/", dirBegin);
	}
	else
		control = 0;
	
	if (size < 0) return -1;
	wsprintf(paquete->length, "%ld", size);

	if (getKeywords(filename, &paquete->palabras) < 0)
		return -1;
	wsprintf(paquete->URL, "http://%s:%d/%s%s", inet_ntoa(*(IN_ADDR *)&config.ip), ntohs(config.puertoCrawler), 
				control? filedir: "",  filename);

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
int rutinaConexionCrawler(SOCKET sockWebServerCrawler)
{
	SOCKET sockCrawler;					/*Socket del cliente remoto*/
	SOCKADDR_IN dirCrawler;				/*Direccion de la conexion entrante*/

	int nAddrSize = sizeof(dirCrawler);

	/*Acepta la conexion entrante*/
	sockCrawler = accept(sockWebServerCrawler, (SOCKADDR *) &dirCrawler, &nAddrSize);
	
	/*Si el servidor No esta fuera de servicio puede atender Crawlers*/
	if (sockCrawler != INVALID_SOCKET) 
	{
		if (codop != OUTOFSERVICE)
		{
			int mode = 0x00;
			int comprobacion;
			char descID[DESCRIPTORID_LEN];

			/*Recibir el msg IRC del WebStore*/
			printf("Se recibira el pedido de creacion de Crawler. ");
			if (ircRequest_recv (sockCrawler, NULL, descID, &mode) < 0)
			{
				printf("Error.\r\n");
				closesocket(sockCrawler);
				return (-1);
			}
			else
				printf("Recibido OK.\r\n");

			if (mode == IRC_CRAWLER_CONNECT)
			{	
				/*Se comprueban las condiciones para que haya una migracion*/
				printf ("Se comprobaran condiciones de creacion. \r\n");

				if ((comprobacion = comprobarCondicionesMigracion(config.esperaCrawler)) == 1)
				/*Si se puede migrar, envia un Crawler Ok y crea thread*/
				{	
					HANDLE threadHandle;
					DWORD threadID;
					
					printf("Se puede crear. Comprobacion satisfactoria.\r\n");
					printf("Se enviara respuesta positiva. ");
					if (ircResponse_send(sockCrawler, descID, IRC_CRAWLER_HANDSHAKE_OK, 
									sizeof(IRC_CRAWLER_HANDSHAKE_OK), IRC_CRAWLER_OK) < 0)
						printf("Error.\r\n");
					else
						printf("Enviado OK.\r\n");
					closesocket(sockCrawler);

					printf("Migracion de Web Crawler satisfactoria.\r\n");

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
					printf("No se puede crear. Comprobacion a fallado.\r\n");

					printf("Se enviara respuesta negativa. ");
					if (ircResponse_send(sockCrawler, descID,IRC_CRAWLER_HANDSHAKE_FAIL, 
									sizeof(IRC_CRAWLER_HANDSHAKE_FAIL), IRC_CRAWLER_FAIL) < 0)
						printf("Error.\r\n");
					else
						printf("Enviado OK.\r\n");
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