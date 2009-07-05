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

#include "Message.h"


#include "Interface.h" //for LADBusCheckFields

#include "adbus/Marshaller.h"
#include "adbus/Message.h"

#include <assert.h>


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static const char* kMessageFields[] = {
    "type",
    "no_reply_expected",
    "no_auto_start",
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

// ----------------------------------------------------------------------------

// Uses the signature provided in the argument if not NULL, otherwise requires
// a list of signature strings in the 'signature' field of the table at index
void LADBusConvertLuaToMessage(
    lua_State*              L,
    int                     index,
    struct ADBusMarshaller* marshaller,
    const char*             signature,
    int                     signatureSize)
{
    if (LADBusCheckFields(L, index, 1, kMessageFields)) {
        luaL_error(L, "Invalid field in the message table");
        return;
    }

    if (signature) {
        ADBusSetSignature(marshaller, signature, signatureSize);
    } else {
        lua_getfield(L, index, "signature");
        if (!lua_istable(L, index)) {
            luaL_error(L, "Missing or invalid signature field of message table");
            return;
        }
    }

    int arg = 1;
    int sigtable = lua_gettop(L);
    while (1) {
        assert(lua_gettop(L) == sigtable);
        lua_rawgeti(L, index, arg);
        int argindex = lua_gettop(L);
        uint validArg = !lua_isnil(L, argindex);

        if (!signature) {
            lua_rawgeti(L, sigtable, arg);
            int sigindex = lua_gettop(L);
            uint validSig = !lua_isstring(L, sigindex);
            if (validSig != validArg) {
                luaL_error(L, "Mismatch between number of arguments and signature");
                return;
            }

            if (!validSig && !validArg)
                break;

            size_t sigLen;
            const char* sig = lua_tolstring(L, sigindex, &sigLen);

            ADBusSetSignature(marshaller, sig, (int) sigLen);
        } else if (!validArg) {
            break;
        }

        LADBusMarshallNextField(marshaller, L, argindex);
    }

}

// ----------------------------------------------------------------------------

// Note these strictly check the type because otherwise the lua_next call in
// LADBusMarshallArray screws up

uint CheckBoolean(lua_State* L, int index)
{
    if (lua_type(L, index) != LUA_TBOOLEAN) {
        luaL_error(L, "Mismatch between argument and signature");
        return 0;
    }
    return (uint) lua_toboolean(L, index);
}

lua_Number CheckNumber(lua_State* L, int index)
{
    if (lua_type(L, index) != LUA_TNUMBER) {
        luaL_error(L, "Mismatch between argument and signature");
        return 0;
    }
    return lua_tonumber(L, index);
}

const char* CheckString(lua_State* L, int index, size_t* size)
{
    if (lua_type(L, index) != LUA_TSTRING) {
        luaL_error(L, "Mismatch between argument and signature");
        return NULL;
    }
    return lua_tolstring(L, index, size);
}

// ----------------------------------------------------------------------------

void LADBusMarshallNextField(
    struct ADBusMarshaller* marshaller,
    lua_State*              L,
    int                     index)
{
    uint b;
    lua_Number n;
    size_t size;
    const char* str;
    switch(*ADBusMarshallerCurrentSignature(marshaller)){
        case ADBusBooleanField:
            b = CheckBoolean(L, index);
            ADBusAppendBoolean(marshaller, b);
            break;
        case ADBusUInt8Field:
            n = CheckNumber(L, index);
            ADBusAppendUInt8(marshaller, (uint8_t) n);
            break;
        case ADBusInt16Field:
            n = CheckNumber(L, index);
            ADBusAppendInt32(marshaller, (int16_t) n);
            break;
        case ADBusUInt16Field:
            n = CheckNumber(L, index);
            ADBusAppendUInt16(marshaller, (uint16_t) n);
            break;
        case ADBusInt32Field:
            n = CheckNumber(L, index);
            ADBusAppendInt32(marshaller, (int32_t) n);
            break;
        case ADBusUInt32Field:
            n = CheckNumber(L, index);
            ADBusAppendUInt32(marshaller, (uint32_t) n);
            break;
        case ADBusInt64Field:
            n = CheckNumber(L, index);
            ADBusAppendUInt64(marshaller, (int64_t) n);
            break;
        case ADBusUInt64Field:
            n = CheckNumber(L, index);
            ADBusAppendUInt64(marshaller, (uint64_t) n);
            break;
        case ADBusDoubleField:
            n = CheckNumber(L, index);
            ADBusAppendDouble(marshaller, (double) n);
            break;
        case ADBusStringField:
            str = CheckString(L, index, &size);
            ADBusAppendString(marshaller, str, (int) size);
            break;
        case ADBusObjectPathField:
            str = CheckString(L, index, &size);
            ADBusAppendObjectPath(marshaller, str, (int) size);
            break;
        case ADBusSignatureField:
            str = CheckString(L, index, &size);
            ADBusAppendSignature(marshaller, str, (int) size);
            break;
        case ADBusArrayBeginField:
            LADBusMarshallArray(marshaller, L, index);
            break;
        case ADBusStructBeginField:
            LADBusMarshallStruct(marshaller, L, index);
            break;
        case ADBusVariantBeginField:
            LADBusMarshallVariant(marshaller, L, index);
            break;
        default:
            luaL_error(L, "Invalid signature on marshalling message");
            return;
    }
}

// ----------------------------------------------------------------------------

void LADBusMarshallStruct(
    struct ADBusMarshaller* marshaller,
    lua_State*              L,
    int                     index)
{
    ADBusBeginStruct(marshaller);

    int i = 1;
    while (1) {
        lua_rawgeti(L, index, i);
        int valueIndex = lua_gettop(L);
        if (lua_isnil(L, valueIndex))
            break;

        LADBusMarshallNextField(marshaller, L, valueIndex);
        assert(lua_gettop(L) == valueIndex);
    }
    lua_pop(L, 1);

    ADBusEndStruct(marshaller);
}

// ----------------------------------------------------------------------------

void LADBusMarshallVariant(
    struct ADBusMarshaller* marshaller,
    lua_State*              L,
    int                     index)
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
                lua_getfield(L, index, "__dbus_signature");
                if (!lua_isfunction(L, -1)) {
                    luaL_error(L, "Can not convert argument to dbus "
                            "variant. Non simple types need to overload the "
                            "__dbus_signature field that returns the variant "
                            "dbus signature as a string.");
                    return;
                }
                lua_pushvalue(L, index);
                lua_call(L, 1, 1);
                if (!lua_isstring(L, -1)) {
                    luaL_error(L, "Can not convert argument to dbus "
                            "variant. Non simple types need to overload the "
                            "__dbus_signature field that returns the variant "
                            "dbus signature as a string.");
                    return;
                }
                signature = lua_tostring(L, 1);
            }
            break;
        case LUA_TUSERDATA:
        case LUA_TNIL:
        case LUA_TLIGHTUSERDATA:
        case LUA_TFUNCTION:
        case LUA_TTHREAD:
        default:
            luaL_error(L, "Can not convert argument to dbus variant.");
            return;
    }
    ADBusBeginVariant(marshaller, signature, -1);
    lua_settop(L, top);
    LADBusMarshallNextField(marshaller, L, index);
    ADBusEndVariant(marshaller);
}

// ----------------------------------------------------------------------------

void LADBusMarshallArray(
    struct ADBusMarshaller* marshaller,
    lua_State*              L,
    int                     index)
{
    ADBusBeginArray(marshaller);

    char nextField = *ADBusMarshallerCurrentSignature(marshaller);

    if (nextField == ADBusDictEntryBeginField) {
        lua_pushnil(L);
        while (lua_next(L, index) != 0) {
            int keyIndex   = lua_gettop(L) - 1;
            int valueIndex = lua_gettop(L);
            ADBusBeginDictEntry(marshaller);

            LADBusMarshallNextField(marshaller, L, keyIndex);
            LADBusMarshallNextField(marshaller, L, valueIndex);

            ADBusEndDictEntry(marshaller);
            lua_pop(L, 1);
            assert(lua_gettop(L) == keyIndex);
        }
    } else {
        int i = 1;
        while (1) {
            lua_rawgeti(L, index, i);
            int valueIndex = lua_gettop(L);
            if (lua_isnil(L, valueIndex))
                break;

            LADBusMarshallNextField(marshaller, L, valueIndex);
            assert(lua_gettop(L) == valueIndex);
        }
        lua_pop(L, 1);
    }

    ADBusEndArray(marshaller);
}

// ----------------------------------------------------------------------------






// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static const char* kMessageTypes[] = {
    "invalid",
    "method_call",
    "method_return",
    "error",
    "signal"
};
static size_t kMessageTypeNum = 5;

// ----------------------------------------------------------------------------

static void SetStringField(
        lua_State* L,
        int table,
        const char* fieldName,
        const char* string,
        int size)
{
    if (!string)
        return;

    lua_pushlstring(L, string, size);
    lua_setfield(L, table, fieldName);
}

// ----------------------------------------------------------------------------

int LADBusConvertMessageToLua(
    struct ADBusMessage*    message,
    lua_State*              L)
{
    lua_newtable(L);
    int table = lua_gettop(L);
    lua_newtable(L);
    int sigtable = lua_gettop(L);

    enum ADBusMessageType type = ADBusGetMessageType(message);
    if (type >= kMessageTypeNum || type <= ADBusInvalidMessage)
        return ADBusInternalError;

    int pathLen, interfaceLen, senderLen, destinationLen, memberLen, errorLen;
    const char* path = ADBusGetPath(message, &pathLen);
    const char* interface = ADBusGetInterface(message, &interfaceLen);
    const char* sender = ADBusGetSender(message, &senderLen);
    const char* destination = ADBusGetDestination(message, &destinationLen);
    const char* member = ADBusGetMember(message, &memberLen);
    const char* error = ADBusGetErrorName(message, &errorLen);
    uint32_t serial = ADBusGetSerial(message);
    uint hasReplySerial = ADBusHasReplySerial(message);
    uint32_t replySerial = ADBusGetReplySerial(message);

    lua_pushstring(L, kMessageTypes[type]);
    lua_setfield(L, table, "type");

    lua_pushnumber(L, serial);
    lua_setfield(L, table, "serial");

    if (hasReplySerial) {
        lua_pushnumber(L, replySerial);
        lua_setfield(L, table, "reply_serial");
    }
    SetStringField(L, table, "path", path, pathLen);
    SetStringField(L, table, "interface", interface, interfaceLen);
    SetStringField(L, table, "sender", sender, senderLen);
    SetStringField(L, table, "destination", destination, destinationLen);
    SetStringField(L, table, "member", member, memberLen);
    SetStringField(L, table, "error_name", error, errorLen);


    ADBusReparseMessage(message);

    int err = 0;
    int argnum = 1;
    const char* sig = ADBusGetSignatureRemaining(message);
    while (!err && !ADBusIsScopeAtEnd(message, 0)) {
        err = LADBusPushNextField(message, L);
        lua_rawseti(L, table, argnum);

        const char* nextsig = ADBusGetSignatureRemaining(message);
        lua_pushlstring(L, sig, nextsig - sig);
        lua_rawseti(L, sigtable, argnum);

        assert(lua_gettop(L) == sigtable);
        argnum++;
    }

    if (err) {
        lua_settop(L, table - 1);
        return err;
    }

    assert(lua_gettop(L) == sigtable);
    lua_setfield(L, table, "signature");
    
    return 0;
}

// ----------------------------------------------------------------------------

// Note depending on the type of lua_Number, some or all of the numeric dbus
// types may lose data on the conversion - for now there is no decent way
// around this
// Also all of the string types (string, object path, signature) all convert
// to a lua string since there is no reasonable reason for them to be
// different types (this could be changed to fancy types that overload
// __tostring, but I don't really see the point of that)
int LADBusPushNextField(
    struct ADBusMessage*    message,
    lua_State*              L)
{
    struct ADBusField f;
    int err = ADBusTakeField(message, &f);
    if (err) return err;

    // If we cannot grow the stack then the message is most likely trying to
    // force our memory limit
    if (!lua_checkstack(L, 3))
        return ADBusInvalidData;

    switch (f.type) {
        case ADBusBooleanField:
            lua_pushboolean(L, f.data.b);
            return 0;
        case ADBusUInt8Field:
            lua_pushnumber(L, (lua_Number) f.data.u8);
            return 0;
        case ADBusInt16Field:
            lua_pushnumber(L, (lua_Number) f.data.i16);
            return 0;
        case ADBusUInt16Field:
            lua_pushnumber(L, (lua_Number) f.data.u16);
            return 0;
        case ADBusInt32Field:
            lua_pushnumber(L, (lua_Number) f.data.i32);
            return 0;
        case ADBusUInt32Field:
            lua_pushnumber(L, (lua_Number) f.data.u32);
            return 0;
        case ADBusInt64Field:
            lua_pushnumber(L, (lua_Number) f.data.i64);
            return 0;
        case ADBusUInt64Field:
            lua_pushnumber(L, (lua_Number) f.data.u64);
            return 0;
        case ADBusDoubleField:
            lua_pushnumber(L, (lua_Number) f.data.d);
            return 0;
        case ADBusStringField:
        case ADBusObjectPathField:
        case ADBusSignatureField:
            lua_pushlstring(L, f.data.string.str, f.data.string.size);
            return 0;
        case ADBusArrayBeginField:
            return LADBusPushArray(message, L, &f);
        case ADBusStructBeginField:
            return LADBusPushStruct(message, L, &f);
        case ADBusDictEntryBeginField:
            return LADBusPushDictEntry(message, L, &f);
        case ADBusVariantBeginField:
            return LADBusPushVariant(message, L, &f);
        default:
            return ADBusInvalidData;
    }

}

// ----------------------------------------------------------------------------

// Structs are seen from lua indentically to an array of variants, ie they are
// just expanded into an array
int LADBusPushStruct(
    struct ADBusMessage*    message,
    lua_State*              L,
    struct ADBusField*      field)
{
    lua_newtable(L);
    int table = lua_gettop(L);
    int i = 1;
    int err = 0;
    while (!ADBusIsScopeAtEnd(message, field->scope)) {
        err = LADBusPushNextField(message, L);
        if (err) return err;

        assert(lua_gettop(L) == table + 1);
        lua_rawseti(L, table, i++);
    }
    return ADBusTakeStructEnd(message);
}

// ----------------------------------------------------------------------------

int LADBusPushDictEntry(
    struct ADBusMessage*    message,
    lua_State*              L,
    struct ADBusField*      field)
{
    int table = lua_gettop(L);
    assert(lua_istable(L, table));

    int err = LADBusPushNextField(message, L);
    if (err) return err;

    int key = lua_gettop(L);

    err = LADBusPushNextField(message, L);
    if (err) return err;

    int value = lua_gettop(L);
    assert(value == key + 1);

    // Pops the two last items on the stack as the key and value
    lua_settable(L, table);

    return 0;
}

// ----------------------------------------------------------------------------

// Since lua is dynamically typed it does need to know that a particular
// argument was originally a variant
int LADBusPushVariant(
    struct ADBusMessage*    message,
    lua_State*              L,
    struct ADBusField*      field)
{
    int err = 0;
    while (!ADBusIsScopeAtEnd(message, field->scope)) {
        err = LADBusPushNextField(message, L);
        if (err) return err;
    }
    return ADBusTakeVariantEnd(message);
}

// ----------------------------------------------------------------------------

// Arrays are pushed as standard lua arrays using 1 based indexes.
// Note since maps come up from dbus as a{xx} ie an array of dict entries we
// should only set the index if the inner type was not a dict entry
int LADBusPushArray(
    struct ADBusMessage*    message,
    lua_State*              L,
    struct ADBusField*      field)
{
    lua_newtable(L);
    int table = lua_gettop(L);
    int i = 1;
    int err = 0;
    while (!ADBusIsScopeAtEnd(message, field->scope)) {
        err = LADBusPushNextField(message, L);
        if (err) return err;

        // No value is left on the stack if the inner type was a dict entry
        if (lua_gettop(L) == table)
            continue;

        assert(lua_gettop(L) == table + 1);
        lua_rawseti(L, table, i++);
    }
    return ADBusTakeArrayEnd(message);
}

// ----------------------------------------------------------------------------


