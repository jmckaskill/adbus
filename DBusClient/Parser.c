#include "Parser.h"

#include "Misc_p.h"


static const char kHeaderType[] = "a(yv)";

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct DBusParser_
{
  DBusMessage         message;
  DBusParserCallback  callback;
  void*               callbackData;
};

//-----------------------------------------------------------------------------

DBusParser* DBusCreateParser()
{
  DBusParser* p = calloc(1, sizeof(DBusParser));
}

//-----------------------------------------------------------------------------

void DBusFreeParser(DBusParser* p)
{
  free(p);
}

//-----------------------------------------------------------------------------

static int processData(Message* m, uint8_t* data, size_t dataSize, size_t* dataUsed);

int DBusParse(DBusParser* p, uint8_t* data, size_t dataSize, size_t* dataUsed)
{
  int err = 0;
  *dataUsed = 0;
  while(*dataUsed < dataSize)
  {
    err = processData(m, data + *dataUsed, dataSize, dataUsed);
    if (err)
      return err;
    
    p->callback(p->callbackData, &p->message);
  }
  return 0;
}

//-----------------------------------------------------------------------------

void DBusSetParserCallback(DBusParser* p, DBusParserCallback callback, void* data)
{
  p->callback     = callback;
  p->callbackData = data;
}

//-----------------------------------------------------------------------------

static int processHeaderFields(Message* m);

static int processData(Message* m, uint8_t* data, size_t dataSize, size_t* dataUsed)
{
  ASSERT(m && data && dataUsed);

  if ((uintptr_t(data) % 8) != 0)
    return DBusInvalidAlignment;

  if (dataSize < sizeof(MessageHeader))
    return DBusNeedMoreData;

  MessageHeader* header = reinterpret_cast<MessageHeader*>(data);

  if (header->version != 1)
    return DBusInvalidVersion;

  if (header->type == InvalidType)
    return DBusInvalidData;

  if (!(header->endianness == 'B' || header->endianness == 'l'))
    return DBusInvalidData;

  m->nativeEndian = (header->endianness == kNativeEndianness);

  // Get the non single byte fields out of the header
  size_t length = m->nativeEndian 
    ? header->length 
    : ENDIAN_CONVERT32(header->length);

  size_t headerFieldLength = m->nativeEndian 
    ? header->headerFieldLength 
    : ENDIAN_CONVERT32(header->headerFieldLength);

  m->serial = m->nativeEndian
    ? header->serial
    : ENDIAN_CONVERT32(header->serial);

  if (length > MaximumMessageLength)
    return DBusInvalidData;
  if (headerFieldLength > MaximumArrayLength)
    return DBusInvalidData;

  size_t headerSize = sizeof(MessageHeader)
                    + headerFieldLength;

  headerSize += headerSize % 8;

  size_t messageSize = headerSize + length;
  size_t fullMessageSize = messageSize + messageSize % 8;
  
  if (dataSize < fullMessageSize)
    return DBusNeedMoreData;

  *dataUsed  += fullMessageSize;

  if (header->type > MessageTypeMax)
    return IgnoredData;

  m->data    = data;
  m->dataEnd = data + messageSize;

  int err = 0;
  if ((err = processHeaderFields(m)))
    return err;

  if (!m->nativeEndian)
  {
    const char* oldSignature = m->signature;

    uint8_t* oldData = m->data;
    uint8_t* oldDataEnd = m->dataEnd;

    Field f;
    while (!err && f.type != MessageEndField)
    {
      err = DBusTakeField(m, &f);
    }
    // Parsing through the data changes the buffer as we go through
    // we want to do this so that the user can then cast say "ai" to uint32_t[]
    // Even though this means we parse through the data twice, it's not all 
    // that big a deal since if the endianness is different from ours, then
    // a different machine produced the data (and this should be minimal compared
    // to network overhead)
    m->nativeEndian = true;

    m->signature = oldSignature;
    m->data      = oldData;
    m->dataEnd   = oldDataEnd;
  }
  return err;
}

//-----------------------------------------------------------------------------

static int processHeaderFields(Message* m)
{
  m->data += sizeof(MessageInitialHeader);

  m->signature = "a(yv)";
  m->path = NULL;
  m->pathSize = 0;
  m->interface = NULL;
  m->interfaceSize = 0;
  m->member = NULL;
  m->memberSize = 0;
  m->errorName = NULL;
  m->errorNameSize = 0;
  m->destination = NULL;
  m->destinationSize = 0;
  m->sender = NULL;
  m->senderSize = 0;

  m->replySerial = 0;
  m->haveReplySerial = false;

  int err = 0;

  int arrayScope;
  err = takeArrayBegin(m, &arrayScope);

  while (!err && !DBusIsScopeAtEnd(m, arrayScope))
  {
    uint8_t fieldCode = InvalidFieldCode;
    int variantScope;

    err = err || takeUInt8(m, &fieldCode);
    err = err || takeVariantBegin(m, &variantScope);

    switch(fieldCode)
    {

    case ReplySerialFieldCode:
      err = err || takeUInt32(m, &m->replySerial);
      m->haveReplySerial = true;
      break;

    case InterfaceFieldCode:
      err = err || takeString(m, &m->interface, &m->interfaceSize);
      if (!_DBusIsValidInterfaceName(m->interface, m->interfaceSize))
        err = DBusInvalidData;
      break;
    case MemberFieldCode:
      err = err || takeString(m, &m->member, &m->memberSize);
      if (!_DBusIsValidMemberName(m->member, m->memberSize))
        err = DBusInvalidData;
      break;
    case DestinationFieldCode:
      err = err || takeString(m, &m->destination, &m->destinationSize);
      if (!_DBusIsValidBusName(m->destination, m->destinationSize))
        err = DBusInvalidData;
      break;
    case SenderFieldCode:
      err = err || takeString(m, &m->sender, &m->senderSize);
      if (!_DBusIsValidBusName(m->sender, m->senderSize))
        err = DBusInvalidData;
      break;

    case PathFieldCode:
      err = err || takeObjectPath(m, &m->path, &m->pathSize);
      break;
    case ErrorNameFieldCode:
      err = err || takeString(m, &m->errorName, &m->errorNameSize);
      break;
    case SignatureFieldCode:
      err = err || takeString(m, &m->signature);
      break;

    case InvalidFieldCode:
      err = DBusInvalidData;
      break;

    default:
      while (!err && !DBusIsScopeAtEnd(m, variantScope))
        err = DBusTakeField(m);
      break;
    }

    err = err || takeVariantEnd(m);
  }

  err = err || takeArrayEnd(m);

  if (err)
    return err;

  switch(m->messageType)
  {
  case MethodCallType:
    if (!m->path || !m->member)
      return DBusInvalidData;
    break;

  case MethodReturnType:
    if (!m->haveReplySerial)
      return DBusInvalidData;
    break;

  case ErrorType:
    if (!m->haveReplySerial || !m->errorName)
      return DBusInvalidData;
    break;

  case SignalType:
    if (!m->path || !m->interface || !m->member)
      return DBusInvalidData;
    break;

  default:
    // message type should've already been checked
    ASSERT(false);
    return DBusInvalidData;
  }

  if (!m->signature)
    m->signature = "";

  return 0;
}

//-----------------------------------------------------------------------------


