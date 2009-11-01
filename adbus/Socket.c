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
#include "Auth.h"
#include "Misc_p.h"

#ifdef WIN32
#   include <Winsock2.h>
#   include <WS2tcpip.h>
#   include <windows.h>
#else
#   include <sys/socket.h>
#   include <sys/types.h>
#   include <sys/un.h>
#   include <netdb.h>
#   include <unistd.h>
#endif

#include <string.h>

#ifndef WIN32
#   define closesocket(x) close(x)
#endif


// ----------------------------------------------------------------------------

struct Fields
{
    char* proto;
    char* file;
    char* abstract;
    char* host;
    char* port;
};

static void ParseFields(struct Fields* f, const char* bstr, size_t size)
{
    const char* estr = bstr + size;

    const char* p = (const char*) memchr(bstr, ':', estr - bstr);
    if (!p)
        return;

    f->proto = strndup_(bstr, p - bstr);
    bstr = p + 1;

    while (bstr < estr) {
        const char* bkey = bstr;
        const char* ekey = (const char*) memchr(bstr, '=', estr - bstr);
        if (!ekey)
            return;

        bstr = ekey + 1;

        const char* bval = bstr;
        const char* eval = (const char*) memchr(bstr, ',', estr - bstr);
        if (eval) {
            bstr = eval + 1;
        } else {
            bstr = estr;
            eval = estr;
        }

        char* val = strndup_(bval, eval - bval);

        if (strncmp(bkey, "file", ekey - bkey) == 0) {
            f->file = val;
        } else if (strncmp(bkey, "abstract", ekey - bkey) == 0) {
            f->abstract = val;
        } else if (strncmp(bkey, "host", ekey - bkey) == 0) {
            f->host = val;
        } else if (strncmp(bkey, "port", ekey - bkey) == 0) {
            f->port = val;
        } else {
            free(val);
        }
    }
}

// ----------------------------------------------------------------------------

static ADBusSocket_t Tcp(struct Fields* f)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    ADBusSocket_t sfd = ADBUS_INVALID_SOCKET;

    /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    int s = getaddrinfo(f->host, f->port, &hints, &result);
    if (s != 0) {
        return ADBUS_INVALID_SOCKET;
    }

    /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family,
                     rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == ADBUS_INVALID_SOCKET)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        closesocket(sfd);
        sfd = ADBUS_INVALID_SOCKET;
    }

    freeaddrinfo(result);           /* No longer needed */

    return sfd;
}

// ----------------------------------------------------------------------------

#ifndef WIN32
#define UNIX_PATH_MAX 108
static ADBusSocket_t Abstract(struct Fields* f)
{
    size_t psize = min(UNIX_PATH_MAX-1, strlen(f->abstract));

    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    strncat(sa.sun_path+1, f->abstract, psize);

    ADBusSocket_t sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    int err = connect(sfd,
                      (const struct sockaddr*) &sa,
                      sizeof(sa.sun_family) + psize + 1);
    if (err) {
        closesocket(sfd);
        return ADBUS_INVALID_SOCKET;
    }

    return sfd;
}


static ADBusSocket_t Unix(struct Fields* f)
{
    size_t psize = min(UNIX_PATH_MAX, strlen(f->file));

    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    strncat(sa.sun_path, f->file, psize);

    ADBusSocket_t sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    int err = connect(sfd,
                      (const struct sockaddr*) &sa,
                      sizeof(sa.sun_family) + psize);
    if (err) {
        closesocket(sfd);
        return ADBUS_INVALID_SOCKET;
    }

    return sfd;
}
#endif

// ----------------------------------------------------------------------------

static void Send(void* socket, const char* data, size_t size)
{
    send(*(ADBusSocket_t*) socket, data, size, 0);
}

static int Recv(void* socket, char* data, size_t size)
{
    return recv(*(ADBusSocket_t*) socket, data, size, 0);
}

static uint8_t Rand(void* socket)
{
    (void) socket;
    return (uint8_t) rand();
}

// ----------------------------------------------------------------------------

ADBusSocket_t ADBusConnectSocket(
        uint        systembus,
        const char* envstr,
        int         size)
{
    (void) systembus;
#ifndef WIN32
    if (systembus && envstr == NULL) {
        envstr = "unix:file=/var/run/dbus/system_bus_socket";
        size = -1;
    }
#endif

    if (size < 0)
        size = strlen(envstr);

    struct Fields f;
    memset(&f, 0, sizeof(struct Fields));

    ParseFields(&f, envstr, size);

    ADBusSocket_t sfd = ADBUS_INVALID_SOCKET;

    uint cookie = 0;

#ifdef WIN32
   if (strcmp(f.proto, "tcp") == 0 && f.host && f.port) {
        sfd = Tcp(&f);
    }
#else
    if (strcmp(f.proto, "tcp") == 0 && f.host && f.port) {
        sfd = Tcp(&f);
        cookie = 1;
    } else if (strcmp(f.proto, "unix") == 0 && f.file) {
        sfd = Unix(&f);
    } else if (strcmp(f.proto, "unix") == 0 && f.abstract) {
        sfd = Abstract(&f);
    }
#endif

    int aerr = 0;
    if (sfd != ADBUS_INVALID_SOCKET) {
        if (cookie) {
            aerr = ADBusAuthDBusCookieSha1(&Send, &Recv, &Rand, (void*) &sfd);
        } else {
            aerr = ADBusAuthExternal(&Send, &Recv, (void*) &sfd);
        }
    }

    if (aerr) {
        closesocket(sfd);
        sfd = ADBUS_INVALID_SOCKET;
    }


    free(f.abstract);
    free(f.file);
    free(f.host);
    free(f.port);
    free(f.proto);

    return sfd;
}




