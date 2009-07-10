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


#include "Match.h"

#include "Connection.h"
#include "Data.h"
#include "Message.h"

#include "adbus/Connection.h"



// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

static int UnpackEnum(
        lua_State* L,
        int offset,
        const char* fieldName,
        const char** types,
        const char* typesString)
{
    if (!lua_isstring(L, offset)){
        luaL_error(L, "Value for field %s in the match registration is "
                "not a string",
                fieldName);
        return -1;
    }

    size_t valueSize;
    const char* value = lua_tolstring(L, offset, &valueSize);
    int i = 0;
    const char* compareKey = types[i];
    while (compareKey != NULL){
        if (strncmp(value, compareKey, valueSize) == 0){
            return i;
        }
        compareKey = types[++i];
    }
    luaL_error(L, "Invalid value for field %s in the match registration."
            " Valid values are %s.",
            fieldName, typesString);
    return -1;
}

// ----------------------------------------------------------------------------

static unsigned int UnpackBoolean(
        lua_State* L,
        int offset,
        const char* fieldName)
{
    if (!lua_isboolean(L, offset)){
        luaL_error(L, "Value for field %s in the match registration is "
                "not a boolean",
                fieldName);
        return 0;
    }

    return lua_toboolean(L, offset);
}

// ----------------------------------------------------------------------------

static void UnpackString(
        lua_State* L,
        int offset,
        const char* fieldName,
        const char** string,
        int* size)
{
    if (!lua_isstring(L, offset)) {
        luaL_error(L, "Value for field %s in the match registration is "
                "not a string",
                fieldName);
        return;
    }

    size_t s;
    *string = lua_tolstring(L, offset, &s);
    *size = s;
}

// ----------------------------------------------------------------------------

static void UnpackCallback(
        lua_State* L,
        int offset,
        const char* fieldName,
        struct LADBusData* data)
{
    if (lua_istable(L, offset)) {
        lua_rawgeti(L, offset, 1);
        lua_rawgeti(L, offset, 2);
        if (!lua_isfunction(L, -2)) {
            luaL_error(L, "Value for field %s in the match registration "
                    "is not a function or table with a function as key 1",
                    fieldName);
            return;
        }

        lua_pushvalue(L, -2);
        data->ref[0] = luaL_ref(L, LUA_REGISTRYINDEX);
        if (!lua_isnil(L, -1)) {
            lua_pushvalue(L, -1);
            data->ref[1] = luaL_ref(L, LUA_REGISTRYINDEX);
        }

        lua_pop(L, 2);

    } else if (lua_isfunction(L, offset)) {
        lua_pushvalue(L, offset);
        data->ref[0] = luaL_ref(L, LUA_REGISTRYINDEX);

    } else {
        luaL_error(L, "Value for field %s in the match registration is "
                "not a function or table with a function as key 1",
                fieldName);
        return;
    }
}

// ----------------------------------------------------------------------------

static void UnpackMatch(
        lua_State* L,
        int offset,
        struct ADBusMatch* match,
        struct LADBusData* data,
        unsigned int allowRemove)
{
    if (!lua_istable(L, offset)){
        luaL_error(L, "Invalid argument %d - must be a table detailing "
                "the match registration.",
                offset);
        return;
    }
    lua_pushnil(L);
    while (lua_next(L, offset) != 0){
        // Key is at keyIndex -- leave between loops
        // Value is at valueIndex -- pop after loop
        int keyIndex   = lua_gettop(L) - 1;
        int valueIndex = keyIndex + 1;

        if (lua_type(L, keyIndex) != LUA_TSTRING) {
            luaL_error(L, "Invalid key type for match registration "
                    "table - must be a string");
            return;
        }

        size_t keySize;
        const char* key = lua_tolstring(L, keyIndex, &keySize);

        if (strncmp(key, "type", keySize) == 0) {
            int i = UnpackEnum(L,
                               valueIndex,
                               "type",
                               kValidTypes,
                               kTypesString);
            match->type = (enum ADBusMessageType) i;

        } else if (strncmp(key, "sender", keySize) == 0) {
            UnpackString(L,
                         valueIndex,
                         "sender",
                         &match->sender,
                         &match->senderSize);

        } else if (strncmp(key, "destination", keySize) == 0) {
            UnpackString(L,
                         valueIndex,
                         "destination",
                         &match->destination,
                         &match->destinationSize);

        } else if (strncmp(key, "interface", keySize) == 0) {
            UnpackString(L,
                         valueIndex,
                         "interface",
                         &match->interface,
                         &match->interfaceSize);

        } else if (strncmp(key, "path", keySize) == 0) {
            UnpackString(L,
                         valueIndex,
                         "path",
                         &match->path,
                         &match->pathSize);

        } else if (strncmp(key, "callback", keySize) == 0) {
            UnpackCallback(L,
                           valueIndex,
                           "callback",
                           data);

        } else if (allowRemove && strncmp(key, "remove_on_first_match", keySize) == 0) {
            match->removeOnFirstMatch 
                = UnpackBoolean(L,
                                valueIndex,
                                "remove_on_first_match");

        } else {
            luaL_error(L, "Invalid field %s given in match registration",
                    key);
            return;
        }
    }

    if (!data->ref[0]) {
        luaL_error(L, "Missing required 'callback' field in match "
                "registration");
        return;
    }

}

// ----------------------------------------------------------------------------

int LADBusAddBusMatch(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    struct ADBusMatch match;
    struct LADBusData data;
    memset(&match, 0, sizeof(struct ADBusMatch));
    memset(&data, 0, sizeof(struct LADBusData));

    UnpackMatch(L, 2, &match, &data, 0);
    match.addMatchToBusDaemon = 1;
    match.callback = &LADBusMatchCallback;

    data.L = L;
    LADBusSetupData(&data, &match.user);

    int id = ADBusAddMatch(c->connection, &match);
    lua_pushinteger(L, id);
    return 1;
}

// ----------------------------------------------------------------------------

int LADBusAddMatch(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    struct ADBusMatch match;
    struct LADBusData data;
    memset(&match, 0, sizeof(struct ADBusMatch));
    memset(&data, 0, sizeof(struct LADBusData));

    UnpackMatch(L, 2, &match, &data, 1);
    match.callback = &LADBusMatchCallback;

    data.L = L;
    LADBusSetupData(&data, &match.user);

    int id = ADBusAddMatch(c->connection, &match);
    lua_pushinteger(L, id);
    return 1;
}

// ----------------------------------------------------------------------------

int LADBusRemoveMatch(lua_State* L)
{
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    int id = luaL_checkint(L, 2);

    ADBusRemoveMatch(c->connection, id);
    return 0;
}

// ----------------------------------------------------------------------------

int LADBusMatchCallback(
        struct ADBusConnection*     connection,
        int                         id,
        struct ADBusUser*           user,
        struct ADBusMessage*        message)
{
    const struct LADBusData* data = LADBusCheckData(user);

    int top = lua_gettop(data->L);

    int err = LADBusConvertMessageToLua(message, data->L);
    if (err) return err;

    lua_rawgeti(data->L, LUA_REGISTRYINDEX, data->ref[0]);
    lua_insert(data->L, top + 1);
    if (data->ref[1]) {
        lua_rawgeti(data->L, LUA_REGISTRYINDEX, data->ref[1]);
        lua_insert(data->L, top + 2);
    }

    lua_call(data->L, lua_gettop(data->L) - top, 0);

    return 0;
}

// ----------------------------------------------------------------------------





