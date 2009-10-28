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

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Stupid visual studio complains about interactions between setjmp and c++ 
// exceptions even with exceptions turned off - all because we have to 
// compile as c++ because it doesn't support c99
#ifdef WIN32
#   pragma warning(disable: 4611)
#endif

// ----------------------------------------------------------------------------

#ifndef NDEBUG
#   define LOGD ADBusPrintDebug_
#else
#   define LOGD if(0) {} else ADBusPrintDebug_
#endif

ADBUSI_FUNC void ADBusPrintDebug_(const char* format, ...);

// ----------------------------------------------------------------------------

ADBUSI_DATA const char ADBusNativeEndianness_;
ADBUSI_DATA const uint8_t ADBusMajorProtocolVersion_;

ADBUSI_FUNC int ADBusRequiredAlignment_(char type);

ADBUSI_FUNC uint ADBusRequiresServiceLookup_(const char* name, int size);

// ----------------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)
struct ADBusHeader_
{
  // 8 byte begin padding
  uint8_t   endianness;
  uint8_t   type;
  uint8_t   flags;
  uint8_t   version;
  uint32_t  length;
  uint32_t  serial;
};

struct ADBusExtendedHeader_
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

#ifndef min
#   define min(x,y) (((x) < (y)) ? (x) : (y))
#endif

#define ASSERT_RETURN(x) assert(x); if (!(x)) return;
#define ASSERT_RETURN_ERR(x) assert(x); if (!(x)) return -1;

static inline char* strndup_(const char* string, size_t n)
{
    char* s = (char*) malloc(n + 1);
    memcpy(s, string, n);
    s[n] = '\0';
    return s;
}
static inline char* strdup_(const char* string)
{
    return strndup_(string, strlen(string));
}

ADBUSI_FUNC uint64_t TimerBegin();
ADBUSI_FUNC void TimerEnd(uint64_t begin, const char* what);

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

#define ADBUS_ALIGN_VALUE_(TYPE, PTR, BOUNDARY)                       \
  ((TYPE) ( (((uintptr_t) (PTR)) + (((uintptr_t) (BOUNDARY)) - 1))    \
                 & (~(((uintptr_t)(BOUNDARY))-1))                     \
                 )                                                    \
  )
// ----------------------------------------------------------------------------

// where b0 is the lowest byte
#define MAKE16(b1,b0) \
  (((uint16_t)(b1) << 8) | (b0))
#define MAKE32(b3,b2,b1,b0) \
  (((uint32_t)(MAKE16((b3),(b2))) << 16) | MAKE16((b1),(b0)))
#define MAKE64(b7,b6,b5,b4,b3,b2,b1,b0)  \
  (((uint64_t)(MAKE32((b7),(b6),(b5),(b4))) << 32) | MAKE32((b3),(b2),(b1),(b0)))

#define ENDIAN_CONVERT16(val) MAKE16( (val)        & 0xFF, ((val) >> 8)  & 0xFF)

#define ENDIAN_CONVERT32(val) MAKE32( (val)        & 0xFF, ((val) >> 8)  & 0xFF,\
                                     ((val) >> 16) & 0xFF, ((val) >> 24) & 0xFF)

#define ENDIAN_CONVERT64(val) MAKE64( (val)        & 0xFF, ((val) >> 8)  & 0xFF,\
                                     ((val) >> 16) & 0xFF, ((val) >> 24) & 0xFF,\
                                     ((val) >> 32) & 0xFF, ((val) >> 40) & 0xFF,\
                                     ((val) >> 48) & 0xFF, ((val) >> 56) & 0xFF)

// ----------------------------------------------------------------------------

ADBUSI_FUNC uint ADBusIsValidObjectPath_(const char* str, size_t len);
ADBUSI_FUNC uint ADBusIsValidInterfaceName_(const char* str, size_t len);
ADBUSI_FUNC uint ADBusIsValidBusName_(const char* str, size_t len);
ADBUSI_FUNC uint ADBusIsValidMemberName_(const char* str, size_t len);
ADBUSI_FUNC uint ADBusHasNullByte_(const uint8_t* str, size_t len);
ADBUSI_FUNC uint ADBusIsValidUtf8_(const uint8_t* str, size_t len);
ADBUSI_FUNC const char* ADBusFindArrayEnd_(const char* arrayBegin);

// ----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

