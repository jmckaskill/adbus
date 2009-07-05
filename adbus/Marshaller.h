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

struct ADBusMarshaller* ADBusCreateMarshaller();
void ADBusFreeMarshaller(struct ADBusMarshaller* m);

void ADBusClearMarshaller(struct ADBusMarshaller* m);
void ADBusSendMessage(struct ADBusMarshaller* m, ADBusSendCallback callback, void* user);

void ADBusSetMessageType(struct ADBusMarshaller* m, enum ADBusMessageType type);
void ADBusSetSerial(struct ADBusMarshaller* m, uint32_t serial);
void ADBusSetFlags(struct ADBusMarshaller* m, int flags);

void ADBusSetReplySerial(struct ADBusMarshaller* m, uint32_t reply);
void ADBusSetPath(struct ADBusMarshaller* m, const char* path, int size);
void ADBusSetInterface(struct ADBusMarshaller* m, const char* path, int size);
void ADBusSetMember(struct ADBusMarshaller* m, const char* path, int size);
void ADBusSetErrorName(struct ADBusMarshaller* m, const char* path, int size);
void ADBusSetDestination(struct ADBusMarshaller* m, const char* path, int size);
void ADBusSetSender(struct ADBusMarshaller* m, const char* path, int size);

void ADBusSetSignature(struct ADBusMarshaller*, const char* sig, int size);
const char* ADBusMarshallerCurrentSignature(struct ADBusMarshaller* m);

void ADBusAppendBoolean(struct ADBusMarshaller* m, uint32_t data);
void ADBusAppendUInt8(struct ADBusMarshaller* m, uint8_t data);
void ADBusAppendInt16(struct ADBusMarshaller* m, int16_t data);
void ADBusAppendUInt16(struct ADBusMarshaller* m, uint16_t data);
void ADBusAppendInt32(struct ADBusMarshaller* m, int32_t data);
void ADBusAppendUInt32(struct ADBusMarshaller* m, uint32_t data);
void ADBusAppendInt64(struct ADBusMarshaller* m, int64_t data);
void ADBusAppendUInt64(struct ADBusMarshaller* m, uint64_t data);

void ADBusAppendDouble(struct ADBusMarshaller* m, double data);

void ADBusAppendString(struct ADBusMarshaller* m, const char* str, int size);
void ADBusAppendObjectPath(struct ADBusMarshaller* m, const char* str, int size);
void ADBusAppendSignature(struct ADBusMarshaller* m, const char* str, int size);

void ADBusBeginArray(struct ADBusMarshaller* m);
void ADBusEndArray(struct ADBusMarshaller* m);

void ADBusBeginStruct(struct ADBusMarshaller* m);
void ADBusEndStruct(struct ADBusMarshaller* m);

void ADBusBeginDictEntry(struct ADBusMarshaller* m);
void ADBusEndDictEntry(struct ADBusMarshaller* m);

void ADBusBeginVariant(struct ADBusMarshaller* m, const char* type, int size);
void ADBusEndVariant(struct ADBusMarshaller* m);

// ----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
