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

#include <assert.h>
#include <string.h>

/* ------------------------------------------------------------------------- */

int adbusluaI_getoption(
        lua_State*      L,
        int             index,
        const char**    types,
        const char*     error)
{
    if (lua_isstring(L, index)) {
        const char* string = lua_tostring(L, index);
        int i = 0;
        while (types[i] != NULL && strcmp(string, types[i]) != 0) {
            ++i;
        }

        if (types[i] == NULL || *types[i] == '\0') {
            return luaL_error(L, "%s", error);
        }

        return i;

    } else {
        return luaL_error(L, "%s", error);
    }
    return -1;
}

adbus_Bool adbusluaI_getboolean(
        lua_State*      L,
        int             index,
        const char*     error)
{
    if (lua_type(L, index) != LUA_TBOOLEAN) {
        luaL_error(L, "%s", error);
        return 0;
    }
    return (adbus_Bool) lua_toboolean(L, index);
}

lua_Number adbusluaI_getnumber(
        lua_State*      L,
        int             index,
        const char*     error)
{
    if (lua_type(L, index) != LUA_TNUMBER) {
        luaL_error(L, "%s", error);
        return 0;
    }
    return lua_tonumber(L, index);
}

const char* adbusluaI_getstring(
        lua_State*      L,
        int             index,
        size_t*         size,
        const char*     error)
{
    if (lua_type(L, index) != LUA_TSTRING) {
        luaL_error(L, "%s", error);
        return NULL;
    }
    return lua_tolstring(L, index, size);
}


/* ------------------------------------------------------------------------- */

static adbus_Bool IsValidKey(const char* valid[], const char* key)
{
    for (int i = 0; valid[i] != NULL; ++i) {
        if (strcmp(valid[i], key) == 0)
            return 1;
    }
    return 0;
}

int adbusluaI_check_fields(lua_State* L, int table, const char* valid[])
{
    lua_pushnil(L);
    while (lua_next(L, table) != 0) {
        if (lua_type(L, -2) != LUA_TSTRING) {
            return -1;
        }

        if (!IsValidKey(valid, lua_tostring(L, -2)))
            return -1;

        lua_pop(L, 1); // pop value - leave key
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

        lua_pop(L, 1); // pop value - leave key
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static void Setup(lua_State* L, int module, const char* name)
{
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index"); //metatable.__index = metatable
    lua_setfield(L, module, name);
}

static const luaL_Reg reg[] = {
    {NULL, NULL}
};

int luaopen_adbuslua_core(lua_State* L)
{
#ifdef _WIN32
    WSADATA wsadata;
    int err = WSAStartup(MAKEWORD(2,2), &wsadata);
    if (err)
        return luaL_error(L, "WSAStartup error %d", err);
#endif
    luaL_register(L, "adbuslua_core", reg);
    int module = lua_gettop(L);

    adbusluaI_reg_connection(L);    Setup(L, module, "connection");
    adbusluaI_reg_object(L);        Setup(L, module, "object");
    adbusluaI_reg_interface(L);     Setup(L, module, "interface");
    adbusluaI_reg_socket(L);        Setup(L, module, "socket");

    assert(lua_gettop(L) == module);
    return 1;
}


