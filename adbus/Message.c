/* vim: ts=4 sw=4 sts=4 et
 *
 * Copyright (c) 2009 James R. McKaskill
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 */

#include "Message.h"
#include "Message_p.h"
#include "Misc_p.h"
#include "Marshaller.h"
#include "Iterator.h"
#include "str.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
# define strdup _strdup
#endif

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

size_t ADBusNextMessageSize(const uint8_t* data, size_t size)
{
    // Check that we have enough correct data to decode the header
    if (size < sizeof(struct ADBusExtendedHeader_))
        return 0;

    struct ADBusExtendedHeader_* header = (struct ADBusExtendedHeader_*) data;

    uint nativeEndian = (header->endianness == ADBusNativeEndianness_);

    size_t length = nativeEndian
        ? header->length
        : ENDIAN_CONVERT32(header->length);

    size_t headerFieldLength = nativeEndian
        ? header->headerFieldLength
        : ENDIAN_CONVERT32(header->headerFieldLength);

    size_t headerSize = sizeof(struct ADBusExtendedHeader_) + headerFieldLength;
    // Note the header size is 8 byte aligned even if there is no argument data
    size_t messageSize = ADBUS_ALIGN_VALUE_(size_t, headerSize, 8) + length;

    return messageSize;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

struct ADBusMessage* ADBusCreateMessage()
{
    struct ADBusMessage* m = NEW(struct ADBusMessage);
    m->marshaller = ADBusCreateMarshaller();
    m->argumentMarshaller = ADBusCreateMarshaller();
    m->headerIterator = ADBusCreateIterator();
    return m;
}

// ----------------------------------------------------------------------------

void ADBusFreeMessage(struct ADBusMessage* m)
{
    if (!m)
        return;

    ADBusFreeMarshaller(m->marshaller);
    ADBusFreeMarshaller(m->argumentMarshaller);
    ADBusFreeIterator(m->headerIterator);
    free(m->path);
    free(m->interface);
    free(m->member);
    free(m->errorName);
    free(m->destination);
    free(m->sender);
    free(m->signature);
    free(m);
}

// ----------------------------------------------------------------------------

void ADBusResetMessage(struct ADBusMessage* m)
{
    ADBusResetMarshaller(m->marshaller);
    ADBusResetMarshaller(m->argumentMarshaller);
    m->argumentOffset = 0;
    m->hasReplySerial = 0;
    m->replySerial = 0;
#define FREE(x) free(x); x = NULL;
    FREE(m->path);
    FREE(m->interface);
    FREE(m->member);
    FREE(m->errorName);
    FREE(m->destination);
    FREE(m->sender);
    FREE(m->signature);
#undef FREE
}

// ----------------------------------------------------------------------------

#ifdef WIN32
#define strdup _strdup
#endif

int ADBusSetMessageData(struct ADBusMessage* m, const uint8_t* data, size_t size)
{
    // Reset the message internals first
    ADBusResetMessage(m);

    // We need to copy into the marshaller so that we can hold onto the data
    // For parsing we will use that copy as it will force 8 byte alignment
    ADBusSetMarshalledData(m->marshaller, NULL, 0, data, size);
    ADBusGetMarshalledData(m->marshaller, NULL, NULL, &data, &size);

    // Check that we have enough correct data to decode the header
    if (size < sizeof(struct ADBusExtendedHeader_))
        return ADBusInvalidData;

    struct ADBusExtendedHeader_* header = (struct ADBusExtendedHeader_*) data;

    // Check the single byte header fields
    if (header->version != ADBusMajorProtocolVersion_)
        return ADBusInvalidData;
    if (header->type == ADBusInvalidMessage)
        return ADBusInvalidData;
    if (header->endianness != 'B' && header->endianness != 'l')
        return ADBusInvalidData;

    m->messageType = (enum ADBusMessageType) header->type;
    m->nativeEndian = (header->endianness == ADBusNativeEndianness_);

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

    if (length > ADBusMaximumMessageLength)
        return ADBusInvalidData;
    if (headerFieldLength > ADBusMaximumArrayLength)
        return ADBusInvalidData;

    // Figure out the message size and see if we have the right amount
    size_t headerSize = sizeof(struct ADBusExtendedHeader_) + headerFieldLength;
    headerSize = ADBUS_ALIGN_VALUE_(size_t, headerSize, 8);
    size_t messageSize = headerSize + length;

    if (size != messageSize)
        return ADBusInvalidData;

    m->argumentOffset = messageSize - length;

    // Should we parse this?
    if (header->type > ADBusMessageTypeMax) {
        ADBusResetMessage(m);
        return ADBusSuccess;
    }

    // Parse the header fields
    const uint8_t* fieldBegin = data + sizeof(struct ADBusHeader_);
    size_t fieldSize = headerSize - sizeof(struct ADBusHeader_);

    struct ADBusIterator* iterator =  m->headerIterator;
    ADBusResetIterator(iterator, "a(yv)", -1, fieldBegin, fieldSize);
    ADBusSetIteratorEndianness(iterator, (enum ADBusEndianness) header->endianness);
    struct ADBusField field;

#define ITERATE(M, TYPE, PFIELD)                        \
    {                                                   \
        int err = ADBusIterate(iterator, PFIELD);    \
        if (err || (PFIELD)->type != TYPE)                \
            return ADBusInvalidData;                    \
    }
    
    ITERATE(m, ADBusArrayBeginField, &field);
    uint arrayScope = field.scope;
    while (!ADBusIsScopeAtEnd(iterator, arrayScope))
    {
        ITERATE(m, ADBusStructBeginField, &field);
        ITERATE(m, ADBusUInt8Field, &field);
        uint8_t code = field.u8;
        ITERATE(m, ADBusVariantBeginField, &field);

        int err = ADBusIterate(iterator, &field);
        if (err)
            return ADBusInvalidData;

        switch(code)
        {
            case ADBusReplySerialCode:
                if (field.type != ADBusUInt32Field)
                    return ADBusInvalidData;
                m->replySerial = field.u32;
                m->hasReplySerial = 1;
                break;
            case ADBusSignatureCode:
                if (field.type != ADBusSignatureField)
                    return ADBusInvalidData;
                m->signature = strndup(field.string, field.size);
                break;
            case ADBusPathCode:
                if (field.type != ADBusObjectPathField)
                    return ADBusInvalidData;
                m->path = strndup(field.string, field.size);
                break;
            case ADBusInterfaceCode:
                if (field.type != ADBusStringField)
                    return ADBusInvalidData;
                m->interface = strndup(field.string, field.size);
                break;
            case ADBusMemberCode:
                if (field.type != ADBusStringField)
                    return ADBusInvalidData;
                m->member = strndup(field.string, field.size);
                break;
            case ADBusErrorNameCode:
                if (field.type != ADBusStringField)
                    return ADBusInvalidData;
                m->errorName = strndup(field.string, field.size);
                break;
            case ADBusDestinationCode:
                if (field.type != ADBusStringField)
                    return ADBusInvalidData;
                m->destination = strndup(field.string, field.size);
                break;
            case ADBusSenderCode:
                if (field.type != ADBusStringField)
                    return ADBusInvalidData;
                m->sender = strndup(field.string, field.size);
                break;
            default:
                break;
        }
        ITERATE(m, ADBusVariantEndField, &field);
        ITERATE(m, ADBusStructEndField, &field);
    }
    ITERATE(m, ADBusArrayEndField, &field);
#undef ITERATE

    // Check that we have required fields
    if (m->messageType == ADBusMethodCallMessage) {
        if (!m->path || !m->member)
            return ADBusInvalidData;

    } else if (m->messageType == ADBusMethodReturnMessage) {
        if (!m->replySerial)
            return ADBusInvalidData;

    } else if (m->messageType == ADBusErrorMessage) {
        if (!m->errorName)
            return ADBusInvalidData;

    } else if (m->messageType == ADBusSignalMessage) {
        if (!m->interface || !m->member)
            return ADBusInvalidData;

    } else {
        assert(0);
    }

    // We need to convert the arguments to native endianness so that they
    // can be pulled out as a memory block
    // We do this by iterating over the arguments and remarshall them into the
    // argument marshaller
    if (!m->nativeEndian && m->signature) {
        struct ADBusMarshaller* mar = ADBusArgumentMarshaller(m);
        // Realistically we should break up the signature into the seperate
        // arguments and begin and end each one with a seperate signature, but
        // the iterator doesn't return begin and end argument fields
        ADBusBeginArgument(mar, m->signature, -1);

        ADBusArgumentIterator(m, iterator);
        ADBusSetIteratorEndianness(iterator, (enum ADBusEndianness) header->endianness);

        int err = ADBusAppendIteratorData(mar, iterator, 0);
        if (err)
            return err;

        ADBusEndArgument(mar);
    }

    return 0;
}

// ----------------------------------------------------------------------------
// Getter functions
// ----------------------------------------------------------------------------

static void AppendString(struct ADBusMessage* m, uint8_t code, const char* field)
{
    if (!field)
        return;
    ADBusBeginStruct(m->marshaller);
    ADBusAppendUInt8(m->marshaller, code);
    ADBusBeginVariant(m->marshaller, "s", -1);
    ADBusAppendString(m->marshaller, field, -1);
    ADBusEndVariant(m->marshaller);
    ADBusEndStruct(m->marshaller);
}

static void AppendSignature(struct ADBusMessage* m, uint8_t code, const char* field)
{
    if (!field)
        return;
    ADBusBeginStruct(m->marshaller);
    ADBusAppendUInt8(m->marshaller, code);
    ADBusBeginVariant(m->marshaller, "g", -1);
    ADBusAppendSignature(m->marshaller, field, -1);
    ADBusEndVariant(m->marshaller);
    ADBusEndStruct(m->marshaller);
}

static void AppendObjectPath(struct ADBusMessage* m, uint8_t code, const char* field)
{
    if (!field)
        return;
    ADBusBeginStruct(m->marshaller);
    ADBusAppendUInt8(m->marshaller, code);
    ADBusBeginVariant(m->marshaller, "o", -1);
    ADBusAppendObjectPath(m->marshaller, field, -1);
    ADBusEndVariant(m->marshaller);
    ADBusEndStruct(m->marshaller);
}

static void AppendUInt32(struct ADBusMessage* m, uint8_t code, uint32_t field)
{
    ADBusBeginStruct(m->marshaller);
    ADBusAppendUInt8(m->marshaller, code);
    ADBusBeginVariant(m->marshaller, "u", -1);
    ADBusAppendUInt32(m->marshaller, field);
    ADBusEndVariant(m->marshaller);
    ADBusEndStruct(m->marshaller);
}

static void BuildMessage(struct ADBusMessage* m)
{
    const char* signature;
    size_t sigsize;
    const uint8_t* argumentData;
    size_t argumentSize;
    ADBusGetMarshalledData(m->argumentMarshaller,
                           &signature, &sigsize,
                           &argumentData, &argumentSize);

    ADBusResetMarshaller(m->marshaller);
    struct ADBusHeader_ header;

    header.endianness   = ADBusNativeEndianness_;
    header.type         = (uint8_t) m->messageType;
    header.flags        = m->flags;
    header.version      = ADBusMajorProtocolVersion_;
    header.length       = argumentSize;
    header.serial       = m->serial;
    ADBusAppendData(m->marshaller, (const uint8_t*) &header, sizeof(struct ADBusHeader_));

    ADBusBeginArgument(m->marshaller, "a(yv)", -1);
    ADBusBeginArray(m->marshaller);
    AppendString(m, ADBusInterfaceCode, m->interface);
    AppendString(m, ADBusMemberCode, m->member);
    AppendString(m, ADBusErrorNameCode, m->errorName);
    AppendString(m, ADBusDestinationCode, m->destination);
    AppendString(m, ADBusSenderCode, m->sender);
    AppendObjectPath(m, ADBusPathCode, m->path);

    if (m->hasReplySerial)
      AppendUInt32(m, ADBusReplySerialCode, m->replySerial);

    if (argumentData && argumentSize > 0)
      AppendSignature(m, ADBusSignatureCode, signature);

    ADBusEndArray(m->marshaller);
    ADBusEndArgument(m->marshaller);

    size_t headerSize;
    ADBusGetMarshalledData(m->marshaller, NULL, NULL, NULL, &headerSize);

    // The header is 8 byte padded even if there is no argument data
    static uint8_t paddingData[8]; // since static, guarenteed to be 0s
    size_t padding = ADBUS_ALIGN_VALUE_(size_t, headerSize, 8) - headerSize;
    if (padding != 0)
      ADBusAppendData(m->marshaller, paddingData, padding);

    if (argumentData && argumentSize > 0)
        ADBusAppendData(m->marshaller, argumentData, argumentSize);
}

void ADBusGetMessageData(struct ADBusMessage* m, const uint8_t** pdata, size_t* psize)
{
    size_t size;
    ADBusGetMarshalledData(m->marshaller, NULL, NULL, NULL, &size);
    if (size == 0)
        BuildMessage(m);
    ADBusGetMarshalledData(m->marshaller, NULL, NULL, pdata, psize);
}

// ----------------------------------------------------------------------------

const char* ADBusGetPath(struct ADBusMessage* m, size_t* len)
{
    if (len)
        *len = m->path ? strlen(m->path) : 0;
    return m->path;
}

// ----------------------------------------------------------------------------

const char* ADBusGetInterface(struct ADBusMessage* m, size_t* len)
{
    if (len)
        *len = m->interface ? strlen(m->interface) : 0;
    return m->interface;
}

// ----------------------------------------------------------------------------

const char* ADBusGetSender(struct ADBusMessage* m, size_t* len)
{
    if (len)
        *len = m->sender ? strlen(m->sender) : 0;
    return m->sender;
}

// ----------------------------------------------------------------------------

const char* ADBusGetDestination(struct ADBusMessage* m, size_t* len)
{
    if (len)
        *len = m->destination ? strlen(m->destination) : 0;
    return m->destination;
}

// ----------------------------------------------------------------------------

const char* ADBusGetMember(struct ADBusMessage* m, size_t* len)
{
    if (len)
        *len = m->member ? strlen(m->member) : 0;
    return m->member;
}

// ----------------------------------------------------------------------------

const char* ADBusGetErrorName(struct ADBusMessage* m, size_t* len)
{
    if (len)
        *len = m->errorName ? strlen(m->errorName) : 0;
    return m->errorName;
}

// ----------------------------------------------------------------------------

const char* ADBusGetSignature(struct ADBusMessage* m, size_t* len)
{
    if (len)
        *len = m->signature ? strlen(m->signature) : 0;
    return m->signature;
}

// ----------------------------------------------------------------------------

enum ADBusMessageType ADBusGetMessageType(struct ADBusMessage* m)
{
    return m->messageType;
}

// ----------------------------------------------------------------------------

uint8_t ADBusGetFlags(struct ADBusMessage* m)
{
    return m->flags;
}

// ----------------------------------------------------------------------------

uint32_t ADBusGetSerial(struct ADBusMessage* m)
{
    return m->serial;
}

// ----------------------------------------------------------------------------

uint ADBusHasReplySerial(struct ADBusMessage* m)
{
    return m->hasReplySerial;
}

// ----------------------------------------------------------------------------

uint32_t ADBusGetReplySerial(struct ADBusMessage* m)
{
    return m->replySerial;
}

// ----------------------------------------------------------------------------

void ADBusGetArgumentData(struct ADBusMessage* m, const uint8_t** data, size_t* size)
{
    ADBusGetMessageData(m, data, size);
    if (data)
        *data += m->argumentOffset;
    if (size)
        *size -= m->argumentOffset;
}

// ----------------------------------------------------------------------------
// Setter functions
// ----------------------------------------------------------------------------

void ADBusSetMessageType(struct ADBusMessage* m, enum ADBusMessageType type)
{
    m->messageType = type;
}

// ----------------------------------------------------------------------------

void ADBusSetSerial(struct ADBusMessage* m, uint32_t serial)
{
    m->serial = serial;
}

// ----------------------------------------------------------------------------

void ADBusSetFlags(struct ADBusMessage* m, uint8_t flags)
{
    m->flags = flags;
}

// ----------------------------------------------------------------------------

void ADBusSetReplySerial(struct ADBusMessage* m, uint32_t reply)
{
    m->replySerial = reply;
    m->hasReplySerial = 1;
}

// ----------------------------------------------------------------------------

void ADBusSetPath(struct ADBusMessage* m, const char* path, int size)
{
    free(m->path);
    m->path = (size >= 0)
            ? strndup(path, size)
            : strdup(path);
}

// ----------------------------------------------------------------------------

void ADBusSetInterface(struct ADBusMessage* m, const char* interface, int size)
{
    free(m->interface);
    m->interface = (size >= 0)
                 ? strndup(interface, size)
                 : strdup(interface);
}

// ----------------------------------------------------------------------------

void ADBusSetMember(struct ADBusMessage* m, const char* member, int size)
{
    free(m->member);
    m->member = (size >= 0)
              ? strndup(member, size)
              : strdup(member);
}

// ----------------------------------------------------------------------------

void ADBusSetErrorName(struct ADBusMessage* m, const char* errorName, int size)
{
    free(m->errorName);
    m->errorName = (size >= 0)
                 ? strndup(errorName, size)
                 : strdup(errorName);
}

// ----------------------------------------------------------------------------

void ADBusSetDestination(struct ADBusMessage* m, const char* destination, int size)
{
    free(m->destination);
    m->destination = (size >= 0)
                   ? strndup(destination, size)
                   : strdup(destination);
}

// ----------------------------------------------------------------------------

void ADBusSetSender(struct ADBusMessage* m, const char* sender, int size)
{
    free(m->sender);
    m->sender = (size >= 0)
              ? strndup(sender, size)
              : strdup(sender);
}

// ----------------------------------------------------------------------------

struct ADBusMarshaller* ADBusArgumentMarshaller(struct ADBusMessage* m)
{
    return m->argumentMarshaller;
}

// ----------------------------------------------------------------------------

void ADBusArgumentIterator(struct ADBusMessage* m, struct ADBusIterator* iterator)
{
    const char* signature;
    size_t sigsize;
    const uint8_t* data;
    size_t datasize;
    if (m->argumentOffset > 0 && m->nativeEndian) {
        ADBusGetMarshalledData(m->marshaller, NULL, NULL, &data, &datasize);
        signature = m->signature;
        sigsize = signature ? strlen(signature) : 0;
        data += m->argumentOffset;
        datasize -= m->argumentOffset;

    } else {
        ADBusGetMarshalledData(m->argumentMarshaller, &signature, &sigsize, &data, &datasize);
    }
    ADBusResetIterator(iterator, signature, sigsize, data, datasize);
}

// ----------------------------------------------------------------------------

static void PrintStringField(
        str_t* str,
        const char* field,
        const char* what)
{
    if (field)
        str_printf(str, "\n%-15s \"%s\"", what, field);
}

static void LogField(str_t* str, struct ADBusIterator* i, struct ADBusField* field);
static void LogScope(str_t* str, struct ADBusIterator* i, enum ADBusFieldType end)
{
    uint first = 1;
    struct ADBusField field;
    while (!ADBusIterate(i, &field) && field.type != end && field.type != ADBusEndField) {
        if (!first)
            str_printf(str, ", ");
        first = 0;
        LogField(str, i, &field);
    }
}

static void LogField(str_t* str, struct ADBusIterator* i, struct ADBusField* field)
{
    enum ADBusFieldType type = field->type;
    switch (type)
    {
    case ADBusUInt8Field:
        str_printf(str, "%d", (int) field->u8);
        break;
    case ADBusBooleanField:
        str_printf(str, "%s", field->b ? "true" : "false");
        break;
    case ADBusInt16Field:
        str_printf(str, "%d", (int) field->i16);
        break;
    case ADBusUInt16Field:
        str_printf(str, "%d", (int) field->u16);
        break;
    case ADBusInt32Field:
        str_printf(str, "%d", (int) field->i32);
        break;
    case ADBusUInt32Field:
        str_printf(str, "%u", (unsigned int) field->u32);
        break;
    case ADBusInt64Field:
        str_printf(str, "%lld", (long long int) field->i64);
        break;
    case ADBusUInt64Field:
        str_printf(str, "%llu", (long long unsigned int) field->u64);
        break;
    case ADBusDoubleField:
        str_printf(str, "%.15g", field->d);
        break;
    case ADBusStringField:
    case ADBusObjectPathField:
    case ADBusSignatureField:
        str_printf(str, "\"%s\"", field->string);
        break;
    case ADBusArrayBeginField:
        str_printf(str, "[ ");
        LogScope(str, i, ADBusArrayEndField);
        str_printf(str, " ]");
        break;
    case ADBusStructBeginField:
        str_printf(str, "( ");
        LogScope(str, i, ADBusStructEndField);
        str_printf(str, " )");
        break;
    case ADBusDictEntryBeginField:
        str_printf(str, "{ ");
        LogScope(str, i, ADBusDictEntryEndField);
        str_printf(str, " }");
        break;
    case ADBusVariantBeginField:
        str_printf(str, "<%s>{ ", field->string);
        LogScope(str, i, ADBusVariantEndField);
        str_printf(str, " }");
        break;
    default:
        assert(0);
    }
}

char* ADBusNewMessageSummary(struct ADBusMessage* m, size_t* size)
{
    str_t str = NULL;
    size_t messageSize;
    ADBusGetMarshalledData(m->marshaller, NULL, NULL, NULL, &messageSize);
    messageSize -= m->argumentOffset;

    enum ADBusMessageType type = ADBusGetMessageType(m);
    if (type == ADBusMethodCallMessage)
      str_printf(&str, "Type method_call (1), ");
    else if (type == ADBusMethodReturnMessage)
      str_printf(&str, "Type method_return (2), ");
    else if (type == ADBusErrorMessage)
      str_printf(&str, "Type error (3), ");
    else if (type == ADBusSignalMessage)
      str_printf(&str, "Type signal (4), ");
    else
      str_printf(&str, "Type unknown (%d), ", (int) type);

    str_printf(&str, "Flags %d, Length %d, Serial %d",
            (int) ADBusGetFlags(m),
            (int) messageSize,
            (int) ADBusGetSerial(m));
    PrintStringField(&str, m->path, "Path");
    PrintStringField(&str, m->interface, "Interface");
    PrintStringField(&str, m->member, "Member");
    PrintStringField(&str, m->errorName, "Error name");
    if (m->hasReplySerial)
        str_printf(&str, "\n%-15s %d", "Reply serial", m->replySerial);
    PrintStringField(&str, m->destination, "Destination");
    PrintStringField(&str, m->sender, "Sender");
    PrintStringField(&str, m->signature, "Signature");

    int argnum = 0;
    struct ADBusIterator* iter = m->headerIterator;
    ADBusArgumentIterator(m, iter);
    struct ADBusField field;
    while (!ADBusIterate(iter, &field) && field.type != ADBusEndField) {
        str_printf(&str, "\nArgument %2d     ", argnum++);
        LogField(&str, iter, &field);
    }
    if (size)
        *size = str_size(&str);
    return str;
}

void ADBusFreeMessageSummary(char* str)
{
    str_free(&str);
}


