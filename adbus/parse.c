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

#include "parse.h"
#include <stddef.h>

/** \struct adbus_Message
 *  
 *  \brief Container for parsed messages
 *
 *  A message structure can be filled out by using adbus_parse() or it may be
 *  filled out manually. When filling out manually all fields except arguments
 *  are required to be set if they are in the message (otherwise they should
 *  be zero initialised). Most places that require arguments to be set should
 *  call adbus_parseargs() before using it.
 *
 *  \warning adbus_parseargs() allocates some space on the heap to store the
 *  arguments vector. Call adbus_freeargs() to free the arguments vector when
 *  finished using them.
 *
 *  For example to parse some incoming data and dispatch using
 *  adbus_conn_dispatch() (this is just a simple version of
 *  adbus_conn_parse()):
 *
 *  \code
 *  // These initialised somewhere else
 *  static adbus_Buffer* buf;
 *  static adbus_Connect* connection;
 *  int ParseData(char* data, size_t sz)
 *  {
 *      // Copy the data into a buffer to ensure its 8 byte aligned
 *      adbus_buf_append(buf, data, sz);
 *      size_t msgsz = 0;
 *      while (1)
 *      {
 *          char* msgdata = adbus_buf_data(buf);
 *          size_t bufdata = adbus_buf_size(buf);
 *          msgsz = adbus_parse_size(msgdata, bufdata);
 *          // Need more data
 *          if (msgsz == 0 || msgsz > bufdata)
 *              return 0;
 *          
 *          adbus_Message m = {};
 *          
 *          // Check for parse errors
 *          if (adbus_parse(&m, msgdata, msgsz))
 *              return -1;
 *
 *          // adbus_conn_dispatch() will return an error on a parse error
 *          // detected further down
 *          int ret = adbus_conn_dispatch(connection, &m);
 *          adbus_freeargs(&m);
 *          if (ret)
 *              return ret;
 *      }
 *  }
 *  \endcode
 */

/** \var adbus_Message::data
 *  Beginning of message data (must be 8 byte aligned).
 */

/** \var adbus_Message::size
 *  Size of message data.
 */

/** \var adbus_Message::argdata
 *  Beginning of argument data.
 */

/** \var adbus_Message::argsize
 *  Size of argument data.
 */

/** \var adbus_Message::type
 *  Type of message.
 */

/** \var adbus_Message::flags
 *  Message flags.
 */

/** \var adbus_Message::serial
 *  Message serial - used to correlate method calls with replies.
 */

/** \var adbus_Message::signature
 *  Argument signature or NULL if not present.
 */

/** \var adbus_Message::signatureSize
 *  Length of adbus_Message::signature field.
 */

/** \var adbus_Message::replySerial
 *  Pointer to reply serial value of -1 if not present.
 */

/** \var adbus_Message::path
 *  Object path header field or NULL if not present.
 */

/** \var adbus_Message::pathSize
 *  Length of path field.
 */

/** \var adbus_Message::interface
 *  Interface header field or NULL if not present.
 */

/** \var adbus_Message::interfaceSize
 *  Length of interface field.
 */


/** \var adbus_Message::member
 *  Member header field or NULL if not present.
 */

/** \var adbus_Message::memberSize
 *  Length of member field.
 */


/** \var adbus_Message::error
 *  Error name header field or NULL if not present.
 */

/** \var adbus_Message::errorSize
 *  Length of error field.
 */

/** \var adbus_Message::destination
 *  Destination header field or NULL if not present.
 */

/** \var adbus_Message::destinationSize
 *  Length of destination field.
 */

/** \var adbus_Message::sender
 *  Sender header field or NULL if not present.
 */

/** \var adbus_Message::senderSize
 *  Length of sender field.
 */

/** \var adbus_Message::arguments
 *  Array of unpacked arguments.
 *
 *  This should only be used for matching against match rules. For proper
 *  unpacking use adbus_Iterator.
 */

/** \var adbus_Message::argumentsSize
 *  Size of arguments field.
 */

/* --------------------------------------------------------------------------
 * Manually unpack even for native endianness since value not be 4 byte
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

/** Figures out the size of a message.
 *
 *  \relates adbus_Message
 *
 *  The data does not need to be aligned.
 *
 *  \returns -1 on parse error
 *  \returns 0 on insufficient data
 *  \returns message size
 *
 */
int adbus_parse_size(const char* data, size_t size)
{
    adbusI_ExtendedHeader* h = (adbusI_ExtendedHeader*) data;
    size_t msgsize, hsize;

    if (size < sizeof(adbusI_ExtendedHeader))
        return 0;

    hsize = sizeof(adbusI_ExtendedHeader) + Get32(h->endianness, &h->headerFieldLength);
    msgsize = ADBUS_ALIGN(hsize, 8) + Get32(h->endianness, &h->length);
    if (msgsize > ADBUSI_MAXIMUM_MESSAGE_LENGTH)
        return -1;

    return (int) msgsize;
}

/** Fills out an adbus_Message by parsing the data.
 *
 *  \relates adbus_Message
 *
 *  \param[in] m    The message structure to fill out.
 *  \param[in] data Data to parse. This \e must be 8 byte aligned.
 *  \param[in] size The size of the message. This \e must be the size of the
 *                  message exactly.
 *
 *  Parses a message and fills out the message structure. It will endian flip
 *  the data if it is not native (hence char* instead of const char*). The
 *  pointers in the message structure will point into the passed data.
 *
 *  Due to the 8 byte alignment the size normally needs to be known
 *  beforehand to copy the data into an 8 byte aligned buffer.
 *  adbus_parse_size() can be used to figure out the message size.
 *
 *  \sa adbus_parse_size()
 */ 
int adbus_parse(adbus_Message* m, char* data, size_t size)
{
    adbusI_ExtendedHeader* h = (adbusI_ExtendedHeader*) data;
    adbus_Bool native = (h->endianness == adbusI_nativeEndianness());
    adbus_Iterator i;
    adbus_IterArray a;

    assert((int) size == adbus_parse_size(data, size));
    assert((char*) ADBUS_ALIGN(data, 8) == data);
    assert(size > sizeof(adbusI_ExtendedHeader));

    if (h->type == ADBUS_MSG_INVALID) {
        return -1;
    } else if (h->type > ADBUS_MSG_SIGNAL) {
        return 0;
    }

    if (!native && adbus_flip_data(data, size, "yyyyuua(yv)"))
        return -1;

    h->endianness = adbusI_nativeEndianness();

    memset(m, 0, sizeof(adbus_Message));
    m->replySerial = -1;

    m->data     = data;
    m->size     = ADBUS_ALIGN(h->headerFieldLength + sizeof(adbusI_ExtendedHeader), 8) + h->length;
    m->argsize  = h->length;
    m->argdata  = data + m->size - m->argsize;
    m->type     = (adbus_MessageType) h->type;
    m->flags    = h->flags;
    m->serial   = h->serial;

    i.data = data + sizeof(adbusI_Header);
    i.size = size - sizeof(adbusI_Header);
    i.sig  = "a(yv)";
    if (adbus_iter_beginarray(&i, &a))
        return -1;

    while (adbus_iter_inarray(&i, &a)) {
        uint8_t code;
        uint32_t replySerial;
        adbus_IterVariant v;
        const char** pstr;
        size_t* psize;

        if (    adbus_iter_beginstruct(&i) 
            ||  adbus_iter_u8(&i, &code)
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

            case ADBUSI_HEADER_SENDER:
                pstr  = &m->sender;
                psize = &m->senderSize;
                goto string;

            string:
                if (    i.sig[0] != 's'
                    ||  i.sig[1] != '\0'
                    ||  adbus_iter_string(&i, pstr, psize))
                {
                    return -1;
                }
                break;

            case ADBUSI_HEADER_OBJECT_PATH:
                if (    i.sig[0] != 'o' 
                    ||  i.sig[1] != '\0'
                    ||  adbus_iter_objectpath(&i, &m->path, &m->pathSize))
                {
                    return -1;
                }
                break;

            case ADBUSI_HEADER_SIGNATURE:
                if (    i.sig[0] != 'g' 
                    ||  i.sig[1] != '\0'
                    ||  adbus_iter_signature(&i, &m->signature, &m->signatureSize))
                {
                    return -1;
                }
                break;

            case ADBUSI_HEADER_REPLY_SERIAL:
                if (    i.sig[0] != 'u' 
                    ||  i.sig[1] != '\0'
                    ||  adbus_iter_u32(&i, &replySerial))
                {
                    return -1;
                }
                m->replySerial = replySerial;
                break;

            default:
                if (adbus_iter_value(&i)) {
                    return -1;
                }
                break;
        }

        if (adbus_iter_endvariant(&i, &v) || adbus_iter_endstruct(&i))
            return -1;

    }

    if (adbus_iter_endarray(&i, &a))
        return -1;

    /* Check that we have the required fields */
    if (m->type == ADBUS_MSG_METHOD && (!m->path || !m->member)) {
        return -1;
    } else if (m->type == ADBUS_MSG_RETURN && m->replySerial < 0) {
        return -1;
    } else if (m->type == ADBUS_MSG_ERROR && !m->error) {
        return -1;
    } else if (m->type == ADBUS_MSG_SIGNAL &&  (!m->interface || !m->member)) {
        return -1;
    } else if (m->argsize > 0 && !m->signature) {
        return -1;
    }

    if (!native && m->signature && adbus_flip_data((char*) m->argdata, m->argsize, m->signature))
        return -1;

    return 0;
}

DVECTOR_INIT(Argument, adbus_Argument)

/** Parse the arguments in a message.
 *  
 *  \relates adbus_Message
 *
 *  This should be called after filling out the message struct using
 *  adbus_parse() or manually and will fill out the arguments and
 *  argumentsSize fields. These fields only tell you about string arguments
 *  and should only be used for matching match rules.
 *
 *  \warning The argument vector filled out in the message structure is
 *  allocated on the heap. You must use adbus_freeargs() after using the
 *  message to free this.
 *
 */
int adbus_parseargs(adbus_Message* m)
{
    d_Vector(Argument) args;
    adbus_Iterator i;

    if (m->arguments)
        return 0;

    ZERO(args);
    i.data = m->argdata;
    i.size = m->argsize;
    i.sig  = m->signature;

    assert(m->signature != NULL);

    while (*i.sig) {
        adbus_Argument* arg = dv_push(Argument, &args, 1);
        arg->value = NULL;
        arg->size  = 0;

        if (*i.sig == 's') {
            size_t sz;
            if (adbus_iter_string(&i, &arg->value, &sz))
                goto err;
            arg->size = sz;

        } else {
            if (adbus_iter_value(&i))
                goto err;

        }
    }

    m->argumentsSize = dv_size(&args);
    m->arguments = dv_release(Argument, &args);
    return 0;

err:
    dv_free(Argument, &args);
    return -1;
}

/** Clones the message data in \a from into \a to.
 *  \relates adbus_Message
 */
void adbus_clonedata(adbus_Buffer* buf, adbus_Message* from, adbus_Message* to)
{
    ptrdiff_t off;
	size_t alloc;

	memcpy(to, from, sizeof(adbus_Message));

	alloc = ADBUS_ALIGN(from->size, 8) + sizeof(adbus_Argument) * from->argumentsSize;
	adbus_buf_resize(buf, alloc);

	to->data = adbus_buf_data(buf);

    memcpy((char*) to->data, from->data, from->size);

    off = to->data - from->data;

    /* Update all data pointers to point into the new data section */
    to->argdata += off;

	if (to->signature) {
	    to->signature += off;
	}

	if (to->path) {
	    to->path += off;
	}

	if (to->interface) {
	    to->interface += off;
	}

	if (to->member) {
	    to->member += off;
	}

	if (to->error) {
	    to->error += off;
	}

	if (to->destination) {
	    to->destination += off;
	}

	if (to->sender) {
	    to->sender += off;
	}

    if (from->arguments) {
        size_t i;
		to->arguments = (adbus_Argument*) ADBUS_ALIGN(to->data + to->size, 8);
		memcpy(to->arguments, from->arguments, sizeof(adbus_Argument) * from->argumentsSize);

        for (i = 0; i < to->argumentsSize; i++) {
            if (to->arguments[i].value) {
                to->arguments[i].value += off;
            }
        }
    }

}

void adbus_freeargs(adbus_Message* m)
{ free(m->arguments); }

