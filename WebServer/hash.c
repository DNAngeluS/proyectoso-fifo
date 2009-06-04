#include "hash.h"

extern struct nlist *hashtab[HASHSIZE];

unsigned hash(char *s)
{
	unsigned hashval;

	for (hashval = 0; *s != '\0'; s++)
		hashval = *s + 31 * hashval;
	return hashval % HASHSIZE;
}

struct nlist *lookup(/*struct nlist **hashtab, */char *s)
{
	struct nlist *np;

	for (np = hashtab[hash(s)]; np != NULL; np = np->next)
		if (strcmp(s, np->file) == 0)
			return np;	/*Se encontro*/
	return NULL;	/*No se encontro*/
}

struct nlist *install(/*struct nlist **hashtab, */char *file, char *md5)
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

int clean(/*struct nlist **hashtab, */char *file)
{
    struct nlist *np1, *np2;

    if ((np1 = lookup(/*hashtab, */file)) == NULL)	/*No encontro*/
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

int hashMD5(char *filename, char *md5sum)
{
    DWORD dwStatus = 0;
    BOOL bResult = FALSE;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = NULL;
    BYTE rgbFile[BUFSIZE];
    DWORD cbRead = 0;
    BYTE rgbHash[MD5LEN];
    DWORD cbHash = 0;
    CHAR rgbDigits[] = "0123456789abcdef";

	lstrcpy(md5sum,"");

    hFile = CreateFile(filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwStatus = GetLastError();
        printf("Error opening file %s\nError: %d\r\n", filename, 
			dwStatus); 
        return dwStatus;
    }

    if (!CryptAcquireContext(&hProv,
        NULL,
        NULL,
        PROV_RSA_FULL,
        CRYPT_VERIFYCONTEXT))
    {
        dwStatus = GetLastError();
        printf("CryptAcquireContext failed: %d\r\n", dwStatus); 
        CloseHandle(hFile);
        return dwStatus;
    }

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        dwStatus = GetLastError();
        printf("CryptAcquireContext failed: %d\r\n", dwStatus); 
        CloseHandle(hFile);
        CryptReleaseContext(hProv, 0);
        return dwStatus;
    }

    while (bResult = ReadFile(hFile, rgbFile, BUFSIZE, 
		&cbRead, NULL))
    {
        if (0 == cbRead)
        {
            break;
        }

        if (!CryptHashData(hHash, rgbFile, cbRead, 0))
        {
            dwStatus = GetLastError();
            printf("CryptHashData failed: %d\r\n", dwStatus); 
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            CloseHandle(hFile);
            return dwStatus;
        }
    }

    if (!bResult)
    {
        dwStatus = GetLastError();
        printf("ReadFile failed: %d\r\n", dwStatus); 
        CryptReleaseContext(hProv, 0);
        CryptDestroyHash(hHash);
        CloseHandle(hFile);
        return dwStatus;
    }

    cbHash = MD5LEN;
    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
    {
        DWORD i;
        
        for (i = 0; i < cbHash; i++)
        {
            char buf[5];
            sprintf_s(buf, sizeof(buf), "%c%c", rgbDigits[rgbHash[i] >> 4],
                rgbDigits[rgbHash[i] & 0xf]);
            strcat(md5sum, buf);
        }
    }
    else
    {
        dwStatus = GetLastError();
        printf("CryptGetHashParam failed: %d\r\n", dwStatus); 
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);

    return dwStatus; 
}

int hashSave(char *tmpFile)
{
	return 0;
}

int hastLoad()
{
	return 0;
}














