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

#include "LuaInclude.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#   ifdef LADBUS_BUILDING
#       define LADBUS_API __declspec(dllexport)
#   else
#       define LADBUS_API __declspec(dllimport)
#   endif
#else
#   define LADBUS_API extern
#endif


#ifndef NDEBUG
#   define LOGD LADBusPrintDebug
#else
#   define LOGD if(0) {} else LADBusPrintDebug
#endif

void LADBusPrintDebug(const char* format, ...);


struct ADBusConnection;
struct LADBusConnection;
struct LADBusObject;
struct LADBusInterface;

struct LADBusConnection*    LADBusPushNewConnection(lua_State* L);
struct LADBusObject*        LADBusPushNewObject(lua_State* L);
struct LADBusInterface*     LADBusPushNewInterface(lua_State* L);

LADBUS_API void LADBusPushExistingConnection(lua_State* L, struct ADBusConnection* connection);

struct LADBusConnection*    LADBusCheckConnection(lua_State* L, int index);
struct LADBusObject*        LADBusCheckObject(lua_State* L, int index);
struct LADBusInterface*     LADBusCheckInterface(lua_State* L, int index);

LADBUS_API int luaopen_adbuslua_core(lua_State* L);


#ifdef __cplusplus
}
#endif
