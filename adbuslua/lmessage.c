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

/* ------------------------------------------------------------------------- */

static int Error(lua_State* L, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    lua_pushvfstring(L, format, ap);
    va_end(ap);
    return -1;
}

/* ------------------------------------------------------------------------- */

static int AppendStruct(
    lua_State*      L,
    int             index,
    adbus_Buffer*   b)
{
    adbus_buf_beginstruct(b);

    int n = (int) lua_objlen(L, index);
    for (int i = 1; i <= n; ++i) {
        lua_rawgeti(L, index, i);
        int val = lua_gettop(L);

        if (adbuslua_to_argument(L, val, b))
            return -1;

        assert(lua_gettop(L) == val);
        lua_pop(L, 1);
    }

    adbus_buf_endstruct(b);

    return 0;
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
        lua_pcall(L, 1, 1, 0);
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
    if (!lua_isnil(L, metatable)) {
        lua_getfield(L, metatable, "__dbus_value");
        if (lua_isfunction(L, -1)) {
            lua_pushvalue(L, table);
            lua_pcall(L, 1, 1, 0);
        }

        if (!lua_isnil(L, -1))
            lua_replace(L, table);
    }
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
    adbus_Buffer*          b)
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
        return Error(L, "Can not convert argument to dbus variant");
    }

    adbus_BufVariant v;
    adbus_buf_beginvariant(b, &v, signature, -1);

    if (adbuslua_to_argument(L, index, b))
        return -1;

    adbus_buf_endvariant(b, &v);

    // signature may point to string on stack, so only clean up the stack
    // once we are done using the sig
    lua_settop(L, top);

    return 0;
}


/* ------------------------------------------------------------------------- */

static int AppendArray(
        lua_State*          L,
        int                 index,
        adbus_Buffer*       b)
{
    adbus_BufArray a;
    adbus_buf_beginarray(b, &a);

    int n = (int) lua_objlen(L, index);
    for (int i = 1; i <= n; ++i) {
        lua_rawgeti(L, index, i);
        int val = lua_gettop(L);

        adbus_buf_arrayentry(b, &a);
        if (adbuslua_to_argument(L, val, b))
            return -1;
        assert(lua_gettop(L) == val);
        lua_pop(L, 1);
    }

    adbus_buf_endarray(b, &a);

    return 0;
}

/* ------------------------------------------------------------------------- */

static int AppendMap(
        lua_State*          L,
        int                 index,
        adbus_Buffer*      b)
{
    adbus_BufArray a;
    adbus_buf_beginarray(b, &a);

    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        int key = lua_gettop(L) - 1;
        int val = lua_gettop(L);

        adbus_buf_arrayentry(b, &a);
        adbus_buf_begindictentry(b);
        if (adbuslua_to_argument(L, key, b))
            return -1;
        if (adbuslua_to_argument(L, val, b))
            return -1;
        if (*adbus_buf_sig(b, NULL) != '}')
            return Error(L, "Invalid signature");

        adbus_buf_enddictentry(b);

        lua_pop(L, 1); // pop the value, leaving the key
        assert(lua_gettop(L) == key);
    }

    adbus_buf_endarray(b, &a);

    return 0;
}

/* ------------------------------------------------------------------------- */

static const char kBooleanArg[] = "Expected boolean argument";
static const char kNumberArg[] = "Expected number argument";
static const char kStringArg[] = "Expected string argument";

int adbuslua_to_argument(
        lua_State*          L,
        int                 index,
        adbus_Buffer*      buffer)
{
    if (index < 0)
        index += lua_gettop(L) + 1;

    size_t size;
    const char* str;
    const char* sig = adbus_buf_signext(buffer, NULL);
    switch(*sig){
        case ADBUS_BOOLEAN:
            if (lua_type(L, index) != LUA_TBOOLEAN)
                return Error(L, "Invalid value - expected a boolean");
            adbus_buf_bool(buffer, lua_toboolean(L, index));
            return 0;

        case ADBUS_UINT8:
            if (lua_type(L, index) != LUA_TNUMBER)
                return Error(L, "Invalid value - expected a number");
            adbus_buf_u8(buffer, lua_tonumber(L, index));
            return 0;

        case ADBUS_INT16:
            if (lua_type(L, index) != LUA_TNUMBER)
                return Error(L, "Invalid value - expected a number");
            adbus_buf_i16(buffer, lua_tonumber(L, index));
            return 0;

        case ADBUS_UINT16:
            if (lua_type(L, index) != LUA_TNUMBER)
                return Error(L, "Invalid value - expected a number");
            adbus_buf_u16(buffer, lua_tonumber(L, index));
            return 0;

        case ADBUS_INT32:
            if (lua_type(L, index) != LUA_TNUMBER)
                return Error(L, "Invalid value - expected a number");
            adbus_buf_i32(buffer, lua_tonumber(L, index));
            return 0;

        case ADBUS_UINT32:
            if (lua_type(L, index) != LUA_TNUMBER)
                return Error(L, "Invalid value - expected a number");
            adbus_buf_u32(buffer, lua_tonumber(L, index));
            return 0;

        case ADBUS_INT64:
            if (lua_type(L, index) != LUA_TNUMBER)
                return Error(L, "Invalid value - expected a number");
            adbus_buf_i64(buffer, lua_tonumber(L, index));
            return 0;

        case ADBUS_UINT64:
            if (lua_type(L, index) != LUA_TNUMBER)
                return Error(L, "Invalid value - expected a number");
            adbus_buf_u64(buffer, lua_tonumber(L, index));
            return 0;

        case ADBUS_DOUBLE:
            if (lua_type(L, index) != LUA_TNUMBER)
                return Error(L, "Invalid value - expected a number");
            adbus_buf_double(buffer, (double) lua_tonumber(L, index));
            return 0;

        case ADBUS_STRING:
            if (lua_type(L, index) != LUA_TSTRING)
                return Error(L, "Invalid value - expected a string");
            str = lua_tolstring(L, index, &size);
            adbus_buf_string(buffer, str, (int) size);
            return 0;

        case ADBUS_OBJECT_PATH:
            if (lua_type(L, index) != LUA_TSTRING)
                return Error(L, "Invalid value - expected a string");
            str = lua_tolstring(L, index, &size);
            adbus_buf_objectpath(buffer, str, (int) size);
            return 0;

        case ADBUS_SIGNATURE:
            if (lua_type(L, index) != LUA_TSTRING)
                return Error(L, "Invalid value - expected a string");
            str = lua_tolstring(L, index, &size);
            adbus_buf_signature(buffer, str, (int) size);
            return 0;

        case ADBUS_ARRAY_BEGIN:
            if (sig[1] == '{')
                return AppendMap(L, index, buffer);
            else
                return AppendArray(L, index, buffer);

        case ADBUS_STRUCT_BEGIN:
            return AppendStruct(L, index, buffer);

        case ADBUS_VARIANT_BEGIN:
            return AppendVariant(L, index, buffer);

        default:
            return Error(L, "Invalid type in signature");
    }
}

/* ------------------------------------------------------------------------- */

static const char* kMessageFields[] = {
    "type",
    "no_reply",
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

int adbuslua_to_message(
        lua_State*              L,
        int                     index,
        adbus_MsgFactory*       msg)
{
    adbus_msg_reset(msg);
    if (adbusluaI_check_fields_numbers(L, index, kMessageFields)) {
        return Error(L,
                "Invalid field in the msg table. Valid fields are 'type', "
                "'no_reply', 'no_autostart', 'serial', 'interface', "
                "'path', 'member', 'error_name', 'reply_serial', 'destination', "
                "'sender', and 'signature'.");
    }

    // Type
    adbus_MessageType type;
    lua_getfield(L, index, "type");
    if (lua_isstring(L, -1) && strcmp(lua_tostring(L, -1), "method_call") == 0) {
        type = ADBUS_MSG_METHOD;
    } else if (lua_isstring(L, -1) && strcmp(lua_tostring(L, -1), "method_return") == 0) {
        type = ADBUS_MSG_RETURN;
    } else if (lua_isstring(L, -1) && strcmp(lua_tostring(L, -1), "error") == 0) {
        type = ADBUS_MSG_ERROR;
    } else if (lua_isstring(L, -1) && strcmp(lua_tostring(L, -1), "signal") == 0) {
        type = ADBUS_MSG_SIGNAL;
    } else {
        return Error(L, 
                "Error in 'type' field - expected 'method_call', "
                "'method_return', 'error', or 'signal'");
    }
    adbus_msg_settype(msg, type);
    lua_pop(L, 1);


    // Unpack fields
    adbus_Bool noreply = 0;
    adbus_Bool noautostart = 0;
    int64_t serial = -1;
    int64_t reply = -1;
    const char* path = NULL;
    const char* iface = NULL;
    const char* mbr = NULL;
    const char* errname = NULL;
    const char* dest = NULL;
    const char* sender = NULL;
    const char* sig = NULL;
    int pathsz, ifacesz, mbrsz, errsz, destsz, sendersz, sigsz;
    if (    adbusluaI_boolField(L, index, "no_reply", &noreply)
        ||  adbusluaI_boolField(L, index, "no_autostart", &noautostart)
        ||  adbusluaI_intField(L, index, "serial", &serial)
        ||  adbusluaI_intField(L, index, "reply_serial", &reply)
        ||  adbusluaI_stringField(L, index, "path", &path, &pathsz)
        ||  adbusluaI_stringField(L, index, "interface", &iface, &ifacesz)
        ||  adbusluaI_stringField(L, index, "member", &mbr, &mbrsz)
        ||  adbusluaI_stringField(L, index, "error_name", &errname, &errsz)
        ||  adbusluaI_stringField(L, index, "destination", &dest, &destsz)
        ||  adbusluaI_stringField(L, index, "sender", &sender, &sendersz)
        ||  adbusluaI_stringField(L, index, "signature", &sig, &sigsz))
    {
        return -1;
    }

    uint8_t flags = 0;
    if (noreply)
        flags |= ADBUS_MSG_NO_REPLY;
    if (noautostart)
        flags |= ADBUS_MSG_NO_AUTOSTART;
    adbus_msg_setflags(msg, flags);

    if (serial >= 0)
        adbus_msg_setserial(msg, (uint32_t) serial);
    if (reply >= 0)
        adbus_msg_setreply(msg, (uint32_t) reply);
    if (path)
        adbus_msg_setpath(msg, path, pathsz);
    if (iface)
        adbus_msg_setinterface(msg, iface, ifacesz);
    if (mbr)
        adbus_msg_setmember(msg, mbr, mbrsz);
    if (errname)
        adbus_msg_seterror(msg, errname, errsz);
    if (dest)
        adbus_msg_setdestination(msg, dest, destsz);
    if (sender)
        adbus_msg_setsender(msg, sender, sendersz);


    adbus_Buffer* b = adbus_msg_argbuffer(msg);

    // Arguments
    int argnum = (int) lua_objlen(L, index);
    if (argnum > 0) {
        if (!sig)
            return Error(L, "Missing 'signature' field - expected a string");

        adbus_buf_appendsig(b, sig, sigsz);
        for (int i = 1; i <= argnum; ++i) {
            lua_rawgeti(L, index, i);
            int arg = lua_gettop(L);
            UNUSED(arg);

            if (adbuslua_to_argument(L, -1, b))
                return -1;

            assert(lua_gettop(L) == arg);
            lua_pop(L, 1);
        }
    }

    switch(type)
    {
    case ADBUS_MSG_METHOD:
        if (!path)
            return Error(L, "Missing 'path' field - expected a string");
        if (!mbr)
            return Error(L, "Missing 'member' field - expected a string");
        break;

    case ADBUS_MSG_RETURN:
        if (reply == -1)
            return Error(L, "Missing 'reply_serial' field - expected a number");
        break;

    case ADBUS_MSG_ERROR:
        if (!errname)
            return Error(L, "Missing 'error_name' field - expected a string");
        break;

    case ADBUS_MSG_SIGNAL:
        if (!iface)
            return Error(L, "Missing 'interface' field - expected a string");
        if (!mbr)
            return Error(L, "Missing 'member' field - expected a string");
        break;

    default:
        assert(0);
        break;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

int adbuslua_to_message_unpacked(
        lua_State*          L,
        int                 begin,
        int                 end,
        const char*         sig,
        int                 sigsz,
        adbus_MsgFactory*   msg)
{
    adbus_msg_reset(msg);

    // Note begin-end is an inclusive range

    // No arguments to marshall
    if (end < begin)
        return 0;

    // A nil followed by a string is an error message with the string being
    // the error name, if there is an argument following that, it is the error
    // message
    if (begin + 1 <= end && lua_isnil(L, begin) && lua_isstring(L, begin + 1)) {
        adbus_msg_settype(msg, ADBUS_MSG_ERROR);
        size_t sz;
        const char* name = lua_tolstring(L, begin + 1, &sz);
        adbus_msg_seterror(msg, name, (int) sz);
        begin += 2;
    }

    adbus_Buffer* b = adbus_msg_argbuffer(msg);
    if (sig) {
        adbus_buf_appendsig(b, sig, sigsz);
    }
    for (int i = begin; i <= end; i++) {
        if (adbuslua_to_argument(L, i, b))
            return -1;
    }

    return 0;
}



/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

static int PushNextField(lua_State* L, adbus_Iterator* i);

// Structs are seen from lua indentically to an array of variants, ie they are
// just expanded into an array
static int PushStruct(lua_State* L, adbus_Iterator* i)
{
    if (adbus_iter_beginstruct(i))
        return -1;

    lua_newtable(L);
    int table = lua_gettop(L);
    int arg = 1;
    while (*i->sig != ADBUS_STRUCT_END) {
        if (PushNextField(L, i))
            return -1;

        lua_rawseti(L, table, arg++);
        assert(lua_gettop(L) == table);
    }

    if (adbus_iter_endstruct(i))
        return -1;

    return 0;
}

/* ------------------------------------------------------------------------- */

// Since lua is dynamically typed it does need to know that a particular
// argument was originally a variant
static int PushVariant(lua_State* L, adbus_Iterator* i)
{
    adbus_IterVariant v;
    if (adbus_iter_beginvariant(i, &v))
        return -1;
    if (PushNextField(L, i))
        return -1;
    if (adbus_iter_endvariant(i, &v))
        return -1;
    
    return 0;
}

/* ------------------------------------------------------------------------- */

// Arrays are pushed as standard lua arrays using 1 based indexes.
static int PushArray(lua_State* L, adbus_Iterator* i)
{
    adbus_IterArray a;
    if (adbus_iter_beginarray(i, &a))
        return -1;

    lua_newtable(L);
    int table = lua_gettop(L);
    int arg = 1;
    while (adbus_iter_inarray(i, &a)) {
        if (PushNextField(L, i))
            return -1;

        lua_rawseti(L, table, arg++);
        assert(lua_gettop(L) == table);
    }

    if (adbus_iter_endarray(i, &a))
        return -1;

    return 0;
}

/* ------------------------------------------------------------------------- */

static int PushMap(lua_State* L, adbus_Iterator* i)
{
    adbus_IterArray a;
    if (adbus_iter_beginarray(i, &a))
        return -1;

    lua_newtable(L);
    int table = lua_gettop(L);
    while (adbus_iter_inarray(i, &a)) {
        if (adbus_iter_begindictentry(i))
            return -1;

        // key
        if (PushNextField(L, i))
            return -1;

        // value
        if (PushNextField(L, i))
            return -1;

        if (adbus_iter_enddictentry(i))
            return -1;

        lua_settable(L, table);
        assert(lua_gettop(L) == table);
    }

    if (adbus_iter_endarray(i, &a))
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
static int PushNextField(lua_State* L, adbus_Iterator* i)
{
    const adbus_Bool*     b;
    const uint8_t*        u8;
    const int16_t*        i16;
    const uint16_t*       u16;
    const int32_t*        i32;
    const uint32_t*       u32;
    const int64_t*        i64;
    const uint64_t*       u64;
    const double*         d;
    const char*     string;
    size_t          size;

    if (!lua_checkstack(L, 3))
        return -1;

    switch (*i->sig)
    {
    case ADBUS_BOOLEAN:
        if (adbus_iter_bool(i, &b))
            return -1;
        lua_pushboolean(L, *b);
        return 0;
    case ADBUS_UINT8:
        if (adbus_iter_u8(i, &u8))
            return -1;
        lua_pushnumber(L, (lua_Number) *u8);
        return 0;
    case ADBUS_INT16:
        if (adbus_iter_i16(i, &i16))
            return -1;
        lua_pushnumber(L, (lua_Number) *i16);
        return 0;
    case ADBUS_UINT16:
        if (adbus_iter_u16(i, &u16))
            return -1;
        lua_pushnumber(L, (lua_Number) *u16);
        return 0;
    case ADBUS_INT32:
        if (adbus_iter_i32(i, &i32))
            return -1;
        lua_pushnumber(L, (lua_Number) *i32);
        return 0;
    case ADBUS_UINT32:
        if (adbus_iter_u32(i, &u32))
            return -1;
        lua_pushnumber(L, (lua_Number) *u32);
        return 0;
    case ADBUS_INT64:
        if (adbus_iter_i64(i, &i64))
            return -1;
        lua_pushnumber(L, (lua_Number) *i64);
        return 0;
    case ADBUS_UINT64:
        if (adbus_iter_u64(i, &u64))
            return -1;
        lua_pushnumber(L, (lua_Number) *u64);
        return 0;
    case ADBUS_DOUBLE:
        if (adbus_iter_double(i, &d))
            return -1;
        lua_pushnumber(L, (lua_Number) *d);
        return 0;
    case ADBUS_STRING:
        if (adbus_iter_string(i, &string, &size))
            return -1;
        lua_pushlstring(L, string, size);
        return 0;
    case ADBUS_OBJECT_PATH:
        if (adbus_iter_objectpath(i, &string, &size))
            return -1;
        lua_pushlstring(L, string, size);
        return 0;
    case ADBUS_SIGNATURE:
        if (adbus_iter_signature(i, &string, &size))
            return -1;
        lua_pushlstring(L, string, size);
        return 0;
    case ADBUS_ARRAY_BEGIN:
        if (i->sig[1] == ADBUS_DICTENTRY_BEGIN)
            return PushMap(L, i);
        else
            return PushArray(L, i);
    case ADBUS_STRUCT_BEGIN:
        return PushStruct(L, i);
    case ADBUS_VARIANT_BEGIN:
        return PushVariant(L, i);
    default:
        return -1;
    }
}

/* ------------------------------------------------------------------------- */

int adbuslua_push_argument(
        lua_State*          L,
        adbus_Iterator*     iter)
{
    int top = lua_gettop(L);
    if (PushNextField(L, iter)) {
        lua_settop(L, top);
        return -1;
    }
    return 0;
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

static int PushMessage(
        lua_State*              L,
        const adbus_Message*    msg)
{
    lua_newtable(L);
    int table = lua_gettop(L);

    if (msg->type == ADBUS_MSG_METHOD) {
        lua_pushstring(L, "method_call");
    } else if (msg->type == ADBUS_MSG_RETURN) {
        lua_pushstring(L, "method_return");
    } else if (msg->type == ADBUS_MSG_ERROR) {
        lua_pushstring(L, "error");
    } else if (msg->type == ADBUS_MSG_SIGNAL) {
        lua_pushstring(L, "signal");
    } else {
        return -1;
    }

    lua_setfield(L, table, "type");

    lua_pushnumber(L, msg->serial);
    lua_setfield(L, table, "serial");

    if (msg->replySerial) {
        lua_pushnumber(L, *msg->replySerial);
        lua_setfield(L, table, "reply_serial");
    }
    SetStringField(L, table, "path", msg->path, msg->pathSize);
    SetStringField(L, table, "interface", msg->interface, msg->interfaceSize);
    SetStringField(L, table, "sender", msg->sender, msg->senderSize);
    SetStringField(L, table, "destination", msg->destination, msg->destinationSize);
    SetStringField(L, table, "member", msg->member, msg->memberSize);
    SetStringField(L, table, "error", msg->error, msg->errorSize);
    SetStringField(L, table, "signature", msg->signature, msg->signatureSize);

    adbus_Iterator i;
    adbus_iter_args(&i, msg);
    int arg = 1;
    while (i.sig && *i.sig) {
        if (PushNextField(L, &i))
            return -1;
        lua_rawseti(L, table, arg++);
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

int adbuslua_push_message(
        lua_State*              L,
        const adbus_Message*    msg,
        adbus_Bool              unpack)
{
    int top = lua_gettop(L);
    if (unpack) {
        if (msg->type == ADBUS_MSG_ERROR) {
            lua_pushnil(L);
            lua_pushstring(L, msg->error);
        }

        adbus_Iterator i;
        adbus_iter_args(&i, msg);
        while (*i.sig) {
            if (PushNextField(L, &i))
                return -1;
        }

    } else {
        if (PushMessage(L, msg)) {
            lua_settop(L, top);
            return -1;
        }

    }
    return 0;
}



