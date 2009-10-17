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

#pragma once

#include "LuaInclude.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#   ifdef ADBUSLUA_LIBRARY
#       define LADBUS_API __declspec(dllexport)
#   else
#       define LADBUS_API __declspec(dllimport)
#   endif
#else
#   define LADBUS_API extern
#endif

#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && \
    defined(__ELF__)

#   define LADBUSI_FUNC	__attribute__((visibility("hidden"))) extern
#   define LADBUSI_DATA	LADBUS_IFUNC

#else
#   define LADBUSI_FUNC	extern
#   define LADBUSI_DATA	extern
#endif

#ifndef LOGD
  #ifndef NDEBUG
  #   define LOGD LADBusPrintDebug
  #else
  #   define LOGD if(0) {} else LADBusPrintDebug
  #endif
#endif

LADBUSI_FUNC void LADBusPrintDebug(const char* format, ...);

LADBUSI_FUNC int LADBusCheckFields(
        lua_State*         L,
        int                table,
        const char*        valid[]);

LADBUSI_FUNC int LADBusCheckFieldsAllowNumbers(
        lua_State*         L,
        int                table,
        const char*        valid[]);

struct ADBusConnection;
struct LADBusConnection;
struct ADBusInterface;
struct LADBusSocket;
struct ADBusInterface;

LADBUSI_FUNC struct LADBusConnection* LADBusPushNewConnection(lua_State* L);
LADBUSI_FUNC struct LADBusSocket*     LADBusPushNewSocket(lua_State* L);
LADBUSI_FUNC void LADBusPushNewInterface(lua_State* L, struct ADBusInterface* interface);

LADBUS_API void LADBusPushExistingConnection(lua_State* L, struct ADBusConnection* connection);

LADBUSI_FUNC struct LADBusConnection*    LADBusCheckConnection(lua_State* L, int index);
LADBUSI_FUNC struct ADBusInterface*      LADBusCheckInterface(lua_State* L, int index);
LADBUSI_FUNC struct LADBusSocket*        LADBusCheckSocket(lua_State* L, int index);

// Objects are simply referenced by paths in lua
LADBUSI_FUNC struct ADBusObject* LADBusCheckObject(
        lua_State*               L,
        struct LADBusConnection* connection,
        int                      index);

LADBUS_API int luaopen_adbuslua_core(lua_State* L);


#ifdef __cplusplus
}
#endif
