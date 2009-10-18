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


#include "Socket.h"
#include "ADBusLua.h"

#include "adbus/Auth.h"

#ifdef WIN32
#   include <Winsock2.h>
#   include <WS2tcpip.h>
#else
#   include <sys/un.h>
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <unistd.h>
#   include <netdb.h>
#   include <errno.h>
#   define closesocket(x) close(x)
#endif

#include <time.h>
#include <string.h>
#include <stdio.h>

// ----------------------------------------------------------------------------

static socket_t TcpConnect(lua_State* L, const char* address, const char* service)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    socket_t sfd = INVALID_SOCKET;

    /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    int s = getaddrinfo(address, service, &hints, &result);
    if (s != 0) {
        luaL_error(L, "getaddrinfo error %d", s);
    }

    /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == INVALID_SOCKET)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        closesocket(sfd);
    }

    if (rp == NULL) {               /* No address succeeded */
        luaL_error(L, "Could not connect");
    }

    freeaddrinfo(result);           /* No longer needed */

    return sfd;
}

// ----------------------------------------------------------------------------

#ifndef WIN32
#define UNIX_PATH_MAX 108
static socket_t UnixConnect(lua_State* L, const char* path, uint abstract)
{
    socket_t sfd;
    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    if (abstract) {
        strncat(sa.sun_path+1, path, UNIX_PATH_MAX-2);
    } else {
        strncat(sa.sun_path, path, UNIX_PATH_MAX-1);
    }
    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == INVALID_SOCKET) {
        luaL_error(L, "socket error '%s'", gai_strerror(errno));
    }

    int s = connect(sfd,
                    (const struct sockaddr*) &sa,
                    sizeof(sa.sun_family) + strlen(path) + 1);
    if (s != 0) {
        closesocket(sfd);
        luaL_error(L, "connect error '%s'", gai_strerror(s));
    }

    return sfd;
}
#endif

// ----------------------------------------------------------------------------

static void Send(void* socket, const char* data, size_t len)
{
    int r = send(*(socket_t*) socket, data, len, 0);
    if (r < 0)
        fprintf(stderr, "Send error %s\n", strerror(errno));
}

static int Recv(void* socket, char* buf, size_t len)
{
    int r = recv(*(socket_t*) socket, buf, len, 0);
    if (r < 0)
        fprintf(stderr, "Recv error %s\n", strerror(errno));
    return r;
}

static uint8_t Rand(void* socket)
{
    (void) socket;
    return (uint8_t) rand();
}

// ----------------------------------------------------------------------------

int LADBusNewTcpSocket(lua_State* L)
{
    const char* address = luaL_checkstring(L, 1);
    const char* service = luaL_checkstring(L, 2);

    socket_t s = TcpConnect(L, address, service);

    srand((unsigned int) time(NULL));

    ADBusAuthDBusCookieSha1(&Send, &Recv, &Rand, (void*) &s);

    struct LADBusSocket* lsocket = LADBusPushNewSocket(L);
    lsocket->socket = s;

    return 1;
}

// ----------------------------------------------------------------------------

#ifndef WIN32

int LADBusNewUnixSocket(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);
    uint abstract = lua_isboolean(L, 2) && lua_toboolean(L, 2);

    socket_t s = UnixConnect(L, path, abstract);

    ADBusAuthExternal(&Send, &Recv, (void*) &s);

    struct LADBusSocket* lsocket = LADBusPushNewSocket(L);
    lsocket->socket = s;

    return 1;
}

#endif

// ----------------------------------------------------------------------------

int LADBusCloseSocket(lua_State* L)
{
    struct LADBusSocket* s = LADBusCheckSocket(L, 1);
    if (s->socket != INVALID_SOCKET)
        closesocket(s->socket);
    s->socket = INVALID_SOCKET;

    return 0;
}

// ----------------------------------------------------------------------------

int LADBusSocketSend(lua_State* L)
{
    size_t size;

    struct LADBusSocket* s = LADBusCheckSocket(L, 1);
    const char* data = luaL_checklstring(L, 2, &size);

    if (s->socket != INVALID_SOCKET) {
        send(s->socket, data, size, 0);
    }

    return 0;
}

// ----------------------------------------------------------------------------

int LADBusSocketRecv(lua_State* L)
{
    struct LADBusSocket* s = LADBusCheckSocket(L, 1);

    if (s->socket != INVALID_SOCKET) {
        char buf[4096];
        int recvd = Recv((void*) &s->socket, buf, 4096);
        lua_pushlstring(L, buf, recvd);
        return 1;
    }

    return 0;
}








