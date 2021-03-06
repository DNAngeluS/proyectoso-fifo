#include "htmlParser.h"

extern configuracion config;
extern int EnviarCrawler(in_addr_t nDireccion, in_port_t nPort);
extern SOCKET establecerConexionServidor(in_addr_t nDireccionIP, in_port_t nPort, SOCKADDR_IN *their_addr);

/*
Descripcion: Completa la estructura crawler_URL con los datos tomados de un archivo html,
			 ademas hace pedidos de instanciacion de Crawlers a otros Web Servers.
Ultima modificacion: Scheinkman, Mariano.
Recibe: nombre de arhivo html, estructura crawler_URL
Devuelve: ok? 0: -1. crawler_URL completo.
*/
int parsearHtml(const char *htmlFile, crawler_URL *paquete) 
{
	xmlDoc *xmlTree = NULL;
	xmlNode *rootElement = NULL;
	char dirFile[MAX_PATH];

	memset(dirFile, '\0', sizeof(dirFile));
	memset(paquete, '\0', sizeof(*paquete));

	wsprintf(dirFile, "%s\\%s", config.directorioFiles, htmlFile);

	if (xmlOpen(dirFile, &xmlTree, &rootElement) < 0)
		return -1;

	xmlAddUrl(htmlFile, paquete->URL);

	xmlReadNodo(rootElement, paquete);

	if (xmlCopyCode(dirFile, paquete->htmlCode) < 0)
		return -1;

	xmlFreeDoc(xmlTree);

	printf("Titulo: %s\r\n", paquete->titulo);
	printf("Descripcion: %s\r\n", paquete->descripcion);
	printf("URL: %s\r\n", paquete->URL);
	printf("Palabras: %s\r\n", paquete->palabras);
	printf("\r\n");
	
	return 0;
}

/*
Descripcion: Completa el campo URL de la estructura crawler_ULR
Ultima modificacion: Moya Farina, Lucio
Recibe: nombre de arhivo, campo URL de crawler_URL
Devuelve: ok? 0: -1. campo URL completo.
*/
int xmlAddUrl(const char *filename, char *url)
{
	wsprintf(url, "%s%s:%d/%s", "http://", inet_ntoa(*(IN_ADDR *)&config.ip), ntohs(config.puerto), filename );

	return 0;
}
/*
Descripcion: Abre un archivo XML y crea el arbol desde la raiz del mismo
Ultima modificacion: Moya Farina, Lucio
Recibe: nombre de archivo, xmlNode estrutura para el arbol del xml
Devuelve: ok? 0: -1. completa la estructura xmlNode
*/
int xmlOpen(const char *filename, xmlDoc **doc, xmlNode **root)
{
	/*Apertura del archivo XML*/
	*doc = xmlReadFile(filename, NULL, 0);
	if (*doc == NULL) 
	{
		printf("xmlOpen: Fallo al leer el archivo %s\r\n", filename);
		return -1;
    }

	/*Creacion del arbol desde la raiz*/
	*root = xmlDocGetRootElement(*doc);
	if (*root == NULL) 
	{
		printf("xmlOpen: Fallo al obtener el nodo Raiz de %s\r\n", filename);
		return -1;
    }	
     

	return 0;
}
/*
Descripcion: Recorre todos los elementos del arbol del xHTML
Ultima modificacion: Moya Farina, Lucio
Recibe: nodo/raiz de arbol XML, Estructura para cargar los datos
Devuelve: ok? 0: -1. completa la estructura crawler_URL
*/
int xmlReadNodo(xmlNode *nodo, crawler_URL *paq)
{
	xmlNode *curNode = NULL;

	for (curNode = nodo; curNode; curNode = curNode->next) 
	{
        if (curNode->type == XML_ELEMENT_NODE)
		{
			if(!(strcmp(curNode->name, "title")))
			{
				lstrcpy(paq->titulo, xmlGetContent(curNode) );
			}
			if( (!(strcmp(curNode->name, "META"))) ||
				(!(strcmp(curNode->name, "a"))) ||
				(!(strcmp(curNode->name, "img"))) )
				xmlReadAtt(curNode->properties, paq);

		}		
			
        xmlReadNodo(curNode->children, paq);
    }

	return 0;
}
/*
Descripcion: Recorre todos los atributos de un elemento del xHTML
Ultima modificacion: Moya Farina, Lucio
Recibe: nodo de atributos XML, Estructura para cargar los datos
Devuelve: ok? 0: -1. completa la estructura crawler_URL
*/
int xmlReadAtt(xmlAttr *nodo, crawler_URL *paq)
{
	xmlAttr *curNode = NULL;
	char *campo = NULL;
	int keywords = 0;

	for (curNode = nodo; curNode; curNode = curNode->next)
	{
		if (curNode->type == XML_ATTRIBUTE_NODE)
		{
			if (!(strcmp(curNode->children->content, "keywords")))
			{
				char palabras[MAX_PATH];
				memset(palabras, '\0', MAX_PATH);
				campo = palabras;
				keywords = 1;
			}
			if (!(strcmp(curNode->children->content, "description")))
				campo = paq->descripcion;
			if (!(strcmp(curNode->name, "content")))
				lstrcpy(campo, xmlGetAttContent(curNode) );	
			if (!(strcmp(curNode->name, "href")))
			{ 
				char *htmlDir = xmlGetAttContent(curNode); /*contiene "http://xxxx.xxxx.xxxx.xxxx:xxxxx/unaPAG.html"*/
				
				printf("Servidor Web detectado. ");
				if (xmlEnviarCrawler(htmlDir) < 0)
					printf("Crawler no se a podido instanciar.\r\n\r\n");
				else
					printf("Crawler se a instanciado con exito.\r\n\r\n");			
			}
			if (!(strcmp(curNode->name, "src")))
			{
				char *imgDir = xmlGetAttContent(curNode); /*contiene "http://xxxx.xxxx.xxxx.xxxx:xxxx/unaIMG.jpg" o "img/unaIMG.jpg"*/ 
				
				if (xmlIdentificarWebServer(imgDir) == 1)
				/*Si es un servidor externo*/
				{
					printf("Servidor Web detectado. ");
					if (xmlEnviarCrawler(imgDir) < 0)
						printf("Crawler no se a podido instanciar.\r\n\r\n");
					else
						printf("Crawler se a instanciado con exito.\r\n\r\n");
				}
				/*else ignora*/
			}
		}	
    }
	if (keywords)
	{	
		DWORD palabrasLen;

		xmlGetKeywords(campo, &paq->palabras, &palabrasLen);
	}

	return 0;
}
/*
Descripcion: Obtiene el contenido del nodo Atributo y los devuelve en un char*
Ultima modificacion: Moya Farina, Lucio
Recibe: Nodo de XML Element
Devuelve: char* con el contenido
*/
char* xmlGetContent(xmlNode *nodo)
{
	xmlNode *texto = nodo->children;

	return texto->content;
}
/*
Descripcion: Obtiene el contenido del nodo Atributo y los devuelve en un char*
Ultima modificacion: Moya Farina, Lucio
Recibe: Nodo de XML Atribute
Devuelve: char* con el contenido
*/
char* xmlGetAttContent(xmlAttr *nodo)
{
	xmlNode *texto = nodo->children;

	return texto->content;
}
/*
Descripcion: Arma el array de palabras desde una palabra con las keywords separadas por coma
Ultima modificacion: Scheinkman, Mariano.
Recibe: palabras separada por com, array de palabras, puntero qe espera la cantidad de palabras.
Devuelve: ok? 0: -1. completo el array de palabras y la cantidad de palabras en el array.
*/
int xmlGetKeywords (const char *keywords, char **palabra, int *palabrasLen)
{
	char *vp = NULL;

	if ( (vp = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lstrlen(keywords) + 2)) == NULL)
		return -1;

	lstrcpy(vp, keywords);
	lstrcat(vp,	",");

	*palabrasLen = lstrlen(vp);
	*palabra = vp;

	return 0;

}
/*
Descripcion: Copia el contenido del html completo en la estructura.
Ultima modificacion: Scheinkman, Mariano
Recibe: nombre de archivo, estructura que almacenara el codigo.
Devuelve: ok? 0: -1. completa la estructura htmlCode
*/
int xmlCopyCode(const char *filename, char htmlCode[MAX_HTMLCODE])
{
	int buf_size, temp;
	HANDLE archivoHtml;

	archivoHtml = CreateFileA(filename, GENERIC_READ, 0, NULL,
						  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (archivoHtml == INVALID_HANDLE_VALUE) 
	{
		 printf("No se pudo abrir el archivo html: "
								 "(error %d)\r\n", GetLastError());
		 return -1;
	}
	temp = ReadFile(archivoHtml, htmlCode, MAX_HTMLCODE, &buf_size, NULL);
	CloseHandle(archivoHtml);

	if (temp == FALSE) 
	{
		 printf("No se pudo leer el archivo html: "
								 "(error %d)\r\n", GetLastError());
		 return -1;
	}

	return 0;
}


/*
Descripcion: Copia el contenido del html completo en la estructura.
Ultima modificacion: Scheinkman, Mariano.
Recibe: nombre de archivo, Estrutura para guardar los datos
Devuelve: ok? 0: -1. completa la estructura htmlCode
*/
int xmlEnviarCrawler(const char *htmlDir)
{
	char *dir, *nextT;
	in_addr_t ip;
	in_port_t puerto;
	char *p = NULL;
    
	dir = strstr(htmlDir, "//");
	dir+=2;
    
	ip = inet_addr(strtok_s(dir, ":", &nextT));

	if ((p = strtok_s(NULL, "/", &nextT)) == NULL)
	{
		puerto = htons(80);
	}
	else
		puerto = htons(atoi(p));

	if (EnviarCrawler(ip, config.puertoCrawler) < 0)
		return -1;
	else 
		return 0;
}


/*
Descripcion: Identifica si el URL esta en el servidor propio o ajeno.
Ultima modificacion: Scheinkman, Mariano.
Recibe: url de la forma "http://xxxx.xxxx.xxxx.xxxx:xxxxx/carpeta/unFILE.ext"
Devuelve: 1 si el URL es externo. 0 si es propio o si es del formato "/carpeta/unFILE.ext"
*/
int xmlIdentificarWebServer(const char *imgDir)
{
	char *dir, *nextT;
	in_addr_t ip;
	in_port_t puerto;
    char *p = NULL;
    
	dir = strstr(imgDir, "//");
	if (dir == NULL) return 0;
	dir+=2;
    
	ip = inet_addr(strtok_s(dir, ":", &nextT));

	if ((p = strtok_s(NULL, "/", &nextT)) == NULL)
	{
		puerto = htons(80);
	}
	else
		puerto = htons(atoi(p));

	return !(ip == config.ip && (puerto == config.puerto || puerto == config.puertoCrawler));
		
}