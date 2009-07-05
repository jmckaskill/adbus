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

#pragma once

#include "adbus/Marshaller.h"
#include "adbus/Message.h"

#include <lua.h>
#include <lauxlib.h>

void LADBusConvertLuaToMessage(
    lua_State*              L,
    int                     index,
    struct ADBusMarshaller* marshaller,
    const char*             signature,
    int                     signatureSie);

void LADBusMarshallNextField(
    struct ADBusMarshaller* marshaller,
    lua_State*              L,
    int                     index);

void LADBusMarshallStruct(
    struct ADBusMarshaller* marshaller,
    lua_State*              L,
    int                     index);

void LADBusMarshallVariant(
    struct ADBusMarshaller* marshaller,
    lua_State*              L,
    int                     index);

void LADBusMarshallArray(
    struct ADBusMarshaller* marshaller,
    lua_State*              L,
    int                     index);




int LADBusConvertMessageToLua(
    struct ADBusMessage*    message,
    lua_State*              L);

int LADBusPushNextField(
    struct ADBusMessage*    message,
    lua_State*              L);

int LADBusPushStruct(
    struct ADBusMessage*    message,
    lua_State*              L,
    struct ADBusField*      field);

int LADBusPushDictEntry(
    struct ADBusMessage*    message,
    lua_State*              L,
    struct ADBusField*      field);

int LADBusPushVariant(
    struct ADBusMessage*    message,
    lua_State*              L,
    struct ADBusField*      field);

int LADBusPushArray(
    struct ADBusMessage*    message,
    lua_State*              L,
    struct ADBusField*      field);


