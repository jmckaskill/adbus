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
    enum ADBusFieldType   type;

    // Entries used for simple types
    uint                  b;
    uint8_t               u8;
    int16_t               i16;
    uint16_t              u16;
    int32_t               i32;
    uint32_t              u32;
    int64_t               i64;
    uint64_t              u64;
    double                d;

    // Data is filled out for arrays, string for string types, size for both
    // in bytes. The string is guarenteed by dbus protocol to be valid utf8
    // with a trailing null (not included in size) and no embedded nulls.
    // Data can be used for array of fixed sized correctly padded simple types
    // and structs for very quick decomposing (use ADBusIterate with array
    // begin, then ADBusJumpToEndOfArray, then ADBusIterate with array end) to
    // quickly get the array data.
    const uint8_t*        data;
    const char*           string;
    size_t                size;

    // Filled out by all scoped begins/ends (arrays, struct, dict, variant)
    uint                  scope;
};

ADBUS_API int ADBusIterate(struct ADBusIterator* i, struct ADBusField* field);
ADBUS_API int ADBusJumpToEndOfArray(struct ADBusIterator* i, uint scope);

// ----------------------------------------------------------------------------

// Check functions in the style of the lua check functions (eg
// luaL_checkstring). They will longjmp back to throw an invalid argument
// error. Thus they should only be used to pull out the arguments at the very
// beginning of a message callback.

ADBUS_API void        ADBusCheckMessageEnd(struct ADBusCallDetails* d);
ADBUS_API uint        ADBusCheckBoolean(struct ADBusCallDetails* d);
ADBUS_API uint8_t     ADBusCheckUInt8(struct ADBusCallDetails* d);
ADBUS_API int16_t     ADBusCheckInt16(struct ADBusCallDetails* d);
ADBUS_API uint16_t    ADBusCheckUInt16(struct ADBusCallDetails* d);
ADBUS_API int32_t     ADBusCheckInt32(struct ADBusCallDetails* d);
ADBUS_API uint32_t    ADBusCheckUInt32(struct ADBusCallDetails* d);
ADBUS_API int64_t     ADBusCheckInt64(struct ADBusCallDetails* d);
ADBUS_API uint64_t    ADBusCheckUInt64(struct ADBusCallDetails* d);
ADBUS_API double      ADBusCheckDouble(struct ADBusCallDetails* d);
ADBUS_API const char* ADBusCheckString(struct ADBusCallDetails* d, size_t* size);
ADBUS_API const char* ADBusCheckObjectPath(struct ADBusCallDetails* d, size_t* size);
ADBUS_API const char* ADBusCheckSignature(struct ADBusCallDetails* d, size_t* size);
ADBUS_API uint        ADBusCheckArrayBegin(struct ADBusCallDetails* d, const uint8_t** data, size_t* size);
ADBUS_API void        ADBusCheckArrayEnd(struct ADBusCallDetails* d);
ADBUS_API uint        ADBusCheckStructBegin(struct ADBusCallDetails* d);
ADBUS_API void        ADBusCheckStructEnd(struct ADBusCallDetails* d);
ADBUS_API uint        ADBusCheckDictEntryBegin(struct ADBusCallDetails* d);
ADBUS_API void        ADBusCheckDictEntryEnd(struct ADBusCallDetails* d);
ADBUS_API const char* ADBusCheckVariantBegin(struct ADBusCallDetails* d, size_t* size);
ADBUS_API void        ADBusCheckVariantEnd(struct ADBusCallDetails* d);

// ----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
