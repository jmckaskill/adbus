// vim: ts=4 sw=4 sts=4 et
//
// Copyright (c) 2009 James R. McKaskill
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// ----------------------------------------------------------------------------

#include "Connection_p.h"

#include "Misc_p.h"

#include <algorithm>

#include <assert.h>
#include <string.h>



// ----------------------------------------------------------------------------
// Connection management
// ----------------------------------------------------------------------------

struct ADBusConnection* ADBusCreateConnection()
{
    struct ADBusConnection* c = new ADBusConnection;
#if 0
    c->marshaller = ADBusCreateMarshaller();
    c->parser = ADBusCreateParser();
    ADBusSetParserCallback(c->parser, &_ADBusDispatchMessage, (void*) c);



    c->introspectableInterface 
        = ADBusCreateInterface("org.freedesktop.DBus.Introspectable", -1);

    ADBusMember* m = ADBusAddMember(c->introspectableInterface,
            ADBusMethodMember,
            "Introspect", -1);
    ADBusAddArgument(m, ADBusOutArgument, "xml_data", -1, "s", -1);
    ADBusSetMethodCallback(m, &_ADBusIntrospectCallback);


#endif
    return c;
}

// ----------------------------------------------------------------------------

ADBusConnection::~ADBusConnection()
{
    ADBusFreeInterface(introspectableInterface);
    ADBusFreeMarshaller(marshaller);
    ADBusFreeParser(parser);

    ADBusConnection::Objects::iterator oi;

    for (oi = this->objects.begin(); oi != this->objects.end(); ++oi)
        delete oi->second;
}

// ----------------------------------------------------------------------------

void ADBusFreeConnection(struct ADBusConnection* connection)
{
    delete connection;
}

// ----------------------------------------------------------------------------

void ADBusSetConnectionSendCallback(
        struct ADBusConnection* c,
        ADBusConnectionSendCallback callback,
        struct ADBusUser* user)
{
    c->sendCallback = callback;
    ADBusUserClone(user, &c->sendCallbackData);
}

// ----------------------------------------------------------------------------

int ADBusConnectionParse(
        struct ADBusConnection* c,
        const uint8_t*          data,
        size_t                  size)
{
    return ADBusParse(c->parser, data, size);
}

// ----------------------------------------------------------------------------

int _ADBusDispatchMessage(void* connection, struct ADBusMessage* message)
{
    struct ADBusConnection* c = (struct ADBusConnection*) connection;
    enum ADBusMessageType type = ADBusGetMessageType(message);
    if (type == ADBusMethodCallMessage)
        return _ADBusDispatchMethodCall(c, message);
    else
        return _ADBusDispatchMatch(c, message);
}

// ----------------------------------------------------------------------------

static int CallMethodOnInterface(
        struct ADBusInterface*  interface,
        struct ADBusObject*     object,
        const struct ADBusUser* bindData,
        const char*             memberName,
        struct ADBusMessage*    message,
        bool*                   called = NULL)
{
    ADBusInterface::Members::const_iterator mi;
    mi = interface->members.find(memberName);

    if (*called)
        *called = false;

    if ( mi == interface->members.end() 
      || mi->second == NULL
      || mi->second->type != ADBusMethodMember) {
        return 0;
    }

    ADBusConnection* c = object->connection;
    ADBusMember* member = mi->second;

    assert(member->methodCallback);

    int err = member->methodCallback(c,
                                     bindData,
                                     member,
                                     message);

    if (called)
        *called = true;

    return err;
}

// ----------------------------------------------------------------------------

int _ADBusDispatchMethodCall(
        struct ADBusConnection* c,
        struct ADBusMessage* message)
{
    const char* path = ADBusGetPath(message, NULL);
    assert(path); // parser should have already checked this

    ADBusConnection::Objects::const_iterator oi = c->objects.find(path);
    if (oi == c->objects.end() || oi->second == NULL) {
        ADBusSendError(
                c,
                message,
                "nz.co.foobar.ADBus.Error.InvalidPath", -1,
                "The requested path could not be found", -1);
        return 0;
    }

    ADBusObject* object = oi->second;

    const char* interface = ADBusGetInterface(message, NULL);
    const char* member = ADBusGetMember(message, NULL);
    assert(member); // should have been checked by parser

    if (interface) {
        ADBusObject::Interfaces::const_iterator ii;
        ii = object->interfaces.find(interface);

        if (ii == object->interfaces.end() || ii->second.interface == NULL) {
            ADBusSendError(
                    c,
                    message,
                    "nz.co.foobar.ADBus.Error.InvalidInterface", -1,
                    "The requested object does not have the requested interface.", -1);
            return 0;
        }

        ADBusInterface* interface = ii->second.interface;

        if (!CallMethodOnInterface(
                    interface,
                    object,
                    &ii->second.user,
                    member,
                    message))
        {
            ADBusSendError(
                    c,
                    message,
                    "nz.co.foobar.ADBus.Error.InvalidMethod", -1,
                    "The requested object does not have the requested method.", -1);
            return 0;
        }

    } else {
        ADBusObject::Interfaces::const_iterator ii;
        for (ii = object->interfaces.begin(); ii != object->interfaces.end(); ++ii) {
            bool called;
            int err = CallMethodOnInterface(
                        ii->second.interface,
                        object,
                        &ii->second.user,
                        member,
                        message,
                        &called);
            if (called)
                return err;
        }

        ADBusSendError(
                c,
                message,
                "nz.co.foobar.ADBus.Error.InvalidMethod", -1,
                "The requested object does not have the requested method.", -1);
        return 0;
    }
    return 0;
}

// ----------------------------------------------------------------------------

int _ADBusDispatchMatch(
        struct ADBusConnection* c,
        struct ADBusMessage* message)
{
    ADBusMessageType type   = ADBusGetMessageType(message);
    uint32_t replySerial    = ADBusGetReplySerial(message);
    const char* sender      = ADBusGetSender(message, NULL);
    const char* destination = ADBusGetDestination(message, NULL);
    const char* path        = ADBusGetPath(message, NULL);
    const char* interface   = ADBusGetInterface(message, NULL);
    const char* member      = ADBusGetMember(message, NULL);
    const char* errorName   = ADBusGetErrorName(message, NULL);

    for (size_t i = 0; i < c->registrations.size();)
    {
        MatchRegistration& r = c->registrations[i];
        if (r.type && r.type != type)
            ++i;
        else if (replySerial && r.replySerial && r.replySerial != replySerial)
            ++i;
        else if (sender && !r.sender.empty() && r.sender != sender)
            ++i;
        else if (destination && !r.destination.empty() && r.destination != destination)
            ++i;
        else if (path && !r.path.empty() && r.path != path)
            ++i;
        else if (interface && !r.interface.empty() && r.interface != interface)
            ++i;
        else if (member && !r.member.empty() && r.member != member)
            ++i;
        else if (errorName && !r.errorName.empty() && r.errorName != errorName)
            ++i;
        else if (r.callback)
        {
            r.callback(
                    c,
                    r.id,
                    &r.user,
                    message);

            if (r.removeOnFirstMatch)
                c->registrations.erase(c->registrations.begin() + i);
            else
                ++i;

            ADBusReparseMessage(message);
        }
    }
    return 0;
}

// ----------------------------------------------------------------------------






// ----------------------------------------------------------------------------
// Bus management
// ----------------------------------------------------------------------------

static int ConnectionCallback(
        struct ADBusConnection*     c,
        int                         id,
        struct ADBusUser*           user,
        struct ADBusMessage*        message)
{
    const char* unique;
    int err = ADBusTakeString(message, &unique, NULL);
    if (err) return err;

    err = ADBusTakeMessageEnd(message);
    if (err) return err;

    c->uniqueService = unique;
    c->connected = true;

    if (c->connectCallback)
        c->connectCallback(c, &c->connectCallbackData);

    return 0;
}

void ADBusConnectToBus(
        struct ADBusConnection*     c,
        ADBusConnectToBusCallback   callback,
        struct ADBusUser*           callbackData)
{
    assert(!c->connected);
    c->connectCallback = callback;
    ADBusUserClone(callbackData, &c->connectCallbackData);

    struct ADBusMatch match;
    memset(&match, 0, sizeof(ADBusMatch));
    match.type                  = ADBusMethodReturnMessage;
    match.callback              = &ConnectionCallback;
    match.replySerial           = ADBusNextSerial(c);
    match.removeOnFirstMatch    = 1;

    ADBusAddMatch(c, &match);

    struct ADBusMarshaller* m = c->marshaller;
    ADBusClearMarshaller(m);
    ADBusSetMessageType(m, ADBusMethodCallMessage);
    ADBusSetSerial(m, match.replySerial);
    ADBusSetDestination(m, "org.freedesktop.DBus", -1);
    ADBusSetPath(m, "/org/freedesktop/DBus", -1);
    ADBusSetInterface(m, "org.freedesktop.DBus", -1);
    ADBusSetMember(m, "Hello", -1);

    ADBusConnectionSendMessage(c, m);
}

// ----------------------------------------------------------------------------

uint ADBusIsConnectedToBus(struct ADBusConnection* c)
{
    return c->connected ? 1 : 0;
}

// ----------------------------------------------------------------------------

const char*  ADBusGetUniqueServiceName(struct ADBusConnection* c, int* size)
{
    assert(c->connected);
    if (size)
        *size = (int)c->uniqueService.size();
    return c->uniqueService.c_str();
}

// ----------------------------------------------------------------------------







// ----------------------------------------------------------------------------
// Callback registrations
// ----------------------------------------------------------------------------

uint32_t ADBusNextSerial(struct ADBusConnection* c)
{
    c->nextSerial++;
    if (!c->nextSerial)
        c->nextSerial++;
    return c->nextSerial;
}

// ----------------------------------------------------------------------------

static void SetString(
        std::string* dest,
        const char* str,
        int size)
{
    if (!str)
        return;
    if (size < 0)
        size = strlen(str);
    *dest = std::string(str, str + size);
}

int ADBusAddMatch(
        struct ADBusConnection*     c,
        const struct ADBusMatch*    reg)
{
    MatchRegistration m;
    SetString(&m.sender,        reg->sender,        reg->senderSize);
    SetString(&m.destination,   reg->destination,   reg->destinationSize);
    SetString(&m.interface,     reg->interface,     reg->interfaceSize);
    SetString(&m.path,          reg->path,          reg->pathSize);
    SetString(&m.member,        reg->member,        reg->memberSize);
    SetString(&m.errorName,     reg->errorName,     reg->errorNameSize);

    m.callback              = reg->callback;
    m.replySerial           = reg->replySerial;
    m.addMatchToBusDaemon   = reg->addMatchToBusDaemon;
    m.removeOnFirstMatch    = reg->removeOnFirstMatch;
    m.type                  = reg->type;
    m.id                    = c->nextWatchId++;

    ADBusUserClone(&reg->user, &m.user);

    c->registrations.push_back(m);

    return m.id;
}

// ----------------------------------------------------------------------------

void ADBusRemoveMatch(struct ADBusConnection* c, int id)
{
    ADBusConnection::Registrations::iterator ii = 
        std::find(c->registrations.begin(), c->registrations.end(), id);

    if (ii != c->registrations.end())
        c->registrations.erase(ii);
}

// ----------------------------------------------------------------------------

void ADBusSendError(struct ADBusConnection* c,
        struct ADBusMessage* message,
        const char* errorName, int errorNameSize,
        const char* errorMessage, int errorMessageSize)
{
    int destinationSize;
    const char* destination = ADBusGetSender(message, &destinationSize);
    int serial = ADBusGetSerial(message);

    ADBusSendErrorExpanded(
            c,
            serial,
            destination,
            destinationSize,
            errorName,
            errorNameSize,
            errorMessage,
            errorMessageSize);
}

// ----------------------------------------------------------------------------

void ADBusSendErrorExpanded(
        struct ADBusConnection*     c,
        int                         replySerial,
        const char*                 destination,
        int                         destinationSize,
        const char*                 errorName,
        int                         errorNameSize,
        const char*                 errorMessage,
        int                         errorMessageSize)
{
    ADBusSetupReturnMarshallerExpanded(
            c,
            replySerial,
            destination,
            destinationSize,
            c->marshaller);

    ADBusSetMessageType(c->marshaller, ADBusErrorMessage);
    ADBusSetErrorName(c->marshaller, errorName, errorNameSize);
    if (errorMessage)
    {
        ADBusSetSignature(c->marshaller, "s", 1);
        ADBusAppendString(c->marshaller, errorMessage, errorMessageSize);
    }
    ADBusConnectionSendMessage(c, c->marshaller);
}

// ----------------------------------------------------------------------------

void ADBusSetupReturnMarshaller(
        struct ADBusConnection*     c,
        struct ADBusMessage*        message,
        struct ADBusMarshaller*     marshaller)
{
    int destinationSize;
    const char* destination = ADBusGetSender(message, &destinationSize);
    int replySerial = ADBusGetSerial(message);

    ADBusSetupReturnMarshallerExpanded(
            c,
            replySerial,
            destination,
            destinationSize,
            marshaller);
}

// ----------------------------------------------------------------------------

void ADBusSetupReturnMarshallerExpanded(
        struct ADBusConnection*     c,
        int                         replySerial,
        const char*                 destination,
        int                         destinationSize,
        struct ADBusMarshaller*     marshaller)
{
    int serial = ADBusNextSerial(c);

    ADBusClearMarshaller(marshaller);
    ADBusSetSerial(marshaller, serial);
    ADBusSetMessageType(marshaller, ADBusMethodReturnMessage);
    ADBusSetReplySerial(marshaller, replySerial);
    if (destination)
        ADBusSetDestination(marshaller, destination, destinationSize);
}

// ----------------------------------------------------------------------------

void ADBusSetupSignalMarshaller(
        struct ADBusObject*         object,
        struct ADBusMember*         signal,
        struct ADBusMarshaller*     marshaller)
{
    struct ADBusConnection* c = object->connection;
    struct ADBusInterface* i = signal->interface;
    int serial = ADBusNextSerial(c);

    ADBusClearMarshaller(marshaller);
    ADBusSetSerial(marshaller, serial);
    ADBusSetMessageType(marshaller, ADBusSignalMessage);
    ADBusSetPath(marshaller, object->name.c_str(), object->name.size());
    ADBusSetInterface(marshaller, i->name.c_str(), i->name.size());
    ADBusSetMember(marshaller, signal->name.c_str(), signal->name.size());
}

// ----------------------------------------------------------------------------








// ----------------------------------------------------------------------------
// Interface management
// ----------------------------------------------------------------------------

struct ADBusInterface* ADBusCreateInterface(
        const char*     name,
        int             size)
{
    if (size < 0)
        size = strlen(name);

    std::string n(name, name + size);

    ADBusInterface* i = new ADBusInterface;
    i->name = n;

    return i;  
}

// ----------------------------------------------------------------------------

void ADBusFreeInterface(struct ADBusInterface* i)
{
    delete i;
}

// ----------------------------------------------------------------------------

struct ADBusMember* ADBusAddMember(
        struct ADBusInterface*  i,
        enum ADBusMemberType    type,
        const char*             name,
        int                     size)
{
    assert(name);

    if (size < 0)
        size = strlen(name);

    std::string n(name, name + size);

    struct ADBusMember*& m = i->members[n];
    assert(m == NULL);

    m = new struct ADBusMember;

    m->interface = i;
    m->name      = n;
    m->type      = type;

    return m;
}

// ----------------------------------------------------------------------------

struct ADBusMember* ADBusGetInterfaceMember(
        struct ADBusInterface*      interface,
        enum ADBusMemberType        type,
        const char*                 name,
        int                         size)
{
    if (size < 0)
        size = strlen(name);

    std::string n(name, name + size);

    ADBusInterface::Members::iterator ii = interface->members.find(n);
    if (ii == interface->members.end())
        return NULL;
    else
        return ii->second;
}

// ----------------------------------------------------------------------------

void ADBusAddArgument(struct ADBusMember* m,
        enum ADBusArgumentDirection direction,
        const char* name, int nameSize,
        const char* type, int typeSize)
{
    assert(type);

    if (name && nameSize < 0)
        nameSize = strlen(name);

    if (typeSize < 0)
        typeSize = strlen(type);

    Argument arg;
    if (name)
        arg.name = std::string(name, name + nameSize);
    arg.type = std::string(type, type + typeSize);

    if (direction == ADBusInArgument) {
        m->inArguments.push_back(arg);
        m->inArgumentsSignature += arg.type;
    } else if (direction == ADBusOutArgument) {
        m->outArguments.push_back(arg);
        m->outArgumentsSignature += arg.type;
    } else {
        assert(false);
    }
}

// ----------------------------------------------------------------------------

const char* ADBusFullMemberSignature(
        const struct ADBusMember*   member,
        enum ADBusArgumentDirection direction,
        int*                        size)
{
    const std::string* sig = NULL;
    if (member->type == ADBusPropertyMember) {
        sig = &member->propertyType;
    } else {
        sig = direction == ADBusInArgument 
            ? &member->inArgumentsSignature 
            : &member->outArgumentsSignature;
    }

    if (size)
        *size = sig->size();
    return sig->c_str();
}

// ----------------------------------------------------------------------------

void ADBusAddAnnotation(struct ADBusMember* m,
        const char* name, int nameSize,
        const char* value, int valueSize)
{
    assert(name && value);

    if (nameSize < 0)
        nameSize = strlen(name);

    if (valueSize < 0)
        valueSize = strlen(value);

    std::string n(name, name + nameSize);
    std::string v(value, value + valueSize);

    m->annotations[n] = v;
}

// ----------------------------------------------------------------------------

void ADBusSetMemberUserData(
        struct ADBusMember*         member,
        struct ADBusUser*           user)
{
    ADBusUserClone(user, &member->user);
}

// ----------------------------------------------------------------------------

const struct ADBusUser* ADBusMemberUserData(
        const struct ADBusMember*   member)
{
    return &member->user;
}

// ----------------------------------------------------------------------------

void ADBusSetMethodCallback(
        struct ADBusMember* m,
        ADBusMemberCallback callback)
{
    LOGD("ADBusSetMethodCallback");
    m->methodCallback     = callback;
}

// ----------------------------------------------------------------------------

void ADBusSetPropertyType(
        struct ADBusMember*         member,
        const char*                 type,
        int                         typeSize)
{
    if (typeSize < 0)
        typeSize = strlen(type);

    member->propertyType = std::string(type, type + typeSize);
}

// ----------------------------------------------------------------------------

void ADBusSetPropertyGetCallback(
        struct ADBusMember* m,
        ADBusMemberCallback callback)
{
    m->getPropertyCallback     = callback;
}

// ----------------------------------------------------------------------------

void ADBusSetPropertySetCallback(
        struct ADBusMember* m,
        ADBusMemberCallback callback)
{
    m->setPropertyCallback     = callback;
}

// ----------------------------------------------------------------------------







// ----------------------------------------------------------------------------
// Object management
// ----------------------------------------------------------------------------

struct ADBusObject* ADBusAddObject(struct ADBusConnection* c,
        const char* name,
        int size)
{
    assert(name);

    if (size < 0)
        size = strlen(name);

    std::string n(name, name + size);

    ADBusObject*& o = c->objects[n];
    assert(o == NULL);

    o = new ADBusObject;
    o->name = n;

    struct ADBusUser user;
    ADBusUserInit(&user);
    user.data = (void*) o;

    ADBusBindInterface(o, c->introspectableInterface, &user);

    return o;
}

// ----------------------------------------------------------------------------

struct ADBusObject* ADBusGetObject(const struct ADBusConnection* c,
        const char* name,
        int size)
{
    assert(name);

    if (size < 0)
        size = strlen(name);

    std::string n(name, name + size);

    ADBusConnection::Objects::const_iterator oi = c->objects.find(n);

    return oi == c->objects.end() ? NULL : oi->second;
}

// ----------------------------------------------------------------------------

void ADBusRemoveObject(
        struct ADBusConnection* c,
        struct ADBusObject*     object)
{
    c->objects.erase(object->name);
}

// ----------------------------------------------------------------------------

void ADBusBindInterface(
        struct ADBusObject*     object,
        struct ADBusInterface*  interface,
        struct ADBusUser*       bindData)
{
    assert(object && interface);

    BoundInterface b;
    b.interface = interface;
    ADBusUserClone(bindData, &b.user);

    object->interfaces[interface->name] = b;
}

// ----------------------------------------------------------------------------

void ADBusSetupMarshallerForSignal(
        struct ADBusObject* object,
        struct ADBusMember* signal,
        struct ADBusMarshaller* marshaller)
{
    assert(object && signal && marshaller);

    const std::string& path   = object->name;
    const std::string& iname  = signal->interface->name;
    const std::string& member = signal->name;

    ADBusClearMarshaller(marshaller);
    ADBusSetMessageType(marshaller, ADBusSignalMessage);
    ADBusSetPath(marshaller, path.c_str(), path.size());
    ADBusSetInterface(marshaller, iname.c_str(), iname.size());
    ADBusSetMember(marshaller, member.c_str(), member.size());
}

// ----------------------------------------------------------------------------

static void ConnectionSendCallback(
        void*           user,
        const uint8_t*  data,
        size_t          len)
{
    struct ADBusConnection* c = (struct ADBusConnection*) user;
    
    assert(c->sendCallback);
    c->sendCallback(
            c,
            &c->sendCallbackData,
            data,
            len);
}

void ADBusConnectionSendMessage(
        struct ADBusConnection*     connection,
        struct ADBusMarshaller*     marshaller)
{
    ADBusSendMessage(marshaller, &ConnectionSendCallback, (void*) connection);
}

// ----------------------------------------------------------------------------










// ----------------------------------------------------------------------------
// Introspection
// ----------------------------------------------------------------------------

int _ADBusIntrospectCallback(
        struct ADBusConnection*     connection,
        const struct ADBusUser*     bindData, // Holds a ADBusObject*
        const struct ADBusMember*   member,
        struct ADBusMessage*        message)
{
    ADBusObject* o = (ADBusObject*) bindData->data;
    ADBusConnection* c = connection;

    int err = ADBusTakeMessageEnd(message);
    if (err) return err;

    std::string out;
    _ADBusIntrospectNode(c, o->name.c_str(), o->name.size(), out);

    ADBusSetupReturnMarshaller(c, message, c->marshaller);

    ADBusSetSignature(c->marshaller, "s", -1);
    ADBusAppendString(c->marshaller, out.c_str(), out.size());

    ADBusConnectionSendMessage(c, c->marshaller);

    return 0;
}

// ----------------------------------------------------------------------------

void _ADBusIntrospectNode(struct ADBusConnection* connection,
        const char* path,
        int size,
        std::string& out)
{
    if (size < 0)
        size = strlen(path);

    std::string p(path, path + size);
    ADBusConnection::Objects::const_iterator oi = connection->objects.find(p);

    assert(oi != connection->objects.end());

    out += "<!DOCTYPE node PUBLIC \"-//freedesktop/DTD D-BUS Object Introspection 1.0//EN\"\n"
        "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
        "<node>\n";

    _ADBusIntrospectInterfaces(oi->second, out);


    // Now we find any child objects
    if (p[p.size() - 1] != '/')
        p += "/";

    while (oi != connection->objects.end())
    {
        const std::string& path = oi->first;
        // Break out once we hit a path that does not begin with p
        if (path.size() < p.size())
            break;
        if (strncmp(path.c_str(), p.c_str(), p.size()) != 0)
            break;

        // Make sure its a direct child of the object by checking for a '/'
        // between the end of p and the end of path
        if (memchr(path.c_str() + p.size(), path.size() - p.size(), '/') != NULL)
            continue;

        out += "\t<node name=\"";
        // We only want the tail name
        out += path.c_str() + p.size();
        out += "\"\n";
    }

    out += "<node>\n";
}

// ----------------------------------------------------------------------------

void _ADBusIntrospectInterfaces(struct ADBusObject* object,
        std::string& out)
{
    ADBusObject::Interfaces::const_iterator ii;
    ADBusInterface::Members::const_iterator mi;
    for (ii = object->interfaces.begin(); ii != object->interfaces.end(); ++ii)
    {
        out += "\t<interface name=\"";
        out += ii->first;
        out += "\">\n";

        ADBusInterface* i = ii->second.interface;

        for (mi = i->members.begin(); mi != i->members.end(); ++mi)
        {
            _ADBusIntrospectMember(mi->second, out);
        }

        out += "\t</interface>\n";
    }
}

// ----------------------------------------------------------------------------

void _ADBusIntrospectMember(struct ADBusMember* member,
        std::string& out)
{
    switch (member->type)
    {
        case ADBusPropertyMember:
            {
                out += "\t\t<property name=\"";
                out += member->name;
                out += "\" type=\"";
                out += member->propertyType;
                out += "\" access=\"";
                if (member->getPropertyCallback && member->setPropertyCallback)
                    out += "readwrite";
                else if (member->getPropertyCallback)
                    out += "read";
                else if (member->setPropertyCallback)
                    out += "write";
                else
                    assert(false);

                if (member->annotations.empty())
                {
                    out += "\"/>\n";
                }
                else
                {
                    out += "\">\n";
                    _ADBusIntrospectAnnotations(member, out);
                    out += "\t\t</property>\n";
                }
            }
            break;
        case ADBusMethodMember:
            {
                out += "\t\t<method name=\"";
                out += member->name;
                out += "\">\n";

                _ADBusIntrospectAnnotations(member, out);
                _ADBusIntrospectArguments(member, out);

                out += "\t\t</method>\n";
            }
            break;
        case ADBusSignalMember:
            {
                out += "\t\t<signal name=\"";
                out += member->name;
                out += "\">\n";

                _ADBusIntrospectAnnotations(member, out);
                _ADBusIntrospectArguments(member, out);

                out += "\t\t</signal>\n";
            }
            break;
        default:
            assert(false);
            break;
    }
}

// ----------------------------------------------------------------------------

void _ADBusIntrospectArguments(struct ADBusMember* m,
        std::string& out)
{
    ADBusMember::Arguments::const_iterator ai;

    for (ai = m->outArguments.begin(); ai != m->outArguments.end(); ++ai)
    {
        out += "\t\t\t<arg type=\"";
        out += ai->type;
        if (!ai->name.empty())
        {
            out += "\" name=\"";
            out += ai->name;
        }
        out += "\" direction=\"out\"/>\n";
    }

    for (ai = m->inArguments.begin(); ai != m->inArguments.end(); ++ai)
    {
        out += "\t\t\t<arg type=\"";
        out += ai->type;
        if (!ai->name.empty())
        {
            out += "\" name=\"";
            out += ai->name;
        }
        out += "\" direction=\"in\"/>\n";
    }

}

// ----------------------------------------------------------------------------

void _ADBusIntrospectAnnotations(struct ADBusMember* member,
        std::string& out)
{
    ADBusMember::Annotations::const_iterator ai;

    for (ai = member->annotations.begin(); ai != member->annotations.end(); ++ai)
    {
        out += "\t\t\t<annotation name=\"";
        out += ai->first;
        out += "\" value=\"";
        out += ai->second;
        out += "\"/>\n";
    }
}

// ----------------------------------------------------------------------------









// ----------------------------------------------------------------------------
// Errors
// ----------------------------------------------------------------------------

void _ADBusSendInternalError(
        struct ADBusConnection* c,
        struct ADBusMessage* m)
{
    ADBusSendError(c, m,
            "nz.co.foobar.ADBus.Error.Internal", -1,
            "An internal error has occurred.", -1);
}

// ----------------------------------------------------------------------------

void _ADBusSendInvalidData(struct ADBusConnection* c,
        struct ADBusMessage* m)
{
    ADBusSendError(c, m,
            "nz.co.foobar.ADBus.Error.InvalidData", -1,
            "Invalid data passed in the message.", -1);
}

// ----------------------------------------------------------------------------

void _ADBusSendInvalidArgument(struct ADBusConnection* c,
        struct ADBusMessage* m)
{
    ADBusSendError(c, m,
            "nz.co.foobar.ADBus.Error.InvalidArgs", -1,
            "Invalid arguments passed to a method call.", -1);
}

// ----------------------------------------------------------------------------

void _ADBusSendInvalidPath(struct ADBusConnection* c,
        struct ADBusMessage* m)
{
    ADBusSendError(c, m,
            "nz.co.foobar.ADBus.Error.ObjectNotFoundl", -1,
            "No object exists for the requested path.", -1);
}

// ----------------------------------------------------------------------------

void _ADBusSendInvalidInterface(struct ADBusConnection* c,
        struct ADBusMessage* m)
{
    ADBusSendError(c, m,
            "nz.co.foobar.ADBus.Error.InterfaceNotFound", -1,
            "The requested path does not export the requested interface.", -1);
}

// ----------------------------------------------------------------------------

void _ADBusSendInvalidMethod(struct ADBusConnection* c,
        struct ADBusMessage* m)
{
    ADBusSendError(c, m,
            "nz.co.foobar.ADBus.Error.MethodNotFound", -1,
            "Method name you invoked isn't known by the object you invoked it on.", -1);
}

// ----------------------------------------------------------------------------

void _ADBusSendInvalidProperty(struct ADBusConnection* c,
        struct ADBusMessage* m)
{
    ADBusSendError(c, m,
            "nz.co.foobar.ADBus.Error.PropertyNotFound", -1,
            "The requested object and interface do not export the requested property.", -1);
}

// ----------------------------------------------------------------------------

void _ADBusSendReadOnlyProperty(struct ADBusConnection* c,
        struct ADBusMessage* m)
{
    ADBusSendError(c, m,
            "nz.co.foobar.ADBus.Error.ReadOnlyProperty", -1,
            "The requested property is read only.", -1);
}

// ----------------------------------------------------------------------------

void _ADBusSendWriteOnlyProperty(struct ADBusConnection* c,
        struct ADBusMessage* m)
{
    ADBusSendError(c, m,
            "nz.co.foobar.ADBus.Error.WriteOnlyProperty", -1,
            "The requested property is write only.", -1);
}

// ----------------------------------------------------------------------------




