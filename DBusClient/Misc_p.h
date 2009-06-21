// vim: ts=2 sw=2 sts=2 et
//
// Copyright (c) 2009 James R. McKaskill
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// ----------------------------------------------------------------------------

#pragma once

#include "Common.h"



// ----------------------------------------------------------------------------

extern const char _DBusNativeEndianness;

int _DBusRequiredAlignment(char type);

// ----------------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)
struct _DBusMessageHeader
{
  // 8 byte begin padding
  uint8_t   endianness;
  uint8_t   type;
  uint8_t   flags;
  uint8_t   version;
  uint32_t  length;
  uint32_t  serial;
};

struct _DBusMessageExtendedHeader
{
  struct _DBusMessageHeader header;

  // HeaderFields are a(yv)
  uint32_t  headerFieldLength;
  // Alignment of header data is 8 byte since array element is a struct
  // sizeof(MessageHeader) == 16 therefore no beginning padding necessary

  // uint8_t headerData[headerFieldLength];
  // uint8_t headerEndPadding -- pads to 8 byte
};
#pragma pack(pop)

// ----------------------------------------------------------------------------


#define ASSERT_RETURN(x) assert(x); if (!(x)) return;

/*
 * Realloc the buffer pointed at by variable 'x' so that it can hold
 * at least 'nr' entries; the number of entries currently allocated
 * is 'alloc', using the standard growing factor alloc_nr() macro.
 *
 * DO NOT USE any expression with side-effect for 'x' or 'alloc'.
 */
#define alloc_nr(x) (((x)+16)*3/2)
#define ALLOC_GROW(Type, x, nr, alloc) \
	do { \
		if ((nr) > alloc) { \
			if (alloc_nr(alloc) < (nr)) \
				alloc = (nr); \
			else \
				alloc = alloc_nr(alloc); \
			x = (Type*)realloc((x), alloc * sizeof(*(x))); \
		} \
	} while(0)

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

#define _DBUS_ALIGN_VALUE(this, boundary) \
  (( ((unsigned long)(this)) + (((unsigned long)(boundary)) -1)) & (~(((unsigned long)(boundary))-1)))

// ----------------------------------------------------------------------------

unsigned int _DBusIsValidObjectPath(const char* str, size_t len);
unsigned int _DBusIsValidInterfaceName(const char* str, size_t len);
unsigned int _DBusIsValidBusName(const char* str, size_t len);
unsigned int _DBusIsValidMemberName(const char* str, size_t len);
unsigned int _DBusHasNullByte(const char* str, size_t len);
unsigned int _DBusIsValidUtf8(const char* str, size_t len);

// ----------------------------------------------------------------------------


