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

#include "internal.h"

/* -------------------------------------------------------------------------- */

#ifdef _WIN32
/* Stupid visual studio complains about interactions between setjmp and c++
 * exceptions even with exceptions turned off - all because we have to
 * compile as c++ because it doesn't support c99 
 */
#   pragma warning(disable: 4611)
#   pragma warning(disable: 4267) /* conversion from size_t to int */
#endif

/* -------------------------------------------------------------------------- */

#define NEW_ARRAY(TYPE, NUM) ((TYPE*) calloc(NUM, sizeof(TYPE)))
#define NEW(TYPE) ((TYPE*) calloc(1, sizeof(TYPE)))
#define ZERO(v) memset(&v, 0, sizeof(v));
#define UNUSED(x) ((void) (x))

/* -------------------------------------------------------------------------- */

#ifdef __GNUC__
    ADBUS_INLINE long adbusI_InterlockedIncrement(long volatile* addend)
    { return __sync_add_and_fetch(addend, 1); }

    ADBUS_INLINE long adbusI_InterlockedDecrement(long volatile* addend)
    { return __sync_add_and_fetch(addend, -1); }

    ADBUS_INLINE void* adbusI_InterlockedExchangePointer(void* volatile* xch, void* new_value)
    { return __sync_lock_test_and_set(xch, new_value); }

#elif defined _MSC_VER && _MSC_VER < 1300 && defined _M_IX86
#error Apparently MSVC++ 6.0 generates rubbish when optimizations are on

#elif defined _MSC_VER && defined _WIN32 && !defined _WIN32_WCE

extern "C" {
    long __cdecl _InterlockedIncrement(volatile long *);
    long __cdecl _InterlockedDecrement(volatile long *);
}
#  pragma intrinsic (_InterlockedIncrement)
#  pragma intrinsic (_InterlockedDecrement)

ADBUS_INLINE long adbus_InterlockedIncrement(long volatile* addend)
{ return _InterlockedIncrement(addend); }

ADBUS_INLINE long adbus_InterlockedDecrement(long volatile* addend)
{ return _InterlockedDecrement(addend); }

#elif defined _MSC_VER && defined _WIN32 && defined _WIN32_WCE

#if _WIN32_WCE < 0x600 && defined(_X86_)
/* For X86 Windows CE build we need to include winbase.h to be able
 * to catch the inline functions which overwrite the regular 
 * definitions inside of coredll.dll. Though one could use the 
 * original version of Increment/Decrement, the others are not
 * exported at all.
 */
#include <winbase.h>
#else

#if _WIN32_WCE >= 0x600
#define VOLATILE volatile
#  if defined(_X86_)
#    define InterlockedIncrement _InterlockedIncrement
#    define InterlockedDecrement _InterlockedDecrement
#  endif
#else
#define VOLATILE
#endif

extern "C" {
    long __cdecl InterlockedIncrement(long VOLATILE* addend);
    long __cdecl InterlockedDecrement(long VOLATILE * addend);
}

#if _WIN32_WCE >= 0x600 && defined(_X86_)
#  pragma intrinsic (_InterlockedIncrement)
#  pragma intrinsic (_InterlockedDecrement)
#endif

ADBUS_INLINE long adbus_InterlockedIncrement(long volatile* addend)
{ return _InterlockedIncrement(addend); }

ADBUS_INLINE long adbus_InterlockedDecrement(long volatile* addend)
{ return _InterlockedDecrement(addend); }

#endif

#endif

/* -------------------------------------------------------------------------- */

ADBUSI_DATA const uint8_t adbusI_majorProtocolVersion;

ADBUSI_FUNC char adbusI_nativeEndianness(void);
ADBUSI_FUNC int  adbusI_alignment(char type);

/* -------------------------------------------------------------------------- */

#ifndef min
#   define min(x,y) (((y) < (x)) ? (y) : (x))
#endif

#ifdef _GNU_SOURCE
#   define adbusI_strndup strndup
#   define adbusI_strdup  strdup
#elif _MSC_VER
#   define strndup _strndup
#   define strdup  _strdup
#else
    ADBUS_INLINE char* adbusI_strndup(const char* string, size_t n)
    {
        char* s = (char*) malloc(n + 1);
        memcpy(s, string, n);
        s[n] = '\0';
        return s;
    }
    ADBUS_INLINE char* adbusI_strdup(const char* string)
    { return adbusI_strndup(string, strlen(string)); }
#endif

/* -------------------------------------------------------------------------- */

/* where b0 is the lowest byte */
#define ADBUSI_MAKE16(b1,b0) \
  (((uint16_t)(b1) << 8) | (b0))
#define ADBUSI_MAKE32(b3,b2,b1,b0) \
  (((uint32_t)(ADBUSI_MAKE16((b3),(b2))) << 16) | ADBUSI_MAKE16((b1),(b0)))
#define ADBUSI_MAKE64(b7,b6,b5,b4,b3,b2,b1,b0)  \
  (((uint64_t)(ADBUSI_MAKE32((b7),(b6),(b5),(b4))) << 32) | ADBUSI_MAKE32((b3),(b2),(b1),(b0)))

#define ADBUSI_FLIP16(val) ADBUSI_MAKE16( (val)        & 0xFF, ((val) >> 8)  & 0xFF)

#define ADBUSI_FLIP32(val) ADBUSI_MAKE32( (val)        & 0xFF, ((val) >> 8)  & 0xFF,\
                                     ((val) >> 16) & 0xFF, ((val) >> 24) & 0xFF)

#define ADBUSI_FLIP64(val) ADBUSI_MAKE64( (val)        & 0xFF, ((val) >> 8)  & 0xFF,\
                                     ((val) >> 16) & 0xFF, ((val) >> 24) & 0xFF,\
                                     ((val) >> 32) & 0xFF, ((val) >> 40) & 0xFF,\
                                     ((val) >> 48) & 0xFF, ((val) >> 56) & 0xFF)

/* -------------------------------------------------------------------------- */

ADBUSI_FUNC adbus_Bool adbusI_isValidObjectPath(const char* str, size_t len);
ADBUSI_FUNC adbus_Bool adbusI_isValidInterfaceName(const char* str, size_t len);
ADBUSI_FUNC adbus_Bool adbusI_isValidBusName(const char* str, size_t len);

/* -------------------------------------------------------------------------- */

ADBUSI_FUNC int adbusI_dispatch(adbus_MsgCallback cb, adbus_CbData* details);


/* -------------------------------------------------------------------------- */


ADBUSI_FUNC void adbusI_relativePath(d_String* out, const char* path1, int size1, const char* path2, int size2);
ADBUSI_FUNC void adbusI_sanitisePath(d_String* out, const char* path, int sz);
ADBUSI_FUNC void adbusI_parentPath(dh_strsz_t path, dh_strsz_t* parent);

ADBUSI_FUNC void adbusI_matchString(d_String* out, const adbus_Match* match);
ADBUSI_FUNC adbus_Bool adbusI_matchesMessage(const adbus_Match* match, adbus_Message* msg);

