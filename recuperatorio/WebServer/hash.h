#ifndef HASH
#define HASH

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <Wincrypt.h>


#define HASHSIZE 101
#define BUF_SIZE 1024
#define MD5LEN 16

enum estadoHash_t {VACIO, LLENO, RECIENTEMENTE_ACCEDIDO};

struct nlist {
	struct nlist *next;
	char *file;
	char *md5;
};

struct hashManager {
	int ocupados;
	int flag[HASHSIZE];
	struct nlist *hashtab[HASHSIZE];
};

unsigned hash					(char *s);
struct nlist *hashLookup		(char *s);
struct nlist *hashInstall		(char *file, char *md5);
int hashCleanAll				();
int hashClean					(char *file);
int hashMD5						(char *filename, char *dir, char *md5sum);
int hashSave					();
int hashLoad					();
BOOL hashVacia					();

#endif