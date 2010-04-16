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
#define NEW_CONNECTION "new adbus.connection"

/* ------------------------------------------------------------------------- */

static struct Connection* CheckConnection(lua_State* L, int index)
{ return (struct Connection*) luaL_checkudata(L, index, CONNECTION); }

/* ------------------------------------------------------------------------- */

int adbuslua_push_connection(lua_State* L, adbus_Connection* connection)
{
    struct Connection* c;

    /* Load the adbus lua code */
    lua_getglobal(L, "require");
    lua_pushstring(L, "adbus");
    if (lua_pcall(L, 1, 0, 0))
        return -1;

    /* Get the connection function */
    lua_getglobal(L, "adbuslua_core");
    if (!lua_istable(L, -1)) {
        lua_pushstring(L, "adbuslua_core.new_lua_connection does not exist");
        return -1;
    }

    lua_getfield(L, -1, "new_lua_connection");
    if (!lua_isfunction(L, -1)) {
        lua_pushstring(L, "adbuslua_core.new_lua_connection does not exist");
        return -1;
    }
    /* Remove adbuslua_core table */
    lua_remove(L, -2);

    /* Get the core connection */
    c = (struct Connection*) lua_newuserdata(L, sizeof(struct Connection));
    memset(c, 0, sizeof(struct Connection));
    c->L        = L;
    c->message  = adbus_msg_new();

    luaL_getmetatable(L, CONNECTION);
    lua_setmetatable(L, -2);

    if (lua_pcall(L, 1, 1, 0))
        return -1;

    c->connection = connection;
    adbus_conn_ref(c->connection);

    return 0;
}

/* ------------------------------------------------------------------------- */

static int FreeConnection(lua_State* L)
{
    struct Connection* c = CheckConnection(L, 1);

    assert(c->L == L);

    adbus_msg_free(c->message);

    if (c->connection) {
        adbus_conn_deref(c->connection);
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

adbus_Connection* adbuslua_check_connection(lua_State* L, int index)
{
    struct Connection* c;

    /* Get the core connection object */
    lua_getfield(L, index, "_connection");

    c = CheckConnection(L, -1);

    /* Remove the core connection object */
    lua_remove(L, -2);

    return c->connection;
}

/* ------------------------------------------------------------------------- */

static int NewConnection(lua_State* L)
{
    adbus_Connection* connection = NULL;
    uintptr_t handle = 0;

    const char* addr = luaL_checkstring(L, 1);

    if (strcmp(addr, "default") == 0) {
        connection = adbus_sock_busconnect(ADBUS_DEFAULT_BUS, NULL);
    } else if (strcmp(addr, "session") == 0) {
        connection = adbus_sock_busconnect(ADBUS_SESSION_BUS, NULL);
    } else if (strcmp(addr, "system") == 0) {
        connection = adbus_sock_busconnect(ADBUS_SYSTEM_BUS, NULL);
    } else {
        connection = adbus_sock_busconnect_s(addr, -1, NULL);
    }

    if (!connection) {
        return luaL_error(L, "Failed to connect");
    }

    adbus_conn_ref(connection);

    if (adbus_conn_block(connection, ADBUS_WAIT_FOR_CONNECTED, &handle, -1)) {
        adbus_conn_deref(connection);
        return luaL_error(L, "Failed to connect");
    }

    if (adbuslua_push_connection(L, connection)) {
        adbus_conn_deref(connection);
        return lua_error(L);
    }

    adbus_conn_deref(connection);
    return 1;
}

/* ------------------------------------------------------------------------- */

#define BLOCK "adbuslua block"
static int Block(lua_State* L)
{
    uintptr_t* block;
    struct Connection* c = CheckConnection(L, 1);

    block = (uintptr_t*) lua_newuserdata(L, sizeof(uintptr_t));
    luaL_getmetatable(L, BLOCK);
    lua_setmetatable(L, -2);

    lua_setfield(L, 2, "_blockhandle");
    *block = 0;

    if (adbus_conn_block(c->connection, ADBUS_BLOCK, block, INT_MAX)) {
        return luaL_error(L, "Failed to block");
    }

    return 0;
}

static int Unblock(lua_State* L)
{
    uintptr_t* block;
    struct Connection* c = CheckConnection(L, 1);

    lua_getfield(L, 2, "_blockhandle");
    block = (uintptr_t*) luaL_checkudata(L, -1, BLOCK);

    if (adbus_conn_block(c->connection, ADBUS_UNBLOCK, block, -1)) {
        return luaL_error(L, "Failed to unblock");
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

static int IsConnected(lua_State* L)
{
    struct Connection* c = CheckConnection(L, 1);
    lua_pushboolean(L, adbus_conn_isconnected(c->connection));
    return 1;
}

/* ------------------------------------------------------------------------- */

static int UniqueName(lua_State* L)
{
    struct Connection* c = CheckConnection(L, 1);

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
    struct Connection* c = CheckConnection(L, 1);
    lua_pushnumber(L, adbus_conn_serial(c->connection));
    return 1;
}

/* ------------------------------------------------------------------------- */

static int Send(lua_State* L)
{
    struct Connection* c = CheckConnection(L, 1);

    if (adbuslua_to_message(L, 2, c->message)) {
        return lua_error(L);
    }

    adbus_msg_send(c->message, c->connection);

    return 0;
}

/* ------------------------------------------------------------------------- */

#define OBJECT "adbuslua_Object"

static struct Object* NewObject(lua_State* L, int luaconnection)
{
    struct Object *o, **po;
    struct Connection* c;

    lua_getfield(L, luaconnection, "_connection");
    c = CheckConnection(L, -1);
    lua_remove(L, -1);

    o               = NEW(struct Object);
    o->connref      = LUA_NOREF;
    o->callback     = LUA_NOREF;
    o->object       = LUA_NOREF;
    o->user         = LUA_NOREF;
    o->L            = L;
    o->c            = c->connection;

    adbus_conn_ref(o->c);

    lua_pushvalue(L, luaconnection);
    o->connref = luaL_ref(L, LUA_REGISTRYINDEX);

    po  = (struct Object**) lua_newuserdata(L, sizeof(struct Object*));
    *po = o;

    luaL_getmetatable(L, OBJECT);
    lua_setmetatable(L, -2);

    return o;
}

/* Called on the connection thread if the connection still exists - #4 */
static void CloseObjectServices(void* u)
{
    struct Object* o = (struct Object*) u;
    adbus_conn_removematch(o->c, o->match);
    adbus_conn_removereply(o->c, o->reply);
    adbus_conn_unbind(o->c, o->bind);
}

/* Called on the lua thread (if it still exists, otherwise some random thread) - #5 */
static void FreeObject(void* u)
{
    struct Object* o = (struct Object*) u;
    adbus_conn_deref(o->c);
    free(o);
}

/* Called on the connection thread (if it still exists, otherwise some random
 * thread) - #4 */
static void PingBack(void* u)
{
    struct Object* o = (struct Object*) u;
    /* Ping back to the lua thread */
    if (o->proxy) {
        o->proxy(o->puser, NULL, &FreeObject, o);
    } else {
        FreeObject(o);
    }
}

/* Called on the local thread by lua - #3 */
static int CloseObject(lua_State* L)
{
    struct Object** po = (struct Object**) luaL_checkudata(L, 1, OBJECT);
    struct Object* o = *po;

    assert(o->L == L);

    /* Stop any further callbacks */
    o->L = NULL;

    /* Unref lua data so the lua vm can die. Note o->c still holds a valid
     * connection reference for the connection ping, which won't be unref'd
     * until the object is destroyed (#5).
     */
    luaL_unref(L, LUA_REGISTRYINDEX, o->callback);
    luaL_unref(L, LUA_REGISTRYINDEX, o->object);
    luaL_unref(L, LUA_REGISTRYINDEX, o->user);
    luaL_unref(L, LUA_REGISTRYINDEX, o->connref);

    /* Ping the connection thread to clear out services (#4), this will then
     * call back FreeObject (#5) to free the object itself */
    adbus_conn_proxy(o->c, &CloseObjectServices, &PingBack, o);

    return 0;
}

/* Called on the connection thread */
static void ReleaseObject(void* d)
{ 
    struct Object* o = (struct Object*) d;
    /* Unset the service pointers so that the unregistrations in
     * CloseConnectionServices are noop'ed.
     */
    o->match = NULL;
    o->reply = NULL;
    o->bind  = NULL;
    /* The actual object will be freed when lua garbage collects it */
}

static const luaL_Reg objectreg[] = {
    { "__gc", &CloseObject },
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

static void ProxiedAddMatch(void* u)
{
    struct Object* o = (struct Object*) u;
    o->match = adbus_conn_addmatch(o->c, &o->u.match);
}

static void FreeProxiedAddMatch(void* u)
{
    size_t i;
    struct Object* o    = (struct Object*) u;
    adbus_Match* m      = &o->u.match;

    free((char*) m->sender);
    free((char*) m->destination);
    free((char*) m->interface);
    free((char*) m->path);
    free((char*) m->member);
    free((char*) m->error);

    for (i = 0; i < m->argumentsSize; i++) {
        free((char*) m->arguments[i].value);
    }
    free(m->arguments);
}

static int AddMatch(lua_State* L)
{
    struct Object* o        = NewObject(L, 1);
    adbus_Match* m          = &o->u.match;

    luaL_checktype(L, 2, LUA_TFUNCTION);
    luaL_checktype(L, 3, LUA_TTABLE);

    lua_pushvalue(L, 2);
    o->callback = luaL_ref(L, LUA_REGISTRYINDEX);

    adbus_match_init(m);

    lua_getfield(L, 3, "type");
    if (lua_isstring(L, -1)) {
        const char* typestr = lua_tostring(L, -1);
        if (strcmp(typestr, "method") == 0) {
            m->type = ADBUS_MSG_METHOD;
        } else if (strcmp(typestr, "return") == 0) {
            m->type = ADBUS_MSG_RETURN;
        } else if (strcmp(typestr, "error") == 0) {
            m->type = ADBUS_MSG_ERROR;
        } else if (strcmp(typestr, "signal") == 0) {
            m->type = ADBUS_MSG_SIGNAL;
        } else {
            return luaL_error(L, "Invalid message type %s", typestr);
        }
    } else if (!lua_isnil(L, -1)) {
        return luaL_error(L, "Expected a string for 'type'");
    }
    lua_pop(L, 1);

    lua_getfield(L, 3, "add_to_bus");
    if (lua_isboolean(L, -1)) {
		m->addToBus = lua_toboolean(L, -1) ? ADBUS_TRUE : ADBUS_FALSE;
    } else if (!lua_isnil(L, -1)) {
        return luaL_error(L, "Expected a boolean for 'reply_serial'");
    }
    lua_pop(L, 1);

    lua_getfield(L, 3, "reply_serial");
    if (lua_isnumber(L, -1)) {
        m->replySerial = (uint32_t) lua_tonumber(L, -1);
    } else if (!lua_isnil(L, -1)) {
        return luaL_error(L, "Expected a number for 'reply_serial'");
    }
    lua_pop(L, 1);

    GetStringHeader(L, "sender", &m->sender, &m->senderSize);
    GetStringHeader(L, "destination", &m->destination, &m->destinationSize);
    GetStringHeader(L, "interface", &m->interface, &m->interfaceSize);
    GetStringHeader(L, "path", &m->path, &m->pathSize);
    GetStringHeader(L, "member", &m->member, &m->memberSize);
    GetStringHeader(L, "error", &m->error, &m->errorSize);

    lua_getfield(L, 3, "n");
    if (lua_isnumber(L, -1)) {
        m->argumentsSize = (size_t) lua_tonumber(L, -1);
    } else if (!lua_isnil(L, -1)) {
        return luaL_error(L, "Expected a number for 'n'");
    }
    lua_pop(L, 1);

    if (m->argumentsSize > 0) {
        int i;

        m->arguments = (adbus_Argument*) malloc(sizeof(adbus_Argument) * m->argumentsSize);
        adbus_arg_init(m->arguments, m->argumentsSize);

        for (i = 0; i < (int) m->argumentsSize; i++) {
            lua_rawgeti(L, 3, i + 1);
            if (lua_isstring(L, -1)) {
                size_t sz;
                m->arguments[i].value = lua_tolstring(L, -1, &sz);
                m->arguments[i].size  = (int) sz;
            } else if (!lua_isnil(L, -1)) {
                free(m->arguments);
                return luaL_error(L, "Expected a string for '%d'", i + 1);
            }
            lua_pop(L, 1);
        }
    }

    adbus_conn_getproxy(o->c, &o->proxy, &o->puser);

    /* ReleaseObject is called on the connection thread */

    m->callback             = &adbusluaI_callback;
    m->cuser                = o;
    m->release[0]           = &ReleaseObject;
    m->ruser[0]             = o;
    m->proxy                = o->proxy;
    m->puser                = o->puser;

    if (adbus_conn_shouldproxy(o->c)) {
        size_t i;

        /* Dup the data in the match reg - freed in FreeProxiedAddMatch */
        m->sender = adbusluaI_strndup(m->sender, m->senderSize);
        m->destination = adbusluaI_strndup(m->destination, m->destinationSize);
        m->interface = adbusluaI_strndup(m->interface, m->interfaceSize);
        m->path = adbusluaI_strndup(m->path, m->pathSize);
        m->member = adbusluaI_strndup(m->member, m->memberSize);
        m->error = adbusluaI_strndup(m->error, m->errorSize);

        for (i = 0; i < m->argumentsSize; i++) {
            m->arguments[i].value = adbusluaI_strndup(m->arguments[i].value, m->arguments[i].size);
        }

        /* m->arguments is also freed by FreeProxiedAddMatch */

        adbus_conn_proxy(o->c, &ProxiedAddMatch, &FreeProxiedAddMatch, o);

    } else {
        o->match = adbus_conn_addmatch(o->c, m);
        free(m->arguments);

        if (!o->match) {
            return luaL_error(L, "Failed to add match");
        }
    }

    return 1;
}

/* ------------------------------------------------------------------------- */

static void ProxiedAddReply(void* u)
{
    struct Object* o = (struct Object*) u;
    o->reply = adbus_conn_addreply(o->c, &o->u.reply);
}

static void FreeProxiedAddReply(void* u)
{
    struct Object* o    = (struct Object*) u;
    adbus_Reply* r      = &o->u.reply;

    free((char*) r->remote);
}

static int AddReply(lua_State* L)
{
    size_t remotesz;

    const char* remote      = luaL_checklstring(L, 2, &remotesz);
    lua_Number serial       = luaL_checknumber(L, 3);
    struct Object* o        = NewObject(L, 1);
    adbus_Reply* r          = &o->u.reply;

    luaL_checktype(L, 4, LUA_TFUNCTION);
    lua_pushvalue(L, 4);
    o->callback = luaL_ref(L, LUA_REGISTRYINDEX);

    adbus_reply_init(r);

    r->remote               = remote;
    r->remoteSize           = (int) remotesz;
    r->serial               = (uint32_t) serial;

    adbus_conn_getproxy(o->c, &o->proxy, &o->puser);

    /* ReleaseObject is called on the connection thread */

    r->callback             = &adbusluaI_callback;
    r->cuser                = o;
    r->error                = &adbusluaI_callback;
    r->euser                = o;
    r->release[0]           = &ReleaseObject;
    r->ruser[0]             = o;
    r->proxy                = o->proxy;
    r->puser                = o->puser;

    if (adbus_conn_shouldproxy(o->c)) {
        /* Dup the data in the reply reg - freed in FreeProxiedAddReply */
        r->remote = adbusluaI_strndup(r->remote, r->remoteSize);

        adbus_conn_proxy(o->c, &ProxiedAddReply, &FreeProxiedAddReply, o);

    } else {
        o->reply = adbus_conn_addreply(o->c, r);
        if (!o->reply) {
            return luaL_error(L, "Failed to add reply");
        }

    }

    assert(lua_gettop(L) == 5);

    return 1;
}

/* ------------------------------------------------------------------------- */

static void ProxiedBind(void* u)
{
    struct Object* o = (struct Object*) u;
    o->bind = adbus_conn_bind(o->c, &o->u.bind);
}

static void FreeProxiedBind(void* u)
{
    struct Object* o    = (struct Object*) u;
    adbus_Bind* b       = &o->u.bind;

    adbus_iface_deref(b->interface);
    free((char*) b->path);
}

static int Bind(lua_State* L)
{
    size_t pathsz;

    const char* path        = luaL_checklstring(L, 4, &pathsz);
    adbus_Interface* i      = adbusluaI_tointerface(L, 5);
    struct Object* o        = NewObject(L, 1);
    adbus_Bind* b           = &o->u.bind;

    lua_pushvalue(L, 2);
    o->object = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, 3);
    o->user = luaL_ref(L, LUA_REGISTRYINDEX);

    adbus_bind_init(b);

    adbus_conn_getproxy(o->c, &o->proxy, &o->puser);

    /* ReleaseObject is called on the connection thread */

    b->path                 = path;
    b->pathSize             = (int) pathsz;
    b->interface            = i;
    b->cuser2               = o;
    b->release[0]           = &ReleaseObject;
    b->ruser[0]             = o;
    b->proxy                = o->proxy;
    b->puser                = o->puser;


    if (adbus_conn_shouldproxy(o->c)) {
        /* Dup the data in the bind reg - freed in FreeProxiedBind */
        b->path = adbusluaI_strndup(b->path, b->pathSize);
        adbus_iface_ref(b->interface);

        adbus_conn_proxy(o->c, &ProxiedBind, &FreeProxiedBind, o);

    } else {
        o->bind = adbus_conn_bind(o->c, b);
        if (!o->bind) {
            return luaL_error(L, "Failed to bind");
        }
    }


    return 1;
}

/* ------------------------------------------------------------------------- */

static int traceback (lua_State *L) {
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

/* Pops message from the top of the stack */
static int OnMessage(struct Object* o, lua_State* L)
{
    int top = lua_gettop(L);

    lua_pushcfunction(L, &traceback);

    /* Call connection:process_message(msg) */
    lua_rawgeti(L, LUA_REGISTRYINDEX, o->connref);
    /* Use rawget as we don't want to call into lua without a pcall */
    lua_pushstring(L, "process_message");
    lua_rawget(L, -2);

    /* Current stack: msg, traceback, connection, function
     * Rearrange to: traceback, function, connection, message 
     */
    lua_insert(L, -2); /* flip connection and function */
    lua_pushvalue(L, top); /* bring message to top */
    lua_remove(L, top);

    /* Stack is now: traceback, function, connection, message */
    if (lua_pcall(L, 2, 0, -4)) {
        /* Use rawget and pcall to avoid erroring */
        lua_pushstring(L, "print");
        lua_rawget(L, LUA_GLOBALSINDEX);
        /* Insert print below the error string */
        lua_insert(L, -2);

        lua_pcall(L, 1, 0, 0);

        lua_settop(L, top - 1);
        return -1;
    }

    lua_settop(L, top - 1);
    return 0;
}

int adbusluaI_callback(adbus_CbData* d)
{
    struct Object* o        = (struct Object*) d->user1;
    lua_State* L            = o->L;

    /* We can get called after the lua vm has gc'd the object but the ping to
     * the connection thread hasn't completed.
     */
    if (!L) {
        return 0;
    }

    /* Get the message */
    if (adbuslua_push_message(L, d->msg)) {
        return -1;
    }
    
    lua_rawgeti(L, LUA_REGISTRYINDEX, o->callback); 
    lua_setfield(L, -2, "callback");

    /* Drop errors */
    OnMessage(o, L);

    return 0;
}

/* ------------------------------------------------------------------------- */

int adbusluaI_method(adbus_CbData* d)
{
    struct Object* o        = (struct Object*) d->user2;
    lua_State* L            = o->L;

    /* We can get called after the lua vm has gc'd the object but the ping to
     * the connection thread hasn't completed.
     */
    if (!L) {
        return 0;
    }

    if (adbuslua_push_message(L, d->msg))
        return -1;

    lua_rawgeti(L, LUA_REGISTRYINDEX, o->object);
    lua_setfield(L, -2, "object");

    lua_rawgeti(L, LUA_REGISTRYINDEX, o->user);
    lua_setfield(L, -2, "user");

    if (OnMessage(o, L)) {
        return adbus_error(d, "nz.co.foobar.adbus.LuaError", -1, NULL, 0);
    }

    /* The process_message in lua will handle the return */
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

    /* We can get called after the lua vm has gc'd the object but the ping to
     * the connection thread hasn't completed.
     */
    if (!L) {
        return 0;
    }

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

    /* We can get called after the lua vm has gc'd the object but the ping to
     * the connection thread hasn't completed.
     */
    if (!L) {
        return 0;
    }

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
    { "__gc", &FreeConnection },
    { "block", &Block },
    { "unblock", &Unblock },
    { "is_connected", &IsConnected },
    { "unique_name", &UniqueName },
    { "serial", &Serial },
    { "send", &Send },
    { "add_match", &AddMatch },
    { "add_reply", &AddReply },
    { "bind", &Bind },
    { NULL, NULL }
};

static const luaL_Reg blockreg[] = {
    {  NULL, NULL }
};

void adbusluaI_reg_connection(lua_State* L)
{
    luaL_newmetatable(L, BLOCK);
    luaL_register(L, NULL, blockreg);
    lua_pop(L, 1);

    luaL_newmetatable(L, CONNECTION);
    luaL_register(L, NULL, reg);
}


