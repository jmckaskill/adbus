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

#include "Object.h"

#include "Data.h"
#include "Interface.h"
#include "Message.h"

#include "adbus/Interface.h"
#include "adbus/CommonMessages.h"

#include <assert.h>

// ----------------------------------------------------------------------------

static struct ADBusObject* GetObject(lua_State* L, struct LADBusConnection* c, int pathIndex)
{
    size_t size;
    const char* path = luaL_checklstring(L, index, &size);
    struct ADBusObject* o = ADBusGetObject(c->connection, path, size);
    return o;
}

// ----------------------------------------------------------------------------

int LADBusBindInterface(lua_State* L)
{
    struct LADBusConnection* connection = LADBusCheckConnection(L, 1);
    struct ADBusObject* object = GetObject(L, connection, 2);
    struct ADBusInterface* interface = LADBusCheckInterface(L, 3);

    struct LADBusData* data = LADBusCreateData();
    data->L = L;
    // If the user provides an object/argument then we need to add that as well
    if (!lua_isnoneornil(L, 4))
        data->argument = LADBusGetRef(L, 4);

    // We need a handle on the connection so that we can fill out
    // _connectiondata in the message (so we can send delayed replies)
    data->connection = LADBusGetRef(L, 1);
    
    // We also need a handle on the interface so that the interface is not
    // destroyed until all objects that use the interface have been removed
    data->interface = LADBusGetRef(L, 3);


    ADBusBindInterface(object, interface, &data->header);

    return 0;
}

// ----------------------------------------------------------------------------

int LADBusUnbindInterface(lua_State* L)
{
    struct LADBusConnection* connection = LADBusCheckConnection(L, 1);
    struct ADBusObject* object = GetObject(L, connection, 2);
    struct ADBusInterface* interface = LADBusCheckInterface(L, 3);

    ADBusUnbindInterface(object, interface);

    return 0;
}

// ----------------------------------------------------------------------------

int LADBusEmit(lua_State* L)
{
    struct LADBusConnection* c  = LADBusCheckConnection(L, 1);
    struct ADBusObject* object  = GetObject(L, c, 2);
    struct ADBusInterface* interface = LADBusCheckInterface(L, 3);
    size_t signalSize;
    const char* signalstr = luaL_checklstring(L, 4, &signalSize);

    struct ADBusMember* signal = ADBusGetInterfaceMember(
                interface,
                ADBusSignalMember,
                signalstr,
                signalSize);

    if (!signal) {
        return luaL_error(L, "Signal %s does not exist on the interface",
                          signalstr);
    }

    ADBusSetupSignal(c->message, c->connection, object, signal);

    uint havesig  = lua_istable(L, 5) && lua_objlen(L, 5) > 0;
    uint haveargs = lua_istable(L, 6) && lua_objlen(L, 6) > 0;
    if (haveargs && havesig) {
        // Check the signature table
        int argnum = lua_objlen(L, 5);
        int signum = lua_objlen(L, 6);
        if (argnum != signum) {
            return luaL_error(L, "Mismatch between signature and arguments");
        }

        struct ADBusMarshaller* marshaller = ADBusArgumentMarshaller(c->message);
        for (int i = 1; i <= argnum; ++i) {
            // Get the argument signature
            lua_rawgeti(L, 5, i);
            size_t sigsize;
            const char* signature = luaL_checklstring(L, -1, &sigsize);

            // Marshall the argument
            lua_rawgeti(L, 6, i);
            int err = LADBusMarshallArgument(L,
                                             lua_gettop(L),
                                             signature,
                                             sigsize,
                                             marshaller);
            if (err) {
                return luaL_error(L, "Error on marshalling argument %d", i);
            }

            // Pop the argument and signature
            lua_pop(L, 2);
        }
        
    } else if (haveargs || havesig) {
        return luaL_error(L, "The fifth argument must be nil or a table "
                "comprising the arguments");
    }

    ADBusSendMessage(c->connection, c->message);

    return 0;
}

// ----------------------------------------------------------------------------

// For the callbacks, we get the function to call from the member data and the
// first argument from the bind data if its valid

void LADBusMethodCallback(struct ADBusCallDetails* details)
{
    const struct LADBusData* method_data = (const struct LADBusData*) details->user1;
    const struct LADBusData* bind_data = (const struct LADBusData*) details->user2;

    assert(method_data
        && bind_data
        && method_data->L == bind_data->L
        && method_data->callback);

    lua_State* L = method_data->L;
    int top = lua_gettop(L);

    LADBusPushRef(L, method_data->callback);
    assert(lua_isfunction(L, -1));
    if (bind_data->argument)
        LADBusPushRef(L, bind_data->argument);


    int err = LADBusPushMessage(L, details->message, details->arguments);
    if (err) {
        lua_settop(L, top);
        return;
    }

    // Now we need to set a few fields in the message for use with replies
    int messageIndex = lua_gettop(L);
    LADBusPushRef(L, bind_data->connection);
    lua_setfield(L, messageIndex, "_connection");

    LADBusPushRef(L, bind_data->returnSignature);
    lua_setfield(L, messageIndex, "_return_signature");

    // function itself is not included in arg count
    lua_call(L, lua_gettop(L) - top - 1, 1);

    // If the function returns a message, we pack it off and send it back
    // via the call details mechanism
    if (lua_istable(L, top + 1) && details->returnMessage) {
        LADBusMarshallMessage(L, top + 1, details->returnMessage);
        details->manualReply = 0;
    } else {
        details->manualReply = 1;
    }
    lua_pop(L, 1);

}

// ----------------------------------------------------------------------------

void LADBusGetPropertyCallback(struct ADBusCallDetails* details)
{
    const struct LADBusData* method_data = (const struct LADBusData*) details->user1;
    const struct LADBusData* bind_data = (const struct LADBusData*) details->user2;

    assert(method_data
        && bind_data
        && method_data->L == bind_data->L
        && method_data->callback);

    lua_State* L = method_data->L;
    int top = lua_gettop(L);

    LADBusPushRef(L, method_data->callback);
    assert(lua_isfunction(L, -1));
    if (bind_data->argument)
        LADBusPushRef(L, bind_data->argument);

    // function itself is not included in arg count
    lua_call(L, lua_gettop(L) - top - 1, 1);

    LADBusPushRef(L, method_data->propertyType);
    assert(lua_isstring(L, -1));
    size_t sigsize;
    const char* signature = lua_tolstring(L, -1, &sigsize);

    LADBusMarshallArgument(
            L,
            -2,
            signature,
            (int) sigsize,
            details->propertyMarshaller);

    lua_pop(L, 2); // pop the signature and value
}

// ----------------------------------------------------------------------------

void LADBusSetPropertyCallback(struct ADBusCallDetails* details)
{
    const struct LADBusData* method_data = (const struct LADBusData*) details->user1;
    const struct LADBusData* bind_data = (const struct LADBusData*) details->user2;

    assert(method_data
        && bind_data
        && method_data->L == bind_data->L
        && method_data->callback);

    lua_State* L = method_data->L;
    int top = lua_gettop(L);

    LADBusPushRef(L, method_data->callback);
    assert(lua_isfunction(L, -1));
    if (bind_data->argument)
        LADBusPushRef(L, bind_data->argument);

    int err = LADBusPushArgument(L, details->propertyIterator);
    if (err) {
        lua_settop(L, top);
        return;
    }

    // function itself is not included in arg count
    lua_call(L, lua_gettop(L) - top - 1, 0);
}





