#include "Marshaller.h"

#include "Misc_p.h"





//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

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
  StackEntryType type;

  union
  {
    struct ArrayStackData array;
    struct VariantStackData variant;
  }d;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct DBusMarshaller_
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
  DBusSendCallback    callback;
  void*               callbackData;
};


#define GROW_HEADER(marshaller, newsize)  \
  ALLOC_GROW(m->h, newsize, m->halloc); \
  memset(&m->h[m->hsize], 0, newsize - m->hsize); \
  m->hsize = newsize

#define GROW_ARGUMENTS(m, newsize)  \
  ALLOC_GROW(m->a, newsize, mr->aalloc); \
  memset(&m->a[m->asize], 0, newsize - m->asize); \
  m->asize = newsize

#define HEADER(marshaller) ((struct MessageHeader*)((marshaller)->h))

static StackEntry* stackTop(DBusMarshaller* m)
{
  return &m->stack[m->stackSize - 1];
}

static StackEntry* stackPush(DBusMarshaller* m)
{
  ALLOC_GROW(m->stack, m->stackSize + 1, m->stackAlloc);
  memset(&m->stack[m->stackSize], 0, sizeof(struct StackEntry));
  m->stackSize++;
  return stackTop(m);
}

static void stackPop(DBusMarshaller* m)
{
  m->stackSize--;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

DBusMarshaller* DBusCreateMarshaller()
{
  DBusMarshaller* ret = new Marshaller;
  memset(ret, 0, sizeof(Marshaller));
  clearMarshaller(ret);
  return ret;
}

//-----------------------------------------------------------------------------

void DBusClearMarshaller(DBusMarshaller* m)
{
  GROW_HEADER(m, sizeof(struct MessageHeader));
  HEADER(m)->endianness = kNativeEndianness;
  HEADER(m)->type = InvalidType;
  HEADER(m)->flags = 0;
  HEADER(m)->version = 1;
  HEADER(m)->length = 0;
  HEADER(m)->serial = 0;
  HEADER(m)->headerFieldLength = 0;

  m->asize = 0;
  m->hsize = 0;
  m->stack.size = 0;

  m->typeSizeOffset = 0;
  m->type = NULL;
}

//-----------------------------------------------------------------------------

void DBusFreeMarshaller(DBusMarshaller* m)
{
  free(m->a);
  free(m->h);
  free(m->stack);
  free(m);
}

//-----------------------------------------------------------------------------

void DBusSetCallback(DBusMarshaller* m, DBusSendCallback callback, void* userData)
{
  m->callback = callback;
  m->callbackData = userData;
}

//-----------------------------------------------------------------------------

void DBusSendMessage(DBusMarshaller* m)
{
  if (!m->callback)
    return;
  
  ASSERT_RETURN(*m->type == '\0');
  if (*m->type != '\0')
    return;

  ASSERT_RETURN(m->stack.size == 0);
  if (m->stack.size > 0)
    return;

  size_t headerEnd = m->hsize;
  GROW_HEADER(m, m->hsize + m->asize);

  memcpy(&m->h[headerEnd], m->a, m->asize);

  HEADER(m)->length = m->asize;

  m->callback(m->callbackData, m->h, m->hsize);

  clearMarshaller(m);
}

//-----------------------------------------------------------------------------

void DBusSetMessageType(DBusMarshaller* m, MessageType type)
{
  HEADER(m)->type = type;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void setUInt32HeaderField(DBusMarshaller* m, HeaderFieldCode type, uint32_t data)
{
  size_t typei, datai;

  size_t needed = m->hsize;
  needed += needed % 8;   // pad to structure
  typei = needed;     
  needed += 1;            // field type
  needed += 3;            // field variant type
  needed += needed % 4;   // pad to data
  datai = needed;
  needed += 4;

  GROW_HEADER(m, needed);

  uint8_t* typep = &m->h[typei];
  uint32_t* datap = (uint32_t*)&m->h[datai];

  typep[0] = type;
  typep[1] = 1;
  typep[2] = UInt32Field;
  typep[3] = '\0';

  datap[0] = data;
}

//-----------------------------------------------------------------------------

static void setStringHeaderField(DBusMarshaller* m, HeaderFieldCode type,
                                 const char* str, size_t size)
{
  if (size < 0)
    size = strlen(str);

  size_t typei, stringi;

  size_t needed = m->hsize;
  needed += needed % 8;   // pad to structure
  typei = needed;
  needed += 1;            // field type
  needed += 3;            // field variant type
  needed += needed % 4;   // pad to length
  stringi = needed;
  needed += 4 + size + 1; // string len + string + null

  GROW_HEADER(m, needed);

  uint8_t* typep = &m->h[typei];
  uint32_t* sizep = (uint32_t*)&m->h[stringi];
  uint8_t* stringp = (uint8_t*)&m->h[stringi + 4];

  typep[0] = type;
  typep[1] = 1;
  typep[2] = 's';
  typep[3] = '\0';

  sizep[0] = size;

  memcpy(stringp, str, size);
  stringp[size] = '\0';
}

//-----------------------------------------------------------------------------

void DBusSetReplySerial(DBusMarshaller* m, uint32_t reply)
{
  ASSERT_RETURN(!m->type);
  setUInt32HeaderField(m, ReplySerialFieldCode, reply);
}

//-----------------------------------------------------------------------------

void DBusSetPath(DBusMarshaller* m, const char* path, int size)
{
  ASSERT_RETURN(!m->type);
  setStringHeaderField(m, PathFieldCode, path, size);
}

//-----------------------------------------------------------------------------

void DBusSetInterface(DBusMarshaller* m, const char* interface, int size)
{
  ASSERT_RETURN(!m->type);
  setStringHeaderField(m, InterfaceFieldCode, interface, size);
}

//-----------------------------------------------------------------------------

void DBusSetMember(DBusMarshaller* m, const char* member, int size)
{
  ASSERT_RETURN(!m->type);
  setStringHeaderField(m, MemberFieldCode, member, size);
}

//-----------------------------------------------------------------------------

void DBusSetErrorName(DBusMarshaller* m, const char* errorName, int size)
{
  ASSERT_RETURN(!m->type);
  setStringHeaderField(m, ErrorNameFieldCode, errorName, size);
}

//-----------------------------------------------------------------------------

void DBusSetDestination(DBusMarshaller* m, const char* destination, int size)
{
  ASSERT_RETURN(!m->type);
  setStringHeaderField(m, DestinationFieldCode, destination, size);
}

//-----------------------------------------------------------------------------

void DBusSetSender(DBusMarshaller* m, const char* sender, int size)
{
  ASSERT_RETURN(!m->type);
  setStringHeaderField(m, SenderFieldCode, sender, size);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void appendField(DBusMarshaller* m);

#define APPEND_FIXED_SIZE_FIELD(m, fieldtype, data) \
{ \
  ASSERT_RETURN(*m->type == fieldtype); \
  size_t index = m->asize;  \
  index += index % sizeof(data); \
  \
  GROW_ARGUMENTS(m, index + sizeof(data)); \
  \
  T* pdata = (T*)&m->a[index];  \
  *pdata = data;  \
  \
  m->type++;  \
  \
  appendField(m); \
}

void DBusAppendBoolean(DBusMarshaller* m, uint32_t data)
{ APPEND_FIXED_SIZE_FIELD(m, BooleanField, data ? 1 : 0); }

void DBusAppendUInt8(DBusMarshaller* m, uint8_t data)
{ APPEND_FIXED_SIZE_FIELD(m, UInt8Field, data); }

void DBusAppendInt16(DBusMarshaller* m, int16_t data)
{ APPEND_FIXED_SIZE_FIELD(m, Int16Field, data); }

void DBusAppendUInt16(DBusMarshaller* m, uint16_t data)
{ APPEND_FIXED_SIZE_FIELD(m, UInt16Field, data); }

void DBusAppendInt32(DBusMarshaller* m, int32_t data)
{ APPEND_FIXED_SIZE_FIELD(m, Int32Field, data); }

void DBusAppendUInt32(DBusMarshaller* m, uint32_t data)
{ APPEND_FIXED_SIZE_FIELD(m, UInt32Field, data); }

void DBusAppendInt64(DBusMarshaller* m, int64_t data)
{ APPEND_FIXED_SIZE_FIELD(m, Int64Field, data); }

void DBusAppendUInt64(DBusMarshaller* m, uint64_t data)
{ APPEND_FIXED_SIZE_FIELD(m, UInt64Field, data); }

void DBusAppendDouble(DBusMarshaller* m, double data)
{ APPEND_FIXED_SIZE_FIELD(m, DoubleField, data); }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void appendLongString(DBusMarshaller* m, const char* str, int size)
{
  if (size < 0)
    size = strlen(str);

  size_t index = m->asize;
  index += index % 4;

  GROW_ARGUMENTS(m, index + 4 + size + 1);

  uint32_t* psize = (uint32_t*)&m->a[index];
  char* pstr = (char*)&m->a[index + 4];

  *psize = size;
  memcpy(pstr, str, size);
  pstr[size] = '\0';

  m->type++;

  appendField(m);
}

//-----------------------------------------------------------------------------

void DBusAppendString(DBusMarshaller* m, const char* str, int size)
{
  ASSERT_RETURN(*m->type == StringField);
  appendLongString(m, str, size);
}

//-----------------------------------------------------------------------------

void DBusAppendObjectPath(DBusMarshaller* m, const char* str, int size)
{
  ASSERT_RETURN(*m->type == ObjectPathField);
  appendLongString(m, str, size);
}

//-----------------------------------------------------------------------------

void DBusAppendSignature(DBusMarshaller* m, const char* str, int size)
{
  ASSERT_RETURN(*m->type == SignatureField);

  if (size < 0)
    size = strlen(str);

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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void DBusBeginArgument(DBusMarshaller* m, const char* type, int typeSize)
{
  ASSERT_RETURN(m->type == NULL || *m->type);
  ASSERT_RETURN(m->stack.size == 0);

  if (typeSize < 0)
    typeSize = strlen(type);

  if (!m->typeSizeOffset)
  {
    size_t typei, stringi;

    size_t needed = m->hsize;
    needed += needed % 8;   // pad to structure
    typei = needed;     
    needed += 1;            // field type
    needed += 3;            // field variant type
    needed += needed % 4;   // pad to length
    stringi = needed;
    needed += 4 + 1;        // string len + null

    GROW_HEADER(m, needed);

    uint8_t* typep = &m->h[typei];
    uint32_t* sizep = (uint32_t*)&m->h[stringi];
    uint8_t* stringp = (uint8_t*)&m->h[stringi + 4];

    typep[0] = SignatureFieldCode;
    typep[1] = 1;
    typep[2] = 's';
    typep[3] = '\0';

    sizep[0] = 0;

    stringp[0] = '\0';

    m->typeSizeOffset = typei;
  }

  // Append type onto the end of the header buf (ignoring the existing null
  // and adding a null back on afterwards)

  // First we increment the string size
  uint8_t* psize = &m->h[m->typeSizeOffset];
  ASSERT_RETURN(*psize + typeSize <= 256);
  *psize += typeSize;


  size_t needed = m->asize;
  needed += typeSize;
  GROW_HEADER(m, needed);

  size_t stringi = m->hsize - typeSize - 1; // need to insert before previously existing null
  uint8_t* pstr = &m->h[stringi];
  memcpy(pstr, type, typeSize);
  pstr[typeSize] = '\0';

  m->type = (char*)&m->h[stringi];
}

//-----------------------------------------------------------------------------

static void appendArgumentChild(DBusMarshaller* m)
{
  // Each arugment should only be one complete type
  ASSERT_RETURN(*m->type == '\0');
}

//-----------------------------------------------------------------------------

void DBusEndArgument(DBusMarshaller* m)
{
  ASSERT_RETURN(*m->type == '\0');
  ASSERT_RETURN(m->stack.size == 0);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void DBusBeginArray(DBusMarshaller* m)
{
  ASSERT_RETURN(*m->type == ArrayBeginField);

  m->type++;

  size_t dataIndex;
  size_t needed = m->asize;
  needed += needed % 4;
  dataIndex = needed;
  needed += 4;
  needed += needed % requiredAlignment(FieldType(*m->type));

  GROW_ARGUMENTS(m, needed);

  StackEntry* s = stackPush(m);
  s->type = ArrayStackEntry;
  s->d.array.dataIndex = dataIndex;
  s->d.array.dataBegin = needed;
  s->d.array.typeBegin = m->type;

}

//-----------------------------------------------------------------------------

static void appendArrayChild(DBusMarshaller* m)
{
  StackEntry* s = stackTop(m);
  m->type = s->d.array.typeBegin;
}

//-----------------------------------------------------------------------------

void DBusEndArray(DBusMarshaller* m)
{
  StackEntry* s = stackTop(m);
  uint32_t* psize = (uint32_t*)&m->a[s->d.array.dataIndex];
  *psize = m->asize - s->d.array.dataBegin;
  ASSERT_RETURN(*psize < MaximumArrayLength);

  stackPop(m);

  appendField(m);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void DBusBeginStruct(DBusMarshaller* m)
{
  ASSERT_RETURN(*m->type == StructBeginField);

  m->type++;

  size_t needed = m->asize;
  needed += needed % 8;
  GROW_ARGUMENTS(m, needed);

  StackEntry* s = stackPush(m);
  s->type = StructStackEntry;
}

//-----------------------------------------------------------------------------

static void appendStructChild(DBusMarshaller* m)
{
  // Nothing to do
}

//-----------------------------------------------------------------------------

void DBusEndStruct(DBusMarshaller* m)
{
  ASSERT_RETURN(*m->type == StructEndField);

  m->type++;

  stackPop(m);

  appendField(m);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void DBusBeginDictEntry(DBusMarshaller* m)
{
  ASSERT_RETURN(*m->type == DictEntryBeginField);

  m->type++;

  size_t needed = m->asize;
  needed += needed % 8;
  GROW_ARGUMENTS(m, needed);

  StackEntry* s = stackPush(m);
  s->type = DictEntryStackEntry;
}

//-----------------------------------------------------------------------------

static void appendDictEntryChild(DBusMarshaller* m)
{
  // Nothing to do
}

//-----------------------------------------------------------------------------

void DBusEndDictEntry(DBusMarshaller* m)
{
  ASSERT_RETURN(*m->type == DictEntryEndField);

  m->type++;

  stackPop(m);

  appendField(m);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void DBusBeginVariant(DBusMarshaller* m, const char* type)
{
  ASSERT_RETURN(*m->type == VariantBeginField);

  size_t typeSize = strlen(type);

  ASSERT_RETURN(typeSize <= 256);
  if (typeSize > 256)
    return;

  m->type++;

  // Set the variant type in the output buffer
  size_t index = m->asize;
  GROW_ARGUMENTS(m, index + 1 + typeSize + 1);

  uint8_t* psize = &m->a[index];
  char* pstr = (char*)(psize + 1);

  *psize = typeSize;
  memcpy(pstr, type, typeSize);
  pstr[typeSize] = '\0';

  // Setup stack entry
  StackEntry* s = stackPush(m);
  s->type = VariantStackEntry;
  s->d.variant.oldType = m->type;

  m->type = type;
}

//-----------------------------------------------------------------------------

static void appendVariantChild(DBusMarshaller* m)
{
  // Nothing to do
}

//-----------------------------------------------------------------------------

void DBusEndVariant(DBusMarshaller* m)
{
  ASSERT_RETURN(*m->type == '\0');
  StackEntry* s = stackTop(m);
  m->type = s->d.variant.oldType;

  stackPop(m);

  appendField(m);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void appendField(DBusMarshaller* m)
{
  if (m->stack.size == 0)
    return appendArgumentChild(m);
  
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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
