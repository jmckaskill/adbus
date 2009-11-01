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

#define HANDLE "adbus_Object*"

/* ------------------------------------------------------------------------- */

static int NewObject(lua_State* L)
{
    adbus_Object** pobj = (adbus_Object**) lua_newuserdata(L, sizeof(adbus_Object*));
    luaL_getmetatable(L, HANDLE);
    lua_setmetatable(L, -2);

    *pobj = NULL;

    return 1;
}

/* ------------------------------------------------------------------------- */

static int FreeObject(lua_State* L)
{
    adbus_Object** pobj = (adbus_Object**) luaL_checkudata(L, 1, HANDLE);
    adbus_obj_free(*pobj);

    return 0;
}

/* ------------------------------------------------------------------------- */

static int Bind(lua_State* L)
{
    adbus_Object** pobj = (adbus_Object**) luaL_checkudata(L, 1, HANDLE);
    adbus_Connection* c = adbuslua_check_connection(L, 2);
    adbus_Interface* i = adbuslua_check_interface(L, 3);
    const char* path = luaL_checkstring(L, 4);

    if (*pobj == NULL)
        *pobj = adbus_obj_new();

    adbuslua_Data* d = adbusluaI_newdata(L);

    // If the user provides an object/argument then we need to add that as well
    if (!lua_isnone(L, 4)) {
        d->argument = adbusluaI_ref(L, 5);
    }

    // We need a handle on the connection so that we can fill out
    // _connectiondata in the message (so we can send delayed replies)
    d->connection = adbusluaI_ref(L, 2);
    
    // We also need a handle on the interface so that the interface is not
    // destroyed until all objects that use the interface have been removed
    d->interface = adbusluaI_ref(L, 3);


    adbus_Path* p = adbus_conn_path(c, path, -1);
    adbus_obj_bind(*pobj, p, i, &d->h);

    return 0;
}

/* ------------------------------------------------------------------------- */

static int Unbind(lua_State* L)
{
    adbus_Object** pobj = (adbus_Object**) luaL_checkudata(L, 1, HANDLE);
    adbus_Connection* c = adbuslua_check_connection(L, 2);
    adbus_Interface* i = adbuslua_check_interface(L, 3);
    const char* path = luaL_checkstring(L, 4);

    if (*pobj == NULL)
        return 0;

    adbus_Path* p = adbus_conn_path(c, path, -1);

    adbus_obj_unbind(*pobj, p, i);

    return 0;
}

/* ------------------------------------------------------------------------- */

static int UnbindAll(lua_State* L)
{
    adbus_Object** pobj = (adbus_Object**) luaL_checkudata(L, 1, HANDLE);
    adbus_obj_free(*pobj);
    *pobj = NULL;

    return 0;
}

/* ------------------------------------------------------------------------- */

static const luaL_Reg reg[] = {
    { "new", &NewObject },
    { "__gc", &FreeObject },
    { "bind", &Bind },
    { "unbind", &Unbind },
    { "unbind_all", &UnbindAll },
    { NULL, NULL }
};

void adbusluaI_reg_object(lua_State* L)
{
    luaL_newmetatable(L, HANDLE);
    luaL_register(L, NULL, reg);
}



