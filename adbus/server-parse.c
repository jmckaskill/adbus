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
#include <adbus.h>

/* -------------------------------------------------------------------------- */
static void InitBuffer(adbus_Buffer* b)
{
    adbus_buf_reset(b);
    adbus_Message m;
    ZERO(&m);
    adbus_buf_append(b, (const char*) &m, sizeof(adbus_Message));
    adbus_buf_align(b, 8);
}

static void UnpackBuffer(adbus_Buffer* b, adbus_Message** m, char** data, size_t* size)
{
    size_t off  = ADBUS_ALIGN(sizeof(adbus_Message), 8);
    char* bdata = (char*) adbus_buf_data(b);
    *m          = (adbus_Message*) bdata;
    *data       = bdata + off;
    *size       = adbus_buf_size(b) - off;
}

/* -------------------------------------------------------------------------- */
static int DispatchMsg(adbus_Remote* r, adbus_Buffer* b)
{
    adbus_Message* m;
    char* data;
    size_t size;
    UnpackBuffer(b, &m, &data, &size);
    if (adbus_parse(m, data, size))
        return -1;

    if (m->signature && !r->native && adbus_flip_data((char*) m->argdata, m->argsize, m->signature))
        return -1;

    if (ADBUS_TRACE_BUS) {
        adbusI_logmsg("dispatch", m);
    }

    adbus_Server* s = r->server;

    // If we haven't yet gotten a hello, we only accept a method call to the
    // hello method. This needs:
    // type     - method call
    // dest     - null or "org.freedesktop.DBus"
    // iface    - null or "org.freedesktop.DBus"
    // path     - "/" or "/org/freedesktop/DBus"
    // member   - "Hello"
    // The arguments will be checked in the callback
    if (!r->haveHello) {
        if (m->type != ADBUS_MSG_METHOD)
            return -1;
        if (m->destination && strcmp(m->destination, "org.freedesktop.DBus") != 0)
            return -1;
        if (m->interface && strcmp(m->interface, "org.freedesktop.DBus") != 0)
            return -1;
        if (m->path == NULL || (strcmp(m->path, "/") != 0 && strcmp(m->path, "/org/freedesktop/DBus") != 0))
            return -1;
        if (m->member == NULL || strcmp(m->member, "Hello") != 0)
            return -1;

        assert(s->helloRemote == NULL);
        s->helloRemote = r;
    }

    int ret = adbusI_serv_dispatch(r->server, m);

    free(m->arguments);
    adbus_buf_reset(b);

    s->helloRemote   = NULL;
    r->msgSize       = 0;
    r->headerSize    = 0;
    r->parsedMsgSize = 0;

    return ret;
}

/* --------------------------------------------------------------------------
 * Iterate over the header fields, removing any sender fields, and then
 * append a correct sender field.
 */
static int FixHeaders(adbus_Remote* r, adbus_Buffer* b)
{
    // Reserve enough so that the addition of the sender field does not cause
    // a realloc
    size_t reserve = adbus_buf_size(b)
                   + 7    // adbus_buf_struct alignment
                   + 1    // adbus_buf_u8
                   + 3    // adbus_buf_variant of string (0x01 's' 0x00)
                   + 3    // adbus_buf_string alignment
                   + 4    // adbus_buf_string size
                   + ds_size(&r->unique)
                   + 1    // adbus_buf_string null
                   + 7;   // adbus_buf_struct header end alignment
    adbus_buf_reserve(b, reserve);

    const char* begin = adbus_buf_data(b);

    adbus_Message* m;
    char* data;
    size_t size;
    UnpackBuffer(b, &m, &data, &size);


    adbusI_ExtendedHeader* h = (adbusI_ExtendedHeader*) data;
    r->native     = (h->endianness == adbusI_nativeEndianness());
    h->endianness = adbusI_nativeEndianness();

    if (!r->native && adbus_flip_data(data, size, "yyyyuua(yv)"))
        return -1;

    // For our purposes here we consider a field to be everything from the
    // beginning of the field through to the end of the 8 byte padding after
    // the field data
    adbus_Iterator i = {
        data + sizeof(adbusI_Header),
        4 + ADBUSI_ALIGN(h->headerFieldLength, 8),
        "a(yv)"
    };
    assert(i.data + i.size == adbus_buf_data(b) + adbus_buf_size(b));


    adbus_IterArray a;
    if (adbus_iter_beginarray(&i, &a))
        return -1;

    const char* arraybegin = i.data;

    while (adbus_iter_inarray(&i, &a)) {
        adbus_IterVariant v;
        char* fieldbegin = (char*) i.data;
        const uint8_t* code;
        if (adbus_iter_beginstruct(&i))
            return -1;
        if (adbus_iter_u8(&i, &code))
            return -1;
        if (adbus_iter_beginvariant(&i, &v))
            return -1;
        if (adbus_iter_value(&i))
            return -1;
        if (adbus_iter_endvariant(&i, &v))
            return -1;
        if (adbus_iter_endstruct(&i))
            return -1;
        if (adbus_iter_align(&i, 8))
            return -1;

        if (*code == HEADER_SENDER) {
            // Remove the sender field
            size_t fieldsz = i.data - fieldbegin;
            adbus_buf_remove(b, fieldbegin - begin, fieldsz);
            i.data  -= fieldsz;
            a.size  -= fieldsz;
        }
    }

    adbus_BufVariant v;
    adbus_buf_setsig(b, "(yv)", 4);
    adbus_buf_beginstruct(b);
    adbus_buf_u8(b, HEADER_SENDER);
    adbus_buf_beginvariant(b, &v, "s", 1);
    adbus_buf_string(b, ds_cstr(&r->unique), ds_size(&r->unique));
    adbus_buf_endvariant(b, &v);
    adbus_buf_endstruct(b);

    h->headerFieldLength = adbus_buf_data(b) + adbus_buf_size(b) - arraybegin;
    adbus_buf_align(b, 8);

    r->parsedMsgSize = r->msgSize - r->headerSize
                     + ADBUS_ALIGN(h->headerFieldLength, 8)
                     + sizeof(adbusI_ExtendedHeader);

    return 0;
}




/** Dispatches a message from the given remote
 *  \relates adbus_Server
 *
 *  \return non-zero on error at which point the remote should be kicked
 */
int adbus_remote_dispatch(adbus_Remote* r, adbus_Message* m)
{
    assert(r->msgSize == r->parsedMsgSize && r->msgSize == r->headerSize && r->msgSize == 0);

    adbus_Buffer* b = r->dispatch;
    InitBuffer(b);

    adbus_buf_append(b, m->data, m->size - m->argsize);

    if (FixHeaders(r, b))
        return -1;

    adbus_buf_append(b, m->argdata, m->argsize);

    return DispatchMsg(r, b);
}







/* -------------------------------------------------------------------------- */
// Manually unpack even for native endianness since value not be 4 byte
// aligned
static const int RECV_SIZE = 4 * 1024;
static uint32_t Get32(char endianness, uint32_t* value)
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

static int Move(adbus_Buffer* dest, const char** data, size_t* size, size_t need)
{
    if (*size >= need) {
        adbus_buf_append(dest, *data, need);
        *data += need;
        *size -= need;
        return 0;
    } else {
        adbus_buf_append(dest, *data, *size);
        *data += *size;
        *size = 0;
        return -1;
    }
}


/** Dispatches all complete messages in the provided buffer from the given remote
 *  \relates adbus_Server
 *
 *  \return non-zero on error at which point the remote should be kicked
 */
int adbus_remote_parse(adbus_Remote* r, adbus_Buffer* b)
{
    const char* data = adbus_buf_data(b);
    size_t size = adbus_buf_size(b);

    switch (r->parseState) {
        while (1) {
            case BEGIN:
                {
                    r->parseState = BEGIN;
                    if (size < sizeof(adbusI_ExtendedHeader))
                        goto end;

                    // Copy header
                    adbusI_ExtendedHeader* h = (adbusI_ExtendedHeader*) data;

                    size_t hsize    = sizeof(adbusI_ExtendedHeader)
                        + Get32(h->endianness, &h->headerFieldLength);
                    r->headerSize   = ADBUSI_ALIGN(hsize, 8);
                    r->msgSize      = r->headerSize + Get32(h->endianness, &h->length);

                    InitBuffer(r->msg);

                    // fall through
                }

            case HEADER:
                {
                    // Copy header fields
                    r->parseState = HEADER;
                    adbus_Message* m;
                    char* mdata;
                    size_t msize;
                    UnpackBuffer(r->msg, &m, &mdata, &msize);

                    assert(r->headerSize > msize);
                    size_t need = r->headerSize - msize;
                    if (Move(r->msg, &data, &size, need))
                        goto end;

                    if (FixHeaders(r, r->msg))
                        return -1;

                    // fall through
                }

            case DATA:
                {
                    // Copy data
                    r->parseState = DATA;
                    adbus_Message* m;
                    char* mdata;
                    size_t msize;
                    UnpackBuffer(r->msg, &m, &mdata, &msize);

                    assert(r->parsedMsgSize >= msize);
                    size_t need = r->parsedMsgSize - msize;
                    if (need > 0 && Move(r->msg, &data, &size, need))
                        goto end;

                    if (DispatchMsg(r, r->msg))
                        return -1;

                    // loop around
                }
        }
    }

end:
    adbus_buf_remove(b, 0, adbus_buf_size(b) - size);
    return 0;
}


