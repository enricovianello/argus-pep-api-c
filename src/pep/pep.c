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
 * $Id: pep.c,v 1.13 2009/03/17 11:37:31 vtschopp Exp $
 */
#include <stdarg.h>  /* va_list, va_arg, ... */
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>

#include "util/linkedlist.h"
#include "util/buffer.h"
#include "util/base64.h"
#include "util/log.h"

#include "pep/pep.h"
#include "pep/io.h"

#ifdef HAVE_CONFIG_H
#include "config.h"  /* PACKAGE_NAME and PACKAGE_VERSION const */
#else
#define PACKAGE_NAME "PEP-C"
#define PACKAGE_VERSION "1.0.0"
#endif

/** List of PIPs */
static linkedlist_t * pips;

/** List of ObligationHandlers */
static linkedlist_t * ohs;

/** CURL handler */
static CURL * curl= NULL;

/** Options */
static int option_loglevel= PEP_LOGLEVEL_NONE;
static FILE * option_logout= NULL;
static linkedlist_t * option_urls= NULL;
static long option_timeout= 10L;

static int option_pips_enabled= TRUE;
static int option_ohs_enabled= TRUE;

pep_error_t pep_initialize(void) {
	// clear all err message
	pep_clearerr();

	// create all required lists
	pips= llist_create();
	if (pips == NULL) {
		log_error("pep_initialize: PIPs list allocation failed.");
		pep_errmsg("failed to allocate PIPs list");
		return PEP_ERR_INIT_LISTS;
	}
	ohs= llist_create();
	if (ohs == NULL) {
		llist_delete(pips);
		log_error("pep_initialize: OHs list allocation failed.");
		pep_errmsg("failed to allocate OHs list");
		return PEP_ERR_INIT_LISTS;
	}
	// init curl and create curl handler
	CURLcode curl_rc= curl_global_init(CURL_GLOBAL_ALL);
	if (curl_rc != CURLE_OK) {
		log_error("pep_initialize: CURL global initialization failed: %s", curl_easy_strerror(curl_rc));
		llist_delete(pips);
		llist_delete(ohs);
		pep_errmsg("curl_global_init(CURL_GLOBAL_ALL) failed: %s",curl_easy_strerror(curl_rc));
		return PEP_ERR_INIT_CURL;
	}
	curl= curl_easy_init();
	if (curl == NULL) {
		log_error("pep_initialize: can't create CURL session handler.");
		llist_delete(pips);
		llist_delete(ohs);
		pep_errmsg("curl_easy_init() failed");
		return PEP_ERR_INIT_CURL;
	}
	// create option_urls list
	option_urls= llist_create();
	if (option_urls == NULL) {
		log_error("pep_initialize: can't create endpoint URLs list.");
		llist_delete(pips);
		llist_delete(ohs);
		pep_errmsg("failed to create URLs list");
		return PEP_ERR_INIT_LISTS;
	}
	return PEP_OK;
}

pep_error_t pep_addpip(pep_pip_t * pip) {
	if (pip == NULL) {
		log_error("pep_addpip: NULL pip pointer");
		pep_errmsg("NULL pep_pip_t pointer");
		return PEP_ERR_NULL_POINTER;
	}
	int pip_rc = -1;
	if ((pip_rc= pip->init()) != 0) {
		log_error("pep_addpip: PIP[%s] init() failed: %d.",pip->id, pip_rc);
		pep_errmsg("PIP[%s] init() failed with code: %d",pip->id, pip_rc);
		return PEP_ERR_INIT_PIP;
	}
	if (llist_add(pips,pip) != LLIST_OK) {
		log_error("pep_addpip: failed to add initialized PIP[%s] in list.",pip->id);
		pep_errmsg("can't add PIP[%s] into list",pip->id);
		return PEP_ERR_INIT_LISTS;
	}
	return PEP_OK;
}

pep_error_t pep_addobligationhandler(pep_obligationhandler_t * oh) {
	if (oh == NULL) {
		log_error("pep_addobligationhandler: NULL oh pointer");
		pep_errmsg("NULL pep_obligationhandler_t pointer");
		return PEP_ERR_NULL_POINTER;
	}
	int oh_rc= -1;
	if ((oh_rc= oh->init()) != 0) {
		log_error("pep_addobligationhandler: OH[%s] init() failed: %d",oh->id, oh_rc);
		pep_errmsg("OH[%s] init() failed with code: %d",oh->id, oh_rc);
		return PEP_ERR_INIT_OH;
	}
	if (llist_add(ohs,oh) != LLIST_OK) {
		log_error("pep_addobligationhandler: failed to add initialized OH[%s] in list.", oh->id);
		pep_errmsg("can't add OH[%s] into list",oh->id);
		return PEP_ERR_INIT_LISTS;
	}
	return PEP_OK;
}

// FIXME: implement all options
pep_error_t pep_setoption(pep_option_t option, ... ) {
	pep_error_t rc= PEP_OK;
	va_list args;
	va_start(args,option);
	char * str= NULL;
	int value= -1;
	FILE * file= NULL;
	pep_log_handler_callback * log_handler= NULL;
	switch (option) {
		case PEP_OPTION_ENDPOINT_URL:
			str= va_arg(args,char *);
			if (str == NULL) {
				log_error("pep_setoption: PEP_OPTION_ENDPOINT_URL argument is NULL.");
				rc= PEP_ERR_OPTION_INVALID;
				break;
			}
			// copy url
			size_t size= strlen(str);
			char * url= calloc(size + 1, sizeof(char));
			if (url == NULL) {
				log_error("pep_setoption: can't allocate url: %s.", str);
				pep_errmsg("can't allocate url: %s.", str);
				rc= PEP_ERR_MEMORY;
				break;
			}
			strncpy(url,str,size);
			// and add it to the urls list
			int rc= llist_add(option_urls,url);
			if (rc == LLIST_ERROR) {
				free(url);
				log_error("pep_setoption: can't add url: %s to list.", str);
				pep_errmsg("can't add url: %s to list.", str);
				rc= PEP_ERR_MEMORY;
				break;
			}
			log_debug("pep_setoption: PEP_OPTION_ENDPOINT_URL: %s",url);
			break;
		case PEP_OPTION_ENABLE_PIPS:
			value= va_arg(args,int);
			if (value == 1) {
				option_pips_enabled= TRUE;
			}
			else {
				option_pips_enabled= FALSE;
			}
			log_debug("pep_setoption: PEP_OPTION_ENABLE_PIPS: %s",(option_pips_enabled == TRUE) ? "TRUE" : "FALSE");
			break;
		case PEP_OPTION_ENABLE_OBLIGATIONHANDLERS:
			value= va_arg(args,int);
			if (value == 1) {
				option_ohs_enabled= TRUE;
			}
			else {
				option_ohs_enabled= FALSE;
			}
			log_debug("pep_setoption: PEP_OPTION_ENABLE_OBLIGATIONHANDLERS: %s",(option_ohs_enabled == TRUE) ? "TRUE" : "FALSE");
			break;
		case PEP_OPTION_LOG_LEVEL:
			value= va_arg(args,int);
			if (PEP_LOGLEVEL_NONE <= value && value <= PEP_LOGLEVEL_DEBUG) {
				option_loglevel= value;
				log_setlevel(option_loglevel);
			}
			log_debug("pep_setoption: PEP_OPTION_LOG_LEVEL: %d",option_loglevel);
			break;
		case PEP_OPTION_LOG_STDERR:
			file= va_arg(args,FILE *);
			option_logout= file;
			log_setout(file);
			log_debug("pep_setoption: PEP_OPTION_LOG_STDERR: 0x%04X",(int)option_logout);
			break;
		case PEP_OPTION_LOG_HANDLER:
			log_handler= va_arg(args,pep_log_handler_callback *);
			log_sethandler(log_handler);
			log_debug("pep_setoption: PEP_OPTION_LOG_HANDLER: 0x%04X",(int)log_handler);
			break;
		default:
			//XXX
			printf("XXX:pep_setoption: %d option NOT YET IMPLEMENTED.", option);
			log_error("pep_setoption: %d invalid option.", option);
			pep_errmsg("option: %d", option);
			rc= PEP_ERR_OPTION_INVALID;
			break;
	}
	va_end(args);
	return rc;
}

pep_error_t pep_authorize(xacml_request_t ** inout_request, xacml_response_t ** out_response) {
	if (*inout_request == NULL) {
		log_error("pep_authorize: NULL request pointer");
		pep_errmsg("NULL xacml_request_t pointer");
		return PEP_ERR_NULL_POINTER;
	}
	xacml_request_t * request= *inout_request;
	int i= 0;
	// apply pips if enabled and any
	int pip_rc= -1;
	if (option_pips_enabled && llist_length(pips) > 0) {
		size_t pips_l= llist_length(pips);
		log_info("pep_authorize: %d PIPs available, processing...", (int)pips_l);
		for (i= 0; i<pips_l; i++) {
			pep_pip_t * pip= llist_get(pips,i);
			if (pip != NULL) {
				log_debug("pep_authorize: calling pip[%s]->process(request)...",pip->id);
				pip_rc= pip->process(&request);
				if (pip_rc != 0) {
					log_error("pep_authorize: PIP[%s] process(request) failed: %d", pip->id, pip_rc);
					pep_errmsg("PIP[%s] process(request) failed: %d", pip->id, pip_rc);
					return PEP_ERR_AUTHZ_PIP_PROCESS;
				}
			}
		}
	}

	// marshal the PEP request in output buffer
	BUFFER * output= buffer_create(512);
	if (output == NULL) {
		log_error("pep_authorize: can't create output buffer.");
		pep_errmsg("can't allocate output buffer (512 bytes)");
		return PEP_ERR_MEMORY;
	}
	pep_error_t marshal_rc= xacml_request_marshalling(request,output);
	if ( marshal_rc != PEP_OK ) {
		log_error("pep_authorize: can't marshal PEP request: %s.", pep_strerror(marshal_rc));
		buffer_delete(output);
		// errmsg already set by pep_request_marshalling(...)
		return marshal_rc;
	}

	// base64 encode the output buffer
	size_t output_l= buffer_length(output);
	BUFFER * b64output= buffer_create( output_l );
	if (b64output == NULL) {
		log_error("ERROR:pep_authorize: can't create base64 output buffer.");
		buffer_delete(output);
		pep_errmsg("can't allocate base64 output buffer (%d)", (int)output_l);
		return PEP_ERR_MEMORY;
	}
	base64_encode_l(output,b64output,BASE64_DEFAULT_LINE_SIZE);

	// output buffer not needed anymore.
	buffer_delete(output);

	// set the CURL related options
	//XXX: options debug and timeout
	if (option_loglevel >= PEP_LOGLEVEL_DEBUG) {
		log_debug("pep_authorize: setting curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L).");
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		FILE * log_stderr= log_getout();
		if (log_stderr != NULL) {
			log_debug("pep_authorize: setting curl_easy_setopt(curl,CURLOPT_STDERR,log_stderr).");
			curl_easy_setopt(curl, CURLOPT_STDERR,log_stderr);
		}
	}
	// set the UserAgent string
	CURLcode curl_rc= curl_easy_setopt(curl, CURLOPT_USERAGENT, PACKAGE_NAME "/" PACKAGE_VERSION);

    // FIXME: check return code
	 curl_rc= curl_easy_setopt(curl, CURLOPT_TIMEOUT, option_timeout);


	// configure curl handler to POST the base64 encoded marshalled PEP request buffer
	curl_rc= curl_easy_setopt(curl, CURLOPT_POST, 1L);
	if (curl_rc != CURLE_OK) {
		log_error("pep_authorize: curl_easy_setopt(curl,CURLOPT_POST,1) failed: %s.",curl_easy_strerror(curl_rc));
		buffer_delete(b64output);
		pep_errmsg("curl_easy_setopt(curl,CURLOPT_POST,1) failed: %s.",curl_easy_strerror(curl_rc));
		return PEP_ERR_AUTHZ_CURL;
	}

	long b64output_l= (long)buffer_length(b64output);
	curl_rc= curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, b64output_l);
	if (curl_rc != CURLE_OK) {
		log_error("pep_authorize: curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,%d) failed: %s.",(int)b64output_l,curl_easy_strerror(curl_rc));
		buffer_delete(b64output);
		pep_errmsg("curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,%d) failed: %s.",(int)b64output_l,curl_easy_strerror(curl_rc));
		return PEP_ERR_AUTHZ_CURL;
	}

	curl_rc= curl_easy_setopt(curl, CURLOPT_READDATA, b64output);
	if (curl_rc != CURLE_OK) {
		log_error("pep_authorize: curl_easy_setopt(curl,CURLOPT_READDATA,b64output) failed: %s.",curl_easy_strerror(curl_rc));
		buffer_delete(b64output);
		pep_errmsg("curl_easy_setopt(curl,CURLOPT_READDATA,b64output) failed: %s.",curl_easy_strerror(curl_rc));
		return PEP_ERR_AUTHZ_CURL;
	}

	curl_rc= curl_easy_setopt(curl, CURLOPT_READFUNCTION, buffer_read);
	if (curl_rc != CURLE_OK) {
		log_error("pep_authorize: curl_easy_setopt(curl,CURLOPT_READFUNCTION,buffer_read) failed: %s.",curl_easy_strerror(curl_rc));
		buffer_delete(b64output);
		pep_errmsg("curl_easy_setopt(curl,CURLOPT_READFUNCTION,buffer_read) failed: %s.",curl_easy_strerror(curl_rc));
		return PEP_ERR_AUTHZ_CURL;
	}


	// configure curl handler to read the base64 encoded HTTP response
	// TODO: optimize size
	BUFFER * b64input= buffer_create(1024);
	if (b64input == NULL) {
		log_error("pep_authorize: can't create base64 input buffer.");
		buffer_delete(b64output);
		pep_errmsg("can't create base64 input buffer");
		return PEP_ERR_MEMORY;
	}

    curl_rc= curl_easy_setopt(curl, CURLOPT_WRITEDATA, b64input);
	if (curl_rc != CURLE_OK) {
		log_error("pep_authorize: curl_easy_setopt(curl,CURLOPT_WRITEDATA,b64input) failed: %s.",curl_easy_strerror(curl_rc));
		buffer_delete(b64output);
		buffer_delete(b64input);
		pep_errmsg("curl_easy_setopt(curl,CURLOPT_WRITEDATA,b64input) failed: %s.",curl_easy_strerror(curl_rc));
		return PEP_ERR_AUTHZ_CURL;
	}
    curl_rc= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, buffer_write);
	if (curl_rc != CURLE_OK) {
		log_error("pep_authorize: curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,buffer_write) failed: %s.",curl_easy_strerror(curl_rc));
		buffer_delete(b64output);
		buffer_delete(b64input);
		pep_errmsg("curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,buffer_write) failed: %s.",curl_easy_strerror(curl_rc));
		return PEP_ERR_AUTHZ_CURL;
	}

	// create the Hessian input buffer
	// TODO: optimize size
	BUFFER * input= buffer_create(1024);
	if (output == NULL) {
		log_error("pep_authorize: can't create input buffer.");
		buffer_delete(b64output);
		buffer_delete(b64input);
		pep_errmsg("can't create input buffer");
		return PEP_ERR_MEMORY;
	}

	/*
	 * FAILOVER BEGIN
	 */
	xacml_response_t * response= NULL;
	int failover_ok= FALSE;
	// set the PEPd endpoint url
	const char * url= NULL;
	size_t url_l= llist_length(option_urls);
	log_info("pep_authorize: %d PEPd failover URLs available",(int)url_l);

	for (i= 0; i<url_l; i++) {

		response= NULL;

		url= (const char *)llist_get(option_urls,i);
		log_debug("pep_authorize: trying PEPd[%s]...",url);
		curl_rc= curl_easy_setopt(curl, CURLOPT_URL, url);
		if (curl_rc != CURLE_OK) {
			log_warn("pep_authorize: PEPd[%s]: curl_easy_setopt(curl,CURLOPT_URL,%s) failed: %s.",url,curl_easy_strerror(curl_rc));
			// try next PEPd failover url
			log_debug("pep_authorize: PEPd[%s] failed: trying next failover URL...",url);
			continue;
		}

		// send the request
		log_info("pep_authorize: send request to PEPd[%s]",url);
		curl_rc= curl_easy_perform(curl);
		if (curl_rc != CURLE_OK) {
			log_warn("pep_authorize: PEPd[%s]: curl_easy_perform() failed:[%d] %s.",url,(int)curl_rc,curl_easy_strerror(curl_rc));
			buffer_rewind(b64output);
			buffer_reset(b64input);
			buffer_reset(input);
			log_debug("pep_authorize: PEPd[%s] failed: reset buffers, trying next failover URL...",url);
			continue;
		}

		// check for HTTP 200 response code
		long http_code= 0;
		curl_rc= curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&http_code);
		if (curl_rc != CURLE_OK) {
			log_warn("pep_authorize: PEPd[%s]: curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&http_code) failed: %s.",url,curl_easy_strerror(curl_rc));
		}
		if (http_code != 200) {
			log_warn("pep_authorize: PEPd[%s]: HTTP status code: %d.",url,(int)http_code);
			buffer_rewind(b64output);
			buffer_reset(b64input);
			buffer_reset(input);
			log_debug("pep_authorize: PEPd[%s]: reset buffers, trying next failover URL...",url);
			continue;
		}

		log_debug("pep_authorize: PEPd[%s]: HTTP status code: %d.",url,(int)http_code);

		// base64 decode the input buffer into the Hessian buffer.
		base64_decode(b64input,input);

		// unmarshal the PEP response
		pep_error_t unmarshal_rc= xacml_response_unmarshalling(&response,input);
		if ( unmarshal_rc != PEP_OK) {
			log_warn("pep_authorize: PEPd[%s]: can't unmarshal the PEP response: %s.", url, pep_strerror(unmarshal_rc));
			buffer_rewind(b64output);
			buffer_reset(b64input);
			buffer_reset(input);
			log_debug("pep_authorize: PEPd[%s]: reset buffers, trying next failover URL...",url);
			continue;
		}

		failover_ok= TRUE;
		log_info("pep_authorize: PEPd[%s]: authorization Request decoded and unmarshalled.",url);
		break;

	} // failover loop

	if (failover_ok != TRUE) {
		log_error("pep_authorize: all %d PEPd failover URLs failed",(int)url_l);
		buffer_delete(b64output);
		buffer_delete(b64input);
		buffer_delete(input);
		pep_errmsg("all %d PEPd failover URLs failed",(int)url_l);
		return PEP_ERR_AUTHZ_REQUEST;
	}
	/*
	 * FAILOVER END
	 */

	// not required anymore
	buffer_delete(b64output);

	// apply obligation handlers if enabled and any
	int oh_rc= 0;
	if (option_ohs_enabled && llist_length(ohs) > 0) {
		size_t ohs_l= llist_length(ohs);
		log_info("pep_authorize: %d OHs available, processing...", (int)ohs_l);
		for (i= 0; i<ohs_l; i++) {
			pep_obligationhandler_t * oh= llist_get(ohs,i);
			if (oh != NULL) {
				log_debug("pep_authorize: calling OH[%s]->process(request,response)...", oh->id);
				oh_rc = oh->process(&request,&response);
				if (oh_rc != 0) {
					log_error("pep_authorize: OH[%s] process(request,response) failed: %d.", oh->id,oh_rc);
					pep_errmsg("OH[%s] process(request,response) failed: %d.", oh->id,oh_rc);
					return PEP_ERR_AUTHZ_OH_PROCESS;
				}
			}
		}
	}

	// return response
	*out_response= response;

	return PEP_OK;
}

// TODO: return code...
pep_error_t pep_destroy(void) {
	// free options...
	if (option_urls != NULL) {
		llist_delete_elements(option_urls,free);
		llist_delete(option_urls);
		option_urls= NULL;
	}
	// FIXME: free all char options

	// release curl
	if (curl != NULL) {
		curl_easy_cleanup(curl);
		curl= NULL;
	}
	curl_global_cleanup();


	// destroy all pips if any
	int pips_destroy_rc= 0;
	while (llist_length(pips) > 0) {
		pep_pip_t * pip= llist_remove(pips,0);
		if (pip != NULL) {
			pips_destroy_rc += pip->destroy();
		}
	}
	llist_delete(pips);
	if (pips_destroy_rc > 0) {
		log_warn("pep_destroy: some PIP->destroy() failed...");
	}

	// destroy all obligation handlers if any
	int ohs_destroy_rc= 0;
	while (llist_length(ohs) > 0) {
		pep_obligationhandler_t * oh= llist_remove(ohs,0);
		if (oh != NULL) {
			ohs_destroy_rc += oh->destroy();
		}
	}
	llist_delete(ohs);
	if (ohs_destroy_rc > 0) {
		log_warn("pep_destroy: some OH->destroy() failed...");
	}

	// reset all error messages
	pep_clearerr();

	return PEP_OK;
}

