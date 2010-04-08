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

#if !defined MT_INLINE
#   if defined __cplusplus || __STDC_VERSION__ + 0 >= 199901L
#       define MT_INLINE static inline
#   else
#       define MT_INLINE static
#   endif
#endif

#ifdef __cplusplus
# define MT_EXTERN_C extern "C"
#else
# define MT_EXTERN_C extern
#endif

#if defined MT_SHARED_LIBRARY
#   ifdef _MSC_VER
#       if defined MT_LIBRARY
#           define MT_API MT_EXTERN_C __declspec(dllexport)
#       else
#           define MT_API MT_EXTERN_C __declspec(dllimport)
#       endif
#   elif defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && defined(__ELF__)
#       define MT_API MT_EXTERN_C __attribute__((visibility("default")))
#   else
#       define MT_API MT_EXTERN_C
#   endif
#else
#   define MT_API MT_EXTERN_C
#endif

typedef struct MT_EventLoop MT_EventLoop;
typedef struct MT_Lock MT_Lock;
typedef struct MTI_EventQueue MTI_EventQueue;
typedef struct MT_Message MT_Message;
typedef struct MT_Freelist MT_Freelist;
typedef struct MT_FreelistHeader MT_FreelistHeader;
typedef struct MT_ThreadStorage MT_ThreadStorage;

typedef void (*MT_Callback)(void*);

#ifndef MT_NOT_COPYABLE
# define MT_NOT_COPYABLE(c) private: c(c& __DummyArg); c& operator=(c& __DummyArg)
#endif

#ifdef _WIN32
  typedef void* MT_Handle; /* HANDLE */
#else
  typedef int MT_Handle;   /* fd_t */
#endif

#ifdef MT_LIBRARY
#   define NEW(type) (type*) calloc(1, sizeof(type))
#endif


