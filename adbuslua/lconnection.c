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
#include <adbus/adbuslua.h>
#include "internal.h"

#include <assert.h>
#include <stdio.h>

#define HANDLE "adbuslua_Connection*"

typedef struct Connection Connection;

struct Connection
{
    adbus_Connection*       connection;
    adbus_Message*          message;
    adbus_Stream*           stream;
    adbus_Bool              free;
    adbus_Bool              debug;
};

/* ------------------------------------------------------------------------- */

adbus_Connection* adbuslua_check_connection(lua_State* L, int index)
{
    Connection* c = (Connection*) luaL_checkudata(L, index, HANDLE);
    return c->connection;
}

/* ------------------------------------------------------------------------- */

static void PushConnection(lua_State* L, adbus_Connection* conn, adbus_Bool debug, adbus_Bool free)
{
    Connection* c = (Connection*) lua_newuserdata(L, sizeof(Connection));
    luaL_getmetatable(L, HANDLE);
    lua_setmetatable(L, -2);
    c->connection   = conn;
    c->free         = free;
    c->message      = adbus_msg_new();
    c->stream       = adbus_stream_new();
    c->debug        = debug;
}

/* ------------------------------------------------------------------------- */

static int NewConnection(lua_State* L)
{
    adbus_Bool debug = lua_type(L, 1) == LUA_TBOOLEAN && lua_toboolean(L, 1);
    PushConnection(L, adbus_conn_new(), debug, 1);
    return 1;
}

/* ------------------------------------------------------------------------- */

int adbuslua_push_connection(lua_State* L, adbus_Connection* connection)
{
    PushConnection(L, connection, 0, 0);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int FreeConnection(lua_State* L)
{
    Connection* c = (Connection*) luaL_checkudata(L, 1, HANDLE);
    if (c->free)
      adbus_conn_free(c->connection);
    adbus_msg_free(c->message);
    adbus_stream_free(c->stream);
    return 0;
}

/* ------------------------------------------------------------------------- */

static const char SendHeader[]    =   "Sending ";
static const char ReceiveHeader[] =   "Received";
static const char BlankHeader[]   = "\n        ";

static const char* strchrnul_(const char* str, int ch)
{
    while (*str != '\0' && *str != ch)
        ++str;
    return str;
}

static void PrintMessage(
        lua_State*              L,
        const char*             header,
        adbus_Message*          message)
{
    int args = 0;
    const char* msg = adbus_msg_summary(message, NULL);

    lua_getglobal(L, "print");
    for (const char* line = msg; *line != '\0';) {
        const char* lineend = strchrnul_(line, '\n');
        lua_checkstack(L, 3);
        lua_pushstring(L, header);
        lua_pushlstring(L, line, lineend - line);
        line = lineend + 1;
        header = BlankHeader;
        args += 2;
    }
    lua_pushstring(L, "\n");
    lua_call(L, args + 1, 0);
}

/* ------------------------------------------------------------------------- */

static int Parse(lua_State* L)
{
    Connection* c = (Connection*) luaL_checkudata(L, 1, HANDLE);
    size_t size;
    const uint8_t* data = (const uint8_t*) luaL_checklstring(L, 2, &size);

    while (size > 0) {
        if (adbus_stream_parse(c->stream, c->message, &data, &size))
            return luaL_error(L, "Parse error");
        if (c->debug)
            PrintMessage(L, ReceiveHeader, c->message);
        if (adbus_conn_dispatch(c->connection, c->message))
            return luaL_error(L, "Dispatch error");
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static void Sender(adbus_Message* message, const adbus_User* user)
{
    const adbuslua_Data* d = (adbuslua_Data*) user;

    if (d->debug) {
        PrintMessage(d->L, SendHeader, message);
    }

    size_t size;
    const uint8_t* data = adbus_msg_data(message, &size);

    adbusluaI_push(d->L, d->callback);
    lua_pushlstring(d->L, (const char*) data, size);
    lua_call(d->L, 1, 0);
}

static int SetSender(lua_State* L)
{
    Connection* c = (Connection*) luaL_checkudata(L, 1, HANDLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    adbuslua_Data* d = adbusluaI_newdata(L);
    d->callback = adbusluaI_ref(L, 2);
    d->debug = c->debug;

    adbus_conn_setsender(c->connection, &Sender, &d->h);

    return 0;
}

/* ------------------------------------------------------------------------- */

static void ConnectCallback(
        const char*             unique,
        size_t                  uniqueSize,
        const adbus_User*       user)
{
    const adbuslua_Data* d = (adbuslua_Data*) user;
    adbusluaI_push(d->L, d->callback);
    lua_pushlstring(d->L, unique, uniqueSize);
    lua_call(d->L, 1, 0);
}

static int Connect(lua_State* L)
{
    adbus_Connection* c = adbuslua_check_connection(L, 1);

    if (lua_isfunction(L, 2)) {
        adbuslua_Data* d = adbusluaI_newdata(L);
        d->callback = adbusluaI_ref(L, 2);

        adbus_conn_connect(c, &ConnectCallback, &d->h);

    } else {
        adbus_conn_connect(c, NULL, NULL);
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

static int IsConnected(lua_State* L)
{
    adbus_Connection* c = adbuslua_check_connection(L, 1);
    lua_pushboolean(L, adbus_conn_isconnected(c));
    return 1;
}

/* ------------------------------------------------------------------------- */

static int UniqueName(lua_State* L)
{
    adbus_Connection* c = adbuslua_check_connection(L, 1);

    size_t size;
    const char* name = adbus_conn_uniquename(c, &size);
    if (!name)
        return 0;

    lua_pushlstring(L, name, size);
    return 1;
}

/* ------------------------------------------------------------------------- */

static int Serial(lua_State* L)
{
    adbus_Connection* c = adbuslua_check_connection(L, 1);
    lua_pushinteger(L, adbus_conn_serial(c));
    return 1;
}

/* ------------------------------------------------------------------------- */

static int Send(lua_State* L)
{
    Connection* c = (Connection*) luaL_checkudata(L, 1, HANDLE);
    adbuslua_check_message(L, 2, c->message);
    adbus_conn_send(c->connection, c->message);

    return 0;
}

/* ------------------------------------------------------------------------- */

static int Emit(lua_State* L)
{
    if (!lua_istable(L, 1))
        return luaL_error(L, "Expected a table of fields");

    // Connection
    lua_getfield(L, 1, "connection");
    if (!lua_isuserdata(L, -1)) {
        return luaL_error(L, "Missing connection field");
    }
    Connection* c = (Connection*) luaL_checkudata(L, -1, HANDLE);


    // Path
    lua_getfield(L, 1, "path");
    if (!lua_isstring(L, -1)) {
        return luaL_error(L, "Missing path field");
    }
    const char* pname = lua_tostring(L, -1);
    adbus_Path* path = adbus_conn_path(c->connection, pname, -1);


    // Interface
    lua_getfield(L, 1, "interface");
    if (!lua_isuserdata(L, 3)) {
        return luaL_error(L, "Missing interface field");
    }
    adbus_Interface* iface = adbuslua_check_interface(L, 3);


    // Member
    lua_getfield(L, 1, "member");
    if (!lua_isstring(L, -1)) {
        return luaL_error(L, "Missing member field");
    }
    const char* mname = lua_tostring(L, -1);
    adbus_Member* mbr = adbus_iface_signal(iface, mname, -1);
    if (mbr == NULL) {
        return luaL_error(L, "Invalid signal name");
    }


    // Signature
    lua_getfield(L, 1, "signature");
    if (!lua_isstring(L, -1)) {
        return luaL_error(L, "Missing signature field");
    }
    const char* sig = lua_tostring(L, -1);


    adbus_setup_signal(c->message, path, mbr);
    adbus_msg_append(c->message, sig, -1);

    adbus_Buffer* b = adbus_msg_buffer(c->message);
    int args = lua_objlen(L, 1);
    for (int i = 1; i <= args; ++i) {
        if (adbuslua_check_argument(L, i, NULL, 0, b)) {
            return luaL_error(L, "Error on marshalling arguments.");
        }
    }

    adbus_conn_send(c->connection, c->message);

    lua_pop(L, 5); // conn + path + iface + mbr + sig

    return 0;
}

/* ------------------------------------------------------------------------- */

static int AddMatch(lua_State* L)
{
    adbus_Match m;

    adbus_Connection* c = adbuslua_check_connection(L, 1);
    adbuslua_check_match(L, 2, &m);

    uint32_t id = adbus_conn_addmatch(c, &m);
    lua_pushnumber(L, id);
    return 1;
}

/* ------------------------------------------------------------------------- */

static int RemoveMatch(lua_State* L)
{
    adbus_Connection* c = adbuslua_check_connection(L, 1);
    uint32_t id = (uint32_t) luaL_checknumber(L, 2);

    adbus_conn_removematch(c, id);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int MatchId(lua_State* L)
{
    adbus_Connection* c = adbuslua_check_connection(L, 1);

    uint32_t id = adbus_conn_matchid(c);
    lua_pushnumber(L, (lua_Number) id);
    return 1;
}

/* ------------------------------------------------------------------------- */

static const luaL_Reg reg[] = {
    { "new", &NewConnection },
    { "__gc", &FreeConnection },
    { "parse", &Parse },
    { "set_sender", &SetSender },
    { "connect_to_bus", &Connect },
    { "is_connected", &IsConnected },
    { "unique_name", &UniqueName },
    { "serial", &Serial },
    { "send", &Send },
    { "emit", &Emit },
    { "add_match", &AddMatch },
    { "remove_match", &RemoveMatch },
    { "match_id", &MatchId },
    { NULL, NULL }
};

void adbusluaI_reg_connection(lua_State* L)
{
    luaL_newmetatable(L, HANDLE);
    luaL_register(L, NULL, reg);
}


