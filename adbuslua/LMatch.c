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


#include "LMatch.h"

#include "LConnection.h"
#include "LData.h"
#include "LInterface.h"
#include "LMessage.h"

#include "adbus/Connection.h"

#include <assert.h>


// ----------------------------------------------------------------------------

static uint UnpackOptionalEnumField(
        lua_State* L,
        int offset,
        const char* fieldName,
        const char** types,
        const char* typesString,
        int*        value)
{
    uint haveField = 0;
    lua_getfield(L, offset, fieldName);
    if (lua_isnil(L, -1)) {
        // Field is not filled out, but since its optional we ignore it

    } else if (lua_isstring(L, -1)) {
        int i = 0;
        const char* string = lua_tostring(L, -1);
        const char* compareKey = types[i];
        while (compareKey != NULL && strcmp(string, compareKey) != 0)
        {
            compareKey = types[++i];
        }

        if (compareKey == NULL) {
            luaL_error(L, "Invalid value for field %s in the match "
                              "registration. Valid values are %s.",
                              fieldName, typesString);
            return 0;
        }

        *value = i;
        haveField = 1;

    } else {
        luaL_error(L, "Invalid value for field %s in the match "
                          "registration. Valid values are %s.",
                          fieldName, typesString);
        return 0;

    }
    lua_pop(L, 1);
    return haveField;
}

// ----------------------------------------------------------------------------

static uint UnpackOptionalUInt32Field(
        lua_State* L,
        int offset,
        const char* fieldName,
        uint32_t* u)
{
    uint haveField = 0;
    lua_getfield(L, offset, fieldName);
    if (lua_isnil(L, -1)) {
        // Field is not filled out, but since its optional we ignore it

    } else if (lua_isnumber(L, -1)) {
        *u = (uint32_t) lua_tonumber(L, -1);
        haveField = 1;

    } else {
        luaL_error(L, "Value for field %s in the match registration is "
                "not a number",
                fieldName);
    }
    lua_pop(L, 1);
    return haveField;
}

// ----------------------------------------------------------------------------

static uint UnpackOptionalBooleanField(
        lua_State* L,
        int offset,
        const char* fieldName,
        uint* b)
{
    uint haveField = 0;
    lua_getfield(L, offset, fieldName);
    if (lua_isnil(L, -1)) {
        // Field is not filled out, but since its optional we ignore it

    } else if (lua_isboolean(L, -1)) {
        *b = lua_toboolean(L, -1);
        haveField = 1;

    } else {
        luaL_error(L, "Value for field %s in the match registration is "
                "not a boolean",
                fieldName);
        return 0;
    }
    lua_pop(L, 1);
    return haveField;
}

// ----------------------------------------------------------------------------

static uint UnpackOptionalStringField(
        lua_State* L,
        int offset,
        const char* fieldName,
        const char** string,
        int* size)
{
    uint haveField = 0;
    lua_getfield(L, offset, fieldName);
    if (lua_isnil(L, -1)) {
        // Field is not filled out, but since its optional we ignore it

    } else if (lua_isstring(L, -1)) {
        size_t size2;
        *string = lua_tolstring(L, -1, &size2);
        *size = (int) size2;
        haveField = 1;

    } else {
        luaL_error(L, "Value for field %s in the match registration is "
                   "not a string",
                   fieldName);
        return 0;
    }
    lua_pop(L, 1);
    return haveField;
}

// ----------------------------------------------------------------------------

static uint UnpackCallbackField(
        lua_State*          L,
        int                 offset,
        const char*         fieldName,
        int*                callbackRef)
{
    uint haveField = 0;
    lua_getfield(L, offset, fieldName);
    if (lua_isfunction(L, -1)) {
        *callbackRef = LADBusGetRef(L, -1);
        haveField = 1;

    } else {
        luaL_error(L, "Value for the required field %s in the match registration is "
                "missing or not a function",
                fieldName);
        return 0;
    }
    lua_pop(L, 1);
    return haveField;
}

// ----------------------------------------------------------------------------

static uint UnpackOptionalObjectField(
        lua_State*            L,
        int                   offset,
        const char*           fieldName,
        int*                  ref)
{
  uint haveField = 0;
  lua_getfield(L, offset, fieldName);
  if (!lua_isnil(L, -1)) {
      *ref = LADBusGetRef(L, -1);
      haveField = 1;
  }
  lua_pop(L, 1);
  return haveField;
}

// ----------------------------------------------------------------------------

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
    "invalid",
    "method_call",
    "method_return",
    "error",
    "signal",
    NULL,
};

static const char kTypesString[]
    = "'method_call', 'method_return', 'error' and 'signal'";


static int UnpackMatch(
        lua_State* L,
        int offset,
        struct ADBusMatch* match,
        struct LADBusData* data)
{
    if (!lua_istable(L, offset)){
        return luaL_error(L, "Invalid argument %d - must be a table detailing "
                "the match registration.",
                offset);
    }

    int err = LADBusCheckFields(L, offset, kMatchFields);
    if (err) {
        return luaL_error(L, "Invalid field in match table. Supported fields "
                          "are 'type', 'id', 'sender', 'destination', 'interface', "
                          "'reply_serial', 'path', 'member', 'error_name', "
                          "'remove_on_first_match', 'add_match_to_bus_daemon', "
                          "'object', and 'callback'.");
    }

    int type = -1;
    UnpackOptionalEnumField(L,
                            offset,
                            "type",
                            kValidTypes,
                            kTypesString,
                            &type);
    if (type > 0)
        match->type = (enum ADBusMessageType) type;

    UnpackOptionalUInt32Field(L,
                              offset,
                              "id",
                              &match->id);

    match->checkReplySerial = UnpackOptionalUInt32Field(L,
                              offset,
                              "reply_serial",
                              &match->replySerial);

    UnpackOptionalStringField(L,
                              offset,
                              "sender",
                              &match->sender,
                              &match->senderSize);

    UnpackOptionalStringField(L,
                              offset,
                              "interface",
                              &match->interface,
                              &match->interfaceSize);

    UnpackOptionalStringField(L,
                              offset,
                              "destination",
                              &match->destination,
                              &match->destinationSize);

    UnpackOptionalStringField(L,
                              offset,
                              "path",
                              &match->path,
                              &match->pathSize);

    UnpackOptionalStringField(L,
                              offset,
                              "member",
                              &match->member,
                              &match->memberSize);

    UnpackOptionalStringField(L,
                              offset,
                              "error_name",
                              &match->errorName,
                              &match->errorNameSize);

    UnpackOptionalBooleanField(L,
                               offset,
                               "add_match_to_bus_daemon",
                               &match->addMatchToBusDaemon);

    UnpackOptionalBooleanField(L,
                               offset,
                               "remove_on_first_match",
                               &match->removeOnFirstMatch);

    UnpackOptionalObjectField(L,
                              offset,
                              "object",
                              &data->argument);

    UnpackCallbackField(L,
                        offset,
                        "callback",
                        &data->callback);

    return 0;
}

// ----------------------------------------------------------------------------

static void MatchCallback(struct ADBusCallDetails* details)
{
    const struct LADBusData* data = (const struct LADBusData*) details->user1;

    lua_State* L = data->L;
    int top = lua_gettop(L);

    LADBusPushRef(L, data->callback);
    if (data->argument)
        LADBusPushRef(L, data->argument);

    int err = LADBusPushMessage(L, details->message, details->arguments);
    if (err) {
        lua_settop(L, top);
        return;
    }

    // function itself is not included in the arg count
    lua_call(L, lua_gettop(L) - top - 1, 0);
}

// ----------------------------------------------------------------------------

int LADBusAddMatch(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    struct LADBusData* data = LADBusCreateData();
    data->L = L;

    struct ADBusMatch match;
    ADBusInitMatch(&match);

    UnpackMatch(L, 2, &match, data);
    match.callback = &MatchCallback;
    match.user1 = &data->header;

    uint32_t id = ADBusAddMatch(c->connection, &match);
    lua_pushnumber(L, id);
    return 1;
}

// ----------------------------------------------------------------------------

int LADBusRemoveMatch(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    uint32_t id = (uint32_t) luaL_checknumber(L, 2);

    ADBusRemoveMatch(c->connection, id);
    return 0;
}

// ----------------------------------------------------------------------------

int LADBusNextMatchId(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    int id = ADBusNextMatchId(c->connection);

    lua_pushinteger(L, id);
    return 1;
}



