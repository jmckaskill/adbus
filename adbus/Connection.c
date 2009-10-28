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
#include "Interface_p.h"
#include "Iterator.h"
#include "Message.h"
#include "Misc_p.h"
#include "Message_p.h"
#include "memory/kvector.h"
#include "memory/kstring.h"

#include <assert.h>


// ----------------------------------------------------------------------------
// Errors (private)
// ----------------------------------------------------------------------------

static void ThrowInvalidArgument(struct ADBusCallDetails* details)
{
    ADBusSetupError(details,
            "nz.co.foobar.ADBus.Error.InvalidArgument", -1,
            "Invalid arguments passed to a method call.", -1);
    longjmp(details->jmpbuf, 0);
}

// ----------------------------------------------------------------------------

static void ThrowInvalidPath(struct ADBusCallDetails* details)
{
    ADBusSetupError(details,
            "nz.co.foobar.ADBus.Error.ObjectNotFound", -1,
            "No object exists for the requested path.", -1);
    longjmp(details->jmpbuf, 0);
}

// ----------------------------------------------------------------------------

static void ThrowInvalidInterface(struct ADBusCallDetails* details)
{
    ADBusSetupError(details,
            "nz.co.foobar.ADBus.Error.InterfaceNotFound", -1,
            "The requested path does not export the requested interface.", -1);
    longjmp(details->jmpbuf, 0);
}

// ----------------------------------------------------------------------------

static void ThrowInvalidMethod(struct ADBusCallDetails* details)
{
    ADBusSetupError(details,
            "nz.co.foobar.ADBus.Error.MethodNotFound", -1,
            "Method name you invoked isn't known by the object you invoked it on.", -1);
    longjmp(details->jmpbuf, 0);
}

// ----------------------------------------------------------------------------

static void ThrowInvalidProperty(struct ADBusCallDetails* details)
{
    ADBusSetupError(details,
            "nz.co.foobar.ADBus.Error.PropertyNotFound", -1,
            "The requested object and interface do not export the requested property.", -1);
    longjmp(details->jmpbuf, 0);
}

// ----------------------------------------------------------------------------

static void ThrowReadOnlyProperty(struct ADBusCallDetails* details)
{
    ADBusSetupError(details,
            "nz.co.foobar.ADBus.Error.ReadOnlyProperty", -1,
            "The requested property is read only.", -1);
    longjmp(details->jmpbuf, 0);
}

// ----------------------------------------------------------------------------

static void ThrowWriteOnlyProperty(struct ADBusCallDetails* details)
{
    ADBusSetupError(details,
            "nz.co.foobar.ADBus.Error.WriteOnlyProperty", -1,
            "The requested property is write only.", -1);
    longjmp(details->jmpbuf, 0);
}




// ----------------------------------------------------------------------------
// UserPointer (used for local callbacks)
// ----------------------------------------------------------------------------

struct UserPointer
{
    struct ADBusUser header;
    void* pointer;
};

static void FreeUserPointer(struct ADBusUser* data)
{
    free(data);
}

struct ADBusUser* CreateUserPointer(void* p)
{
    struct UserPointer* u = NEW(struct UserPointer);
    u->header.free = &FreeUserPointer;
    u->pointer = p;
    return &u->header;
}



// ----------------------------------------------------------------------------
// Connection management
// ----------------------------------------------------------------------------

static void IntrospectCallback(struct ADBusCallDetails* details);
static void GetPropertyCallback(struct ADBusCallDetails* details);
static void GetAllPropertiesCallback(struct ADBusCallDetails* details);
static void SetPropertyCallback(struct ADBusCallDetails* details);

struct ADBusConnection* ADBusCreateConnection()
{
    struct ADBusConnection* c = NEW(struct ADBusConnection);

    c->sendCallbackData    = NULL;
    c->connectCallbackData = NULL;
    c->nextSerial   = 1;
    c->nextMatchId  = 1;
    c->connected    = 0;

    c->returnMessage   = ADBusCreateMessage();
    c->outgoingMessage = ADBusCreateMessage();

    c->dispatchIterator = ADBusCreateIterator();

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

static void FreeMatchData(struct Match* match);
static void FreeObject(struct ADBusObject* object);
static void FreeService(struct Service* match);

void ADBusFreeConnection(struct ADBusConnection* c)
{
    if (!c)
        return;

    ADBusFreeInterface(c->introspectable);
    ADBusFreeInterface(c->properties);

    ADBusFreeMessage(c->returnMessage);
    ADBusFreeMessage(c->outgoingMessage);

    ADBusFreeIterator(c->dispatchIterator);

    ADBusUserFree(c->connectCallbackData);
    ADBusUserFree(c->sendCallbackData);

    free(c->uniqueService);

    for (khiter_t ii = kh_begin(c->objects); ii != kh_end(c->objects); ++ii) {
        if (kh_exist(c->objects, ii)){
            FreeObject(kh_val(c->objects,ii));
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

void ADBusSetSendMessageCallback(
        struct ADBusConnection* c,
        ADBusMessageCallback    callback,
        struct ADBusUser*       user)
{
    ADBusUserFree(c->sendMessageData);
    c->sendMessageCallback = callback;
    c->sendMessageData = user;
}

// ----------------------------------------------------------------------------

void ADBusSetReceiveMessageCallback(
        struct ADBusConnection* c,
        ADBusMessageCallback    callback,
        struct ADBusUser*       user)
{
    ADBusUserFree(c->receiveMessageData);
    c->receiveMessageCallback = callback;
    c->receiveMessageData = user;
}

// ----------------------------------------------------------------------------

void ADBusSendMessage(
        struct ADBusConnection* c,
        struct ADBusMessage*    message)
{
    ADBusBuildMessage_(message);
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

static void DispatchMethodCall(
        struct ADBusCallDetails*    details)
{
    size_t pathSize, interfaceSize, memberSize;
    struct ADBusMessage* message = details->message;
    struct ADBusConnection* c = details->connection;
    const char* path = ADBusGetPath(message, &pathSize);
    const char* interfaceName = ADBusGetInterface(message, &interfaceSize);
    const char* memberName = ADBusGetMember(message, &memberSize);

    // should have been checked by parser
    assert(memberName);
    assert(path);

    // Find the object
    khiter_t oi = kh_get(ObjectPtr, c->objects, path);
    if (oi == kh_end(c->objects))
        ThrowInvalidPath(details);

    struct ADBusObject* object = kh_val(c->objects, oi);

    // Find the member
    struct ADBusMember* member = NULL;
    if (interfaceName) {
        // If we know the interface, then we try and find the method on that
        // interface
        struct ADBusInterface* interface = ADBusGetBoundInterface(
                object,
                interfaceName,
                interfaceSize,
                &details->user2);

        if (!interface)
            ThrowInvalidInterface(details);

        member = ADBusGetInterfaceMember(
                interface,
                ADBusMethodMember,
                memberName,
                memberSize);

    } else {
        // We don't know the interface, try and find the first method on any
        // interface with the member name
        member = ADBusGetBoundMember(
                object,
                ADBusMethodMember,
                memberName,
                memberSize,
                &details->user2);

    }

    if (!member)
        ThrowInvalidMethod(details);

    ADBusCallMethod(member, details);

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
    if (!r->m.checkReplySerial)
        return 1; // ignoring this field
    if (!ADBusHasReplySerial(message))
        return 0; // message doesn't have this field

    return r->m.replySerial == ADBusGetReplySerial(message);
}

static void DispatchMatch(
        struct ADBusCallDetails*  details)
{
    struct ADBusConnection* c   = details->connection;
    struct ADBusMessage* m      = details->message;
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
                ADBusArgumentIterator(m, details->arguments);

                details->user1 = r->m.user1;
                details->user2 = r->m.user2;

                // Ignore errors from match callbacks
                if (!setjmp(details->jmpbuf))
                  r->m.callback(details);
            }

            if (r->m.removeOnFirstMatch) {
                FreeMatchData(r);
                kv_remove(Match, c->registrations, i, 1);
                --i;
            }
        }
    }
    return;
}

// ----------------------------------------------------------------------------

void ADBusRawDispatch(struct ADBusCallDetails*    details)
{
    // Reset the returnMessage field for the dispatch match so that matches 
    // don't try and use it
    struct ADBusMessage* returnMessage = details->returnMessage;

    size_t sz;
    ADBusGetMessageData(details->message, NULL, &sz);
    if (sz == 0)
        goto end;


    // Dispatch the method call
    if (ADBusGetMessageType(details->message) == ADBusMethodCallMessage) {
        ADBusArgumentIterator(details->message, details->arguments);
        ADBusSetupReturn(details->returnMessage, details->connection, details->message);
        if (!setjmp(details->jmpbuf))
            DispatchMethodCall(details);
    }

    if (details->parseError)
        goto end;

    // Dispatch match
    details->returnMessage = NULL;
    ADBusArgumentIterator(details->message, details->arguments);
    DispatchMatch(details);

end:
    details->returnMessage = returnMessage;
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

    ADBusArgumentIterator(message, c->dispatchIterator);

    struct ADBusCallDetails details;
    ADBusInitCallDetails(&details);
    details.connection  = c;
    details.message     = message;
    details.arguments   = c->dispatchIterator;


    // Set up returnMessage if source wants reply for a method call
    if ( ADBusGetMessageType(message) == ADBusMethodCallMessage
      && !(ADBusGetFlags(message) & ADBusNoReplyExpectedFlag)) 
    {
        details.returnMessage = c->returnMessage;
        ADBusSetupReturn(details.returnMessage, c, message);
    }

    ADBusRawDispatch(&details);
      
    // Send off reply if needed
    if (details.parseError) {
        return details.parseError;
    }

    if ( details.returnMessage != NULL
      && !details.manualReply)
    {
        ADBusSendMessage(c, details.returnMessage);
    }

    return 0;
}

// ----------------------------------------------------------------------------

struct ADBusStreamBuffer* ADBusCreateStreamBuffer()
{
    struct ADBusStreamBuffer* b = NEW(struct ADBusStreamBuffer);
    b->buf = kv_new(uint8_t);
    return b;
}

// ----------------------------------------------------------------------------

void ADBusFreeStreamBuffer(struct ADBusStreamBuffer* b)
{
    if (b) {
        kv_free(uint8_t, b->buf);
        free(b);
    }
}

// ----------------------------------------------------------------------------

static uint RequireDataInParseBuffer(
        struct ADBusStreamBuffer*   b,
        size_t                      needed,
        const uint8_t**             data,
        size_t*                     size)
{
    int toAdd = needed - kv_size(b->buf);
    if (toAdd > (int) *size) {
        // Don't have enough data
        uint8_t* dest = kv_push(uint8_t, b->buf, *size);
        memcpy(dest, *data, *size);
        *data += *size;
        *size = 0;
        return 0;

    } else if (toAdd > 0) {
        // Need to push data into the buffer, but we have enough
        uint8_t* dest = kv_push(uint8_t, b->buf, toAdd);
        memcpy(dest, *data, toAdd);
        *data += toAdd;
        *size -= toAdd;
        return 1;

    } else {
        // Buffer already has enough data
        return 1;
    }
}

int ADBusParse(
        struct ADBusStreamBuffer*   b,
        struct ADBusMessage*        message,
        const uint8_t**             data,
        size_t*                     size)
{
    ADBusResetMessage(message);
    if (kv_size(b->buf) > 0)
    {
        // Use up message from the stream buffer
        
        // We need to add enough so we can figure out how big the next message is
        if (!RequireDataInParseBuffer(b, sizeof(struct ADBusExtendedHeader_), data, size))
            return 0;

        // Now we add enough to the buffer to be able to parse the message
        size_t messageSize = ADBusNextMessageSize(&kv_a(b->buf, 0), kv_size(b->buf));
        assert(messageSize > 0);
        if (!RequireDataInParseBuffer(b, messageSize, data, size))
            return 0;

        int err = ADBusSetMessageData(message, &kv_a(b->buf, 0), messageSize);
        kv_remove(uint8_t, b->buf, 0, messageSize);
        return err;
    }
    else
    {
        size_t messageSize = ADBusNextMessageSize(*data, *size);
        if (messageSize == 0 || messageSize > *size)
        {
            // We don't have enough to parse the message
            uint8_t* dest = kv_push(uint8_t, b->buf, *size);
            memcpy(dest, *data, *size);
            return 0;
        }

        int err = ADBusSetMessageData(message, *data, messageSize);

        *data += messageSize;
        *size -= messageSize;
        return err;
    }
}





// ----------------------------------------------------------------------------
// Bus management
// ----------------------------------------------------------------------------

static void ConnectionCallback(
        struct ADBusCallDetails*    details)
{
    struct ADBusField field;
    ADBusIterate(details->arguments, &field);
    if (field.type != ADBusStringField)
        return;

    struct ADBusConnection* c = details->connection;
    free(c->uniqueService);
    c->uniqueService = strdup_(field.string);
    c->connected = 1;

    if (c->connectCallback)
        c->connectCallback(c, c->connectCallbackData);
}

void ADBusConnectToBus(
        struct ADBusConnection*     c,
        ADBusConnectionCallback     callback,
        struct ADBusUser*           callbackData)
{
    assert(!c->connected);
    ADBusUserFree(c->connectCallbackData);
    c->connectCallback = callback;
    c->connectCallbackData = callbackData;

    ADBusSetupHello(c->outgoingMessage, c);

    struct ADBusMatch match;
    ADBusInitMatch(&match);
    match.type                  = ADBusMethodReturnMessage;
    match.callback              = &ConnectionCallback;
    match.replySerial           = ADBusGetSerial(c->outgoingMessage);
    match.removeOnFirstMatch    = 1;
    ADBusAddMatch(c, &match);

    ADBusSendMessage(c, c->outgoingMessage);
}

// ----------------------------------------------------------------------------

uint ADBusIsConnectedToBus(const struct ADBusConnection* c)
{
    return c->connected;
}

// ----------------------------------------------------------------------------

const char*  ADBusGetUniqueServiceName(
        const struct ADBusConnection* c,
        size_t* size)
{
    if (size)
        *size = c->connected ? strlen(c->uniqueService) : 0;
    return c->connected ? c->uniqueService : NULL;
}

// ----------------------------------------------------------------------------

struct ServiceData
{
    struct ADBusUser        header;
    ADBusServiceCallback    callback;
};

static void FreeServiceData(struct ADBusUser* data)
{
    free(data);
}

static void ServiceCallback(
        struct ADBusCallDetails*        details)
{
    const struct ServiceData* data = (const struct ServiceData*) details->user1;

    struct ADBusField field;
    ADBusIterate(details->arguments, &field);
    if (field.type != ADBusUInt32Field)
        return;

    enum ADBusServiceCode retcode = (enum ADBusServiceCode) field.u32;
    data->callback(details->connection, details->user2, retcode);
}

// ----------------------------------------------------------------------------

static void AddServiceMatch(
        struct ADBusConnection*             c,
        ADBusServiceCallback                callback,
        struct ADBusUser*                   user)
{
    struct ServiceData* data = NEW(struct ServiceData);
    data->header.free = &FreeServiceData;
    data->callback    = callback;

    struct ADBusMatch match;
    ADBusInitMatch(&match);
    match.type               = ADBusMethodReturnMessage;
    match.callback           = &ServiceCallback;
    match.replySerial        = ADBusGetSerial(c->outgoingMessage);
    match.removeOnFirstMatch = 1;
    match.user1              = &data->header;
    match.user2              = user;
    ADBusAddMatch(c, &match);
}

void ADBusRequestServiceName(
        struct ADBusConnection*             c,
        const char*                         service,
        int                                 size,
        uint32_t                            flags,
        ADBusServiceCallback                callback,
        struct ADBusUser*                   user)
{
    ADBusSetupRequestServiceName(c->outgoingMessage, c, service, size, flags);

    if (callback)
        AddServiceMatch(c, callback, user);

    ADBusSendMessage(c, c->outgoingMessage);
}

// ----------------------------------------------------------------------------

void ADBusReleaseServiceName(
        struct ADBusConnection*             c,
        const char*                         service,
        int                                 size,
        ADBusServiceCallback                callback,
        struct ADBusUser*                   user)
{
    ADBusSetupReleaseServiceName(c->outgoingMessage, c, service, size);

    if (callback)
        AddServiceMatch(c, callback, user);

    ADBusSendMessage(c, c->outgoingMessage);
}








// ----------------------------------------------------------------------------
// Match registrations
// ----------------------------------------------------------------------------

static void CloneString(const char* from, int fsize, const char** to, int* tsize)
{
    if (from) {
        if (fsize < 0)
            fsize = strlen(from);
        *to = strndup_(from, fsize);
        *tsize = fsize;
    }
}

static void SanitisePath(kstring_t* out, const char* path, int size);

static void CloneMatch(const struct ADBusMatch* from,
                       struct Match* to)
{
    to->m.type                = from->type;
    to->m.addMatchToBusDaemon = from->addMatchToBusDaemon;
    to->m.removeOnFirstMatch  = from->removeOnFirstMatch;
    to->m.replySerial         = from->replySerial;
    to->m.checkReplySerial    = from->checkReplySerial;
    to->m.callback            = from->callback;
    to->m.user1               = from->user1;
    to->m.user2               = from->user2;
    to->m.id                  = from->id;

#define STRING(NAME) CloneString(from->NAME, from->NAME ## Size, &to->m.NAME, &to->m.NAME ## Size)
    STRING(sender);
    STRING(destination);
    STRING(interface);
    STRING(member);
    STRING(errorName);
    if (from->path) {
        kstring_t* sanitised = ks_new();
        SanitisePath(sanitised, from->path, from->pathSize);
        to->m.pathSize = ks_size(sanitised);
        to->m.path     = ks_release(sanitised);
        ks_free(sanitised);
    }
    to->m.arguments = NEW_ARRAY(struct ADBusMatchArgument, from->argumentsSize);
    to->m.argumentsSize = from->argumentsSize;
    for (size_t i = 0; i < from->argumentsSize; ++i) {
        to->m.arguments[i].number = from->arguments[i].number;
        STRING(arguments[i].value);
    }
#undef STRING
}

static void FreeMatchData(struct Match* m)
{
    if (m) {
        free((char*) m->m.sender);
        free((char*) m->m.destination);
        free((char*) m->m.interface);
        free((char*) m->m.member);
        free((char*) m->m.errorName);
        free((char*) m->m.path);
        ADBusUserFree(m->m.user1);
        ADBusUserFree(m->m.user2);
        for (size_t i = 0; i < m->m.argumentsSize; ++i) {
            free((char*) m->m.arguments[i].value);
        }
        free(m->m.arguments);
    }
}

static void FreeService(struct Service* s)
{
    if (s) {
        free(s->serviceName);
        free(s->uniqueName);
        free(s);
    }
}

// ----------------------------------------------------------------------------

static void GetNameOwner(struct ADBusCallDetails* details)
{
    struct UserPointer* u = (struct UserPointer*) details->user1;
    struct Service* d = (struct Service*) u->pointer;

    d->methodMatch = 0;

    const char* unique = ADBusCheckString(details, NULL);
    ADBusCheckMessageEnd(details);

    free(d->uniqueName);
    d->uniqueName = strdup_(unique);
}

static void NameOwnerChanged(struct ADBusCallDetails* details)
{
    struct UserPointer* u = (struct UserPointer*) details->user1;
    struct Service* d = (struct Service*) u->pointer;

    ADBusCheckString(details, NULL);
    ADBusCheckString(details, NULL);
    const char* to = ADBusCheckString(details, NULL);
    ADBusCheckMessageEnd(details);

    free(d->uniqueName);
    d->uniqueName = strdup_(to);
}

// ----------------------------------------------------------------------------

static struct Service* AddService(
        struct ADBusConnection* c,
        const char*             servname)
{
    struct Service* s;

    int added = 0;
    khiter_t si = kh_put(Service, c->services, servname, &added);
    if (!added) {
        s = kh_val(c->services, si);
        s->refCount++;
    } else {
        s = NEW(struct Service);
        s->refCount = 1;
        s->serviceName = strdup_(servname);

        kh_key(c->services, si) = s->serviceName;
        kh_val(c->services, si) = s;
    }
    return s;
}

static void AddServiceMatches(
        struct ADBusConnection* c,
        struct Service*         s)
{
    // Add the NameOwnerChanged match
    struct ADBusMatchArgument arg0;
    arg0.number = 0;
    arg0.value = s->serviceName;
    arg0.valueSize = -1;

    struct ADBusMatch match;
    ADBusInitMatch(&match);
    match.type = ADBusSignalMessage;
    match.addMatchToBusDaemon = 1;
    match.sender = "org.freedesktop.DBus";
    match.path = "/org/freedesktop/DBus";
    match.interface = "org.freedesktop.DBus";
    match.member = "NameOwnerChanged";
    match.senderSize = -1;
    match.pathSize = -1;
    match.interfaceSize = -1;
    match.memberSize = -1;
    match.arguments = &arg0;
    match.argumentsSize = 1;
    match.callback = &NameOwnerChanged;
    match.user1 = CreateUserPointer(s);

    s->signalMatch = ADBusAddMatch(c, &match);

    // Call GetNameOwner - do this after adding the NameOwnerChanged match to
    // avoid a race condition. Setup the match for the reply first
    ADBusInitMatch(&match);
    match.type = ADBusMethodReturnMessage;
    match.removeOnFirstMatch = 1;
    match.replySerial = ADBusNextSerial(c);
    match.checkReplySerial = 1;
    match.callback = &GetNameOwner;
    match.user1 = CreateUserPointer(s);

    s->methodMatch = ADBusAddMatch(c, &match);

    struct ADBusMessage* m = c->outgoingMessage;
    ADBusSetupMethodCall(m, c, match.replySerial);
    ADBusSetDestination(m, "org.freedesktop.DBus", -1);
    ADBusSetPath(m, "/org/freedesktop/DBus", -1);
    ADBusSetInterface(m, "org.freedesktop.DBus", -1);
    ADBusSetMember(m, "GetNameOwner", -1);
    struct ADBusMarshaller* mar = ADBusArgumentMarshaller(m);
    ADBusAppendArguments(mar, "s", -1);
    ADBusAppendString(mar, s->serviceName, -1);

    ADBusSendMessage(c, m);
}

static void RemoveService(struct ADBusConnection* c, struct Service* s)
{
    s->refCount--;
    if (s->refCount > 0)
        return;

    khiter_t si = kh_get(Service, c->services, s->serviceName);
    if (si != kh_end(c->services))
        kh_del(Service, c->services, si);

    if (s->methodMatch)
        ADBusRemoveMatch(c, s->methodMatch);
    if (s->signalMatch)
        ADBusRemoveMatch(c, s->signalMatch);
    FreeService(s);
}

// ----------------------------------------------------------------------------

uint32_t ADBusAddMatch(
        struct ADBusConnection*     c,
        const struct ADBusMatch*    reg)
{
    struct Match* match = kv_push(Match, c->registrations, 1);
    CloneMatch(reg, match);
    if (match->m.id == 0) {
        match->m.id = ADBusNextMatchId(c);
    }

    if (match->m.addMatchToBusDaemon) {
        ADBusSetupAddBusMatch(c->outgoingMessage, c, &match->m);
        ADBusSendMessage(c, c->outgoingMessage);
    }

    uint32_t id = match->m.id;

    // If the sender field is a service name we have to track the service name
    // assignments
    if (match->m.sender
     && ADBusRequiresServiceLookup_(match->m.sender, match->m.senderSize))
    {
        struct Service* s = AddService(c, match->m.sender);
        match->service = s;
        // WARNING: AddServiceMatch will call ADBusAddMatch which may cause
        // match to be a dangling pointer
        if (s->signalMatch == 0) {
            AddServiceMatches(c, s);
        }
    }

    return id;
}

// ----------------------------------------------------------------------------

void ADBusRemoveMatch(struct ADBusConnection* c, uint32_t id)
{
    for (size_t i = 0; i < kv_size(c->registrations); ++i) {
        if (kv_a(c->registrations, i).m.id == id) {
            struct Match* m = &kv_a(c->registrations, i);
            struct Service* s = m->service;
            FreeMatchData(m);
            kv_remove(Match, c->registrations, i, 1);

            // Remove the service after removing the match - as removing the
            // service will remove more matches
            if (s)
                RemoveService(c, s);
            return;
        }
    }

}

// ----------------------------------------------------------------------------

uint32_t ADBusNextMatchId(struct ADBusConnection* c)
{
    if (c->nextMatchId == UINT32_MAX)
        c->nextSerial = 1;
    return c->nextMatchId++;
}





// ----------------------------------------------------------------------------
// Object management
// ----------------------------------------------------------------------------

static void FreeObject(struct ADBusObject* o)
{
    if (o) {
        khash_t(Bind)* h = o->interfaces;
        for (khiter_t ii = kh_begin(h); ii != kh_end(h); ++ii) {
            if (kh_exist(h, ii)) {
                ADBusUserFree(kh_val(h, ii).user2);
            }
        }
        kh_free(Bind, h);
        kv_free(ObjectPtr, o->children);
        free(o->name);
        free(o);
    }
}

// ----------------------------------------------------------------------------

static void SanitisePath(
        kstring_t*      out,
        const char*     path,
        int             size)
{
    ks_clear(out);
    if (size < 0)
        size = strlen(path);

    // Make sure it starts with a /
    if (size == 0 || path[0] != '/')
        ks_cat(out, "/");
    if (size > 0)
        ks_cat_n(out, path, size);

    // Remove repeating slashes
    for (size_t i = 1; i < ks_size(out); ++i) {
        if (ks_a(out, i) == '/' && ks_a(out, i-1) == '/') {
            ks_remove(out, i, 1);
        }
    }

    // Remove trailing /
    if (ks_size(out) > 1 && ks_a(out, ks_size(out) - 1) == '/')
        ks_remove_end(out, 1);
}

static void ParentPath(
        char*           path,
        size_t          size,
        size_t*         parentSize)
{
    // Path should have already been sanitised so double /'s should not happen
#ifndef NDEBUG
    kstring_t* sanitised = ks_new();
    SanitisePath(sanitised, path, size);
    assert(path[size] == '\0');
    assert(ks_cmp(sanitised, path) == 0);
    ks_free(sanitised);
#endif

    --size;

    while (size > 1 && path[size] != '/') {
        --size;
    }

    path[size] = '\0';

    if (parentSize)
        *parentSize = size;
}

// ----------------------------------------------------------------------------

static struct ADBusObject* DoAddObject(
        struct ADBusConnection* c,
        char*                   path,
        size_t                  size)
{
    int added;
    assert(path[size] == '\0');
    khiter_t ki = kh_put(ObjectPtr, c->objects, path, &added);
    if (!added) {
        return kh_val(c->objects, ki);
    }

    struct ADBusObject* o = NEW(struct ADBusObject);
    o->name         = strdup_(path);
    o->connection   = c;
    o->interfaces   = kh_new(Bind);
    o->children     = kv_new(ObjectPtr);

    kh_key(c->objects, ki) = o->name;
    kh_val(c->objects, ki) = o;

    ADBusBindInterface(o, c->introspectable, CreateUserPointer(o));
    ADBusBindInterface(o, c->properties, CreateUserPointer(o));

    // Setup the parent-child links
    if (strcmp(path, "/") != 0) {
        size_t parentSize;
        ParentPath(path, size, &parentSize);
        o->parent = DoAddObject(c, path, parentSize);

        struct ADBusObject** pchild = kv_push(ObjectPtr, o->parent->children, 1);
        *pchild = o;
    }

    return o;
}

struct ADBusObject* ADBusAddObject(
        struct ADBusConnection* c,
        const char*             path,
        int                     size)
{
    kstring_t* name = ks_new();
    SanitisePath(name, path, size);
    struct ADBusObject* o = DoAddObject(c, ks_data(name), ks_size(name));
    ks_free(name);
    return o;
}


// ----------------------------------------------------------------------------

struct ADBusObject* ADBusGetObject(
        struct ADBusConnection* c,
        const char*             path,
        int                     size)
{
    struct ADBusObject* ret = NULL;

    kstring_t* name = ks_new();
    SanitisePath(name, path, size);
    khiter_t ki = kh_get(ObjectPtr, c->objects, ks_cstr(name));
    if (ki != kh_end(c->objects))
        ret = kh_val(c->objects, ki);

    ks_free(name);
    return ret;
}

// ----------------------------------------------------------------------------

int ADBusBindInterface(
        struct ADBusObject*     o,
        struct ADBusInterface*  interface,
        struct ADBusUser*       user2)
{
    int added;
    khiter_t bi = kh_put(Bind, o->interfaces, interface->name, &added);
    if (!added)
        return -1; // there was already an interface with that name

    kh_val(o->interfaces, bi).user2      = user2;
    kh_val(o->interfaces, bi).interface  = interface;
    kh_key(o->interfaces, bi)            = interface->name;

    return 0;
}

// ----------------------------------------------------------------------------

static void CheckRemoveObject(struct ADBusObject* o)
{
    // We have 2 builtin interfaces (introspectable and properties)
    // If these are the only two left and we have no children then we need
    // to prune this object
    if (kh_size(o->interfaces) > 2 || kv_size(o->children) > 0)
        return;

    // Remove it from its parent
    struct ADBusObject* parent = o->parent;
    for (size_t i = 0; i < kv_size(parent->children); ++i) {
        if (kv_a(parent->children, i) == o) {
            kv_remove(ObjectPtr, parent->children, i, 1);
            break;
        }
    }
    CheckRemoveObject(o->parent);

    // Remove it from the object lookup table
    struct ADBusConnection* c = o->connection;
    khiter_t oi = kh_get(ObjectPtr, c->objects, o->name);
    if (oi != kh_end(c->objects))
        kh_del(ObjectPtr, c->objects, oi);

    // Free the object
    FreeObject(o);
}

// ----------------------------------------------------------------------------

int ADBusUnbindInterface(
        struct ADBusObject*     o,
        struct ADBusInterface*  interface)
{
    khiter_t bi = kh_get(Bind, o->interfaces, interface->name);
    if (bi == kh_end(o->interfaces))
        return -1;
    if (interface != kh_val(o->interfaces, bi).interface)
        return -1;

    ADBusUserFree(kh_val(o->interfaces, bi).user2);
    kh_del(Bind, o->interfaces, bi);

    CheckRemoveObject(o);

    return 0;
}

// ----------------------------------------------------------------------------

struct ADBusInterface* ADBusGetBoundInterface(
        struct ADBusObject*         o,
        const char*                 interface,
        int                         interfaceSize,
        const struct ADBusUser**    user2)
{
    // The hash table lookup requires that the interface name is nul terminated
    // So if we cannot guarentee this (ie interfaceSize >= 0) then copy the
    // string out and free it after the lookup
    if (interfaceSize >= 0)
        interface = strndup_(interface, interfaceSize);

    khiter_t bi = kh_get(Bind, o->interfaces, interface);

    if (interfaceSize >= 0)
        free((char*) interface);

    if (bi == kh_end(o->interfaces))
        return NULL;

    if (user2)
        *user2 = kh_val(o->interfaces, bi).user2;

    return kh_val(o->interfaces, bi).interface;
}

// ----------------------------------------------------------------------------

struct ADBusMember* ADBusGetBoundMember(
        struct ADBusObject*         o,
        enum ADBusMemberType        type,
        const char*                 member,
        int                         memberSize,
        const struct ADBusUser**    user2)
{
    struct ADBusMember* m;

    for (khiter_t bi = kh_begin(o->interfaces); bi != kh_end(o->interfaces); ++bi) {
        if (kh_exist(o->interfaces, bi)) {
            struct Bind* b = &kh_val(o->interfaces, bi);
            m = ADBusGetInterfaceMember(b->interface, type, member, memberSize);
            if (user2)
                *user2 = b->user2;
            if (m)
                return m;
        }
    }

    if (user2)
        *user2 = NULL;
    return NULL;
}










// ----------------------------------------------------------------------------
// Introspection (private)
// ----------------------------------------------------------------------------

static void IntrospectArguments(
        struct ADBusMember*     m,
        kstring_t*               out)
{
    for (size_t i = 0; i < kv_size(m->inArguments); ++i)
    {
        struct Argument* a = &kv_a(m->inArguments, i);
        ks_cat(out, "\t\t\t<arg type=\"");
        ks_cat(out, a->type);
        if (a->name)
        {
            ks_cat(out, "\" name=\"");
            ks_cat(out, a->name);
        }
        ks_cat(out, "\" direction=\"in\"/>\n");
    }

    for (size_t i = 0; i < kv_size(m->outArguments); ++i)
    {
        struct Argument* a = &kv_a(m->outArguments, i);
        ks_cat(out, "\t\t\t<arg type=\"");
        ks_cat(out, a->type);
        if (a->name)
        {
            ks_cat(out, "\" name=\"");
            ks_cat(out, a->name);
        }
        ks_cat(out, "\" direction=\"out\"/>\n");
    }

}

// ----------------------------------------------------------------------------

static void IntrospectAnnotations(
        struct ADBusMember*     m,
        kstring_t*               out)
{
    for (khiter_t ai = kh_begin(m->annotations); ai != kh_end(m->annotations); ++ai) {
        if (kh_exist(m->annotations, ai)) {
            ks_cat(out, "\t\t\t<annotation name=\"");
            ks_cat(out, kh_key(m->annotations, ai));
            ks_cat(out, "\" value=\"");
            ks_cat(out, kh_val(m->annotations, ai));
            ks_cat(out, "\"/>\n");
        }
    }
}
// ----------------------------------------------------------------------------

static void IntrospectMember(
        struct ADBusMember*     m,
        kstring_t*               out)
{
    switch (m->type)
    {
        case ADBusPropertyMember:
            {
                ks_cat(out, "\t\t<property name=\"");
                ks_cat(out, m->name);
                ks_cat(out, "\" type=\"");
                ks_cat(out, m->propertyType);
                ks_cat(out, "\" access=\"");
                uint read  = ADBusIsPropertyReadable(m);
                uint write = ADBusIsPropertyWritable(m);
                if (read && write)
                    ks_cat(out, "readwrite");
                else if (read)
                    ks_cat(out, "read");
                else if (write)
                    ks_cat(out, "write");
                else
                    assert(0);

                if (kh_size(m->annotations) == 0)
                {
                    ks_cat(out, "\"/>\n");
                }
                else
                {
                    ks_cat(out, "\">\n");
                    IntrospectAnnotations(m, out);
                    ks_cat(out, "\t\t</property>\n");
                }
            }
            break;
        case ADBusMethodMember:
            {
                ks_cat(out, "\t\t<method name=\"");
                ks_cat(out, m->name);
                ks_cat(out, "\">\n");

                IntrospectAnnotations(m, out);
                IntrospectArguments(m, out);

                ks_cat(out, "\t\t</method>\n");
            }
            break;
        case ADBusSignalMember:
            {
                ks_cat(out, "\t\t<signal name=\"");
                ks_cat(out, m->name);
                ks_cat(out, "\">\n");

                IntrospectAnnotations(m, out);
                IntrospectArguments(m, out);

                ks_cat(out, "\t\t</signal>\n");
            }
            break;
        default:
            assert(0);
            break;
    }
}

// ----------------------------------------------------------------------------

static void IntrospectInterfaces(
        struct ADBusObject*     o,
        kstring_t*                  out)
{
    for (khiter_t bi = kh_begin(o->interfaces); bi != kh_end(o->interfaces); ++bi) {
        if (kh_exist(o->interfaces, bi)) {
            struct ADBusInterface* i = kh_val(o->interfaces, bi).interface;
            ks_cat(out, "\t<interface name=\"");
            ks_cat(out, i->name);
            ks_cat(out, "\">\n");

            for (khiter_t mi = kh_begin(i->members); mi != kh_end(i->members); ++mi) {
                if (kh_exist(i->members, mi)) {
                    IntrospectMember(kh_val(i->members, mi), out);
                }
            }

            ks_cat(out, "\t</interface>\n");
        }
    }
}

// ----------------------------------------------------------------------------

static void IntrospectNode(
        struct ADBusObject*     o,
        kstring_t*               out)
{
    ks_cat(
            out,
            "<!DOCTYPE node PUBLIC \"-//freedesktop/DTD D-BUS Object Introspection 1.0//EN\"\n"
            "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
            "<node>\n");

    IntrospectInterfaces(o, out);

    size_t namelen = strlen(o->name);
    // Now add the child objects
    for (size_t i = 0; i < kv_size(o->children); ++i) {
        // Find the child tail ie ("bar" for "/foo/bar" or "foo" for "/foo")
        const char* child = kv_a(o->children, i)->name;
        child += namelen;
        if (namelen > 1)
            child += 1; // +1 for '/' when o is not the root node

        ks_cat(out, "\t<node name=\"");
        ks_cat(out, child);
        ks_cat(out, "\"/>\n");
    }

    ks_cat(out, "</node>\n");
}

// ----------------------------------------------------------------------------

static void IntrospectCallback(
        struct ADBusCallDetails*    details)
{
    ADBusCheckMessageEnd(details);

    // If no reply is wanted, we're done
    if (!details->returnMessage) {
        return;
    }

    struct UserPointer* p = (struct UserPointer*) details->user2;
    struct ADBusObject* o = (struct ADBusObject*) p->pointer;

    kstring_t* out = ks_new();
    IntrospectNode(o, out);

    struct ADBusMarshaller* m = ADBusArgumentMarshaller(details->returnMessage);
    ADBusAppendArguments(m, "s", -1);
    ADBusAppendString(m, ks_cstr(out), ks_size(out));

    ks_free(out);
}





// ----------------------------------------------------------------------------
// Properties (private)
// ----------------------------------------------------------------------------

static void GetPropertyCallback(
        struct ADBusCallDetails*    details)
{
    struct UserPointer* p = (struct UserPointer*) details->user2;
    struct ADBusObject* object = (struct ADBusObject*) p->pointer;

    // Unpack the message
    const char* iname  = ADBusCheckString(details, NULL);
    const char* pname  = ADBusCheckString(details, NULL);
    ADBusCheckMessageEnd(details);

    // Get the interface
    struct ADBusInterface* interface = ADBusGetBoundInterface(
            object,
            iname,
            -1,
            &details->user2);

    if (!interface)
        ThrowInvalidInterface(details);

    // Get the property
    struct ADBusMember* property = ADBusGetInterfaceMember(
            interface,
            ADBusPropertyMember,
            pname,
            -1);

    if (!property)
        ThrowInvalidProperty(details);

    // Check that we can read the property
    if (!ADBusIsPropertyReadable(property))
        ThrowWriteOnlyProperty(details);

    // If no reply is wanted we are finished
    if (!details->returnMessage)
        return;

    // Get the property value
    struct ADBusMarshaller* m
            = ADBusArgumentMarshaller(details->returnMessage);
    details->propertyMarshaller = m;

    size_t typeSize;
    const char* type = ADBusPropertyType(property, &typeSize);

    ADBusAppendArguments(m, "v", -1);
    ADBusBeginVariant(m, type, typeSize);
    ADBusCallGetProperty(property, details);
    ADBusEndVariant(m);
}

// ----------------------------------------------------------------------------

static void GetAllPropertiesCallback(
        struct ADBusCallDetails*    details)
{
    struct UserPointer* p = (struct UserPointer*) details->user2;
    struct ADBusObject* object = (struct ADBusObject*) p->pointer;

    // Unpack the message
    const char* iname = ADBusCheckString(details, NULL);
    ADBusCheckMessageEnd(details);

    // Get the interface
    struct ADBusInterface* interface = ADBusGetBoundInterface(
            object,
            iname,
            -1,
            &details->user2);

    if (!interface)
        ThrowInvalidInterface(details);

    // If no reply is wanted we are finished
    if (!details->returnMessage)
        return;

    // Iterate over the properties and marshall up the values
    struct ADBusMarshaller* m = ADBusArgumentMarshaller(details->returnMessage);
    details->propertyMarshaller = m;
    ADBusAppendArguments(m, "a{sv}", -1);
    ADBusBeginArray(m);

    khash_t(MemberPtr)* mbrs = interface->members;
    for (khiter_t mi = kh_begin(mbrs); mi != kh_end(mbrs); ++mi) {
        if (kh_exist(mbrs, mi)) {
            struct ADBusMember* mbr = kh_val(mbrs, mi);
            // Check that it is a property
            if (mbr->type != ADBusPropertyMember) {
                continue;
            }

            // Check that we can read the property
            if (!ADBusIsPropertyReadable(mbr)) {
                continue;
            }

            // Set the property entry
            ADBusBeginDictEntry(m);

            ADBusAppendString(m, mbr->name, -1);

            ADBusBeginVariant(m, mbr->propertyType, -1);
            ADBusCallGetProperty(mbr, details);
            ADBusEndVariant(m);

            ADBusEndDictEntry(m);
        }
    }

    ADBusEndArray(m);
}

// ----------------------------------------------------------------------------

static void SetPropertyCallback(
        struct ADBusCallDetails*    details)
{
    struct UserPointer* p = (struct UserPointer*) details->user2;
    struct ADBusObject* object = (struct ADBusObject*) p->pointer;

    // Unpack the message
    const char* iname = ADBusCheckString(details, NULL);
    const char* pname = ADBusCheckString(details, NULL);

    // Get the interface
    struct ADBusInterface* interface = ADBusGetBoundInterface(
            object,
            iname,
            -1,
            &details->user2);

    if (!interface)
        ThrowInvalidInterface(details);

    // Get the property
    struct ADBusMember* property = ADBusGetInterfaceMember(
            interface,
            ADBusPropertyMember,
            pname,
            -1);

    if (!property)
        ThrowInvalidProperty(details);

    // Check that we can write the property
    if (!ADBusIsPropertyWritable(property))
        ThrowReadOnlyProperty(details);

    // Get the property type
    const char* sig = ADBusCheckVariantBegin(details, NULL);

    // Check the property type
    const char* type = ADBusPropertyType(property, NULL);
    if (strcmp(type, sig) != 0)
        ThrowInvalidArgument(details);

    // Set the property
    details->propertyIterator = details->arguments;
    ADBusCallSetProperty(property, details);
}



