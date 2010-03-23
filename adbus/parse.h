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

enum
{
    ADBUSI_MAXIMUM_ARRAY_LENGTH = 67108864,
    ADBUSI_MAXIMUM_MESSAGE_LENGTH = 134217728
};

enum
{
    ADBUSI_PARSE_ERROR  = -2,
    ADBUSI_ERROR        = -1
};

enum
{
    ADBUSI_HEADER_INVALID      = 0,
    ADBUSI_HEADER_OBJECT_PATH  = 1,
    ADBUSI_HEADER_INTERFACE    = 2,
    ADBUSI_HEADER_MEMBER       = 3,
    ADBUSI_HEADER_ERROR_NAME   = 4,
    ADBUSI_HEADER_REPLY_SERIAL = 5,
    ADBUSI_HEADER_DESTINATION  = 6,
    ADBUSI_HEADER_SENDER       = 7,
    ADBUSI_HEADER_SIGNATURE    = 8
};

#pragma pack(push)
#pragma pack(1)
struct adbusI_Header
{
  /* 8 byte begin padding */
  uint8_t   endianness;
  uint8_t   type;
  uint8_t   flags;
  uint8_t   version;
  uint32_t  length;
  uint32_t  serial;
};

struct adbusI_ExtendedHeader
{
  /* 8 byte begin padding */
  uint8_t   endianness;
  uint8_t   type;
  uint8_t   flags;
  uint8_t   version;
  uint32_t  length;
  uint32_t  serial;

  /* HeaderFields are a(yv) */
  uint32_t  headerFieldLength;

  /* Alignment of header data is 8 byte since array element is a struct
   * sizeof(MessageHeader) == 16 therefore no beginning padding necessary
   *
   * uint8_t headerData[headerFieldLength];
   * uint8_t headerEndPadding -- pads to 8 byte
   */
};
#pragma pack(pop)

