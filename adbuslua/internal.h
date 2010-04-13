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

#ifdef _MSC_VER
#   define _CRT_SECURE_NO_DEPRECATE
#endif

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
};

struct Interface
{
    adbus_Interface*        interface;
    adbus_Member*           member;
};

/* The object structure holds the data for any connection services.
 *
 * Since the connection thread may be on a different thread than the lua vm,
 * the following procedure is followed to make sure everything works
 * correctly (without locks):
 *
 * 1. Create on local thread.
 *
 * 2. Send message to connection thread to add the service.
 *      - This sets the service field on the connection thread.
 *      - The release callbacks unsets the service pointer on the connection
 *      thread (see ReleaseObject). This means we don't use a release proxy.
 *      - Strings and interfaces in the reply, match and bind registration are
 *      duped on our heap, so that #3 can happen before #2 reaches the
 *      connection thread.
 *
 * 3. Lua GCs the object calling CloseObject
 *      - Frees lua refs and unsets L so that any callbacks called before #5
 *      dont call into lua (the lua vm may dissapear immediately after this).
 *
 * 4. Send message to connection thread to remove the service.
 *      - If the service is still registered the service pointer will still be
 *      set.
 *
 * 5. Send message back to free the object.
 *
 * The ping back on destruction is necessary to make sure we can handle
 * callbacks sent from the connection thread between #3 and #4.
 *
 * Since the object needs to be deleted in #5 well after the lua vm may itself
 * shut down in #3, the object is allocated on our heap with a pointer as the
 * lua user data.
 *
 * If the lua vm and connection are on the same thread, then #1 and #2; and
 * #3, #4, and #5 happen synchronously. String and interfaces are not duped
 * for #2.
 */
struct Object
{
    /* Local thread data */
    lua_State*              L;
    adbus_Connection*       c;
    int                     connref;
    int                     callback;
    int                     object;
    int                     user;

    adbus_ProxyCallback     proxy;
    void*                   puser;

    /* Connection thread data */
    adbus_ConnMatch*        match;
    adbus_ConnReply*        reply;
    adbus_ConnBind*         bind;

    union {
        adbus_Reply         reply;
        adbus_Match         match;
        adbus_Bind          bind;
    } u;
};

IFUNC int adbusluaI_callback(adbus_CbData* d);
IFUNC int adbusluaI_method(adbus_CbData* d);
IFUNC int adbusluaI_getproperty(adbus_CbData* d);
IFUNC int adbusluaI_setproperty(adbus_CbData* d);

IFUNC char* adbusluaI_strndup(const char* str, size_t len);


