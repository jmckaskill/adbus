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

#include "message.h"
#include "parse.h"


/** \struct adbus_MsgFactory
 *
 *  \brief Packs up messages for sending
 *
 *  The general workflow is:
 *  -# Setup all of the various header fields
 *  -# Add arguments
 *  -# Build a message into a message struct
 *
 *  Steps 1 and 2 can be intermixed.
 *
 *  After that you can immediately send it off using adbus_conn_send() or with
 *  adbus_msg_send() (this also builds the message) or clone the data to send
 *  it later.
 *
 *  \warning After adbus_msg_build() the pointers in the message struct point
 *  into the factories buffers so it should be used immediately (or the data
 *  cloned) before calling any further factory functions.
 *
 *  For example to send a method call to request the service "com.example":
 *
 *  \code
 *  // Initialised elsewhere
 *  static adbus_MsgFactory* msg;
 *  static adbus_Connection* connection;
 *
 *  adbus_msg_clear(msg);
 *
 *  // Setting up the header
 *  adbus_msg_settype(msg, ADBUS_MSG_METHOD);
 *  adbus_msg_setflags(msg, ADBUS_MSG_NO_REPLY);
 *  adbus_msg_setserial(msg, adbus_conn_serial(connection));
 *  adbus_msg_setdestination(msg, "org.freedesktop.DBus", -1);
 *  adbus_msg_setpath(msg, "/org/freedesktop/DBus", -1);
 *  adbus_msg_setinterface(msg, "org.freedesktop.DBus", -1);
 *  adbus_msg_setmember(msg, "RequestName", -1);
 *
 *  // Adding the arguments
 *  adbus_msg_setsig(msg, "su", -1);
 *  adbus_msg_string(msg, "com.example", -1);
 *  adbus_msg_u32(msg, 0);
 *
 *  // Send the message
 *  adbus_conn_send(msg, connection);
 *  \endcode
 *
 *  To clone and send the message on another thread:
 *
 *  \code
 *  adbus_Message* m = (adbus_Message*) calloc(sizeof(adbus_Message), 1);
 *  adbus_msg_build(msg, m);
 *  adbus_clonedata(m);
 *  SendToOtherThread(m);
 *
 *  // .... on the other thread
 *  void ReceiveMessage(adbus_Message* m)
 *  {
 *      adbus_conn_send(connection, m);
 *      adbus_freedata(m);
 *      free(m);
 *  }
 *  \endcode
 *
 */


/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Creates a new message factory.
 *  \relates adbus_MsgFactory
 */
adbus_MsgFactory* adbus_msg_new(void)
{
    adbus_MsgFactory* m = NEW(adbus_MsgFactory);
    m->buf = adbus_buf_new();
    m->argbuf = adbus_buf_new();
    m->serial = -1;
    m->replySerial = -1;
    ADBUSI_LOG_2("new (msg %p)", (void*) m);
    return m;
}

/* ------------------------------------------------------------------------- */

/** Frees a message factory.
 *  \relates adbus_MsgFactory
 */
void adbus_msg_free(adbus_MsgFactory* m)
{
    if (!m)
        return;

    ADBUSI_LOG_2("free (msg %p)", (void*) m);

    adbus_buf_free(m->buf);
    adbus_buf_free(m->argbuf);
    ds_free(&m->path);
    ds_free(&m->interface);
    ds_free(&m->member);
    ds_free(&m->error);
    ds_free(&m->destination);
    free(m);
}

/* ------------------------------------------------------------------------- */

/** Resets the message factory for reuse.
 *  \relates adbus_MsgFactory
 */
void adbus_msg_reset(adbus_MsgFactory* m)
{
    m->messageType      = ADBUS_MSG_INVALID;
    m->flags            = 0;
    m->serial           = -1;
    m->argumentOffset   = 0;
    m->replySerial      = -1;

    adbus_buf_reset(m->buf);
    adbus_buf_reset(m->argbuf);
    ds_clear(&m->interface);
    ds_clear(&m->member);
    ds_clear(&m->error);
    ds_clear(&m->destination);
}

/* -------------------------------------------------------------------------
 * Getter functions
 * -------------------------------------------------------------------------
 */

static void SignatureField(char** dest, uint32_t code, const char* source, size_t sz, const char** pstr, const char** pend)
{
    /* Find where stuff is, zero the padding, and then set stuff.
     * p32[0] is code
     * p8[0] is string size
     * p8[1] and on is string
     *
     * p64 is the last 8 bytes of the field used to set the padding. Set
     * this first as this will overlap with the other fields.
     *
     * pend is the unaligned end of the field.
     */

    char* str = NULL;
    uint8_t* p8;
    uint64_t* p64;
    uint32_t* p32 = (uint32_t*) *dest;

    /* Find the string */
    p8 = (uint8_t*) &p32[1];
    str = (char*) &p8[1];
    *pend = str + sz + 1;

    /* Find the padding */
    p64 = (uint64_t*) (str + sz);
    p64 = (uint64_t*) (((uintptr_t) p64) & ~0x07);

    /* Set *dest to the 8 byte aligned end of the field */
    *dest = (char*) &p64[1];

    /* Zero the padding */
    p64[0] = UINT64_C(0);

    /* Code includes u8 code, variant sigsz, sig, and signull */
    p32[0] = code;

    /* Size of string */
    p8[0] = (uint8_t) sz;

    /* Copy over the string */
    memcpy(str, source, sz);

    *pstr = str;
}

static void StringField(char** dest, uint32_t code, d_String* field, const char** pstr, size_t* psz)
{
    char* str = NULL;
    size_t sz = ds_size(field);
    if (sz > 0) {

        /* Find where stuff is, zero the padding, and then set stuff.
         * p32[0] is code
         * p32[1] is string size
         * p32[2] and on is string
         *
         * p64 is the last 8 bytes of the field used to set the padding. Set
         * this first as this will overlap with the other fields.
         */

        uint64_t* p64;
        uint32_t* p32 = (uint32_t*) *dest;

        /* Find the string */
        str = (char*) &p32[2];

        /* Find the padding */
        p64 = (uint64_t*) (str + sz);
        p64 = (uint64_t*) (((uintptr_t) p64) & ~0x07);

        /* Set *dest to the 8 byte aligned end of the field */
        *dest = (char*) &p64[1];

        /* Zero the padding */
        p64[0] = UINT64_C(0);

        /* Code includes u8 code, variant sigsz, sig, and signull */
        p32[0] = code;

        /* Size of string */
        p32[1] = (uint32_t) sz;

        /* Copy over the string */
        memcpy(str, ds_cstr(field), sz);
    }
    *pstr = str;
    *psz = sz;
}

static void ReplySerialField(char** dest, uint32_t code, int64_t value)
{
    if (value >= 0) {
        uint32_t* p32 = (uint32_t*) *dest;
        p32[0] = code;
        p32[1] = (uint32_t) value;
        *dest += 8;
    }
}

/** Builds the message and sets the fields in the message struct.
 *  
 *  \relates adbus_MsgFactory
 *
 *  \param[in] m   The message factory
 *  \param[in] msg A zero initialised message struct to fill out
 *
 *  \return 0 on success
 *  \return non zero on error
 *
 *  This will not fill out the arguments and argumentsSize fields.
 *
 *  Will assert if the required header fields for the given message type are
 *  not set.
 *
 *  \warning The pointers set in the message struct point into the message
 *  factory buffers. So any changes to the factory may invalidate the message
 *  struct.
 *
 */
int adbus_msg_build(adbus_MsgFactory* m, adbus_Message* msg)
{
    adbusI_ExtendedHeader* header;
    const char *sig, *argdata, *unalignedHeaderEnd;
    size_t alloc;
    char* dest;

    /* Check that we have the required fields */
    if (m->serial < 0) {
        assert(0);
        return -1;
    }

    if (m->messageType == ADBUS_MSG_METHOD) {
        if (ds_size(&m->path) == 0 || ds_size(&m->member) == 0) {
            assert(0);
            return -1;
        }
    } else if (m->messageType == ADBUS_MSG_RETURN) {
        if (m->replySerial < 0) {
            assert(0);
            return -1;
        }

    } else if (m->messageType == ADBUS_MSG_ERROR) {
        if (m->replySerial < 0 || ds_size(&m->error) == 0) {
            assert(0);
            return -1;
        }

    } else if (m->messageType == ADBUS_MSG_SIGNAL) {
        if (ds_size(&m->member) == 0 || ds_size(&m->interface) == 0) {
            assert(0);
            return -1;
        }

    } else {
        assert(0);
        return -1;
    }

    /* We don't zero the message as we should set every value. Shred in on
     * debug to check that we've got everything.
     */
#ifndef NDEBUG
    memset(msg, 0xAD, sizeof(adbus_Message));
#endif

    /* We don't fill these fields out */
    msg->sender = NULL;
    msg->senderSize = 0;

    argdata = adbus_buf_data(m->argbuf);
    msg->argsize = adbus_buf_size(m->argbuf);

    sig = adbus_buf_sig(m->argbuf, &msg->signatureSize);

    alloc = ds_size(&m->path)
          + ds_size(&m->interface)
          + ds_size(&m->member)
          + ds_size(&m->error)
          + ds_size(&m->destination)
          + msg->argsize
          + sizeof(adbusI_ExtendedHeader)
          + 8       /* for reply_serial */
          + 6 * 16; /* for each string field: 4 for code, 4 for string size, 8 for padding */

    adbus_buf_resize(m->buf, alloc);
    dest = adbus_buf_data(m->buf);
    msg->data = dest;

    /* Shred to make sure we zero the padding correctly */
#ifndef NDEBUG
    memset(dest, '?', alloc);
#endif

    header = (adbusI_ExtendedHeader*) dest;

    header->endianness  = ADBUSI_NATIVE_ENDIANNESS;
    header->type        = (uint8_t) m->messageType;
    header->flags       = (uint8_t) m->flags;
    header->version     = ADBUSI_MAJOR_PROTOCOL_VERSION;
    header->length      = (uint32_t) msg->argsize;
    header->serial      = (uint32_t) m->serial;

    msg->type           = m->messageType;
    msg->flags          = m->flags;
    msg->serial         = (uint32_t) m->serial;
    msg->replySerial    = m->replySerial;

    dest += sizeof(adbusI_ExtendedHeader);

    /* Add the headers manually. This could be done using the adbus_buf methods
     * but it gets hit a lot and we already know that the alignment is going to be
     * correct for many of the fields. Also we need pointers into the data
     * structure to fill out the adbus_Message fields, which is otherwise
     * tricky.
     */
    ReplySerialField(&dest, ADBUSI_HEADER_REPLY_SERIAL_LONG, m->replySerial);
    StringField(&dest, ADBUSI_HEADER_OBJECT_PATH_LONG, &m->path, &msg->path, &msg->pathSize); 
    StringField(&dest, ADBUSI_HEADER_INTERFACE_LONG, &m->interface, &msg->interface, &msg->interfaceSize); 
    StringField(&dest, ADBUSI_HEADER_MEMBER_LONG, &m->member, &msg->member, &msg->memberSize); 
    StringField(&dest, ADBUSI_HEADER_ERROR_NAME_LONG, &m->error, &msg->error, &msg->errorSize); 
    StringField(&dest, ADBUSI_HEADER_DESTINATION_LONG, &m->destination, &msg->destination, &msg->destinationSize); 

    /* Always append the signature field even though we may not need it. This
     * way we only have to figure out the non-aligned end of the header array
     * for one field.
     */
    SignatureField(&dest, ADBUSI_HEADER_SIGNATURE_LONG, sig, msg->signatureSize, &msg->signature, &unalignedHeaderEnd);

    assert((char*) ADBUS_ALIGN(dest, 8) == dest);
    assert(dest - 8 < unalignedHeaderEnd && unalignedHeaderEnd <= dest);

    header->headerFieldLength = unalignedHeaderEnd - (char*) &header[1];

    memcpy(dest, argdata, msg->argsize);
    msg->argdata = dest;
    msg->size = dest + msg->argsize - msg->data;

    return 0;
}

/* ------------------------------------------------------------------------- */

/** Builds and then sends a message.
 *  \relates adbus_MsgFactory
 */
int adbus_msg_send(adbus_MsgFactory* m, adbus_Connection* c)
{
    int ret;
    adbus_Message msg;

    if (adbus_msg_serial(m) < 0) {
        adbus_msg_setserial(m, adbus_conn_serial(c));
    }

    if (adbus_msg_build(m, &msg))
        return -1;

    return adbus_conn_send(c, &msg);
}

/* ------------------------------------------------------------------------- */

/** Returns the current value of the object path header field.
 *
 *  \relates adbus_MsgFactory
 *
 *  \return NULL if the field is not set
 */
const char* adbus_msg_path(const adbus_MsgFactory* m, size_t* len)
{
    if (len)
        *len = ds_size(&m->path);
    if (ds_size(&m->path) > 0)
        return ds_cstr(&m->path);
    else
        return NULL;
}

/* ------------------------------------------------------------------------- */

/** Returns the current value of the interface header field.
 *
 *  \relates adbus_MsgFactory
 *
 *  \return NULL if the field is not set
 */
const char* adbus_msg_interface(const adbus_MsgFactory* m, size_t* len)
{
    if (len)
        *len = ds_size(&m->interface);
    if (ds_size(&m->interface) > 0)
        return ds_cstr(&m->interface);
    else
        return NULL;
}

/* ------------------------------------------------------------------------- */

/** Returns the current value of the destination header field.
 *
 *  \relates adbus_MsgFactory
 *
 *  \return NULL if the field is not set
 */
const char* adbus_msg_destination(const adbus_MsgFactory* m, size_t* len)
{
    if (len)
        *len = ds_size(&m->destination);
    if (ds_size(&m->destination) > 0)
        return ds_cstr(&m->destination);
    else
        return NULL;
}

/* ------------------------------------------------------------------------- */

/** Returns the current value of the member header field.
 *
 *  \relates adbus_MsgFactory
 *
 *  \return NULL if the field is not set
 */
const char* adbus_msg_member(const adbus_MsgFactory* m, size_t* len)
{
    if (len)
        *len = ds_size(&m->member);
    if (ds_size(&m->member) > 0)
        return ds_cstr(&m->member);
    else
        return NULL;
}

/* ------------------------------------------------------------------------- */

/** Returns the current value of the error name header field.
 *
 *  \relates adbus_MsgFactory
 *
 *  \return NULL if the field is not set
 */
const char* adbus_msg_error(const adbus_MsgFactory* m, size_t* len)
{
    if (len)
        *len = ds_size(&m->error);
    if (ds_size(&m->error) > 0)
        return ds_cstr(&m->error);
    else
        return NULL;
}

/* ------------------------------------------------------------------------- */

/** Returns the current value of the message type field.
 *  \relates adbus_MsgFactory
 */
adbus_MessageType adbus_msg_type(const adbus_MsgFactory* m)
{
    return m->messageType;
}

/* ------------------------------------------------------------------------- */

/** Returns the current value of the message type field.
 *  \relates adbus_MsgFactory
 */
int adbus_msg_flags(const adbus_MsgFactory* m)
{
    return m->flags;
}

/* ------------------------------------------------------------------------- */

/** Returns the current value of the serial field.
 *  \relates adbus_MsgFactory
 */
int64_t adbus_msg_serial(const adbus_MsgFactory* m)
{
    return m->serial;
}

/* ------------------------------------------------------------------------- */

/** Returns whether the message has a reply serial and the reply serial itself.
 *
 *  \relates adbus_MsgFactory
 *
 *  \return 1 if the message has a reply serial (and then serial will be set
 *  to the reply serial)
 */
adbus_Bool adbus_msg_reply(const adbus_MsgFactory* m, uint32_t* serial)
{
    if (serial && m->replySerial >= 0)
        *serial = (uint32_t) m->replySerial;
    return m->replySerial >= 0;
}

/* -------------------------------------------------------------------------
 * Setter functions
 * -------------------------------------------------------------------------
 */

/** Sets the message type 
 *  \relates adbus_MsgFactory
 */
void adbus_msg_settype(adbus_MsgFactory* m, adbus_MessageType type)
{
    m->messageType = type;
}

/* ------------------------------------------------------------------------- */

/** Sets the message serial
 *
 *  \relates adbus_MsgFactory
 *
 *  The message serial should be unique for the connection. These can be
 *  generated from adbus_conn_serial().
 *
 *  \sa adbus_conn_serial()
 */
void adbus_msg_setserial(adbus_MsgFactory* m, uint32_t serial)
{
    m->serial = serial;
}

/* ------------------------------------------------------------------------- */

/** Sets the message flags
 *  \relates adbus_MsgFactory
 */
void adbus_msg_setflags(adbus_MsgFactory* m, int flags)
{
    m->flags = flags;
}

/* ------------------------------------------------------------------------- */

/** Sets the message reply serial
 *  \relates adbus_MsgFactory
 */
void adbus_msg_setreply(adbus_MsgFactory* m, uint32_t reply)
{
    m->replySerial = reply;
}

/* ------------------------------------------------------------------------- */

/** Sets the message object path field
 *  \relates adbus_MsgFactory
 */
void adbus_msg_setpath(adbus_MsgFactory* m, const char* path, int size)
{
    if (size < 0)
        size = strlen(path);
    ds_set_n(&m->path, path, size);
}

/* ------------------------------------------------------------------------- */

/** Sets the message interface field
 *  \relates adbus_MsgFactory
 */
void adbus_msg_setinterface(adbus_MsgFactory* m, const char* interface, int size)
{
    if (size < 0)
        size = strlen(interface);
    ds_set_n(&m->interface, interface, size);
}

/* ------------------------------------------------------------------------- */

/** Sets the message member field
 *  \relates adbus_MsgFactory
 */
void adbus_msg_setmember(adbus_MsgFactory* m, const char* member, int size)
{
    if (size < 0)
        size = strlen(member);
    ds_set_n(&m->member, member, size);
}

/* ------------------------------------------------------------------------- */

/** Sets the message error name field
 *  \relates adbus_MsgFactory
 */
void adbus_msg_seterror(adbus_MsgFactory* m, const char* error, int size)
{
    if (size < 0)
        size = strlen(error);
    ds_set_n(&m->error, error, size);
}

/* ------------------------------------------------------------------------- */

/** Sets the message destination field
 *  \relates adbus_MsgFactory
 */
void adbus_msg_setdestination(adbus_MsgFactory* m, const char* destination, int size)
{
    if (size < 0)
        size = strlen(destination);
    ds_set_n(&m->destination, destination, size);
}

/* ------------------------------------------------------------------------- */

/** Returns the message argument buffer
 *
 *  \relates adbus_MsgFactory
 *
 *  This should only be used whilst building a message to append arguments.
 *  After the message is built it may not be valid.
 *
 *  If you are directly adding arguments,it is better to use the adbus_msg_
 *  family of functions (eg adbus_msg_bool()). This is exported for use by
 *  general purpose serialisers that convert from a different typing scheme.
 *
 */
adbus_Buffer* adbus_msg_argbuffer(adbus_MsgFactory* m)
{
    return m->argbuf;
}

void adbus_msg_string_f(adbus_MsgFactory* m, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    adbus_buf_string_vf(adbus_msg_argbuffer(m), format, ap);
}


