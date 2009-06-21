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

#include "Message.h"


enum _DBusMessageStackEntryType
{
  _DBusInvalidMessageStack,
  _DBusVariantMessageStack,
  _DBusDictEntryMessageStack,
  _DBusArrayMessageStack,
  _DBusStructMessageStack,
};

struct _DBusMessageArrayStackData
{
  const char* typeBegin;
  uint8_t* dataEnd;
};

struct _DBusMessageVariantStackData
{
  const char* oldSignature;
  unsigned int seenFirst;
};

struct _DBusMessageStackEntry
{
  enum _DBusMessageStackEntryType type;
  union
  {
    struct _DBusMessageArrayStackData  array;
    struct _DBusMessageVariantStackData variant;
    size_t      dictEntryFields;
  } data;
};

struct DBusMessage
{
  uint8_t*    data;
  uint8_t*    dataEnd;
  const char* signature;

  // Base header
  unsigned int            nativeEndian;
  enum DBusMessageType messageType;
  uint32_t        serial;

  // Header fields
  uint32_t    replySerial;
  unsigned int        haveReplySerial;

  const char* path;
  int         pathSize;

  const char* interface;
  int         interfaceSize;

  const char* member;
  int         memberSize;

  const char* errorName;
  int         errorNameSize;

  const char* destination;
  int         destinationSize;

  const char* sender;
  int         senderSize;

  // Stack
  struct _DBusMessageStackEntry* stack;
  size_t      stackSize;
  size_t      stackAlloc;
};

int _DBusProcessField(struct DBusMessage* m, struct DBusField* f);

int _DBusProcess8Bit(struct DBusMessage* m, struct DBusField* f, uint8_t* fieldData);
int _DBusProcess16Bit(struct DBusMessage* m, struct DBusField* f, uint16_t* fieldData);
int _DBusProcess32Bit(struct DBusMessage* m, struct DBusField* f, uint32_t* fieldData);
int _DBusProcess64Bit(struct DBusMessage* m, struct DBusField* f, uint64_t* fieldData);
int _DBusProcessBoolean(struct DBusMessage* m, struct DBusField* f);

int _DBusProcessStringData(struct DBusMessage* m, struct DBusField* f);
int _DBusProcessObjectPath(struct DBusMessage* m, struct DBusField* f);
int _DBusProcessString(struct DBusMessage* m, struct DBusField* f);
int _DBusProcessSignature(struct DBusMessage* m, struct DBusField* f);

int _DBusNextRootField(struct DBusMessage* m, struct DBusField* f);
unsigned int _DBusIsRootAtEnd(struct DBusMessage* m);

int _DBusProcessStruct(struct DBusMessage* m, struct DBusField* f);
int _DBusNextStructField(struct DBusMessage* m, struct DBusField* f);
unsigned int _DBusIsStructAtEnd(struct DBusMessage* m);

int _DBusProcessDictEntry(struct DBusMessage* m, struct DBusField* f);
int _DBusNextDictEntryField(struct DBusMessage* m, struct DBusField* f);
unsigned int _DBusIsDictEntryAtEnd(struct DBusMessage* m);

int _DBusProcessArray(struct DBusMessage* m, struct DBusField* f);
int _DBusNextArrayField(struct DBusMessage* m, struct DBusField* f);
unsigned int _DBusIsArrayAtEnd(struct DBusMessage* m);

int _DBusProcessVariant(struct DBusMessage* m, struct DBusField* f);
int _DBusNextVariantField(struct DBusMessage* m, struct DBusField* f);
unsigned int _DBusIsVariantAtEnd(struct DBusMessage* m);

int _DBusNextField(struct DBusMessage* m, struct DBusField* f);


// where b0 _DBusIs lowest byte
#define MAKE16(b1,b0) \
  (((uint16_t)(b1) << 8) | (b0))
#define MAKE32(b3,b2,b1,b0) \
  (((uint32_t)(MAKE16((b3),(b2))) << 16) | MAKE16((b1),(b0)))
#define MAKE64(b7,b6,b5,b4,b3,b2,b1,b0)  \
  (((uint64_t)(MAKE32((b7),(b6),(b5),(b4))) << 32) | MAKE32((b3),(b2),(b1),(b0)))

#define ENDIAN_CONVERT16(val) MAKE16( (val)        & 0xFF, ((val) >> 8)  & 0xFF)

#define ENDIAN_CONVERT32(val) MAKE32( (val)        & 0xFF, ((val) >> 8)  & 0xFF,\
                                     ((val) >> 16) & 0xFF, ((val) >> 24) & 0xFF)

#define ENDIAN_CONVERT64(val) MAKE64( (val)        & 0xFF, ((val) >> 8)  & 0xFF,\
                                     ((val) >> 16) & 0xFF, ((val) >> 24) & 0xFF,\
                                     ((val) >> 32) & 0xFF, ((val) >> 40) & 0xFF,\
                                     ((val) >> 48) & 0xFF, ((val) >> 56) & 0xFF)
