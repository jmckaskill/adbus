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

#include <adbus/adbuslua.h>


#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && defined(__ELF__)
#   define IFUNC __attribute__((visibility("hidden"))) extern
#else
#   define IFUNC extern
#endif

IFUNC int adbusluaI_check_fields(lua_State* L, int table, const char** valid);
IFUNC int adbusluaI_check_fields_numbers(lua_State* L, int table, const char** valid);

IFUNC int adbusluaI_getoption(lua_State* L, int index, const char** types, const char* error);
IFUNC adbus_Bool adbusluaI_getboolean(lua_State* L, int index, const char* error);
IFUNC lua_Number adbusluaI_getnumber(lua_State* L, int index, const char* error);
IFUNC const char* adbusluaI_getstring(lua_State* L, int index, size_t* sz, const char* error);

IFUNC void adbusluaI_reg_connection(lua_State* L);
IFUNC void adbusluaI_reg_interface(lua_State* L);
IFUNC void adbusluaI_reg_object(lua_State* L);
IFUNC void adbusluaI_reg_socket(lua_State* L);

typedef struct adbuslua_Data
{
    adbus_User          h;
    lua_State*          L;
    int                 callback;
    int                 argument;
    int                 connection;
    int                 interface;
    char*               signature;
    uint                debug;
} adbuslua_Data;

IFUNC adbuslua_Data* adbusluaI_newdata(lua_State* L);
IFUNC void adbusluaI_push(lua_State* L, int ref);
IFUNC int  adbusluaI_ref(lua_State* L, int index);


