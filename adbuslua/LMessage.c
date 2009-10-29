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

#include "LMessage.h"


#include "LInterface.h" //for LADBusCheckFields

#include "adbus/Iterator.h"
#include "adbus/Marshaller.h"
#include "adbus/Message.h"

#include <assert.h>

#ifdef WIN32
#   pragma warning(disable:4267) // conversion from size_t to int
#endif

static int MarshallNextField(
    lua_State*              L,
    int                     index,
    struct ADBusMarshaller* marshaller);

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static int MarshallStruct(
    lua_State*              L,
    int                     index,
    struct ADBusMarshaller* marshaller)
{
    ADBusBeginStruct(marshaller);

    int n = lua_objlen(L, index);
    for (int i = 1; i <= n; ++i) {
        lua_rawgeti(L, index, i);
        int valueIndex = lua_gettop(L);

        MarshallNextField(L, valueIndex, marshaller);

        assert(lua_gettop(L) == valueIndex);
        lua_pop(L, 1);
    }

    ADBusEndStruct(marshaller);
    return 0;
}

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

static int MarshallVariant(
    lua_State*              L,
    int                     index,
    struct ADBusMarshaller* marshaller)
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
    ADBusBeginVariant(marshaller, signature, -1);
    // signature may point to string on stack, so only clean up the stack
    // once we are done using the sig
    lua_settop(L, top);
    MarshallNextField(L, index, marshaller);
    ADBusEndVariant(marshaller);
    return 0;
}


// ----------------------------------------------------------------------------

static int MarshallArray(
    lua_State*              L,
    int                     index,
    struct ADBusMarshaller* marshaller)
{
    ADBusBeginArray(marshaller);

    if (ADBusNextMarshallerField(marshaller) == ADBusDictEntryBeginField) {
        lua_pushnil(L);
        while (lua_next(L, index) != 0) {
            int keyIndex   = lua_gettop(L) - 1;
            int valueIndex = lua_gettop(L);
            ADBusBeginDictEntry(marshaller);

            MarshallNextField(L, keyIndex, marshaller);
            MarshallNextField(L, valueIndex, marshaller);

            ADBusEndDictEntry(marshaller);
            lua_pop(L, 1); // pop the value, leaving the key
            assert(lua_gettop(L) == keyIndex);
        }
    } else {
        int n = lua_objlen(L, index);
        for (int i = 1; i <= n; ++i) {
            lua_rawgeti(L, index, i);
            int valueIndex = lua_gettop(L);

            MarshallNextField(L, valueIndex, marshaller);
            assert(lua_gettop(L) == valueIndex);
            lua_pop(L, 1);
        }
    }

    ADBusEndArray(marshaller);
    return 0;
}

// ----------------------------------------------------------------------------

// Note these strictly check the type strictly because otherwise the lua_next
// call in LADBusMarshallArray screws up

static uint CheckBoolean(lua_State* L, int index)
{
    if (lua_type(L, index) != LUA_TBOOLEAN) {
        luaL_error(L, "Mismatch between argument and signature");
        return 0;
    }
    return (uint) lua_toboolean(L, index);
}

static lua_Number CheckNumber(lua_State* L, int index)
{
    if (lua_type(L, index) != LUA_TNUMBER) {
        luaL_error(L, "Mismatch between argument and signature");
        return 0;
    }
    return lua_tonumber(L, index);
}

static const char* CheckString(lua_State* L, int index, size_t* size)
{
    if (lua_type(L, index) != LUA_TSTRING) {
        luaL_error(L, "Mismatch between argument and signature");
        return NULL;
    }
    return lua_tolstring(L, index, size);
}

// ----------------------------------------------------------------------------

static int MarshallNextField(
    lua_State*              L,
    int                     index,
    struct ADBusMarshaller* marshaller)
{
    uint b;
    lua_Number n;
    size_t size;
    const char* str;
    int err = 0;
    switch(ADBusNextMarshallerField(marshaller)){
        case ADBusBooleanField:
            b = CheckBoolean(L, index);
            err = ADBusAppendBoolean(marshaller, b);
            break;
        case ADBusUInt8Field:
            n = CheckNumber(L, index);
            err = ADBusAppendUInt8(marshaller, (uint8_t) n);
            break;
        case ADBusInt16Field:
            n = CheckNumber(L, index);
            err = ADBusAppendInt32(marshaller, (int16_t) n);
            break;
        case ADBusUInt16Field:
            n = CheckNumber(L, index);
            err = ADBusAppendUInt16(marshaller, (uint16_t) n);
            break;
        case ADBusInt32Field:
            n = CheckNumber(L, index);
            err = ADBusAppendInt32(marshaller, (int32_t) n);
            break;
        case ADBusUInt32Field:
            n = CheckNumber(L, index);
            err = ADBusAppendUInt32(marshaller, (uint32_t) n);
            break;
        case ADBusInt64Field:
            n = CheckNumber(L, index);
            err = ADBusAppendUInt64(marshaller, (int64_t) n);
            break;
        case ADBusUInt64Field:
            n = CheckNumber(L, index);
            err = ADBusAppendUInt64(marshaller, (uint64_t) n);
            break;
        case ADBusDoubleField:
            n = CheckNumber(L, index);
            err = ADBusAppendDouble(marshaller, (double) n);
            break;
        case ADBusStringField:
            str = CheckString(L, index, &size);
            err = ADBusAppendString(marshaller, str, (int) size);
            break;
        case ADBusObjectPathField:
            str = CheckString(L, index, &size);
            err = ADBusAppendObjectPath(marshaller, str, (int) size);
            break;
        case ADBusSignatureField:
            str = CheckString(L, index, &size);
            err = ADBusAppendSignature(marshaller, str, (int) size);
            break;
        case ADBusArrayBeginField:
            // This covers both arrays and maps
            err = MarshallArray(L, index, marshaller);
            break;
        case ADBusStructBeginField:
            err = MarshallStruct(L, index, marshaller);
            break;
        case ADBusVariantBeginField:
            err = MarshallVariant(L, index, marshaller);
            break;
        default:
            return luaL_error(L, "Invalid signature on marshalling message");
    }
    if (err)
        return luaL_error(L, "Error on marshalling message");
    return 0;
}

// ----------------------------------------------------------------------------

int LADBusMarshallArgument(
        lua_State*              L,
        int                     argumentIndex,
        const char*             signature,
        int                     signatureSize,
        struct ADBusMarshaller* marshaller)
{
    int err = ADBusAppendArguments(marshaller, signature, signatureSize);
    if (err)
        return err;

    return MarshallNextField(L, argumentIndex, marshaller);
}

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

static const char* kMessageTypes[] = {
    "invalid",
    "method_call",
    "method_return",
    "error",
    "signal",
    NULL,
};
static size_t kMessageTypeNum = 5;

// ----------------------------------------------------------------------------

typedef void (*SetStringFunction)(struct ADBusMessage*, const char*, int);
static void SetStringHeader(
        lua_State*              L,
        int                     index,
        struct ADBusMessage*    message,
        const char*             field,
        SetStringFunction       function)
{
    lua_getfield(L, index, field);
    if (!lua_isnil(L, -1)) {
        size_t size;
        const char* string = luaL_checklstring(L, -1, &size);
        function(message, string, (int) size);
    }
    lua_pop(L, 1);
}

int LADBusMarshallMessage(
    lua_State*              L,
    int                     messageIndex,
    struct ADBusMessage*    message)
{
    ADBusResetMessage(message);
    if (LADBusCheckFieldsAllowNumbers(L, messageIndex, kMessageFields)) {
        return luaL_error(L, "Invalid field in the message table");
    }
    int luaTopAtStart = lua_gettop(L);

    // Type
    lua_getfield(L, messageIndex, "type");
    int type = luaL_checkoption(L, -1, "invalid", kMessageTypes);
    if (type != ADBusInvalidMessage) {
        ADBusSetMessageType(message, (enum ADBusMessageType) type);
    }
    lua_pop(L, 1);


    // Flags
    uint8_t flags = 0;
    lua_getfield(L, messageIndex, "no_reply_expected");
    if (!lua_isnil(L, -1)) {
        flags |= luaL_checkint(L, -1) ? ADBusNoReplyExpectedFlag : 0;
    }
    lua_getfield(L, messageIndex, "no_auto_start");
    if (!lua_isnil(L, -1)) {
        flags |= luaL_checkint(L, -1) ? ADBusNoAutoStartFlag : 0;
    }
    ADBusSetFlags(message, flags);
    lua_pop(L, 2);

    // Serial
    lua_getfield(L, messageIndex, "serial");
    if (!lua_isnil(L, -1))
        ADBusSetSerial(message, luaL_checkint(L, -1));

    lua_getfield(L, messageIndex, "reply_serial");
    if (!lua_isnil(L, -1))
        ADBusSetReplySerial(message, luaL_checkint(L, -1));

    lua_pop(L, 2);

    // String fields
    SetStringHeader(L, messageIndex, message, "path", &ADBusSetPath);
    SetStringHeader(L, messageIndex, message, "interface", &ADBusSetInterface);
    SetStringHeader(L, messageIndex, message, "member", &ADBusSetMember);
    SetStringHeader(L, messageIndex, message, "error_name", &ADBusSetErrorName);
    SetStringHeader(L, messageIndex, message, "destination", &ADBusSetDestination);
    SetStringHeader(L, messageIndex, message, "sender", &ADBusSetSender);

    // Signature
    lua_getfield(L, messageIndex, "signature");
    int signatureTable = lua_gettop(L);
    if (!lua_istable(L, signatureTable) && !lua_isnil(L, signatureTable)) {
        return luaL_error(L, "Invalid signature table of message table");
    }

    // Check signature table size
    size_t argumentNum = lua_objlen(L, messageIndex);
    size_t signatureNum = lua_istable(L, signatureTable)
                        ? lua_objlen(L, signatureTable)
                        : 0;
    if (signatureNum != argumentNum) {
        return luaL_error(L, "Mismatch between number of arguments and "
                          "signature");
    }

    // Arguments
    struct ADBusMarshaller* marshaller = ADBusArgumentMarshaller(message);
    for (int i = 1; i <= (int) argumentNum; ++i) {
#ifndef NDEBUG
        int top = lua_gettop(L);
#endif

        // Get the argument signature
        lua_rawgeti(L, signatureTable, i);
        size_t signatureSize;
        const char* signature = luaL_checklstring(L, -1, &signatureSize);

        // Get the argument
        lua_rawgeti(L, messageIndex, i);

        // Marshall the argument
        int err = LADBusMarshallArgument(L,
                                         lua_gettop(L),
                                         signature,
                                         signatureSize,
                                         marshaller);
        if (err)
            return luaL_error(L, "Error on marshalling message");

        // Pop the argument and signature
        lua_pop(L, 2);
        assert(lua_gettop(L) == top);
    }

    lua_settop(L, luaTopAtStart);
    return 0;
}






// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static int PushNextField(
        lua_State*              L,
        struct ADBusIterator*   iterator);

// Structs are seen from lua indentically to an array of variants, ie they are
// just expanded into an array
static int PushStruct(
    lua_State*              L,
    struct ADBusIterator*   iterator,
    struct ADBusField*      field)
{
    lua_newtable(L);
    int table = lua_gettop(L);
    int i = 1;
    int err = 0;
    while (!ADBusIsScopeAtEnd(iterator, field->scope)) {
        err = PushNextField(L, iterator);
        if (err) return err;

        assert(lua_gettop(L) == table + 1);
        lua_rawseti(L, table, i++);
    }
    return err
        || ADBusIterate(iterator, field)
        || field->type != ADBusStructEndField;
}

// ----------------------------------------------------------------------------

static int PushDictEntry(
    lua_State*              L,
    struct ADBusIterator*   iterator,
    struct ADBusField*      field)
{
    (void) field;
    // DBus maps are arrays of dict entries which map directly to a table in
    // lua, so we should be able to get the surrounding array since its on top
    // of the lua stack

    int table = lua_gettop(L);
    assert(lua_istable(L, table));

    int err = PushNextField(L, iterator);
    if (err) return err;

#ifndef NDEBUG
    int key = lua_gettop(L);
#endif

    err = PushNextField(L, iterator);
    if (err) return err;

#ifndef NDEBUG
    int value = lua_gettop(L);
    assert(value == key + 1);
#endif

    // Pops the two last items on the stack - the key and value
    lua_settable(L, table);

    return 0;
}

// ----------------------------------------------------------------------------

// Since lua is dynamically typed it does need to know that a particular
// argument was originally a variant
static int PushVariant(
    lua_State*              L,
    struct ADBusIterator*   iterator,
    struct ADBusField*      field)
{
    int err = 0;
    while (!ADBusIsScopeAtEnd(iterator, field->scope)) {
        err = PushNextField(L, iterator);
        if (err) return err;
    }
    return err
        || ADBusIterate(iterator, field)
        || field->type != ADBusVariantEndField;
}

// ----------------------------------------------------------------------------

// Arrays are pushed as standard lua arrays using 1 based indexes.
// Note since maps come up from dbus as a{xx} ie an array of dict entries we
// should only set the index if the inner type was not a dict entry
static int PushArray(
    lua_State*              L,
    struct ADBusIterator*   iterator,
    struct ADBusField*      field)
{
    lua_newtable(L);
    int table = lua_gettop(L);
    int i = 1;
    int err = 0;
    while (!ADBusIsScopeAtEnd(iterator, field->scope)) {
        err = PushNextField(L, iterator);
        if (err) return err;

        // No value is left on the stack if the inner type was a dict entry
        if (lua_gettop(L) == table)
            continue;

        assert(lua_gettop(L) == table + 1);
        lua_rawseti(L, table, i++);
    }
    return err
        || ADBusIterate(iterator, field)
        || field->type != ADBusArrayEndField;
}

// ----------------------------------------------------------------------------

// Note depending on the type of lua_Number, some or all of the numeric dbus
// types may lose data on the conversion - for now there is no decent way
// around this
// Also all of the string types (string, object path, signature) all convert
// to a lua string since there is no reasonable reason for them to be
// different types (this could be changed to fancy types that overload
// __tostring, but I don't really see the point of that)
static int PushNextField(
    lua_State*              L,
    struct ADBusIterator*    iterator)
{
    struct ADBusField f;
    int err = ADBusIterate(iterator, &f);
    if (err) return err;

    // If we cannot grow the stack then the message is most likely trying to
    // force our memory limit
    if (!lua_checkstack(L, 3))
        return ADBusInvalidData;

    switch (f.type) {
        case ADBusBooleanField:
            lua_pushboolean(L, f.b);
            return 0;
        case ADBusUInt8Field:
            lua_pushnumber(L, (lua_Number) f.u8);
            return 0;
        case ADBusInt16Field:
            lua_pushnumber(L, (lua_Number) f.i16);
            return 0;
        case ADBusUInt16Field:
            lua_pushnumber(L, (lua_Number) f.u16);
            return 0;
        case ADBusInt32Field:
            lua_pushnumber(L, (lua_Number) f.i32);
            return 0;
        case ADBusUInt32Field:
            lua_pushnumber(L, (lua_Number) f.u32);
            return 0;
        case ADBusInt64Field:
            lua_pushnumber(L, (lua_Number) f.i64);
            return 0;
        case ADBusUInt64Field:
            lua_pushnumber(L, (lua_Number) f.u64);
            return 0;
        case ADBusDoubleField:
            lua_pushnumber(L, (lua_Number) f.d);
            return 0;
        case ADBusStringField:
        case ADBusObjectPathField:
        case ADBusSignatureField:
            lua_pushlstring(L, f.string, f.size);
            return 0;
        case ADBusArrayBeginField:
            return PushArray(L, iterator, &f);
        case ADBusStructBeginField:
            return PushStruct(L, iterator, &f);
        case ADBusDictEntryBeginField:
            return PushDictEntry(L, iterator, &f);
        case ADBusVariantBeginField:
            return PushVariant(L, iterator, &f);
        default:
            return ADBusInvalidData;
    }

}

// ----------------------------------------------------------------------------

int LADBusPushArgument(
        lua_State*              L,
        struct ADBusIterator*   iterator)
{
    return PushNextField(L, iterator);
}

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

int LADBusPushMessage(
    lua_State*              L,
    struct ADBusMessage*    message,
    struct ADBusIterator*   iterator)
{
    lua_newtable(L);
    int table = lua_gettop(L);

    enum ADBusMessageType type = ADBusGetMessageType(message);
    if (type >= (enum ADBusMessageType) kMessageTypeNum || type <= ADBusInvalidMessage)
        return ADBusInternalError;

    size_t pathLen, interfaceLen, senderLen, destinationLen, memberLen, errorLen, sigLen;
    const char* path = ADBusGetPath(message, &pathLen);
    const char* interface = ADBusGetInterface(message, &interfaceLen);
    const char* sender = ADBusGetSender(message, &senderLen);
    const char* destination = ADBusGetDestination(message, &destinationLen);
    const char* member = ADBusGetMember(message, &memberLen);
    const char* error = ADBusGetErrorName(message, &errorLen);
    const char* sig = ADBusGetSignature(message, &sigLen);
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
    SetStringField(L, table, "signature", sig, sigLen);

    int err = 0;
    int argnum = 1;
    while (!err && !ADBusIsScopeAtEnd(iterator, 0)) {
        err = LADBusPushArgument(L, iterator);
        lua_rawseti(L, table, argnum);

        argnum++;
    }

    if (err) {
        lua_settop(L, table - 1);
        return err;
    }

    return 0;
}

// ----------------------------------------------------------------------------
