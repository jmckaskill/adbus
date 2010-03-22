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

#define ADBUS_LIBRARY
#include <adbus.h>
#include "message.h"
#include "misc.h"

#include "dmem/string.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

/** Creates a new message factory.
 *  \relates adbus_MsgFactory
 */
adbus_MsgFactory* adbus_msg_new(void)
{
    adbus_MsgFactory* m = NEW(adbus_MsgFactory);
    m->buf = adbus_buf_new();
    m->argbuf = adbus_buf_new();
    m->serial = -1;
    return m;
}

// ----------------------------------------------------------------------------

/** Frees a message factory.
 *  \relates adbus_MsgFactory
 */
void adbus_msg_free(adbus_MsgFactory* m)
{
    if (!m)
        return;

    adbus_buf_free(m->buf);
    adbus_buf_free(m->argbuf);
    ds_free(&m->path);
    ds_free(&m->interface);
    ds_free(&m->member);
    ds_free(&m->error);
    ds_free(&m->destination);
    ds_free(&m->sender);
    free(m);
}

// ----------------------------------------------------------------------------

/** Resets the message factory for reuse.
 *  \relates adbus_MsgFactory
 */
void adbus_msg_reset(adbus_MsgFactory* m)
{
    m->messageType      = ADBUS_MSG_INVALID;
    m->flags            = 0;
    m->serial           = -1;
    m->argumentOffset   = 0;
    m->replySerial      = 0;
    m->hasReplySerial   = 0;
    adbus_buf_reset(m->buf);
    adbus_buf_reset(m->argbuf);
    ds_clear(&m->interface);
    ds_clear(&m->member);
    ds_clear(&m->error);
    ds_clear(&m->destination);
    ds_clear(&m->sender);
}

// ----------------------------------------------------------------------------
// Getter functions
// ----------------------------------------------------------------------------

static void AppendString(adbus_MsgFactory* m, adbus_BufArray* a, uint8_t code, d_String* field)
{
    if (ds_size(field) > 0) {
        adbus_BufVariant v;
        adbus_buf_arrayentry(m->buf, a);
        adbus_buf_beginstruct(m->buf);
        adbus_buf_u8(m->buf, code);
        adbus_buf_beginvariant(m->buf, &v, "s", 1);
        adbus_buf_string(m->buf, ds_cstr(field), ds_size(field));
        adbus_buf_endvariant(m->buf, &v);
        adbus_buf_endstruct(m->buf);
    }
}

static void AppendSignature(adbus_MsgFactory* m, adbus_BufArray* a, uint8_t code, const char* field)
{
    if (field) {
        adbus_BufVariant v;
        adbus_buf_arrayentry(m->buf, a);
        adbus_buf_beginstruct(m->buf);
        adbus_buf_u8(m->buf, code);
        adbus_buf_beginvariant(m->buf, &v, "g", 1);
        adbus_buf_signature(m->buf, field, -1);
        adbus_buf_endvariant(m->buf, &v);
        adbus_buf_endstruct(m->buf);
    }
}

static void AppendObjectPath(adbus_MsgFactory* m, adbus_BufArray* a, uint8_t code, d_String* field)
{
    if (ds_size(field) > 0) {
        adbus_BufVariant v;
        adbus_buf_arrayentry(m->buf, a);
        adbus_buf_beginstruct(m->buf);
        adbus_buf_u8(m->buf, code);
        adbus_buf_beginvariant(m->buf, &v, "o", 1);
        adbus_buf_objectpath(m->buf, ds_cstr(field), ds_size(field));
        adbus_buf_endvariant(m->buf, &v);
        adbus_buf_endstruct(m->buf);
    }
}

static void AppendUInt32(adbus_MsgFactory* m, adbus_BufArray* a, uint8_t code, uint32_t field)
{
    adbus_BufVariant v;
    adbus_buf_arrayentry(m->buf, a);
    adbus_buf_beginstruct(m->buf);
    adbus_buf_u8(m->buf, code);
    adbus_buf_beginvariant(m->buf, &v, "u", 1);
    adbus_buf_u32(m->buf, field);
    adbus_buf_endvariant(m->buf, &v);
    adbus_buf_endstruct(m->buf);
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
    adbus_buf_reset(m->buf);

    // Check that we have required fields
    CHECK(m->serial >= 0);
    if (m->messageType == ADBUS_MSG_METHOD) {
        CHECK(ds_size(&m->path) > 0 && ds_size(&m->member) > 0);

    } else if (m->messageType == ADBUS_MSG_RETURN) {
        CHECK(m->hasReplySerial);

    } else if (m->messageType == ADBUS_MSG_ERROR) {
        CHECK(ds_size(&m->error) > 0);

    } else if (m->messageType == ADBUS_MSG_SIGNAL) {
        CHECK(ds_size(&m->interface) > 0 && ds_size(&m->member) > 0);

    } else {
        assert(0);
        return -1;
    }

    struct adbusI_Header header;
    header.endianness   = adbusI_nativeEndianness();
    header.type         = (uint8_t) m->messageType;
    header.flags        = (uint8_t) m->flags;
    header.version      = adbusI_majorProtocolVersion;
    header.length       = adbus_buf_size(m->argbuf);
    header.serial       = (uint32_t) m->serial;
    adbus_buf_append(m->buf, (const char*) &header, sizeof(struct adbusI_Header));

    adbus_BufArray a;
    adbus_buf_appendsig(m->buf, "a(yv)", 5);
    adbus_buf_beginarray(m->buf, &a);
    AppendString(m, &a, HEADER_INTERFACE, &m->interface);
    AppendString(m, &a, HEADER_MEMBER, &m->member);
    AppendString(m, &a, HEADER_ERROR_NAME, &m->error);
    AppendString(m, &a, HEADER_DESTINATION, &m->destination);
    AppendString(m, &a, HEADER_SENDER, &m->sender);
    AppendObjectPath(m, &a, HEADER_OBJECT_PATH, &m->path);
    if (m->hasReplySerial)
        AppendUInt32(m, &a, HEADER_REPLY_SERIAL, m->replySerial);
    if (adbus_buf_size(m->argbuf) > 0)
        AppendSignature(m, &a, HEADER_SIGNATURE, adbus_buf_sig(m->argbuf, NULL));
    adbus_buf_endarray(m->buf, &a);

    adbus_buf_align(m->buf, 8);

    adbus_buf_end(m->argbuf);
    if (adbus_buf_size(m->argbuf) > 0)
      adbus_buf_append(m->buf, adbus_buf_data(m->argbuf), adbus_buf_size(m->argbuf));

    memset(msg, 0, sizeof(adbus_Message));

    msg->data               = adbus_buf_data(m->buf);
    msg->size               = adbus_buf_size(m->buf);
    msg->argsize            = adbus_buf_size(m->argbuf);
    msg->argdata            = msg->data + msg->size - msg->argsize;
    msg->type               = m->messageType;
    msg->flags              = m->flags;
    msg->serial             = (uint32_t) m->serial;

    if (msg->argsize > 0) {
        msg->signature          = adbus_buf_sig(m->argbuf, &msg->signatureSize);
    }

    if (ds_size(&m->path) > 0) {
        msg->path               = ds_cstr(&m->path);
        msg->pathSize           = ds_size(&m->path);
    }
    if (ds_size(&m->interface) > 0) {
        msg->interface          = ds_cstr(&m->interface);
        msg->interfaceSize      = ds_size(&m->interface);
    }
    if (ds_size(&m->member) > 0) {
        msg->member             = ds_cstr(&m->member);
        msg->memberSize         = ds_size(&m->member);
    }
    if (ds_size(&m->error) > 0) {
        msg->error              = ds_cstr(&m->error);
        msg->errorSize          = ds_size(&m->error);
    }
    if (ds_size(&m->destination) > 0) {
        msg->destination        = ds_cstr(&m->destination);
        msg->destinationSize    = ds_size(&m->destination);
    }
    if (ds_size(&m->sender) > 0) {
        msg->sender             = ds_cstr(&m->sender);
        msg->senderSize         = ds_size(&m->sender);
    }

    if (m->hasReplySerial)
        msg->replySerial = &m->replySerial;

    return 0;
}

// ----------------------------------------------------------------------------

/** Builds and then sends a message.
 *  \relates adbus_MsgFactory
 */
int adbus_msg_send(adbus_MsgFactory* m, adbus_Connection* c)
{
    adbus_Message msg;
    ZERO(&msg);
    if (adbus_msg_serial(m) < 0) {
        adbus_msg_setserial(m, adbus_conn_serial(c));
    }

    if (adbus_msg_build(m, &msg))
        return -1;

    return adbus_conn_send(c, &msg);
}

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

/** Returns the current value of the sender header field.
 *
 *  \relates adbus_MsgFactory
 *
 *  \return NULL if the field is not set
 */
const char* adbus_msg_sender(const adbus_MsgFactory* m, size_t* len)
{
    if (len)
        *len = ds_size(&m->sender);
    if (ds_size(&m->sender) > 0)
        return ds_cstr(&m->sender);
    else
        return NULL;
}

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

/** Returns the current value of the message type field.
 *  \relates adbus_MsgFactory
 */
adbus_MessageType adbus_msg_type(const adbus_MsgFactory* m)
{
    return m->messageType;
}

// ----------------------------------------------------------------------------

/** Returns the current value of the message type field.
 *  \relates adbus_MsgFactory
 */
int adbus_msg_flags(const adbus_MsgFactory* m)
{
    return m->flags;
}

// ----------------------------------------------------------------------------

/** Returns the current value of the serial field.
 *  \relates adbus_MsgFactory
 */
int64_t adbus_msg_serial(const adbus_MsgFactory* m)
{
    return m->serial;
}

// ----------------------------------------------------------------------------

/** Returns whether the message has a reply serial and the reply serial itself.
 *
 *  \relates adbus_MsgFactory
 *
 *  \return 1 if the message has a reply serial (and then serial will be set
 *  to the reply serial)
 */
adbus_Bool adbus_msg_reply(const adbus_MsgFactory* m, uint32_t* serial)
{
    if (serial)
        *serial = m->replySerial;
    return m->hasReplySerial;
}

// ----------------------------------------------------------------------------
// Setter functions
// ----------------------------------------------------------------------------

/** Sets the message type 
 *  \relates adbus_MsgFactory
 */
void adbus_msg_settype(adbus_MsgFactory* m, adbus_MessageType type)
{
    m->messageType = type;
}

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

/** Sets the message flags
 *  \relates adbus_MsgFactory
 */
void adbus_msg_setflags(adbus_MsgFactory* m, int flags)
{
    m->flags = flags;
}

// ----------------------------------------------------------------------------

/** Sets the message reply serial
 *  \relates adbus_MsgFactory
 */
void adbus_msg_setreply(adbus_MsgFactory* m, uint32_t reply)
{
    m->replySerial = reply;
    m->hasReplySerial = 1;
}

// ----------------------------------------------------------------------------

/** Sets the message object path field
 *  \relates adbus_MsgFactory
 */
void adbus_msg_setpath(adbus_MsgFactory* m, const char* path, int size)
{
    if (size < 0)
        size = strlen(path);
    ds_set_n(&m->path, path, size);
}

// ----------------------------------------------------------------------------

/** Sets the message interface field
 *  \relates adbus_MsgFactory
 */
void adbus_msg_setinterface(adbus_MsgFactory* m, const char* interface, int size)
{
    if (size < 0)
        size = strlen(interface);
    ds_set_n(&m->interface, interface, size);
}

// ----------------------------------------------------------------------------

/** Sets the message member field
 *  \relates adbus_MsgFactory
 */
void adbus_msg_setmember(adbus_MsgFactory* m, const char* member, int size)
{
    if (size < 0)
        size = strlen(member);
    ds_set_n(&m->member, member, size);
}

// ----------------------------------------------------------------------------

/** Sets the message error name field
 *  \relates adbus_MsgFactory
 */
void adbus_msg_seterror(adbus_MsgFactory* m, const char* error, int size)
{
    if (size < 0)
        size = strlen(error);
    ds_set_n(&m->error, error, size);
}

// ----------------------------------------------------------------------------

/** Sets the message destination field
 *  \relates adbus_MsgFactory
 */
void adbus_msg_setdestination(adbus_MsgFactory* m, const char* destination, int size)
{
    if (size < 0)
        size = strlen(destination);
    ds_set_n(&m->destination, destination, size);
}

// ----------------------------------------------------------------------------

/** Sets the message sender field
 *  \relates adbus_MsgFactory
 */
void adbus_msg_setsender(adbus_MsgFactory* m, const char* sender, int size)
{
    if (size < 0)
        size = strlen(sender);
    ds_set_n(&m->sender, sender, size);
}

// ----------------------------------------------------------------------------

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



