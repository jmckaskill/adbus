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

#include "Marshaller.h"

#include "Misc_p.h"

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>




// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

enum StackEntryType
{
  ArrayStackEntry,
  StructStackEntry,
  VariantStackEntry,
  DictEntryStackEntry,
};

struct ArrayStackData
{
  size_t  dataIndex;
  size_t  dataBegin;
  const char* typeBegin;
};

struct VariantStackData
{
  const char* oldType;
  char        type[257];
};

struct StackEntry
{
  enum StackEntryType type;

  union
  {
    struct ArrayStackData array;
    struct VariantStackData variant;
  }d;
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

struct ADBusMarshaller
{
  uint8_t*            h;
  size_t              hsize;
  size_t              halloc;
  uint8_t*            a;
  size_t              asize;
  size_t              aalloc;
  size_t              typeSizeOffset;
  const char*         type;
  struct StackEntry*  stack;
  size_t              stackSize;
  size_t              stackAlloc;
};


#define GROW_HEADER(marshaller, newsize)  \
  ALLOC_GROW(uint8_t, m->h, newsize, m->halloc); \
  memset(&m->h[m->hsize], 0, newsize - m->hsize); \
  m->hsize = newsize

#define GROW_ARGUMENTS(m, newsize)  \
  ALLOC_GROW(uint8_t, m->a, newsize, m->aalloc); \
  memset(&m->a[m->asize], 0, newsize - m->asize); \
  m->asize = newsize

#define HEADER(marshaller) ((struct _ADBusMessageHeader*)((marshaller)->h))
#define EXHEADER(marshaller) ((struct _ADBusMessageExtendedHeader*)((marshaller)->h))

static struct StackEntry* stackTop(struct ADBusMarshaller* m)
{
  return &m->stack[m->stackSize - 1];
}

static struct StackEntry* stackPush(struct ADBusMarshaller* m)
{
  ALLOC_GROW(struct StackEntry, m->stack, m->stackSize + 1, m->stackAlloc);
  memset(&m->stack[m->stackSize], 0, sizeof(struct StackEntry));
  m->stackSize++;
  return stackTop(m);
}

static void stackPop(struct ADBusMarshaller* m)
{
  m->stackSize--;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

struct ADBusMarshaller* ADBusCreateMarshaller()
{
  struct ADBusMarshaller* ret = (struct ADBusMarshaller*)calloc(1, sizeof(struct ADBusMarshaller));
  ADBusClearMarshaller(ret);
  return ret;
}

// ----------------------------------------------------------------------------

void ADBusClearMarshaller(struct ADBusMarshaller* m)
{
  m->asize = 0;
  m->hsize = 0;
  m->stackSize = 0;

  m->typeSizeOffset = 0;
  m->type = NULL;

  GROW_HEADER(m, sizeof(struct _ADBusMessageExtendedHeader));
  HEADER(m)->endianness = _ADBusNativeEndianness;
  HEADER(m)->type = ADBusInvalidMessage;
  HEADER(m)->flags = 0;
  HEADER(m)->version = 1;
  HEADER(m)->length = 0;
  HEADER(m)->serial = 0;
  EXHEADER(m)->headerFieldLength = 0;
}

// ----------------------------------------------------------------------------

void ADBusFreeMarshaller(struct ADBusMarshaller* m)
{
  if (!m)
    return;

  free(m->a);
  free(m->h);
  free(m->stack);
  free(m);
}

// ----------------------------------------------------------------------------

void ADBusSendMessage(struct ADBusMarshaller* m, ADBusSendCallback callback, void* user)
{
  if (!callback)
    return;

  ASSERT_RETURN(m->type == NULL || *m->type == '\0');
  ASSERT_RETURN(m->stackSize == 0);

  // Fill out header field size field
  size_t headerFieldBegin = sizeof(struct _ADBusMessageExtendedHeader);
  headerFieldBegin = _ADBUS_ALIGN_VALUE(headerFieldBegin, 8);
  size_t headerFieldEnd = m->hsize;

  size_t headerFieldSize = 0;
  // begin can be greater than end if there are no fields since begin would
  // then be aligned but end is not
  if (headerFieldEnd > headerFieldBegin)
    headerFieldSize = headerFieldEnd - headerFieldBegin;

  EXHEADER(m)->headerFieldLength = headerFieldSize;


  // Copy over arguments and fill out arg length
  size_t headerEnd = m->hsize;
  headerEnd = _ADBUS_ALIGN_VALUE(headerEnd, 8);

  GROW_HEADER(m, headerEnd + m->asize);

  memcpy(&m->h[headerEnd], m->a, m->asize);
  HEADER(m)->length = (uint32_t)m->asize;


  // Send data off
  callback(user, m->h, m->hsize);

  ADBusClearMarshaller(m);
}

// ----------------------------------------------------------------------------

void ADBusSetMessageType(struct ADBusMarshaller* m, enum ADBusMessageType type)
{
  HEADER(m)->type = type;
}

// ----------------------------------------------------------------------------

void ADBusSetSerial(struct ADBusMarshaller* m, uint32_t serial)
{
  HEADER(m)->serial = serial;
}

// ----------------------------------------------------------------------------

void ADBusSetFlags(struct ADBusMarshaller* m, int flags)
{
  HEADER(m)->flags = (uint8_t) flags;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void setUInt32HeaderField(struct ADBusMarshaller* m, enum ADBusHeaderFieldCode type, uint32_t data)
{
  size_t typei, datai;

  size_t needed = m->hsize;
  needed  = _ADBUS_ALIGN_VALUE(needed, 8);   // pad to structure
  typei = needed;
  needed += 1;            // field type
  needed += 3;            // field variant type
  needed  = _ADBUS_ALIGN_VALUE(needed, 4);   // pad to data
  datai = needed;
  needed += 4;

  GROW_HEADER(m, needed);

  uint8_t* typep = &m->h[typei];
  uint32_t* datap = (uint32_t*)&m->h[datai];

  typep[0] = type;
  typep[1] = 1;
  typep[2] = ADBusUInt32Field;
  typep[3] = '\0';

  datap[0] = data;
}

// ----------------------------------------------------------------------------

static void setStringHeaderField(struct ADBusMarshaller* m,
                                 enum ADBusHeaderFieldCode code,
                                 enum ADBusFieldType field,
                                 const char* str, int size)
{
  if (size < 0)
    size = strlen(str);

  size_t typei, stringi;

  size_t needed = m->hsize;
  needed  = _ADBUS_ALIGN_VALUE(needed, 8);   // pad to structure
  typei   = needed;
  needed += 1;            // field type
  needed += 3;            // field variant type
  needed  = _ADBUS_ALIGN_VALUE(needed, 4);   // pad to length
  stringi = needed;
  needed += 4 + size + 1; // string len + string + null

  GROW_HEADER(m, needed);

  uint8_t* typep = &m->h[typei];
  uint32_t* sizep = (uint32_t*)&m->h[stringi];
  uint8_t* stringp = (uint8_t*)&m->h[stringi + 4];

  typep[0] = code;
  typep[1] = 1;
  typep[2] = field;
  typep[3] = '\0';

  sizep[0] = (uint32_t)size;

  memcpy(stringp, str, size);
  stringp[size] = '\0';
}

// ----------------------------------------------------------------------------

void ADBusSetReplySerial(struct ADBusMarshaller* m, uint32_t reply)
{
  ASSERT_RETURN(!m->type);
  setUInt32HeaderField(m, ADBusReplySerialCode, reply);
}

// ----------------------------------------------------------------------------

void ADBusSetPath(struct ADBusMarshaller* m, const char* path, int size)
{
  ASSERT_RETURN(!m->type);
  setStringHeaderField(m, ADBusPathCode, ADBusObjectPathField, path, size);
}

// ----------------------------------------------------------------------------

void ADBusSetInterface(struct ADBusMarshaller* m, const char* interface, int size)
{
  ASSERT_RETURN(!m->type);
  setStringHeaderField(m, ADBusInterfaceCode, ADBusStringField, interface, size);
}

// ----------------------------------------------------------------------------

void ADBusSetMember(struct ADBusMarshaller* m, const char* member, int size)
{
  ASSERT_RETURN(!m->type);
  setStringHeaderField(m, ADBusMemberCode, ADBusStringField, member, size);
}

// ----------------------------------------------------------------------------

void ADBusSetErrorName(struct ADBusMarshaller* m, const char* errorName, int size)
{
  ASSERT_RETURN(!m->type);
  setStringHeaderField(m, ADBusErrorNameCode, ADBusStringField, errorName, size);
}

// ----------------------------------------------------------------------------

void ADBusSetDestination(struct ADBusMarshaller* m, const char* destination, int size)
{
  ASSERT_RETURN(!m->type);
  setStringHeaderField(m, ADBusDestinationCode, ADBusStringField, destination, size);
}

// ----------------------------------------------------------------------------

void ADBusSetSender(struct ADBusMarshaller* m, const char* sender, int size)
{
  ASSERT_RETURN(!m->type);
  setStringHeaderField(m, ADBusSenderCode, ADBusStringField, sender, size);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ADBusSetSignature(struct ADBusMarshaller* m, const char* type, int typeSize)
{
  ASSERT_RETURN(m->type == NULL || *m->type);
  ASSERT_RETURN(m->stackSize == 0);

  if (typeSize < 0)
    typeSize = (int)strlen(type);

  if (!m->typeSizeOffset)
  {
    size_t typei, stringi;

    size_t needed = m->hsize;
    needed  = _ADBUS_ALIGN_VALUE(needed, 8);   // pad to structure
    typei = needed;
    needed += 1;            // field type
    needed += 3;            // field variant type
    stringi = needed;
    needed += 1 + 1;        // string len + null

    GROW_HEADER(m, needed);

    uint8_t* typep = &m->h[typei];
    uint8_t* sizep = &m->h[stringi];
    uint8_t* stringp = &m->h[stringi + 1];

    typep[0] = ADBusSignatureCode;
    typep[1] = 1;
    typep[2] = 'g';
    typep[3] = '\0';

    sizep[0] = 0;

    stringp[0] = '\0';

    m->typeSizeOffset = stringi;
  }

  // Append type onto the end of the header buf

  // First we increment the string size
  uint8_t* psize = &m->h[m->typeSizeOffset];
  ASSERT_RETURN(*psize + typeSize <= 256);
  *psize += typeSize;


  size_t needed = m->hsize;
  needed += typeSize;
  GROW_HEADER(m, needed);

  size_t stringi = m->hsize - typeSize - 1; //insert before terminating null
  uint8_t* pstr = &m->h[stringi];
  memcpy(pstr, type, typeSize);
  pstr[typeSize] = '\0';

  m->type = (char*)&m->h[stringi];
}

// ----------------------------------------------------------------------------

const char* ADBusMarshallerCurrentSignature(struct ADBusMarshaller* m)
{
  return m->type;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void appendField(struct ADBusMarshaller* m);

#define APPEND_FIXED_SIZE_FIELD(m, fieldtype, DataType, data) \
{ \
  ASSERT_RETURN(*m->type == fieldtype); \
  size_t index = m->asize;  \
  index = _ADBUS_ALIGN_VALUE(index, sizeof(data)); \
  \
  GROW_ARGUMENTS(m, index + sizeof(data)); \
  \
  DataType* pdata = (DataType*)&m->a[index];  \
  *pdata = data;  \
  \
  m->type++;  \
  \
  appendField(m); \
}

void ADBusAppendBoolean(struct ADBusMarshaller* m, uint32_t data)
{ APPEND_FIXED_SIZE_FIELD(m, ADBusBooleanField, uint32_t, data ? 1 : 0); }

void ADBusAppendUInt8(struct ADBusMarshaller* m, uint8_t data)
{ APPEND_FIXED_SIZE_FIELD(m, ADBusUInt8Field, uint8_t, data); }

void ADBusAppendInt16(struct ADBusMarshaller* m, int16_t data)
{ APPEND_FIXED_SIZE_FIELD(m, ADBusInt16Field, int16_t, data); }

void ADBusAppendUInt16(struct ADBusMarshaller* m, uint16_t data)
{ APPEND_FIXED_SIZE_FIELD(m, ADBusUInt16Field, uint16_t, data); }

void ADBusAppendInt32(struct ADBusMarshaller* m, int32_t data)
{ APPEND_FIXED_SIZE_FIELD(m, ADBusInt32Field, int32_t, data); }

void ADBusAppendUInt32(struct ADBusMarshaller* m, uint32_t data)
{ APPEND_FIXED_SIZE_FIELD(m, ADBusUInt32Field, uint32_t, data); }

void ADBusAppendInt64(struct ADBusMarshaller* m, int64_t data)
{ APPEND_FIXED_SIZE_FIELD(m, ADBusInt64Field, int64_t, data); }

void ADBusAppendUInt64(struct ADBusMarshaller* m, uint64_t data)
{ APPEND_FIXED_SIZE_FIELD(m, ADBusUInt64Field, uint64_t, data); }

void ADBusAppendDouble(struct ADBusMarshaller* m, double data)
{ APPEND_FIXED_SIZE_FIELD(m, ADBusDoubleField, double, data); }

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void appendLongString(struct ADBusMarshaller* m, const char* str, int size)
{
  if (size < 0)
    size = (int)strlen(str);

  size_t index = m->asize;
  index += _ADBUS_ALIGN_VALUE(index, 4);

  GROW_ARGUMENTS(m, index + 4 + size + 1);

  uint32_t* psize = (uint32_t*)&m->a[index];
  char* pstr = (char*)&m->a[index + 4];

  *psize = size;
  memcpy(pstr, str, size);
  pstr[size] = '\0';

  m->type++;

  appendField(m);
}

// ----------------------------------------------------------------------------

void ADBusAppendString(struct ADBusMarshaller* m, const char* str, int size)
{
  ASSERT_RETURN(*m->type == ADBusStringField);
  appendLongString(m, str, size);
}

// ----------------------------------------------------------------------------

void ADBusAppendObjectPath(struct ADBusMarshaller* m, const char* str, int size)
{
  ASSERT_RETURN(*m->type == ADBusObjectPathField);
  appendLongString(m, str, size);
}

// ----------------------------------------------------------------------------

void ADBusAppendSignature(struct ADBusMarshaller* m, const char* str, int size)
{
  ASSERT_RETURN(*m->type == ADBusSignatureField);

  if (size < 0)
    size = (int)strlen(str);

  ASSERT_RETURN(size <= 256);

  size_t index = m->asize;
  GROW_ARGUMENTS(m, index + 1 + size + 1);

  uint8_t* psize = &m->a[index];
  char* pstr = (char*)(psize + 1);

  *psize = size;
  memcpy(pstr, str, size);
  pstr[size] = '\0';

  m->type++;

  appendField(m);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ADBusBeginArray(struct ADBusMarshaller* m)
{
  ASSERT_RETURN(*m->type == ADBusArrayBeginField);

  m->type++;

  size_t dataIndex;
  size_t needed = m->asize;
  needed  = _ADBUS_ALIGN_VALUE(needed, 4);
  dataIndex = needed;
  needed += 4;
  needed  = _ADBUS_ALIGN_VALUE(needed, _ADBusRequiredAlignment(*m->type));

  GROW_ARGUMENTS(m, needed);

  struct StackEntry* s = stackPush(m);
  s->type = ArrayStackEntry;
  s->d.array.dataIndex = dataIndex;
  s->d.array.dataBegin = needed;
  s->d.array.typeBegin = m->type;

}

// ----------------------------------------------------------------------------

static void appendArrayChild(struct ADBusMarshaller* m)
{
  struct StackEntry* s = stackTop(m);
  m->type = s->d.array.typeBegin;
}

// ----------------------------------------------------------------------------

void ADBusEndArray(struct ADBusMarshaller* m)
{
  struct StackEntry* s = stackTop(m);
  uint32_t* psize = (uint32_t*)&m->a[s->d.array.dataIndex];
  *psize = (uint32_t)(m->asize - s->d.array.dataBegin);
  ASSERT_RETURN(*psize < ADBusMaximumArrayLength);

  stackPop(m);

  appendField(m);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ADBusBeginStruct(struct ADBusMarshaller* m)
{
  ASSERT_RETURN(*m->type == ADBusStructBeginField);

  m->type++;

  size_t needed = m->asize;
  needed = _ADBUS_ALIGN_VALUE(needed, 8);
  GROW_ARGUMENTS(m, needed);

  struct StackEntry* s = stackPush(m);
  s->type = StructStackEntry;
}

// ----------------------------------------------------------------------------

static void appendStructChild(struct ADBusMarshaller* m)
{
  // Nothing to do
}

// ----------------------------------------------------------------------------

void ADBusEndStruct(struct ADBusMarshaller* m)
{
  ASSERT_RETURN(*m->type == ADBusStructEndField);

  m->type++;

  stackPop(m);

  appendField(m);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ADBusBeginDictEntry(struct ADBusMarshaller* m)
{
  ASSERT_RETURN(*m->type == ADBusDictEntryBeginField);

  m->type++;

  size_t needed = m->asize;
  needed = _ADBUS_ALIGN_VALUE(needed, 8);
  GROW_ARGUMENTS(m, needed);

  struct StackEntry* s = stackPush(m);
  s->type = DictEntryStackEntry;
}

// ----------------------------------------------------------------------------

static void appendDictEntryChild(struct ADBusMarshaller* m)
{
  // Nothing to do
}

// ----------------------------------------------------------------------------

void ADBusEndDictEntry(struct ADBusMarshaller* m)
{
  ASSERT_RETURN(*m->type == ADBusDictEntryEndField);

  m->type++;

  stackPop(m);

  appendField(m);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ADBusBeginVariant(struct ADBusMarshaller* m, const char* type, int typeSize)
{
  ASSERT_RETURN(*m->type == ADBusVariantBeginField);

  if (typeSize < 0)
    typeSize = strlen(type);

  ASSERT_RETURN(typeSize <= 256);
  if (typeSize > 256)
    return;

  m->type++;

  // Set the variant type in the output buffer
  size_t index = m->asize;
  GROW_ARGUMENTS(m, index + 1 + typeSize + 1);

  uint8_t* psize = &m->a[index];
  char* pstr = (char*)(psize + 1);

  *psize = (uint8_t)typeSize;
  memcpy(pstr, type, typeSize);
  pstr[typeSize] = '\0';

  // Setup stack entry
  struct StackEntry* s = stackPush(m);
  s->type = VariantStackEntry;
  s->d.variant.oldType = m->type;

  m->type = type;
}

// ----------------------------------------------------------------------------

static void appendVariantChild(struct ADBusMarshaller* m)
{
  // Nothing to do
}

// ----------------------------------------------------------------------------

void ADBusEndVariant(struct ADBusMarshaller* m)
{
  ASSERT_RETURN(*m->type == '\0');
  struct StackEntry* s = stackTop(m);
  m->type = s->d.variant.oldType;

  stackPop(m);

  appendField(m);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void appendField(struct ADBusMarshaller* m)
{
  if (m->stackSize == 0)
    return;

  switch(stackTop(m)->type)
  {
  case ArrayStackEntry:
    return appendArrayChild(m);
  case VariantStackEntry:
    return appendVariantChild(m);
  case DictEntryStackEntry:
    return appendDictEntryChild(m);
  case StructStackEntry:
    return appendStructChild(m);
  }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
