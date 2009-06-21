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

static unsigned int isStackEmpty(struct DBusMessage* m)
{
  return m->stackSize == 0;
}

static size_t stackSize(struct DBusMessage* m)
{
  return m->stackSize;
}

static struct _DBusMessageStackEntry* stackTop(struct DBusMessage* m)
{
  return &m->stack[m->stackSize-1];
}

static struct _DBusMessageStackEntry* pushStackEntry(struct DBusMessage* m)
{
  ALLOC_GROW(struct _DBusMessageStackEntry, m->stack, m->stackSize+1, m->stackAlloc);
  memset(&m->stack[m->stackSize], 0, sizeof(struct _DBusMessageStackEntry));
  m->stackSize++;
  return stackTop(m);
}

void popStackEntry(struct DBusMessage* m)
{
  m->stackSize--;
}

// ----------------------------------------------------------------------------

static size_t dataRemaining(struct DBusMessage* m)
{
  return m->dataEnd - m->data;
}

static uint8_t* getData(struct DBusMessage* m, size_t size)
{
  assert(dataRemaining(m) >= size);
  uint8_t* ret = m->data;
  m->data += size;
  return ret;
}

// ----------------------------------------------------------------------------


static uint8_t get8BitData(struct DBusMessage* m)
{
  return *getData(m, sizeof(uint8_t));
}

static uint16_t get16BitData(struct DBusMessage* m)
{
  uint16_t* data = (uint16_t*)getData(m, sizeof(uint16_t));
  if (!m->nativeEndian)
    *data = ENDIAN_CONVERT16(*data);

  return *data;
}

static uint32_t get32BitData(struct DBusMessage* m)
{
  uint32_t* data = (uint32_t*)getData(m, sizeof(uint32_t));
  if (!m->nativeEndian)
    *data = ENDIAN_CONVERT32(*data);

  return *data;
}

static uint64_t get64BitData(struct DBusMessage* m)
{
  uint64_t* data = (uint64_t*)getData(m, sizeof(uint64_t));
  if (m->nativeEndian)
    *data = ENDIAN_CONVERT64(*data);

  return *data;
}

// ----------------------------------------------------------------------------

static unsigned int isAligned(struct DBusMessage* m)
{
  return (((uintptr_t)(m->data)) % _DBusRequiredAlignment(*m->signature)) == 0;
}

static void processAlignment(struct DBusMessage* m)
{
  uintptr_t data = (uintptr_t) m->data;
  size_t alignment = _DBusRequiredAlignment(*m->signature);
  if (alignment == 0)
    return;
  data = _DBUS_ALIGN_VALUE(data, alignment);
  m->data = (uint8_t*) data;
}

// ----------------------------------------------------------------------------






// ----------------------------------------------------------------------------
// Private API
// ----------------------------------------------------------------------------

int _DBusProcessField(struct DBusMessage* m, struct DBusField* f)
{
  f->type = DBusInvalidField;
  switch(*m->signature)
  {
  case DBusUInt8Field:
    return _DBusProcess8Bit(m,f,(uint8_t*)&f->data.u8);
  case DBusBooleanField:
    return _DBusProcessBoolean(m,f);
  case DBusInt16Field:
    return _DBusProcess16Bit(m,f,(uint16_t*)&f->data.i16);
  case DBusUInt16Field:
    return _DBusProcess16Bit(m,f,(uint16_t*)&f->data.u16);
  case DBusInt32Field:
    return _DBusProcess32Bit(m,f,(uint32_t*)&f->data.i32);
  case DBusUInt32Field:
    return _DBusProcess32Bit(m,f,(uint32_t*)&f->data.u32);
  case DBusInt64Field:
    return _DBusProcess64Bit(m,f,(uint64_t*)&f->data.i64);
  case DBusUInt64Field:
    return _DBusProcess64Bit(m,f,(uint64_t*)&f->data.u64);
  case DBusDoubleField:
    return _DBusProcess64Bit(m,f,(uint64_t*)&f->data.d);
  case DBusStringField:
    return _DBusProcessString(m,f);
  case DBusObjectPathField:
    return _DBusProcessObjectPath(m,f);
  case DBusSignatureField:
    return _DBusProcessSignature(m,f);
  case DBusArrayBeginField:
    return _DBusProcessArray(m,f);
  case DBusStructBeginField:
    return _DBusProcessStruct(m,f);
  case DBusVariantBeginField:
    return _DBusProcessVariant(m,f);
  case DBusDictEntryBeginField:
    return _DBusProcessDictEntry(m,f);
  default:
    return DBusInvalidData;
  }
}

// ----------------------------------------------------------------------------

int _DBusProcess8Bit(struct DBusMessage* m, struct DBusField* f, uint8_t* fieldData)
{
  assert(isAligned(m));

  if (dataRemaining(m) < 1)
    return DBusInvalidData;

  f->type = (enum DBusFieldType)(*m->signature);
  *fieldData = get8BitData(m);

  m->signature += 1;

  return 0;
}

// ----------------------------------------------------------------------------

int _DBusProcess16Bit(struct DBusMessage* m, struct DBusField* f, uint16_t* fieldData)
{
  assert(isAligned(m));

  if (dataRemaining(m) < 2)
    return DBusInvalidData;

  f->type = (enum DBusFieldType)(*m->signature);
  *fieldData = get16BitData(m);

  m->signature += 1;

  return 0;
}

// ----------------------------------------------------------------------------

int _DBusProcess32Bit(struct DBusMessage* m, struct DBusField* f, uint32_t* fieldData)
{
  assert(isAligned(m));

  if (dataRemaining(m) < 4)
    return DBusInvalidData;

  f->type = (enum DBusFieldType)(*m->signature);
  *fieldData = get32BitData(m);

  m->signature += 1;

  return 0;
}

// ----------------------------------------------------------------------------

int _DBusProcess64Bit(struct DBusMessage* m, struct DBusField* f, uint64_t* fieldData)
{
  assert(isAligned(m));

  if (dataRemaining(m) < 8)
    return DBusInvalidData;

  f->type = (enum DBusFieldType)(*m->signature);
  *fieldData = get64BitData(m);

  m->signature += 1;

  return 0;
}

// ----------------------------------------------------------------------------

int _DBusProcessBoolean(struct DBusMessage* m, struct DBusField* f)
{
  int err = _DBusProcess32Bit(m, f, &f->data.b);
  if (err)
    return err;

  if (f->data.b > 1)
    return DBusInvalidData;

  f->type = DBusBooleanField;
  f->data.b = f->data.b ? 1 : 0;

  return 0;
}

// ----------------------------------------------------------------------------

int _DBusProcessStringData(struct DBusMessage* m, struct DBusField* f)
{
  size_t size = f->data.string.size;
  if (dataRemaining(m) < size + 1)
    return DBusInvalidData;

  const char* str = (const char*) getData(m, size + 1);
  if (_DBusHasNullByte(str, size))
    return DBusInvalidData;

  if (*(str + size) != '\0')
    return DBusInvalidData;

  if (!_DBusIsValidUtf8(str, size))
    return DBusInvalidData;

  f->data.string.str = str;

  m->signature += 1;

  return 0;
}

// ----------------------------------------------------------------------------

int _DBusProcessObjectPath(struct DBusMessage* m, struct DBusField* f)
{
  assert(isAligned(m));
  assert(&f->data.objectPath == &f->data.string);
  if (dataRemaining(m) < 4)
    return DBusInvalidData;

  f->type = DBusObjectPathField;
  f->data.objectPath.size = get32BitData(m);

  int err = _DBusProcessStringData(m,f);
  if (err)
    return err;

  if (!_DBusIsValidObjectPath(f->data.objectPath.str, f->data.objectPath.size))
    return DBusInvalidData;

  return 0;
}

// ----------------------------------------------------------------------------

int _DBusProcessString(struct DBusMessage* m, struct DBusField* f)
{
  assert(isAligned(m));
  if (dataRemaining(m) < 4)
    return DBusInvalidData;

  f->type = DBusStringField;
  f->data.string.size = get32BitData(m);

  return _DBusProcessStringData(m,f);
}

// ----------------------------------------------------------------------------

int _DBusProcessSignature(struct DBusMessage* m, struct DBusField* f)
{
  assert(isAligned(m));
  assert(&f->data.signature == &f->data.string);
  if (dataRemaining(m) < 1)
    return DBusInvalidData;

  f->type = DBusSignatureField;
  f->data.signature.size = get8BitData(m);

  return _DBusProcessStringData(m,f);
}

// ----------------------------------------------------------------------------

int _DBusNextField(struct DBusMessage* m, struct DBusField* f)
{
  if (isStackEmpty(m))
  {
    return _DBusNextRootField(m,f);
  }
  else
  {
    switch(stackTop(m)->type)
    {
    case _DBusVariantMessageStack:
      return _DBusNextVariantField(m,f);
    case _DBusDictEntryMessageStack:
      return _DBusNextDictEntryField(m,f);
    case _DBusArrayMessageStack:
      return _DBusNextArrayField(m,f);
    case _DBusStructMessageStack:
      return _DBusNextStructField(m,f);
    default:
      assert(0);
      return DBusInternalError;
    }
  }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int _DBusNextRootField(struct DBusMessage* m, struct DBusField* f)
{
  if (*m->signature == DBusMessageEndField)
    return 0;

  processAlignment(m);
  return _DBusProcessField(m,f);
}

// ----------------------------------------------------------------------------

unsigned int _DBusIsRootAtEnd(struct DBusMessage* m)
{
  return *m->signature == DBusMessageEndField;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int _DBusProcessStruct(struct DBusMessage* m, struct DBusField* f)
{
  assert(isAligned(m));

  if (dataRemaining(m) == 0)
    return DBusInvalidData;

  struct _DBusMessageStackEntry* e = pushStackEntry(m);
  e->type = _DBusStructMessageStack;

  m->signature += 1; // skip over '('

  f->type = DBusStructBeginField;

  return 0;
}

// ----------------------------------------------------------------------------

int _DBusNextStructField(struct DBusMessage* m, struct DBusField* f)
{
  if (*m->signature != ')')
  {
    processAlignment(m);
    return _DBusProcessField(m,f);
  }

  popStackEntry(m);

  m->signature += 1; // skip over ')'

  f->type = DBusStructEndField;

  return 0;
}

// ----------------------------------------------------------------------------

unsigned int _DBusIsStructAtEnd(struct DBusMessage* m)
{
  return *m->signature == ')';
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int _DBusProcessDictEntry(struct DBusMessage* m, struct DBusField* f)
{
  assert(isAligned(m));

  struct _DBusMessageStackEntry* e = pushStackEntry(m);
  e->type = _DBusDictEntryMessageStack;
  e->data.dictEntryFields = 0;

  m->signature += 1; // skip over '{'

  f->type = DBusDictEntryBeginField;

  return 0;
}

// ----------------------------------------------------------------------------

int _DBusNextDictEntryField(struct DBusMessage* m, struct DBusField* f)
{
  struct _DBusMessageStackEntry* e = stackTop(m);
  if (*m->signature != '}')
  {
    processAlignment(m);
    if (++(e->data.dictEntryFields) > 2)
      return DBusInvalidData;
    else
      return _DBusProcessField(m,f);
  }

  popStackEntry(m);

  m->signature += 1; // skip over '}'

  f->type = DBusDictEntryEndField;

  return 0;
}

// ----------------------------------------------------------------------------

unsigned int _DBusIsDictEntryAtEnd(struct DBusMessage* m)
{
  return *m->signature == '}';
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int _DBusProcessArray(struct DBusMessage* m, struct DBusField* f)
{
  assert(isAligned(m));
  if (dataRemaining(m) < 4)
    return DBusInvalidData;
  size_t size = get32BitData(m);

  if (size > DBusMaximumArrayLength || size > dataRemaining(m))
    return DBusInvalidData;

  m->signature += 1; // skip over 'a'

  processAlignment(m);

  struct _DBusMessageStackEntry* e = pushStackEntry(m);
  e->type = _DBusArrayMessageStack;
  e->data.array.dataEnd = m->data + size;
  e->data.array.typeBegin = m->signature;

  f->type = DBusArrayBeginField;
  f->data.arrayDataSize = size;

  return 0;
}

// ----------------------------------------------------------------------------

int _DBusNextArrayField(struct DBusMessage* m, struct DBusField* f)
{
  struct _DBusMessageStackEntry* e = stackTop(m);
  if (m->data > e->data.array.dataEnd)
  {
    return DBusInvalidData;
  }
  else if (m->data < e->data.array.dataEnd)
  {
    m->signature = e->data.array.typeBegin;
    processAlignment(m);
    return _DBusProcessField(m,f);
  }

  f->type = DBusArrayEndField;
  popStackEntry(m);
  return 0;
}

// ----------------------------------------------------------------------------

unsigned int _DBusIsArrayAtEnd(struct DBusMessage* m)
{
  struct _DBusMessageStackEntry* e = stackTop(m);
  return m->data >= e->data.array.dataEnd;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int _DBusProcessVariant(struct DBusMessage* m, struct DBusField* f)
{
  assert(isAligned(m));
  assert(&f->data.variantType == &f->data.signature);
  int err = _DBusProcessSignature(m,f);
  if (err)
    return err;

  // _DBusProcessSignature has alread filled out f->data.variantType
  // and has consumed the field in m->signature

  struct _DBusMessageStackEntry* e = pushStackEntry(m);
  e->type = _DBusVariantMessageStack;
  e->data.variant.oldSignature = m->signature;
  e->data.variant.seenFirst = 0;

  f->type = DBusVariantBeginField;

  m->signature = f->data.variantType.str;

  return 0;
}

// ----------------------------------------------------------------------------

int _DBusNextVariantField(struct DBusMessage* m, struct DBusField* f)
{
  struct _DBusMessageStackEntry* e = stackTop(m);
  if (!e->data.variant.seenFirst)
  {
    e->data.variant.seenFirst = 1;
    processAlignment(m);
    return _DBusProcessField(m,f);
  }
  else if (*m->signature != '\0')
  {
    return DBusInvalidData; // there's more than one root type in the variant type
  }

  m->signature = e->data.variant.oldSignature;

  popStackEntry(m);

  f->type = DBusVariantEndField;

  return 0;
}

// ----------------------------------------------------------------------------

unsigned int _DBusIsVariantAtEnd(struct DBusMessage* m)
{
  return *m->signature == '\0';
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------





// ----------------------------------------------------------------------------
//  Public API
// ----------------------------------------------------------------------------

enum DBusMessageType DBusGetMessageType(struct DBusMessage* m)
{
  return m->messageType;
}

// ----------------------------------------------------------------------------

const char* DBusGetPath(struct DBusMessage* m, int* len)
{
  if (len)
    *len = m->pathSize;
  return m->path;
}

// ----------------------------------------------------------------------------

const char* DBusGetInterface(struct DBusMessage* m, int* len)
{
  if (len)
    *len = m->interfaceSize;
  return m->interface;
}

// ----------------------------------------------------------------------------

const char* DBusGetSender(struct DBusMessage* m, int* len)
{
  if (len)
    *len = m->senderSize;
  return m->sender;
}

// ----------------------------------------------------------------------------

const char* DBusGetDestination(struct DBusMessage* m, int* len)
{
  if (len)
    *len = m->destinationSize;
  return m->destination;
}

// ----------------------------------------------------------------------------

const char* DBusGetMember(struct DBusMessage* m, int* len)
{
  if (len)
    *len = m->memberSize;
  return m->member;
}

// ----------------------------------------------------------------------------

uint32_t DBusGetSerial(struct DBusMessage* m)
{
  return m->serial;
}

// ----------------------------------------------------------------------------

uint32_t DBusGetReplySerial(struct DBusMessage* m)
{
  return m->replySerial;
}

// ----------------------------------------------------------------------------

const char* DBusGetSignature(struct DBusMessage* m)
{
  return m->signature;
}

// ----------------------------------------------------------------------------

unsigned int DBusIsScopeAtEnd(struct DBusMessage* m, unsigned int scope)
{
  if (stackSize(m) < scope)
  {
    assert(0);
    return 1;
  }

  if (stackSize(m) > scope)
    return 0;

  if (isStackEmpty(m))
    return _DBusIsRootAtEnd(m);

  switch(stackTop(m)->type)
  {
  case _DBusVariantMessageStack:
    return _DBusIsVariantAtEnd(m);
  case _DBusDictEntryMessageStack:
    return _DBusIsDictEntryAtEnd(m);
  case _DBusArrayMessageStack:
    return _DBusIsArrayAtEnd(m);
  case _DBusStructMessageStack:
    return _DBusIsStructAtEnd(m);
  default:
    assert(0);
    return 1;
  }
}

// ----------------------------------------------------------------------------

int DBusTakeField(struct DBusMessage* m, struct DBusField* f)
{
  return _DBusNextField(m,f);
}

// ----------------------------------------------------------------------------

static int takeSpecificField(struct DBusMessage* m, struct DBusField* f, char type)
{
  int err = _DBusNextField(m, f);
  if (err)
    return err;

  if (f->type != type)
    return DBusInvalidArgument;

  return 0;
}

// ----------------------------------------------------------------------------

int DBusTakeMessageEnd(struct DBusMessage* m)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusMessageEndField);
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeUInt8(struct DBusMessage* m, uint8_t* data)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusUInt8Field);
  if (data)
    *data = f.data.u8;
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeInt16(struct DBusMessage* m, int16_t* data)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusInt16Field);
  if (data)
    *data = f.data.i16;
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeUInt16(struct DBusMessage* m, uint16_t* data)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusUInt16Field);
  if (data)
    *data = f.data.u16;
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeInt32(struct DBusMessage* m, int32_t* data)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusInt32Field);
  if (data)
    *data = f.data.i32;
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeUInt32(struct DBusMessage* m, uint32_t* data)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusUInt32Field);
  if (data)
    *data = f.data.u32;
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeInt64(struct DBusMessage* m, int64_t* data)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusInt64Field);
  if (data)
    *data = f.data.i64;
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeUInt64(struct DBusMessage* m, uint64_t* data)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusUInt64Field);
  if (data)
    *data = f.data.u64;
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeDouble(struct DBusMessage* m, double* data)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusDoubleField);
  if (data)
    *data = f.data.d;
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeString(struct DBusMessage* m, const char** str, int* size)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusStringField);
  if (str)
    *str = f.data.string.str;
  if (size)
    *size = f.data.string.size;
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeObjectPath(struct DBusMessage* m, const char** str, int* size)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusObjectPathField);
  if (str)
    *str = f.data.objectPath.str;
  if (size)
    *size = f.data.objectPath.size;
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeSignature(struct DBusMessage* m, const char** str, int* size)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusSignatureField);
  if (str)
    *str = f.data.signature.str;
  if (size)
    *size = f.data.signature.size;
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeArrayBegin(struct DBusMessage* m, unsigned int* scope, int* arrayDataSize)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusArrayBeginField);
  if (scope)
    *scope = (unsigned int)stackSize(m);
  if (arrayDataSize)
    *arrayDataSize = f.data.arrayDataSize;
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeArrayEnd(struct DBusMessage* m)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusArrayEndField);
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeStructBegin(struct DBusMessage* m, unsigned int* scope)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusStructBeginField);
  if (scope)
    *scope = (unsigned int)stackSize(m);
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeStructEnd(struct DBusMessage* m)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusStructEndField);
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeDictEntryBegin(struct DBusMessage* m, unsigned int* scope)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusDictEntryBeginField);
  if (scope)
    *scope = (unsigned int)stackSize(m);
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeDictEntryEnd(struct DBusMessage* m)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusDictEntryEndField);
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeVariantBegin(struct DBusMessage* m,
    unsigned int* scope,
    const char** variantType,
    int* variantTypeSize)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusVariantBeginField);
  if (scope)
    *scope = (unsigned int)stackSize(m);
  if (variantType)
    *variantType = f.data.variantType.str;
  if (variantTypeSize)
    *variantTypeSize = f.data.variantType.size;
  return err;
}

// ----------------------------------------------------------------------------

int DBusTakeVariantEnd(struct DBusMessage* m)
{
  struct DBusField f;
  int err = takeSpecificField(m, &f, DBusVariantEndField);
  return err;
}

// ----------------------------------------------------------------------------

