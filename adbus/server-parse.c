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
    adbusI_Header* dh;
    adbus_Bool native = (sh->endianness == adbusI_nativeEndianness());
    adbus_Iterator i;
    adbus_IterArray a;
    size_t headerFieldLength;
    size_t headerLength;
    char* bdata;
    size_t bsize;

    assert(size > sizeof(adbusI_ExtendedHeader));

    if (sh->type == ADBUS_MSG_INVALID) {
        return -1;
    } else if (sh->type > ADBUS_MSG_SIGNAL) {
        return 1;
    }

    memset(m, 0, sizeof(adbus_Message));

    headerFieldLength = Get32(sh->endianness, &sh->headerFieldLength);
    headerLength = sizeof(adbusI_ExtendedHeader) + ADBUS_ALIGN(headerFieldLength, 8);

    adbus_buf_reset(b);
    adbus_buf_append(b, data, headerLength);
    bdata = adbus_buf_data(b);
    bsize = adbus_buf_size(b);
    if (!native && adbus_flip_data(bdata, bsize, "yyyyuua(yv)"))
        return -1;

    data += headerLength;
    size -= headerLength;

    dh              = (adbusI_Header*) bdata;
    dh->endianness  = adbusI_nativeEndianness();

    m->type         = sh->type;
    m->flags        = sh->flags;
    m->serial       = sh->serial;

    i.data = bdata + sizeof(adbusI_Header);
    i.size = bsize - sizeof(adbusI_Header);
    i.sig  = "a(yv)";
    if (adbus_iter_beginarray(&i, &a))
        return -1;

    while (adbus_iter_inarray(&i, &a)) {
        uint8_t code;
        adbus_IterVariant v;
        const char** pstr;
        size_t* psize;
        const char *fieldBegin, *fieldEnd;
        uint32_t replySerial;

        if (adbus_iter_beginstruct(&i))
            return -1;

        fieldBegin = i.data;

        if (    adbus_iter_u8(&i, &code)
            ||  adbus_iter_beginvariant(&i, &v))
        {
            return -1;
        }

        switch (code) {
            case ADBUSI_HEADER_INVALID:
                return -1;

            case ADBUSI_HEADER_INTERFACE:
                pstr  = &m->interface;
                psize = &m->interfaceSize;
                goto string;

            case ADBUSI_HEADER_MEMBER:
                pstr  = &m->member;
                psize = &m->memberSize;
                goto string;

            case ADBUSI_HEADER_ERROR_NAME:
                pstr  = &m->error;
                psize = &m->errorSize;
                goto string;

            case ADBUSI_HEADER_DESTINATION:
                pstr  = &m->destination;
                psize = &m->destinationSize;
                goto string;

            string:
                if (    i.sig[0] != 's'
                    ||  i.sig[1] != '\0'
                    ||  adbus_iter_string(&i, pstr, psize))
                {
                    return -1;
                }
                *pstr -= (uintptr_t) bdata;
                goto finish;

            case ADBUSI_HEADER_OBJECT_PATH:
                if (    i.sig[0] != 'o' 
                    ||  i.sig[1] != '\0'
                    ||  adbus_iter_objectpath(&i, &m->path, &m->pathSize))
                {
                    return -1;
                }
                m->path -= (uintptr_t) bdata;
                goto finish;

            case ADBUSI_HEADER_SIGNATURE:
                if (    i.sig[0] != 'g' 
                    ||  i.sig[1] != '\0'
                    ||  adbus_iter_signature(&i, &m->signature, &m->signatureSize))
                {
                    return -1;
                }
                m->signature -= (uintptr_t) bdata;
                goto finish;

            case ADBUSI_HEADER_REPLY_SERIAL:
                if (    i.sig[0] != 'u' 
                    ||  i.sig[1] != '\0'
                    ||  adbus_iter_u32(&i, &replySerial))
                {
                    return -1;
                }
                m->replySerial = replySerial;
                goto finish;

            default:
                if (adbus_iter_value(&i)) {
                    return -1;
                }
                goto finish;

            finish:
                if (adbus_iter_endvariant(&i, &v) || adbus_iter_endstruct(&i))
                    return -1;
                break;

            case ADBUSI_HEADER_SENDER:
                if (    i.sig[0] != 's'
                    ||  i.sig[1] != '\0'
                    ||  adbus_iter_string(&i, NULL, NULL)
                    ||  adbus_iter_endvariant(&i, &v)
                    ||  adbus_iter_endstruct(&i)
                    ||  adbus_iter_align(&i, 8))
                {
                    return -1;
                }
                fieldEnd = i.data;
                i.data -= (uintptr_t) bdata;
                adbus_buf_remove(b, fieldBegin - bdata, fieldEnd - fieldBegin);
                bdata = adbus_buf_data(b);
                bsize = adbus_buf_size(b);
                i.data += (uintptr_t) bdata;
                break;
        }
    }

    {
        adbus_BufVariant v;
        adbus_buf_setsig(b, "(yv)", -1);
        adbus_buf_beginstruct(b);
        adbus_buf_u8(b, ADBUSI_HEADER_SENDER);
        adbus_buf_beginvariant(b, &v, "s", 1);
        adbus_buf_align(b, 4);
        m->sender = (char*) (adbus_buf_size(b) + 4);
        m->senderSize = ds_size(&r->unique);
        adbus_buf_string(b, ds_cstr(&r->unique), ds_size(&r->unique));
        adbus_buf_endvariant(b, &v);
        adbus_buf_endstruct(b);

        bdata = adbus_buf_data(b);
        bsize = adbus_buf_size(b);

        headerLength = bsize;

        ((adbusI_ExtendedHeader*) bdata)->headerFieldLength = bsize - sizeof(adbusI_ExtendedHeader);

        adbus_buf_align(b, 8);
    }

    {
        size_t headersize = adbus_buf_size(b);
        adbus_buf_append(b, data, size);
        bdata = adbus_buf_data(b);
        bsize = adbus_buf_size(b);

        if (m->signature) {
            m->signature += (uintptr_t) bdata;
        }

        if (m->path) {
            m->path += (uintptr_t) bdata;
        }

        if (m->interface) {
            m->interface += (uintptr_t) bdata;
        }

        if (m->member) {
            m->member += (uintptr_t) bdata;
        }

        if (m->error) {
            m->error += (uintptr_t) bdata;
        }

        if (m->destination) {
            m->destination += (uintptr_t) bdata;
        }

        if (m->sender) {
            m->sender += (uintptr_t) bdata;
        }

        m->data = bdata;
        m->size = bsize;

        m->argdata = bdata + headerLength;
        m->argsize = bsize - headerLength;

        if (!native && m->signature && adbus_flip_data(bdata + headersize, bsize - headersize, m->signature))
            return -1;
    }

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

    while (1) {
        int ret;

        size_t msgsize = adbus_parse_size(data, size);
        if (msgsize == 0 || msgsize > size)
            break;

        ret = ServerParse(r, r->parseBuffer, &m, data, msgsize);

        if (ret < 0)
            return -1;

        if (!ret)
            ret = adbusI_serv_dispatch(r->server, r, &m);

        adbus_freeargs(&m);

        if (ret < 0)
            return -1;

        data += msgsize;
        size -= msgsize;

    }

    adbus_buf_remove(b, 0, adbus_buf_size(b) - size);
    return 0;
}



/** Dispatches a message from the given remote
 *  \relates adbus_Server
 *
 *  \return non-zero on error at which point the remote should be kicked
 */
int adbus_remote_dispatch(adbus_Remote* r, adbus_Message* m)
{
    int ret;
    adbus_Message m2;
    assert(adbus_parse_size(m->data, m->size) == m->size);

    ret = ServerParse(r, r->parseBuffer, &m2, m->data, m->size);

    if (ret < 0)
        return -1;

    if (!ret && adbusI_serv_dispatch(r->server, r, &m2))
        return -1;

    return 0;
}



