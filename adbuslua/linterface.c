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

#include <adbuslua.h>
#include "internal.h"

#include "dmem/string.h"

#include <assert.h>
#include <string.h>



/* ------------------------------------------------------------------------- */

static adbus_Bool CheckOptionalTable(lua_State* L, int table, const char* field)
{
    lua_getfield(L, table, field);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return 0;
    } else if (!lua_istable(L, -1)) {
        return luaL_error(L, "'%s' field should be a table.", field);
    }

    return 1;
}

/* ------------------------------------------------------------------------- */

typedef void (*add_func_t)(adbus_Member*, const char*, int);
static int DoUnpackArguments(
        lua_State*          L,
        adbus_Member*       mbr,
        int                 table,
        add_func_t          sigfunc,
        add_func_t          namefunc,
        d_String*           types)
{
    int length = (int) lua_objlen(L, table);
    for (int i = 1; i <= length; ++i) {

        lua_rawgeti(L, table, i);
        int arg = lua_gettop(L);
        const char* name = NULL;
        const char* type = NULL;

        // Arguments can be any of:
        // {"name", "type"}
        // {"type"}
        // "type"

        if (lua_istable(L, arg) && lua_objlen(L, arg) == 2) {
            lua_rawgeti(L, arg, 1);
            lua_rawgeti(L, arg, 2);
            if (!lua_isstring(L, -2) || !lua_isstring(L, -1))
                return luaL_error(L, "Invalid argument field");
            name = lua_tostring(L, -2);
            type = lua_tostring(L, -1);
        } else if (lua_istable(L, arg) && lua_objlen(L, arg) == 1) {
            lua_rawgeti(L, arg, 1);
            if (!lua_isstring(L, -1))
                return luaL_error(L, "Invalid argument field");
            type = lua_tostring(L, -1);
        } else if (lua_isstring(L, arg)) {
            type = lua_tostring(L, -1);
        } else {
            return luaL_error(L, "Invalid argument field");
        }

        if (!type)
            return luaL_error(L, "Invalid argument field");

        sigfunc(mbr, type, -1);
        if (name)
            namefunc(mbr, name, -1);
        if (types)
            ds_cat(types, type);

        lua_settop(L, arg - 1);
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static int UnpackArguments(
        lua_State*          L,
        adbus_Member*       mbr,
        int                 table,
        d_String*          sig)
{
    if (CheckOptionalTable(L, table, "arguments")) {
        DoUnpackArguments(L, mbr, -1, &adbus_mbr_argsig, &adbus_mbr_argname, sig);
        lua_pop(L, 1);
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static int UnpackReturns(
        lua_State*          L,
        adbus_Member*       mbr,
        int                 table,
        d_String*          sig)
{
    if (CheckOptionalTable(L, table, "returns")) {
        DoUnpackArguments(L, mbr, -1, &adbus_mbr_retsig, &adbus_mbr_retname, sig);
        lua_pop(L, 1);
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static int UnpackAnnotations(
        lua_State*          L,
        adbus_Member*       mbr,
        int                 table)
{
    if (CheckOptionalTable(L, table, "annotations")) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {

            if (!lua_isstring(L, -2) || !lua_isstring(L, -1))
                return luaL_error(L, "Invalid annotation");

            const char* name  = lua_tostring(L, -2);
            const char* value = lua_tostring(L, -1);

            adbus_mbr_annotate(mbr, name, -1, value, -1);

            lua_pop(L, 1); // pop the value - leaving the key
        }
        lua_pop(L, 1);
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static void ReleaseMember(void* d)
{
    struct Member* m = (struct Member*) d;
    lua_State* L = m->L;
    if (m->method)
        luaL_unref(L, LUA_REGISTRYINDEX, m->method);
    if (m->getter)
        luaL_unref(L, LUA_REGISTRYINDEX, m->getter);
    if (m->setter)
        luaL_unref(L, LUA_REGISTRYINDEX, m->setter);
    if (m->argsig)
        luaL_unref(L, LUA_REGISTRYINDEX, m->argsig);
    if (m->retsig)
        luaL_unref(L, LUA_REGISTRYINDEX, m->retsig);
}

/* ------------------------------------------------------------------------- */

static const char* kSignalValid[] = {
    "arguments",
    "annotations",
    NULL,
};

static int UnpackSignal(
        lua_State*          L,
        adbus_Interface*    interface,
        int                 table,
        const char*         name)
{
    if (adbusluaI_check_fields_numbers(L, table, kSignalValid)) {
        return luaL_error(L,
                "Invalid field in signal. Supported fields for signals are "
                "'arguments', and 'annotations'.");
    }

    adbus_Member* mbr = adbus_iface_addsignal(interface, name, -1);

    UnpackArguments(L, mbr, table, NULL);
    UnpackAnnotations(L, mbr, table);

    return 0;
}

/* ------------------------------------------------------------------------- */

static const char* kMethodValid[] = {
    "arguments",
    "returns",
    "annotations",
    "unpack_message",
    "callback",
    NULL,
};

static int UnpackMethod(
        lua_State*          L,
        adbus_Interface*    interface,
        int                 table,
        const char*         name)
{
    if (adbusluaI_check_fields_numbers(L, table, kMethodValid)) {
        return luaL_error(L,
                "Invalid field in method. Supported fields for methods are "
                "'arguments', 'returns', 'annotations', 'unpack_message', and "
                "'callback'.");
    }

    adbus_Member* mbr = adbus_iface_addmethod(interface, name, -1);

    struct Member* mdata = NEW(struct Member);
    mdata->L = L;

    d_String args; ZERO(&args);
    d_String rets; ZERO(&rets);

    UnpackReturns(L, mbr, table, &rets);
    if (ds_size(&rets) > 0) {
        lua_pushlstring(L, ds_cstr(&rets), ds_size(&rets));
        mdata->retsig = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    UnpackArguments(L, mbr, table, &args);
    if (ds_size(&args) > 0) {
        lua_pushlstring(L, ds_cstr(&args), ds_size(&args));
        mdata->argsig = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    ds_free(&args);
    ds_free(&rets);

    UnpackAnnotations(L, mbr, table);

    // default is to unpack
    mdata->unpack = 1;
    if (adbusluaI_boolField(L, table, "unpack_message", &mdata->unpack))
        return lua_error(L);

    if (adbusluaI_functionField(L, table, "callback", &mdata->method))
        return lua_error(L);

    if (!mdata->method)
        return luaL_error(L, "Missing 'callback' field - expected a function");

    adbus_mbr_setmethod(mbr, &adbusluaI_method, mdata);
    adbus_mbr_addrelease(mbr, &ReleaseMember, mdata);

    return 0;
}

/* ------------------------------------------------------------------------- */

static const char* kPropValid[] = {
    "type",
    "annotations",
    "getter",
    "setter",
    NULL,
};

static int UnpackProperty(
        lua_State*          L,
        adbus_Interface*    interface,
        int                 table,
        const char*         name)
{
    if (adbusluaI_check_fields_numbers(L, table, kPropValid))
        return luaL_error(L,
                "Invalid field for property. Supported fields for properties "
                "are 'property_type', 'annotations', 'get_callback', and "
                "'set_callback'.");

    struct Member* mdata = NEW(struct Member);
    mdata->L = L;

    const char* type = NULL;
    int tsz;
    if (adbusluaI_stringField(L, table, "type", &type, &tsz))
        return lua_error(L);
    if (!type)
        return luaL_error(L, "Missing 'type' field - expected a string");

    adbus_Member* mbr = adbus_iface_addproperty(interface, name, -1, type, tsz);

    UnpackAnnotations(L, mbr, table);

    if (adbusluaI_functionField(L, table, "getter", &mdata->getter))
        return lua_error(L);
    if (adbusluaI_functionField(L, table, "setter", &mdata->setter))
        return lua_error(L);

    if (!mdata->getter && !mdata->setter)
        return luaL_error(L, "At least one of the 'getter' or 'setter' fields must be set");

    if (mdata->getter)
        adbus_mbr_setgetter(mbr, &adbusluaI_getproperty, mdata);
    if (mdata->setter)
        adbus_mbr_setsetter(mbr, &adbusluaI_setproperty, mdata);

    adbus_mbr_addrelease(mbr, &ReleaseMember, mdata);

    return 0;
}

/* ------------------------------------------------------------------------- */

static int UnpackInterfaceTable(
        lua_State*              L,
        struct Interface*       interface,
        int                     index)
{
    // The interface table consists of a list of member definitions
    int length = (int) lua_objlen(L, index);
    for (int i = 1; i <= length; i++)
    {
        lua_rawgeti(L, index, i);

        int table = lua_gettop(L);

        lua_rawgeti(L, table, 1);
        lua_rawgeti(L, table, 2);

        if (!lua_isstring(L, -1) || !lua_isstring(L, -2))
            return luaL_error(L, "Error in first (type) or second field (name) - expected a string");

        const char* type = lua_tostring(L, -2);
        const char* name = lua_tostring(L, -1);

        if (strcmp(type, "method") == 0) {
            UnpackMethod(L, interface->interface, table, name);

        } else if (strcmp(type, "signal") == 0) {
            UnpackSignal(L, interface->interface, table, name);

        } else if (strcmp(type, "property") == 0) {
            UnpackProperty(L, interface->interface, table, name);

        } else {
            return luaL_error(L,
                    "Error in first field (type) - expected 'signal', "
                    "'method', or 'property'");
        }

        lua_pop(L, 3); // table, type, and name
        assert(lua_gettop(L) == table - 1);
    }
    return 0;
}



/* ------------------------------------------------------------------------- */

#define INTERFACE "adbuslua Interface"

static int NewInterface(lua_State* L)
{
    struct Interface* i = (struct Interface*) lua_newuserdata(L, sizeof(struct Interface));
    memset(i, 0, sizeof(struct Interface));
    luaL_getmetatable(L, INTERFACE);
    lua_setmetatable(L, -2);

    luaL_checktype(L, 1, LUA_TTABLE);

    int nsz;
    const char* name = NULL;
    if (adbusluaI_stringField(L, 1, "name", &name, &nsz))
        return lua_error(L);
    if (!name)
        return luaL_error(L, "Missing 'name' field - expected a string");

    i->interface = adbus_iface_new(name, nsz);

    UnpackInterfaceTable(L, i, 1);

    return 1;
}

/* ------------------------------------------------------------------------- */

static int FreeInterface(lua_State* L)
{
    struct Interface* iface = (struct Interface*) luaL_checkudata(L, 1, INTERFACE);
    adbus_iface_free(iface->interface);
    return 0;
}

/* ------------------------------------------------------------------------- */

adbus_Interface* adbusluaI_check_interface(lua_State* L, int index)
{
    struct Interface* i = (struct Interface*) luaL_checkudata(L, index, INTERFACE);
    return i->interface;
}

/* ------------------------------------------------------------------------- */

static const luaL_Reg reg[] = {
    { "new", &NewInterface },
    { "__gc", &FreeInterface },
    { NULL, NULL }
};

void adbusluaI_reg_interface(lua_State* L)
{
    luaL_newmetatable(L, INTERFACE);
    luaL_register(L, NULL, reg);
}

