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

#include <adbuslua.h>
#include "internal.h"

#ifdef _WIN32
#   include <Winsock2.h>
#   include <WS2tcpip.h>
#else
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <unistd.h>
#   define closesocket(x) close(x)
#endif

#include <string.h>
#include <stdio.h>

#define HANDLE "adbus_Socket"

/* ------------------------------------------------------------------------- */

static int NewSocket(lua_State* L)
{
    adbus_Socket* s = (adbus_Socket*) lua_newuserdata(L, sizeof(adbus_Socket));
    luaL_getmetatable(L, HANDLE);
    lua_setmetatable(L, -2);
    *s = ADBUS_SOCK_INVALID;

    if (lua_isnil(L, 1)) {
        *s = adbus_sock_connect(ADBUS_DEFAULT_BUS);
    } else {
        size_t addrlen;
        const char* addr = luaL_checklstring(L, 1, &addrlen);

        if (strcmp(addr, "session") == 0) {
            *s = adbus_sock_connect(ADBUS_SESSION_BUS);
        } else if (strcmp(addr, "system") == 0) {
            *s = adbus_sock_connect(ADBUS_SYSTEM_BUS);
        } else {
            *s = adbus_sock_connect_s(addr, (int) addrlen);
        }
    }

    if (*s == ADBUS_SOCK_INVALID)
        return luaL_error(L, "Failure to connect");


    adbus_Buffer* buf = adbus_buf_new();
    if (adbus_sock_cauth(*s, buf)) {
        adbus_buf_free(buf);
        return luaL_error(L, "Failure to auth");
    }

    lua_pushlstring(L, adbus_buf_data(buf), adbus_buf_size(buf));
    adbus_buf_free(buf);

    return 2;
}

/* ------------------------------------------------------------------------- */

static int Close(lua_State* L)
{
    adbus_Socket* s = (adbus_Socket*) luaL_checkudata(L, 1, HANDLE);
    if (*s != ADBUS_SOCK_INVALID)
        closesocket(*s);
    *s = ADBUS_SOCK_INVALID;

    return 0;
}

/* ------------------------------------------------------------------------- */

static int Send(lua_State* L)
{
    adbus_Socket* s = (adbus_Socket*) luaL_checkudata(L, 1, HANDLE);
    if (*s == ADBUS_SOCK_INVALID)
        return luaL_error(L, "Socket is closed");

    size_t size;
    const char* data = luaL_checklstring(L, 2, &size);
    adbus_ssize_t sent = send(*s, data, size, 0);
    if (sent != (adbus_ssize_t) size) {
        Close(L);
        return luaL_error(L, "Send error");
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

static int Recv(lua_State* L)
{
    adbus_Socket* s = (adbus_Socket*) luaL_checkudata(L, 1, HANDLE);
    if (*s == ADBUS_SOCK_INVALID)
        return luaL_error(L, "Socket is closed");

    char buf[64 * 1024];
    int recvd = recv(*s, buf, sizeof(buf), 0);
    if (recvd < 0) {
        Close(L);
        return luaL_error(L, "Receive error");
    }
    lua_pushlstring(L, buf, recvd);
    return 1;
}

/* ------------------------------------------------------------------------- */

static const luaL_Reg reg[] = {
    { "new", &NewSocket },
    { "__gc", &Close },
    { "close", &Close },
    { "send", &Send },
    { "receive", &Recv },
    { NULL, NULL }
};

void adbusluaI_reg_socket(lua_State* L)
{
    luaL_newmetatable(L, HANDLE);
    luaL_register(L, NULL, reg);
}





