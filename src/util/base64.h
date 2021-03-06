/*
 * Copyright (c) Members of the EGEE Collaboration. 2006-2010.
 * See http://www.eu-egee.org/partners/ for details on the copyright holders.
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
 */

#ifndef _PEP_BASE64_H_
#define _PEP_BASE64_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include "buffer.h"

/* PEM default line size (RFC???) */
#ifndef BASE64_DEFAULT_LINE_SIZE
#define BASE64_DEFAULT_LINE_SIZE 64
#endif

/**
 * Base64 encodes the in buffer into the out buffer (without line break).
 *
 * @param pep_buffer_t * in pointer to the in buffer.
 * @param pep_buffer_t * out pointer to the out buffer.
 */
void pep_base64_encode_buffer(pep_buffer_t * in, pep_buffer_t * out);

/**
 * Base64 encodes the in buffer into the out buffer.
 * The base64 encoded block have a line length of linesize [4..inf].
 *
 * @param pep_buffer_t * in pointer to the in buffer.
 * @param pep_buffer_t * out pointer to the out buffer.
 * @param int linesize length of the line (min 4)
 */
void pep_base64_encode_buffer_l(pep_buffer_t * in, pep_buffer_t * out, int linesize);

/**
 * Base64 decodes the in buffer into the out buffer.
 *
 * @param pep_buffer_t * in pointer to the in buffer.
 * @param pep_buffer_t * out pointer to the out buffer.
 */
void pep_base64_decode_buffer(pep_buffer_t * in, pep_buffer_t * out);

#ifdef  __cplusplus
}
#endif

#endif
