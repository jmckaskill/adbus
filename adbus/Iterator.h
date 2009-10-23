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

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------

struct ADBusIterator;
// Iterator over a given block of marshalled memory that it does _not_ own

ADBUS_API struct ADBusIterator* ADBusCreateIterator();
ADBUS_API void ADBusFreeIterator(struct ADBusIterator* i);

ADBUS_API void ADBusResetIterator(struct ADBusIterator* i,
                                  const char* sig, int sigsize,
                                  const uint8_t* data, size_t size);

enum ADBusEndianness
{
    ADBusLittleEndian = 'l',
    ADBusBigEndian = 'B',
};

ADBUS_API void ADBusSetIteratorEndianness(struct ADBusIterator* i,
                                          enum ADBusEndianness endianness);

ADBUS_API const uint8_t* ADBusCurrentIteratorData(struct ADBusIterator* i, size_t* size);
ADBUS_API const char* ADBusCurrentIteratorSignature(struct ADBusIterator* i, size_t* size);

ADBUS_API uint ADBusIsScopeAtEnd(struct ADBusIterator* i, uint scope);

// ----------------------------------------------------------------------------

struct ADBusField
{
  enum ADBusFieldType type;
  uint8_t         u8;
  uint            b;
  int16_t         i16;
  uint16_t        u16;
  int32_t         i32;
  uint32_t        u32;
  int64_t         i64;
  uint64_t        u64;
  double          d;
  const char*     string;
  size_t          size;
  uint            scope;
};

ADBUS_API int ADBusIterate(struct ADBusIterator* i, struct ADBusField* field);
ADBUS_API int ADBusJumpToEndOfArray(struct ADBusIterator* i, uint scope);

// ----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
