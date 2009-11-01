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
#include "connection.h"
#include "interface.h"
#include "misc.h"

#include "memory/kvector.h"
#include "memory/kstring.h"

#include <assert.h>


// ----------------------------------------------------------------------------
// Connection management
// ----------------------------------------------------------------------------

adbus_Connection* adbus_conn_new()
{
    adbus_Connection* c = NEW(adbus_Connection);

    c->sendCallbackData    = NULL;
    c->connectCallbackData = NULL;
    c->nextSerial   = 1;
    c->nextMatchId  = 1;
    c->connected    = 0;

    c->returnMessage    = adbus_msg_new();
    c->dispatchIterator = adbus_iter_new();

    c->parseStream  = adbus_stream_new();
    c->parseMessage = adbus_msg_new();

    c->bus = adbus_proxy_new(
            c,
            "org.freedesktop.DBus", -1,
            "/org/freedesktop/DBus", -1,
            "org.freedesktop.DBus", -1);

    c->objects       = kh_new(ObjectPtr);
    c->services  = kh_new(Service);
    c->registrations = kv_new(Match);

    adbus_Member* m;

    c->introspectable = adbus_iface_new("org.freedesktop.DBus.Introspectable", -1);

    m = adbus_iface_addmethod(c->introspectable, "Introspect", -1);
    adbus_mbr_addreturn(m, "xml_data", -1, "s", -1);
    adbus_mbr_setmethod(m, &adbusI_introspect, NULL);


    c->properties = adbus_iface_new("org.freedesktop.DBus.Properties", -1);

    m = adbus_iface_addmethod(c->properties, "Get", -1);
    adbus_mbr_addargument(m, "interface_name", -1, "s", -1);
    adbus_mbr_addargument(m, "property_name", -1, "s", -1);
    adbus_mbr_addreturn(m, "value", -1, "v", -1);
    adbus_mbr_setmethod(m, &adbusI_getProperty, NULL);

    m = adbus_iface_addmethod(c->properties, "GetAll", -1);
    adbus_mbr_addargument(m, "interface_name", -1, "s", -1);
    adbus_mbr_addreturn(m, "props", -1, "a{sv}", -1);
    adbus_mbr_setmethod(m, &adbusI_getAllProperties, NULL);

    m = adbus_iface_addmethod(c->properties, "Set", -1);
    adbus_mbr_addargument(m, "interface_name", -1, "s", -1);
    adbus_mbr_addargument(m, "property_name", -1, "s", -1);
    adbus_mbr_addargument(m, "value", -1, "v", -1);
    adbus_mbr_setmethod(m, &adbusI_setProperty, NULL);

    return c;
}

// ----------------------------------------------------------------------------

void adbus_conn_free(adbus_Connection* c)
{
    if (c) {
        adbus_iface_free(c->introspectable);
        adbus_iface_free(c->properties);

        adbus_proxy_free(c->bus);

        adbus_msg_free(c->returnMessage);

        adbus_msg_free(c->parseMessage);
        adbus_stream_free(c->parseStream);

        adbus_iter_free(c->dispatchIterator);

        adbus_user_free(c->connectCallbackData);
        adbus_user_free(c->sendCallbackData);

        free(c->uniqueService);

        for (khiter_t ii = kh_begin(c->objects); ii != kh_end(c->objects); ++ii) {
            if (kh_exist(c->objects, ii)){
                adbusI_freeObjectPath(kh_val(c->objects,ii));
            }
        }
        kh_free(ObjectPtr, c->objects);

        for (khiter_t si = kh_begin(c->services); si != kh_end(c->services); ++si) {
            if (kh_exist(c->services, si)) {
                adbusI_freeService(kh_val(c->services, si));
            }
        }
        kh_free(Service, c->services);

        for (size_t i = 0; i < kv_size(c->registrations); ++i)
            adbusI_freeMatchData(&kv_a(c->registrations, i));
        kv_free(Match, c->registrations);

        free(c);
    }
}





// ----------------------------------------------------------------------------
// Message
// ----------------------------------------------------------------------------

void adbus_conn_setsender(
        adbus_Connection* c,
        adbus_SendCallback       callback,
        adbus_User*       user)
{
    adbus_user_free(c->sendCallbackData);
    c->sendCallback = callback;
    c->sendCallbackData = user;
}

// ----------------------------------------------------------------------------

void adbus_conn_send(
        adbus_Connection* c,
        adbus_Message*    message)
{
    adbus_msg_build(message);
    if (c->sendCallback)
        c->sendCallback(message, c->sendCallbackData);
}

// ----------------------------------------------------------------------------

uint32_t adbus_conn_serial(adbus_Connection* c)
{
    if (c->nextSerial == UINT32_MAX)
        c->nextSerial = 1;
    return c->nextSerial++;
}





// ----------------------------------------------------------------------------
// Parsing and dispatch
// ----------------------------------------------------------------------------

static int DispatchMethodCall(adbus_CbData* d)
{
    size_t psize, isize, msize;
    adbus_Message* message = d->message;
    adbus_Connection* c = d->connection;
    const char* pname = adbus_msg_path(message, &psize);
    const char* iname = adbus_msg_interface(message, &isize);
    const char* mname = adbus_msg_member(message, &msize);

    int err = setjmp(d->jmpbuf);
    if (err)
        return err;

    // should have been checked by parser
    assert(mname);
    assert(pname);

    // Find the object
    khiter_t oi = kh_get(ObjectPtr, c->objects, pname);
    if (oi == kh_end(c->objects)) {
        return adbusI_pathError(d);
    }

    adbus_Path* path = &kh_val(c->objects, oi)->h;

    // Find the member
    adbus_Member* member = NULL;
    if (iname) {
        // If we know the interface, then we try and find the method on that
        // interface
        adbus_Interface* interface = adbus_path_interface(
                path,
                iname,
                (int) isize,
                &d->user2);

        if (!interface) {
            return adbusI_interfaceError(d);
        }

        member = adbus_iface_method(interface, mname, (int) msize);

    } else {
        // We don't know the interface, try and find the first method on any
        // interface with the member name
        member = adbus_path_method(path, mname, (int) msize, &d->user2);
    }

    if (!member || !member->methodCallback) {
        return adbusI_methodError(d);
    }

    adbus_Callback callback = member->methodCallback;
    d->user1 = member->methodData;
    return callback(d);
}

// ----------------------------------------------------------------------------

static adbus_Bool Matches(const char* matchString, const char* messageString)
{
    if (!matchString || *matchString == '\0')
        return 1; // ignoring this field
    if (!messageString)
        return 0; // message doesn't have this field

    return strcmp(matchString, messageString) == 0;
}

static adbus_Bool ReplySerialMatches(struct Match* r, adbus_Message* message)
{
    uint32_t serial;
    if (r->m.replySerial < 0)
        return 1; // ignoring this field
    if (!adbus_msg_reply(message, &serial))
        return 0; // message doesn't have this field

    return r->m.replySerial == serial;
}

static int DispatchMatch(adbus_CbData* d)
{
    adbus_Connection* c         = d->connection;
    adbus_Message* m            = d->message;
    adbus_MessageType type      = adbus_msg_type(m);
    const char* sender          = adbus_msg_sender(m, NULL);
    const char* destination     = adbus_msg_destination(m, NULL);
    const char* path            = adbus_msg_path(m, NULL);
    const char* interface       = adbus_msg_interface(m, NULL);
    const char* member          = adbus_msg_member(m, NULL);
    const char* errorName       = adbus_msg_error(m, NULL);

    for (size_t i = 0; i < kv_size(c->registrations); ++i)
    {
        struct Match* r = &kv_a(c->registrations, i);
        if (r->service) {
            // If r->service->uniqueName is still null then we don't know
            // who owns the service name so we can't match anything
            if (r->service->uniqueName == NULL) {
                continue;
            } else if (!Matches(r->service->uniqueName, sender)) {
                continue;
            }
        } else if (!Matches(r->m.sender, sender)) {
            continue;
        }

        if (r->m.type && r->m.type != type) {
            continue;
        } else if (!ReplySerialMatches(r, m)) {
            continue;
        } else if (!Matches(r->m.destination, destination)) {
            continue;
        } else if (!Matches(r->m.path, path)) {
            continue;
        } else if (!Matches(r->m.interface, interface)) {
            continue;
        } else if (!Matches(r->m.member, member)) {
            continue;
        } else if (!Matches(r->m.errorName, errorName)) {
            continue;
        } else {
            if (r->m.callback) {
                // We want to reset the argument iterator for every match
                adbus_msg_iterator(m, d->args);

                d->user1 = r->m.user1;
                d->user2 = r->m.user2;

                int err = setjmp(d->jmpbuf);
                if (err && err != ADBUSI_PARSE_ERROR)
                    return err;

                err = r->m.callback(d);
                if (err)
                    return err;
            }

            if (r->m.removeOnFirstMatch) {
                adbusI_freeMatchData(r);
                kv_remove(Match, c->registrations, i, 1);
                --i;
            }
        }
    }
    return 0;
}

// ----------------------------------------------------------------------------

static void SetupReturn(
        adbus_CbData*    d,
        adbus_Message*        retmessage)
{
    size_t destsize;
    const char* dest = adbus_msg_sender(d->message, &destsize);
    uint32_t reply = adbus_msg_serial(d->message);

    d->retmessage = retmessage;
    d->retargs = adbus_msg_buffer(retmessage);

    adbus_msg_reset(retmessage);
    adbus_msg_settype(retmessage, ADBUS_MSG_RETURN);
    adbus_msg_setflags(retmessage, ADBUS_MSG_NO_REPLY);
    adbus_msg_setserial(retmessage, adbus_conn_serial(d->connection));

    adbus_msg_setreply(retmessage, reply);
    if (dest) {
        adbus_msg_setdestination(retmessage, dest, destsize);
    }
}

// ----------------------------------------------------------------------------

int adbus_conn_rawdispatch(adbus_CbData* d)
{
    size_t sz;
    adbus_msg_data(d->message, &sz);
    if (sz == 0)
        return 0;

    // Reset the returnMessage field for the dispatch match so that matches 
    // don't try and use it
    adbus_Message* retmessage = d->retmessage;
    adbus_Buffer* retargs = d->retargs;

    d->retmessage = NULL;
    d->retargs = NULL;

    // Dispatch match
    adbus_msg_iterator(d->message, d->args);
    int err = DispatchMatch(d);
    if (err)
        goto end;

    // Dispatch the method call
    if (adbus_msg_type(d->message) == ADBUS_MSG_METHOD)
    {
        adbus_msg_iterator(d->message, d->args);
        SetupReturn(d, retmessage);
        err = DispatchMethodCall(d);
    }


end:
    d->retmessage = retmessage;
    d->retargs = retargs;
    return (err == ADBUSI_PARSE_ERROR) ? 0 : err;
}

// ----------------------------------------------------------------------------

int adbus_conn_dispatch(
        adbus_Connection* c,
        adbus_Message*    message)
{
    size_t sz;
    adbus_msg_data(message, &sz);
    if (sz == 0)
        return 0;

    adbus_CbData d;
    memset(&d, 0, sizeof(adbus_CbData));
    d.connection    = c;
    d.message       = message;
    d.args          = c->dispatchIterator;
    d.retmessage    = c->returnMessage;

    int err = adbus_conn_rawdispatch(&d);
      
    // Send off reply if needed
    if (!err
        && !d.manualReply
        && adbus_msg_type(message) == ADBUS_MSG_METHOD
        && !(adbus_msg_flags(message) & ADBUS_MSG_NO_REPLY))
    {
        adbus_conn_send(c, c->returnMessage);
    }

    return err;
}

// ----------------------------------------------------------------------------

int adbus_conn_parse(
        adbus_Connection*   c,
        const uint8_t*      data,
        size_t              size)
{
    adbus_Message* m = c->parseMessage;
    adbus_Stream* s = c->parseStream;
    while (size > 0) {
        if (adbus_stream_parse(s, m, &data, &size))
            return -1;
        if (adbus_conn_dispatch(c, m))
            return -1;
    }

    return 0;
}


