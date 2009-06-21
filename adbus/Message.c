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

#include "Message.h"
#include "Message_p.h"
#include "Misc_p.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


// ----------------------------------------------------------------------------
// Helper Functions
// ----------------------------------------------------------------------------

static unsigned int isStackEmpty(struct ADBusMessage* m)
{
  return m->stackSize == 0;
}

static size_t stackSize(struct ADBusMessage* m)
{
  return m->stackSize;
}

static struct _ADBusMessageStackEntry* stackTop(struct ADBusMessage* m)
{
  return &m->stack[m->stackSize-1];
}

static struct _ADBusMessageStackEntry* pushStackEntry(struct ADBusMessage* m)
{
  ALLOC_GROW(struct _ADBusMessageStackEntry, m->stack, m->stackSize+1, m->stackAlloc);
  memset(&m->stack[m->stackSize], 0, sizeof(struct _ADBusMessageStackEntry));
  m->stackSize++;
  return stackTop(m);
}

void popStackEntry(struct ADBusMessage* m)
{
  m->stackSize--;
}

// ----------------------------------------------------------------------------

static size_t dataRemaining(struct ADBusMessage* m)
{
  return m->dataEnd - m->data;
}

static uint8_t* getData(struct ADBusMessage* m, size_t size)
{
  assert(dataRemaining(m) >= size);
  uint8_t* ret = m->data;
  m->data += size;
  return ret;
}

// ----------------------------------------------------------------------------


static uint8_t get8BitData(struct ADBusMessage* m)
{
  return *getData(m, sizeof(uint8_t));
}

static uint16_t get16BitData(struct ADBusMessage* m)
{
  uint16_t* data = (uint16_t*)getData(m, sizeof(uint16_t));
  if (!m->nativeEndian)
    *data = ENDIAN_CONVERT16(*data);

  return *data;
}

static uint32_t get32BitData(struct ADBusMessage* m)
{
  uint32_t* data = (uint32_t*)getData(m, sizeof(uint32_t));
  if (!m->nativeEndian)
    *data = ENDIAN_CONVERT32(*data);

  return *data;
}

static uint64_t get64BitData(struct ADBusMessage* m)
{
  uint64_t* data = (uint64_t*)getData(m, sizeof(uint64_t));
  if (m->nativeEndian)
    *data = ENDIAN_CONVERT64(*data);

  return *data;
}

// ----------------------------------------------------------------------------

static unsigned int isAligned(struct ADBusMessage* m)
{
  return (((uintptr_t)(m->data)) % _ADBusRequiredAlignment(*m->signature)) == 0;
}

static void processAlignment(struct ADBusMessage* m)
{
  uintptr_t data = (uintptr_t) m->data;
  size_t alignment = _ADBusRequiredAlignment(*m->signature);
  if (alignment == 0)
    return;
  data = _ADBUS_ALIGN_VALUE(data, alignment);
  m->data = (uint8_t*) data;
}

// ----------------------------------------------------------------------------






// ----------------------------------------------------------------------------
// Private API
// ----------------------------------------------------------------------------

int _ADBusProcessField(struct ADBusMessage* m, struct ADBusField* f)
{
  f->type = ADBusInvalidField;
  switch(*m->signature)
  {
  case ADBusUInt8Field:
    return _ADBusProcess8Bit(m,f,(uint8_t*)&f->data.u8);
  case ADBusBooleanField:
    return _ADBusProcessBoolean(m,f);
  case ADBusInt16Field:
    return _ADBusProcess16Bit(m,f,(uint16_t*)&f->data.i16);
  case ADBusUInt16Field:
    return _ADBusProcess16Bit(m,f,(uint16_t*)&f->data.u16);
  case ADBusInt32Field:
    return _ADBusProcess32Bit(m,f,(uint32_t*)&f->data.i32);
  case ADBusUInt32Field:
    return _ADBusProcess32Bit(m,f,(uint32_t*)&f->data.u32);
  case ADBusInt64Field:
    return _ADBusProcess64Bit(m,f,(uint64_t*)&f->data.i64);
  case ADBusUInt64Field:
    return _ADBusProcess64Bit(m,f,(uint64_t*)&f->data.u64);
  case ADBusDoubleField:
    return _ADBusProcess64Bit(m,f,(uint64_t*)&f->data.d);
  case ADBusStringField:
    return _ADBusProcessString(m,f);
  case ADBusObjectPathField:
    return _ADBusProcessObjectPath(m,f);
  case ADBusSignatureField:
    return _ADBusProcessSignature(m,f);
  case ADBusArrayBeginField:
    return _ADBusProcessArray(m,f);
  case ADBusStructBeginField:
    return _ADBusProcessStruct(m,f);
  case ADBusVariantBeginField:
    return _ADBusProcessVariant(m,f);
  case ADBusDictEntryBeginField:
    return _ADBusProcessDictEntry(m,f);
  default:
    return ADBusInvalidData;
  }
}

// ----------------------------------------------------------------------------

int _ADBusProcess8Bit(struct ADBusMessage* m, struct ADBusField* f, uint8_t* fieldData)
{
  assert(isAligned(m));

  if (dataRemaining(m) < 1)
    return ADBusInvalidData;

  f->type = (enum ADBusFieldType)(*m->signature);
  *fieldData = get8BitData(m);

  m->signature += 1;

  return 0;
}

// ----------------------------------------------------------------------------

int _ADBusProcess16Bit(struct ADBusMessage* m, struct ADBusField* f, uint16_t* fieldData)
{
  assert(isAligned(m));

  if (dataRemaining(m) < 2)
    return ADBusInvalidData;

  f->type = (enum ADBusFieldType)(*m->signature);
  *fieldData = get16BitData(m);

  m->signature += 1;

  return 0;
}

// ----------------------------------------------------------------------------

int _ADBusProcess32Bit(struct ADBusMessage* m, struct ADBusField* f, uint32_t* fieldData)
{
  assert(isAligned(m));

  if (dataRemaining(m) < 4)
    return ADBusInvalidData;

  f->type = (enum ADBusFieldType)(*m->signature);
  *fieldData = get32BitData(m);

  m->signature += 1;

  return 0;
}

// ----------------------------------------------------------------------------

int _ADBusProcess64Bit(struct ADBusMessage* m, struct ADBusField* f, uint64_t* fieldData)
{
  assert(isAligned(m));

  if (dataRemaining(m) < 8)
    return ADBusInvalidData;

  f->type = (enum ADBusFieldType)(*m->signature);
  *fieldData = get64BitData(m);

  m->signature += 1;

  return 0;
}

// ----------------------------------------------------------------------------

int _ADBusProcessBoolean(struct ADBusMessage* m, struct ADBusField* f)
{
  int err = _ADBusProcess32Bit(m, f, &f->data.b);
  if (err)
    return err;

  if (f->data.b > 1)
    return ADBusInvalidData;

  f->type = ADBusBooleanField;
  f->data.b = f->data.b ? 1 : 0;

  return 0;
}

// ----------------------------------------------------------------------------

int _ADBusProcessStringData(struct ADBusMessage* m, struct ADBusField* f)
{
  size_t size = f->data.string.size;
  if (dataRemaining(m) < size + 1)
    return ADBusInvalidData;

  const char* str = (const char*) getData(m, size + 1);
  if (_ADBusHasNullByte(str, size))
    return ADBusInvalidData;

  if (*(str + size) != '\0')
    return ADBusInvalidData;

  if (!_ADBusIsValidUtf8(str, size))
    return ADBusInvalidData;

  f->data.string.str = str;

  m->signature += 1;

  return 0;
}

// ----------------------------------------------------------------------------

int _ADBusProcessObjectPath(struct ADBusMessage* m, struct ADBusField* f)
{
  assert(isAligned(m));
  assert(&f->data.objectPath == &f->data.string);
  if (dataRemaining(m) < 4)
    return ADBusInvalidData;

  f->type = ADBusObjectPathField;
  f->data.objectPath.size = get32BitData(m);

  int err = _ADBusProcessStringData(m,f);
  if (err)
    return err;

  if (!_ADBusIsValidObjectPath(f->data.objectPath.str, f->data.objectPath.size))
    return ADBusInvalidData;

  return 0;
}

// ----------------------------------------------------------------------------

int _ADBusProcessString(struct ADBusMessage* m, struct ADBusField* f)
{
  assert(isAligned(m));
  if (dataRemaining(m) < 4)
    return ADBusInvalidData;

  f->type = ADBusStringField;
  f->data.string.size = get32BitData(m);

  return _ADBusProcessStringData(m,f);
}

// ----------------------------------------------------------------------------

int _ADBusProcessSignature(struct ADBusMessage* m, struct ADBusField* f)
{
  assert(isAligned(m));
  assert(&f->data.signature == &f->data.string);
  if (dataRemaining(m) < 1)
    return ADBusInvalidData;

  f->type = ADBusSignatureField;
  f->data.signature.size = get8BitData(m);

  return _ADBusProcessStringData(m,f);
}

// ----------------------------------------------------------------------------

int _ADBusNextField(struct ADBusMessage* m, struct ADBusField* f)
{
  if (isStackEmpty(m))
  {
    return _ADBusNextRootField(m,f);
  }
  else
  {
    switch(stackTop(m)->type)
    {
    case _ADBusVariantMessageStack:
      return _ADBusNextVariantField(m,f);
    case _ADBusDictEntryMessageStack:
      return _ADBusNextDictEntryField(m,f);
    case _ADBusArrayMessageStack:
      return _ADBusNextArrayField(m,f);
    case _ADBusStructMessageStack:
      return _ADBusNextStructField(m,f);
    default:
      assert(0);
      return ADBusInternalError;
    }
  }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int _ADBusNextRootField(struct ADBusMessage* m, struct ADBusField* f)
{
  if (*m->signature == ADBusMessageEndField)
    return 0;

  processAlignment(m);
  return _ADBusProcessField(m,f);
}

// ----------------------------------------------------------------------------

unsigned int _ADBusIsRootAtEnd(struct ADBusMessage* m)
{
  return *m->signature == ADBusMessageEndField;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int _ADBusProcessStruct(struct ADBusMessage* m, struct ADBusField* f)
{
  assert(isAligned(m));

  if (dataRemaining(m) == 0)
    return ADBusInvalidData;

  struct _ADBusMessageStackEntry* e = pushStackEntry(m);
  e->type = _ADBusStructMessageStack;

  m->signature += 1; // skip over '('

  f->type = ADBusStructBeginField;

  return 0;
}

// ----------------------------------------------------------------------------

int _ADBusNextStructField(struct ADBusMessage* m, struct ADBusField* f)
{
  if (*m->signature != ')')
  {
    processAlignment(m);
    return _ADBusProcessField(m,f);
  }

  popStackEntry(m);

  m->signature += 1; // skip over ')'

  f->type = ADBusStructEndField;

  return 0;
}

// ----------------------------------------------------------------------------

unsigned int _ADBusIsStructAtEnd(struct ADBusMessage* m)
{
  return *m->signature == ')';
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int _ADBusProcessDictEntry(struct ADBusMessage* m, struct ADBusField* f)
{
  assert(isAligned(m));

  struct _ADBusMessageStackEntry* e = pushStackEntry(m);
  e->type = _ADBusDictEntryMessageStack;
  e->data.dictEntryFields = 0;

  m->signature += 1; // skip over '{'

  f->type = ADBusDictEntryBeginField;

  return 0;
}

// ----------------------------------------------------------------------------

int _ADBusNextDictEntryField(struct ADBusMessage* m, struct ADBusField* f)
{
  struct _ADBusMessageStackEntry* e = stackTop(m);
  if (*m->signature != '}')
  {
    processAlignment(m);
    if (++(e->data.dictEntryFields) > 2)
      return ADBusInvalidData;
    else
      return _ADBusProcessField(m,f);
  }

  popStackEntry(m);

  m->signature += 1; // skip over '}'

  f->type = ADBusDictEntryEndField;

  return 0;
}

// ----------------------------------------------------------------------------

unsigned int _ADBusIsDictEntryAtEnd(struct ADBusMessage* m)
{
  return *m->signature == '}';
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int _ADBusProcessArray(struct ADBusMessage* m, struct ADBusField* f)
{
  assert(isAligned(m));
  if (dataRemaining(m) < 4)
    return ADBusInvalidData;
  size_t size = get32BitData(m);

  if (size > ADBusMaximumArrayLength || size > dataRemaining(m))
    return ADBusInvalidData;

  m->signature += 1; // skip over 'a'

  processAlignment(m);

  struct _ADBusMessageStackEntry* e = pushStackEntry(m);
  e->type = _ADBusArrayMessageStack;
  e->data.array.dataEnd = m->data + size;
  e->data.array.typeBegin = m->signature;

  f->type = ADBusArrayBeginField;
  f->data.arrayDataSize = size;

  return 0;
}

// ----------------------------------------------------------------------------

int _ADBusNextArrayField(struct ADBusMessage* m, struct ADBusField* f)
{
  struct _ADBusMessageStackEntry* e = stackTop(m);
  if (m->data > e->data.array.dataEnd)
  {
    return ADBusInvalidData;
  }
  else if (m->data < e->data.array.dataEnd)
  {
    m->signature = e->data.array.typeBegin;
    processAlignment(m);
    return _ADBusProcessField(m,f);
  }

  f->type = ADBusArrayEndField;
  popStackEntry(m);
  return 0;
}

// ----------------------------------------------------------------------------

unsigned int _ADBusIsArrayAtEnd(struct ADBusMessage* m)
{
  struct _ADBusMessageStackEntry* e = stackTop(m);
  return m->data >= e->data.array.dataEnd;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int _ADBusProcessVariant(struct ADBusMessage* m, struct ADBusField* f)
{
  assert(isAligned(m));
  assert(&f->data.variantType == &f->data.signature);
  int err = _ADBusProcessSignature(m,f);
  if (err)
    return err;

  // _ADBusProcessSignature has alread filled out f->data.variantType
  // and has consumed the field in m->signature

  struct _ADBusMessageStackEntry* e = pushStackEntry(m);
  e->type = _ADBusVariantMessageStack;
  e->data.variant.oldSignature = m->signature;
  e->data.variant.seenFirst = 0;

  f->type = ADBusVariantBeginField;

  m->signature = f->data.variantType.str;

  return 0;
}

// ----------------------------------------------------------------------------

int _ADBusNextVariantField(struct ADBusMessage* m, struct ADBusField* f)
{
  struct _ADBusMessageStackEntry* e = stackTop(m);
  if (!e->data.variant.seenFirst)
  {
    e->data.variant.seenFirst = 1;
    processAlignment(m);
    return _ADBusProcessField(m,f);
  }
  else if (*m->signature != '\0')
  {
    return ADBusInvalidData; // there's more than one root type in the variant type
  }

  m->signature = e->data.variant.oldSignature;

  popStackEntry(m);

  f->type = ADBusVariantEndField;

  return 0;
}

// ----------------------------------------------------------------------------

unsigned int _ADBusIsVariantAtEnd(struct ADBusMessage* m)
{
  return *m->signature == '\0';
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------





// ----------------------------------------------------------------------------
//  Public API
// ----------------------------------------------------------------------------

enum ADBusMessageType ADBusGetMessageType(struct ADBusMessage* m)
{
  return m->messageType;
}

// ----------------------------------------------------------------------------

const char* ADBusGetPath(struct ADBusMessage* m, int* len)
{
  if (len)
    *len = m->pathSize;
  return m->path;
}

// ----------------------------------------------------------------------------

const char* ADBusGetInterface(struct ADBusMessage* m, int* len)
{
  if (len)
    *len = m->interfaceSize;
  return m->interface;
}

// ----------------------------------------------------------------------------

const char* ADBusGetSender(struct ADBusMessage* m, int* len)
{
  if (len)
    *len = m->senderSize;
  return m->sender;
}

// ----------------------------------------------------------------------------

const char* ADBusGetDestination(struct ADBusMessage* m, int* len)
{
  if (len)
    *len = m->destinationSize;
  return m->destination;
}

// ----------------------------------------------------------------------------

const char* ADBusGetMember(struct ADBusMessage* m, int* len)
{
  if (len)
    *len = m->memberSize;
  return m->member;
}

// ----------------------------------------------------------------------------

uint32_t ADBusGetSerial(struct ADBusMessage* m)
{
  return m->serial;
}

// ----------------------------------------------------------------------------

uint32_t ADBusGetReplySerial(struct ADBusMessage* m)
{
  return m->replySerial;
}

// ----------------------------------------------------------------------------

const char* ADBusGetSignature(struct ADBusMessage* m)
{
  return m->signature;
}

// ----------------------------------------------------------------------------

unsigned int ADBusIsScopeAtEnd(struct ADBusMessage* m, unsigned int scope)
{
  if (stackSize(m) < scope)
  {
    assert(0);
    return 1;
  }

  if (stackSize(m) > scope)
    return 0;

  if (isStackEmpty(m))
    return _ADBusIsRootAtEnd(m);

  switch(stackTop(m)->type)
  {
  case _ADBusVariantMessageStack:
    return _ADBusIsVariantAtEnd(m);
  case _ADBusDictEntryMessageStack:
    return _ADBusIsDictEntryAtEnd(m);
  case _ADBusArrayMessageStack:
    return _ADBusIsArrayAtEnd(m);
  case _ADBusStructMessageStack:
    return _ADBusIsStructAtEnd(m);
  default:
    assert(0);
    return 1;
  }
}

// ----------------------------------------------------------------------------

int ADBusTakeField(struct ADBusMessage* m, struct ADBusField* f)
{
  return _ADBusNextField(m,f);
}

// ----------------------------------------------------------------------------

static int takeSpecificField(struct ADBusMessage* m, struct ADBusField* f, char type)
{
  int err = _ADBusNextField(m, f);
  if (err)
    return err;

  if (f->type != type)
    return ADBusInvalidArgument;

  return 0;
}

// ----------------------------------------------------------------------------

int ADBusTakeMessageEnd(struct ADBusMessage* m)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusMessageEndField);
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeUInt8(struct ADBusMessage* m, uint8_t* data)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusUInt8Field);
  if (data)
    *data = f.data.u8;
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeInt16(struct ADBusMessage* m, int16_t* data)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusInt16Field);
  if (data)
    *data = f.data.i16;
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeUInt16(struct ADBusMessage* m, uint16_t* data)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusUInt16Field);
  if (data)
    *data = f.data.u16;
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeInt32(struct ADBusMessage* m, int32_t* data)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusInt32Field);
  if (data)
    *data = f.data.i32;
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeUInt32(struct ADBusMessage* m, uint32_t* data)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusUInt32Field);
  if (data)
    *data = f.data.u32;
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeInt64(struct ADBusMessage* m, int64_t* data)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusInt64Field);
  if (data)
    *data = f.data.i64;
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeUInt64(struct ADBusMessage* m, uint64_t* data)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusUInt64Field);
  if (data)
    *data = f.data.u64;
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeDouble(struct ADBusMessage* m, double* data)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusDoubleField);
  if (data)
    *data = f.data.d;
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeString(struct ADBusMessage* m, const char** str, int* size)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusStringField);
  if (str)
    *str = f.data.string.str;
  if (size)
    *size = f.data.string.size;
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeObjectPath(struct ADBusMessage* m, const char** str, int* size)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusObjectPathField);
  if (str)
    *str = f.data.objectPath.str;
  if (size)
    *size = f.data.objectPath.size;
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeSignature(struct ADBusMessage* m, const char** str, int* size)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusSignatureField);
  if (str)
    *str = f.data.signature.str;
  if (size)
    *size = f.data.signature.size;
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeArrayBegin(struct ADBusMessage* m, unsigned int* scope, int* arrayDataSize)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusArrayBeginField);
  if (scope)
    *scope = (unsigned int)stackSize(m);
  if (arrayDataSize)
    *arrayDataSize = f.data.arrayDataSize;
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeArrayEnd(struct ADBusMessage* m)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusArrayEndField);
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeStructBegin(struct ADBusMessage* m, unsigned int* scope)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusStructBeginField);
  if (scope)
    *scope = (unsigned int)stackSize(m);
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeStructEnd(struct ADBusMessage* m)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusStructEndField);
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeDictEntryBegin(struct ADBusMessage* m, unsigned int* scope)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusDictEntryBeginField);
  if (scope)
    *scope = (unsigned int)stackSize(m);
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeDictEntryEnd(struct ADBusMessage* m)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusDictEntryEndField);
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeVariantBegin(struct ADBusMessage* m,
    unsigned int* scope,
    const char** variantType,
    int* variantTypeSize)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusVariantBeginField);
  if (scope)
    *scope = (unsigned int)stackSize(m);
  if (variantType)
    *variantType = f.data.variantType.str;
  if (variantTypeSize)
    *variantTypeSize = f.data.variantType.size;
  return err;
}

// ----------------------------------------------------------------------------

int ADBusTakeVariantEnd(struct ADBusMessage* m)
{
  struct ADBusField f;
  int err = takeSpecificField(m, &f, ADBusVariantEndField);
  return err;
}

// ----------------------------------------------------------------------------

