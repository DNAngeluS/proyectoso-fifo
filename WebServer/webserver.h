#ifndef WEBSERVER
#define WEBSERVER

#include <stdio.h>
#include <process.h>
#include <winsock2.h>
#include <windows.h>

#define WIN32_LEAN_AND_MEAN
#define MAXCONEXIONES SOMAXCONN
#define BUF_SIZE 4096
#define MAX_HEAP 1024*1024

enum codop_t {RUNNING, FINISH, OUTOFSERVICE}

typedef unsigned int in_addr_t;
typedef unsigned short in_port_t;

#endif