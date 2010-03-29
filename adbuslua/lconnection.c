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


#define ADBUSLUA_LIBRARY
#include <adbus.h>
#include <adbuslua.h>
#include "internal.h"

#include <assert.h>
#include <stdio.h>

#define CONNECTION "adbuslua_Connection*"

/* ------------------------------------------------------------------------- */

static struct Connection* CheckConnection(lua_State* L, int index)
{ return (struct Connection*) luaL_checkudata(L, index, CONNECTION); }

static struct Connection* CheckOpenedConnection(lua_State* L, int index)
{
    struct Connection* c = CheckConnection(L, index);
    if (c->connection == NULL) {
        luaL_error(L, "Connection is closed");
        return NULL;
    }
    return c;
}

/* ------------------------------------------------------------------------- */

static int NewConnection(lua_State* L)
{
    void* handle = NULL;
    struct Connection* c = (struct Connection*) lua_newuserdata(L, sizeof(struct Connection));
    luaL_getmetatable(L, CONNECTION);
    lua_setmetatable(L, -2);

    c->L            = L;
    c->message      = adbus_msg_new();

    luaL_checktype(L, 2, LUA_TTABLE);
    lua_pushvalue(L, 2);
    c->queue = luaL_ref(L, LUA_REGISTRYINDEX);

    if (lua_isnil(L, 1)) {
        c->connection = adbus_sock_busconnect(ADBUS_DEFAULT_BUS, NULL);
    } else {
        const char* addr = luaL_checkstring(L, 1);

        if (strcmp(addr, "session") == 0) {
            c->connection = adbus_sock_busconnect(ADBUS_SESSION_BUS, NULL);
        } else if (strcmp(addr, "system") == 0) {
            c->connection = adbus_sock_busconnect(ADBUS_SYSTEM_BUS, NULL);
        } else {
            c->connection = adbus_sock_busconnect_s(addr, -1, NULL);
        }
    }

    if (!c->connection || adbus_conn_block(c->connection, ADBUS_WAIT_FOR_CONNECTED, &handle, -1)) {
        return luaL_error(L, "Failed to connect");
    }

    return 1;
}

/* ------------------------------------------------------------------------- */

static int CloseConnection(lua_State* L)
{
    struct Connection* c = CheckConnection(L, 1);

    adbus_conn_free(c->connection);
    adbus_msg_free(c->message);

    c->connection = NULL;

    return 0;
}

/* ------------------------------------------------------------------------- */

static int Process(lua_State* L)
{
    struct Connection* c = CheckOpenedConnection(L, 1);

    /* This is unblocked by adbusluaI_callback and adbusluaI_method */
    if (adbus_conn_block(c->connection, ADBUS_BLOCK, &c->block, -1)) {
        return CloseConnection(L);
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

static int IsConnected(lua_State* L)
{
    struct Connection* c = CheckOpenedConnection(L, 1);
    lua_pushboolean(L, adbus_conn_isconnected(c->connection));
    return 1;
}

/* ------------------------------------------------------------------------- */

static int UniqueName(lua_State* L)
{
    struct Connection* c = CheckOpenedConnection(L, 1);

    size_t size;
    const char* name = adbus_conn_uniquename(c->connection, &size);
    if (!name)
        return 0;

    lua_pushlstring(L, name, size);
    return 1;
}

/* ------------------------------------------------------------------------- */

static int Serial(lua_State* L)
{
    struct Connection* c = CheckOpenedConnection(L, 1);
    lua_pushnumber(L, adbus_conn_serial(c->connection));
    return 1;
}

/* ------------------------------------------------------------------------- */

static int Send(lua_State* L)
{
    struct Connection* c = CheckOpenedConnection(L, 1);

    if (adbuslua_to_message(L, 2, c->message)) {
        return lua_error(L);
    }

    adbus_msg_send(c->message, c->connection);

    return 0;
}

/* ------------------------------------------------------------------------- */

#define OBJECT "adbuslua_Object"

static struct Object* CheckObject(lua_State* L, int index)
{ return (struct Object*) luaL_checkudata(L, index, OBJECT); }

static struct Object* NewObject(lua_State* L)
{
    struct Object* o = (struct Object*) lua_newuserdata(L, sizeof(struct Object));
    memset(o, 0, sizeof(struct Object));
    luaL_getmetatable(L, OBJECT);
    lua_setmetatable(L, -2);
    return o;
}

static void DoCloseObject(struct Object* o)
{
    struct Connection* c = o->connection;
    lua_State* L         = o->L;

    o->L = NULL;
    o->connection = NULL;

    if (L && c && c->connection) {
        if (o->bind)
            adbus_conn_unbind(c->connection, o->bind);
        if (o->match)
            adbus_conn_removematch(c->connection, o->match);
        if (o->reply)
            adbus_conn_removereply(c->connection, o->reply);
        if (o->callback)
            luaL_unref(L, LUA_REGISTRYINDEX, o->callback);
        if (o->object)
            luaL_unref(L, LUA_REGISTRYINDEX, o->object);
        if (o->connref)
            luaL_unref(L, LUA_REGISTRYINDEX, o->connref);
        if (o->queue)
            luaL_unref(L, LUA_REGISTRYINDEX, o->queue);
        if (o->user)
            luaL_unref(L, LUA_REGISTRYINDEX, o->user);
    }
}

static int CloseObject(lua_State* L)
{
    DoCloseObject(CheckObject(L, 1));
    return 0;
}

static void ReleaseObject(void* d)
{ 
    struct Object* o = (struct Object*) d;

    /* Don't call adbus_conn_removereply etc when we are in the release
     * callback for the same service.
     */
    o->bind = NULL;
    o->match = NULL;
    o->reply = NULL;

    DoCloseObject(o); 
}

static const luaL_Reg objectreg[] = {
    { "__gc", &CloseObject },
    { "close", &CloseObject },
    { NULL, NULL },
};

void adbusluaI_reg_object(lua_State* L)
{
    luaL_newmetatable(L, OBJECT);
    luaL_register(L, NULL, objectreg);
}

/* ------------------------------------------------------------------------- */

static void GetStringHeader(lua_State* L, const char* field, const char** mstr, int* msz)
{
    lua_getfield(L, 3, field);
    if (lua_isstring(L, -1)) {
        size_t sz;
        *mstr = lua_tolstring(L, -1, &sz);
        *msz  = (int) sz;
    } else if (!lua_isnil(L, -1)) {
        luaL_error(L, "Expected a string for '%s'", field);
    }
    lua_pop(L, 1);
}

static int AddMatch(lua_State* L)
{
    adbus_Match m;
    struct Object* o;

    struct Connection* c    = CheckOpenedConnection(L, 1);
    o                       = NewObject(L);
    o->connection           = c;
    o->L                    = L;

    adbus_match_init(&m);

    luaL_checktype(L, 2, LUA_TFUNCTION);
    luaL_checktype(L, 3, LUA_TTABLE);

    lua_pushvalue(L, 2);
    o->callback = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_rawgeti(L, LUA_REGISTRYINDEX, c->queue);
    o->queue = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, 1);
    o->connref = luaL_ref(L, LUA_REGISTRYINDEX);

    m.callback              = &adbusluaI_callback;
    m.cuser                 = o;
    m.release[0]            = &ReleaseObject;
    m.ruser[0]              = o;

    lua_getfield(L, 3, "type");
    if (lua_isstring(L, -1)) {
        const char* typestr = lua_tostring(L, -1);
        if (strcmp(typestr, "method") == 0) {
            m.type = ADBUS_MSG_METHOD;
        } else if (strcmp(typestr, "return") == 0) {
            m.type = ADBUS_MSG_RETURN;
        } else if (strcmp(typestr, "error") == 0) {
            m.type = ADBUS_MSG_ERROR;
        } else if (strcmp(typestr, "signal") == 0) {
            m.type = ADBUS_MSG_SIGNAL;
        } else {
            return luaL_error(L, "Invalid message type %s", typestr);
        }
    } else if (!lua_isnil(L, -1)) {
        return luaL_error(L, "Expected a string for 'type'");
    }
    lua_pop(L, 1);

    lua_getfield(L, 3, "add_to_bus");
    if (lua_isboolean(L, -1)) {
        m.addMatchToBusDaemon = lua_toboolean(L, -1);
    } else if (!lua_isnil(L, -1)) {
        return luaL_error(L, "Expected a boolean for 'reply_serial'");
    }
    lua_pop(L, 1);

    lua_getfield(L, 3, "reply_serial");
    if (lua_isnumber(L, -1)) {
        m.replySerial = (uint32_t) lua_tonumber(L, -1);
    } else if (!lua_isnil(L, -1)) {
        return luaL_error(L, "Expected a number for 'reply_serial'");
    }
    lua_pop(L, 1);

    GetStringHeader(L, "sender", &m.sender, &m.senderSize);
    GetStringHeader(L, "destination", &m.destination, &m.destinationSize);
    GetStringHeader(L, "interface", &m.interface, &m.interfaceSize);
    GetStringHeader(L, "path", &m.path, &m.pathSize);
    GetStringHeader(L, "member", &m.member, &m.memberSize);
    GetStringHeader(L, "error", &m.error, &m.errorSize);

    lua_getfield(L, 3, "n");
    if (lua_isnumber(L, -1)) {
        m.argumentsSize = (size_t) lua_tonumber(L, -1);
    } else if (!lua_isnil(L, -1)) {
        return luaL_error(L, "Expected a number for 'n'");
    }
    lua_pop(L, 1);

    if (m.argumentsSize > 0) {
        m.arguments = (adbus_Argument*) malloc(sizeof(adbus_Argument) * m.argumentsSize);
        for (int i = 0; i < (int) m.argumentsSize; i++) {
            lua_rawgeti(L, 3, i + 1);
            if (lua_isstring(L, -1)) {
                size_t sz;
                m.arguments[i].value = lua_tolstring(L, -1, &sz);
                m.arguments[i].size  = (int) sz;
            } else if (!lua_isnil(L, -1)) {
                free(m.arguments);
                return luaL_error(L, "Expected a string for '%d'", i + 1);
            }
            lua_pop(L, 1);
        }
    }

    o->match = adbus_conn_addmatch(c->connection, &m);
    free(m.arguments);

    if (!o->match) {
        return luaL_error(L, "Failed to add match");
    }

    return 1;
}

/* ------------------------------------------------------------------------- */

static int AddReply(lua_State* L)
{
    size_t remotesz;
    adbus_Reply r;
    struct Object* o;

    struct Connection* c    = CheckOpenedConnection(L, 1);
    const char* remote      = luaL_checklstring(L, 2, &remotesz);
    lua_Number serial       = luaL_checknumber(L, 3);
    o                       = NewObject(L);
    o->L                    = L;
    o->connection           = c;

    luaL_checktype(L, 4, LUA_TFUNCTION);
    lua_pushvalue(L, 4);
    o->callback = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_rawgeti(L, LUA_REGISTRYINDEX, c->queue);
    o->queue = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, 1);
    o->connref = luaL_ref(L, LUA_REGISTRYINDEX);

    adbus_reply_init(&r);
    r.callback              = &adbusluaI_callback;
    r.cuser                 = o;
    r.error                 = &adbusluaI_callback;
    r.euser                 = o;
    r.release[0]            = &ReleaseObject;
    r.ruser[0]              = o;
    r.remote                = remote;
    r.remoteSize            = (int) remotesz;
    r.serial                = (uint32_t) serial;

    o->reply = adbus_conn_addreply(c->connection, &r);


    if (!o->reply) {
        return luaL_error(L, "Failed to add reply");
    }

    assert(lua_gettop(L) == 5);

    return 1;
}

/* ------------------------------------------------------------------------- */

static int Bind(lua_State* L)
{
    struct Object* o;
    size_t pathsz;
    adbus_Bind b;

    struct Connection* c    = CheckOpenedConnection(L, 1);
    const char* path        = luaL_checklstring(L, 4, &pathsz);
    adbus_Interface* i      = adbusluaI_tointerface(L, 5);
    o                       = NewObject(L);
    o->connection           = c;
    o->L                    = L;

    lua_pushvalue(L, 2);
    o->object = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, 3);
    o->user = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, 1);
    o->connref = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_rawgeti(L, LUA_REGISTRYINDEX, c->queue);
    o->queue = luaL_ref(L, LUA_REGISTRYINDEX);

    adbus_bind_init(&b);
    b.path        = path;
    b.pathSize    = (int) pathsz;
    b.interface   = i;
    b.cuser2      = o;
    b.release[0]  = &ReleaseObject;
    b.ruser[0]    = o;

    o->bind = adbus_conn_bind(c->connection, &b);

    if (!o->bind) {
        return luaL_error(L, "Failed to bind");
    }

    return 1;
}

/* ------------------------------------------------------------------------- */

int adbusluaI_callback(adbus_CbData* d)
{
    struct Object* o    = (struct Object*) d->user1;
    lua_State* L        = o->L;

    lua_rawgeti(L, LUA_REGISTRYINDEX, o->queue);
    assert(lua_istable(L, -1));

    if (adbuslua_push_message(L, d->msg))
        return -1;
    
    lua_rawgeti(L, LUA_REGISTRYINDEX, o->callback); 
    lua_setfield(L, -2, "callback");

    /* Append message to queue */
    lua_rawseti(L, -2, (int) (lua_objlen(L, -2) + 1));

    adbus_conn_block(o->connection->connection, ADBUS_UNBLOCK, &o->connection->block, -1);

    return 0;
}

/* ------------------------------------------------------------------------- */

int adbusluaI_method(adbus_CbData* d)
{
    struct Object* o    = (struct Object*) d->user2;
    lua_State* L        = o->L;

    lua_rawgeti(L, LUA_REGISTRYINDEX, o->queue);
    assert(lua_istable(L, -1));

    if (adbuslua_push_message(L, d->msg))
        return -1;

    lua_rawgeti(L, LUA_REGISTRYINDEX, o->object);
    lua_setfield(L, -2, "object");

    lua_rawgeti(L, LUA_REGISTRYINDEX, o->user);
    lua_setfield(L, -2, "user");

    /* Append message to queue */
    lua_rawseti(L, -2, (int) (lua_objlen(L, -2) + 1));

    adbus_conn_block(o->connection->connection, ADBUS_UNBLOCK, &o->connection->block, -1);

    /* The message queue in lua will handle the return */
    d->ret = NULL;

    return 0;
}

/* ------------------------------------------------------------------------- */

static int ProtectedGetField(lua_State* L, int index, const char* field)
{
    int ret = 0;
    int top = lua_gettop(L);

    if (index < 0) {
        index += lua_gettop(L) + 1;
    }

    if (lua_type(L, index) == LUA_TTABLE) {
        lua_pushstring(L, field);
        lua_rawget(L, index);
        if (!lua_isnil(L, -1)) {
            goto end;
        }
        lua_pop(L, 1);
    }

    lua_getmetatable(L, index);
    if (!lua_istable(L, -1)) {
        ret = -1;
        goto end;
    }
    lua_pushstring(L, "__index");
    lua_rawget(L, -2);
    lua_remove(L, -2);

    lua_pushvalue(L, index);
    lua_pushstring(L, field);

    /* Pcall will return an error if there is no metamethod */
    ret = lua_pcall(L, 2, 1, 0);

end:
    lua_settop(L, top + 1);
    return ret;
}

static int ProtectedSetField(lua_State* L, int index, const char* field)
{
    int ret = 0;
    int top = lua_gettop(L);

    if (index < 0) {
        index += lua_gettop(L) + 1;
    }

    if (lua_type(L, index) == LUA_TTABLE) {
        lua_pushstring(L, field);
        lua_rawget(L, index);
        if (!lua_isnil(L, -1)) {
            lua_pushstring(L, field);
            lua_pushvalue(L, top);
            lua_rawset(L, index);
            goto end;
        }
        lua_pop(L, 1);
    }

    lua_getmetatable(L, index);
    if (!lua_istable(L, -1)) {
        ret = -1;
        goto end;
    }
    lua_pushstring(L, "__newindex");
    lua_rawget(L, -2);
    lua_remove(L, -2);

    lua_pushvalue(L, index);
    lua_pushvalue(L, top);

    /* Pcall will return an error if there is no metamethod */
    ret = lua_pcall(L, 2, 0, 0);

end:
    lua_settop(L, top - 1);
    return ret;
}

/* ------------------------------------------------------------------------- */

int adbusluaI_getproperty(adbus_CbData* d)
{
    const char* member  = (const char*) d->user1;
    struct Object* o    = (struct Object*) d->user2;
    lua_State* L        = o->L;
    int top             = lua_gettop(L);

    /* We can't call lua_getfield since we need to protect against errors in
     * the metamethod.
     */
    lua_rawgeti(L, LUA_REGISTRYINDEX, o->object);

    if (    ProtectedGetField(L, -1, member) 
        ||  adbuslua_to_argument(L, -1, d->getprop)) 
    {
        lua_settop(L, top);
        return adbus_errorf(d, "nz.co.foobar.adbuslua.LuaError", NULL);
    }

    lua_settop(L, top);
    return 0;
}


int adbusluaI_setproperty(adbus_CbData* d)
{
    const char* member  = (const char*) d->user1;
    struct Object* o    = (struct Object*) d->user2;
    lua_State* L        = o->L;
    int top             = lua_gettop(L);

    /* We can't call lua_setfield since we need to protect against errors in
     * the metamethod.
     */

    lua_rawgeti(L, LUA_REGISTRYINDEX, o->object);
    if (adbuslua_push_argument(L, &d->setprop))
        return -1;

    if (ProtectedSetField(L, -2, member)) {
        lua_settop(L, top);
        return adbus_errorf(d, "nz.co.foobar.adbuslua.LuaError", NULL);
    }

    lua_settop(L, top);
    return 0;
}



/* ------------------------------------------------------------------------- */

static const luaL_Reg reg[] = {
    { "new", &NewConnection },
    { "__gc", &CloseConnection },
    { "process", &Process },
    { "is_connected", &IsConnected },
    { "unique_name", &UniqueName },
    { "serial", &Serial },
    { "send", &Send },
    { "add_match", &AddMatch },
    { "add_reply", &AddReply },
    { "bind", &Bind },
    { NULL, NULL }
};

void adbusluaI_reg_connection(lua_State* L)
{
    luaL_newmetatable(L, CONNECTION);
    luaL_register(L, NULL, reg);
}


