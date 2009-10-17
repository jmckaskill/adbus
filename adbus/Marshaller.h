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

struct ADBusMarshaller;
// Marshalls arguments into a dbus marshalled block and signature
// The marshaller owns the data internally, but it can be copied out via
// GetMarshalledData to another owner
//
// The various append functions can return an error code if they are used
// incorrectly eg. appending an array of uint8_t where the first item is a
// uint8_t but the second is a uint16_t or where the append does not match the
// signature
//
// The signature must be set before appending any data. This is because
// certain cases have a signature but no data to figure out the sig from eg
// an empty list
//
// All functions taking a string take both a const char* and a size. The size
// can be set to the actual string length or -1 at which point strlen is used
// to find the string length.

ADBUS_API struct ADBusMarshaller* ADBusCreateMarshaller();
ADBUS_API void ADBusFreeMarshaller(struct ADBusMarshaller* m);

ADBUS_API void ADBusResetMarshaller(struct ADBusMarshaller* m);

// Data and string is valid until the marshaller is reset, freed, or updated
ADBUS_API void ADBusGetMarshalledData(
        const struct ADBusMarshaller* m,
        const char**            sig,
        size_t*                 sigsize,
        const uint8_t**         data,
        size_t*                 datasize);

ADBUS_API void ADBusSetMarshalledData(
        struct ADBusMarshaller* m,
        const char*             sig,
        int                     sigsize,
        const uint8_t*          data,
        size_t                  datasize);

ADBUS_API enum ADBusFieldType ADBusNextMarshallerField(struct ADBusMarshaller* m);

ADBUS_API int ADBusAppendData(struct ADBusMarshaller* m, const uint8_t* data, size_t size);

ADBUS_API int ADBusAppendBoolean(struct ADBusMarshaller* m, uint32_t data);
ADBUS_API int ADBusAppendUInt8(struct ADBusMarshaller* m, uint8_t data);
ADBUS_API int ADBusAppendInt16(struct ADBusMarshaller* m, int16_t data);
ADBUS_API int ADBusAppendUInt16(struct ADBusMarshaller* m, uint16_t data);
ADBUS_API int ADBusAppendInt32(struct ADBusMarshaller* m, int32_t data);
ADBUS_API int ADBusAppendUInt32(struct ADBusMarshaller* m, uint32_t data);
ADBUS_API int ADBusAppendInt64(struct ADBusMarshaller* m, int64_t data);
ADBUS_API int ADBusAppendUInt64(struct ADBusMarshaller* m, uint64_t data);

ADBUS_API int ADBusAppendDouble(struct ADBusMarshaller* m, double data);

ADBUS_API int ADBusAppendString(struct ADBusMarshaller* m, const char* str, int size);
ADBUS_API int ADBusAppendObjectPath(struct ADBusMarshaller* m, const char* str, int size);
ADBUS_API int ADBusAppendSignature(struct ADBusMarshaller* m, const char* str, int size);

ADBUS_API int ADBusBeginArgument(struct ADBusMarshaller* m, const char* sig, int size);
ADBUS_API int ADBusEndArgument(struct ADBusMarshaller* m);

ADBUS_API int ADBusBeginArray(struct ADBusMarshaller* m);
ADBUS_API int ADBusEndArray(struct ADBusMarshaller* m);

ADBUS_API int ADBusBeginStruct(struct ADBusMarshaller* m);
ADBUS_API int ADBusEndStruct(struct ADBusMarshaller* m);

ADBUS_API int ADBusBeginDictEntry(struct ADBusMarshaller* m);
ADBUS_API int ADBusEndDictEntry(struct ADBusMarshaller* m);

ADBUS_API int ADBusBeginVariant(struct ADBusMarshaller* m, const char* sig, int size);
ADBUS_API int ADBusEndVariant(struct ADBusMarshaller* m);

// ----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
