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


/** \defgroup ADBusIterator Iterator
 *  \ingroup adbus
 *
 * \brief Iterator brief 
 *
 * Iterator description
 *
 *  @{
 */

// ----------------------------------------------------------------------------

/** Iterator over a given block of marshalled memory that it does _not_ own.
 */
struct ADBusIterator;

ADBUS_API struct ADBusIterator* ADBusCreateIterator();
ADBUS_API void ADBusFreeIterator(struct ADBusIterator* iterator);

ADBUS_API void ADBusResetIterator(
        struct ADBusIterator*       iterator,
        const char*                 sig,
        int                         sigsize,
        const uint8_t*              data,
        size_t                      size);

ADBUS_API void ADBusGetIteratorData(
        const struct ADBusIterator* iterator,
        const char**                sig,
        size_t*                     sigsize,
        const uint8_t**             data,
        size_t*                     datasize);

enum ADBusEndianness
{
    ADBusLittleEndian = 'l',
    ADBusBigEndian = 'B',
};

ADBUS_API void ADBusSetIteratorEndianness(
        struct ADBusIterator*   iterator,
        enum ADBusEndianness    endianness);

// ----------------------------------------------------------------------------

struct ADBusField
{
    /** The type of field returned.
     *
     * Based on this only the related fields will be initialised.
     */
    enum ADBusFieldType   type;

    uint     b;     /**< boolean data */
    uint8_t  u8;    /**< uint8 data */
    int16_t  i16;   /**< int16 data */
    uint16_t u16;   /**< uint16 data */
    int32_t  i32;   /**< int32 data */
    uint32_t u32;   /**< uint32 data */
    int64_t  i64;   /**< int64 data */
    uint64_t u64;   /**< uint64 data */
    double   d;     /**< double data */

    /** Data pointer for the array begin field.
     *
     * The size is specified in the size field.
     *
     * This can be used in combination with \ref ADBusJumpToEndOfArray with a
     * fixed size data member to reference the array data directly in the
     * buffer.
     *
     * For example (this should really also be checking error codes or using
     * the check functions - \ref ADBusCheckArrayBegin):
     *
     * \code
     * struct ADBusField field;
     * ADBusIterate(iter, &field); // field.type == ADBusArrayBeginField
     * struct my_struct_t* p = (struct my_struct_t*) field->data;
     * size_t count = field->size / sizeof(struct my_struct_t);
     * ADBusJumpToEndOfArray(iter, field.scope);
     * ADBusIterate(iter, &field); // field.type == ADBusArrayEndField
     * \endcode
     */
    const uint8_t*        data;

    /** String pointer for string fields (string, object path, signature) and the
     * type of variants.
     *
     * The string is guarenteed by the dbus protocol and the iterator to be
     * valid UTF8, contain no embedded nulls and have a terminating null.
     */
    const char*           string;

    /** Indicates the size of the data.
     *  This is used for both the string and data fields.
     */
    size_t                size;

    /** The scope of the current field.
     *
     * The scoped begin/end functions all increment this when returning their
     * begin field (eg \ref ADBusBeginArrayField) and decrement this when
     * returning their end field (eg \ref ADBusEndArrayField).
     *
     * The scoped types are:
     * - variant
     * - struct
     * - array
     * - dict entry
     *
     */
    uint                  scope;
};

ADBUS_API uint ADBusIsScopeAtEnd(struct ADBusIterator* i, uint scope);
ADBUS_API int ADBusIterate(struct ADBusIterator* i, struct ADBusField* field);
ADBUS_API int ADBusJumpToEndOfArray(struct ADBusIterator* i, uint scope);

// ----------------------------------------------------------------------------

/* Check functions in the style of the lua check functions (eg luaL_checkint).
 *
 * They will longjmp back to throw an invalid argument
 * error. Thus they should only be used to pull out the arguments at the very
 * beginning of a message callback.
 */

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

/* @} */

