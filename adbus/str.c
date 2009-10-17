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

#include "str.h"


#include "vector.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

VECTOR_INSTANTIATE(char, c)

#ifdef NDEBUG
#   define str_assert(x) do {}while(0)
#else
void str_assert(const str_t* pstring)
{
    if (*pstring) {
        size_t size = cvector_size(pstring);
        assert(size >= 1);
        assert((*pstring)[size - 1] == '\0');
    }
}
#endif

size_t str_size(const str_t* pstring)
{
    str_assert(pstring);
    return *pstring
         ? cvector_size(pstring) - 1
         : 0;
}

void str_append_n(str_t* pstring, const char* str, size_t n)
{
    str_assert(pstring);
    // If we have a vector then it already has the null terminator, otherwise
    // we need to add it in
    void* dest = (cvector_size(pstring) == 0)
               ? cvector_insert_end(pstring, n + 1)
               : cvector_insert_end(pstring, n) - 1;
    memcpy(dest, str, n);
    ((char*) dest)[n] = '\0';
    str_assert(pstring);
}

void str_append(str_t* pstring, const char* str)
{
    if (str)
        str_append_n(pstring, str, strlen(str));
}


void str_append_char(str_t* pstring, int ch)
{
    uint8_t u8 = ch;
    str_append_n(pstring, (const char*) &u8, 1);
}

void str_insert_n(str_t* pstring, size_t index, const char* str, size_t n)
{
    str_assert(pstring);
    assert(index <= str_size(pstring));
    // Special case the initial insert since append sets up the null terminator
    if (*pstring) {
        char* dest = cvector_insert(pstring, index, n);
        memcpy(dest, str, n);
    } else {
        str_append_n(pstring, str, n);
    }
    str_assert(pstring);
}

void str_insert(str_t* pstring, size_t index, const char* str)
{
    str_insert_n(pstring, index, str, strlen(str));
}

void str_remove(str_t* pstring, size_t index, size_t number)
{
    str_assert(pstring);
    assert(number > 0);
    assert(number <= str_size(pstring));
    cvector_remove(pstring, index, number);
    str_assert(pstring);
}

void str_remove_end(str_t* pstring, size_t number)
{
    str_remove(pstring, str_size(pstring) - number, number);
}

void str_clear(str_t* pstring)
{
    size_t size = str_size(pstring);
    if (size > 0)
        str_remove_end(pstring, size);
}

void str_set_n(str_t* pstring, const char* str, size_t n)
{
    str_clear(pstring);
    str_append_n(pstring, str, n);
}

void str_set(str_t* pstring, const char* str)
{
    str_clear(pstring);
    str_append(pstring, str);
}

void str_free(str_t* pstring)
{
    str_assert(pstring);
    cvector_free(pstring);
    str_assert(pstring);
}

#ifdef WIN32
int str_printf(str_t* pstring, const char* format, ...)
{
    va_list ap;
    size_t base = str_size(pstring);
    int ret = -1;
    while(ret <= 0)
    {
      cvector_insert_end(pstring, (str_size(pstring) - base) + 128);
      va_start(ap, format);
      ret = vsnprintf(*pstring + base, str_size(pstring) - base - 1, format, ap);
      va_end(ap);
    }
    size_t toremove = str_size(pstring) - (base + ret);
    if (toremove > 0)
        str_remove_end(pstring, toremove);
    str_assert(pstring);
    return ret;
}

#else
int str_printf(str_t* pstring, const char* format, ...)
{
    va_list ap;
    str_assert(pstring);
    size_t base = str_size(pstring); // base of formatted string
    size_t end  = base + 128 + 1;     // end of string inc nul
    cvector_insert_end(pstring, end - cvector_size(pstring));
    va_start(ap, format);
    int nchars = vsnprintf(*pstring + base, end - base, format, ap);
    va_end(ap);
    if (nchars + 1 >= end - base) {
        end = base + nchars + 1;
        cvector_insert_end(pstring, end - cvector_size(pstring));
        va_start(ap, format);
        nchars = vsnprintf(*pstring + base, end - base, format, ap);
        va_end(ap);
    }
    // we may need to cut off extra allocated bytes
    size_t printfend = base + nchars + 1;
    if (end > printfend)
        cvector_remove_end(pstring, end - printfend);
    str_assert(pstring);
    return nchars;
}
#endif



