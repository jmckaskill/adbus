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


#include "ADBusLua.h"

#include "Connection.h"
#include "Data.h"
#include "Interface.h"
#include "Match.h"
#include "Message.h"
#include "Object.h"

#include <assert.h>
#include <lauxlib.h>

#define LADBUSCONNECTION_HANDLE "LADBusConnection"
#define LADBUSOBJECT_HANDLE     "LADBusObject"
#define LADBUSINTERFACE_HANDLE   "LADBusInterface"

// ----------------------------------------------------------------------------

void LADBusPrintDebug(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "[adbuslua] ");
  vfprintf(stderr, format, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

// ----------------------------------------------------------------------------

struct LADBusConnection* LADBusPushNewConnection(lua_State* L)
{
    void* udata = lua_newuserdata(L, sizeof(struct LADBusConnection));
    luaL_getmetatable(L, LADBUSCONNECTION_HANDLE);
    lua_setmetatable(L, -2);
    return (struct LADBusConnection*) udata;
}

// ----------------------------------------------------------------------------

struct LADBusObject* LADBusPushNewObject(lua_State* L)
{
    void* udata = lua_newuserdata(L, sizeof(struct LADBusObject));
    luaL_getmetatable(L, LADBUSOBJECT_HANDLE);
    lua_setmetatable(L, -2);
    return (struct LADBusObject*) udata;
}

// ----------------------------------------------------------------------------

struct LADBusInterface* LADBusPushNewInterface(lua_State* L)
{
    void* udata = lua_newuserdata(L, sizeof(struct LADBusInterface));
    luaL_getmetatable(L, LADBUSINTERFACE_HANDLE);
    lua_setmetatable(L, -2);
    return (struct LADBusInterface*) udata;
}

// ----------------------------------------------------------------------------

struct LADBusConnection*    LADBusCheckConnection(lua_State* L, int index)
{
    void* udata = luaL_checkudata(L, index, LADBUSCONNECTION_HANDLE);
    return (struct LADBusConnection*) udata;
}

// ----------------------------------------------------------------------------

struct LADBusObject* LADBusCheckObject(lua_State* L, int index)
{
    void* udata = luaL_checkudata(L, index, LADBUSOBJECT_HANDLE);
    return (struct LADBusObject*) udata;
}

// ----------------------------------------------------------------------------

struct LADBusInterface* LADBusCheckInterface(lua_State* L, int index)
{
    void* udata = luaL_checkudata(L, index, LADBUSINTERFACE_HANDLE);
    return (struct LADBusInterface*) udata;
}

// ----------------------------------------------------------------------------

// Reg for adbuslua_core.connection
static const luaL_Reg kConnectionReg[] = {
    {"new", &LADBusCreateConnection},
    {"__gc", &LADBusFreeConnection},
    {"set_send_callback", &LADBusSetConnectionSendCallback},
    {"parse", &LADBusParse},
    {"connect_to_bus", &LADBusConnectToBus},
    {"is_connected_to_bus", &LADBusIsConnectedToBus},
    {"unique_service_name", &LADBusUniqueServiceName},
    {"next_serial", &LADBusNextSerial},
    {"add_bus_match", &LADBusAddBusMatch},
    {"remove_bus_match", &LADBusRemoveMatch},
    {"add_match", &LADBusAddMatch},
    {"remove_match", &LADBusRemoveMatch},
    {"add_object", &LADBusAddObject},
    {NULL, NULL},
};

// Reg for adbuslua_core.interface
static const luaL_Reg kInterfaceReg[] = {
    {"new", &LADBusCreateInterface},
    {"__gc", &LADBusFreeInterface},
    {"name", &LADBusInterfaceName},
    {NULL, NULL},
};

// Reg for adbuslua_core.object
static const luaL_Reg kObjectReg[] = {
    {"__gc", &LADBusRemoveObject},
    {"bind_interface", &LADBusBindInterface},
    {"emit", &LADBusEmit},
    {NULL, NULL},
};

// Reg for adbuslua_core
static const luaL_Reg kCoreReg[] = {
    {"send_error", &LADBusSendError},
    {"send_reply", &LADBusSendReply},
    {NULL, NULL},
};


static void CreateMetatable(
        lua_State* L,
        int libTableIndex,
        const char* handle,
        const char* luaName,
        const luaL_Reg* functions)
{
    // The metatable gets registered both as
    // REGISTRY[handle] and adbuslua_core[luaName]
    luaL_newmetatable(L, handle);
    luaL_register(L, NULL, functions);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index"); //metatable.__index = metatable
    lua_setfield(L, libTableIndex, luaName);
}

int luaopen_adbuslua_core(lua_State* L)
{
    luaL_register(L, "adbuslua_core", kCoreReg);
    int libTable = lua_gettop(L);
    CreateMetatable(L, libTable, LADBUSCONNECTION_HANDLE, "connection", kConnectionReg);
    CreateMetatable(L, libTable, LADBUSINTERFACE_HANDLE, "interface", kInterfaceReg);
    CreateMetatable(L, libTable, LADBUSOBJECT_HANDLE, "object", kObjectReg);
    assert(lua_gettop(L) == libTable);
    return 1;
}

// ----------------------------------------------------------------------------

