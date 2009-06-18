#pragma once

#include "Common.h"


#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------------------------------

typedef struct DBusParser_  DBusParser;
typedef struct DBusMessage_ DBusMessage;

DBusParser* DBusCreateParser();
void DBusFreeParser(DBusParser* parser);

enum DBusParseError
{
  DBusInternalError = -1,
  DBusSuccess = 0,
  DBusNeedMoreData,
  DBusIgnoredData,
  DBusInvalidData,
  DBusInvalidVersion,
  DBusInvalidAlignment,
  DBusInvalidArgument,
};
// Returns an error code or 0 on none
// if sizeUsed < size then the remaining data needs to 
// be re-parsed after more data is available
int  DBusParse(uint8_t* data, size_t dataSize, size_t* dataUsed);

typedef void (*DBusParserCallback)(void* /*userData*/, DBusMessage*);
void DBusSetParserCallback(DBusParserCallback callback, void* userData);

//-----------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif
