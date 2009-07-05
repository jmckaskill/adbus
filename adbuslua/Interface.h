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

#include "ADBusLua.h"
#include "Data.h"

#include "adbus/Connection.h"
#include <lua.h>
#include <lauxlib.h>

struct LADBusInterface
{
    struct ADBusInterface* interface;
    int nameRef;
};

int LADBusCreateInterface(lua_State* L);
int LADBusFreeInterface(lua_State* L);
int LADBusInterfaceName(lua_State* L);

enum LADBusInterfaceData
{
    LADBusMethodRef,
    LADBusGetPropertyRef,
    LADBusSetPropertyRef,
};

int LADBusCheckFields(
        lua_State*                  L,
        int                         table,
        uint                        allowNumbers,
        const char*                 valid[]);

struct ADBusMember* LADBusUnpackMember(
        lua_State*              L,
        int                     memberIndex,
        int                     memberTable,
        struct ADBusInterface*  interface,
        enum ADBusMemberType*   type);

void LADBusUnpackSignalMember(
        lua_State*          L,
        int                 memberIndex,
        int                 memberTable,
        struct ADBusMember* member);

void LADBusUnpackMethodMember(
        lua_State*          L,
        int                 memberIndex,
        int                 memberTable,
        struct ADBusMember* member,
        struct LADBusData*  data);

void LADBusUnpackPropertyMember(
        lua_State*          L,
        int                 memberIndex,
        int                 memberTable,
        struct ADBusMember* member,
        struct LADBusData*  data);

void LADBusUnpackArguments(
        lua_State*                  L,
        int                         memberIndex,
        int                         argsTable,
        struct ADBusMember*         member,
        enum ADBusArgumentDirection defaultDirection);

void LADBusUnpackAnnotations(
        lua_State*          L,
        int                 memberIndex,
        int                 annotationsTable,
        struct ADBusMember* member);

void LADBusUnpackCallback(
        lua_State*          L,
        int                 memberIndex,
        const char*         fieldName,
        int                 callback,
        int*                function);




