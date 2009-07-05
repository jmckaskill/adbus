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

#include "Object.h"

#include "Data.h"
#include "Interface.h"
#include "Message.h"

#include <assert.h>


// ----------------------------------------------------------------------------

int LADBusAddObject(lua_State* L)
{
    int argnum = lua_gettop(L);
    struct LADBusConnection* c = LADBusCheckConnection(L, 1);
    size_t nameSize;
    const char* name = luaL_checklstring(L, 2, &nameSize);

    struct ADBusObject* object = ADBusAddObject(c->connection, name, nameSize);

    struct LADBusObject* lobject = LADBusPushNewObject(L);

    lobject->object = object;
    lobject->connection = c->connection;

    lua_pushvalue(L, 1);
    lobject->connectionRef = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_newtable(L);
    lobject->interfaceRefTable = luaL_ref(L, LUA_REGISTRYINDEX);

    assert(lua_gettop(L) == argnum + 1);
    return 1;
}

// ----------------------------------------------------------------------------

int LADBusRemoveObject(lua_State* L)
{
    struct LADBusObject* object = LADBusCheckObject(L, 1);
    ADBusRemoveObject(object->connection, object->object);

    luaL_unref(L, LUA_REGISTRYINDEX, object->connectionRef);
    luaL_unref(L, LUA_REGISTRYINDEX, object->interfaceRefTable);
    return 0;
}

// ----------------------------------------------------------------------------

int LADBusBindInterface(lua_State* L)
{
    struct LADBusObject* object       = LADBusCheckObject(L, 1);
    struct LADBusInterface* interface = LADBusCheckInterface(L, 2);

    if (!lua_isnoneornil(L, 3)){
        struct LADBusData data;
        memset(&data, 0, sizeof(struct LADBusData));
        data.L = L;
        lua_pushvalue(L, 3);
        data.ref[0] = luaL_ref(L, LUA_REGISTRYINDEX);
        struct ADBusUser user;
        LADBusSetupData(&data, &user);

        ADBusBindInterface(object->object, interface->interface, &user);
    } else {
        ADBusBindInterface(object->object, interface->interface, NULL);
    }

    // Store a copy of the interface object in our interfaceRefTable so the
    // interface doesn't get collected until after the object is
    lua_rawgeti(L, LUA_REGISTRYINDEX, object->interfaceRefTable);
    lua_pushvalue(L, 2);
    luaL_ref(L, -2);


    return 0;
}

// ----------------------------------------------------------------------------

int LADBusEmit(lua_State* L)
{
    struct LADBusConnection* c  = LADBusCheckConnection(L, 1);
    struct LADBusObject* o      = LADBusCheckObject(L, 2);
    struct LADBusInterface* i   = LADBusCheckInterface(L, 3);
    size_t signalSize;
    const char* signalstr = luaL_checklstring(L, 4, &signalSize);

    struct ADBusMember* signal = ADBusGetInterfaceMember(
                i->interface,
                ADBusSignalMember,
                signalstr,
                signalSize);

    ADBusSetupSignalMarshaller(o->object, signal, c->marshaller);

    if (lua_istable(L, 5)) {

        int signatureSize;
        const char* signature = 
            ADBusFullMemberSignature(signal, ADBusOutArgument, &signatureSize);

        LADBusConvertLuaToMessage(
                L,
                5,
                c->marshaller,
                signature,
                signatureSize);
        
    } else if (!lua_isnoneornil(L, 5)) {
        return luaL_error(L, "The fifth argument must be nil or a table "
                "comprising the arguments");
    }

    ADBusConnectionSendMessage(c->connection, c->marshaller);

    return 0;
}

// ----------------------------------------------------------------------------

// For the callbacks, we get the function to call from the member data and the
// first argument from the bind data if its valid

static int CallCallback(
        struct ADBusConnection*     connection,
        const struct ADBusUser*     bindData,
        const struct ADBusMember*   member,
        struct ADBusMessage*        message,
        int                         refIndex)
{
    
    const struct ADBusUser* memberData = ADBusMemberUserData(member);

    const struct LADBusData* mdata = LADBusCheckData(memberData);
    const struct LADBusData* bdata = LADBusCheckData(bindData);

    assert(mdata);

    lua_State* L = mdata->L;
    int top = lua_gettop(L);

    lua_rawgeti(L, LUA_REGISTRYINDEX, mdata->ref[refIndex]);
    if (bdata) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, bdata->ref[0]);
    }

    int err = LADBusConvertMessageToLua(message, L);
    if (err) {
        lua_settop(L, top);
        return err;
    }

    lua_call(L, lua_gettop(L) - top, 0);

    return 0;
}

// ----------------------------------------------------------------------------

int LADBusMethodCallback(
        struct ADBusConnection*     connection,
        const struct ADBusUser*     bindData,
        const struct ADBusMember*   member,
        struct ADBusMessage*        message)
{
    return CallCallback(
            connection,
            bindData,
            member,
            message,
            LADBusMethodRef);
}

// ----------------------------------------------------------------------------



