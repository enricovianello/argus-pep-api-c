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
 * $Id: attributeassignment.c,v 1.1 2008/12/12 11:34:27 vtschopp Exp $
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util/linkedlist.h"
#include "pep/model.h"

struct pep_attribute_assignment {
	char * id; // mandatory
	linkedlist_t * values; // string list
};

/**
 * Creates a PEP attribute assignment with the given id. id can be NULL, but not recommended.
 */
pep_attribute_assignment_t * pep_attribute_assignment_create(const char * id) {
	pep_attribute_assignment_t * attr= calloc(1,sizeof(pep_attribute_assignment_t));
	if (attr == NULL) {
		fprintf(stderr,"ERROR:pep_attribute_assignemnt_create: can't allocate pep_attribute_assignment_t.\n");
		return NULL;
	}
	attr->id= NULL;
	if (id != NULL) {
		size_t size= strlen(id);
		attr->id= calloc(size + 1,sizeof(char));
		if (attr->id == NULL) {
			fprintf(stderr,"ERROR:pep_attribute_assignemnt_create: can't allocate id (%d bytes).\n",(int)size);
			free(attr);
			return NULL;
		}
		strncpy(attr->id,id,size);
	}
	attr->values= llist_create();
	if (attr->values == NULL) {
		fprintf(stderr,"ERROR:pep_attribute_assignemnt_create: can't create values list.\n");
		free(attr->id);
		free(attr);
		return NULL;
	}
	return attr;
}

/**
 * Sets the PEP attribute id. id is mandatory and can't be NULL.
 */
int pep_attribute_assignment_setid(pep_attribute_assignment_t * attr, const char * id) {
	if (attr == NULL) {
		fprintf(stderr,"ERROR:pep_attribute_assignment_setid: NULL attribute.\n");
		return PEP_MODEL_ERROR;
	}
	if (id == NULL) {
		fprintf(stderr,"ERROR:pep_attribute_assignment_setid: NULL id.\n");
		return PEP_MODEL_ERROR;
	}
	if (attr->id != NULL) {
		free(attr->id);
	}
	size_t size= strlen(id);
	attr->id= calloc(size + 1,sizeof(char));
	if (attr->id == NULL) {
		fprintf(stderr,"ERROR:pep_attribute_assignment_setid: can't allocate id (%d bytes).\n", (int)size);
		return PEP_MODEL_ERROR;
	}
	strncpy(attr->id,id,size);
	return PEP_MODEL_OK;
}

/**
 * Returns the PEP attribute assignment attribute id.
 */
const char * pep_attribute_assignment_getid(const pep_attribute_assignment_t * attr) {
	if (attr == NULL) {
		fprintf(stderr,"ERROR:pep_attribute_assignment_getid: NULL attribute.\n");
		return NULL;
	}
	return attr->id;
}

int pep_attribute_assignment_addvalue(pep_attribute_assignment_t * attr, const char *value) {
	if (attr == NULL || value == NULL) {
		fprintf(stderr,"ERROR:pep_attribute_assignment_addvalue: NULL attribute or value.\n");
		return PEP_MODEL_ERROR;
	}
	// copy the const value
	size_t size= strlen(value);
	char * v= calloc(size + 1, sizeof(char));
	if (v == NULL) {
		fprintf(stderr,"ERROR:pep_attribute_assignment_addvalue: can't allocate value (%d bytes).\n", (int)size);
		return PEP_MODEL_ERROR;
	}
	strncpy(v,value,size);
	if (llist_add(attr->values,v) != LLIST_OK) {
		fprintf(stderr,"ERROR:pep_attribute_assignment_addvalue: can't add value to list.\n");
		return PEP_MODEL_ERROR;
	}
	else return PEP_MODEL_OK;
}
/**
 * Returns the count of PEP attribute assignemnt values.
 */
size_t pep_attribute_assignment_values_length(const pep_attribute_assignment_t * attr) {
	if (attr == NULL) {
		fprintf(stderr,"ERROR:pep_attribute_assignment_values_length: NULL attribute.\n");
		return PEP_MODEL_ERROR;
	}
	return llist_length(attr->values);
}

/**
 * Returns the PEP attribute assignment value at index i
 */
const char * pep_attribute_assignment_getvalue(const pep_attribute_assignment_t * attr,int i) {
	if (attr == NULL) {
		fprintf(stderr,"ERROR:pep_attribute_assignment_getvalue: NULL attribute.\n");
		return NULL;
	}
	return llist_get(attr->values,i);
}



/**
 * Deletes the PEP attribute.
 */
void pep_attribute_assignment_delete(pep_attribute_assignment_t * attr) {
	if (attr == NULL) return;
	free(attr->id);
	llist_delete_elements(attr->values,(delete_element_func)free);
	llist_delete(attr->values);
	free(attr);
	attr= NULL;
}