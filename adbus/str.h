/* vim: ts=4 sw=4 sts=4 et
 *
 * Copyright (c) 2009 James R. McKaskill
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 */

#pragma once

#include "Common.h"

typedef char* str_t;

#ifdef __cplusplus
extern "C" {
#endif

ADBUSI_FUNC size_t str_size(const str_t* pstring);

ADBUSI_FUNC void   str_append(str_t* pstring, const char* str);
ADBUSI_FUNC void   str_append_n(str_t* pstring, const char* str, size_t n);
ADBUSI_FUNC void   str_append_char(str_t* pstring, int ch);

ADBUSI_FUNC void   str_insert(str_t* pstring, size_t index, const char* str);
ADBUSI_FUNC void   str_insert_n(str_t* pstring, size_t index, const char* str, size_t n);

ADBUSI_FUNC void   str_remove(str_t* pstring, size_t index, size_t number);
ADBUSI_FUNC void   str_remove_end(str_t* pstring, size_t number);

ADBUSI_FUNC void   str_set(str_t* pstring, const char* str);
ADBUSI_FUNC void   str_set_n(str_t* pstring, const char* str, size_t n);

ADBUSI_FUNC void   str_clear(str_t* pstring);
ADBUSI_FUNC void   str_free(str_t* pstring);
ADBUSI_FUNC int    str_printf(str_t* pstring, const char* format, ...);

#ifdef __cplusplus
}
#endif
