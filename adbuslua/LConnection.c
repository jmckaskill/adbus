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


#include "LConnection.h"

#include "LData.h"
#include "LInterface.h"
#include "LMessage.h"

#include "adbus/CommonMessages.h"
#include "adbus/Interface.h"

#include <assert.h>
#include <stdio.h>

// ----------------------------------------------------------------------------

int LADBusCreateConnection(lua_State* L)
{
    uint debug = lua_type(L, 1) == LUA_TBOOLEAN && lua_toboolean(L, 1);

    int argnum = lua_gettop(L);
    struct LADBusConnection* c = LADBusPushNewConnection(L);
    c->connection = ADBusCreateConnection();
    c->message    = ADBusCreateMessage();
    c->buffer     = ADBusCreateStreamBuffer();
    c->debug      = debug;
    c->existing_connection = 0;

    (void) argnum;
    assert(lua_gettop(L) == argnum + 1);
    return 1;
}

// ----------------------------------------------------------------------------

int LADBusFreeConnection(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    if (!c->existing_connection)
      ADBusFreeConnection(c->connection);
    ADBusFreeMessage(c->message);
    ADBusFreeStreamBuffer(c->buffer);
    return 0;
}

// ----------------------------------------------------------------------------

int LADBusParse(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    size_t size;
    const uint8_t* data = (const uint8_t*) luaL_checklstring(L, 2, &size);

    while (size > 0) {
        int err = ADBusParse(c->buffer, c->message, &data, &size);
        if (err)
            return luaL_error(L, "Parse error %d", err);

        if (c->debug) {
            lua_getglobal(L, "print");
            size_t sz;
            char* msg = ADBusNewMessageSummary(c->message, &sz);
            lua_pushlstring(L, msg, sz);
            lua_pushstring(L, "\n");
            free(msg);
            lua_call(L, 2, 0);
        }

        ADBusDispatch(c->connection, c->message);
    }
    return 0;
}

// ----------------------------------------------------------------------------

static void SendCallback(
        struct ADBusMessage* message,
        const struct ADBusUser* user)
{
    const struct LADBusData* d = (const struct LADBusData*) user;

    if (d->debug) {
        lua_getglobal(d->L, "print");
        size_t sz;
        char* msg = ADBusNewMessageSummary(message, &sz);
        lua_pushlstring(d->L, msg, sz);
        lua_pushstring(d->L, "\n");
        free(msg);
        lua_call(d->L, 2, 0);
    }

    const uint8_t* data;
    size_t size;
    ADBusGetMessageData(message, &data, &size);

    LADBusPushRef(d->L, d->callback);
    lua_pushlstring(d->L, (const char*) data, size);
    lua_call(d->L, 1, 0);
}

int LADBusSetConnectionSendCallback(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    struct LADBusData* data = LADBusCreateData();
    data->L = L;
    data->callback = LADBusGetRef(L, 2);
    data->debug = c->debug;

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




