/*
 * Copyright 2008 Members of the EGEE Collaboration.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * $Id: log.c,v 1.1 2009/03/13 12:36:42 vtschopp Exp $
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/*
 * local logging handler
 */
static void _vfprintf(FILE * fd, const char * level, const char * format, va_list args) {
	int BUFFER_SIZE= 1024;
	char BUFFER[BUFFER_SIZE];
	memset(BUFFER,0,BUFFER_SIZE);
	size_t size= BUFFER_SIZE;
	strncat(BUFFER,level,size);
	size= size - strlen(BUFFER);
	strncat(BUFFER,": ",size);
	size= size - strlen(BUFFER);
	strncat(BUFFER,format,size);
	size= size - strlen(BUFFER);
	strncat(BUFFER,"\n",size);
	vfprintf(fd,BUFFER,args);
}

/*
 * Logs an INFO message on stdout
 */
void info(const char * format, ...) {
	va_list args;
	va_start(args,format);
	_vfprintf(stdout,"INFO",format,args);
	va_end(args);
}

/*
 * Logs an ERROR message on stderr
 */
void error(const char * format, ...) {
	va_list args;
	va_start(args,format);
	_vfprintf(stderr,"ERROR",format,args);
	va_end(args);
}

/*
 * Logs an DEBUG message on stdout
 */
void debug(const char * format, ...) {
	va_list args;
	va_start(args,format);
	_vfprintf(stdout,"DEBUG",format,args);
	va_end(args);
}

/*
 * PEP-C logging callback function
 */
static void log_handler_pep(int level, const char * format, va_list args) {
	// TODO if needed
}
