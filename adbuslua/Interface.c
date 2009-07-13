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

#include "Interface.h"

#include "Connection.h"
#include "Data.h"
#include "Object.h"

#include <assert.h>

// ----------------------------------------------------------------------------

int LADBusCheckFields(
        lua_State*         L,
        int                table,
        uint               allowNumbers,
        const char*        valid[])
{
    lua_pushnil(L);
    while (lua_next(L, table) != 0) {
        // Key is at keyIndex -- leave between loops
        // Value is at valueIndex -- pop after loop
        int keyIndex   = lua_gettop(L) - 1;
        int valueIndex = keyIndex + 1;

        if (lua_type(L, keyIndex) == LUA_TNUMBER) {
            if (allowNumbers) {
                assert(lua_gettop(L) == valueIndex);
                lua_settop(L, keyIndex);
                continue;
            } else {
                return -1;
            }
        }

        // lua_next doesn't like lua_tolstring if the type is in fact a number
        if (lua_type(L, keyIndex) != LUA_TSTRING) {
            return -1;
        }

        size_t keySize;
        const char* key = lua_tolstring(L, keyIndex, &keySize);

        int i = 0;
        while (valid[i] != NULL) {
            if (strncmp(key, valid[i], keySize) == 0)
                break;
            i++;
        }

        if (valid[i] == NULL) {
            return -1;
        }

        assert(lua_gettop(L) == valueIndex);
        lua_settop(L, keyIndex);
    }
    return 0;
}

// ----------------------------------------------------------------------------

int LADBusCreateInterface(lua_State* L)
{
    size_t size;
    const char* name = luaL_checklstring(L, 1, &size);
    LOGD("LADBusCreateInterface: %s", name);

    int argnum = lua_gettop(L);
    struct LADBusInterface* interface = LADBusPushNewInterface(L);
    interface->interface = ADBusCreateInterface(name, size);
    lua_pushvalue(L, 1);
    interface->nameRef = luaL_ref(L, LUA_REGISTRYINDEX);

    // Methods
    int argstop = lua_gettop(L);
    int length = lua_objlen(L, 2);
    for (int i = 1; i <= length; i++)
    {
        lua_rawgeti(L, 2, i);

        LOGD("Parsing member table %d", i);

        int memberTable = lua_gettop(L);

        enum ADBusMemberType memberType;
        struct ADBusMember* member;
        member = LADBusUnpackMember(L, i, memberTable, interface->interface, &memberType);

        LOGD("Type %d", memberType);

        assert(lua_gettop(L) == memberTable);

        struct LADBusData data;
        memset(&data, 0, sizeof(struct LADBusData));
        data.L = L;

        switch(memberType)
        {
            case ADBusMethodMember:
                LADBusUnpackMethodMember(L, i, memberTable, member, &data);
                break;
            case ADBusSignalMember:
                LADBusUnpackSignalMember(L, i, memberTable, member);
                break;
            case ADBusPropertyMember:
                LADBusUnpackPropertyMember(L, i, memberTable, member, &data);
                break;
        }

        struct ADBusUser user;
        LADBusSetupData(&data, &user);
        assert(member);
        ADBusSetMemberUserData(member, &user);

        lua_pop(L, 1); // membertable
        assert(argstop == lua_gettop(L));
    }
    LOGD("Finished parsing members table");
    assert(lua_gettop(L) == argnum + 1);
    return 1;
}

// ----------------------------------------------------------------------------

int LADBusFreeInterface(lua_State* L)
{
    struct LADBusInterface* i = LADBusCheckInterface(L, 1);
    luaL_unref(L, LUA_REGISTRYINDEX, i->nameRef);
    ADBusFreeInterface(i->interface);
    return 0;
}

// ----------------------------------------------------------------------------

int LADBusInterfaceName(lua_State* L)
{
    struct LADBusInterface* i = LADBusCheckInterface(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, i->nameRef);
    return 1;
}

// ----------------------------------------------------------------------------

struct ADBusMember* LADBusUnpackMember(
        lua_State*              L,
        int                     memberIndex,
        int                     memberTable,
        struct ADBusInterface*  interface,
        enum ADBusMemberType*   type)
{
    lua_getfield(L, memberTable, "type");
    lua_getfield(L, memberTable, "name");

    if ( lua_type(L, -2) != LUA_TSTRING
      || lua_type(L, -1) != LUA_TSTRING) {
        luaL_error(L, "Member table %d is missing the required string"
                " fields for 'type' and/or 'name'.",
                memberIndex);
        return NULL;
    }

    size_t typeSize, nameSize;
    const char* typestr = lua_tolstring(L, -2, &typeSize);
    const char* name = lua_tolstring(L, -1, &nameSize);

    enum ADBusMemberType memberType;
    if (strncmp(typestr, "method", typeSize) == 0) {
        memberType = ADBusMethodMember;
    } else if (strncmp(typestr, "signal", typeSize) == 0) {
        memberType = ADBusSignalMember;
    } else if (strncmp(typestr, "property", typeSize) == 0) {
        memberType = ADBusPropertyMember;
    } else {
        luaL_error(L, "Member table %d has an invalid type %s (allowed "
                "values are 'method', 'signal', or 'property')",
                memberIndex, type);
        return NULL;
    }

    struct ADBusMember* member 
        = ADBusAddMember(interface, memberType, name, nameSize);

    if (type)
        *type = memberType;

    // type and name
    lua_pop(L, 2);
    return member;
}

// ----------------------------------------------------------------------------

static const char* kSignalValid[] = {
    "type",
    "name",
    "arguments",
    "annotations",
    NULL,
};

void LADBusUnpackSignalMember(
        lua_State*          L,
        int                 memberIndex,
        int                 memberTable,
        struct ADBusMember* member)
{
    if (LADBusCheckFields(
            L,
            memberTable,
            0,
            kSignalValid)) {
        luaL_error(L, "Invalid field in member %d. Supported fields for "
                "signals are 'type', 'name', 'arguments', and 'annotations'.",
                memberIndex);
        return;
    }


    lua_getfield(L, memberTable, "arguments");
    if (!lua_isnil(L, -1)) {
        LADBusUnpackArguments(
                L,
                memberIndex,
                lua_gettop(L),
                member,
                ADBusOutArgument);
    }

    lua_getfield(L, memberTable, "annotations");
    if (!lua_isnil(L, -1))
        LADBusUnpackAnnotations(L, memberIndex, lua_gettop(L), member);

    lua_pop(L, 2);
    LOGD("Unpacking signal fields complete");
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

void LADBusUnpackMethodMember(
        lua_State*          L,
        int                 memberIndex,
        int                 memberTable,
        struct ADBusMember* member,
        struct LADBusData*  data)
{
    LOGD("Unpacking method fields");
    if (LADBusCheckFields(
            L,
            memberTable,
            0,
            kMethodValid)) {
        luaL_error(L, "Invalid field in member %d. Supported fields for "
                "methods are 'type', 'name', 'arguments', 'annotations', and "
                "'callback'.",
                memberIndex);
        return;
    }

    lua_getfield(L, memberTable, "arguments");
    if (!lua_isnil(L, -1)) {
        LADBusUnpackArguments(
                L,
                memberIndex,
                lua_gettop(L),
                member,
                ADBusInArgument);
    }

    lua_getfield(L, memberTable, "annotations");
    if (!lua_isnil(L, -1))
        LADBusUnpackAnnotations(L, memberIndex, lua_gettop(L), member);

    lua_getfield(L, memberTable, "callback");
    if (!lua_isfunction(L, -1)) {
        luaL_error(L, "Missing or invalid type for required 'callback' "
                "field for member %d",
                memberIndex);
        return;
    }

    LADBusUnpackCallback(L, memberIndex, "callback",
            lua_gettop(L), &data->ref[LADBusMethodRef]);

    ADBusSetMethodCallback(member, &LADBusMethodCallback);

    lua_pop(L, 3);
    LOGD("Unpacking method fields complete");
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

void LADBusUnpackPropertyMember(
        lua_State*          L,
        int                 memberIndex,
        int                 memberTable,
        struct ADBusMember* member,
        struct LADBusData*  data)
{
    LOGD("Unpacking property fields");
    if (LADBusCheckFields(
            L,
            memberTable,
            0,
            kPropertyValid)) {
        luaL_error(L, "Invalid field in member %d. Supported fields for "
                "properties are 'type', 'name', 'property_type', "
                "'annotations', 'get_callback', and 'set_callback'.",
                memberIndex);
        return;
    }

    lua_getfield(L, memberTable, "annotations");
    if (!lua_isnil(L, -1))
        LADBusUnpackAnnotations(L, memberIndex, lua_gettop(L), member);

    lua_getfield(L, memberTable, "property_type");
    if (!lua_isstring(L, -1)) {
        luaL_error(L, "Missing or invalid type for required "
                "'property_type' field for member %d",
                memberIndex);
        return;
    }

    size_t propertyTypeSize;
    const char* propertyType = lua_tolstring(L, -1, &propertyTypeSize);
    ADBusSetPropertyType(member, propertyType, propertyTypeSize);

    lua_getfield(L, memberTable, "get_callback");
    if (!lua_isnil(L, -1)) {
        LADBusUnpackCallback(
                L,
                memberIndex,
                "get_callback",
                lua_gettop(L),
                &data->ref[LADBusGetPropertyRef]);

        //ADBusSetPropertyGetCallback(member, &LADBusGetPropertyCallback);
    }

    lua_getfield(L, memberTable, "set_callback");
    if (!lua_isnil(L, -1)) {
        LADBusUnpackCallback(
                L,
                memberIndex,
                "set_callback",
                lua_gettop(L),
                &data->ref[LADBusSetPropertyRef]);

        //ADBusSetPropertySetCallback(member, &LADBusMethodCallback);
    }

    if (lua_isnil(L, -2) && lua_isnil(L, -1)) {
        luaL_error(L, "One or both of the 'get_callback' and 'set_callback'"
               " fields must be filled out for member %d",
               memberIndex);
        return;
    }

    lua_pop(L, 4);
    LOGD("Unpacking property fields complete");
}

// ----------------------------------------------------------------------------

void LADBusUnpackArguments(
        lua_State*                  L,
        int                         memberIndex,
        int                         argsTable,
        struct ADBusMember*         member,
        enum ADBusArgumentDirection defaultDirection)
{
    LOGD("Unpacking arguments");
    int i = 1;
    while (1) {
        lua_rawgeti(L, argsTable, i++);
        if (lua_isnil(L, -1))
            break;

        int argTable = lua_gettop(L);
        lua_getfield(L, argTable, "name");
        lua_getfield(L, argTable, "type");
        lua_getfield(L, argTable, "direction");

        if (lua_type(L, argTable + 2) != LUA_TSTRING) {
            luaL_error(L, "Argument table %d of member %d is missing"
                    " the required 'type' field",
                    i, memberIndex);
            return;
        }

        size_t nameSize, typeSize, directionSize;
        const char* name = lua_tolstring(L, argTable + 1, &nameSize);
        const char* type = lua_tolstring(L, argTable + 2, &typeSize);
        const char* direction = lua_tolstring(L, argTable + 3, &directionSize);
        LOGD("Unpacking argument with name '%s', type '%s', and direction '%s'",
                name, type, direction);

        enum ADBusArgumentDirection dir;
        if (direction == NULL) {
            dir = defaultDirection;
        } else if (strncmp(direction, "in", directionSize) == 0) {
            dir = ADBusInArgument;
        } else if (strncmp(direction, "out", directionSize) == 0) {
            dir = ADBusOutArgument;
        } else {
            luaL_error(L, "Invalid direction '%s' for argument %d of "
                    "member %d (supported values are 'in' or 'out')",
                    direction, i, memberIndex);
            return;
        }

        ADBusAddArgument(member, dir, name, nameSize, type, typeSize);

        // 3 fields + the argument table
        lua_pop(L, 4);
        assert(lua_gettop(L) == argsTable);
    }
    lua_pop(L, 1); // nil
}

// ----------------------------------------------------------------------------

void LADBusUnpackAnnotations(
        lua_State*          L,
        int                 memberIndex,
        int                 annotationsTable,
        struct ADBusMember* member)
{
    LOGD("Unpacking annotations");
    lua_pushnil(L);
    while (lua_next(L, annotationsTable) != 0) {

        if ( lua_type(L, -2) != LUA_TSTRING
          || lua_type(L, -1) != LUA_TSTRING) {
            luaL_error(L, "The annotations table of member table %d has "
                    "an invalid entry",
                    memberIndex);
            return;
        }

        size_t nameSize, valueSize;
        const char* name  = lua_tolstring(L, -2, &nameSize);
        const char* value = lua_tolstring(L, -1, &valueSize);

        ADBusAddAnnotation(member, name, nameSize, value, valueSize);

        lua_pop(L, 1); // pop the value - leaving the key
    }
}

// ----------------------------------------------------------------------------

void LADBusUnpackCallback(
        lua_State*          L,
        int                 memberIndex,
        const char*         fieldName,
        int                 callback,
        int*                function)
{
    LOGD("Unpacking callback");
    if (!lua_isfunction(L, callback)) {
        luaL_error(L, "Member table %d has a non-function for the '%s' "
               "field",
               fieldName, memberIndex);
        return;
    }
    lua_pushvalue(L, callback);
    *function = luaL_ref(L, LUA_REGISTRYINDEX);
}

// ----------------------------------------------------------------------------




