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
#include <string.h>


static const char kHeaderType[] = "a(yv)";

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

struct ADBusParser
{
  struct ADBusMessage  message;
  ADBusParserCallback  callback;
  void*               callbackData;
  uint8_t*            buffer;
  size_t              bufferSize;
  size_t              bufferAlloc;
};

// ----------------------------------------------------------------------------

struct ADBusParser* ADBusCreateParser()
{
  struct ADBusParser* p = (struct ADBusParser*)calloc(1, sizeof(struct ADBusParser));
  return p;
}

// ----------------------------------------------------------------------------

void ADBusFreeParser(struct ADBusParser* p)
{
  if (!p)
    return;

  free(p->buffer);
  free(p);
}

// ----------------------------------------------------------------------------

static int processData(struct ADBusMessage* m, uint8_t* data, size_t dataSize, size_t* dataUsed);

int ADBusParse(struct ADBusParser* p, const uint8_t* data, size_t dataSize)
{
  size_t insertOffset = p->bufferSize;
  ALLOC_GROW(uint8_t, p->buffer, insertOffset + dataSize, p->bufferAlloc);
  p->bufferSize += dataSize;

  memcpy(&p->buffer[insertOffset], data, dataSize);

  int err = 0;
  while(!err && p->bufferSize > 0)
  {
    size_t dataUsed = 0;
    int err = processData(&p->message, p->buffer, p->bufferSize, &dataUsed);
    if (!err)
      p->callback(p->callbackData, &p->message);
    if (err == ADBusIgnoredData)
      err = ADBusSuccess;
    assert(p->bufferSize >= dataUsed);
    memmove(p->buffer, p->buffer + dataUsed, p->bufferSize - dataUsed);
    p->bufferSize -= dataUsed;
  }

  return err;
}

// ----------------------------------------------------------------------------

void ADBusSetParserCallback(struct ADBusParser* p, ADBusParserCallback callback, void* data)
{
  p->callback     = callback;
  p->callbackData = data;
}

// ----------------------------------------------------------------------------

static int processHeaderFields(struct ADBusMessage* m);

static int processData(struct ADBusMessage* m, uint8_t* data, size_t dataSize, size_t* dataUsed)
{
  assert(m && data && dataUsed);

  // Check that we have enough correct data to decode the header
  if (((uintptr_t)data % 8) != 0)
    return ADBusInvalidAlignment;

  if (dataSize < sizeof(struct _ADBusMessageExtendedHeader))
    return ADBusNeedMoreData;

  struct _ADBusMessageHeader* header = (struct _ADBusMessageHeader*)(data);

  struct _ADBusMessageExtendedHeader* extended =
    (struct _ADBusMessageExtendedHeader*)(data);

  // Check the single byte header fields
  if (header->version != 1)
    return ADBusInvalidVersion;

  if (header->type == ADBusInvalidMessage)
    return ADBusInvalidData;

  if (!(header->endianness == 'B' || header->endianness == 'l'))
    return ADBusInvalidData;

  m->messageType  = (enum ADBusMessageType) header->type;
  m->nativeEndian = (header->endianness == _ADBusNativeEndianness);

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

  if (length > ADBusMaximumMessageLength)
    return ADBusInvalidData;
  if (headerFieldLength > ADBusMaximumArrayLength)
    return ADBusInvalidData;

  // Figure out the amount of data being used
  size_t headerSize = sizeof(struct _ADBusMessageExtendedHeader)
                    + headerFieldLength;

  headerSize = _ADBUS_ALIGN_VALUE(headerSize, 8);

  size_t messageSize = headerSize + length;

  if (dataSize < messageSize)
    return ADBusNeedMoreData;

  *dataUsed  += messageSize;

  if (header->type > ADBusMessageTypeMax)
    return ADBusIgnoredData;

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

    struct ADBusField f;
    f.type = ADBusInvalidField;
    while (!err && f.type != ADBusMessageEndField)
    {
      err = ADBusTakeField(m, &f);
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

  m->origSignature  = m->signature;
  m->origData       = m->data;
  m->origDataEnd    = m->dataEnd;

  return err;
}

// ----------------------------------------------------------------------------

static int processHeaderFields(struct ADBusMessage* m)
{
  size_t offset = sizeof(struct _ADBusMessageHeader);
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
  m->hasReplySerial = 0;

  int err = 0;

  // We can't store the signature in m->signature
  // since that is being used to parse the fields themselves
  const char* argumentSignature = NULL;

  unsigned int arrayScope;
  err = ADBusTakeArrayBegin(m, &arrayScope, NULL);

  while (!err && !ADBusIsScopeAtEnd(m, arrayScope))
  {
    uint8_t fieldCode = ADBusInvalidCode;
    unsigned int variantScope;

    err = err || ADBusTakeStructBegin(m, NULL);
    err = err || ADBusTakeUInt8(m, &fieldCode);
    err = err || ADBusTakeVariantBegin(m, &variantScope, NULL, NULL);

    switch(fieldCode)
    {

    case ADBusReplySerialCode:
      err = err || ADBusTakeUInt32(m, &m->replySerial);
      if (!err)
        m->hasReplySerial = 1;
      break;

    case ADBusInterfaceCode:
      err = err || ADBusTakeString(m, &m->interface, &m->interfaceSize);
      if (!err && !_ADBusIsValidInterfaceName(m->interface, m->interfaceSize))
        err = ADBusInvalidData;
      break;
    case ADBusMemberCode:
      err = err || ADBusTakeString(m, &m->member, &m->memberSize);
      if (!err && !_ADBusIsValidMemberName(m->member, m->memberSize))
        err = ADBusInvalidData;
      break;
    case ADBusDestinationCode:
      err = err || ADBusTakeString(m, &m->destination, &m->destinationSize);
      if (!err && !_ADBusIsValidBusName(m->destination, m->destinationSize))
        err = ADBusInvalidData;
      break;
    case ADBusSenderCode:
      err = err || ADBusTakeString(m, &m->sender, &m->senderSize);
      if (!err && !_ADBusIsValidBusName(m->sender, m->senderSize))
        err = ADBusInvalidData;
      break;

    case ADBusPathCode:
      err = err || ADBusTakeObjectPath(m, &m->path, &m->pathSize);
      break;
    case ADBusErrorNameCode:
      err = err || ADBusTakeString(m, &m->errorName, &m->errorNameSize);
      break;
    case ADBusSignatureCode:
      err = err || ADBusTakeSignature(m, &argumentSignature, NULL);
      break;

    case ADBusInvalidCode:
      err = ADBusInvalidData;
      break;

    default:
      while (!err && !ADBusIsScopeAtEnd(m, variantScope))
        err = ADBusTakeField(m, NULL);
      break;
    }

    err = err || ADBusTakeVariantEnd(m);
    err = err || ADBusTakeStructEnd(m);
  }

  err = err || ADBusTakeArrayEnd(m);

  if (err)
    return err;

  switch(m->messageType)
  {
  case ADBusMethodCallMessage:
    if (!m->path || !m->member)
      return ADBusInvalidData;
    break;

  case ADBusMethodReturnMessage:
    if (!m->hasReplySerial)
      return ADBusInvalidData;
    break;

  case ADBusErrorMessage:
    if (!m->hasReplySerial || !m->errorName)
      return ADBusInvalidData;
    break;

  case ADBusSignalMessage:
    if (!m->path || !m->interface || !m->member)
      return ADBusInvalidData;
    break;

  default:
    // message type should've already been checked
    assert(0);
    return ADBusInvalidData;
  }

  m->signature = argumentSignature;
  if (!m->signature)
    m->signature = "";

  return 0;
}

// ----------------------------------------------------------------------------


