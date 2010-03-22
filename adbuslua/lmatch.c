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
#include <adbuslua.h>
#include "internal.h"
#include "dmem/vector.h"

DVECTOR_INIT(Argument, adbus_Argument);

/* ------------------------------------------------------------------------- */

static const char* kMatchFields[] = {
  "type",
  "sender",
  "destination",
  "interface",
  "reply_serial",
  "path",
  "member",
  "error",
  "remove_on_first_match",
  "add_match_to_bus_daemon",
  "unpack_message",
  "callback",
  "object",
  "arguments",
  NULL
};

int adbusluaI_tomatch(
        lua_State*      L,
        int             table,
        adbus_Match*    m,
        int*            callback,
        int*            object,
        adbus_Bool*     unpack)
{
    int top = lua_gettop(L);
    UNUSED(top);

    adbus_match_init(m);

    if (!lua_istable(L, table)){
        return luaL_error(L,
                "Invalid argument - must be a table detailing the match registration.");
    }

    if (adbusluaI_check_fields(L, table, kMatchFields)) {
        return luaL_error(L,
                "Invalid field in match table. Supported fields are 'type', "
                "'sender', 'destination', 'interface', 'reply_serial', 'path', "
                "'member', 'error', 'add_match_to_bus_daemon', 'object', "
                "'callback', and 'unpack_message'.");
    }

    lua_getfield(L, table, "type");
    if (lua_isnil(L, -1)) {
        m->type = ADBUS_MSG_INVALID;
    } else if (lua_isstring(L, -1) && strcmp(lua_tostring(L, -1), "method_call") == 0) {
        m->type = ADBUS_MSG_METHOD;
    } else if (lua_isstring(L, -1) && strcmp(lua_tostring(L, -1), "method_return") == 0) {
        m->type = ADBUS_MSG_RETURN;
    } else if (lua_isstring(L, -1) && strcmp(lua_tostring(L, -1), "error") == 0) {
        m->type = ADBUS_MSG_ERROR;
    } else if (lua_isstring(L, -1) && strcmp(lua_tostring(L, -1), "signal") == 0) {
        m->type = ADBUS_MSG_SIGNAL;
    } else {
        return luaL_error(L,
                "Error in 'type' field - expected 'method_call', "
                "'method_return', 'error', or 'signal'");
    }
    lua_pop(L, 1);

    // Default is to unpack
    *unpack = 1;

    if (    adbusluaI_intField(L, table, "reply_serial", &m->replySerial)
        ||  adbusluaI_stringField(L, table, "sender", &m->sender, &m->senderSize)
        ||  adbusluaI_stringField(L, table, "interface", &m->interface, &m->interfaceSize)
        ||  adbusluaI_stringField(L, table, "destination", &m->destination, &m->destinationSize)
        ||  adbusluaI_stringField(L, table, "path", &m->path, &m->pathSize)
        ||  adbusluaI_stringField(L, table, "member", &m->member, &m->memberSize)
        ||  adbusluaI_stringField(L, table, "error", &m->error, &m->errorSize)
        ||  adbusluaI_boolField(L, table, "add_match_to_bus_daemon", &m->addMatchToBusDaemon)
        ||  adbusluaI_boolField(L, table, "unpack_message", unpack)
        ||  adbusluaI_functionField(L, table, "callback", callback))
    {
        return lua_error(L);
    }

    if (!*callback)
        return luaL_error(L, "Missing 'callback' field - expected a function");

    lua_getfield(L, table, "object");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
    } else {
        *object = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    lua_getfield(L, table, "arguments");
    if (lua_isnil(L, -1)) {
        m->arguments = NULL;
        m->argumentsSize = 0;

    } else if (lua_istable(L, -1)) {
        d_Vector(Argument) vec;
        ZERO(&vec);
        int args = lua_gettop(L);
        lua_pushnil(L);
        while (lua_next(L, args) != 0) {
            if (!lua_isnumber(L, -2) || !lua_isstring(L, -1)) {
                dv_free(Argument, &vec);
                return luaL_error(L, 
                        "Error in 'arguments' field - expected a table with "
                        "numeric keys and string values");
            }

            size_t num = lua_tointeger(L, -2) - 1;
            if (num >= dv_size(&vec)) {
                size_t add = dv_size(&vec) - num;
                adbus_Argument* dest = dv_push(Argument, &vec, add);
                adbus_arg_init(dest, add);
            }

            size_t sz;
            adbus_Argument* arg = &dv_a(&vec, num);
            arg->value  = lua_tolstring(L, -1, &sz);
            arg->size   = (int) sz;
            
            lua_pop(L, 1); // leave the key
        }
        m->argumentsSize = dv_size(&vec);
        m->arguments = dv_release(Argument, &vec);

    } else if (!lua_isnil(L, -1)) {
        return luaL_error(L, 
                "Error in 'arguments' field - expected a table with numeric "
                "keys and string values");
    }
    lua_pop(L, 1);

    assert(lua_gettop(L) == top);

    return 0;
}

/* ------------------------------------------------------------------------- */

static const char* kReplyFields[] = {
  "serial",
  "remote",
  "callback",
  "object",
  "unpack_message",
  NULL
};

int adbusluaI_toreply(
        lua_State*          L,
        int                 table,
        adbus_Reply*        r,
        int*                callback,
        int*                object,
        adbus_Bool*         unpack)
{
    int top = lua_gettop(L);
    UNUSED(top);

    adbus_reply_init(r);

    if (!lua_istable(L, table)){
        return luaL_error(L,
                "Invalid argument - must be a table detailing the reply registration.");
    }

    if (adbusluaI_check_fields(L, table, kReplyFields)) {
        return luaL_error(L,
                "Invalid field in reply table. Supported fields are 'serial', "
                "'remote', 'callback', 'object', and 'unpack_message'.");
    }

    // default is to unpack
    *unpack = 1;

    int64_t serial = -1;

    if (    adbusluaI_intField(L, table, "serial", &serial)
        ||  adbusluaI_stringField(L, table, "remote", &r->remote, &r->remoteSize)
        ||  adbusluaI_functionField(L, table, "callback", callback)
        ||  adbusluaI_boolField(L, table, "unpack_message", unpack))
    {
        return lua_error(L);
    }

    if (serial == -1)
        return luaL_error(L, "Missing 'serial' field - expected a number");
    if (!r->remote)
        return luaL_error(L, "Missing 'remote' field - expected a string");
    if (!*callback)
        return luaL_error(L, "Missing 'callback' field - expected a function");

    r->serial = (uint32_t) serial;

    lua_getfield(L, table, "object");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
    } else {
        *object = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    assert(lua_gettop(L) == top);

    return 0;
}


