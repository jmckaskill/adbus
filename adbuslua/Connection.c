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

#include "Data.h"
#include "Interface.h"
#include "Message.h"

#include "adbus/CommonMessages.h"
#include "adbus/Interface.h"

#include <assert.h>
#include <stdio.h>

// ----------------------------------------------------------------------------

static void DebugSend(struct ADBusCallDetails* details)
{
    char* msg = ADBusNewMessageSummary(details->message, NULL);
    fprintf(stderr, "Sending: %s\n\n", msg);
    ADBusFreeMessageSummary(msg);
}

static void DebugReceive(struct ADBusCallDetails* details)
{
    char* msg = ADBusNewMessageSummary(details->message, NULL);
    fprintf(stderr, "Received: %s\n\n", msg);
    ADBusFreeMessageSummary(msg);
}

int LADBusCreateConnection(lua_State* L)
{
    int argnum = lua_gettop(L);
    uint debug = (argnum >= 1) ? lua_toboolean(L, 1) : 0;
    struct LADBusConnection* c = LADBusPushNewConnection(L);
    c->connection = ADBusCreateConnection();
    c->unpacker   = ADBusCreateStreamUnpacker();
    c->message    = ADBusCreateMessage();
    c->existing_connection = 0;
    if (debug) {
        ADBusSetSendMessageCallback(c->connection, &DebugSend, NULL);
        ADBusSetReceiveMessageCallback(c->connection, &DebugReceive, NULL);
    }

    assert(lua_gettop(L) == argnum + 1);
    return 1;
}

// ----------------------------------------------------------------------------

int LADBusFreeConnection(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    if (!c->existing_connection)
      ADBusFreeConnection(c->connection);
    ADBusFreeStreamUnpacker(c->unpacker);
    ADBusFreeMessage(c->message);
    return 0;
}

// ----------------------------------------------------------------------------

int LADBusParse(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    size_t size;
    const char* data = luaL_checklstring(L, 2, &size);

    ADBusDispatchData(c->unpacker, c->connection, (const uint8_t*) data, size);
    return 0;
}

// ----------------------------------------------------------------------------

static void SendCallback(
        const struct ADBusUser* user,
        const uint8_t* data,
        size_t len)
{
    const struct LADBusData* d = (const struct LADBusData*) user;
    LADBusPushRef(d->L, d->callback);
    lua_pushlstring(d->L, (const char*) data, len);
    lua_call(d->L, 1, 0);
}

int LADBusSetConnectionSendCallback(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    struct LADBusData* data = LADBusCreateData();
    data->L = L;
    data->callback = LADBusGetRef(L, 2);

    ADBusSetSendCallback(c->connection, &SendCallback, &data->header);

    return 0;
}

// ----------------------------------------------------------------------------

static void ConnectToBusCallback(
        struct ADBusConnection* connection,
        const struct ADBusUser* user)
{
    const struct LADBusData* d = (const struct LADBusData*) user;
    LADBusPushRef(d->L, d->callback);
    lua_pushstring(d->L, ADBusGetUniqueServiceName(connection, NULL));
    lua_call(d->L, 1, 0);
}

int LADBusConnectToBus(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);

    if (lua_isfunction(L, 2)) {
        struct LADBusData* data = LADBusCreateData();
        data->L = L;
        data->callback = LADBusGetRef(L, 2);

        ADBusConnectToBus(c->connection, &ConnectToBusCallback, &data->header);

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
    size_t size;
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

int LADBusSendMessage(lua_State* L)
{
  struct LADBusConnection* c = LADBusCheckConnection(L, 1);
  LADBusMarshallMessage(L, 2, c->message);

  ADBusSendMessage(c->connection, c->message);

  return 0;
}




