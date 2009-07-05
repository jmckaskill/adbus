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


enum _ADBusMessageStackEntryType
{
  _ADBusInvalidMessageStack,
  _ADBusVariantMessageStack,
  _ADBusDictEntryMessageStack,
  _ADBusArrayMessageStack,
  _ADBusStructMessageStack,
};

struct _ADBusMessageArrayStackData
{
  const char* typeBegin;
  uint8_t* dataEnd;
};

struct _ADBusMessageVariantStackData
{
  const char* oldSignature;
  unsigned int seenFirst;
};

struct _ADBusMessageStackEntry
{
  enum _ADBusMessageStackEntryType type;
  union
  {
    struct _ADBusMessageArrayStackData  array;
    struct _ADBusMessageVariantStackData variant;
    size_t      dictEntryFields;
  } data;
};

struct ADBusMessage
{
  uint8_t*    origData;
  uint8_t*    origDataEnd;
  const char* origSignature;

  uint8_t*    data;
  uint8_t*    dataEnd;
  const char* signature;

  // Base header
  uint            nativeEndian;
  enum ADBusMessageType messageType;
  uint32_t        serial;

  // Header fields
  uint32_t    replySerial;
  uint        hasReplySerial;

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
  struct _ADBusMessageStackEntry* stack;
  size_t      stackSize;
  size_t      stackAlloc;
};

int _ADBusProcessField(struct ADBusMessage* m, struct ADBusField* f);

int _ADBusProcess8Bit(struct ADBusMessage* m, struct ADBusField* f, uint8_t* fieldData);
int _ADBusProcess16Bit(struct ADBusMessage* m, struct ADBusField* f, uint16_t* fieldData);
int _ADBusProcess32Bit(struct ADBusMessage* m, struct ADBusField* f, uint32_t* fieldData);
int _ADBusProcess64Bit(struct ADBusMessage* m, struct ADBusField* f, uint64_t* fieldData);
int _ADBusProcessBoolean(struct ADBusMessage* m, struct ADBusField* f);

int _ADBusProcessStringData(struct ADBusMessage* m, struct ADBusField* f);
int _ADBusProcessObjectPath(struct ADBusMessage* m, struct ADBusField* f);
int _ADBusProcessString(struct ADBusMessage* m, struct ADBusField* f);
int _ADBusProcessSignature(struct ADBusMessage* m, struct ADBusField* f);

int _ADBusNextRootField(struct ADBusMessage* m, struct ADBusField* f);
uint _ADBusIsRootAtEnd(struct ADBusMessage* m);

int _ADBusProcessStruct(struct ADBusMessage* m, struct ADBusField* f);
int _ADBusNextStructField(struct ADBusMessage* m, struct ADBusField* f);
uint _ADBusIsStructAtEnd(struct ADBusMessage* m);

int _ADBusProcessDictEntry(struct ADBusMessage* m, struct ADBusField* f);
int _ADBusNextDictEntryField(struct ADBusMessage* m, struct ADBusField* f);
uint _ADBusIsDictEntryAtEnd(struct ADBusMessage* m);

int _ADBusProcessArray(struct ADBusMessage* m, struct ADBusField* f);
int _ADBusNextArrayField(struct ADBusMessage* m, struct ADBusField* f);
uint _ADBusIsArrayAtEnd(struct ADBusMessage* m);

int _ADBusProcessVariant(struct ADBusMessage* m, struct ADBusField* f);
int _ADBusNextVariantField(struct ADBusMessage* m, struct ADBusField* f);
uint _ADBusIsVariantAtEnd(struct ADBusMessage* m);

int _ADBusNextField(struct ADBusMessage* m, struct ADBusField* f);


// where b0 _ADBusIs lowest byte
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
