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

#define CONNECTION "adbuslua_Connection*"

static int ThrowError(lua_State* L, int errlist);

/* ------------------------------------------------------------------------- */

static struct Connection* CheckConnection(lua_State* L, int index)
{
    struct Connection* c = (struct Connection*) luaL_checkudata(L, index, CONNECTION);
    return c->connection ? c : NULL;
}

static struct Connection* CheckOpenedConnection(lua_State* L, int index)
{
    struct Connection* c = CheckConnection(L, index);
    if (!c) {
        luaL_error(L, "Connection is closed");
        return NULL;
    }
    return c;
}

/* ------------------------------------------------------------------------- */

static int NewConnection(lua_State* L)
{
    struct Connection* c = (struct Connection*) lua_newuserdata(L, sizeof(struct Connection));
    luaL_getmetatable(L, CONNECTION);
    lua_setmetatable(L, -2);

    adbus_ConnectionCallbacks cb = {};
    cb.send_message = &adbusluaI_send;

    c->L            = L;
    c->connection   = adbus_conn_new(&cb, c);
    c->message      = adbus_msg_new();
    c->buf          = adbus_buf_new();

    adbus_conn_setbuffer(c->connection, c->buf);

    lua_newtable(L);
    c->errors = luaL_ref(L, LUA_REGISTRYINDEX);

    return 1;
}

/* ------------------------------------------------------------------------- */

static int CloseConnection(lua_State* L)
{
    struct Connection* c = CheckConnection(L, 1);
    if (!c)
        return 0;

    lua_rawgeti(L, LUA_REGISTRYINDEX, c->errors);
    int errlist = lua_gettop(L);

    adbus_conn_free(c->connection);
    adbus_msg_free(c->message);
    adbus_buf_free(c->buf);
    luaL_unref(L, LUA_REGISTRYINDEX, c->sender);
    luaL_unref(L, LUA_REGISTRYINDEX, c->connect);
    luaL_unref(L, LUA_REGISTRYINDEX, c->errors);

    c->connection = NULL;

    if (lua_objlen(L, errlist) > 0)
        return ThrowError(L, errlist);

    return 0;
}

/* ------------------------------------------------------------------------- */

static int Parse(lua_State* L)
{
    size_t size;
    struct Connection* c = CheckOpenedConnection(L, 1);
    const char* data = luaL_checklstring(L, 2, &size);

    lua_rawgeti(L, LUA_REGISTRYINDEX, c->errors);
    int errlist = lua_gettop(L);

    adbus_buf_append(c->buf, data, size);
    if (adbus_conn_parse(c->connection))
        return luaL_error(L, "Parse error");

    if (lua_objlen(L, errlist) > 0)
        return ThrowError(L, errlist);

    return 0;
}

/* ------------------------------------------------------------------------- */

static int SetSender(lua_State* L)
{
    struct Connection* c = CheckOpenedConnection(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    if (c->sender)
        luaL_unref(L, LUA_REGISTRYINDEX, c->sender);

    lua_pushvalue(L, 2);
    c->sender = luaL_ref(L, LUA_REGISTRYINDEX);

    return 0;
}

/* ------------------------------------------------------------------------- */

static int Connect(lua_State* L)
{
    struct Connection* c = CheckOpenedConnection(L, 1);

    if (c->connect)
        luaL_unref(L, LUA_REGISTRYINDEX, c->connect);

    if (lua_isfunction(L, 2)) {
        lua_pushvalue(L, 2);
        c->connect = luaL_ref(L, LUA_REGISTRYINDEX);
        adbus_conn_connect(c->connection, &adbusluaI_connect, c);
    } else {
        adbus_conn_connect(c->connection, NULL, NULL);
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
    lua_pushinteger(L, adbus_conn_serial(c->connection));
    return 1;
}

/* ------------------------------------------------------------------------- */

static int Send(lua_State* L)
{
    struct Connection* c = CheckOpenedConnection(L, 1);

    if (adbuslua_to_message(L, 2, c->message)) {
        return lua_error(L);
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, c->errors);
    int errlist = lua_gettop(L);

    adbus_msg_send(c->message, c->connection);

    if (lua_objlen(L, errlist) > 0)
        return ThrowError(L, errlist);

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
    lua_State* L = o->L;
    adbus_Connection* c = o->connection->connection;

    assert(L && c);

    if (o->bind)
        adbus_conn_unbind(c, o->bind);
    if (o->match)
        adbus_conn_removematch(c, o->match);
    if (o->reply)
        adbus_conn_removereply(c, o->reply);
    if (o->callback)
        luaL_unref(L, LUA_REGISTRYINDEX, o->callback);
    if (o->object)
        luaL_unref(L, LUA_REGISTRYINDEX, o->object);

    o->L = NULL;
}

static int CloseObject(lua_State* L)
{
    struct Object* o = CheckObject(L, 1);
    if (o->L) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, o->connection->errors);
        int errlist = lua_gettop(L);

        DoCloseObject(o);

        if (lua_objlen(L, errlist) > 0)
            return ThrowError(L, errlist);

    }
    return 0;
}

static void ReleaseObject(void* d)
{
    struct Object* o = (struct Object*) d;
    o->bind = NULL;
    o->reply = NULL;
    o->match = NULL;
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

static int AddMatch(lua_State* L)
{
    int top = lua_gettop(L);
    UNUSED(top);

    struct Connection* c = CheckOpenedConnection(L, 1);

    struct Object* o = NewObject(L);
    o->L = L;
    o->connection = c;

    adbus_Match m;
    adbusluaI_tomatch(L, 2, &m, &o->callback, &o->object, &o->unpack);

    m.callback      = &adbusluaI_callback;
    m.cuser         = o;
    m.release[0]    = &ReleaseObject;
    m.ruser[0]      = o;

    lua_rawgeti(L, LUA_REGISTRYINDEX, c->errors);
    int errlist = lua_gettop(L);

    o->match = adbus_conn_addmatch(c->connection, &m);
    free(m.arguments);

    if (lua_objlen(L, errlist) > 0)
        return ThrowError(L, errlist);

    if (!o->match)
        return luaL_error(L, "Failed to add match");

    lua_remove(L, errlist);

    assert(top + 1 == lua_gettop(L));
    return 1;
}

static int AddReply(lua_State* L)
{
    int top = lua_gettop(L);
    UNUSED(top);

    struct Connection* c = CheckOpenedConnection(L, 1);

    struct Object* o = NewObject(L);
    o->L = L;
    o->connection = c;

    adbus_Reply r;
    adbusluaI_toreply(L, 2, &r, &o->callback, &o->object, &o->unpack);

    r.callback      = &adbusluaI_callback;
    r.cuser         = o;
    r.error         = &adbusluaI_callback;
    r.euser         = o;
    r.release[0]    = &ReleaseObject;
    r.ruser[0]      = o;

    lua_rawgeti(L, LUA_REGISTRYINDEX, c->errors);
    int errlist = lua_gettop(L);

    o->reply = adbus_conn_addreply(c->connection, &r);

    if (lua_objlen(L, errlist) > 0)
        return ThrowError(L, errlist);

    if (!o->reply)
        return luaL_error(L, "Failed to add reply");

    lua_remove(L, errlist);

    assert(top + 1 == lua_gettop(L));
    return 1;
}

static int Bind(lua_State* L)
{
    int top = lua_gettop(L);
    UNUSED(top);

    size_t pathsz;
    struct Connection* c    = CheckOpenedConnection(L, 1);
    const char* path        = luaL_checklstring(L, 2, &pathsz);
    adbus_Interface* i      = adbusluaI_check_interface(L, 3);

    struct Object* o = NewObject(L);
    o->L = L;
    o->connection = c;

    adbus_Bind b;
    adbus_bind_init(&b);
    b.path        = path;
    b.pathSize    = (int) pathsz;
    b.interface   = i;
    b.cuser2      = o;
    b.release[0]  = &ReleaseObject;
    b.ruser[0]    = o;

    lua_rawgeti(L, LUA_REGISTRYINDEX, c->errors);
    int errlist = lua_gettop(L);

    o->bind = adbus_conn_bind(c->connection, &b);

    if (lua_objlen(L, errlist) > 0)
        return ThrowError(L, errlist);

    if (!o->bind)
        return luaL_error(L, "Failed to bind");

    lua_remove(L, errlist);

    assert(top + 1 == lua_gettop(L));
    return 1;
}

/* ------------------------------------------------------------------------- */

static int Traceback (lua_State *L) 
{
    if (!lua_isstring(L, 1))  /* 'message' not a string? */
        return 1;  /* keep it intact */
    lua_getfield(L, LUA_GLOBALSINDEX, "debug");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 1;
    }
    lua_getfield(L, -1, "traceback");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        return 1;
    }
    lua_pushvalue(L, 1);  /* pass error message */
    lua_pushinteger(L, 2);  /* skip this function and traceback */
    lua_call(L, 2, 1);  /* call debug.traceback */
    return 1;
}

static void CollectError(adbus_CbData* d, lua_State* L, int errlist)
{
    if (d)
        adbus_error(d, "nz.co.foobar.LuaError", -1, NULL, -1);
    int errorlen = (int) lua_objlen(L, errlist);
    lua_rawseti(L, errlist, errorlen + 1);
}

static int ThrowError(lua_State* L, int errlist)
{
    int errorlen = (int) lua_objlen(L, errlist);
    int top = lua_gettop(L);
    lua_checkstack(L, 2 * errorlen + 2);
    lua_pushstring(L, "The following errors were collected in callbacks:");
    for (int i = 1; i <= errorlen; i++) {
        lua_pushfstring(L, "\n%d. ", i);
        lua_rawgeti(L, errlist, i);
        lua_pushnil(L);
        lua_rawseti(L, errlist, i);
    }
    lua_pushstring(L, "\n");
    lua_concat(L, lua_gettop(L) - top);
    return lua_error(L);
}

int adbusluaI_send(void* user, adbus_Message* msg)
{
    struct Connection* c = (struct Connection*) user;

    if (!c->sender)
        return -1;

    lua_State* L = c->L;
    int top = lua_gettop(L);

    lua_rawgeti(L, LUA_REGISTRYINDEX, c->errors);
    int errlist = lua_gettop(L);

    lua_pushcfunction(L, &Traceback);
    lua_rawgeti(L, LUA_REGISTRYINDEX, c->sender);
    lua_pushlstring(L, msg->data, msg->size);

    // WARNING: this callback may result in c being closed/freed so 
    // we shouldn't use them after this point

    if (lua_pcall(L, 1, 0, -2)) {
        CollectError(NULL, L, errlist);
        lua_settop(L, top);
        return -1;
    }

    lua_settop(L, top);
    return (int) msg->size;
}

void adbusluaI_connect(void* user)
{
    struct Connection* c = (struct Connection*) user;
    lua_State* L = c->L;
    int top = lua_gettop(L);

    lua_rawgeti(L, LUA_REGISTRYINDEX, c->errors);
    int errlist = lua_gettop(L);

    lua_pushcfunction(L, &Traceback);
    lua_rawgeti(L, LUA_REGISTRYINDEX, c->connect);
    lua_pushstring(L, adbus_conn_uniquename(c->connection, NULL));

    // WARNING: this callback may result in c being closed/freed so 
    // we shouldn't use them after this point

    if (lua_pcall(L, 1, 0, -2)) {
        CollectError(NULL, L, errlist);
    }

    lua_settop(L, top);
}

int adbusluaI_callback(adbus_CbData* d)
{
    struct Object* o = (struct Object*) d->user1;

    lua_State* L = o->L;

    int top = lua_gettop(L);

    lua_rawgeti(L, LUA_REGISTRYINDEX, o->connection->errors);
    int errlist = lua_gettop(L);

    lua_pushcfunction(L, &Traceback);
    lua_rawgeti(L, LUA_REGISTRYINDEX, o->callback); 

    int fun = lua_gettop(L);

    if (o->object)
        lua_rawgeti(L, LUA_REGISTRYINDEX, o->object);

    if (adbuslua_push_message(L, d->msg, o->unpack)) {
        lua_settop(L, top);
        return -1;
    }

    // WARNING: this callback may result in o being closed/freed
    // so we shouldn't use it after this point

    if (lua_pcall(L, lua_gettop(L) - fun, 0, fun - 1)) {
        CollectError(d, L, errlist);
        lua_settop(L, top);
        return 0;
    }

    lua_settop(L, top);
    return 0;
}

int adbusluaI_method(adbus_CbData* d)
{
    struct Member* m = (struct Member*) d->user1;
    struct Object* o = (struct Object*) d->user2;

    lua_State* L = o->L;
    int top = lua_gettop(L);

    // Check the signature
    
    lua_rawgeti(L, LUA_REGISTRYINDEX, m->argsig);
    size_t argsz;
    const char* argsig = lua_tolstring(L, -1, &argsz);
    if (argsz != d->msg->signatureSize || memcmp(argsig, d->msg->signature, argsz) != 0) {
        lua_settop(L, top);
        return -1;
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, m->retsig);
    const char* retsig = lua_tostring(L, -1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, o->connection->errors);
    int errlist = lua_gettop(L);

    // Push the function and object

    lua_pushcfunction(L, &Traceback);
    lua_rawgeti(L, LUA_REGISTRYINDEX, m->method);

    int fun = lua_gettop(L);

    if (o->object)
        lua_rawgeti(L, LUA_REGISTRYINDEX, o->object);

    // Push the message

    if (adbuslua_push_message(L, d->msg, m->unpack)) {
        lua_settop(L, top);
        return -1;
    }

    int rets;
    if (d->ret && m->unpack && retsig) {
        rets = LUA_MULTRET;
    } else if (d->ret && retsig) {
        rets = 1;
    } else {
        rets = 0;
    }

    // WARNING: this callback may result in m and/or o being closed/freed
    // so we shouldn't use them after this point

    if (lua_pcall(L, lua_gettop(L) - fun, rets, fun - 1)) {
        CollectError(d, L, errlist);
        lua_settop(L, top);
        return 0;
    }

    if (rets == LUA_MULTRET) {
        if (adbuslua_to_message_unpacked(L, fun, lua_gettop(L), retsig, -1, d->ret)) {
            CollectError(d, L, errlist);
            lua_settop(L, top);
            return 0;
        }
    } else if (rets == 1) {
        if (adbuslua_to_message(L, -1, d->ret)) {
            CollectError(d, L, errlist);
            lua_settop(L, top);
            return 0;
        }
    }

    lua_settop(L, top);
    return 0;
}


int adbusluaI_getproperty(adbus_CbData* d)
{
    struct Member* m = (struct Member*) d->user1;
    struct Object* o = (struct Object*) d->user2;

    lua_State* L = o->L;
    int top = lua_gettop(L);

    // The signature has already been checked by the caller

    lua_rawgeti(L, LUA_REGISTRYINDEX, o->connection->errors);
    int errlist = lua_gettop(L);

    // Push the function and object

    lua_pushcfunction(L, &Traceback);
    lua_rawgeti(L, LUA_REGISTRYINDEX, m->getter);

    int fun = lua_gettop(L);

    if (o->object)
        lua_rawgeti(L, LUA_REGISTRYINDEX, o->object);

    // WARNING: this callback may result in m and/or o being closed/freed
    // so we shouldn't use them after this point

    if (lua_pcall(L, lua_gettop(L) - fun, 1, fun - 1)) {
        CollectError(d, L, errlist);
        lua_settop(L, top);
        return 0;
    } 

    if (adbuslua_to_argument(L, -1, d->getprop)) {
        CollectError(d, L, errlist);
        lua_settop(L, top);
        return 0;
    }

    lua_settop(L, top);
    return 0;
}


int adbusluaI_setproperty(adbus_CbData* d)
{
    struct Member* m = (struct Member*) d->user1;
    struct Object* o = (struct Object*) d->user2;

    lua_State* L = o->L;
    int top = lua_gettop(L);

    // The signature has already been checked by the caller

    lua_rawgeti(L, LUA_REGISTRYINDEX, o->connection->errors);
    int errlist = lua_gettop(L);

    // Push the function and object

    lua_pushcfunction(L, &Traceback);
    lua_rawgeti(L, LUA_REGISTRYINDEX, m->setter);

    int fun = lua_gettop(L);

    if (o->object)
        lua_rawgeti(L, LUA_REGISTRYINDEX, o->object);

    if (adbuslua_push_argument(L, &d->setprop)) {
        lua_settop(L, top);
        return -1;
    }

    // WARNING: this callback may result in m and/or o being closed/freed
    // so we shouldn't use them after this point

    if (lua_pcall(L, lua_gettop(L) - fun, 0, fun - 1)) {
        CollectError(d, L, errlist);
        lua_settop(L, top);
        return 0;
    } 

    lua_settop(L, top);
    return 0;
}



/* ------------------------------------------------------------------------- */

static const luaL_Reg reg[] = {
    { "new", &NewConnection },
    //{ "__gc", &CloseConnection },
    { "close", &CloseConnection },
    { "parse", &Parse },
    { "set_sender", &SetSender },
    { "connect_to_bus", &Connect },
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


