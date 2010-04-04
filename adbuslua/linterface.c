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

#include "dmem/string.h"

#include <assert.h>
#include <string.h>

/* ------------------------------------------------------------------------- */

#define INTERFACE "adbuslua Interface"

static int NewInterface(lua_State* L)
{
    size_t namesz;
    const char* name = luaL_checklstring(L, 1, &namesz);

    struct Interface* i = (struct Interface*) lua_newuserdata(L, sizeof(struct Interface));
    memset(i, 0, sizeof(struct Interface));
    luaL_getmetatable(L, INTERFACE);
    lua_setmetatable(L, -2);

    i->interface = adbus_iface_new(name, (int) namesz);
    adbus_iface_ref(i->interface);

    return 1;
}

/* ------------------------------------------------------------------------- */

static int FreeInterface(lua_State* L)
{
    struct Interface* i = (struct Interface*) luaL_checkudata(L, 1, INTERFACE);
    adbus_iface_deref(i->interface);
    return 0;
}

/* ------------------------------------------------------------------------- */

adbus_Interface* adbusluaI_tointerface(lua_State* L, int index)
{
    struct Interface* i;
    if (lua_isstring(L, index)) {

        lua_getglobal(L, "adbus");
        lua_getfield(L, -1, "interface");
        lua_remove(L, -2);

        lua_pushvalue(L, index);

        lua_call(L, 1, 1);

        i = (struct Interface*) luaL_checkudata(L, index, INTERFACE);
        return i->interface;

    } else {
        i = (struct Interface*) luaL_checkudata(L, index, INTERFACE);
        return i->interface;

    }
}

/* ------------------------------------------------------------------------- */

static int AddMethod(lua_State* L)
{
    size_t namesz;
    struct Interface* i = (struct Interface*) luaL_checkudata(L, 1, INTERFACE);
    const char* name = luaL_checklstring(L, 2, &namesz);
    i->member = adbus_iface_addmethod(i->interface, name, (int) namesz);
    adbus_mbr_setmethod(i->member, &adbusluaI_method, NULL);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int AddProperty(lua_State* L)
{
    char* namedup;
    size_t namesz, typesz;
    struct Interface* i = (struct Interface*) luaL_checkudata(L, 1, INTERFACE);
    const char* name = luaL_checklstring(L, 2, &namesz);
    const char* type = luaL_checklstring(L, 3, &typesz);
    const char* access = luaL_checkstring(L, 4);
    i->member = adbus_iface_addproperty(i->interface, name, (int) namesz, type, (int) typesz);

    /* Dup the name in C rather than hold a ref to the string in lua, as the
     * interface may be freed on any thread.
     */
    namedup = adbusluaI_strndup(name, namesz);
    adbus_mbr_addrelease(i->member, &free, namedup);

    if (strcmp(access, "read") == 0) {
        adbus_mbr_setgetter(i->member, &adbusluaI_getproperty, namedup);
    } else if (strcmp(access, "write") == 0) {
        adbus_mbr_setsetter(i->member, &adbusluaI_setproperty, namedup);
    } else if (strcmp(access, "readwrite") == 0) {
        adbus_mbr_setgetter(i->member, &adbusluaI_getproperty, namedup);
        adbus_mbr_setsetter(i->member, &adbusluaI_setproperty, namedup);
    } else {
        return luaL_error(L, "Invalid access type %s", access);
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

static int AddSignal(lua_State* L)
{
    size_t namesz;
    struct Interface* i = (struct Interface*) luaL_checkudata(L, 1, INTERFACE);
    const char* name = luaL_checklstring(L, 2, &namesz);
    i->member = adbus_iface_addsignal(i->interface, name, (int) namesz);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int ArgumentName(lua_State* L)
{
    size_t namesz;
    struct Interface* i = (struct Interface*) luaL_checkudata(L, 1, INTERFACE);
    const char* name = luaL_checklstring(L, 2, &namesz);
    adbus_mbr_argname(i->member, name, (int) namesz);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int ReturnName(lua_State* L)
{
    size_t namesz;
    struct Interface* i = (struct Interface*) luaL_checkudata(L, 1, INTERFACE);
    const char* name = luaL_checklstring(L, 2, &namesz);
    adbus_mbr_retname(i->member, name, (int) namesz);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int ArgumentSignature(lua_State* L)
{
    size_t sigsz;
    struct Interface* i = (struct Interface*) luaL_checkudata(L, 1, INTERFACE);
    const char* sig = luaL_checklstring(L, 2, &sigsz);
    adbus_mbr_argsig(i->member, sig, (int) sigsz);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int ReturnSignature(lua_State* L)
{
    size_t sigsz;
    struct Interface* i = (struct Interface*) luaL_checkudata(L, 1, INTERFACE);
    const char* sig = luaL_checklstring(L, 2, &sigsz);
    adbus_mbr_retsig(i->member, sig, (int) sigsz);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int Annotate(lua_State* L)
{
    size_t namesz, valuesz;
    struct Interface* i = (struct Interface*) luaL_checkudata(L, 1, INTERFACE);
    const char* name = luaL_checklstring(L, 2, &namesz);
    const char* value = luaL_checklstring(L, 3, &valuesz);
    adbus_mbr_annotate(i->member, name, (int) namesz, value, (int) valuesz);
    return 0;
}


/* ------------------------------------------------------------------------- */

static const luaL_Reg reg[] = {
    { "new", &NewInterface },
    { "__gc", &FreeInterface },
    { "add_method", &AddMethod },
    { "add_signal", &AddSignal },
    { "add_property", &AddProperty },
    { "argument_name", &ArgumentName },
    { "return_name", &ReturnName },
    { "argument_signature", &ArgumentSignature },
    { "return_signature", &ReturnSignature },
    { "annotate", &Annotate },
    { NULL, NULL }
};

void adbusluaI_reg_interface(lua_State* L)
{
    luaL_newmetatable(L, INTERFACE);
    luaL_register(L, NULL, reg);
}

