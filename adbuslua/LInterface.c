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

#include "LInterface.h"

#include "LConnection.h"
#include "LData.h"
#include "LObject.h"

#include "adbus/Interface.h"

#include <assert.h>

// ----------------------------------------------------------------------------

static int UnpackInterfaceTable(
        lua_State*              L,
        struct ADBusInterface*  interface,
        int                     index);

int LADBusCreateInterface(lua_State* L)
{
    size_t size;
    const char* name = luaL_checklstring(L, 1, &size);

#ifndef NDEBUG
    int argnum = lua_gettop(L);
#endif
    struct ADBusInterface* interface = ADBusCreateInterface(name, size);

    UnpackInterfaceTable(L, interface, 2);
    LADBusPushNewInterface(L, interface);
    assert(lua_gettop(L) == argnum + 1);
    return 1;
}

// ----------------------------------------------------------------------------

int LADBusFreeInterface(lua_State* L)
{
    struct ADBusInterface* i = LADBusCheckInterface(L, 1);
    ADBusFreeInterface(i);
    return 0;
}

// ----------------------------------------------------------------------------

static int UnpackArguments(
        lua_State*                  L,
        int                         memberIndex,
        int                         argsTable,
        struct ADBusMember*         member,
        enum ADBusArgumentDirection defaultDirection)
{
    int length = lua_objlen(L, argsTable);
    for (int i = 1; i <= length; ++i) {

        lua_rawgeti(L, argsTable, i);
        int argTable = lua_gettop(L);
        lua_getfield(L, argTable, "name");
        lua_getfield(L, argTable, "direction");
        lua_getfield(L, argTable, "type");

        if (lua_type(L, -1) != LUA_TSTRING) {
            return luaL_error(L, "Argument table %d of member %d is missing"
                    " the required 'type' field",
                    i, memberIndex);
        }

        size_t nameSize, typeSize;
        const char* name      = lua_tolstring(L, -3, &nameSize);
        const char* type      = lua_tolstring(L, -2, &typeSize);
        const char* direction = lua_tostring(L, -1);

        enum ADBusArgumentDirection dir;
        if (direction == NULL) {
            dir = defaultDirection;
        } else if (strcmp(direction, "in") == 0) {
            dir = ADBusInArgument;
        } else if (strcmp(direction, "out") == 0) {
            dir = ADBusOutArgument;
        } else {
            return luaL_error(L, "Invalid direction '%s' for argument %d of "
                    "member %d (supported values are 'in' or 'out')",
                    direction, i, memberIndex);
        }

        ADBusAddArgument(member, dir, name, nameSize, type, typeSize);

        // 3 fields + the argument table
        lua_pop(L, 4);
        assert(lua_gettop(L) == argsTable);
    }
    return 0;
}

// ----------------------------------------------------------------------------

static int PushReturnSignatureTable(
        lua_State*          L,
        int                 argsTable)
{
    // This runs through the arguments list and copies over the return
    // arguments into a new list which is pushed on top of the stack
    // The reason for doing is that we can then tie this new table to the
    // method callback so we can do returns with the correct signature
    lua_newtable(L);
    int j = 1;
#ifndef NDEBUG
    int newtable = lua_gettop(L);
#endif
    int length = lua_objlen(L, argsTable);
    for (int i = 1; i <= length; ++i) {
        lua_rawgeti(L, argsTable, i);
        lua_getfield(L, -1, "direction");
        if (!lua_isstring(L, -1))
            continue;

        const char* direction = lua_tostring(L, -1);
        if (strcmp(direction, "out") != 0)
            continue;

        lua_pop(L, 1); // pop the direction
        lua_rawseti(L, -1, j++);
        lua_pop(L, 1); // pop the arg entry
    }
    assert(lua_gettop(L) == newtable);
    return 0;
}

// ----------------------------------------------------------------------------

static int UnpackAnnotations(
        lua_State*          L,
        int                 memberIndex,
        int                 annotationsTable,
        struct ADBusMember* member)
{
    lua_pushnil(L);
    while (lua_next(L, annotationsTable) != 0) {

        if ( lua_type(L, -2) != LUA_TSTRING
          || lua_type(L, -1) != LUA_TSTRING)
        {
            return luaL_error(L, "The annotations table of member table %d has "
                    "an invalid entry",
                    memberIndex);
        }

        size_t nameSize, valueSize;
        const char* name  = lua_tolstring(L, -2, &nameSize);
        const char* value = lua_tolstring(L, -1, &valueSize);

        ADBusAddAnnotation(member, name, nameSize, value, valueSize);

        lua_pop(L, 1); // pop the value - leaving the key
    }
    return 1;
}

// ----------------------------------------------------------------------------

static const char* kSignalValid[] = {
    "type",
    "name",
    "arguments",
    "annotations",
    NULL,
};

static int UnpackSignal(
        lua_State*          L,
        int                 memberIndex,
        int                 memberTable,
        struct ADBusMember* member)
{
    int err = LADBusCheckFields(L, memberTable, kSignalValid);
    if (err) {
        return luaL_error(L, "Invalid field in member %d. Supported fields for "
                          "signals are 'type', 'name', 'arguments', and "
                          "'annotations'.",
                          memberIndex);
    }


    lua_getfield(L, memberTable, "arguments");
    if (!lua_isnil(L, -1)) {
        int table = lua_gettop(L);
        UnpackArguments(L,
                        memberIndex,
                        table,
                        member,
                        ADBusSignalArgument);
    }
    lua_pop(L, 1);

    lua_getfield(L, memberTable, "annotations");
    if (!lua_isnil(L, -1)) {
        UnpackAnnotations(L,
                          memberIndex,
                          lua_gettop(L),
                          member);
    }
    lua_pop(L, 1);

    return 0;
}

// ----------------------------------------------------------------------------

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
        int                 memberIndex,
        int                 memberTable,
        struct ADBusMember* member)
{
    int err = LADBusCheckFields(L, memberTable, kMethodValid);
    if (err) {
        return luaL_error(L, "Invalid field in member %d. Supported fields for "
                          "methods are 'type', 'name', 'arguments', "
                          "'annotations', and 'callback'.",
                          memberIndex);
    }

    int returnSigTable = 0;
    lua_getfield(L, memberTable, "arguments");
    if (!lua_isnil(L, -1)) {
        int table = lua_gettop(L);
        UnpackArguments(L,
                        memberIndex,
                        table,
                        member,
                        ADBusInArgument);

        PushReturnSignatureTable(L, table);
        returnSigTable = lua_gettop(L);
    }
    lua_pop(L, 1);

    lua_getfield(L, memberTable, "annotations");
    if (!lua_isnil(L, -1)) {
        UnpackAnnotations(L,
                          memberIndex,
                          lua_gettop(L),
                          member);
    }
    lua_pop(L, 1);

    lua_getfield(L, memberTable, "callback");
    if (lua_isfunction(L, -1)) {
        struct LADBusData* data = LADBusCreateData();
        data->L = L;
        data->callback = LADBusGetRef(L, -1);
        if (returnSigTable)
            data->returnSignature = LADBusGetRef(L, returnSigTable);

        ADBusSetMethodCallback(member,
                               &LADBusMethodCallback,
                               &data->header);

    } else {
        return luaL_error(L, "Missing or invalid type for required 'callback' "
                "field for member %d",
                memberIndex);
    }
    if (returnSigTable)
        lua_remove(L, returnSigTable);
    lua_pop(L, 1);

    return 0;
}

// ----------------------------------------------------------------------------

static const char* kPropertyValid[] = {
    "type",
    "name",
    "property_type",
    "annotations",
    "get_callback",
    "set_callback",
    NULL,
};

static int UnpackProperty(
        lua_State*          L,
        int                 memberIndex,
        int                 memberTable,
        struct ADBusMember* member)
{
    int err = LADBusCheckFields(L, memberTable, kPropertyValid);
    if (err) {
        return luaL_error(L, "Invalid field in member %d. Supported fields "
                          "for properties are 'type', 'name', "
                          "'property_type', 'annotations', 'get_callback',"
                          "and 'set_callback'.",
                          memberIndex);
    }

    lua_getfield(L, memberTable, "annotations");
    if (!lua_isnil(L, -1)) {
        UnpackAnnotations(L,
                          memberIndex,
                          lua_gettop(L),
                          member);
    }
    lua_pop(L, 1);

    lua_getfield(L, memberTable, "property_type");
    if (lua_isstring(L, -1)) {
        size_t size;
        const char* type = lua_tolstring(L, -1, &size);
        ADBusSetPropertyType(member, type, size);

    } else {
        return luaL_error(L, "Missing or invalid type for required "
                "'property_type' field for member %d",
                memberIndex);
    }
    lua_pop(L, 1);


    lua_getfield(L, memberTable, "get_callback");
    if (lua_isfunction(L, -1)) {
        struct LADBusData* data = LADBusCreateData();
        data->L = L;
        data->callback = LADBusGetRef(L, -1);
        ADBusSetPropertyGetCallback(member,
                                    &LADBusGetPropertyCallback,
                                    &data->header);

    } else if (!lua_isnil(L, -1)) {
        return luaL_error(L, "'get_callback' field should be a function");
    }


    lua_getfield(L, memberTable, "set_callback");
    if (lua_isfunction(L, -1)) {
        struct LADBusData* data = LADBusCreateData();
        data->L = L;
        data->callback = LADBusGetRef(L, -1);
        ADBusSetPropertySetCallback(member,
                                    &LADBusSetPropertyCallback,
                                    &data->header);

    } else if (!lua_isnil(L, -1)) {
        return luaL_error(L, "'set_callback' field should be a function");
    }

    if (lua_isnil(L, -2) && lua_isnil(L, -1)) {
        return luaL_error(L, "One or both of the 'get_callback' and 'set_callback'"
               " fields must be filled out for member %d",
               memberIndex);
    }
    lua_pop(L, 2);
    return 0;
}

// ----------------------------------------------------------------------------

static int UnpackMemberNameType(
        lua_State*              L,
        int                     memberIndex,
        int                     memberTable,
        const char**            name,
        size_t*                 nameSize,
        enum ADBusMemberType*   type)
{
    lua_getfield(L, memberTable, "name");
    lua_getfield(L, memberTable, "type");

    if ( lua_type(L, -2) != LUA_TSTRING
      || lua_type(L, -1) != LUA_TSTRING)
    {
        return luaL_error(L, "Member table %d is missing the required string"
                " fields for 'type' and/or 'name'.",
                memberIndex);
    }

    *name = lua_tolstring(L, -2, nameSize);
    const char* typestr = lua_tostring(L, -1);

    if (strcmp(typestr, "method") == 0) {
        *type = ADBusMethodMember;
    } else if (strcmp(typestr, "signal") == 0) {
        *type = ADBusSignalMember;
    } else if (strcmp(typestr, "property") == 0) {
        *type = ADBusPropertyMember;
    } else {
        return luaL_error(L, "Member table %d has an invalid type %s (allowed "
                "values are 'method', 'signal', or 'property')",
                memberIndex, type);
    }

    // type and name
    lua_pop(L, 2);

    return 0;
}

// ----------------------------------------------------------------------------

static int UnpackInterfaceTable(
        lua_State*              L,
        struct ADBusInterface*  interface,
        int                     index)
{
    // The interface table consists of a list of member definitions
#ifndef NDEBUG
    int argstop = lua_gettop(L);
#endif
    int length = lua_objlen(L, index);
    for (int i = 1; i <= length; i++)
    {
        lua_rawgeti(L, index, i);

        int memberTable = lua_gettop(L);

        enum ADBusMemberType type;
        const char* name = NULL;
        size_t nameSize = 0;

        // Unpack the general member fields (name and type)
        UnpackMemberNameType(L, i, memberTable, &name, &nameSize, &type);
        assert(lua_gettop(L) == memberTable);

        struct ADBusMember* member = ADBusAddMember(interface, type, name, nameSize);

        // Unpack the member specific fields (annotations, arguments, property
        // type, callbacks)
        switch(type)
        {
            case ADBusMethodMember:
                UnpackMethod(L, i, memberTable, member);
                break;
            case ADBusSignalMember:
                UnpackSignal(L, i, memberTable, member);
                break;
            case ADBusPropertyMember:
                UnpackProperty(L, i, memberTable, member);
                break;
        }

        lua_pop(L, 1); // membertable
        assert(argstop == lua_gettop(L));
    }
    return 0;
}






