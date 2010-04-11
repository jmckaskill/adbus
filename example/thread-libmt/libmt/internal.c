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

#include "internal.h"
#include <dmem/string.h>
#include <stdio.h>

/* ------------------------------------------------------------------------- */

void MT_Log(const char* format, ...)
{
	d_String str;
	va_list ap;

	memset(&str, 0, sizeof(str));

#ifdef _WIN32
    ds_cat_f(&str, "[libmt %d] ", GetCurrentThreadId());
#elif __linux__
    ds_cat_f(&str, "[libmt %p] ", (void*) pthread_self());
#endif

	va_start(ap, format);
	ds_cat_vf(&str, format, ap);
    va_end(ap);

	ds_cat(&str, "\n");

#if defined _WIN32 && !defined NDEBUG
	_CrtDbgReport(_CRT_WARN, NULL, 0, NULL, "%.*s", (int) ds_size(&str), ds_cstr(&str));
#else
	fwrite(ds_cstr(&str), 1, ds_size(&str), stderr);
#endif

    ds_free(&str);
}

