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

struct DBusStringRef
{
  const char* str;
  size_t      size;
};

struct DBusField
{
  enum DBusFieldType type;
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
    struct DBusStringRef   string;
    struct DBusStringRef   objectPath;
    struct DBusStringRef   signature;
    struct DBusStringRef   variantType;
    size_t          arrayDataSize;
  } data;
};

// ----------------------------------------------------------------------------

struct DBusMessage;

enum DBusMessageType DBusGetMessageType(struct DBusMessage* m);

const char* DBusGetPath(struct DBusMessage* m, int* len);
const char* DBusGetInterface(struct DBusMessage* m, int* len);
const char* DBusGetSender(struct DBusMessage* m, int* len);
const char* DBusGetDestination(struct DBusMessage* m, int* len);
const char* DBusGetMember(struct DBusMessage* m, int* len);
uint32_t    DBusGetSerial(struct DBusMessage* m);
uint32_t    DBusGetReplySerial(struct DBusMessage* m);

const char* DBusGetSignature(struct DBusMessage* m);

unsigned int DBusIsScopeAtEnd(struct DBusMessage* m, unsigned int scope);
int DBusTakeField(struct DBusMessage* m, struct DBusField* field);

int DBusTakeMessageEnd(struct DBusMessage* m);

int DBusTakeUInt8(struct DBusMessage* m, uint8_t* data);

int DBusTakeInt16(struct DBusMessage* m, int16_t* data);
int DBusTakeUInt16(struct DBusMessage* m, uint16_t* data);

int DBusTakeInt32(struct DBusMessage* m, int32_t* data);
int DBusTakeUInt32(struct DBusMessage* m, uint32_t* data);

int DBusTakeInt64(struct DBusMessage* m, int64_t* data);
int DBusTakeUInt64(struct DBusMessage* m, uint64_t* data);

int DBusTakeDouble(struct DBusMessage* m, double* data);

int DBusTakeString(struct DBusMessage* m, const char** str, int* size);
int DBusTakeObjectPath(struct DBusMessage* m, const char** str, int* size);
int DBusTakeSignature(struct DBusMessage* m, const char** str, int* size);

int DBusTakeArrayBegin(struct DBusMessage* m, unsigned int* scope, int* arrayDataSize);
int DBusTakeArrayEnd(struct DBusMessage* m);

int DBusTakeStructBegin(struct DBusMessage* m, unsigned int* scope);
int DBusTakeStructEnd(struct DBusMessage* m);

int DBusTakeDictEntryBegin(struct DBusMessage* m, unsigned int* scope);
int DBusTakeDictEntryEnd(struct DBusMessage* m);

int DBusTakeVariantBegin(struct DBusMessage* m, unsigned int* scope,
                         const char** variantType, int* variantSize);
int DBusTakeVariantEnd(struct DBusMessage* m);

// ----------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif


