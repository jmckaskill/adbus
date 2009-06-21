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

#include "Parser.h"

#include "Message_p.h"
#include "Misc_p.h"

#include <assert.h>


static const char kHeaderType[] = "a(yv)";

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

struct DBusParser
{
  struct DBusMessage  message;
  DBusParserCallback  callback;
  void*               callbackData;
};

// ----------------------------------------------------------------------------

struct DBusParser* DBusCreateParser()
{
  struct DBusParser* p = (struct DBusParser*)calloc(1, sizeof(struct DBusParser));
  return p;
}

// ----------------------------------------------------------------------------

void DBusFreeParser(struct DBusParser* p)
{
  free(p);
}

// ----------------------------------------------------------------------------

static int processData(struct DBusMessage* m, uint8_t* data, size_t dataSize, size_t* dataUsed);

int DBusParse(struct DBusParser* p, uint8_t* data, size_t dataSize, size_t* dataUsed)
{
  *dataUsed = 0;

  int err = processData(&p->message, data + *dataUsed, dataSize, dataUsed);
  if (err)
    return err;

  p->callback(p->callbackData, &p->message);

  return 0;
}

// ----------------------------------------------------------------------------

void DBusSetParserCallback(struct DBusParser* p, DBusParserCallback callback, void* data)
{
  p->callback     = callback;
  p->callbackData = data;
}

// ----------------------------------------------------------------------------

static int processHeaderFields(struct DBusMessage* m);

static int processData(struct DBusMessage* m, uint8_t* data, size_t dataSize, size_t* dataUsed)
{
  assert(m && data && dataUsed);

  // Check that we have enough correct data to decode the header
  if (((uintptr_t)data % 8) != 0)
    return DBusInvalidAlignment;

  if (dataSize < sizeof(struct _DBusMessageExtendedHeader))
    return DBusNeedMoreData;

  struct _DBusMessageHeader* header = (struct _DBusMessageHeader*)(data);

  struct _DBusMessageExtendedHeader* extended =
    (struct _DBusMessageExtendedHeader*)(data);

  // Check the single byte header fields
  if (header->version != 1)
    return DBusInvalidVersion;

  if (header->type == DBusInvalidMessage)
    return DBusInvalidData;

  if (!(header->endianness == 'B' || header->endianness == 'l'))
    return DBusInvalidData;

  m->messageType  = header->type;
  m->nativeEndian = (header->endianness == _DBusNativeEndianness);

  // Get the non single byte fields out of the header
  size_t length = m->nativeEndian
    ? header->length
    : ENDIAN_CONVERT32(header->length);

  size_t headerFieldLength = m->nativeEndian
    ? extended->headerFieldLength
    : ENDIAN_CONVERT32(extended->headerFieldLength);

  m->serial = m->nativeEndian
    ? header->serial
    : ENDIAN_CONVERT32(header->serial);

  if (length > DBusMaximumMessageLength)
    return DBusInvalidData;
  if (headerFieldLength > DBusMaximumArrayLength)
    return DBusInvalidData;

  // Figure out the amount of data being used
  size_t headerSize = sizeof(struct _DBusMessageExtendedHeader)
                    + headerFieldLength;

  headerSize = _DBUS_ALIGN_VALUE(headerSize, 8);

  size_t messageSize = headerSize + length;

  if (dataSize < messageSize)
    return DBusNeedMoreData;

  *dataUsed  += messageSize;

  if (header->type > DBusMessageTypeMax)
    return DBusSuccess;

  m->data    = data;
  m->dataEnd = data + messageSize;

  // Process header fields
  int err = 0;
  if ((err = processHeaderFields(m)))
    return err;

  // Fixups for non native endian
  if (!m->nativeEndian)
  {
    const char* oldSignature = m->signature;

    uint8_t* oldData = m->data;
    uint8_t* oldDataEnd = m->dataEnd;

    struct DBusField f;
    f.type = DBusInvalidField;
    while (!err && f.type != DBusMessageEndField)
    {
      err = DBusTakeField(m, &f);
    }
    // Parsing through the data changes the buffer as we go through
    // we want to do this so that the user can then cast say "ai" to uint32_t[]
    // Even though this means we parse through the data twice, it's not all
    // that big a deal since if the endianness is different from ours, then
    // a different machine produced the data (and this should be minimal compared
    // to network overhead)
    m->nativeEndian = 1;

    m->signature = oldSignature;
    m->data      = oldData;
    m->dataEnd   = oldDataEnd;
  }

  return err;
}

// ----------------------------------------------------------------------------

static int processHeaderFields(struct DBusMessage* m)
{
  size_t offset = sizeof(struct _DBusMessageHeader);
  m->data += offset;

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
  m->haveReplySerial = 0;

  int err = 0;

  // We can't store the signature in m->signature
  // since that is being used to parse the fields themselves
  const char* argumentSignature = NULL;

  unsigned int arrayScope;
  err = DBusTakeArrayBegin(m, &arrayScope, NULL);

  while (!err && !DBusIsScopeAtEnd(m, arrayScope))
  {
    uint8_t fieldCode = DBusInvalidCode;
    unsigned int variantScope;

    err = err || DBusTakeStructBegin(m, NULL);
    err = err || DBusTakeUInt8(m, &fieldCode);
    err = err || DBusTakeVariantBegin(m, &variantScope, NULL, NULL);

    switch(fieldCode)
    {

    case DBusReplySerialCode:
      err = err || DBusTakeUInt32(m, &m->replySerial);
      if (!err)
        m->haveReplySerial = 1;
      break;

    case DBusInterfaceCode:
      err = err || DBusTakeString(m, &m->interface, &m->interfaceSize);
      if (!err && !_DBusIsValidInterfaceName(m->interface, m->interfaceSize))
        err = DBusInvalidData;
      break;
    case DBusMemberCode:
      err = err || DBusTakeString(m, &m->member, &m->memberSize);
      if (!err && !_DBusIsValidMemberName(m->member, m->memberSize))
        err = DBusInvalidData;
      break;
    case DBusDestinationCode:
      err = err || DBusTakeString(m, &m->destination, &m->destinationSize);
      if (!err && !_DBusIsValidBusName(m->destination, m->destinationSize))
        err = DBusInvalidData;
      break;
    case DBusSenderCode:
      err = err || DBusTakeString(m, &m->sender, &m->senderSize);
      if (!err && !_DBusIsValidBusName(m->sender, m->senderSize))
        err = DBusInvalidData;
      break;

    case DBusPathCode:
      err = err || DBusTakeObjectPath(m, &m->path, &m->pathSize);
      break;
    case DBusErrorNameCode:
      err = err || DBusTakeString(m, &m->errorName, &m->errorNameSize);
      break;
    case DBusSignatureCode:
      err = err || DBusTakeSignature(m, &argumentSignature, NULL);
      break;

    case DBusInvalidCode:
      err = DBusInvalidData;
      break;

    default:
      while (!err && !DBusIsScopeAtEnd(m, variantScope))
        err = DBusTakeField(m, NULL);
      break;
    }

    err = err || DBusTakeVariantEnd(m);
    err = err || DBusTakeStructEnd(m);
  }

  err = err || DBusTakeArrayEnd(m);

  if (err)
    return err;

  switch(m->messageType)
  {
  case DBusMethodCallMessage:
    if (!m->path || !m->member)
      return DBusInvalidData;
    break;

  case DBusMethodReturnMessage:
    if (!m->haveReplySerial)
      return DBusInvalidData;
    break;

  case DBusErrorMessage:
    if (!m->haveReplySerial || !m->errorName)
      return DBusInvalidData;
    break;

  case DBusSignalMessage:
    if (!m->path || !m->interface || !m->member)
      return DBusInvalidData;
    break;

  default:
    // message type should've already been checked
    assert(0);
    return DBusInvalidData;
  }

  m->signature = argumentSignature;
  if (!m->signature)
    m->signature = "";

  return 0;
}

// ----------------------------------------------------------------------------


