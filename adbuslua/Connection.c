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

#include "Data.h"
#include "Interface.h"
#include "Message.h"

#include <assert.h>

// ----------------------------------------------------------------------------

int LADBusCreateConnection(lua_State* L)
{
    int argnum = lua_gettop(L);
    struct LADBusConnection* c = LADBusPushNewConnection(L);
    c->connection = ADBusCreateConnection();
    c->marshaller = ADBusCreateMarshaller();

    assert(lua_gettop(L) == argnum + 1);
    return 1;
}

// ----------------------------------------------------------------------------

int LADBusFreeConnection(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    ADBusFreeConnection(c->connection);
    ADBusFreeMarshaller(c->marshaller);
    return 0;
}

// ----------------------------------------------------------------------------

int LADBusParse(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    size_t size;
    const char* data = luaL_checklstring(L, 2, &size);

    ADBusConnectionParse(c->connection, (const uint8_t*) data, size);
    return 0;
}

// ----------------------------------------------------------------------------

static void SendCallback(
        struct ADBusConnection* connection,
        struct ADBusUser* user,
        const uint8_t* data,
        size_t len)
{
    const struct LADBusData* d = LADBusCheckData(user);
    lua_rawgeti(d->L, LUA_REGISTRYINDEX, d->ref[0]);
    lua_pushlstring(d->L, (const char*) data, len);
    lua_call(d->L, 1, 0);
}

int LADBusSetConnectionSendCallback(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    struct LADBusData data;
    memset(&data, 0, sizeof(struct LADBusData));

    data.L      = L;
    lua_pushvalue(L, 2);
    data.ref[0] = luaL_ref(L, LUA_REGISTRYINDEX);

    struct ADBusUser user;
    LADBusSetupData(&data, &user);

    ADBusSetConnectionSendCallback(c->connection, &SendCallback, &user);

    return 0;
}

// ----------------------------------------------------------------------------

static void ConnectToBusCallback(
        struct ADBusConnection* connection,
        struct ADBusUser* user)
{
    const struct LADBusData* d = LADBusCheckData(user);
    lua_rawgeti(d->L, LUA_REGISTRYINDEX, d->ref[0]);
    lua_call(d->L, 0, 0);
}

int LADBusConnectToBus(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);

    if (lua_isfunction(L, 2)) {
        struct LADBusData data;
        memset(&data, 0, sizeof(struct LADBusData));
        data.L      = L;
        lua_pushvalue(L, 2);
        data.ref[0] = luaL_ref(L, LUA_REGISTRYINDEX);

        struct ADBusUser user;
        LADBusSetupData(&data, &user);
        ADBusConnectToBus(c->connection, &ConnectToBusCallback, &user);
    } else {
        ADBusConnectToBus(c->connection, NULL, NULL);
    }

    return 0;
}

// ----------------------------------------------------------------------------

int LADBusIsConnectedToBus(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    unsigned int connected = ADBusIsConnectedToBus(c->connection);
    lua_pushboolean(L, connected);
    return 1;
}

// ----------------------------------------------------------------------------

int LADBusUniqueServiceName(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    int size;
    const char* name = ADBusGetUniqueServiceName(c->connection, &size);
    if (!name)
        return 0;

    lua_pushlstring(L, name, size);
    return 1;
}

// ----------------------------------------------------------------------------

int LADBusNextSerial(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    int serial = ADBusNextSerial(c->connection);
    lua_pushinteger(L, serial);
    return 1;
}

// ----------------------------------------------------------------------------

int LADBusSendError(lua_State* L)
{
    size_t errorNameSize, errorMessageSize, destinationSize;

    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    const char* errorName    = luaL_checklstring(L, 3, &errorNameSize);
    const char* errorMessage = lua_tolstring(L, 4, &errorMessageSize);

    lua_getfield(L, 2, "serial");
    int serial = luaL_checkinteger(L, -1);

    lua_getfield(L, 2, "sender");
    const char* destination = luaL_checklstring(L, -1, &destinationSize);

    ADBusSendErrorExpanded(c->connection, serial,
            destination, destinationSize,
            errorName, errorNameSize,
            errorMessage, errorMessageSize);

    return 0;
}

// ----------------------------------------------------------------------------

int LADBusSendReply(lua_State* L)
{
    size_t destinationSize, memberSize;
    int signatureSize;

    luaL_checktype(L, 1, LUA_TTABLE); // original message
    luaL_checktype(L, 2, LUA_TTABLE); // response

    lua_getfield(L, 1, "serial");
    int serial = luaL_checkinteger(L, -1);
    lua_getfield(L, 1, "sender");
    const char* destination = luaL_checklstring(L, -1, &destinationSize);
    lua_getfield(L, 1, "member");
    const char* memberstr = luaL_checklstring(L, -1, &memberSize);

    lua_getfield(L, 1, "_interfacedata");
    struct LADBusInterface* interface = LADBusCheckInterface(L, -1);

    struct ADBusMember* member 
        = ADBusGetInterfaceMember(
                interface->interface,
                ADBusMethodMember,
                memberstr,
                memberSize);

    if (!member) {
        return luaL_error(L, "Internal error");
    }

    const char* signature 
        = ADBusFullMemberSignature(member, ADBusOutArgument, &signatureSize);

    lua_getfield(L, 1, "_connectiondata");
    struct LADBusConnection* c = LADBusCheckConnection(L, -1);

    ADBusSetupReturnMarshallerExpanded(
            c->connection,
            serial,
            destination,
            destinationSize,
            c->marshaller);

    LADBusConvertLuaToMessage(L, 2, c->marshaller, signature, signatureSize);

    ADBusConnectionSendMessage(c->connection, c->marshaller);

    return 0;
}

// ----------------------------------------------------------------------------




