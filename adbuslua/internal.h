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

#include <adbuslua.h>

#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && defined(__ELF__)
#   define IFUNC __attribute__((visibility("hidden"))) extern
#else
#   define IFUNC extern
#endif

#define NEW(type) ((type*) calloc(1, sizeof(type)))
#define ZERO(v) memset(&v, 0, sizeof(v));
#define UNUSED(x) ((void) (x))

IFUNC int adbusluaI_check_fields_numbers(lua_State* L, int table, const char** valid);

IFUNC int adbusluaI_boolField(lua_State* L, int table, const char* field, adbus_Bool* val);
IFUNC int adbusluaI_intField(lua_State* L, int table, const char* field, int64_t* val);
IFUNC int adbusluaI_stringField(lua_State* L, int table, const char* field, const char** str, int* sz);

IFUNC void adbusluaI_reg_connection(lua_State* L);
IFUNC void adbusluaI_reg_interface(lua_State* L);
IFUNC void adbusluaI_reg_object(lua_State* L);

IFUNC adbus_Interface* adbusluaI_tointerface(lua_State* L, int index);

struct Connection
{
    adbus_Connection*       connection;
    adbus_MsgFactory*       message;
    lua_State*              L;
    int                     queue;
};

struct Interface
{
    adbus_Interface*        interface;
    adbus_Member*           member;
};

struct Object
{
    lua_State*              L;
    struct Connection*      connection;
    int                     connref;
    int                     callback;
    int                     object;
    int                     user;
    int                     queue;
    adbus_ConnMatch*        match;
    adbus_ConnReply*        reply;
    adbus_ConnBind*         bind;
};

IFUNC int adbusluaI_callback(adbus_CbData* d);
IFUNC int adbusluaI_method(adbus_CbData* d);
IFUNC int adbusluaI_getproperty(adbus_CbData* d);
IFUNC int adbusluaI_setproperty(adbus_CbData* d);


