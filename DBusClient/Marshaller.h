#pragma once

#include "Common.h"


#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------

typedef void (*DBusSendCallback)(void* user, uint8_t* data, size_t len);

struct DBusMarshaller_;
typedef struct DBusMarshaller_ DBusMarshaller;

DBusMarshaller* DBusCreateMarshaller();
void DBusFreeMarshaller(DBusMarshaller* m);

void DBusClearMarshaller(DBusMarshaller* m);
void DBusSetCallback(DBusMarshaller* m, DBusSendCallback callback, void* userData);
void DBusSendMessage(DBusMarshaller* m);

void DBusSetMessageType(DBusMarshaller* m, MessageType type);
void DBusSetReplySerial(DBusMarshaller* m, uint32_t reply);
void DBusSetPath(DBusMarshaller* m, const char* path, int size = -1);
void DBusSetInterface(DBusMarshaller* m, const char* path, int size = -1);
void DBusSetMember(DBusMarshaller* m, const char* path, int size = -1);
void DBusSetErrorName(DBusMarshaller* m, const char* path, int size = -1);
void DBusSetDestination(DBusMarshaller* m, const char* path, int size = -1);
void DBusSetSender(DBusMarshaller* m, const char* path, int size = -1);

void DBusBeginArgument(DBusMarshaller* m, const char* type, int typeSize = -1);
void DBusEndArgument(DBusMarshaller* m);

void DBusAppendBoolean(DBusMarshaller* m, uint32_t data);
void DBusAppendUInt8(DBusMarshaller* m, uint8_t data);
void DBusAppendInt16(DBusMarshaller* m, int16_t data);
void DBusAppendUInt16(DBusMarshaller* m, uint16_t data);
void DBusAppendInt32(DBusMarshaller* m, int32_t data);
void DBusAppendUInt32(DBusMarshaller* m, uint32_t data);
void DBusAppendInt64(DBusMarshaller* m, int64_t data);
void DBusAppendUInt64(DBusMarshaller* m, uint64_t data);

void DBusAppendDouble(DBusMarshaller* m, double data);

void DBusAppendString(DBusMarshaller* m, const char* str, int size = -1);
void DBusAppendObjectPath(DBusMarshaller* m, const char* str, int size = -1);
void DBusAppendSignature(DBusMarshaller* m, const char* str, int size = -1);

void DBusBeginArray(DBusMarshaller* m);
void DBusEndArray(DBusMarshaller* m);

void DBusBeginStruct(DBusMarshaller* m);
void DBusEndStruct(DBusMarshaller* m);

void DBusBeginDictEntry(DBusMarshaller* m);
void DBusEndDictEntry(DBusMarshaller* m);

void DBusBeginVariant(DBusMarshaller* m, const char* type);
void DBusEndVariant(DBusMarshaller* m);

//-----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
