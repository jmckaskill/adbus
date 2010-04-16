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

#ifdef _WIN32
#   include <Winsock2.h>
#   include <WS2tcpip.h>
#endif

#include <assert.h>
#include <string.h>

/* ------------------------------------------------------------------------- */

char* adbusluaI_strndup(const char* str, size_t len)
{
    char* ret = (char*) malloc(len + 1);
    memcpy(ret, str, len);
    ret[len] = '\0';
    return ret;
}

/* ------------------------------------------------------------------------- */

static adbus_Bool IsValidKey(const char* valid[], const char* key)
{
    int i;
    for (i = 0; valid[i] != NULL; ++i) {
        if (strcmp(valid[i], key) == 0)
            return 1;
    }
    return 0;
}

int adbusluaI_check_fields_numbers(lua_State* L, int table, const char* valid[])
{
    lua_pushnil(L);
    while (lua_next(L, table) != 0) {

        if (lua_type(L, -2) == LUA_TSTRING) {
            if (!IsValidKey(valid, lua_tostring(L, -2))) {
                return -1;
            }
        } else if (lua_type(L, -2) != LUA_TNUMBER) {
            return -1;
        }

        lua_pop(L, 1); /* pop value - leave key */
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

int adbusluaI_boolField(lua_State* L, int table, const char* field, adbus_Bool* val)
{
    lua_getfield(L, table, field);
    if (lua_isboolean(L, -1)) {
		*val = lua_toboolean(L, -1) ? ADBUS_TRUE : ADBUS_FALSE;
        lua_pop(L, 1);
        return 0;
    } else if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return 0;
    } else {
        lua_pop(L, 1);
        lua_pushfstring(L, "Error in '%s' field - expected a boolean", field);
        return -1;
    }
}

int adbusluaI_intField(lua_State* L, int table, const char* field, int64_t* val)
{
    lua_getfield(L, table, field);
    if (lua_isnumber(L, -1)) {
        *val = (int64_t) lua_tonumber(L, -1);
        lua_pop(L, 1);
        return 0;
    } else if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return 0;
    } else {
        lua_pop(L, 1);
        lua_pushfstring(L, "Error in '%s' field - expected a number", field);
        return -1;
    }
}

int adbusluaI_stringField(lua_State* L, int table, const char* field, const char** str, int* sz)
{
    lua_getfield(L, table, field);
    if (lua_isstring(L, -1)) {
        size_t size;
        *str = lua_tolstring(L, -1, &size);
        *sz = (int) size;
        lua_pop(L, 1);
        return 0;
    } else if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return 0;
    } else {
        lua_pop(L, 1);
        lua_pushfstring(L, "Error in '%s' field - expected a string", field);
        return -1;
    }
}

/* ------------------------------------------------------------------------- */

static int ConnectAddress(lua_State* L)
{
    char buf[255];
    if (adbus_connect_address(ADBUS_DEFAULT_BUS, buf, sizeof(buf)))
        return luaL_error(L, "Failed to get dbus address");

    lua_pushstring(L, buf);
    return 1;
}

static int BindAddress(lua_State* L)
{
    char buf[255];
    if (adbus_bind_address(ADBUS_DEFAULT_BUS, buf, sizeof(buf)))
        return luaL_error(L, "Failed to get dbus address");

    lua_pushstring(L, buf);
    return 1;
}

static int SetLogLevel(lua_State* L)
{
    adbus_set_loglevel(luaL_checkinteger(L, 1));
    return 0;
}

/* ------------------------------------------------------------------------- */

static void Setup(lua_State* L, int module, const char* name)
{
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index"); /* metatable.__index = metatable */
    lua_setfield(L, module, name);
}

static const luaL_Reg reg[] = {
    {"connect_address", &ConnectAddress},
    {"bind_address", &BindAddress},
    {"set_log_level", &SetLogLevel},
    {NULL, NULL}
};

int luaopen_adbuslua_core(lua_State* L)
{
    int module;

#ifdef _WIN32
    WSADATA wsadata;
    int err = WSAStartup(MAKEWORD(2,2), &wsadata);
    if (err)
        return luaL_error(L, "WSAStartup error %d", err);
#endif
    lua_newtable(L);
    luaL_register(L, "adbuslua_core", reg);
    module = lua_gettop(L);

    adbusluaI_reg_connection(L);    Setup(L, module, "connection");
    adbusluaI_reg_interface(L);     Setup(L, module, "interface");
    adbusluaI_reg_object(L);        Setup(L, module, "object");

    assert(lua_gettop(L) == module);
    return 1;
}


