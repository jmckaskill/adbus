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

#include <adbus/adbus.h>
#include "message.h"
#include "misc.h"

#include "memory/kstring.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum
{
    HEADER_OBJECT_PATH  = 1,
    HEADER_INTERFACE    = 2,
    HEADER_MEMBER       = 3,
    HEADER_ERROR_NAME   = 4,
    HEADER_REPLY_SERIAL = 5,
    HEADER_DESTINATION  = 6,
    HEADER_SENDER       = 7,
    HEADER_SIGNATURE    = 8,
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

size_t adbus_parse_size(const uint8_t* data, size_t size)
{
    // Check that we have enough correct data to decode the header
    if (size < sizeof(struct adbusI_ExtendedHeader))
        return 0;

    struct adbusI_ExtendedHeader* header = (struct adbusI_ExtendedHeader*) data;

    adbus_Bool little = (header->endianness == 'l');
    adbus_Bool nativeEndian = (little == adbusI_littleEndian());

    size_t length = nativeEndian
        ? header->length
        : ADBUSI_FLIP32(header->length);

    size_t headerFieldLength = nativeEndian
        ? header->headerFieldLength
        : ADBUSI_FLIP32(header->headerFieldLength);

    size_t headerSize = sizeof(struct adbusI_ExtendedHeader) + headerFieldLength;
    // Note the header size is 8 byte aligned even if there is no argument data
    size_t messageSize = ADBUSI_ALIGN(headerSize, 8) + length;

    return messageSize;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

adbus_Message* adbus_msg_new(void)
{
    adbus_Message* m = NEW(adbus_Message);
    m->marshaller = adbus_buf_new();
    m->argumentMarshaller = adbus_buf_new();
    m->headerIterator = adbus_iter_new();
    return m;
}

// ----------------------------------------------------------------------------

void adbus_msg_free(adbus_Message* m)
{
    if (!m)
        return;

    adbus_buf_free(m->marshaller);
    adbus_buf_free(m->argumentMarshaller);
    adbus_iter_free(m->headerIterator);
    ks_free(m->summary);
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

void adbus_msg_reset(adbus_Message* m)
{
    adbus_buf_reset(m->marshaller);
    adbus_buf_reset(m->argumentMarshaller);
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

int adbus_parse(adbus_Message* m, const uint8_t* data, size_t size)
{
    // Reset the message internals first
    adbus_msg_reset(m);

    // We need to copy into the marshaller so that we can hold onto the data
    // For parsing we will use that copy as it will force 8 byte alignment
    CHECK(adbus_parse_size(data, size) == size);
    adbus_buf_set(m->marshaller, NULL, 0, data, size);
    adbus_buf_get(m->marshaller, NULL, NULL, &data, &size);

    // Check that we have enough correct data to decode the header
    CHECK(size >= sizeof(struct adbusI_ExtendedHeader));

    struct adbusI_ExtendedHeader* header = (struct adbusI_ExtendedHeader*) data;

    // Check the single byte header fields
    CHECK(header->version == adbusI_majorProtocolVersion);
    CHECK(header->type != ADBUS_MSG_INVALID);
    CHECK(header->endianness == 'B' || header->endianness == 'l');

    char localEndianness = adbusI_littleEndian() ? 'l' : 'B';
    m->messageType = (adbus_MessageType) header->type;
    m->nativeEndian = (header->endianness == localEndianness);

    // Get the non single byte fields out of the header
    size_t length = m->nativeEndian
        ? header->length
        : ADBUSI_FLIP32(header->length);

    size_t headerFieldLength = m->nativeEndian
        ? header->headerFieldLength
        : ADBUSI_FLIP32(header->headerFieldLength);

    m->serial = m->nativeEndian
        ? header->serial
        : ADBUSI_FLIP32(header->serial);

    CHECK(length <= ADBUSI_MAXIMUM_MESSAGE_LENGTH);
    CHECK(headerFieldLength <= ADBUSI_MAXIMUM_ARRAY_LENGTH);

    // Figure out the message size and see if we have the right amount
    size_t headerSize = sizeof(struct adbusI_ExtendedHeader) + headerFieldLength;
    headerSize = ADBUSI_ALIGN(headerSize, 8);
    size_t messageSize = headerSize + length;

    CHECK(size == messageSize);

    m->argumentOffset = messageSize - length;

    // Should we parse this?
    if (header->type > ADBUS_MSG_SIGNAL) {
        adbus_msg_reset(m);
        return 0;
    }

    // Parse the header fields
    const uint8_t* fieldBegin = data + sizeof(struct adbusI_Header);
    size_t fieldSize = headerSize - sizeof(struct adbusI_Header);

    adbus_Iterator* iterator =  m->headerIterator;
    adbus_iter_reset(iterator, "a(yv)", -1, fieldBegin, fieldSize);
    if (!m->nativeEndian)
        adbus_iter_setnonnative(iterator);
    adbus_Field field;

#define ITERATE(M, TYPE, PFIELD) \
        CHECK(!adbus_iter_next(iterator, PFIELD) && (PFIELD)->type == TYPE);
    
    ITERATE(m, ADBUS_ARRAY_BEGIN, &field);
    adbus_Bool arrayScope = field.scope;
    while (!adbus_iter_isfinished(iterator, arrayScope))
    {
        ITERATE(m, ADBUS_STRUCT_BEGIN, &field);
        ITERATE(m, ADBUS_UINT8, &field);
        uint8_t code = field.u8;
        ITERATE(m, ADBUS_VARIANT_BEGIN, &field);

        CHECK(!adbus_iter_next(iterator, &field));

        switch(code)
        {
            case HEADER_REPLY_SERIAL:
                CHECK(field.type == ADBUS_UINT32);
                m->replySerial = field.u32;
                m->hasReplySerial = 1;
                break;
            case HEADER_SIGNATURE:
                CHECK(field.type == ADBUS_SIGNATURE);
                m->signature = adbusI_strndup(field.string, field.size);
                break;
            case HEADER_OBJECT_PATH:
                CHECK(field.type == ADBUS_OBJECT_PATH);
                m->path = adbusI_strndup(field.string, field.size);
                break;
            case HEADER_INTERFACE:
                CHECK(field.type == ADBUS_STRING);
                m->interface = adbusI_strndup(field.string, field.size);
                break;
            case HEADER_MEMBER:
                CHECK(field.type == ADBUS_STRING);
                m->member = adbusI_strndup(field.string, field.size);
                break;
            case HEADER_ERROR_NAME:
                CHECK(field.type == ADBUS_STRING);
                m->errorName = adbusI_strndup(field.string, field.size);
                break;
            case HEADER_DESTINATION:
                CHECK(field.type == ADBUS_STRING);
                m->destination = adbusI_strndup(field.string, field.size);
                break;
            case HEADER_SENDER:
                CHECK(field.type == ADBUS_STRING);
                m->sender = adbusI_strndup(field.string, field.size);
                break;
            default:
                break;
        }
        ITERATE(m, ADBUS_VARIANT_END, &field);
        ITERATE(m, ADBUS_STRUCT_END, &field);
    }
    ITERATE(m, ADBUS_ARRAY_END, &field);
#undef ITERATE

    // Check that we have required fields
    if (m->messageType == ADBUS_MSG_METHOD) {
        CHECK(m->path && m->member);

    } else if (m->messageType == ADBUS_MSG_RETURN) {
        CHECK(m->hasReplySerial);

    } else if (m->messageType == ADBUS_MSG_ERROR) {
        CHECK(m->errorName);

    } else if (m->messageType == ADBUS_MSG_SIGNAL) {
        CHECK(m->interface && m->member);

    } else {
        CHECK(0);
    }

    // We need to convert the arguments to native endianness so that they
    // can be pulled out as a memory block
    // We do this by iterating over the arguments and remarshall them into the
    // argument marshaller
    if (!m->nativeEndian && m->signature) {
        adbus_Buffer* mar = adbus_msg_buffer(m);
        // Realistically we should break up the signature into the seperate
        // arguments and begin and end each one with a seperate signature, but
        // the iterator doesn't return begin and end argument fields
        adbus_buf_append(mar, m->signature, -1);

        adbus_msg_iterator(m, iterator);
        adbus_iter_setnonnative(iterator);

        CHECK(!adbus_buf_copy(mar, iterator, 0));
    }

    return 0;
}

// ----------------------------------------------------------------------------
// Getter functions
// ----------------------------------------------------------------------------

static void AppendString(adbus_Message* m, uint8_t code, const char* field)
{
    if (field) {
        adbus_buf_beginstruct(m->marshaller);
        adbus_buf_uint8(m->marshaller, code);
        adbus_buf_beginvariant(m->marshaller, "s", -1);
        adbus_buf_string(m->marshaller, field, -1);
        adbus_buf_endvariant(m->marshaller);
        adbus_buf_endstruct(m->marshaller);
    }
}

static void AppendSignature(adbus_Message* m, uint8_t code, const char* field)
{
    if (field) {
        adbus_buf_beginstruct(m->marshaller);
        adbus_buf_uint8(m->marshaller, code);
        adbus_buf_beginvariant(m->marshaller, "g", -1);
        adbus_buf_signature(m->marshaller, field, -1);
        adbus_buf_endvariant(m->marshaller);
        adbus_buf_endstruct(m->marshaller);
    }
}

static void AppendObjectPath(adbus_Message* m, uint8_t code, const char* field)
{
    if (field) {
        adbus_buf_beginstruct(m->marshaller);
        adbus_buf_uint8(m->marshaller, code);
        adbus_buf_beginvariant(m->marshaller, "o", -1);
        adbus_buf_objectpath(m->marshaller, field, -1);
        adbus_buf_endvariant(m->marshaller);
        adbus_buf_endstruct(m->marshaller);
    }
}

static void AppendUInt32(adbus_Message* m, uint8_t code, uint32_t field)
{
    adbus_buf_beginstruct(m->marshaller);
    adbus_buf_uint8(m->marshaller, code);
    adbus_buf_beginvariant(m->marshaller, "u", -1);
    adbus_buf_uint32(m->marshaller, field);
    adbus_buf_endvariant(m->marshaller);
    adbus_buf_endstruct(m->marshaller);
}

void adbus_msg_build(adbus_Message* m)
{
    // Check that we aren't already built
    size_t datasize;
    adbus_buf_get(m->marshaller, NULL, NULL, NULL, &datasize);
    if (datasize > 0)
        return;

    const char* signature;
    size_t sigsize;
    const uint8_t* argumentData;
    size_t argumentSize;

    adbus_buf_get(
            m->argumentMarshaller,
            &signature,
            &sigsize,
            &argumentData,
            &argumentSize);

    adbus_buf_reset(m->marshaller);

    if (m->messageType == ADBUS_MSG_METHOD) {
        assert(m->path && m->member);
    } else if (m->messageType == ADBUS_MSG_RETURN) {
        assert(m->hasReplySerial);
    } else if (m->messageType == ADBUS_MSG_SIGNAL) {
        assert(m->interface && m->member);
    } else if (m->messageType == ADBUS_MSG_ERROR) {
        assert(m->errorName);
    } else {
        assert(0);
    }

    struct adbusI_Header header;
    header.endianness   = adbusI_littleEndian() ? 'l' : 'B';
    header.type         = (uint8_t) m->messageType;
    header.flags        = m->flags;
    header.version      = adbusI_majorProtocolVersion;
    header.length       = argumentSize;
    header.serial       = m->serial;
    adbus_buf_appenddata(m->marshaller, (const uint8_t*) &header, sizeof(struct adbusI_Header));

    adbus_buf_append(m->marshaller, "a(yv)", -1);
    adbus_buf_beginarray(m->marshaller);
    AppendString(m, HEADER_INTERFACE, m->interface);
    AppendString(m, HEADER_MEMBER, m->member);
    AppendString(m, HEADER_ERROR_NAME, m->errorName);
    AppendString(m, HEADER_DESTINATION, m->destination);
    AppendString(m, HEADER_SENDER, m->sender);
    AppendObjectPath(m, HEADER_OBJECT_PATH, m->path);
    if (m->hasReplySerial)
        AppendUInt32(m, HEADER_REPLY_SERIAL, m->replySerial);
    if (argumentData && argumentSize > 0)
        AppendSignature(m, HEADER_SIGNATURE, signature);
    adbus_buf_endarray(m->marshaller);

    size_t headerSize;
    adbus_buf_get(m->marshaller, NULL, NULL, NULL, &headerSize);

    // The header is 8 byte padded even if there is no argument data
    static uint8_t paddingData[8]; // since static, guarenteed to be 0s
    size_t padding = ADBUSI_ALIGN(headerSize, 8) - headerSize;
    if (padding != 0)
        adbus_buf_appenddata(m->marshaller, paddingData, padding);

    if (argumentData && argumentSize > 0)
        adbus_buf_appenddata(m->marshaller, argumentData, argumentSize);
}

// ----------------------------------------------------------------------------

const uint8_t* adbus_msg_data(const adbus_Message* m, size_t* psize)
{
    const uint8_t* data;
    adbus_buf_get(m->marshaller, NULL, NULL, &data, psize);
    return data;
}

// ----------------------------------------------------------------------------

const char* adbus_msg_path(const adbus_Message* m, size_t* len)
{
    if (len)
        *len = m->path ? strlen(m->path) : 0;
    return m->path;
}

// ----------------------------------------------------------------------------

const char* adbus_msg_interface(const adbus_Message* m, size_t* len)
{
    if (len)
        *len = m->interface ? strlen(m->interface) : 0;
    return m->interface;
}

// ----------------------------------------------------------------------------

const char* adbus_msg_sender(const adbus_Message* m, size_t* len)
{
    if (len)
        *len = m->sender ? strlen(m->sender) : 0;
    return m->sender;
}

// ----------------------------------------------------------------------------

const char* adbus_msg_destination(const adbus_Message* m, size_t* len)
{
    if (len)
        *len = m->destination ? strlen(m->destination) : 0;
    return m->destination;
}

// ----------------------------------------------------------------------------

const char* adbus_msg_member(const adbus_Message* m, size_t* len)
{
    if (len)
        *len = m->member ? strlen(m->member) : 0;
    return m->member;
}

// ----------------------------------------------------------------------------

const char* adbus_msg_error(const adbus_Message* m, size_t* len)
{
    if (len)
        *len = m->errorName ? strlen(m->errorName) : 0;
    return m->errorName;
}

// ----------------------------------------------------------------------------

adbus_MessageType adbus_msg_type(const adbus_Message* m)
{
    return m->messageType;
}

// ----------------------------------------------------------------------------

uint8_t adbus_msg_flags(const adbus_Message* m)
{
    return m->flags;
}

// ----------------------------------------------------------------------------

uint32_t adbus_msg_serial(const adbus_Message* m)
{
    return m->serial;
}

// ----------------------------------------------------------------------------

adbus_Bool adbus_msg_reply(const adbus_Message* m, uint32_t* serial)
{
    if (serial)
        *serial = m->replySerial;
    return m->hasReplySerial;
}

// ----------------------------------------------------------------------------
// Setter functions
// ----------------------------------------------------------------------------

void adbus_msg_settype(adbus_Message* m, adbus_MessageType type)
{
    m->messageType = type;
}

// ----------------------------------------------------------------------------

void adbus_msg_setserial(adbus_Message* m, uint32_t serial)
{
    m->serial = serial;
}

// ----------------------------------------------------------------------------

void adbus_msg_setflags(adbus_Message* m, uint8_t flags)
{
    m->flags = flags;
}

// ----------------------------------------------------------------------------

void adbus_msg_setreply(adbus_Message* m, uint32_t reply)
{
    m->replySerial = reply;
    m->hasReplySerial = 1;
}

// ----------------------------------------------------------------------------

void adbus_msg_setpath(adbus_Message* m, const char* path, int size)
{
    free(m->path);
    m->path = (size >= 0)
            ? adbusI_strndup(path, size)
            : adbusI_strdup(path);
}

// ----------------------------------------------------------------------------

void adbus_msg_setinterface(adbus_Message* m, const char* interface, int size)
{
    free(m->interface);
    m->interface = (size >= 0)
                 ? adbusI_strndup(interface, size)
                 : adbusI_strdup(interface);
}

// ----------------------------------------------------------------------------

void adbus_msg_setmember(adbus_Message* m, const char* member, int size)
{
    free(m->member);
    m->member = (size >= 0)
              ? adbusI_strndup(member, size)
              : adbusI_strdup(member);
}

// ----------------------------------------------------------------------------

void adbus_msg_seterror(adbus_Message* m, const char* errorName, int size)
{
    free(m->errorName);
    m->errorName = (size >= 0)
                 ? adbusI_strndup(errorName, size)
                 : adbusI_strdup(errorName);
}

// ----------------------------------------------------------------------------

void adbus_msg_setdestination(adbus_Message* m, const char* destination, int size)
{
    free(m->destination);
    m->destination = (size >= 0)
                   ? adbusI_strndup(destination, size)
                   : adbusI_strdup(destination);
}

// ----------------------------------------------------------------------------

void adbus_msg_setsender(adbus_Message* m, const char* sender, int size)
{
    free(m->sender);
    m->sender = (size >= 0)
              ? adbusI_strndup(sender, size)
              : adbusI_strdup(sender);
}

// ----------------------------------------------------------------------------

adbus_Buffer* adbus_msg_buffer(adbus_Message* m)
{
    return m->argumentMarshaller;
}

// ----------------------------------------------------------------------------

const char* adbus_msg_signature(const adbus_Message* m, size_t* sigsize)
{
    if (m->argumentOffset > 0 && m->nativeEndian) {
        if (sigsize)
            *sigsize = m->signature ? strlen(m->signature) : 0;
        return m->signature;

    } else {
        const char* signature;
        adbus_buf_get(m->argumentMarshaller, &signature, sigsize, NULL, NULL);
        return signature;
    }
}

// ----------------------------------------------------------------------------

void adbus_msg_iterator(const adbus_Message* m, adbus_Iterator* iterator)
{
    const char* signature;
    size_t sigsize;
    const uint8_t* data;
    size_t datasize;
    if (m->argumentOffset > 0 && m->nativeEndian) {
        adbus_buf_get(m->marshaller, NULL, NULL, &data, &datasize);
        signature = m->signature;
        sigsize = signature ? strlen(signature) : 0;
        data += m->argumentOffset;
        datasize -= m->argumentOffset;

    } else {
        adbus_buf_get(m->argumentMarshaller, &signature, &sigsize, &data, &datasize);
    }
    adbus_iter_reset(iterator, signature, sigsize, data, datasize);
}

// ----------------------------------------------------------------------------

static void PrintStringField(
        kstring_t* str,
        const char* field,
        const char* what)
{
    if (field)
        ks_printf(str, "\n%-15s \"%s\"", what, field);
}

static void LogField(kstring_t* str, adbus_Iterator* i, adbus_Field* field);
static void LogScope(kstring_t* str, adbus_Iterator* i, adbus_FieldType end)
{
    adbus_Bool first = 1;
    adbus_Field field;
    while (!adbus_iter_next(i, &field) && field.type != end && field.type != ADBUS_END_FIELD) {
        if (!first)
            ks_printf(str, ", ");
        first = 0;
        LogField(str, i, &field);
    }
}

static void LogMap(kstring_t* str, adbus_Iterator* i, adbus_FieldType end)
{
    adbus_Bool first = 1;
    adbus_Bool key = 0;
    adbus_Field field;
    while (!adbus_iter_next(i, &field) && field.type != end && field.type != ADBUS_END_FIELD) {
        if (key)
            ks_printf(str, " = ");
        else if (!first)
            ks_printf(str, ", ");
        first = 0;
        key = !key;
        LogField(str, i, &field);
    }
}

static void LogField(kstring_t* str, adbus_Iterator* i, adbus_Field* field)
{
    adbus_FieldType type = field->type;
    switch (type)
    {
    case ADBUS_UINT8:
        ks_printf(str, "%d", (int) field->u8);
        break;
    case ADBUS_BOOLEAN:
        ks_printf(str, "%s", field->b ? "true" : "false");
        break;
    case ADBUS_INT16:
        ks_printf(str, "%d", (int) field->i16);
        break;
    case ADBUS_UINT16:
        ks_printf(str, "%d", (int) field->u16);
        break;
    case ADBUS_INT32:
        ks_printf(str, "%d", (int) field->i32);
        break;
    case ADBUS_UINT32:
        ks_printf(str, "%u", (unsigned int) field->u32);
        break;
    case ADBUS_INT64:
        ks_printf(str, "%lld", (long long int) field->i64);
        break;
    case ADBUS_UINT64:
        ks_printf(str, "%llu", (long long unsigned int) field->u64);
        break;
    case ADBUS_DOUBLE:
        ks_printf(str, "%.15g", field->d);
        break;
    case ADBUS_STRING:
    case ADBUS_OBJECT_PATH:
    case ADBUS_SIGNATURE:
        ks_printf(str, "\"%s\"", field->string);
        break;
    case ADBUS_ARRAY_BEGIN:
        ks_printf(str, "[ ");
        LogScope(str, i, ADBUS_ARRAY_END);
        ks_printf(str, " ]");
        break;
    case ADBUS_STRUCT_BEGIN:
        ks_printf(str, "( ");
        LogScope(str, i, ADBUS_STRUCT_END);
        ks_printf(str, " )");
        break;
    case ADBUS_MAP_BEGIN:
        ks_printf(str, "{ ");
        LogMap(str, i, ADBUS_MAP_END);
        ks_printf(str, " }");
        break;
    case ADBUS_VARIANT_BEGIN:
        ks_printf(str, "<%s>{ ", field->string);
        LogScope(str, i, ADBUS_VARIANT_END);
        ks_printf(str, " }");
        break;
    default:
        assert(0);
    }
}

const char* adbus_msg_summary(adbus_Message* m, size_t* size)
{
    if (!m->summary)
        m->summary = ks_new();

    kstring_t* str = m->summary;
    ks_clear(str);

    size_t messageSize = 0;
    adbus_buf_get(m->marshaller, NULL, NULL, NULL, &messageSize);

    adbus_MessageType type = adbus_msg_type(m);
    if (type == ADBUS_MSG_METHOD) {
        ks_printf(str, "Method call: ");
    } else if (type == ADBUS_MSG_RETURN) {
        ks_printf(str, "Return: ");
    } else if (type == ADBUS_MSG_ERROR) {
        ks_printf(str, "Error: ");
    } else if (type == ADBUS_MSG_SIGNAL) {
        ks_printf(str, "Signal: ");
    } else {
        ks_printf(str, "Unknown (%d): ", (int) type);
    }

    ks_printf(str, "Flags %d, Length %d, Serial %d",
            (int) adbus_msg_flags(m),
            (int) messageSize,
            (int) adbus_msg_serial(m));
    PrintStringField(str, m->sender, "Sender");
    PrintStringField(str, m->destination, "Destination");
    PrintStringField(str, m->path, "Path");
    PrintStringField(str, m->interface, "Interface");
    PrintStringField(str, m->member, "Member");
    PrintStringField(str, m->errorName, "Error name");
    if (m->hasReplySerial) {
        ks_printf(str, "\n%-15s %d", "Reply serial", m->replySerial);
    }

    const char* sig = adbus_msg_signature(m, NULL);
    if (sig && *sig) {
        PrintStringField(str, sig, "Signature");
    }

    int argnum = 0;
    adbus_Field field;
    adbus_Iterator* iter = m->headerIterator;
    adbus_msg_iterator(m, iter);
    while (!adbus_iter_next(iter, &field) && field.type != ADBUS_END_FIELD) {
        ks_printf(str, "\nArgument %2d     ", argnum++);
        LogField(str, iter, &field);
    }

    if (size)
        *size = ks_size(str);

    return ks_cstr(str);
}


