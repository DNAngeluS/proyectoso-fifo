#ifndef HTMLPARSER
#define HTMLPARSER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include "irc.h"
#include "config.h"

int parsearHtml						(const char *htmlFile, crawler_URL * estructuraRetorno);

int xmlOpen							(const char *filname, xmlDoc **doc, xmlNode **root);
int xmlAddUrl						(const char *filename, char *url);

int xmlReadNodo						(xmlNode *nodo, crawler_URL *paq);
int xmlReadAtt						(xmlAttr *nodo, crawler_URL *paq);

char* xmlGetContent					(xmlNode *nodo);
char* xmlGetAttContent				(xmlAttr *nodo);

int xmlGetKeywords					(const char *keywords, char **palabra, int *palabrasLen);
int xmlCopyCode						(const char *filename, char htmlCode[MAX_HTMLCODE]);

int xmlIdentificarWebServer			(const char *imgDir);
int xmlEnviarCrawler				(const char *htmlDir);

#endif	/* _HTMLPARSER_H */
