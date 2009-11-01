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

#ifndef ADBUSLUA_H
#define ADBUSLUA_H

#include <adbus/adbus.h>

#ifdef __cplusplus
    extern "C" {
#endif

#include "lua.h"
#include "lauxlib.h"

#ifdef WIN32
#   ifdef ADBUSLUA_LIBRARY
#       define ADBUSLUA_API __declspec(dllexport)
#   else
#       define ADBUSLUA_API __declspec(dllimport)
#   endif
#else
#   define ADBUSLUA_API extern
#endif

ADBUSLUA_API int adbuslua_push_connection(lua_State* L, adbus_Connection* c);
ADBUSLUA_API int adbuslua_push_message(lua_State* L, adbus_Message* msg, adbus_Iterator* iter);
ADBUSLUA_API int adbuslua_push_argument(lua_State* L, adbus_Iterator* iter);

ADBUSLUA_API adbus_Connection* adbuslua_check_connection(lua_State* L, int index);
ADBUSLUA_API adbus_Interface*  adbuslua_check_interface(lua_State* L, int index);

ADBUSLUA_API int adbuslua_check_match(
        lua_State*          L,
        int                 index,
        adbus_Match*        match);
ADBUSLUA_API int adbuslua_check_message(
        lua_State*          L,
        int                 index,
        adbus_Message*      msg);
ADBUSLUA_API int adbuslua_check_argument(
        lua_State*          L,
        int                 index,
        const char*         sig,
        int                 size,
        adbus_Buffer*       buf);


ADBUSLUA_API int luaopen_adbuslua(lua_State* L);

#ifdef __cplusplus
    }
#endif

#endif /* ADBUSLUA_H */


