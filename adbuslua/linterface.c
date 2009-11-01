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

#include "memory/kstring.h"

#include <assert.h>
#include <string.h>

#define HANDLE "adbus_Interface*"


/* ------------------------------------------------------------------------- */

// For the callbacks, we get the function to call from the member data and the
// first argument from the bind data if its valid

static int MethodCallback(adbus_CbData* d)
{
    adbuslua_Data* mdata = (adbuslua_Data*) d->user1;
    adbuslua_Data* bdata = (adbuslua_Data*) d->user2;

    assert(mdata
        && bdata
        && mdata->L == bdata->L
        && mdata->callback);

    lua_State* L = mdata->L;
    int top = lua_gettop(L);

    adbusluaI_push(L, mdata->callback);
    assert(lua_isfunction(L, -1));
    if (bdata->argument)
        adbusluaI_push(L, bdata->argument);


    if (adbuslua_push_message(L, d->message, d->args)) {
        lua_settop(L, top);
        return -1;
    }

    // Now we need to set a few fields in the message for use with replies
    int message = lua_gettop(L);
    adbusluaI_push(L, bdata->connection);
    lua_setfield(L, message, "_connection");

    // function itself is not included in arg count
    lua_call(L, lua_gettop(L) - top - 1, 1);

    // If the function returns a message, we pack it off and send it back
    // via the call details mechanism
    if (lua_istable(L, -1) && d->retmessage) {
        adbuslua_check_message(L, -1, d->retmessage);
        d->manualReply = 0;
    } else {
        d->manualReply = 1;
    }
    lua_pop(L, 1);

    return 0;
}

/* ------------------------------------------------------------------------- */

static int GetPropertyCallback(adbus_CbData* d)
{
    adbuslua_Data* mdata = (adbuslua_Data*) d->user1;
    adbuslua_Data* bdata = (adbuslua_Data*) d->user2;

    assert(mdata
        && bdata
        && mdata->L == bdata->L
        && mdata->callback);

    lua_State* L = mdata->L;

    adbusluaI_push(L, mdata->callback);
    assert(lua_isfunction(L, -1));
    if (bdata->argument) {
        adbusluaI_push(L, bdata->argument);
        lua_call(L, 1, 1);
    } else {
        lua_call(L, 0, 1);
    }

    adbuslua_check_argument(L, -1, mdata->signature, -1, d->propertyMarshaller);

    lua_pop(L, 1); // pop the value
    return 0;
}

/* ------------------------------------------------------------------------- */

static int SetPropertyCallback(adbus_CbData* d)
{
    adbuslua_Data* mdata = (adbuslua_Data*) d->user1;
    adbuslua_Data* bdata = (adbuslua_Data*) d->user2;

    assert(mdata
        && bdata
        && mdata->L == bdata->L
        && mdata->callback);

    lua_State* L = mdata->L;
    int top = lua_gettop(L);

    adbusluaI_push(L, mdata->callback);
    assert(lua_isfunction(L, -1));
    if (bdata->argument) {
        adbusluaI_push(L, bdata->argument);
        lua_call(L, 1, 1);
    } else {
        lua_call(L, 0, 1);
    }

    if (adbuslua_push_argument(L, d->propertyIterator)) {
        lua_settop(L, top);
        return -1;
    }

    return 0;
}


/* ------------------------------------------------------------------------- */

static kstring_t* UnpackArguments(
        lua_State*          L,
        adbus_Member*       mbr,
        int                 table,
        adbus_Bool          method)
{
    kstring_t* sig = NULL;
    if (method)
        sig = ks_new();

    lua_getfield(L, table, "arguments");
    if (lua_isnil(L, -1)) {
        return sig;
    } else if (!lua_istable(L, -1)) {
        luaL_error(L, "'arguments' field should be a table.");
        return sig;
    }

    int length = lua_objlen(L, -1);
    for (int i = 1; i <= length; ++i) {

        lua_rawgeti(L, -1, i);
        int arg = lua_gettop(L);
        lua_getfield(L, arg, "name");
        lua_getfield(L, arg, "direction");
        lua_getfield(L, arg, "type");

        if (!lua_isstring(L, -1)) {
            luaL_error(L, "'type' argument field shoulde be a string");
            return sig;
        }

        const char* name = lua_tostring(L, -3);
        const char* type = lua_tostring(L, -2);
        const char* dir  = lua_tostring(L, -1);

        if (method && strcmp(dir, "out") == 0) {
            adbus_mbr_addreturn(mbr, name, -1, type, -1);
            ks_cat(sig, type);
        } else {
            adbus_mbr_addargument(mbr, name, -1, type, -1);
        }

        // 3 fields + the argument table
        lua_pop(L, 4);
        assert(lua_gettop(L) == arg - 1);
    }

    return sig;
}

/* ------------------------------------------------------------------------- */

static int UnpackAnnotations(
        lua_State*          L,
        adbus_Member*       mbr,
        int                 table)
{
    lua_getfield(L, table, "annotations");
    if (lua_isnil(L, -1))
        return 0;
    else if (!lua_istable(L, -1))
        return luaL_error(L, "'annotations' field should be a table.");

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {

        if (!lua_isstring(L, -2) || !lua_isstring(L, -1))
            return luaL_error(L, "The annotations table has an invalid entry");

        const char* name  = lua_tostring(L, -2);
        const char* value = lua_tostring(L, -1);

        adbus_mbr_addannotation(mbr, name, -1, value, -1);

        lua_pop(L, 1); // pop the value - leaving the key
    }
    return 1;
}

/* ------------------------------------------------------------------------- */

static const char* kSignalValid[] = {
    "type",
    "name",
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
    if (adbusluaI_check_fields(L, table, kSignalValid))
        return luaL_error(L,
                "Invalid field in signal. Supported fields for signals are "
                "'type', 'name', 'arguments', and 'annotations'.");

    adbus_Member* mbr = adbus_iface_addsignal(interface, name, -1);

    UnpackArguments(L, mbr, table, 0);
    UnpackAnnotations(L, mbr, table);

    return 0;
}

/* ------------------------------------------------------------------------- */

static const char* kMethodValid[] = {
    "type",
    "name",
    "arguments",
    "annotations",
    "callback",
    NULL,
};

static int UnpackMethod(
        lua_State*          L,
        adbus_Interface*    interface,
        int                 table,
        const char*         name)
{
    if (adbusluaI_check_fields(L, table, kMethodValid))
        return luaL_error(L,
                "Invalid field in method. Supported fields for methods are "
                "'type', 'name', 'arguments', 'annotations', and 'callback'.");

    adbus_Member* mbr = adbus_iface_addmethod(interface, name, -1);


    kstring_t* sig = UnpackArguments(L, mbr, table, 1);
    UnpackAnnotations(L, mbr, table);

    lua_getfield(L, table, "callback");
    if (!lua_isfunction(L, -1))
        return luaL_error(L, "Required 'callback' field should be a function.");

    adbuslua_Data* d = adbusluaI_newdata(L);
    d->callback = adbusluaI_ref(L, -1);
    d->signature = ks_release(sig);
    adbus_mbr_setmethod(mbr, &MethodCallback, &d->h);

    lua_pop(L, 1);

    ks_free(sig);
    return 0;
}

/* ------------------------------------------------------------------------- */

static const char* kPropValid[] = {
    "type",
    "name",
    "property_type",
    "annotations",
    "get_callback",
    "set_callback",
    NULL,
};

static char* propdup(lua_State* L, int table)
{
    lua_getfield(L, table, "property_type");
    size_t sz;
    const char* prop = lua_tolstring(L, -1, &sz);
    char* ret = (char*) malloc(sz + 1);
    memcpy(ret, prop, sz + 1);
    lua_pop(L, 1);
    return ret;
}

static int UnpackProperty(
        lua_State*          L,
        adbus_Interface*    interface,
        int                 table,
        const char*         name)
{
    if (adbusluaI_check_fields(L, table, kPropValid))
        return luaL_error(L,
                "Invalid field for property. Supported fields for properties "
                "are 'type', 'name', 'property_type', 'annotations', "
                "'get_callback', and 'set_callback'.");


    lua_getfield(L, table, "property_type");
    if (!lua_isstring(L, -1))
        return luaL_error(L, "Required 'property_type' field should be a string.");
    adbus_Member* mbr = adbus_iface_addproperty(interface, name, -1, lua_tostring(L, -1), -1);
    lua_pop(L, 1);


    UnpackAnnotations(L, mbr, table);


    lua_getfield(L, table, "get_callback");
    if (lua_isfunction(L, -1)) {
        adbuslua_Data* d = adbusluaI_newdata(L);
        d->callback = adbusluaI_ref(L, -1);
        d->signature = propdup(L, table);

        adbus_mbr_setgetter(mbr, &GetPropertyCallback, &d->h);

    } else if (!lua_isnil(L, -1)) {
        return luaL_error(L, "'get_callback' field should be a function");
    }


    lua_getfield(L, table, "set_callback");
    if (lua_isfunction(L, -1)) {
        adbuslua_Data* d = adbusluaI_newdata(L);
        d->callback = adbusluaI_ref(L, -1);
        d->signature = propdup(L, table);

        adbus_mbr_setsetter(mbr, &SetPropertyCallback, &d->h);

    } else if (!lua_isnil(L, -1)) {
        return luaL_error(L, "'set_callback' field should be a function");
    }

    if (lua_isnil(L, -2) && lua_isnil(L, -1)) {
        return luaL_error(L, 
                "One or both of the 'get_callback' and 'set_callback' fields "
                "must be filled out for member");
    }
    lua_pop(L, 2);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int UnpackInterfaceTable(
        lua_State*              L,
        adbus_Interface*        interface,
        int                     index)
{
    // The interface table consists of a list of member definitions
    int length = lua_objlen(L, index);
    for (int i = 1; i <= length; i++)
    {
        lua_rawgeti(L, index, i);

        int table = lua_gettop(L);

        lua_getfield(L, table, "name");
        lua_getfield(L, table, "type");

        if (!lua_isstring(L, -2))
            return luaL_error(L, "Required 'name' field should be a string.");
        if (!lua_isstring(L, -1))
            return luaL_error(L, "Required 'type' field should be a string.");

        const char* name = lua_tostring(L, -2);
        const char* type = lua_tostring(L, -1);

        if (strcmp(type, "method") == 0) {
            UnpackMethod(L, interface, table, name);
        } else if (strcmp(type, "signal") == 0) {
            UnpackSignal(L, interface, table, name);
        } else if (strcmp(type, "property") == 0) {
            UnpackProperty(L, interface, table, name);
        } else {
            return luaL_error(L,
                    "Member table %d has an invalid type %s (allowed values are "
                    "'method', 'signal', and 'property')",
                    i,
                    type);
        }

        lua_pop(L, 3); // table, type, and name
        assert(lua_gettop(L) == table - 1);
    }
    return 0;
}



/* ------------------------------------------------------------------------- */

static int NewInterface(lua_State* L)
{
    size_t size;
    const char* name = luaL_checklstring(L, 1, &size);

    adbus_Interface** piface = (adbus_Interface**) lua_newuserdata(L, sizeof(adbus_Interface*));
    luaL_getmetatable(L, HANDLE);
    lua_setmetatable(L, -2);

    *piface = adbus_iface_new(name, size);
    UnpackInterfaceTable(L, *piface, 2);

    return 1;
}

/* ------------------------------------------------------------------------- */

static int FreeInterface(lua_State* L)
{
    adbus_Interface** piface = (adbus_Interface**) luaL_checkudata(L, 1, HANDLE);
    adbus_iface_free(*piface);
    return 0;
}

/* ------------------------------------------------------------------------- */

adbus_Interface* adbuslua_check_interface(lua_State* L, int index)
{
    adbus_Interface** piface = (adbus_Interface**) luaL_checkudata(L, index, HANDLE);
    return *piface;
}

/* ------------------------------------------------------------------------- */

static const luaL_Reg reg[] = {
    { "new", &NewInterface },
    { "__gc", &FreeInterface },
    { NULL, NULL }
};

void adbusluaI_reg_interface(lua_State* L)
{
    luaL_newmetatable(L, HANDLE);
    luaL_register(L, NULL, reg);
}

