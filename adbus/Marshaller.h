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

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------

typedef void (*ADBusSendCallback)(void*           user,
                                  const uint8_t*  data,
                                  size_t          len);

struct ADBusMarshaller;

ADBUS_API struct ADBusMarshaller* ADBusCreateMarshaller();
ADBUS_API void ADBusFreeMarshaller(struct ADBusMarshaller* m);

ADBUS_API void ADBusClearMarshaller(struct ADBusMarshaller* m);
ADBUS_API void ADBusSendMessage(struct ADBusMarshaller* m, ADBusSendCallback callback, void* user);

ADBUS_API void ADBusSetMessageType(struct ADBusMarshaller* m, enum ADBusMessageType type);
ADBUS_API void ADBusSetSerial(struct ADBusMarshaller* m, uint32_t serial);
ADBUS_API void ADBusSetFlags(struct ADBusMarshaller* m, int flags);

ADBUS_API void ADBusSetReplySerial(struct ADBusMarshaller* m, uint32_t reply);
ADBUS_API void ADBusSetPath(struct ADBusMarshaller* m, const char* path, int size);
ADBUS_API void ADBusSetInterface(struct ADBusMarshaller* m, const char* path, int size);
ADBUS_API void ADBusSetMember(struct ADBusMarshaller* m, const char* path, int size);
ADBUS_API void ADBusSetErrorName(struct ADBusMarshaller* m, const char* path, int size);
ADBUS_API void ADBusSetDestination(struct ADBusMarshaller* m, const char* path, int size);
ADBUS_API void ADBusSetSender(struct ADBusMarshaller* m, const char* path, int size);

ADBUS_API void ADBusSetSignature(struct ADBusMarshaller*, const char* sig, int size);
ADBUS_API const char* ADBusMarshallerCurrentSignature(struct ADBusMarshaller* m);

ADBUS_API void ADBusAppendBoolean(struct ADBusMarshaller* m, uint32_t data);
ADBUS_API void ADBusAppendUInt8(struct ADBusMarshaller* m, uint8_t data);
ADBUS_API void ADBusAppendInt16(struct ADBusMarshaller* m, int16_t data);
ADBUS_API void ADBusAppendUInt16(struct ADBusMarshaller* m, uint16_t data);
ADBUS_API void ADBusAppendInt32(struct ADBusMarshaller* m, int32_t data);
ADBUS_API void ADBusAppendUInt32(struct ADBusMarshaller* m, uint32_t data);
ADBUS_API void ADBusAppendInt64(struct ADBusMarshaller* m, int64_t data);
ADBUS_API void ADBusAppendUInt64(struct ADBusMarshaller* m, uint64_t data);

ADBUS_API void ADBusAppendDouble(struct ADBusMarshaller* m, double data);

ADBUS_API void ADBusAppendString(struct ADBusMarshaller* m, const char* str, int size);
ADBUS_API void ADBusAppendObjectPath(struct ADBusMarshaller* m, const char* str, int size);
ADBUS_API void ADBusAppendSignature(struct ADBusMarshaller* m, const char* str, int size);

ADBUS_API void ADBusBeginArray(struct ADBusMarshaller* m);
ADBUS_API void ADBusEndArray(struct ADBusMarshaller* m);

ADBUS_API void ADBusBeginStruct(struct ADBusMarshaller* m);
ADBUS_API void ADBusEndStruct(struct ADBusMarshaller* m);

ADBUS_API void ADBusBeginDictEntry(struct ADBusMarshaller* m);
ADBUS_API void ADBusEndDictEntry(struct ADBusMarshaller* m);

ADBUS_API void ADBusBeginVariant(struct ADBusMarshaller* m, const char* type, int size);
ADBUS_API void ADBusEndVariant(struct ADBusMarshaller* m);

// ----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
