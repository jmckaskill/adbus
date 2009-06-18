#include "Message.h"
#include "Message_p.h"
#include "Misc_p.h"



//-----------------------------------------------------------------------------
// Helper Functions
//-----------------------------------------------------------------------------

static bool isStackEmpty(Message* m)
{
  return m->stackSize == 0;
}

static int stackSize(Message* m)
{
  return m->stackSize;
}

static struct StackEntry* stackTop(Message* m)
{
  return &m->stack[m->stackSize-1];
}

static struct StackEntry* pushStackEntry(Message* m)
{
  ALLOC_GROW(m->stack, m->stackSize+1, m->stackAlloc);
  memset(&m->stack[m->stackSize], 0, sizeof(struct StackEntry));
  m->stackSize++;
  return stackTop(m);
}

void _DBusPopStackEntry(Message* m)
{
  m->stackSize--;
}

//-----------------------------------------------------------------------------

static size_t dataRemaining(Message* m)
{
  return m->dataEnd - m->data;
}

static uint8_t* getData(Message* m, size_t size)
{
  ASSERT(size > dataRemaining(m));
  uint8_t* ret = m->data;
  m->data += size;
  return ret;
}

//-----------------------------------------------------------------------------

// where b0 _DBusIs lowest byte
#define MAKE16(b1,b0) \
  ((uint16_t((b1)) << 8) | (b0))
#define MAKE32(b3,b2,b1,b0) \
  ((uint32_t(MAKE16((b3),(b2))) << 16) | MAKE16((b1),(b0)))
#define MAKE64(b7,b6,b5,b4,b3,b2,b1,b0)  \
  ((uint64_t(MAKE32((b7),(b6),(b5),(b4))) << 32) | MAKE32((b3),(b2),(b1),(b0)))

#define ENDIAN_CONVERT16(val) MAKE16( (val)        & 0xFF, ((val) >> 8)  & 0xFF)

#define ENDIAN_CONVERT32(val) MAKE32( (val)        & 0xFF, ((val) >> 8)  & 0xFF,\
                                     ((val) >> 16) & 0xFF, ((val) >> 24) & 0xFF)

#define ENDIAN_CONVERT64(val) MAKE64( (val)        & 0xFF, ((val) >> 8)  & 0xFF,\
                                     ((val) >> 16) & 0xFF, ((val) >> 24) & 0xFF,\
                                     ((val) >> 32) & 0xFF, ((val) >> 40) & 0xFF,\
                                     ((val) >> 48) & 0xFF, ((val) >> 56) & 0xFF)

static uint8_t get8BitData(Message* m)
{
  return *getData(m, sizeof(uint8_t));
}

static uint16_t get16BitData(Message* m)
{
  uint8_t* data = getData(m, sizeof(uint16_t));
  if (!m->nativeEndian)
    *reinterpret_cast<uint16_t*>(data) = ENDIAN_CONVERT16(*reinterpret_cast<uint16_t*>(data));
  return *reinterpret_cast<uint16_t*>(data);
}

static uint32_t get32BitData(Message* m)
{
  uint8_t* data = getData(m, sizeof(uint32_t));
  if (!m->nativeEndian)
    *reinterpret_cast<uint32_t*>(data) = ENDIAN_CONVERT32(*reinterpret_cast<uint32_t*>(data));
  return *reinterpret_cast<uint32_t*>(data);
}

static uint64_t get64BitData(Message* m)
{
  uint8_t* data = getData(m, sizeof(uint64_t));
  if (m->nativeEndian)
    *reinterpret_cast<uint64_t*>(data) = ENDIAN_CONVERT64(*reinterpret_cast<uint64_t*>(data));
  return *reinterpret_cast<uint64_t*>(data);
}

//-----------------------------------------------------------------------------

static bool isAligned(Message* m)
{
  return uintptr_t(m->data) % requiredAlignment(FieldType(*m->signature)) == 0;
}

static void processAlignment(Message* m)
{
  m->data += uintptr_t(m->data) % requiredAlignment(FieldType(*m->signature));
}

//-----------------------------------------------------------------------------






//-----------------------------------------------------------------------------
// Private API
//-----------------------------------------------------------------------------

int _DBusProcessField(Message* m, Field* f)
{
  switch(*m->signature)
  {
  case UInt8Field:
    return _DBusProcess8Bit(m,f,(uint8_t*)&f->data.u8);
  case BooleanField:
    return _DBusProcessBoolean(m,f);
  case Int16Field:
    return _DBusProcess16Bit(m,f,(uint16_t*)&f->data.i16);
  case UInt16Field:
    return _DBusProcess16Bit(m,f,(uint16_t*)&f->data.u16);
  case Int32Field:
    return _DBusProcess32Bit(m,f,(uint32_t*)&f->data.i32);
  case UInt32Field:
    return _DBusProcess32Bit(m,f,(uint32_t*)&f->data.u32);
  case Int64Field:
    return _DBusProcess64Bit(m,f,(uint64_t*)&f->data.i64);
  case UInt64Field:
    return _DBusProcess64Bit(m,f,(uint64_t*)&f->data.u64);
  case DoubleField:
    return _DBusProcess64Bit(m,f,(uint64_t*)&f->data.d);
  case StringField:
    return _DBusProcessString(m,f);
  case ObjectPathField:
    return _DBusProcessObjectPath(m,f);
  case SignatureField:
    return _DBusProcessSignature(m,f);
  case ArrayBeginField:
    return _DBusProcessArray(m,f);
  case StructBeginField:
    return _DBusProcessStruct(m,f);
  case VariantBeginField:
    return _DBusProcessVariant(m,f);
  case DictEntryBeginField:
    return _DBusProcessDictEntry(m,f);
  default:
    return DBusInvalidData;
  }
}

//-----------------------------------------------------------------------------

int _DBusProcess8Bit(Message* m, Field* f, uint8_t* fieldData)
{
  ASSERT(isAligned(m));

  if (dataRemaining(m) < 1)
    return DBusInvalidData;

  f->type = FieldType(*m->signature);
  *fieldData = get8BitData(m);

  m->signature += 1;

  return 0;
}

//-----------------------------------------------------------------------------

int _DBusProcess16Bit(Message* m, Field* f, uint16_t* fieldData)
{
  ASSERT(isAligned(m));

  if (dataRemaining(m) < 2)
    return DBusInvalidData;

  f->type = FieldType(*m->signature);
  *fieldData = get16BitData(m);

  m->signature += 1;

  return 0;
}

//-----------------------------------------------------------------------------

int _DBusProcess32Bit(Message* m, Field* f, uint32_t* fieldData)
{
  ASSERT(isAligned(m));

  if (dataRemaining(m) < 4)
    return DBusInvalidData;

  f->type = FieldType(*m->signature);
  *fieldData = get32BitData(m);

  m->signature += 1;

  return 0;
}

//-----------------------------------------------------------------------------

int _DBusProcess64Bit(Message* m, Field* f, uint64_t* fieldData)
{
  ASSERT(isAligned(m));

  if (dataRemaining(m) < 8)
    return DBusInvalidData;

  f->type = FieldType(*m->signature);
  *fieldData = get64BitData(m);

  m->signature += 1;

  return 0;
}

//-----------------------------------------------------------------------------

int _DBusProcessBoolean(Message* m, Field* f)
{
  int err = _DBusProcess32Bit(m, f, &f->data.b);
  if (err)
    return err;

  if (f->data > 1)
    return DBusInvalidData;

  f->data.b = f->data.b ? 1 : 0;

  return 0;
}

//-----------------------------------------------------------------------------

int _DBusProcessStringData(Message* m, Field* f)
{
  size_t size = f->data.string.size;
  if (dataRemaining(m) < size + 1)
    return DBusInvalidData;

  const char* str = (const char*) getData(m, size + 1);
  if (hasNullByte(str, size))
    return DBusInvalidData;

  if (str + size != '\0')
    return DBusInvalidData;

  if (!_DBusIsValidUtf8(str, size))
    return DBusInvalidData;

  f->data.string.str = str;

  m->signature += 1;

  return 0;
}

//-----------------------------------------------------------------------------

int _DBusProcessObjectPath(Message* m, Field* f)
{
  ASSERT(isAligned(m));
  ASSERT(&f->data.objectPath == &f->data.string);
  if (dataRemaining(m) < 4)
    return DBusInvalidData;

  f->type = ObjectPathField;
  f->data.objectPath.size = get32BitData(m);

  int err = _DBusProcessStringData(m,f);
  if (err)
    return err;

  if (!_DBusIsValidObjectPath(f->data.objectPath.str, f->data.objectPath.size))
    return DBusInvalidData;

  return 0;
}

//-----------------------------------------------------------------------------

int _DBusProcessString(Message* m, Field* f)
{
  ASSERT(isAligned(m));
  if (dataRemaining(m) < 4)
    return DBusInvalidData;

  f->type = StringField;
  f->data.string.size = get32BitData(m);

  return _DBusProcessStringData(m,f);
}

//-----------------------------------------------------------------------------

int _DBusProcessSignature(Message* m, Field* f)
{
  ASSERT(isAligned(m));
  ASSERT(&f->data.signature == &f->data.string);
  if (dataRemaining(m) < 1)
    return DBusInvalidData;

  f->type = SignatureField;
  f->data.signature.size = get8BitData(m);

  return _DBusProcessStringData(m,f);
}

//-----------------------------------------------------------------------------

int _DBusNextField(Message* m, Field* f)
{
  if (_DBusIsStackEmpty(m))
  {
    return _DBusNextRootField(m,f);
  }
  else
  {
    switch(stackTop(m)->type)
    {
    case VariantStackEntry:
      return _DBusNextVariantField(m,f);
    case DictEntryStackEntry:
      return _DBusNextDictEntryField(m,f);
    case ArrayStackEntry:
      return _DBusNextArrayField(m,f);
    case StructStackEntry:
      return _DBusNextStructField(m,f);
    default:
      ASSERT(false);
      return InternalError;
    }
  }
}

//-----------------------------------------------------------------------------

int _DBusNextRootField(Message* m, Field* f)
{
  if (*m->signature == MessageEndField)
    return 0;

  processAlignment(m);
  return _DBusProcessField(m,f);
}

//-----------------------------------------------------------------------------

bool _DBusIsRootAtEnd(Message* m)
{
  return *m->signature == MessageEndField;
}

//-----------------------------------------------------------------------------

int _DBusProcessStruct(Message* m, Field* f)
{
  ASSERT(isAligned(m));

  if (dataRemaining(m) == 0)
    return DBusInvalidData;

  StackEntry* e = pushStackEntry(m);
  e->type = StructStackEntry;

  m->signature += 1; // skip over '('

  return 0;
}

//-----------------------------------------------------------------------------

int _DBusNextStructField(Message* m, Field* f)
{
  if (*m->signature != ')')
  {
    processAlignment(m);
    return _DBusProcessField(m,f);
  }

  f->type = StructEndField;
  popStackEntry(m);

  m->signature += 1; // skip over ')'

  return 0;
}

//-----------------------------------------------------------------------------

bool _DBusIsStructAtEnd(Message* m)
{
  return *m->signature == ')';
}

//-----------------------------------------------------------------------------

int _DBusProcessDictEntry(Message* m, Field* f)
{
  ASSERT(isAligned(m));

  StackEntry* e = pushStackEntry(m);
  e->type = DictEntryStackEntry;
  e->data.dictEntryFields = 0;

  f->type = DictEntryBeginField;

  m->signature += 1; // skip over '{'

  return 0;
}

//-----------------------------------------------------------------------------

int _DBusNextDictEntryField(Message* m, Field* f)
{
  StackEntry* e = stackTop(m);
  if (*m->signature != '}')
  {
    processAlignment(m);
    if (++(e->data.dictEntryFields) > 2)
      return DBusInvalidData;
    else
      return _DBusProcessField(m,f);
  }

  f->type = DictEntryEndField;
  popStackEntry(m);

  m->signature += 1; // skip over '}'

  return 0;
}

//-----------------------------------------------------------------------------

bool _DBusIsDictEntryAtEnd(Message* m)
{
  return *m->signature == '}';
}

//-----------------------------------------------------------------------------

int _DBusProcessArray(Message* m, Field* f)
{
  ASSERT(isAligned(m));
  if (dataRemaining(m) < 4)
    return DBusInvalidData;
  size_t size = get32BitData(m);

  if (size > MaximumArrayLength || size > dataRemaining(m))
    return DBusInvalidData;

  m->signature += 1; // skip over 'a'
  
  processAlignment(m);

  StackEntry* e = pushStackEntry(m);
  e->type = ArrayStackEntry;
  e->data.array.dataEnd = m->data + size;
  e->data.array.typeBegin = m->signature;

  f->type = ArrayBeginField;
  f->data.arrayDataSize = size;

  return 0;
}

//-----------------------------------------------------------------------------

int _DBusNextArrayField(Message* m, Field* f)
{
  StackEntry* e = stackTop(m);
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

  f->type = ArrayEndField;
  popStackEntry(m);
  return 0;
}

//-----------------------------------------------------------------------------

bool _DBusIsArrayAtEnd(Message* m)
{
  StackEntry* e = stackTop(m);
  return m->data >= e->data.array.dataEnd;
}

//-----------------------------------------------------------------------------

int _DBusProcessVariant(Message* m, Field* f)
{
  ASSERT(isAligned(m));
  ASSERT(&f->data.variantType == &f->data.signature);
  int err = _DBusProcessSignature(m,f);
  if (err)
    return err;

  StackEntry* e = pushStackEntry(m);
  e->type = VariantStackEntry;
  e->data.variantOldType = m->signature + 1;

  f->type = VariantBeginField;
  // f->data.variantType has already been filled out by _DBusProcessSignature

  m->signature = f->data.variantType.str;

  return 0;
}

//-----------------------------------------------------------------------------

int _DBusNextVariantField(Message* m, Field* f)
{
  StackEntry* e = stackTop(m);
  if (m->signature == e->data.variantOldType)
  {
    processAlignment(m);
    return _DBusProcessField(m,f);
  }
  else if (*m->signature != '\0')
  {
    return DBusInvalidData; // there's more than one root type in the variant type
  }

  m->signature = e->data.variantOldType;
  
  return 0;
}

//-----------------------------------------------------------------------------

bool _DBusIsVariantAtEnd(Message* m)
{
  return *m->signature == '\0';
}

//-----------------------------------------------------------------------------





//-----------------------------------------------------------------------------
//  Public API
//-----------------------------------------------------------------------------

const char* DBusMessageSignature(Message* m)
{
  return m->signature;
}

//-----------------------------------------------------------------------------

bool DBusIsScopeAtEnd(Message* m, int scope)
{
  if (stackSize(m) < scope)
  {
    ASSERT(false);
    return true;
  }

  if (stackSize(m) > scope)
    return false;

  if (_DBusIsStackEmpty(m))
    return _DBusIsRootAtEnd(m);

  switch(stackTop(m)->type)
  {
  case VariantStackEntry:
    return _DBusIsVariantAtEnd(m);
  case DictEntryStackEntry:
    return _DBusIsDictEntryAtEnd(m);
  case ArrayStackEntry:
    return _DBusIsArrayAtEnd(m);
  case StructStackEntry:
    return _DBusIsStructAtEnd(m);
  default:
    ASSERT(false);
    return true;
  }
}

//-----------------------------------------------------------------------------

int DBusTakeField(Message* m, Field* f)
{
  return ::_DBusNextField(m,f);
}

//-----------------------------------------------------------------------------

static int takeSpecificField(Message* m, Field* f, char type)
{
  int err = ::_DBusNextField(m, f);
  if (err)
    return err;

  if (f->type != type)
    return InvalidArgument;

  return 0;
}

//-----------------------------------------------------------------------------

int DBusTakeMessageEnd(Message* m)
{
  Field f;
  int err = takeSpecificField(m, &f, MessageEndField);
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeUInt8(Message* m, uint8_t* data)
{
  Field f;
  int err = takeSpecificField(m, &f, UInt8Field);
  if (data)
    *data = f.data.u8;
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeInt16(Message* m, int16_t* data)
{
  Field f;
  int err = takeSpecificField(m, &f, Int16Field);
  if (data)
    *data = f.data.i16;
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeUInt16(Message* m, uint16_t* data)
{
  Field f;
  int err = takeSpecificField(m, &f, UInt16Field);
  if (data)
    *data = f.data.u16;
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeInt32(Message* m, int32_t* data)
{
  Field f;
  int err = takeSpecificField(m, &f, Int32Field);
  if (data)
    *data = f.data.i32;
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeUInt32(Message* m, uint32_t* data)
{
  Field f;
  int err = takeSpecificField(m, &f, UInt32Field);
  if (data)
    *data = f.data.u32;
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeInt64(Message* m, int64_t* data)
{
  Field f;
  int err = takeSpecificField(m, &f, Int64Field);
  if (data)
    *data = f.data.i64;
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeUInt64(Message* m, uint64_t* data)
{
  Field f;
  int err = takeSpecificField(m, &f, UInt64Field);
  if (data)
    *data = f.data.u64;
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeDouble(Message* m, double* data)
{
  Field f;
  int err = takeSpecificField(m, &f, DoubleField);
  if (data)
    *data = f.data.d;
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeString(Message* m, const char** str, size_t* size)
{
  Field f;
  int err = takeSpecificField(m, &f, StringField);
  if (str)
    *str = f.data.string.str;
  if (size)
    *size = f.data.string.size;
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeObjectPath(Message* m, const char** str, size_t* size)
{
  Field f;
  int err = takeSpecificField(m, &f, ObjectPathField);
  if (str)
    *str = f.data.objectPath.str;
  if (size)
    *size = f.data.objectPath.size;
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeSignature(Message* m, const char** str, size_t* size)
{
  Field f;
  int err = takeSpecificField(m, &f, SignatureField);
  if (str)
    *str = f.data.signature.str;
  if (size)
    *size = f.data.signature.size;
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeArrayBegin(Message* m, int* scope, size_t* arrayDataSize)
{
  Field f;
  int err = takeSpecificField(m, &f, ArrayBeginField);
  if (scope)
    *scope = stackSize(m);
  if (arrayDataSize)
    *arrayDataSize = f.data.arrayDataSize;
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeArrayEnd(Message* m)
{
  Field f;
  int err = takeSpecificField(m, &f, ArrayEndField);
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeStructBegin(Message* m, int* scope)
{
  Field f;
  int err = takeSpecificField(m, &f, StructBeginField);
  if (scope)
    *scope = stackSize(m);
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeStructEnd(Message* m)
{
  Field f;
  int err = takeSpecificField(m, &f, StructEndField);
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeDictEntryBegin(Message* m, int* scope)
{
  Field f;
  int err = takeSpecificField(m, &f, DictEntryBeginField);
  if (scope)
    *scope = stackSize(m);
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeDictEntryEnd(Message* m)
{
  Field f;
  int err = takeSpecificField(m, &f, DictEntryEndField);
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeVariantBegin(Message* m,
    int* scope,
    const char** variantType,
    size_t* variantTypeSize)
{
  Field f;
  int err = takeSpecificField(m, &f, VariantBeginField);
  if (scope)
    *scope = stackSize(m);
  if (variantType)
    *variantType = f.data.variantType.str;
  if (variantTypeSize)
    *variantTypeSize = f.data.variantType.size;
  return err;
}

//-----------------------------------------------------------------------------

int DBusTakeVariantEnd(Message* m)
{
  Field f;
  int err = takeSpecificField(m, &f, VariantEndField);
  return err;
}

//-----------------------------------------------------------------------------

