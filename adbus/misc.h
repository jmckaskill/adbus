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

#include <adbus/adbus.h>

#include "memory/kstring.h"
#include <string.h>

#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && defined(__ELF__)
#   define ADBUSI_FUNC __attribute__((visibility("hidden"))) extern
#   define ADBUSI_DATA __attribute__((visibility("hidden"))) extern
#else
#   define ADBUSI_FUNC
#   define ADBUSI_DATA extern
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

ADBUSI_DATA const uint8_t adbusI_majorProtocolVersion;

ADBUSI_FUNC adbus_Bool adbusI_littleEndian(void);
ADBUSI_FUNC int adbusI_alignment(char type);

ADBUSI_FUNC adbus_Bool adbusI_requiresServiceLookup(const char* name, int size);

// ----------------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)
struct adbusI_Header
{
  // 8 byte begin padding
  uint8_t   endianness;
  uint8_t   type;
  uint8_t   flags;
  uint8_t   version;
  uint32_t  length;
  uint32_t  serial;
};

struct adbusI_ExtendedHeader
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
};
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

// ----------------------------------------------------------------------------

#ifndef min
#   define min(x,y) (((x) < (y)) ? (x) : (y))
#endif

#define ASSERT_RETURN(x) assert(x); if (!(x)) return;
#define ASSERT_RETURN_ERR(x) assert(x); if (!(x)) return -1;

static inline char* adbusI_strndup(const char* string, size_t n)
{
    char* s = (char*) malloc(n + 1);
    memcpy(s, string, n);
    s[n] = '\0';
    return s;
}
static inline char* adbusI_strdup(const char* string)
{
    return adbusI_strndup(string, strlen(string));
}

// ----------------------------------------------------------------------------

/* This alignment thing is from ORBit2 */
/* Align a value upward to a boundary, expressed as a number of bytes.
 * E.g. align to an 8-byte boundary with argument of 8.
 */

/*
 *   (this + boundary - 1)
 *          &
 *    ~(boundary - 1)
 */

#define ADBUSI_ALIGN(PTR, BOUNDARY) \
  ((((uintptr_t) (PTR)) + (((uintptr_t) (BOUNDARY)) - 1)) & (~(((uintptr_t)(BOUNDARY))-1)))

// ----------------------------------------------------------------------------

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
ADBUSI_FUNC adbus_Bool adbusI_hasNullByte(const uint8_t* str, size_t len);
ADBUSI_FUNC adbus_Bool adbusI_isValidUtf8(const uint8_t* str, size_t len);
ADBUSI_FUNC const char* adbusI_findArrayEnd(const char* arrayBegin);

// ----------------------------------------------------------------------------

ADBUSI_FUNC int adbusI_argumentError(adbus_CbData* d);
ADBUSI_FUNC int adbusI_pathError(adbus_CbData* d);
ADBUSI_FUNC int adbusI_interfaceError(adbus_CbData* d);
ADBUSI_FUNC int adbusI_methodError(adbus_CbData* d);
ADBUSI_FUNC int adbusI_propertyError(adbus_CbData* d);
ADBUSI_FUNC int PropWriteError(adbus_CbData* d);
ADBUSI_FUNC int PropReadError(adbus_CbData* d);
ADBUSI_FUNC int PropTypeError(adbus_CbData* d);

// ----------------------------------------------------------------------------


ADBUSI_FUNC adbus_User* adbusI_puser_new(void* p);
ADBUSI_FUNC void* adbusI_puser_get(const adbus_User* data);

ADBUSI_FUNC void adbusI_relativePath(kstring_t* out, const char* path1, int size1, const char* path2, int size2);
ADBUSI_FUNC void adbusI_parentPath(char* path, size_t size, size_t* parentSize);

ADBUSI_FUNC kstring_t* adbusI_matchString(const adbus_Match* match);

