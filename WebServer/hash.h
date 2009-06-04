#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASHSIZE 101
#define MD5LEN 16

struct nlist {
	struct nlist *next;
	char *file;
	char *md5;
};

unsigned hash(char *s);
struct nlist *lookup(struct nlist **hashtab, char *s);
struct nlist *install(struct nlist **hashtab, char *file, char *md5);
struct nlist *clean(struct nlist **hashtab, char *file);
char *ObtenerMD5(char *s);
