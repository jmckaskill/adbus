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


#include "adbus/Message.h"

#include <lua.h>
#include <lauxlib.h>

static int PushNextField(lua_State* L, struct ADBusMessage* m)
{
    int err = 0;
    ADBusField f;
    if (err = ADBusGetNextField(m, &f))
        return err;

    lua_checkstack(L, 1);

    switch (f->type) {
        case ADBusBooleanField:
            lua_pushboolean(L, f.data.b);
            return 0;
        case ADBusUInt8Field:
            lua_pushinteger(L, f.data.u8);
            return 0;
        case ADBusInt16Field:
            lua_pushinteger(L, f.data.i16);
            return 0;
        case ADBusUInt16Field:
            lua_pushinteger(L, f.data.u16);
            return 0;
        case ADBusInt32Field:
            lua_pushinteger(L, f.data.i32);
            return 0;
        case ADBusUInt32Field:
            lua_pushinteger(L, f.data.u32);
            return 0;
        case ADBusInt64Field:
            lua_pushinteger(L, f.data.i64);
            return 0;
        case ADBusUInt64Field:
            lua_pushinteger(L, f.data.u64);
            return 0;
        case ADBusDoubleField:
            lua_pushnumber(L, f.data.d);
            return 0;
        case ADBusStringField:
        case ADBusObjectPathField:
        case ADBusSignatureField:
            lua_pushlstring(L, f.data.string.str, f.data.string.size);
            return 0;
        case ADBusArrayBeginField:
            return PushArray(L, m, &f);
        case ADBusStructBeginField:
            return PushStruct(L, m, &f);
        case ADBusDictEntryBeginField:
            return PushDictEntry(L, m, &f);
        case ADBusVariantBeginField:
            return PushVariant(L, m, &f);
        default:
            return ADBusInvalidData;
    }

}

static int PushStruct(lua_State* L, struct ADBusMessage* m, struct ADBusField* f)
{
    lua_newtable(L);
    int table = lua_gettop(L);
    int i = 1;
    int err = 0;
    while (!ADBusIsScopeAtEnd(f->scope)) {
        if (err = PushNextField(L, m))
            return err;

        assert(lua_gettop(L) == table + 1);
        lua_rawseti(L, table, i++);
    }
    return ADBusTakeStructEnd(m);
}

static int PushVariant(lua_State* L, struct ADBusMessage* m, struct ADBusField* f)
{
    int err = 0;
    while (!ADBusIsScopeAtEnd(f->scope)) {
        if (err = PushNextField(L, m))
            return err;
    }
    return ADBusTakeVariantEnd(m);
}


static int PushDictEntry(lua_State* L, struct ADBusMessage* m, struct ADBusField* f)
{
    int table = lua_gettop(L);
    assert(lua_istable(L, table));

    int err = 0;
    if (err = PushNextField(L, m))
        return err;

    int key = lua_gettop(L);
    assert(key == table + 1 && lua_isnumber(L, key) || lua_isstring(L, key));

    if (err = PushNextField(L,m))
        return err;

    int value = lua_gettop(L);
    assert(value == key + 1);

    // Pops the two last items on the stack as the key and value
    lua_settable(L, table);

    return 0;
}


static int PushArray(lua_State* L, struct ADBusMessage* m, struct ADBusField* f)
{
    lua_newtable(L);
    int table = lua_gettop(L);
    int i = 1;
    int err = 0;
    while (!ADBusIsScopeAtEnd(f->scope)) {
        if (err = PushNextField(L, m))
            return err;

        // No value is left on the stack if the inner type was a dict entry
        if (lua_gettop(L) == table)
            continue;

        assert(lua_gettop(L) == table + 1);
        lua_rawseti(L, table, i++);
    }
    return ADBusTakeArrayEnd(m);
}




static int PushMessageArguments(lua_State* L, struct ADBusMessage* m)
{
    int err;
    while (!ADBusIsScopeAtEnd(m, 0)) {
        if (err = PushNextField(L, m))
            return err;
    }

    return 0;
}

struct CallbackData
{
    int        id;
    lua_State* L;
    int        function;
};

struct LADBusConnection
{
    struct ADBusConnection* connection;
    struct ADBusMarshaller* marshaller;

    struct CallbackData     sendCallback;
    struct CallbackData     connectToBusCallback;

    struct CallbackData*    callbacks;
    size_t                  callbacksSize;
    size_t                  callbacksAlloc;
};


static struct CallbackData* AddNewCallback(struct LADBusConnection* c)
{
}

static void RemoveCallback(struct LADBusConnection* c, int id)
{
}


#define ADBUS_CONNECTION_HANDLE "LADBusConnection"
#define ADBUS_INTERFACE_HANDLE  "ADBusInterface"
#define ADBUS_OBJECT_HANDLE     "ADBusObject"


struct LADBusConnection* CheckConnection(lua_State* L, int index)
{
    return (struct LADBusConnection*) luaL_checkudata(L, index, ADBUS_CONNECTION_HANDLE);
}

struct ADBusInterface* CheckInterface(lua_State* L, int index)
{
    return (struct ADBusInterface*) luaL_checkudata(L, index, ADBUS_INTERFACE_HANDLE);
}

struct ADBusObject* CheckObject(lua_State* L, int index)
{
    return (struct ADBusObject*) luaL_checkudata(L, index, ADBUS_OBJECT_HANDLE);
}

int LADBusFreeConnection(lua_State* L)
{
    struct LADBusConnection* c = CheckConnection(L, 1);
    ADBusFreeConnection(c->connection);
    free(c);
    return 0;
}
static int MatchCallback(void* data, struct ADBusMessage* message)
{
    struct CallbackData* callback = (struct CallbackData*) data;
    lua_State* L = callback->L;

    int err = PushMessage(L, message);
    if (err)
        return err;

    lua_pushfield(L, LUA_REGISTRYINDEX, callback->function);
    lua_insert(L, -2);

    lua_call(L, 1, 0);
    return 0;
}

void UnpackField(lua_State* L, const char* field, const char** string, int* size)
{
    size_t s;
    lua_getfield(L, 2, field);
    if (lua_isstring(L, -1))
    {
        *string = lua_tolstring(L, -1, &s);
        *size   = s;
    }
}

int LADBusAddMatch(lua_State* L)
{
    static const char* types[] = {
        "invalid",
        "method_call", 
        "method_return",
        "error",
        "signal",
        NULL
    };

    struct ADBusMatch match;
    memset(&match, 0, sizeof(ADBusMatch));

    struct LADBusConnection* c = CheckConnection(L, 1);
    int serial = ADBusGetNextSerial(c->connection);

    lua_checktype(L, 2, LUA_TTABLE);
    lua_getfield(L, 2, "type");
    match.type = lua_checkoption(L, -1, "invalid", types);

    UnpackField(L, "sender",      &match.sender,      &match.senderSize);
    UnpackField(L, "destination", &match.destination, &match.destinationSize);
    UnpackField(L, "interface",   &match.interface,   &match.interfaceSize);
    UnpackField(L, "path",        &match.path,        &match.pathSize);
    UnpackField(L, "member",      &match.member,      &match.memberSize);

    lua_getfield(L, 2, "callback");
    if (lua_isfunction(L, -1)) {
        struct CallbackData* callback = AddNewCallback(c, serial);
        callback->L        = L;
        lua_pushvalue(L, -1);
        callback->function = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    ADBusAddMatch(c->connection, serial, &match);
    lua_pushinteger(L, serial);
    return 1;
}

// ----------------------------------------------------------------------------

int LADBusRemoveMatch(lua_State* L)
{
    struct LADBusConnection* c = CheckConnection(L, 1);
    int serial = lua_checkinteger(L, 2);
    ADBusRemoveWatch(c->connection, serial);
    RemoveCallback(c, serial);
    return 0;
}

// ----------------------------------------------------------------------------








