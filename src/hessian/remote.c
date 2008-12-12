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
 * $Id: remote.c,v 1.1 2008/12/12 11:33:43 vtschopp Exp $
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hessian/hessian.h"

/**
 * Method prototypes
 */
OBJECT_CTOR(hessian_remote);
OBJECT_DTOR(hessian_remote);
OBJECT_SERIALIZE(hessian_remote);
OBJECT_DESERIALIZE(hessian_remote);

/**
 * Initializes and registers the hessian remote class.
 */
static const hessian_class_t _hessian_remote_descr = {
    HESSIAN_REMOTE,
    "hessian.Remote",
    sizeof(hessian_remote_t),
    'r', 0,
    hessian_remote_ctor,
    hessian_remote_dtor,
    hessian_remote_serialize,
    hessian_remote_deserialize
};
const void * hessian_remote_class = &_hessian_remote_descr;

/**
 * Hessian remote constructor.
 *
 * hessian_object_t * remote= hessian_create(HESSIAN_REMOTE, "type", "url");
 */
hessian_object_t * hessian_remote_ctor (hessian_object_t * object, va_list * ap) {
    hessian_remote_t * self= object;
    if (self == NULL) {
		fprintf(stderr,"ERROR:hessian_remote_ctor: NULL object pointer.\n");
    	return NULL;
    }
    const char * type = va_arg( *ap, const char *);
    if (type == NULL) {
		fprintf(stderr,"ERROR:hessian_remote_ctor: NULL type parameter 2.\n");
    	return NULL;
    }
    const char * url= va_arg( *ap, const char *);
    if (type == NULL) {
		fprintf(stderr,"ERROR:hessian_remote_ctor: NULL url parameter 3.\n");
    	return NULL;
    }
    size_t type_l= strlen(type);
    self->type= calloc(type_l + 1, sizeof(char));
    if (self->type == NULL) {
		fprintf(stderr,"ERROR:hessian_remote_ctor: can't allocate type (%d chars).\n", (int)type_l);
    	return NULL;
    }
    strncpy(self->type,type,type_l);
    //printf("XXX:remote_ctor: type: %s\n",self->type);
    size_t url_l= strlen(url);
    self->url= calloc(url_l + 1, sizeof(char));
    if (self->type == NULL) {
		fprintf(stderr,"ERROR:hessian_remote_ctor: can't allocate url (%d chars).\n", (int)url_l);
		free(self->type);
    	return NULL;
    }
    strncpy(self->url,url,url_l);
    //printf("XXX:remote_ctor: url: %s\n",self->url);
    return self;
}

int hessian_remote_dtor (hessian_object_t * object) {
    hessian_remote_t * self= object;
    if (self == NULL) {
		fprintf(stderr,"ERROR:hessian_remote_dtor: NULL object pointer.\n");
    	return HESSIAN_ERROR;
    }
    if (self->type != NULL) {
    	free(self->type);
    }
    self->type= NULL;
    if (self->url != NULL) {
    	free(self->url);
    }
    self->url= NULL;
    return HESSIAN_OK;
}

/**
 * hessian_remote serialize method.
 */
int hessian_remote_serialize (const hessian_object_t * object, BUFFER * output) {
    const hessian_remote_t * self= object;
    if (self == NULL) {
		fprintf(stderr,"ERROR:hessian_remote_serialize: NULL object pointer.\n");
    	return HESSIAN_ERROR;
    }
    const hessian_class_t * class= hessian_getclass(object);
    if (class == NULL) {
		fprintf(stderr,"ERROR:hessian_remote_serialize: NULL class descriptor.\n");
    	return HESSIAN_ERROR;
    }
    if (class->type != HESSIAN_REMOTE) {
		fprintf(stderr,"ERROR:hessian_remote_serialize: wrong class type: %d.\n",class->type);
    	return HESSIAN_ERROR;
    }
    buffer_putc(class->tag,output);
    // write type
    size_t str_l= strlen(self->type);
    size_t utf8_l= utf8_strlen(self->type);
    int b16= utf8_l >> 8;
    int b8= utf8_l & 0x00FF;
    buffer_putc('t',output);
    buffer_putc(b16,output);
    buffer_putc(b8,output);
    //printf("XXX:remote_serialize: type{%d}: %s\n",(int)str_l,self->type);
    buffer_write(self->type,1,str_l,output);
    // write url (utf8)
    str_l= strlen(self->url);
    utf8_l= utf8_strlen(self->url);
    b16= utf8_l >> 8;
    b8= utf8_l & 0x00FF;
    buffer_putc('S',output);
    buffer_putc(b16,output);
    buffer_putc(b8,output);
    //printf("XXX:remote_serialize: url{%d}: %s\n",(int)str_l,self->url);
    buffer_write(self->url,1,str_l,output);

    return HESSIAN_OK;
}

/**
 * hessian_remote deserialize method.
 */
int hessian_remote_deserialize (hessian_object_t * object, int tag, BUFFER * input) {
    hessian_remote_t * self= object;
    if (self == NULL) {
		fprintf(stderr,"ERROR:hessian_remote_deserialize: NULL object pointer.\n");
    	return HESSIAN_ERROR;
    }
    const hessian_class_t * class= hessian_getclass(object);
    if (class == NULL) {
		fprintf(stderr,"ERROR:hessian_remote_deserialize: NULL class descriptor.\n");
    	return HESSIAN_ERROR;
    }
    if (class->type != HESSIAN_REMOTE) {
		fprintf(stderr,"ERROR:hessian_remote_deserialize: wrong class type: %d.\n",class->type);
    	return HESSIAN_ERROR;
    }
    // tag is 'r'
    if (tag != class->tag) {
		fprintf(stderr,"ERROR:hessian_remote_deserialize: invalid tag: %c (%d).\n",(char)tag,tag);
    	return HESSIAN_ERROR;
    }
    // parse type 't' and url 'S'
    int type_tag= buffer_getc(input);
    if (type_tag != 't') {
		fprintf(stderr,"ERROR:hessian_remote_deserialize: invalid type tag: %c (%d).\n",(char)type_tag,type_tag);
    	return HESSIAN_ERROR;
    }
    // read the utf8 type length
	int b16= buffer_getc(input);
	int b8= buffer_getc(input);
	size_t utf8_l= (b16 << 8) + b8;
	char * type= utf8_bgets(utf8_l,input);
	//printf("XXX:hessian_remote_deserialize: type: %s\n", type);
	self->type= type;
    int url_tag= buffer_getc(input);
    if (url_tag != 'S') {
		fprintf(stderr,"ERROR:hessian_remote_deserialize: invalid url tag: %c (%d).\n",(char)url_tag,url_tag);
    	return HESSIAN_ERROR;
    }
    // read the utf8 url length
	b16= buffer_getc(input);
	b8= buffer_getc(input);
	utf8_l= (b16 << 8) + b8;
	char * url= utf8_bgets(utf8_l,input);
	//printf("XXX:hessian_remote_deserialize: url: %s\n", url);
	self->url= url;
    return HESSIAN_OK;
}

/**
 * Returns the Hessian remote type.
 */
const char * hessian_remote_gettype(const hessian_object_t * object) {
	const hessian_remote_t * self= object;
    if (self == NULL) {
		fprintf(stderr,"ERROR:hessian_remote_gettype: NULL object pointer.\n");
    	return NULL;
    }
    const hessian_class_t * class= hessian_getclass(object);
    if (class == NULL) {
		fprintf(stderr,"ERROR:hessian_remote_gettype: NULL class descriptor.\n");
    	return NULL;
    }
    if (class->type != HESSIAN_REMOTE) {
		fprintf(stderr,"ERROR:hessian_remote_gettype: wrong class type: %d.\n",class->type);
    	return NULL;
    }
	return self->type;
}

/**
 * Returns the Hessian remote url.
 */
const char * hessian_remote_geturl(const hessian_object_t * object) {
	const hessian_remote_t * self= object;
    if (self == NULL) {
		fprintf(stderr,"ERROR:hessian_remote_geturl: NULL object pointer.\n");
    	return NULL;
    }
    const hessian_class_t * class= hessian_getclass(object);
    if (class == NULL) {
		fprintf(stderr,"ERROR:hessian_remote_geturl: NULL class descriptor.\n");
    	return NULL;
    }
    if (class->type != HESSIAN_REMOTE) {
		fprintf(stderr,"ERROR:hessian_remote_geturl: wrong class type: %d.\n",class->type);
    	return NULL;
    }
	return self->url;
}
