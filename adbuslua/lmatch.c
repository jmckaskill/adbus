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

#include <adbus/adbuslua.h>
#include "internal.h"

/* ------------------------------------------------------------------------- */

static int UnpackOptionalIntField(
        lua_State*      L,
        int             table,
        const char*     field,
        int64_t*        value)
{
    lua_getfield(L, table, field);
    if (lua_isnil(L, -1)) {
        // Field is not filled out, but since its optional we ignore it

    } else if (lua_isnumber(L, -1)) {
        *value = (uint32_t) lua_tonumber(L, -1);

    } else {
        return luaL_error(L, 
                "Value for field %s in the match registration is not a number.",
                field);
    }
    lua_pop(L, 1);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int UnpackOptionalBooleanField(
        lua_State*      L,
        int             table,
        const char*     field,
        adbus_Bool*     value)
{
    lua_getfield(L, table, field);
    if (lua_isnil(L, -1)) {
        // Field is not filled out, but since its optional we ignore it

    } else if (lua_isboolean(L, -1)) {
        *value = lua_toboolean(L, -1);

    } else {
        return luaL_error(L, 
                "Value for field %s in the match registration is not a boolean.",
                field);
    }
    lua_pop(L, 1);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int UnpackOptionalStringField(
        lua_State*      L,
        int             table,
        const char*     field,
        const char**    string,
        int*            size)
{
    lua_getfield(L, table, field);
    if (lua_isnil(L, -1)) {
        // Field is not filled out, but since its optional we ignore it

    } else if (lua_isstring(L, -1)) {
        size_t sz;
        *string = lua_tolstring(L, -1, &sz);
        *size = (int) sz;

    } else {
        return luaL_error(L,
                "Value for field %s in the match registration is not a string.",
                field);
    }
    lua_pop(L, 1);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int UnpackCallbackField(
        lua_State*      L,
        int             table,
        const char*     field,
        int*            ref)
{
    lua_getfield(L, table, field);
    if (lua_isfunction(L, -1)) {
        *ref = adbusluaI_ref(L, -1);

    } else {
        return luaL_error(L,
                "Value for the required field %s in the match registration is "
                "missing or not a function",
                field);
    }
    lua_pop(L, 1);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int UnpackOptionalObjectField(
        lua_State*        L,
        int               table,
        const char*       field,
        int*              ref)
{
    lua_getfield(L, table, field);
    if (!lua_isnil(L, -1)) {
        *ref = adbusluaI_ref(L, -1);
    }
    lua_pop(L, 1);
    return 0;
}

/* ------------------------------------------------------------------------- */

static const char* kMatchFields[] = {
  "type",
  "sender",
  "destination",
  "interface",
  "reply_serial",
  "path",
  "member",
  "error_name",
  "remove_on_first_match",
  "add_match_to_bus_daemon",
  "callback",
  "object",
  "id",
  NULL
};

static const char* kValidTypes[] = {
    "",
    "method_call",
    "method_return",
    "error",
    "signal",
    NULL,
};

static const char kTypesError[] = 
    "Invalid value for 'type' field in match registration. "
    "Valid fields are 'method_call', 'method_return', 'error' and 'signal'.";


static int UnpackMatch(
        lua_State*      L,
        int             table,
        adbus_Match*    m,
        adbuslua_Data*  d)
{
    if (!lua_istable(L, table)){
        return luaL_error(L,
                "Invalid argument - must be a table detailing the match registration.");
    }

    if (adbusluaI_check_fields(L, table, kMatchFields)) {
        return luaL_error(L,
                "Invalid field in match table. Supported fields "
                "are 'type', 'id', 'sender', 'destination', 'interface', "
                "'reply_serial', 'path', 'member', 'error_name', "
                "'remove_on_first_match', 'add_match_to_bus_daemon', "
                "'object', and 'callback'.");
    }

    lua_getfield(L, table, "type");
    if (!lua_isnil(L, -1)) {
        int type = adbusluaI_getoption(L, -1, kValidTypes, kTypesError);
        m->type = (adbus_MessageType) type;
    }
    lua_pop(L, 1);

    int64_t id = -1;
    UnpackOptionalIntField(L, table, "id", &id);
    if (id != -1)
        m->id = id;

    int64_t reply = -1;
    UnpackOptionalIntField(L, table, "reply_serial", &reply);
    if (reply != -1)
        m->replySerial = reply;

    UnpackOptionalStringField(L, table, "sender", &m->sender, &m->senderSize);
    UnpackOptionalStringField(L, table, "interface", &m->interface, &m->interfaceSize);
    UnpackOptionalStringField(L, table, "destination", &m->destination, &m->destinationSize);
    UnpackOptionalStringField(L, table, "path", &m->path, &m->pathSize);
    UnpackOptionalStringField(L, table, "member", &m->member, &m->memberSize);
    UnpackOptionalStringField(L, table, "error_name", &m->errorName, &m->errorNameSize);
    UnpackOptionalBooleanField(L, table, "add_match_to_bus_daemon", &m->addMatchToBusDaemon);
    UnpackOptionalBooleanField(L, table, "remove_on_first_match", &m->removeOnFirstMatch);
    UnpackOptionalObjectField(L, table, "object", &d->argument);
    UnpackCallbackField(L, table, "callback", &d->callback);

    return 0;
}

/* ------------------------------------------------------------------------- */

static int MatchCallback(adbus_CbData* d)
{
    adbuslua_Data* data = (adbuslua_Data*) d->user1;

    lua_State* L = data->L;
    int top = lua_gettop(L);

    adbusluaI_push(L, data->callback);
    if (data->argument)
        adbusluaI_push(L, data->argument);

    int err = adbuslua_push_message(L, d->message, d->args);
    if (err) {
        lua_settop(L, top);
        return err;
    }

    // function itself is not included in the arg count
    lua_call(L, lua_gettop(L) - top - 1, 0);
    return 0;
}

/* ------------------------------------------------------------------------- */

int adbuslua_check_match(lua_State* L, int index, adbus_Match* m)
{
    adbus_match_init(m);

    adbuslua_Data* d = adbusluaI_newdata(L);
    m->callback = &MatchCallback;
    m->user1 = &d->h;

    return UnpackMatch(L, 2, m, d);
}

