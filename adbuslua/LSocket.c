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


#include "LSocket.h"
#include "LADBus.h"

#include "adbus/Socket.h"

#ifdef WIN32
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

#ifdef WIN32
#   pragma warning(disable:4267) // conversion from size_t to int
#endif


// ----------------------------------------------------------------------------

int LADBusNewSocket(lua_State* L)
{
    size_t addrlen;
    const char* addr = luaL_checklstring(L, 1, &addrlen);
    uint systembus = lua_type(L, 2) == LUA_TBOOLEAN && lua_toboolean(L, 2);

    ADBusSocket_t s = ADBusConnectSocket(systembus, addr, addrlen);

    if (s == ADBUS_INVALID_SOCKET)
        return luaL_error(L, "Failure to connect");

    struct LADBusSocket* lsocket = LADBusPushNewSocket(L);
    lsocket->socket = s;

    return 1;
}

// ----------------------------------------------------------------------------

int LADBusCloseSocket(lua_State* L)
{
    struct LADBusSocket* s = LADBusCheckSocket(L, 1);
    if (s->socket != ADBUS_INVALID_SOCKET)
        closesocket(s->socket);
    s->socket = ADBUS_INVALID_SOCKET;

    return 0;
}

// ----------------------------------------------------------------------------

int LADBusSocketSend(lua_State* L)
{
    size_t size;

    struct LADBusSocket* s = LADBusCheckSocket(L, 1);
    const char* data = luaL_checklstring(L, 2, &size);

    if (s->socket != ADBUS_INVALID_SOCKET) {
        send(s->socket, data, size, 0);
    }

    return 0;
}

// ----------------------------------------------------------------------------

int LADBusSocketRecv(lua_State* L)
{
    struct LADBusSocket* s = LADBusCheckSocket(L, 1);

    if (s->socket != ADBUS_INVALID_SOCKET) {
        char buf[64 * 1024];
        int recvd = recv(s->socket, buf, sizeof(buf), 0);
        lua_pushlstring(L, buf, recvd);
        return 1;
    }

    return 0;
}








