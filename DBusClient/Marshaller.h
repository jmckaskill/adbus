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

typedef void (*DBusSendCallback)(void* user, uint8_t* data, size_t len);

struct DBusMarshaller;

struct DBusMarshaller* DBusCreateMarshaller();
void DBusFreeMarshaller(struct DBusMarshaller* m);

void DBusClearMarshaller(struct DBusMarshaller* m);
void DBusSetSendCallback(struct DBusMarshaller* m, DBusSendCallback callback, void* userData);
void DBusSendMessage(struct DBusMarshaller* m);

void DBusSetMessageType(struct DBusMarshaller* m, enum DBusMessageType type);
void DBusSetSerial(struct DBusMarshaller* m, uint32_t serial);
void DBusSetFlags(struct DBusMarshaller* m, int flags);

void DBusSetReplySerial(struct DBusMarshaller* m, uint32_t reply);
void DBusSetPath(struct DBusMarshaller* m, const char* path, int size);
void DBusSetInterface(struct DBusMarshaller* m, const char* path, int size);
void DBusSetMember(struct DBusMarshaller* m, const char* path, int size);
void DBusSetErrorName(struct DBusMarshaller* m, const char* path, int size);
void DBusSetDestination(struct DBusMarshaller* m, const char* path, int size);
void DBusSetSender(struct DBusMarshaller* m, const char* path, int size);

void DBusBeginArgument(struct DBusMarshaller* m, const char* type, int typeSize);
void DBusEndArgument(struct DBusMarshaller* m);

void DBusAppendBoolean(struct DBusMarshaller* m, uint32_t data);
void DBusAppendUInt8(struct DBusMarshaller* m, uint8_t data);
void DBusAppendInt16(struct DBusMarshaller* m, int16_t data);
void DBusAppendUInt16(struct DBusMarshaller* m, uint16_t data);
void DBusAppendInt32(struct DBusMarshaller* m, int32_t data);
void DBusAppendUInt32(struct DBusMarshaller* m, uint32_t data);
void DBusAppendInt64(struct DBusMarshaller* m, int64_t data);
void DBusAppendUInt64(struct DBusMarshaller* m, uint64_t data);

void DBusAppendDouble(struct DBusMarshaller* m, double data);

void DBusAppendString(struct DBusMarshaller* m, const char* str, int size);
void DBusAppendObjectPath(struct DBusMarshaller* m, const char* str, int size);
void DBusAppendSignature(struct DBusMarshaller* m, const char* str, int size);

void DBusBeginArray(struct DBusMarshaller* m);
void DBusEndArray(struct DBusMarshaller* m);

void DBusBeginStruct(struct DBusMarshaller* m);
void DBusEndStruct(struct DBusMarshaller* m);

void DBusBeginDictEntry(struct DBusMarshaller* m);
void DBusEndDictEntry(struct DBusMarshaller* m);

void DBusBeginVariant(struct DBusMarshaller* m, const char* type);
void DBusEndVariant(struct DBusMarshaller* m);

// ----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
