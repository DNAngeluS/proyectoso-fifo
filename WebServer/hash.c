#include "hash.h"

unsigned hash(char *s)
{
	unsigned hashval;

	for (hashval = 0; *s != '\0'; s++)
		hashval = *s + 31 * hashval;
	return hashval % HASHSIZE;
}

struct nlist *lookup(struct nlist **hashtab, char *s)
{
	struct nlist *np;
	unsigned hasval;

	for (np = hashtab[hash(s)]; np != NULL; np = np->next)
		if (strcmp(s, np->file) == 0)
			return np;	/*Se encontro*/
	return NULL;	/*No se encontro*/
}

struct nlist *install(struct nlist **hashtab, char *file, char *md5)
{
	struct nlist *np;
	unsigned hashval;

	if ((np = lookup(file)) == NULL)
	/*No fue encontrado*/
	{
		np = (struct nlist *) malloc(sizeof(*np));
		if (np == NULL || (np->file = strdup(file)) == NULL)
			return NULL;
		hashval = hash(file);
		np->next = hashtab[hashval];
		hashtab[hashval] = np;
	}
	else /*Ya esta alli*/
		free ((void *) np->md5); /*Libera anterior md5*/
	if ((np->md5 = strdup(md5)) == NULL)
		return NULL;
	return np;
}

int clean(struct nlist **hashtab, char *file)
{
    struct nlist *np1, *np2;

    if ((np1 = lookup(file)) == NULL)	/*No encontro*/
        return 1;

    for ( np1 = np2 = hashtab[hash(file)]; np1 != NULL; np2 = np1, np1 = np1->next ) 
	{
        if ( strcmp(file, np1->file) == 0 ) 
		/*Encontro*/
		{

            /*Remover nodo de la lista*/
            if ( np1 == np2 )
                hashtab[hash(file)] = np1->next;
            else
                np2->next = np1->next;

            /*Liberar memoria*/
            free(np1->file);
            free(np1->md5);
            free(np1);

            return 0;
        }
    }

    return 1;  /*No encontro*/
}


}

char *ObtenerMD5(char *s)
{
	HCRYPTPROV hCryptProv = NULL;
	HCRYPTHASH hHash;
	DWORD cbHash = 0;
	BYTE rgbFile[BUFSIZE];
	char md5[MAX_PATH];
	CHAR rgbDigits[] = "0123456789abcdef";

	memset(md5, '\0', MAX_PATH);

	/*Adquirir contexto*/
	if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, 0)) 
	{
		fprintf(stderr, "Error during CryptAcquireContext.\r\n");
		return NULL;
	}
	
	/*Crear hash Handle*/
	if (!CryptCreateHash(hCryptProv, CALG_MD5, 0, 0, &hHash))
	{
		fprintf(stderr, "Error during CryptCreateHash.\r\n");
		CryptReleaseContext(hProv, 0);
		return NULL;
	}

	/*Se genera el MD5 para s*/
	if (CryptHashData(hHash, (BYTE *) s, strlen(s)+sizeof('\0'), 0))
	{
		fprintf(stderr, "Error during CryptHashData.\r\n");
		CryptReleaseContext(hProv, 0);
		CryptDestroyHash(hHash);
		return NULL;
	}

	cbHash = MD5LEN;
    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
    {
		DWORD i;
        
		sprintf_s(md5, cbHash, "%x", rgbHash);		
		/*for (i = 0; i < cbHash; i++)
        {
            printf("%c%c", rgbDigits[rgbHash[i] >> 4],
                rgbDigits[rgbHash[i] & 0xf]);
        }
        printf("\n");*/
    }
	else
		printf("CryptGetHashParam failed: %d\r\n", dwStatus);
	/*Elimina hash Handle*/
	if (CryptDestroyHash(hHash))
	{
	   	fprintf(stderr, "Error during CryptDestroyHash.\r\n");
		CryptReleaseContext(hProv, 0);
		return NULL;
	}

	/*Libera el contexto*/
	if (hCryptProv) 
		CryptReleaseContext(hCryptProv,0);

	return md5;
}



















