#pragma once

#include "Common.h"


#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------

typedef struct
{
  const char* str;
  size_t      size;
} DBusStringRef;

struct Field
{
  enum FieldType type;
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
    DBusStringRef   string;
    DBusStringRef   objectPath;
    DBusStringRef   signature;
    DBusStringRef   variantType;
    size_t          arrayDataSize;
  } data;
};

//-----------------------------------------------------------------------------

typedef struct DBusMessage_ DBusMessage;

DBusMessageType DBusGetMessageType(DBusMessage* m);

const char* DBusGetPath(DBusMessage* m, int* len = NULL);
const char* DBusGetInterface(DBusMessage* m, int* len = NULL);
const char* DBusGetSender(DBusMessage* m, int* len = NULL);
const char* DBusGetDestination(DBusMessage* m, int* len = NULL);
const char* DBusGetMember(DBusMessage* m, int* len = NULL);
uint32_t    DBusGetSerial(DBusMessage* m);

const char* DBusGetSignature(DBusMessage* m);

bool DBusIsScopeAtEnd(DBusMessage* m, int scope);
int DBusTakeField(DBusMessage* m, Field* field = NULL);

int DBusTakeMessageEnd(DBusMessage* m);

int DBusTakeUInt8(DBusMessage* m, uint8_t* data = NULL);

int DBusTakeInt16(DBusMessage* m, int16_t* data = NULL);
int DBusTakeUInt16(DBusMessage* m, uint16_t* data = NULL);

int DBusTakeInt32(DBusMessage* m, int32_t* data = NULL);
int DBusTakeUInt32(DBusMessage* m, uint32_t* data = NULL);

int DBusTakeInt64(DBusMessage* m, int64_t* data = NULL);
int DBusTakeUInt64(DBusMessage* m, uint64_t* data = NULL);

int DBusTakeDouble(DBusMessage* m, double* data = NULL);

int DBusTakeString(DBusMessage* m, const char** str = NULL, size_t* size = NULL);
int DBusTakeObjectPath(DBusMessage* m, const char** str = NULL, size_t* size = NULL);
int DBusTakeSignature(DBusMessage* m, const char** str = NULL, size_t* size = NULL);

int DBusTakeArrayBegin(DBusMessage* m, int* scope = NULL, size_t* arrayDataSize = NULL);
int DBusTakeArrayEnd(DBusMessage* m);

int DBusTakeStructBegin(DBusMessage* m, int* scope = NULL);
int DBusTakeStructEnd(DBusMessage* m);

int DBusTakeDictEntryBegin(DBusMessage* m, int* scope = NULL);
int DBusTakeDictEntryEnd(DBusMessage* m);

int DBusTakeVariantBegin(DBusMessage* m, int* scope = NULL, const char** variantType = NULL, size_t* variantSize = NULL);
int DBusTakeVariantEnd(DBusMessage* m);

//-----------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif


