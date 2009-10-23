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
#include "vector.h"
#include "str.h"

#include <assert.h>

#ifdef WIN32
# define strdup _strdup
#endif

// ----------------------------------------------------------------------------
// Errors (private)
// ----------------------------------------------------------------------------

static void SetupInvalidArgument(struct ADBusCallDetails* details)
{
    ADBusSetupError(details,
            "nz.co.foobar.ADBus.Error.InvalidArgument", -1,
            "Invalid arguments passed to a method call.", -1);
}

// ----------------------------------------------------------------------------

static void SetupInvalidPath(struct ADBusCallDetails* details)
{
    ADBusSetupError(details,
            "nz.co.foobar.ADBus.Error.ObjectNotFound", -1,
            "No object exists for the requested path.", -1);
}

// ----------------------------------------------------------------------------

static void SetupInvalidInterface(struct ADBusCallDetails* details)
{
    ADBusSetupError(details,
            "nz.co.foobar.ADBus.Error.InterfaceNotFound", -1,
            "The requested path does not export the requested interface.", -1);
}

// ----------------------------------------------------------------------------

static void SetupInvalidMethod(struct ADBusCallDetails* details)
{
    ADBusSetupError(details,
            "nz.co.foobar.ADBus.Error.MethodNotFound", -1,
            "Method name you invoked isn't known by the object you invoked it on.", -1);
}

// ----------------------------------------------------------------------------

static void SetupInvalidProperty(struct ADBusCallDetails* details)
{
    ADBusSetupError(details,
            "nz.co.foobar.ADBus.Error.PropertyNotFound", -1,
            "The requested object and interface do not export the requested property.", -1);
}

// ----------------------------------------------------------------------------

static void SetupReadOnlyProperty(struct ADBusCallDetails* details)
{
    ADBusSetupError(details,
            "nz.co.foobar.ADBus.Error.ReadOnlyProperty", -1,
            "The requested property is read only.", -1);
}

// ----------------------------------------------------------------------------

static void SetupWriteOnlyProperty(struct ADBusCallDetails* details)
{
    ADBusSetupError(details,
            "nz.co.foobar.ADBus.Error.WriteOnlyProperty", -1,
            "The requested property is write only.", -1);
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

    c->objects = kh_init(ADBusObjectPtr);

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

static void FreeMatchData(struct ADBusMatch* match);
static void FreeObject(struct ADBusObject* object);

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
    kh_destroy(ADBusObjectPtr, c->objects);

    for (size_t i = 0; i < match_vector_size(&c->registrations); ++i)
        FreeMatchData(&c->registrations[i]);
    match_vector_free(&c->registrations);

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
    //struct Timer timer;
    //TimerBegin(&timer);
    if (c->sendCallback)
        c->sendCallback(message, c->sendCallbackData);
    //TimerEnd(&timer, "Send");
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
    khiter_t oi = kh_get(ADBusObjectPtr, c->objects, path);
    if (oi == kh_end(c->objects)) {
        SetupInvalidPath(details);
        return;
    }
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

        if (!interface) {
            SetupInvalidInterface(details);
            return;
        }

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

    if (!member) {
        SetupInvalidMethod(details);
        return;
    }

    // Dispatch the method call
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

static uint ReplySerialMatches(struct ADBusMatch* r, struct ADBusMessage* message)
{
    if (!r->checkReplySerial)
        return 1; // ignoring this field
    if (!ADBusHasReplySerial(message))
        return 0; // message doesn't have this field

    return r->replySerial == ADBusGetReplySerial(message);
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

    for (size_t i = 0; i < match_vector_size(&c->registrations);)
    {
        struct ADBusMatch* r = &c->registrations[i];
        if (r->type && r->type != type) {
            ++i;
        } else if (!ReplySerialMatches(r, m)) {
            ++i;
        } else if (!Matches(r->sender, sender)) {
            ++i;
        } else if (!Matches(r->destination, destination)) {
            ++i;
        } else if (!Matches(r->path, path)) {
            ++i;
        } else if (!Matches(r->interface, interface)) {
            ++i;
        } else if (!Matches(r->member, member)) {
            ++i;
        } else if (!Matches(r->errorName, errorName)) {
            ++i;
        } else {
            if (r->callback) {
                // We want to reset the argument iterator for every match
                ADBusArgumentIterator(m, details->arguments);

                details->user1 = r->user1;
                details->user2 = r->user2;

                r->callback(details);
            }

            if (r->removeOnFirstMatch) {
                FreeMatchData(r);
                match_vector_remove(&c->registrations, i, 1);
            } else {
                ++i;
            }
        }
    }
    return;
}

// ----------------------------------------------------------------------------

void ADBusRawDispatch(struct ADBusCallDetails*    details)
{
    size_t sz;
    ADBusGetMessageData(details->message, NULL, &sz);
    if (sz == 0)
        return;

    // Dispatch the method call
    if (ADBusGetMessageType(details->message) == ADBusMethodCallMessage) {
        ADBusArgumentIterator(details->message, details->arguments);
        ADBusSetupReturn(details->returnMessage, details->connection, details->message);
        DispatchMethodCall(details);
    }

    // Reset the returnMessage field for the dispatch match so that matches 
    // don't try and use it
    struct ADBusMessage* returnMessage = details->returnMessage;
    details->returnMessage = NULL;

    // Dispatch match
    ADBusArgumentIterator(details->message, details->arguments);
    DispatchMatch(details);

    details->returnMessage = returnMessage;
}

// ----------------------------------------------------------------------------

void ADBusDispatch(
        struct ADBusConnection* c,
        struct ADBusMessage*    message)
{
    size_t sz;
    ADBusGetMessageData(message, NULL, &sz);
    if (sz == 0)
        return;

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
    if ( details.returnMessage != NULL
      && !details.manualReply)
    {
        ADBusSendMessage(c, details.returnMessage);
    }
}

// ----------------------------------------------------------------------------

struct ADBusStreamBuffer* ADBusCreateStreamBuffer()
{
    return NEW(struct ADBusStreamBuffer);
}

// ----------------------------------------------------------------------------

void ADBusFreeStreamBuffer(struct ADBusStreamBuffer* buffer)
{
    if (!buffer)
        return;

    u8vector_free(&buffer->buf);
    free(buffer);
}

// ----------------------------------------------------------------------------

static uint RequireDataInParseBuffer(
        struct ADBusStreamBuffer*   b,
        size_t                      needed,
        const uint8_t**             data,
        size_t*                     size)
{
    int toAdd = needed - u8vector_size(&b->buf);
    if (toAdd > (int) *size) {
        // Don't have enough data
        uint8_t* dest = u8vector_insert_end(&b->buf, *size);
        memcpy(dest, *data, *size);
        *data += *size;
        *size = 0;
        return 0;

    } else if (toAdd > 0) {
        // Need to push data into the buffer, but we have enough
        uint8_t* dest = u8vector_insert_end(&b->buf, toAdd);
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
    if (u8vector_size(&b->buf) > 0)
    {
        // Use up message from the stream buffer
        
        // We need to add enough so we can figure out how big the next message is
        if (!RequireDataInParseBuffer(b, sizeof(struct ADBusExtendedHeader_), data, size))
            return 0;

        // Now we add enough to the buffer to be able to parse the message
        size_t messageSize = ADBusNextMessageSize(b->buf, u8vector_size(&b->buf));
        assert(messageSize > 0);
        if (!RequireDataInParseBuffer(b, messageSize, data, size))
            return 0;

        int err = ADBusSetMessageData(message, b->buf, messageSize);
        u8vector_remove(&b->buf, 0, messageSize);
        return err;
    }
    else
    {
        size_t messageSize = ADBusNextMessageSize(*data, *size);
        if (messageSize == 0 || messageSize > *size)
        {
            // We don't have enough to parse the message
            uint8_t* dest = u8vector_insert_end(&b->buf, *size);
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
    c->uniqueService = strdup(field.string);
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
    ADBusMatchInit(&match);
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
    ADBusMatchInit(&match);
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

static void CloneString(const char** str, int* size)
{
    if (!*str)
    {
        *size = 0;
    }
    else
    {
        if (*size < 0)
            *size = strlen(*str);
        *str = strndup(*str, *size);
    }
}

static str_t SanitisePath(const char* path, int size);

static void CloneMatch(const struct ADBusMatch* from,
                       struct ADBusMatch* to)
{
    memcpy(to, from, sizeof(struct ADBusMatch));
    CloneString(&to->sender,         &to->senderSize);
    CloneString(&to->destination,    &to->destinationSize);
    CloneString(&to->interface,      &to->interfaceSize);
    CloneString(&to->member,         &to->memberSize);
    CloneString(&to->errorName,      &to->errorNameSize);
    if (from->path)
        to->path = SanitisePath(from->path, from->pathSize);
}

static void FreeMatchData(struct ADBusMatch* match)
{
    if (!match)
        return;
    free((char*) match->sender);
    free((char*) match->destination);
    free((char*) match->interface);
    free((char*) match->member);
    free((char*) match->errorName);
    str_free((str_t*) &match->path);
    ADBusUserFree(match->user1);
    ADBusUserFree(match->user2);
}

// ----------------------------------------------------------------------------

uint32_t ADBusAddMatch(
        struct ADBusConnection*     c,
        const struct ADBusMatch*    reg)
{
    struct ADBusMatch* match = match_vector_insert_end(&c->registrations, 1);
    CloneMatch(reg, match);
    if (match->id <= 0)
        match->id = ADBusNextMatchId(c);

    if (reg->addMatchToBusDaemon) {
        ADBusSetupAddBusMatch(c->outgoingMessage, c, match);
        ADBusSendMessage(c, c->outgoingMessage);
    }

    return match->id;
}

// ----------------------------------------------------------------------------

void ADBusRemoveMatch(struct ADBusConnection* c, uint32_t id)
{
    for (size_t i = 0; i < match_vector_size(&c->registrations); ++i) {
        if (c->registrations[i].id == id) {
            FreeMatchData(&c->registrations[i]);
            match_vector_remove(&c->registrations, i, 1);
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

static void FreeObject(struct ADBusObject* o)
{
    if (!o)
        return;

    for (khiter_t ii = kh_begin(o->interfaces); ii != kh_end(o->interfaces); ++ii) {
        if (kh_exist(o->interfaces, ii)) {
            ADBusUserFree(kh_val(o->interfaces, ii).user2);
        }
    }
    kh_destroy(BoundInterface, o->interfaces);
    pobject_vector_free(&o->children);
    free(o->name);
    free(o);
}

// ----------------------------------------------------------------------------

static str_t SanitisePath(
        const char*     path,
        int             size)
{
    if (size < 0)
        size = strlen(path);

    str_t ret = NULL;

    // Make sure it starts with a /
    if (size == 0 || path[0] != '/')
        str_append(&ret, "/");
    if (size > 0)
        str_append_n(&ret, path, size);

    // Remove repeating slashes
    for (size_t i = 1; i < str_size(&ret); ++i) {
        if (ret[i] == '/' && ret[i-1] == '/') {
            str_remove(&ret, i, 1);
        }
    }

    // Remove trailing /
    if (str_size(&ret) > 1 && ret[str_size(&ret) - 1] == '/')
        str_remove_end(&ret, 1);

    return ret;
}

static void ParentPath(
        char*           path,
        size_t          size,
        size_t*         parentSize)
{
    // Path should have already been sanitised so double /'s should not happen
#ifndef NDEBUG
    str_t sanitised = SanitisePath(path, size);
    assert(path[size] == '\0');
    assert(strcmp(sanitised, path) == 0);
    str_free(&sanitised);
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
    khiter_t ki = kh_put(ADBusObjectPtr, c->objects, path, &added);
    if (!added) {
        return kh_val(c->objects, ki);
    }

    struct ADBusObject* o = NEW(struct ADBusObject);
    o->name         = strdup(path);
    o->connection   = c;
    o->interfaces   = kh_init(BoundInterface);

    kh_key(c->objects, ki) = o->name;
    kh_val(c->objects, ki) = o;

    ADBusBindInterface(o, c->introspectable, CreateUserPointer(o));
    ADBusBindInterface(o, c->properties, CreateUserPointer(o));

    // Setup the parent-child links
    if (strcmp(path, "/") != 0) {
        struct ADBusObject** pchild;
        size_t parentSize;

        ParentPath(path, size, &parentSize);
        o->parent = DoAddObject(c, path, parentSize);
        pchild  = pobject_vector_insert_end(&o->parent->children, 1);
        *pchild = o;
    }

    return o;
}

struct ADBusObject* ADBusAddObject(
        struct ADBusConnection* c,
        const char*             path,
        int                     size)
{
    str_t name = SanitisePath(path, size);
    struct ADBusObject* o = DoAddObject(c, name, str_size(&name));
    str_free(&name);
    return o;
}


// ----------------------------------------------------------------------------

struct ADBusObject* ADBusGetObject(
        struct ADBusConnection* c,
        const char*             path,
        int                     size)
{
    struct ADBusObject* ret = NULL;

    str_t name = SanitisePath(path, size);
    khiter_t ki = kh_get(ADBusObjectPtr, c->objects, name);
    if (ki != kh_end(c->objects))
        ret = kh_val(c->objects, ki);

    str_free(&name);
    return ret;
}

// ----------------------------------------------------------------------------

int ADBusBindInterface(
        struct ADBusObject*     o,
        struct ADBusInterface*  interface,
        struct ADBusUser*       user2)
{
    int added;
    khiter_t bi = kh_put(BoundInterface, o->interfaces, interface->name, &added);
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
    if (kh_size(o->interfaces) > 2 || pobject_vector_size(&o->children) > 0)
        return;

    // Remove it from its parent
    struct ADBusObject* parent = o->parent;
    for (size_t i = 0; i < pobject_vector_size(&parent->children); ++i) {
        if (parent->children[i] == o) {
            pobject_vector_remove(&parent->children, i, 1);
            break;
        }
    }
    CheckRemoveObject(o->parent);

    // Remove it from the object lookup table
    struct ADBusConnection* c = o->connection;
    khiter_t oi = kh_get(ADBusObjectPtr, c->objects, o->name);
    if (oi != kh_end(c->objects))
        kh_del(ADBusObjectPtr, c->objects, oi);

    // Free the object
    FreeObject(o);
}

// ----------------------------------------------------------------------------

int ADBusUnbindInterface(
        struct ADBusObject*     o,
        struct ADBusInterface*  interface)
{
    khiter_t bi = kh_get(BoundInterface, o->interfaces, interface->name);
    if (bi == kh_end(o->interfaces))
        return -1;
    if (interface != kh_val(o->interfaces, bi).interface)
        return -1;

    ADBusUserFree(kh_val(o->interfaces, bi).user2);
    kh_del(BoundInterface, o->interfaces, bi);

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
        interface = strndup(interface, interfaceSize);

    khiter_t bi = kh_get(BoundInterface, o->interfaces, interface);

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
            struct BoundInterface* b = &kh_val(o->interfaces, bi);
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
        str_t*               out)
{
    for (size_t i = 0; i < arg_vector_size(&m->inArguments); ++i)
    {
        struct Argument* a = &m->inArguments[i];
        str_append(out, "\t\t\t<arg type=\"");
        str_append(out, a->type);
        if (a->name)
        {
            str_append(out, "\" name=\"");
            str_append(out, a->name);
        }
        str_append(out, "\" direction=\"in\"/>\n");
    }

    for (size_t i = 0; i < arg_vector_size(&m->outArguments); ++i)
    {
        struct Argument* a = &m->outArguments[i];
        str_append(out, "\t\t\t<arg type=\"");
        str_append(out, a->type);
        if (a->name)
        {
            str_append(out, "\" name=\"");
            str_append(out, a->name);
        }
        str_append(out, "\" direction=\"out\"/>\n");
    }

}

// ----------------------------------------------------------------------------

static void IntrospectAnnotations(
        struct ADBusMember*     m,
        str_t*               out)
{
    for (khiter_t ai = kh_begin(m->annotations); ai != kh_end(m->annotations); ++ai) {
        if (kh_exist(m->annotations, ai)) {
            str_append(out, "\t\t\t<annotation name=\"");
            str_append(out, kh_key(m->annotations, ai));
            str_append(out, "\" value=\"");
            str_append(out, kh_val(m->annotations, ai));
            str_append(out, "\"/>\n");
        }
    }
}
// ----------------------------------------------------------------------------

static void IntrospectMember(
        struct ADBusMember*     m,
        str_t*               out)
{
    switch (m->type)
    {
        case ADBusPropertyMember:
            {
                str_append(out, "\t\t<property name=\"");
                str_append(out, m->name);
                str_append(out, "\" type=\"");
                str_append(out, m->propertyType);
                str_append(out, "\" access=\"");
                uint read  = ADBusIsPropertyReadable(m);
                uint write = ADBusIsPropertyWritable(m);
                if (read && write)
                    str_append(out, "readwrite");
                else if (read)
                    str_append(out, "read");
                else if (write)
                    str_append(out, "write");
                else
                    assert(0);

                if (kh_size(m->annotations) == 0)
                {
                    str_append(out, "\"/>\n");
                }
                else
                {
                    str_append(out, "\">\n");
                    IntrospectAnnotations(m, out);
                    str_append(out, "\t\t</property>\n");
                }
            }
            break;
        case ADBusMethodMember:
            {
                str_append(out, "\t\t<method name=\"");
                str_append(out, m->name);
                str_append(out, "\">\n");

                IntrospectAnnotations(m, out);
                IntrospectArguments(m, out);

                str_append(out, "\t\t</method>\n");
            }
            break;
        case ADBusSignalMember:
            {
                str_append(out, "\t\t<signal name=\"");
                str_append(out, m->name);
                str_append(out, "\">\n");

                IntrospectAnnotations(m, out);
                IntrospectArguments(m, out);

                str_append(out, "\t\t</signal>\n");
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
        str_t*                  out)
{
    for (khiter_t bi = kh_begin(o->interfaces); bi != kh_end(o->interfaces); ++bi) {
        if (kh_exist(o->interfaces, bi)) {
            struct ADBusInterface* i = kh_val(o->interfaces, bi).interface;
            str_append(out, "\t<interface name=\"");
            str_append(out, i->name);
            str_append(out, "\">\n");

            for (khiter_t mi = kh_begin(i->members); mi != kh_end(i->members); ++mi) {
                if (kh_exist(i->members, mi)) {
                    IntrospectMember(kh_val(i->members, mi), out);
                }
            }

            str_append(out, "\t</interface>\n");
        }
    }
}

// ----------------------------------------------------------------------------

static void IntrospectNode(
        struct ADBusObject*     o,
        str_t*               out)
{
    str_append(
            out,
            "<!DOCTYPE node PUBLIC \"-//freedesktop/DTD D-BUS Object Introspection 1.0//EN\"\n"
            "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
            "<node>\n");

    IntrospectInterfaces(o, out);

    size_t namelen = strlen(o->name);
    // Now add the child objects
    for (size_t i = 0; i < pobject_vector_size(&o->children); ++i) {
        // Find the child tail ie ("bar" for "/foo/bar" or "foo" for "/foo")
        const char* child = o->children[i]->name;
        child += namelen;
        if (namelen > 1)
            child += 1; // +1 for '/' when o is not the root node

        str_append(out, "\t<node name=\"");
        str_append(out, child);
        str_append(out, "\"/>\n");
    }

    str_append(out, "</node>\n");
}

// ----------------------------------------------------------------------------

static void IntrospectCallback(
        struct ADBusCallDetails*    details)
{
    // If no reply is wanted, we're done
    if (!details->returnMessage) {
        return;
    }

    struct UserPointer* p = (struct UserPointer*) details->user2;
    struct ADBusObject* o = (struct ADBusObject*) p->pointer;

    str_t out = NULL;
    IntrospectNode(o, &out);

    struct ADBusMarshaller* m = ADBusArgumentMarshaller(details->returnMessage);
    ADBusBeginArgument(m, "s", -1);
    ADBusAppendString(m, &out[0], str_size(&out));
    ADBusEndArgument(m);

    str_free(&out);
}





// ----------------------------------------------------------------------------
// Properties (private)
// ----------------------------------------------------------------------------

static void GetPropertyCallback(
        struct ADBusCallDetails*    details)
{
    struct UserPointer* p = (struct UserPointer*) details->user2;
    struct ADBusObject* object = (struct ADBusObject*) p->pointer;

    struct ADBusField field;

    // Get the interface
    ADBusIterate(details->arguments, &field);
    if (field.type != ADBusStringField) {
        SetupInvalidArgument(details);
        return;
    }

    struct ADBusInterface* interface = ADBusGetBoundInterface(
            object,
            field.string,
            field.size,
            &details->user2);

    if (!interface) {
        SetupInvalidInterface(details);
        return;
    }

    // Get the property
    ADBusIterate(details->arguments, &field);
    if (field.type != ADBusStringField) {
        SetupInvalidArgument(details);
        return;
    }

    struct ADBusMember* property = ADBusGetInterfaceMember(
            interface,
            ADBusPropertyMember,
            field.string,
            field.size);

    if (!property) {
        SetupInvalidProperty(details);
        return;
    }

    // Check that we can read the property
    if (!ADBusIsPropertyReadable(property)) {
        SetupWriteOnlyProperty(details);
        return;
    }

    // If no reply is wanted we are finished
    if (!details->returnMessage)
        return;

    // Get the property value
    struct ADBusMarshaller* m
            = ADBusArgumentMarshaller(details->returnMessage);
    details->propertyMarshaller = m;

    size_t typeSize;
    const char* type = ADBusPropertyType(property, &typeSize);

    ADBusBeginArgument(m, "v", -1);
    ADBusBeginVariant(m, type, typeSize);
    
    ADBusCallGetProperty(property, details);

    ADBusEndVariant(m);
    ADBusEndArgument(m);
}

// ----------------------------------------------------------------------------

static void GetAllPropertiesCallback(
        struct ADBusCallDetails*    details)
{
    struct UserPointer* p = (struct UserPointer*) details->user2;
    struct ADBusObject* object = (struct ADBusObject*) p->pointer;

    struct ADBusField field;

    // Get the interface
    ADBusIterate(details->arguments, &field);
    if (field.type != ADBusStringField) {
        SetupInvalidArgument(details);
        return;
    }

    struct ADBusInterface* interface = ADBusGetBoundInterface(
            object,
            field.string,
            field.size,
            &details->user2);

    if (!interface) {
        SetupInvalidInterface(details);
        return;
    }

    // If no reply is wanted we are finished
    if (!details->returnMessage)
        return;

    // Iterate over the properties
    struct ADBusMarshaller* m = ADBusArgumentMarshaller(details->returnMessage);
    details->propertyMarshaller = m;
    ADBusBeginArgument(m, "a{sv}", -1);
    ADBusBeginArray(m);

    khash_t(ADBusMemberPtr)* mbrs = interface->members;
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
    ADBusEndArgument(m);
}

// ----------------------------------------------------------------------------

static void SetPropertyCallback(
        struct ADBusCallDetails*    details)
{
    struct UserPointer* p = (struct UserPointer*) details->user2;
    struct ADBusObject* object = (struct ADBusObject*) p->pointer;

    struct ADBusField field;

    // Get the interface
    ADBusIterate(details->arguments, &field);
    if (field.type != ADBusStringField) {
        SetupInvalidArgument(details);
        return;
    }

    struct ADBusInterface* interface = ADBusGetBoundInterface(
            object,
            field.string,
            field.size,
            &details->user2);

    if (!interface) {
        SetupInvalidInterface(details);
        return;
    }

    // Get the property
    ADBusIterate(details->arguments, &field);
    if (field.type != ADBusStringField) {
        SetupInvalidArgument(details);
        return;
    }

    struct ADBusMember* property = ADBusGetInterfaceMember(
            interface,
            ADBusPropertyMember,
            field.string,
            field.size);

    if (!property) {
        SetupInvalidProperty(details);
        return;
    }

    // Check that we can write the property
    if (!ADBusIsPropertyWritable(property)) {
        SetupReadOnlyProperty(details);
        return;
    }

    // Get the property type
    ADBusIterate(details->arguments, &field);
    if (field.type != ADBusVariantBeginField) {
        SetupInvalidArgument(details);
        return;
    }
    const char* messageType = field.string;

    // Check the property type
    const char* type = ADBusPropertyType(property, NULL);
    if (strcmp(type, messageType) != 0) {
        SetupInvalidArgument(details);
        return;
    }

    // Set the property
    details->propertyIterator = details->arguments;
    ADBusCallSetProperty(property, details);
}



