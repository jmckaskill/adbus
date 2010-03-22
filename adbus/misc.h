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

#ifndef __STDC_LIMIT_MACROS
#   define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_CONSTANT_MACROS
#   define __STDC_CONSTANT_MACROS
#endif

#include <adbus.h>

#include "dmem/hash.h"
#include "dmem/string.h"

#include <string.h>

#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && defined(__ELF__)
#   define ADBUSI_FUNC __attribute__((visibility("hidden"))) extern
#   define ADBUSI_DATA __attribute__((visibility("hidden"))) extern
#else
#   define ADBUSI_FUNC
#   define ADBUSI_DATA extern
#endif

#ifdef NDEBUG
#   define ADBUS_TRACE 0
#else
#   define ADBUS_TRACE 1
#endif

#ifndef ADBUS_TRACE_AUTH
#   define ADBUS_TRACE_AUTH     ADBUS_TRACE
#endif

#ifndef ADBUS_TRACE_BIND
#   define ADBUS_TRACE_BIND     ADBUS_TRACE
#endif

#ifndef ADBUS_TRACE_BUS
#   define ADBUS_TRACE_BUS      ADBUS_TRACE
#endif

#ifndef ADBUS_TRACE_MATCH
#   define ADBUS_TRACE_MATCH    ADBUS_TRACE
#endif

#ifndef ADBUS_TRACE_MEMORY
#   define ADBUS_TRACE_MEMORY   ADBUS_TRACE
#endif

#ifndef ADBUS_TRACE_METHOD
#   define ADBUS_TRACE_METHOD   ADBUS_TRACE
#endif

#ifndef ADBUS_TRACE_MSG
#   define ADBUS_TRACE_MSG      ADBUS_TRACE
#endif

#ifndef ADBUS_TRACE_REPLY
#   define ADBUS_TRACE_REPLY    ADBUS_TRACE
#endif

// ----------------------------------------------------------------------------

#ifdef _WIN32
// Stupid visual studio complains about interactions between setjmp and c++
// exceptions even with exceptions turned off - all because we have to
// compile as c++ because it doesn't support c99
#   pragma warning(disable: 4611)
#   pragma warning(disable: 4267) // conversion from size_t to int
#endif

// ----------------------------------------------------------------------------

#define NEW_ARRAY(TYPE, NUM) ((TYPE*) calloc(NUM, sizeof(TYPE)))
#define NEW(TYPE) ((TYPE*) calloc(1, sizeof(TYPE)))
#define ZERO(p) memset(p, 0, sizeof(*p))
#define UNUSED(x) ((void) (x))

// ----------------------------------------------------------------------------

#ifdef __GNUC__
  ADBUS_INLINE long adbus_InterlockedIncrement(long volatile* addend)
  { return __sync_add_and_fetch(addend, 1); }

  ADBUS_INLINE long adbus_InterlockedDecrement(long volatile* addend)
  { return __sync_add_and_fetch(addend, -1); }

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
// For X86 Windows CE build we need to include winbase.h to be able
// to catch the inline functions which overwrite the regular 
// definitions inside of coredll.dll. Though one could use the 
// original version of Increment/Decrement, the others are not
// exported at all.
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

// ----------------------------------------------------------------------------

ADBUSI_FUNC void adbusI_addheader(d_String* str, const char* format, ...);
ADBUSI_FUNC void adbusI_dolog(const char* format, ...);
ADBUSI_FUNC void adbusI_klog(d_String* str);
ADBUSI_FUNC int  adbusI_log_enabled(void);

#if ADBUS_TRACE
#   define adbusI_log adbusI_dolog
#else
#   define adbusI_log if (1){} else adbusI_dolog
#endif

// ----------------------------------------------------------------------------

ADBUSI_DATA const uint8_t adbusI_majorProtocolVersion;

ADBUSI_FUNC char adbusI_nativeEndianness(void);
ADBUSI_FUNC int  adbusI_alignment(char type);

// ----------------------------------------------------------------------------


#pragma pack(push)
#pragma pack(1)
typedef struct adbusI_Header
{
  // 8 byte begin padding
  uint8_t   endianness;
  uint8_t   type;
  uint8_t   flags;
  uint8_t   version;
  uint32_t  length;
  uint32_t  serial;
} adbusI_Header;

typedef struct adbusI_ExtendedHeader
{
  // 8 byte begin padding
  uint8_t   endianness;
  uint8_t   type;
  uint8_t   flags;
  uint8_t   version;
  uint32_t  length;
  uint32_t  serial;

  // HeaderFields are a(yv)
  uint32_t  headerFieldLength;
  // Alignment of header data is 8 byte since array element is a struct
  // sizeof(MessageHeader) == 16 therefore no beginning padding necessary

  // uint8_t headerData[headerFieldLength];
  // uint8_t headerEndPadding -- pads to 8 byte
} adbusI_ExtendedHeader;
#pragma pack(pop)

// ----------------------------------------------------------------------------

enum
{
    ADBUSI_MAXIMUM_ARRAY_LENGTH = 67108864,
    ADBUSI_MAXIMUM_MESSAGE_LENGTH = 134217728,
};

enum
{
    ADBUSI_PARSE_ERROR  = -2,
    ADBUSI_ERROR        = -1,
};

enum
{
    HEADER_INVALID      = 0,
    HEADER_OBJECT_PATH  = 1,
    HEADER_INTERFACE    = 2,
    HEADER_MEMBER       = 3,
    HEADER_ERROR_NAME   = 4,
    HEADER_REPLY_SERIAL = 5,
    HEADER_DESTINATION  = 6,
    HEADER_SENDER       = 7,
    HEADER_SIGNATURE    = 8,
};

// ----------------------------------------------------------------------------

#ifndef min
#   define min(x,y) (((y) < (x)) ? (y) : (x))
#endif

#define CHECK(x) if (!(x)) {assert(0); return -1;}

#define ASSERT_RETURN(x) assert(x); if (!(x)) return;

#ifdef _GNU_SOURCE
#   define adbusI_strndup strndup
#   define adbusI_strdup  strdup
#else
    static inline char* adbusI_strndup(const char* string, size_t n)
    {
        char* s = (char*) malloc(n + 1);
        memcpy(s, string, n);
        s[n] = '\0';
        return s;
    }
    static inline char* adbusI_strdup(const char* string)
    { return adbusI_strndup(string, strlen(string)); }
#endif

// ----------------------------------------------------------------------------

#define ADBUSI_ALIGN(p,b) ADBUS_ALIGN(p,b)

// where b0 is the lowest byte
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

// ----------------------------------------------------------------------------

ADBUSI_FUNC adbus_Bool adbusI_isValidObjectPath(const char* str, size_t len);
ADBUSI_FUNC adbus_Bool adbusI_isValidInterfaceName(const char* str, size_t len);
ADBUSI_FUNC adbus_Bool adbusI_isValidBusName(const char* str, size_t len);
ADBUSI_FUNC adbus_Bool adbusI_isValidMemberName(const char* str, size_t len);
ADBUSI_FUNC adbus_Bool adbusI_hasNullByte(const char* str, size_t len);
ADBUSI_FUNC adbus_Bool adbusI_isValidUtf8(const char* str, size_t len);

// ----------------------------------------------------------------------------

ADBUSI_FUNC int adbusI_dispatch(adbus_MsgCallback cb, adbus_CbData* details);

ADBUSI_FUNC int adbusI_pathError(adbus_CbData* d);
ADBUSI_FUNC int adbusI_interfaceError(adbus_CbData* d);
ADBUSI_FUNC int adbusI_methodError(adbus_CbData* d);
ADBUSI_FUNC int adbusI_propertyError(adbus_CbData* d);
ADBUSI_FUNC int adbusI_propWriteError(adbus_CbData* d);
ADBUSI_FUNC int adbusI_propReadError(adbus_CbData* d);
ADBUSI_FUNC int adbusI_propTypeError(adbus_CbData* d);

// ----------------------------------------------------------------------------


ADBUSI_FUNC void adbusI_relativePath(d_String* out, const char* path1, int size1, const char* path2, int size2);
ADBUSI_FUNC void adbusI_parentPath(dh_strsz_t path, dh_strsz_t* parent);

ADBUSI_FUNC void adbusI_matchString(d_String* out, const adbus_Match* match);

ADBUSI_FUNC void adbusI_logmsg(const char* header, const adbus_Message* msg);
ADBUSI_FUNC void adbusI_logbind(const char* header, const adbus_Bind* bind);
ADBUSI_FUNC void adbusI_logmatch(const char* header, const adbus_Match* match);
ADBUSI_FUNC void adbusI_logreply(const char* header, const adbus_Reply* reply);

