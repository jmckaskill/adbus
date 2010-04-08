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

#include <stdlib.h>

#if !defined HW_INLINE
#   if defined __cplusplus || __STDC_VERSION__ + 0 >= 199901L
#       define HW_INLINE static inline
#   else
#       define HW_INLINE static
#   endif
#endif

#ifdef __cplusplus
# define HW_EXTERN_C extern "C"
#else
# define HW_EXTERN_C extern
#endif

#ifdef HW_BUILD_AS_DLL
# ifdef HW_LIBRARY
#   define HW_API HW_EXTERN_C __declspec(dllexport)
# else
#   define HW_API HW_EXTERN_C __declspec(dllimport)
# endif
#else
# define HW_API HW_EXTERN_C
#endif

typedef struct HW_EventLoop HW_EventLoop;
typedef struct HW_Can HW_Can;
typedef struct HW_Serial HW_Serial;
typedef struct HW_Lock HW_Lock;
typedef struct HWI_EventQueue HWI_EventQueue;
typedef struct HW_Message HW_Message;
typedef struct HW_Freelist HW_Freelist;
typedef struct HW_FreelistHeader HW_FreelistHeader;
typedef struct HW_ThreadStorage HW_ThreadStorage;

typedef void (*HW_Callback)(void*);

#ifndef HW_NOT_COPYABLE
# define HW_NOT_COPYABLE(c) private: c(c& __DummyArg); c& operator=(c& __DummyArg)
#endif

#ifdef _WIN32
  typedef void* HW_Handle; /* HANDLE */
#else
  typedef int HW_Handle;   /* fd_t */
#endif

#ifdef HW_LIBRARY
#	define NEW(type) (type*) calloc(1, sizeof(type))
#endif


