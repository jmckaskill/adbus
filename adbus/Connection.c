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

#include "Connection.h"
#include "Connection_p.h"

#include "CommonMessages.h"
#include "Factory.h"
#include "Interface_p.h"
#include "Iterator.h"
#include "Message.h"
#include "Misc_p.h"
#include "Message_p.h"
#include "Proxy.h"
#include "memory/kvector.h"
#include "memory/kstring.h"

#include <assert.h>


// ----------------------------------------------------------------------------
// Connection management
// ----------------------------------------------------------------------------

struct ADBusConnection* ADBusCreateConnection()
{
    struct ADBusConnection* c = NEW(struct ADBusConnection);

    c->sendCallbackData    = NULL;
    c->connectCallbackData = NULL;
    c->nextSerial   = 1;
    c->nextMatchId  = 1;
    c->connected    = 0;

    c->returnMessage    = ADBusCreateMessage();
    c->dispatchIterator = ADBusCreateIterator();

    c->bus = ADBusCreateProxy(
            c,
            "org.freedesktop.DBus", -1,
            "/org/freedesktop/DBus", -1,
            "org.freedesktop.DBus", -1);

    c->objects       = kh_new(ObjectPtr);
    c->services  = kh_new(Service);
    c->registrations = kv_new(Match);

    struct ADBusMember* m;

    c->introspectable = ADBusCreateInterface("org.freedesktop.DBus.Introspectable", -1);

    m = ADBusAddMember(c->introspectable, ADBusMethodMember, "Introspect", -1);
    ADBusAddArgument(m, ADBusOutArgument, "xml_data", -1, "s", -1);
    ADBusSetMethodCallback(m, &IntrospectCallback, NULL);


    c->properties = ADBusCreateInterface("org.freedesktop.DBus.Properties", -1);

    m = ADBusAddMember(c->properties, ADBusMethodMember, "Get", -1);
    ADBusAddArgument(m, ADBusInArgument, "interface_name", -1, "s", -1);
    ADBusAddArgument(m, ADBusInArgument, "property_name", -1, "s", -1);
    ADBusAddArgument(m, ADBusOutArgument, "value", -1, "v", -1);
    ADBusSetMethodCallback(m, &GetPropertyCallback, NULL);

    m = ADBusAddMember(c->properties, ADBusMethodMember, "GetAll", -1);
    ADBusAddArgument(m, ADBusInArgument, "interface_name", -1, "s", -1);
    ADBusAddArgument(m, ADBusOutArgument, "props", -1, "a{sv}", -1);
    ADBusSetMethodCallback(m, &GetAllPropertiesCallback, NULL);

    m = ADBusAddMember(c->properties, ADBusMethodMember, "Set", -1);
    ADBusAddArgument(m, ADBusInArgument, "interface_name", -1, "s", -1);
    ADBusAddArgument(m, ADBusInArgument, "property_name", -1, "s", -1);
    ADBusAddArgument(m, ADBusInArgument, "value", -1, "v", -1);
    ADBusSetMethodCallback(m, &SetPropertyCallback, NULL);

    return c;
}

// ----------------------------------------------------------------------------

void ADBusFreeConnection(struct ADBusConnection* c)
{
    if (c) {
        ADBusFreeInterface(c->introspectable);
        ADBusFreeInterface(c->properties);

        ADBusFreeProxy(c->bus);

        ADBusFreeMessage(c->returnMessage);

        ADBusFreeIterator(c->dispatchIterator);

        ADBusUserFree(c->connectCallbackData);
        ADBusUserFree(c->sendCallbackData);

        free(c->uniqueService);

        for (khiter_t ii = kh_begin(c->objects); ii != kh_end(c->objects); ++ii) {
            if (kh_exist(c->objects, ii)){
                FreeObjectPath(kh_val(c->objects,ii));
            }
        }
        kh_free(ObjectPtr, c->objects);

        for (khiter_t si = kh_begin(c->services); si != kh_end(c->services); ++si) {
            if (kh_exist(c->services, si)) {
                FreeService(kh_val(c->services, si));
            }
        }
        kh_free(Service, c->services);

        for (size_t i = 0; i < kv_size(c->registrations); ++i)
            FreeMatchData(&kv_a(c->registrations, i));
        kv_free(Match, c->registrations);

        free(c);
    }
}





// ----------------------------------------------------------------------------
// Message
// ----------------------------------------------------------------------------

void ADBusSetSendCallback(
        struct ADBusConnection* c,
        ADBusSendCallback       callback,
        struct ADBusUser*       user)
{
    ADBusUserFree(c->sendCallbackData);
    c->sendCallback = callback;
    c->sendCallbackData = user;
}

// ----------------------------------------------------------------------------

void ADBusSendMessage(
        struct ADBusConnection* c,
        struct ADBusMessage*    message)
{
    BuildMessage(message);
    if (c->sendCallback)
        c->sendCallback(message, c->sendCallbackData);
}

// ----------------------------------------------------------------------------

uint32_t ADBusNextSerial(struct ADBusConnection* c)
{
    if (c->nextSerial == UINT32_MAX)
        c->nextSerial = 1;
    return c->nextSerial++;
}





// ----------------------------------------------------------------------------
// Parsing and dispatch
// ----------------------------------------------------------------------------

static int DispatchMethodCall(struct ADBusCallDetails* d)
{
    size_t psize, isize, msize;
    struct ADBusMessage* message = d->message;
    struct ADBusConnection* c = d->connection;
    const char* pname = ADBusGetPath(message, &psize);
    const char* iname = ADBusGetInterface(message, &isize);
    const char* mname = ADBusGetMember(message, &msize);

    int err = setjmp(d->jmpbuf);
    if (err)
        return err;

    // should have been checked by parser
    assert(mname);
    assert(pname);

    // Find the object
    khiter_t oi = kh_get(ObjectPtr, c->objects, pname);
    if (oi == kh_end(c->objects)) {
        return InvalidPathError(d);
    }

    struct ADBusObjectPath* path = &kh_val(c->objects, oi)->h;

    // Find the member
    struct ADBusMember* member = NULL;
    if (iname) {
        // If we know the interface, then we try and find the method on that
        // interface
        struct ADBusInterface* interface = ADBusGetBoundInterface(
                path,
                iname,
                (int) isize,
                &d->user2);

        if (!interface) {
            return InvalidInterfaceError(d);
        }

        member = ADBusGetInterfaceMember(
                interface,
                ADBusMethodMember,
                mname,
                (int) msize);

    } else {
        // We don't know the interface, try and find the first method on any
        // interface with the member name
        member = ADBusGetBoundMember(
                path,
                ADBusMethodMember,
                mname,
                (int) msize,
                &d->user2);
    }

    if (!member || !member->methodCallback) {
        return InvalidMethodError(d);
    }

    ADBusMessageCallback callback = member->methodCallback;
    d->user1 = member->methodData;
    return callback(d);
}

// ----------------------------------------------------------------------------

static uint Matches(const char* matchString, const char* messageString)
{
    if (!matchString || *matchString == '\0')
        return 1; // ignoring this field
    if (!messageString)
        return 0; // message doesn't have this field

    return strcmp(matchString, messageString) == 0;
}

static uint ReplySerialMatches(struct Match* r, struct ADBusMessage* message)
{
    if (r->m.replySerial < 0)
        return 1; // ignoring this field
    if (!ADBusHasReplySerial(message))
        return 0; // message doesn't have this field

    return r->m.replySerial == ADBusGetReplySerial(message);
}

static int DispatchMatch(struct ADBusCallDetails* d)
{
    struct ADBusConnection* c   = d->connection;
    struct ADBusMessage* m      = d->message;
    enum ADBusMessageType type  = ADBusGetMessageType(m);
    const char* sender          = ADBusGetSender(m, NULL);
    const char* destination     = ADBusGetDestination(m, NULL);
    const char* path            = ADBusGetPath(m, NULL);
    const char* interface       = ADBusGetInterface(m, NULL);
    const char* member          = ADBusGetMember(m, NULL);
    const char* errorName       = ADBusGetErrorName(m, NULL);

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
                ADBusArgumentIterator(m, d->args);

                d->user1 = r->m.user1;
                d->user2 = r->m.user2;

                int err = setjmp(d->jmpbuf);
                if (err && err != ADBusErrorJmp)
                    return err;

                err = r->m.callback(d);
                if (err)
                    return err;
            }

            if (r->m.removeOnFirstMatch) {
                FreeMatchData(r);
                kv_remove(Match, c->registrations, i, 1);
                --i;
            }
        }
    }
    return 0;
}

// ----------------------------------------------------------------------------

static void SetupReturn(
        struct ADBusCallDetails*    d,
        struct ADBusMessage*        retmessage)
{
    size_t destsize;
    const char* dest = ADBusGetSender(d->message, &destsize);
    uint32_t reply = ADBusGetSerial(d->message);

    d->retmessage = retmessage;
    d->retargs = ADBusArgumentMarshaller(retmessage);

    ADBusResetMessage(retmessage);
    ADBusSetMessageType(retmessage, ADBusMethodReturnMessage);
    ADBusSetFlags(retmessage, ADBusNoReplyExpectedFlag);
    ADBusSetSerial(retmessage, ADBusNextSerial(d->connection));

    ADBusSetReplySerial(retmessage, reply);
    if (dest) {
        ADBusSetDestination(retmessage, dest, destsize);
    }
}

// ----------------------------------------------------------------------------

int ADBusRawDispatch(struct ADBusCallDetails* d)
{
    size_t sz;
    ADBusGetMessageData(d->message, NULL, &sz);
    if (sz == 0)
        return 0;

    // Reset the returnMessage field for the dispatch match so that matches 
    // don't try and use it
    struct ADBusMessage* retmessage = d->retmessage;
    struct ADBusMarshaller* retargs = d->retargs;

    d->retmessage = NULL;
    d->retargs = NULL;

    // Dispatch match
    ADBusArgumentIterator(d->message, d->args);
    int err = DispatchMatch(d);
    if (err)
        goto end;

    // Dispatch the method call
    if (ADBusGetMessageType(d->message) == ADBusMethodCallMessage)
    {
        ADBusArgumentIterator(d->message, d->args);
        SetupReturn(d, retmessage);
        err = DispatchMethodCall(d);
    }


end:
    d->retmessage = retmessage;
    d->retargs = retargs;
    return (err == ADBusErrorJmp) ? 0 : err;
}

// ----------------------------------------------------------------------------

int ADBusDispatch(
        struct ADBusConnection* c,
        struct ADBusMessage*    message)
{
    size_t sz;
    ADBusGetMessageData(message, NULL, &sz);
    if (sz == 0)
        return 0;

    struct ADBusCallDetails d;
    ADBusInitCallDetails(&d);
    d.connection    = c;
    d.message       = message;
    d.args          = c->dispatchIterator;
    d.retmessage    = c->returnMessage;

    int err = ADBusRawDispatch(&d);
      
    // Send off reply if needed
    if (!err
        && !d.manualReply
        && ADBusGetMessageType(message) == ADBusMethodCallMessage
        && !(ADBusGetFlags(message) & ADBusNoReplyExpectedFlag))
    {
        ADBusSendMessage(c, c->returnMessage);
    }

    return err;
}




