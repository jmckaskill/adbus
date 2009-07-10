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

struct ADBusStringRef
{
  const char* str;
  size_t      size;
};

struct ADBusField
{
  enum ADBusFieldType type;
  union
  {
    uint8_t         u8;
    uint32_t        b;
    int16_t         i16;
    uint16_t        u16;
    int32_t         i32;
    uint32_t        u32;
    int64_t         i64;
    uint64_t        u64;
    double          d;
    struct ADBusStringRef   string;
    struct ADBusStringRef   objectPath;
    struct ADBusStringRef   signature;
    struct ADBusStringRef   variantType;
    size_t          arrayDataSize;
  } data;
  uint scope;
};

// ----------------------------------------------------------------------------

struct ADBusMessage;

ADBUS_API void ADBusReparseMessage(struct ADBusMessage* m);

ADBUS_API enum ADBusMessageType ADBusGetMessageType(struct ADBusMessage* m);

ADBUS_API uint32_t    ADBusGetFlags(struct ADBusMessage* m);
ADBUS_API const char* ADBusGetPath(struct ADBusMessage* m, int* len);
ADBUS_API const char* ADBusGetInterface(struct ADBusMessage* m, int* len);
ADBUS_API const char* ADBusGetSender(struct ADBusMessage* m, int* len);
ADBUS_API const char* ADBusGetDestination(struct ADBusMessage* m, int* len);
ADBUS_API const char* ADBusGetMember(struct ADBusMessage* m, int* len);
ADBUS_API const char* ADBusGetErrorName(struct ADBusMessage* m, int* len);
ADBUS_API uint32_t    ADBusGetSerial(struct ADBusMessage* m);
ADBUS_API uint        ADBusHasReplySerial(struct ADBusMessage* m);
ADBUS_API uint32_t    ADBusGetReplySerial(struct ADBusMessage* m);

ADBUS_API const char* ADBusGetSignatureRemaining(struct ADBusMessage* m);

ADBUS_API uint ADBusIsScopeAtEnd(struct ADBusMessage* m, uint scope);
ADBUS_API int ADBusTakeField(struct ADBusMessage* m, struct ADBusField* field);

ADBUS_API int ADBusTakeMessageEnd(struct ADBusMessage* m);

ADBUS_API int ADBusTakeUInt8(struct ADBusMessage* m, uint8_t* data);

ADBUS_API int ADBusTakeInt16(struct ADBusMessage* m, int16_t* data);
ADBUS_API int ADBusTakeUInt16(struct ADBusMessage* m, uint16_t* data);

ADBUS_API int ADBusTakeInt32(struct ADBusMessage* m, int32_t* data);
ADBUS_API int ADBusTakeUInt32(struct ADBusMessage* m, uint32_t* data);

ADBUS_API int ADBusTakeInt64(struct ADBusMessage* m, int64_t* data);
ADBUS_API int ADBusTakeUInt64(struct ADBusMessage* m, uint64_t* data);

ADBUS_API int ADBusTakeDouble(struct ADBusMessage* m, double* data);

ADBUS_API int ADBusTakeString(struct ADBusMessage* m, const char** str, int* size);
ADBUS_API int ADBusTakeObjectPath(struct ADBusMessage* m, const char** str, int* size);
ADBUS_API int ADBusTakeSignature(struct ADBusMessage* m, const char** str, int* size);

ADBUS_API int ADBusTakeArrayBegin(struct ADBusMessage* m, uint* scope, int* arrayDataSize);
ADBUS_API int ADBusTakeArrayEnd(struct ADBusMessage* m);

ADBUS_API int ADBusTakeStructBegin(struct ADBusMessage* m, uint* scope);
ADBUS_API int ADBusTakeStructEnd(struct ADBusMessage* m);

ADBUS_API int ADBusTakeDictEntryBegin(struct ADBusMessage* m, uint* scope);
ADBUS_API int ADBusTakeDictEntryEnd(struct ADBusMessage* m);

ADBUS_API int ADBusTakeVariantBegin(struct ADBusMessage* m, uint* scope,
                                    const char** variantType, int* variantSize);
ADBUS_API int ADBusTakeVariantEnd(struct ADBusMessage* m);

// ----------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif


