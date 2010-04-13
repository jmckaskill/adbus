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

#include "server.h"
#include "parse.h"
#include "misc.h"

/* -------------------------------------------------------------------------- */

void adbusI_remote_initParser(adbus_Remote* r)
{
    r->parseBuffer = adbus_buf_new();
}

void adbusI_remote_freeParser(adbus_Remote* r)
{
    adbus_buf_free(r->parseBuffer);
}





/* -------------------------------------------------------------------------- */

/* Manually unpack even for native endianness since value might not be 4 byte
 * aligned
 */
static uint32_t Get32(char endianness, const uint32_t* value)
{
    uint8_t* p = (uint8_t*) value;
    if (endianness == 'l') {
        return  ((uint32_t) p[0])
            |   ((uint32_t) p[1] << 8)
            |   ((uint32_t) p[2] << 16)
            |   ((uint32_t) p[3] << 24);
    } else {
        return  ((uint32_t) p[3])
            |   ((uint32_t) p[2] << 8)
            |   ((uint32_t) p[1] << 16)
            |   ((uint32_t) p[0] << 24);
    }
}

static int ServerParse(adbus_Remote* r, adbus_Buffer* b, adbus_Message* m, const char* data, size_t size)
{
    const adbusI_ExtendedHeader* sh = (const adbusI_ExtendedHeader*) data;
    adbus_Bool native = (sh->endianness == ADBUSI_NATIVE_ENDIANNESS);
    adbus_Iterator i;
    size_t headerFieldLength;
    size_t argumentsLength;
    const char* sourceArgumentData;
    char* bdata;

    assert(size > sizeof(adbusI_ExtendedHeader));

    if (sh->type == ADBUS_MSG_INVALID) {
        return -1;
    } else if (sh->type > ADBUS_MSG_SIGNAL) {
        return 1;
    }

    argumentsLength = Get32(sh->endianness, &sh->length);
    headerFieldLength = Get32(sh->endianness, &sh->headerFieldLength);
    headerFieldLength = ADBUS_ALIGN(headerFieldLength, 8);
    sourceArgumentData = data + sizeof(adbusI_ExtendedHeader) + headerFieldLength;

    /* Should've been checked in adbus_parse_size */
    assert(sizeof(adbusI_ExtendedHeader) + headerFieldLength + argumentsLength == size);

    memset(m, 0, sizeof(adbus_Message));
    m->replySerial = -1;

    /* Allocate the buffer room to copy the message into */
    adbus_buf_reset(b);
    adbus_buf_append(b, data, size + ds_size(&r->sender));
    bdata = adbus_buf_data(b);

    /* Copy over just the header data now. We then parse that (which will
     * resize due to the sender field). After that we then append the argument
     * data.
     */
    memcpy(bdata, data, sizeof(adbusI_ExtendedHeader) + headerFieldLength);

    i.data = bdata + sizeof(adbusI_ExtendedHeader);
    i.end  = i.data + headerFieldLength;
    /* i.end is the 8 byte aligned end of the headers */
    assert((char*) ADBUS_ALIGN(i.end, 8) == i.end);

    /* Flip the endianness of the header */
    if (!native) {
        adbusI_Header* dh = (adbusI_Header*) bdata;
        dh->endianness = ADBUSI_NATIVE_ENDIANNESS;
        if (adbus_flip_data(bdata, i.end, "yyyyuua(yv)")) {
            return -1;
        }
    }

    m->type     = sh->type;
    m->flags    = sh->flags;
    m->serial   = sh->serial;

    while (i.data < i.end) {
        uint32_t u32[2];

        /* Since i.end is 8 byte aligned and so is each field, the min size
         * for a field is 8.
         */
        assert((char*) ADBUS_ALIGN(i.data, 8) == i.data);
        assert(i.data + 8 <= i.end);

        u32[0] = ((const uint32_t*) i.data)[0];
        u32[1] = ((const uint32_t*) i.data)[1];

#define GET_STRING(str, size)                                               \
        size    = u32[1];                                                   \
        str     = i.data + 8;                                               \
        i.data  = str + size + 1;                                           \
        i.data  = (char*) ADBUS_ALIGN(i.data, 8);                           \
        if (i.data > i.end || str[size] != '\0') {                          \
            return -1;                                                      \
        }

        /* Note we store the fields in m as offsets from the base data and
         * then reapply the base after we stop resizing the buffer.
         */

        switch (u32[0]) {
            case ADBUSI_HEADER_OBJECT_PATH_LONG:
                GET_STRING(m->path, m->pathSize);
                break;

            case ADBUSI_HEADER_INTERFACE_LONG:
                GET_STRING(m->interface, m->interfaceSize);
                break;

            case ADBUSI_HEADER_MEMBER_LONG:
                GET_STRING(m->member, m->memberSize);
                break;

            case ADBUSI_HEADER_ERROR_NAME_LONG:
                GET_STRING(m->error, m->errorSize);
                break;

            case ADBUSI_HEADER_DESTINATION_LONG:
                GET_STRING(m->destination, m->destinationSize);
                break;

            case ADBUSI_HEADER_REPLY_SERIAL_LONG:
                m->replySerial = u32[1];
                i.data += 8;
                break;

            case ADBUSI_HEADER_SIGNATURE_LONG:
                m->signatureSize = ((const uint8_t*) i.data)[4];
                m->signature = i.data + 5;
                i.data = m->signature + m->signatureSize + 1;
                i.data = (char*) ADBUS_ALIGN(i.data, 8);
                if (i.data > i.end || m->signature[m->signatureSize] != '\0') {
                    return -1;
                }
                break;

            case ADBUSI_HEADER_SENDER_LONG:
                {
                    /* Remove any existing sender fields - we then append our
                     * own version further down.
                     */
                    size_t strsz, fieldsz;
                    const char* end;

                    strsz = u32[1];
                    end = i.data + 8 + strsz + 1;
                    end = (char*) ADBUS_ALIGN(end, 8);
                    if (end > i.end) {
                        return -1;
                    }
                    /* Remove the field */
                    fieldsz = end - i.data;
                    memmove((char*) i.data, end, i.end - end);
                    headerFieldLength -= fieldsz;
                    i.end -= fieldsz;
                }
                break;

            default:
                /* If the code is a well known one, but it didn't match the
                 * long code, we have a field with an invalid signature. The
                 * size has already been checked, so this is safe.
                 */
                if (*(uint8_t*) i.data++ <= ADBUSI_HEADER_MAX) {
                    return -1;
                }
                i.sig = "v";
                if (adbus_iter_value(&i)) {
                    return -1;
                }
                i.data = (char*) ADBUS_ALIGN(i.data, 8);
                if (i.data > i.end) {
                    return -1;
                }
                break;
        }
    }

    assert(bdata + sizeof(adbusI_ExtendedHeader) + headerFieldLength == i.end);
    assert(i.data == i.end);
    assert((char*) ADBUS_ALIGN(i.data, 8) == i.data);

    {
        adbusI_ExtendedHeader* header;

        /* Append the new sender field value */
        memcpy((char*) i.end, ds_cstr(&r->sender), ds_size(&r->sender));
        m->senderSize = ds_size(&r->unique);
        m->sender = i.end + 8; /* 4 for long code + 4 for string size */

        headerFieldLength += ds_size(&r->sender);
        i.end += ds_size(&r->sender);

        header = (adbusI_ExtendedHeader*) bdata;
        header->headerFieldLength = headerFieldLength - r->senderPadding;
    }

    assert(bdata + sizeof(adbusI_ExtendedHeader) + headerFieldLength == i.end);

    {
        /* Append the argument data */
        memcpy((char*) i.end, sourceArgumentData, argumentsLength);

        m->argdata = i.end;
        m->argsize = argumentsLength;

        if (!native && m->signature) {
            if (adbus_flip_data((char*) m->argdata, m->argdata + m->argsize, m->signature)) {
                return -1;
            }
        }
    }

    m->data = bdata;
    m->size = sizeof(adbusI_ExtendedHeader) + headerFieldLength + m->argsize;

    return 0;
}


/** Dispatches all complete messages in the provided buffer from the given remote
 *  \relates adbus_Server
 *
 *  \return non-zero on error at which point the remote should be kicked
 */
int adbus_remote_parse(adbus_Remote* r, adbus_Buffer* b)
{
    adbus_Message m;
    const char* data = adbus_buf_data(b);
    size_t size = adbus_buf_size(b);
    d_Vector(Argument) args;

    ZERO(args);

	ADBUSI_LOG_DATA_3(
			data,
			size,
			"bus parse from (remote %s, %p)",
			ds_cstr(&r->unique),
			(void*) r);

    while (1) {
        int ret;

        size_t msgsize = adbus_parse_size(data, size);
        if (msgsize == 0 || msgsize > size)
            break;

        ret = ServerParse(r, r->parseBuffer, &m, data, msgsize);

        if (ret < 0)
            return -1;

        if (!ret) {
            dv_clear(Argument, &args);
            ret = adbusI_serv_dispatch(r->server, r, &m, &args);
        }

        if (ret < 0)
            return -1;

        data += msgsize;
        size -= msgsize;

    }

    dv_free(Argument, &args);
    adbus_buf_remove(b, 0, adbus_buf_size(b) - size);
    return 0;
}



/** Dispatches a message from the given remote
 *  \relates adbus_Server
 *
 *  \return non-zero on error at which point the remote should be kicked
 */
int adbus_remote_dispatch(adbus_Remote* r, const adbus_Message* m)
{
    int ret;
    adbus_Message m2;
    d_Vector(Argument) args;

    ZERO(args);
    assert(adbus_parse_size(m->data, m->size) == (int) m->size);

    ret = ServerParse(r, r->parseBuffer, &m2, m->data, m->size);

    /* Invalid message */
    if (ret < 0)
        return -1;

    /* Ignored message */
    if (ret == 1)
        return 0;

    ret = adbusI_serv_dispatch(r->server, r, &m2, &args);
    dv_free(Argument, &args);

    return ret;
}



