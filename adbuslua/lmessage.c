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

static int AppendNextField(
    lua_State*      L,
    int             index,
    adbus_Buffer*   b);

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

static int AppendStruct(
    lua_State*      L,
    int             index,
    adbus_Buffer*   b)
{
    if (adbus_buf_beginstruct(b))
        return -1;

    int n = lua_objlen(L, index);
    for (int i = 1; i <= n; ++i) {
        lua_rawgeti(L, index, i);
        int val = lua_gettop(L);

        if (AppendNextField(L, val, b))
            return -1;

        assert(lua_gettop(L) == val);
        lua_pop(L, 1);
    }

    return adbus_buf_endstruct(b);
}

/* ------------------------------------------------------------------------- */

static const char* GetMetatableVariantType(
    lua_State*              L,
    int                     table,
    int                     metatable)
{
    if (lua_isnil(L, metatable))
        return NULL;

    lua_getfield(L, metatable, "__dbus_signature");
    if (lua_isfunction(L, -1)) {
      lua_pushvalue(L, table);
      lua_call(L, 1, 1);
    }
    
    if (lua_isstring(L, -1))
        return lua_tostring(L, -1);
    else
        return NULL;
}

static void GetMetatableVariantData(
    lua_State*              L,
    int                     table,
    int                     metatable)
{
    if (lua_isnil(L, metatable))
        return;

    lua_getfield(L, metatable, "__dbus_value");
    if (lua_isfunction(L, -1)) {
        lua_pushvalue(L, table);
        lua_call(L, 1, 1);
    }

    if (!lua_isnil(L, -1))
        lua_replace(L, table);
}

static const char* DetectVariantType(
    lua_State*              L,
    int                     index)
{
    lua_pushnumber(L, 1);
    lua_gettable(L, index);
    if (lua_isnil(L, -1))
        return "a{vv}";
    else
        return "av";
}

/* ------------------------------------------------------------------------- */

static int AppendVariant(
    lua_State*              L,
    int                     index,
    adbus_Buffer*           b)
{
    int top = lua_gettop(L);
    const char* signature = NULL;
    switch (lua_type(L, index)){
        case LUA_TNUMBER:
            signature = "d";
            break;
        case LUA_TBOOLEAN:
            signature = "b";
            break;
        case LUA_TSTRING:
            signature = "s";
            break;
        case LUA_TTABLE:
            {
                lua_getmetatable(L, index);
                int metatable = lua_gettop(L);
                // note run metatable variant type before data since get data
                // may replace the value at index
                signature = GetMetatableVariantType(L, index, metatable);
                GetMetatableVariantData(L, index, metatable);
                if (!signature)
                    signature = DetectVariantType(L, index);
            }
            break;
    }
    if (!signature) {
        return luaL_error(L, "Can not convert argument to dbus variant.");
    }
    adbus_buf_beginvariant(b, signature, -1);
    // signature may point to string on stack, so only clean up the stack
    // once we are done using the sig
    lua_settop(L, top);
    AppendNextField(L, index, b);
    adbus_buf_endvariant(b);
    return 0;
}


/* ------------------------------------------------------------------------- */

static int AppendArray(
    lua_State*              L,
    int                     index,
    adbus_Buffer*           b)
{
    adbus_buf_beginarray(b);

    if (adbus_buf_expected(b) == ADBUS_DICT_ENTRY_BEGIN) {
        lua_pushnil(L);
        while (lua_next(L, index) != 0) {
            int key = lua_gettop(L) - 1;
            int val = lua_gettop(L);
            adbus_buf_begindictentry(b);

            AppendNextField(L, key, b);
            AppendNextField(L, val, b);

            adbus_buf_enddictentry(b);
            lua_pop(L, 1); // pop the value, leaving the key
            assert(lua_gettop(L) == key);
        }
    } else {
        int n = lua_objlen(L, index);
        for (int i = 1; i <= n; ++i) {
            lua_rawgeti(L, index, i);
            int val = lua_gettop(L);

            AppendNextField(L, val, b);
            assert(lua_gettop(L) == val);
            lua_pop(L, 1);
        }
    }

    adbus_buf_endarray(b);
    return 0;
}

/* ------------------------------------------------------------------------- */

static const char kFieldError[] = "Mismatch between argument and signature.";

static int AppendNextField(
    lua_State*              L,
    int                     index,
    adbus_Buffer*           buffer)
{
    adbus_Bool b;
    lua_Number n;
    size_t size;
    const char* str;
    int err = 0;
    switch(adbus_buf_expected(buffer)){
        case ADBUS_BOOLEAN:
            b = adbusluaI_getboolean(L, index, kFieldError);
            err = adbus_buf_bool(buffer, b);
            break;
        case ADBUS_UINT8:
            n = adbusluaI_getnumber(L, index, kFieldError);
            err = adbus_buf_uint8(buffer, (uint8_t) n);
            break;
        case ADBUS_INT16:
            n = adbusluaI_getnumber(L, index, kFieldError);
            err = adbus_buf_int32(buffer, (int16_t) n);
            break;
        case ADBUS_UINT16:
            n = adbusluaI_getnumber(L, index, kFieldError);
            err = adbus_buf_uint16(buffer, (uint16_t) n);
            break;
        case ADBUS_INT32:
            n = adbusluaI_getnumber(L, index, kFieldError);
            err = adbus_buf_int32(buffer, (int32_t) n);
            break;
        case ADBUS_UINT32:
            n = adbusluaI_getnumber(L, index, kFieldError);
            err = adbus_buf_uint32(buffer, (uint32_t) n);
            break;
        case ADBUS_INT64:
            n = adbusluaI_getnumber(L, index, kFieldError);
            err = adbus_buf_uint64(buffer, (int64_t) n);
            break;
        case ADBUS_UINT64:
            n = adbusluaI_getnumber(L, index, kFieldError);
            err = adbus_buf_uint64(buffer, (uint64_t) n);
            break;
        case ADBUS_DOUBLE:
            n = adbusluaI_getnumber(L, index, kFieldError);
            err = adbus_buf_double(buffer, (double) n);
            break;
        case ADBUS_STRING:
            str = adbusluaI_getstring(L, index, &size, kFieldError);
            err = adbus_buf_string(buffer, str, (int) size);
            break;
        case ADBUS_OBJECT_PATH:
            str = adbusluaI_getstring(L, index, &size, kFieldError);
            err = adbus_buf_objectpath(buffer, str, (int) size);
            break;
        case ADBUS_SIGNATURE:
            str = adbusluaI_getstring(L, index, &size, kFieldError);
            err = adbus_buf_signature(buffer, str, (int) size);
            break;
        case ADBUS_ARRAY_BEGIN:
            // This covers both arrays and maps
            err = AppendArray(L, index, buffer);
            break;
        case ADBUS_STRUCT_BEGIN:
            err = AppendStruct(L, index, buffer);
            break;
        case ADBUS_VARIANT_BEGIN:
            err = AppendVariant(L, index, buffer);
            break;
        default:
            return luaL_error(L, "Invalid signature on marshalling message.");
    }
    if (err)
        return luaL_error(L, "Error on marshalling message.");
    return 0;
}

/* ------------------------------------------------------------------------- */

int adbuslua_check_argument(
        lua_State*      L,
        int             index,
        const char*     sig,
        int             size,
        adbus_Buffer*   buf)
{
    if (sig && adbus_buf_append(buf, sig, size))
        return -1;

    return AppendNextField(L, index, buf);
}

/* ------------------------------------------------------------------------- */

static const char* kMessageFields[] = {
    "type",
    "no_reply_expected",
    "no_autostart",
    "serial",
    "interface",
    "path",
    "member",
    "error_name",
    "reply_serial",
    "destination",
    "sender",
    "signature",
    NULL,
};

static const char* kTypes[] = {
    "",
    "method_call",
    "method_return",
    "error",
    "signal",
    NULL,
};

static const char kTypesError[] = 
    "Invalid type field in message. Valid values are 'method_call', "
    "'method_return', 'error', and 'signal'.";

static const char kFlagsError[] = "Expected bool for flags fields.";
static const char kSerialError[] = "Expected a number for the 'serial' field.";
static const char kReplyError[] = "Expected a number for the 'reply_serial' field.";
static const char kStringError[] = "Expected a string for string fields.";

/* ------------------------------------------------------------------------- */

typedef void (*SetStringFunction)(adbus_Message*, const char*, int);
static void SetStringHeader(
        lua_State*              L,
        int                     index,
        adbus_Message*          message,
        const char*             field,
        SetStringFunction       function)
{
    lua_getfield(L, index, field);
    if (!lua_isnil(L, -1)) {
        size_t sz;
        const char* str = adbusluaI_getstring(L, -1, &sz, kStringError);
        function(message, str, (int) sz);
    }
    lua_pop(L, 1);
}

int adbuslua_check_message(
    lua_State*        L,
    int               index,
    adbus_Message*    msg)
{
    adbus_msg_reset(msg);
    if (adbusluaI_check_fields_numbers(L, index, kMessageFields)) {
        return luaL_error(L,
                "Invalid field in the msg table. Valid fields are 'type', "
                "'no_reply_expected', 'no_autostart', 'serial', 'interface', "
                "'path', 'member', 'error_name', 'reply_serial', 'destination', "
                "'sender', and 'signature'.");
    }
    int luaTopAtStart = lua_gettop(L);

    // Type
    lua_getfield(L, index, "type");
    if (!lua_isnil(L, -1)) {
        int type = adbusluaI_getoption(L, -1, kTypes, kTypesError);
        adbus_msg_settype(msg, (adbus_MessageType) type);
    }
    lua_pop(L, 1);


    // Flags
    uint8_t flags = 0;
    lua_getfield(L, index, "no_reply_expected");
    if (!lua_isnil(L, -1)) {
        flags |= adbusluaI_getboolean(L, -1, kFlagsError)
               ? ADBUS_MSG_NO_REPLY
               : 0;
    }
    lua_getfield(L, index, "no_autostart");
    if (!lua_isnil(L, -1)) {
        flags |= adbusluaI_getboolean(L, -1, kFlagsError)
               ? ADBUS_MSG_NO_AUTOSTART
               : 0;
    }
    adbus_msg_setflags(msg, flags);
    lua_pop(L, 2);

    // Serial
    lua_getfield(L, index, "serial");
    if (!lua_isnil(L, -1))
        adbus_msg_setserial(msg, adbusluaI_getnumber(L, -1, kSerialError));
    lua_pop(L, 1);

    lua_getfield(L, index, "reply_serial");
    if (!lua_isnil(L, -1))
        adbus_msg_setreply(msg, adbusluaI_getnumber(L, -1, kReplyError));
    lua_pop(L, 1);

    // String fields
    SetStringHeader(L, index, msg, "path", &adbus_msg_setpath);
    SetStringHeader(L, index, msg, "interface", &adbus_msg_setinterface);
    SetStringHeader(L, index, msg, "member", &adbus_msg_setmember);
    SetStringHeader(L, index, msg, "error_name", &adbus_msg_seterror);
    SetStringHeader(L, index, msg, "destination", &adbus_msg_setdestination);
    SetStringHeader(L, index, msg, "sender", &adbus_msg_setsender);

    adbus_Buffer* b = adbus_msg_buffer(msg);

    // Signature
    lua_getfield(L, index, "signature");
    if (lua_isstring(L, -1)) {
        adbus_buf_append(b, lua_tostring(L, -1), -1);
    } else if (!lua_isnil(L, -1)) {
        return luaL_error(L, "Invalid signature field.");
    }
    lua_pop(L, 1);

    // Arguments
    size_t argnum = lua_objlen(L, index);
    for (size_t i = 1; i <= argnum; ++i) {
        lua_rawgeti(L, index, i);
        int arg = lua_gettop(L);

        if (adbuslua_check_argument(L, arg, NULL, 0, b))
            return luaL_error(L, "Error on marshalling msg.");

        assert(lua_gettop(L) == arg);
        lua_pop(L, 1);
    }

    adbus_msg_build(msg);

    assert(lua_gettop(L) == luaTopAtStart);
    return 0;
}






/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

static int PushNextField(
        lua_State*         L,
        adbus_Iterator*   iter);

// Structs are seen from lua indentically to an array of variants, ie they are
// just expanded into an array
static int PushStruct(
    lua_State*          L,
    adbus_Iterator*     iter,
    adbus_Field*        field)
{
    lua_newtable(L);
    int table = lua_gettop(L);
    int arg = 1;
    while (!adbus_iter_isfinished(iter, field->scope)) {
        if (PushNextField(L, iter))
            return -1;

        lua_rawseti(L, table, arg++);
        assert(lua_gettop(L) == table);
    }
    if (adbus_iter_next(iter, field) || field->type != ADBUS_STRUCT_END)
        return -1;

    return 0;
}

/* ------------------------------------------------------------------------- */

static int PushDictEntry(
    lua_State*          L,
    adbus_Iterator*     iter,
    adbus_Field*        field)
{
    (void) field;
    // DBus maps are arrays of dict entries which map directly to a table in
    // lua, so we should be able to get the surrounding array since its on top
    // of the lua stack

    int table = lua_gettop(L);
    assert(lua_istable(L, table));

    if (PushNextField(L, iter))
        return -1;

    if (PushNextField(L, iter))
        return -1;


    // Pops the two last items on the stack - the key and value
    lua_settable(L, table);
    assert(lua_gettop(L) == table);

    return 0;
}

/* ------------------------------------------------------------------------- */

// Since lua is dynamically typed it does need to know that a particular
// argument was originally a variant
static int PushVariant(
    lua_State*          L,
    adbus_Iterator*     iter,
    adbus_Field*        field)
{
    if (PushNextField(L, iter))
        return -1;
    if (adbus_iter_next(iter, field) || field->type != ADBUS_VARIANT_END)
        return -1;
    
    return 0;
}

/* ------------------------------------------------------------------------- */

// Arrays are pushed as standard lua arrays using 1 based indexes.
// Note since maps come up from dbus as a{xx} ie an array of dict entries we
// should only set the index if the inner type was not a dict entry
static int PushArray(
    lua_State*          L,
    adbus_Iterator*     iter,
    adbus_Field*        field)
{
    lua_newtable(L);
    int table = lua_gettop(L);
    int arg = 1;
    while (!adbus_iter_isfinished(iter, field->scope)) {
        if (PushNextField(L, iter))
            return -1;

        // No value is left on the stack if the inner type was a dict entry
        if (lua_gettop(L) == table)
            continue;

        lua_rawseti(L, table, arg++);
        assert(lua_gettop(L) == table);
    }
    if (adbus_iter_next(iter, field) || field->type != ADBUS_ARRAY_END)
        return -1;

    return 0;
}

/* ------------------------------------------------------------------------- */

// Note depending on the type of lua_Number, some or all of the numeric dbus
// types may lose data on the conversion - for now there is no decent way
// around this
// Also all of the string types (string, object path, signature) all convert
// to a lua string since there is no reasonable reason for them to be
// different types (this could be changed to fancy types that overload
// __tostring, but I don't really see the point of that)
static int PushNextField(
    lua_State*          L,
    adbus_Iterator*     iter)
{
    adbus_Field f;
    if (adbus_iter_next(iter, &f))
        return -1;

    if (!lua_checkstack(L, 3))
        return -1;

    switch (f.type) {
        case ADBUS_BOOLEAN:
            lua_pushboolean(L, f.b);
            return 0;
        case ADBUS_UINT8:
            lua_pushnumber(L, (lua_Number) f.u8);
            return 0;
        case ADBUS_INT16:
            lua_pushnumber(L, (lua_Number) f.i16);
            return 0;
        case ADBUS_UINT16:
            lua_pushnumber(L, (lua_Number) f.u16);
            return 0;
        case ADBUS_INT32:
            lua_pushnumber(L, (lua_Number) f.i32);
            return 0;
        case ADBUS_UINT32:
            lua_pushnumber(L, (lua_Number) f.u32);
            return 0;
        case ADBUS_INT64:
            lua_pushnumber(L, (lua_Number) f.i64);
            return 0;
        case ADBUS_UINT64:
            lua_pushnumber(L, (lua_Number) f.u64);
            return 0;
        case ADBUS_DOUBLE:
            lua_pushnumber(L, (lua_Number) f.d);
            return 0;
        case ADBUS_STRING:
        case ADBUS_OBJECT_PATH:
        case ADBUS_SIGNATURE:
            lua_pushlstring(L, f.string, f.size);
            return 0;
        case ADBUS_ARRAY_BEGIN:
            return PushArray(L, iter, &f);
        case ADBUS_STRUCT_BEGIN:
            return PushStruct(L, iter, &f);
        case ADBUS_DICT_ENTRY_BEGIN:
            return PushDictEntry(L, iter, &f);
        case ADBUS_VARIANT_BEGIN:
            return PushVariant(L, iter, &f);
        default:
            return -1;
    }

}

/* ------------------------------------------------------------------------- */

int adbuslua_push_argument(
        lua_State*          L,
        adbus_Iterator*     iter)
{
    return PushNextField(L, iter);
}

/* ------------------------------------------------------------------------- */

static void SetStringField(
        lua_State*  L,
        int         table,
        const char* field,
        const char* string,
        size_t      size)
{
    if (!string)
        return;

    lua_pushlstring(L, string, size);
    lua_setfield(L, table, field);
}

/* ------------------------------------------------------------------------- */

int adbuslua_push_message(
    lua_State*          L,
    adbus_Message*      msg,
    adbus_Iterator*     iter)
{
    lua_newtable(L);
    int table = lua_gettop(L);

    adbus_MessageType type = adbus_msg_type(msg);
    if (   type < 0 
        || type > (sizeof(kTypes) / sizeof(kTypes[0]))
        || kTypes[type] == NULL
        || *kTypes[type] == '\0')
    {
        return -1;
    }

    size_t pathLen, interfaceLen, senderLen, destinationLen, memberLen, errorLen, sigLen;
    const char* path = adbus_msg_path(msg, &pathLen);
    const char* interface = adbus_msg_interface(msg, &interfaceLen);
    const char* sender = adbus_msg_sender(msg, &senderLen);
    const char* destination = adbus_msg_destination(msg, &destinationLen);
    const char* member = adbus_msg_member(msg, &memberLen);
    const char* error = adbus_msg_error(msg, &errorLen);
    const char* sig = adbus_msg_signature(msg, &sigLen);
    uint32_t serial = adbus_msg_serial(msg);

    lua_pushstring(L, kTypes[type]);
    lua_setfield(L, table, "type");

    lua_pushnumber(L, serial);
    lua_setfield(L, table, "serial");

    uint32_t reply;
    if (adbus_msg_reply(msg, &reply)) {
        lua_pushnumber(L, reply);
        lua_setfield(L, table, "reply_serial");
    }
    SetStringField(L, table, "path", path, pathLen);
    SetStringField(L, table, "interface", interface, interfaceLen);
    SetStringField(L, table, "sender", sender, senderLen);
    SetStringField(L, table, "destination", destination, destinationLen);
    SetStringField(L, table, "member", member, memberLen);
    SetStringField(L, table, "error_name", error, errorLen);
    SetStringField(L, table, "signature", sig, sigLen);

    adbus_msg_iterator(msg, iter);

    int arg = 1;
    while (!adbus_iter_isfinished(iter, 0)) {
        if (adbuslua_push_argument(L, iter)) {
            lua_settop(L, table -1);
            return -1;
        }

        lua_rawseti(L, table, arg++);
    }

    return 0;
}



