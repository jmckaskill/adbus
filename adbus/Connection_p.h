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

#include "Connection.h"
#include "User.h"

#include <map>
#include <string>
#include <vector>

// ----------------------------------------------------------------------------

struct MatchRegistration
{
    MatchRegistration()
    {
        ADBusUserInit(&user);
    }

    int id;

    enum ADBusMessageType type;

    uint        addMatchToBusDaemon;
    uint        removeOnFirstMatch;

    uint32_t    replySerial;
    uint        checkReplySerial;

    std::string sender;
    std::string destination;
    std::string interface;
    std::string path;
    std::string member;
    std::string errorName;

    ADBusMatchCallback callback;
    struct ADBusUser   user;

    bool operator==(int id)const
    { return this->id == id; }
};

struct ADBusConnection
{
    ADBusConnection()
    {
        ADBusUserInit(&sendCallbackData);
        ADBusUserInit(&connectCallbackData);
        nextSerial = 0;
        nextWatchId = 0;
        connected = false;
        parser = NULL;
        marshaller = NULL;
        introspectableInterface = NULL;
    }
    ~ADBusConnection();

    typedef std::vector<MatchRegistration>          Registrations;
    typedef std::map<std::string, ADBusObject*>     Objects;

    Registrations       registrations;
    Objects             objects;
    int                 nextSerial;
    int                 nextWatchId;
    bool                connected;
    std::string         uniqueService;

    std::vector<std::string>  services;

    ADBusConnectToBusCallback connectCallback;
    struct ADBusUser          connectCallbackData;

    ADBusConnectionSendCallback sendCallback;
    struct ADBusUser            sendCallbackData;

    ADBusMarshaller*    marshaller;

    ADBusInterface*     introspectableInterface;

    ADBusParser*        parser;
};

// ----------------------------------------------------------------------------

struct Argument
{
    std::string name;
    std::string type;
};

struct ADBusMember
{
    ADBusMember()
    {
        interface = NULL;
        methodCallback = NULL;
        getPropertyCallback = NULL;
        setPropertyCallback = NULL;
        ADBusUserInit(&user);
    }

    ~ADBusMember()
    {
        ADBusUserFree(&user);
    }

    typedef std::map<std::string, std::string>  Annotations;
    typedef std::vector<Argument>               Arguments;

    ADBusInterface*         interface;
    std::string             name;
    enum ADBusMemberType    type;

    std::string             inArgumentsSignature;
    std::string             outArgumentsSignature;
    std::string             propertyType;

    Arguments               inArguments;
    Arguments               outArguments;

    Annotations             annotations;

    ADBusMemberCallback     methodCallback;
    ADBusMemberCallback     getPropertyCallback;
    ADBusMemberCallback     setPropertyCallback;

    ADBusUser               user;  
};

// ----------------------------------------------------------------------------

struct ADBusInterface
{
    ~ADBusInterface()
    {
        Members::iterator ii;
        for (ii = members.begin(); ii != members.end(); ++ii)
            delete ii->second;
    }


    typedef std::map<std::string, ADBusMember*> Members;

    std::string name;
    Members     members;
};

// ----------------------------------------------------------------------------

struct BoundInterface
{
    BoundInterface()
    {
        ADBusUserInit(&user);
    }
    ~BoundInterface()
    {
        ADBusUserFree(&user);
    }
    struct ADBusInterface*  interface;
    struct ADBusUser        user;
};

struct ADBusObject
{
    ADBusObject()
    {
        connection = NULL;
    }
    typedef std::map<std::string, BoundInterface> Interfaces;

    ADBusConnection*  connection;
    std::string       name;
    Interfaces        interfaces;
};

// ----------------------------------------------------------------------------

extern "C"{

// ----------------------------------------------------------------------------

void _ADBusSendInternalError(
        struct ADBusConnection* c,
        struct ADBusMessage* m);

void _ADBusSendInvalidData(
        struct ADBusConnection* c,
        struct ADBusMessage* m);

void _ADBusSendInvalidArgument(
        struct ADBusConnection* c,
        struct ADBusMessage* m);

void _ADBusSendInvalidPath(
        struct ADBusConnection* c,
        struct ADBusMessage* m);

void _ADBusSendInvalidInterface(
        struct ADBusConnection* c,
        struct ADBusMessage* m);

void _ADBusSendInvalidMethod(
        struct ADBusConnection* c,
        struct ADBusMessage* m);

void _ADBusSendInvalidProperty(
        struct ADBusConnection* c,
        struct ADBusMessage* m);

void _ADBusSendReadOnlyProperty(
        struct ADBusConnection* c,
        struct ADBusMessage* m);

void _ADBusSendWriteOnlyProperty(
        struct ADBusConnection* c,
        struct ADBusMessage* m);

// ----------------------------------------------------------------------------

int _ADBusDispatchMessage(
        void* connection,
        struct ADBusMessage* message);

int _ADBusDispatchMethodCall(
        struct ADBusConnection* c,
        struct ADBusMessage* message);

int _ADBusDispatchMatch(
        struct ADBusConnection* c,
        struct ADBusMessage* message);

// ----------------------------------------------------------------------------

int _ADBusIntrospectCallback(
        struct ADBusConnection*     connection,
        const struct ADBusUser*     bindData, // Holds a ADBusObject*
        const struct ADBusMember*   member,
        struct ADBusMessage*        message);

void _ADBusIntrospectNode(
        struct ADBusConnection* connection,
        const char* path,
        int size,
        std::string& out);

void _ADBusIntrospectInterfaces(
        struct ADBusObject* object,
        std::string& out);

void _ADBusIntrospectMember(
        struct ADBusMember* member,
        std::string& out);

void _ADBusIntrospectArguments(
        struct ADBusMember* member,
        std::string& out);

void _ADBusIntrospectAnnotations(
        struct ADBusMember* member,
        std::string& out);

// ----------------------------------------------------------------------------

}
