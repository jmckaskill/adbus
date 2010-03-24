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

void adbusI_remote_initParser(adbusI_ServerParser* p)
{
    p->state = PARSE_DISPATCH;
    p->buffer = adbus_buf_new();
}

void adbusI_remote_freeParser(adbusI_ServerParser* p)
{
    adbus_buf_free(p->buffer);
}

/* -------------------------------------------------------------------------- */

static void InitBuffer(adbusI_ServerParser* p)
{
    adbus_Message m;

    ZERO(m);
    adbus_buf_reset(p->buffer);
    adbus_buf_append(p->buffer, (const char*) &m, sizeof(adbus_Message));
    adbus_buf_align(p->buffer, 8);
}

static void UnpackBuffer(adbusI_ServerParser* p, adbus_Message** m, char** data, size_t* size)
{
    size_t off  = ADBUS_ALIGN(sizeof(adbus_Message), 8);
    char* bdata = (char*) adbus_buf_data(p->buffer);
    *m          = (adbus_Message*) bdata;
    *data       = bdata + off;
    *size       = adbus_buf_size(p->buffer) - off;
}

/* -------------------------------------------------------------------------- */

static int DispatchMsg(adbus_Remote* r, adbusI_ServerParser* p)
{
    int ret = 0;
    adbus_Message* m;
    char* data;
    size_t size;
    UnpackBuffer(p, &m, &data, &size);

    p->msgSize       = 0;
    p->headerSize    = 0;

    if (adbus_parse(m, data, size)) {
        ret = -1;
        goto end;
    }

    if (m->signature && !p->native && adbus_flip_data((char*) m->argdata, m->argsize, m->signature)) {
        ret = -1;
        goto end;
    }

    ADBUSI_LOG_MSG("Dispatch", m);

    ret = adbusI_serv_dispatch(r->server, r, m);

end:
    adbus_freeargs(m);
    return ret;
}

/* --------------------------------------------------------------------------
 * Iterate over the header fields, removing any sender fields, and then
 * append a correct sender field.
 */
static int FixHeaders(adbus_Remote* r, adbusI_ServerParser* p)
{
    adbus_Message* m;
    char* data;
    size_t size;
    adbusI_ExtendedHeader* h;
    adbus_Iterator i;
    adbus_IterArray a;

    UnpackBuffer(p, &m, &data, &size);
    h = (adbusI_ExtendedHeader*) data;
    h->endianness = adbusI_nativeEndianness();

    /* Flip the header data */
    if (!p->native && adbus_flip_data(data, size, "yyyyuua(yv)"))
        return -1;

    /* For our purposes here we consider a field to be everything from the
     * beginning of the field through to the end of the 8 byte padding after
     * the field data
     */
    i.data = data + sizeof(adbusI_Header);
    i.size = p->headerSize - sizeof(adbusI_Header);
    i.sig  = "a(yv)";
    assert(i.data + i.size == adbus_buf_data(p->buffer) + adbus_buf_size(p->buffer));

    if (adbus_iter_beginarray(&i, &a))
        return -1;

    while (adbus_iter_inarray(&i, &a)) {
        const char *fieldbegin, *fieldend;
        const uint8_t* code;
        if (adbus_iter_align(&i, 8))
            return -1;

        fieldbegin = i.data;

        if (adbus_iter_beginstruct(&i))
            return -1;
        if (adbus_iter_u8(&i, &code))
            return -1;
        if (adbus_iter_value(&i))
            return -1;
        if (adbus_iter_endstruct(&i))
            return -1;
        if (adbus_iter_align(&i, 8))
            return -1;

        fieldend = i.data;

        if (*code == ADBUSI_HEADER_SENDER) {
            /* Remove the sender field */
            size_t fieldsz = fieldend - fieldbegin;
            adbus_buf_remove(p->buffer, fieldbegin - adbus_buf_data(p->buffer), fieldsz);
            i.data  -= fieldsz;
            a.size  -= fieldsz;
        }
    }

    /* Append sender field */
    {
        adbus_Buffer* b = p->buffer;
        adbus_BufVariant v;
        adbus_buf_setsig(b, "(yv)", 4);
        adbus_buf_beginstruct(b);
        adbus_buf_u8(b, ADBUSI_HEADER_SENDER);
        adbus_buf_beginvariant(b, &v, "s", 1);
        adbus_buf_string(b, ds_cstr(&r->unique), ds_size(&r->unique));
        adbus_buf_endvariant(b, &v);
        adbus_buf_endstruct(b);
    }

    h->headerFieldLength = adbus_buf_data(p->buffer) 
                         + adbus_buf_size(p->buffer) 
                         - data 
                         - sizeof(adbusI_ExtendedHeader);

    adbus_buf_align(p->buffer, 8);

    p->msgSize = p->msgSize - p->headerSize
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
    adbusI_ServerParser* p = &r->parser;

    assert(p->state == PARSE_DISPATCH);

    p->native = 1;
    p->headerSize = m->size - m->argsize;
    p->msgSize = m->size;

    InitBuffer(p);
    adbus_buf_append(p->buffer, m->data, m->size - m->argsize);

    if (FixHeaders(r, p))
        return -1;

    adbus_buf_append(p->buffer, m->argdata, m->argsize);

    return DispatchMsg(r, p);
}







/* -------------------------------------------------------------------------- */

/* Manually unpack even for native endianness since value might not be 4 byte
 * aligned
 */
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

static int Require(adbus_Buffer* dest, const char** data, size_t* size, size_t have, size_t need)
{
    size_t copy = need - have;
    assert(need >= have);
    if (*size >= copy) {
        adbus_buf_append(dest, *data, copy);
        *data += copy;
        *size -= copy;
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
    adbusI_ServerParser* p = &r->parser;
    adbus_Buffer* buffer = p->buffer;

    const char* data = adbus_buf_data(b);
    size_t size = adbus_buf_size(b);

    switch (p->state) {
        while (1) {
            case PARSE_DISPATCH:
            case PARSE_BEGIN:
                {
                    size_t hsize;
                    adbusI_ExtendedHeader* h = (adbusI_ExtendedHeader*) data;

                    /* Copy header */

                    p->state = PARSE_BEGIN;
                    if (size < sizeof(adbusI_ExtendedHeader))
                        goto end;

                    hsize           = sizeof(adbusI_ExtendedHeader)
                                    + Get32(h->endianness, &h->headerFieldLength);

                    p->native       = (h->endianness == adbusI_nativeEndianness());
                    p->headerSize   = ADBUS_ALIGN(hsize, 8);
                    p->msgSize      = p->headerSize + Get32(h->endianness, &h->length);

                    InitBuffer(p);

                    /* fall through */
                }

            case PARSE_HEADER:
                {
                    adbus_Message* m;
                    char* mdata;
                    size_t msize;

                    /* Copy header fields */
                    p->state = PARSE_HEADER;

                    UnpackBuffer(p, &m, &mdata, &msize);

                    if (Require(buffer, &data, &size, msize, p->headerSize))
                        goto end;

                    if (FixHeaders(r, p))
                        return -1;

                    /* fall through */
                }

            case PARSE_DATA:
                {
                    adbus_Message* m;
                    char* mdata;
                    size_t msize;

                    /* Copy data */
                    p->state = PARSE_DATA;

                    UnpackBuffer(p, &m, &mdata, &msize);

                    if (Require(buffer, &data, &size, msize, p->msgSize))
                        goto end;

                    if (DispatchMsg(r, p))
                        return -1;

                    /* loop around */
                }
        }
    }

end:
    adbus_buf_remove(b, 0, adbus_buf_size(b) - size);
    return 0;
}


