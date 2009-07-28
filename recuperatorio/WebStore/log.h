/*
 * File:   log.h
 * Author: mariano
 *
 * Created on 4 de julio de 2009, 14:05
 */

#ifndef _LOG_H
#define	_LOG_H

#include "webstore.h"

int WriteLog(int log, char *proceso, pid_t pid, pthread_t thread, char *descripcion, char *tipo);

#endif	/* _LOG_H */

