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

#include <adbus.h>
#include "misc.h"

#ifdef _WIN32
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

#ifndef _WIN32
#   define closesocket(x) close(x)
#endif

/** \defgroup adbus_Socket adbus_Socket
 *  \brief Helper functions for connecting BSD sockets.
 */

/** \defgroup adbus_Misc adbus_Misc
 *  \brief Miscellaneous functions
 */

// ----------------------------------------------------------------------------

struct Fields
{
    const char* proto;
    const char* file;
    const char* abstract;
    const char* host;
    const char* port;
};

static void ParseFields(struct Fields* f, char* bstr, size_t size)
{
    char* estr = bstr + size;

    char* p = (char*) memchr(bstr, ':', estr - bstr);
    if (!p)
        return;

    f->proto = adbusI_strndup(bstr, p - bstr);
    bstr = p + 1;

    while (bstr < estr) {
        char* bkey = bstr;
        char* ekey = (char*) memchr(bstr, '=', estr - bstr);
        if (!ekey)
            return;

        bstr = ekey + 1;

        char* bval = bstr;
        char* eval = (char*) memchr(bstr, ',', estr - bstr);
        if (eval) {
            bstr = eval + 1;
            *eval = '\0';
        } else {
            bstr = estr;
            eval = estr;
        }

        if (strncmp(bkey, "file", ekey - bkey) == 0) {
            f->file = bval;
        } else if (strncmp(bkey, "abstract", ekey - bkey) == 0) {
            f->abstract = bval;
        } else if (strncmp(bkey, "host", ekey - bkey) == 0) {
            f->host = bval;
        } else if (strncmp(bkey, "port", ekey - bkey) == 0) {
            f->port = bval;
        }
    }
}

// ----------------------------------------------------------------------------

static adbus_Socket ConnectTcp(struct Fields* f)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    adbus_Socket sfd = ADBUS_SOCK_INVALID;

    /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    int s = getaddrinfo(f->host, f->port, &hints, &result);
    if (s != 0)
        return ADBUS_SOCK_INVALID;

    /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family,
                     rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == ADBUS_SOCK_INVALID)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        closesocket(sfd);
        sfd = ADBUS_SOCK_INVALID;
    }

    freeaddrinfo(result);           /* No longer needed */

    return sfd;
}

// ----------------------------------------------------------------------------

static adbus_Socket BindTcp(struct Fields* f)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    adbus_Socket sfd = ADBUS_SOCK_INVALID;

    /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;          /* Any protocol */

    int s = getaddrinfo(NULL, f->port, &hints, &result);
    if (s != 0) {
        return ADBUS_SOCK_INVALID;
    }

    /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family,
                     rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == ADBUS_SOCK_INVALID)
            continue;

        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        closesocket(sfd);
        sfd = ADBUS_SOCK_INVALID;
    }

    freeaddrinfo(result);           /* No longer needed */

    return sfd;
}

// ----------------------------------------------------------------------------

#ifndef _WIN32
#define UNIX_PATH_MAX 108
static adbus_Socket ConnectAbstract(struct Fields* f)
{
    size_t psize = min(UNIX_PATH_MAX-1, strlen(f->abstract));

    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    strncat(sa.sun_path+1, f->abstract, psize);

    adbus_Socket sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    int err = connect(sfd,
                      (const struct sockaddr*) &sa,
                      sizeof(sa.sun_family) + psize + 1);
    if (err) {
        closesocket(sfd);
        return ADBUS_SOCK_INVALID;
    }

    return sfd;
}

static adbus_Socket BindAbstract(struct Fields* f)
{
    size_t psize = min(UNIX_PATH_MAX-1, strlen(f->abstract));

    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    strncat(sa.sun_path+1, f->abstract, psize);

    adbus_Socket sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    int err = bind(sfd,
                   (const struct sockaddr*) &sa,
                   sizeof(sa.sun_family) + psize + 1);
    if (err) {
        closesocket(sfd);
        return ADBUS_SOCK_INVALID;
    }

    return sfd;
}

static adbus_Socket ConnectUnix(struct Fields* f)
{
    size_t psize = min(UNIX_PATH_MAX, strlen(f->file));

    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    strncat(sa.sun_path, f->file, psize);

    adbus_Socket sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    int err = connect(sfd,
                      (const struct sockaddr*) &sa,
                      sizeof(sa.sun_family) + psize);
    if (err) {
        closesocket(sfd);
        return ADBUS_SOCK_INVALID;
    }

    return sfd;
}

static adbus_Socket BindUnix(struct Fields* f)
{
    size_t psize = min(UNIX_PATH_MAX, strlen(f->file));

    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    strncat(sa.sun_path, f->file, psize);

    adbus_Socket sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    int err = bind(sfd,
                   (const struct sockaddr*) &sa,
                   sizeof(sa.sun_family) + psize);
    if (err) {
        closesocket(sfd);
        return ADBUS_SOCK_INVALID;
    }

    return sfd;
}

#endif

// ----------------------------------------------------------------------------

static adbus_Bool CopyEnv(const char* env, char* buf, size_t sz)
{
    const char* v = getenv(env);
    if (v == NULL)
        return 0;

    strncpy(buf, v, sz - 1);
    buf[sz - 1] = '\0';
    return 1;
}

#ifdef _WIN32
static adbus_Bool CopySharedMem(const wchar_t* name, char* buf, size_t sz)
{
    HANDLE map = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY, 0, sz - 1, name);
    if (map == INVALID_HANDLE_VALUE)
        return 0;

    if (GetLastError() != ERROR_ALREADY_EXISTS)
    {
        CloseHandle(map);
        return 0;
    }

    void* view = MapViewOfFile(map, FILE_MAP_READ, 0, 0, sz - 1);
    if (view)
        CopyMemory(buf, view, sz - 1);

    UnmapViewOfFile(view);
    CloseHandle(map);

    buf[sz - 1] = '\0';

    return 1;
}
#endif

static adbus_Bool CopyString(const char* str, char* buf, size_t sz)
{
    if (str == NULL)
        return 0;
    strncpy(buf, str, sz - 1);
    buf[sz - 1] = '\0';
    return 1;
}

static adbus_Bool EnvironmentString(adbus_Bool connect, adbus_BusType type, char* buf, size_t sz)
{
    UNUSED(connect);
    if (type == ADBUS_DEFAULT_BUS) {
        return CopyEnv("DBUS_STARTER_ADDRESS", buf, sz)
            || CopyEnv("DBUS_SESSION_BUS_ADDRESS", buf, sz)
#ifdef _WIN32
            || (connect && CopySharedMem(L"Local\\DBUS_SESSION_BUS_ADDRESS", buf, sz))
#endif
            || CopyString("autostart:", buf, sz);

    } else if (type == ADBUS_SESSION_BUS) {
        return CopyEnv("DBUS_SESSION_BUS_ADDRESS", buf, sz)
#ifdef _WIN32
            || (connect && CopySharedMem(L"Local\\DBUS_SESSION_BUS_ADDRESS", buf, sz))
#endif
            || CopyString("autostart:", buf, sz);

    } else if (type == ADBUS_SYSTEM_BUS) {
        return CopyEnv("DBUS_SYSTEM_BUS_ADDRESS", buf, sz)
#ifndef _WIN32
            || CopyString("unix:file=/var/run/dbus/system_bus_socket", buf, sz)
#endif
            || CopyString("autostart:", buf, sz);

    } else {
        return 0;
    }
}

/** Gets the address to connect to for the given bus type.
 *  \ingroup adbus_Misc
 *
 *  The address is copied into the supplied buffer. It is truncated if the
 *  buffer is too small. In general having a buffer size of 256 is sufficient
 *  for any reasonable address.
 *
 *  The lookup rules for ADBUS_DEFAULT_BUS are:
 *  -# DBUS_STARTER_ADDRESS environment variable
 *  -# DBUS_SESSION_BUS_ADDRESS environment variable
 *  -# On windows: the shared memory segment "Local\DBUS_SESSION_BUS_ADDRESS"
 *  -# The hardcoded string "autostart:"
 *
 *  For ADBUS_SESSION_BUS:
 *  -# DBUS_SESSION_BUS_ADDRESS environment variable
 *  -# On windows: the shared memory segment "Local\DBUS_SESSION_BUS_ADDRESS"
 *  -# The hardcoded string "autostart:"
 *
 *  For ADBUS_SYSTEM_BUS:
 *  -# DBUS_SYSTEM_BUS_ADDRESS environment variable
 *  -# On unix: the hardcoded string "unix:file=/var/run/dbus/system_bus_socket"
 *  -# On windows: the hardcoded string "autostart:"
 *
 *  \return non-zero on error
 */
int adbus_connect_address(adbus_BusType type, char* buf, size_t sz)
{
    if (!EnvironmentString(1, type, buf, sz))
        return -1;
    return 0;
}

/** Gets the address to bind to for the given bus type.
 *  \ingroup adbus_Misc
 *
 *  The address is copied into the supplied buffer. It is truncated if the
 *  buffer is too small. In general having a buffer size of 256 is sufficient
 *  for any reasonable address.
 *
 *  The lookup rules for ADBUS_DEFAULT_BUS are:
 *  -# DBUS_STARTER_ADDRESS environment variable
 *  -# DBUS_SESSION_BUS_ADDRESS environment variable
 *  -# The hardcoded string "autostart:"
 *
 *  For ADBUS_SESSION_BUS:
 *  -# DBUS_SESSION_BUS_ADDRESS environment variable
 *  -# The hardcoded string "autostart:"
 *
 *  For ADBUS_SYSTEM_BUS:
 *  -# DBUS_SYSTEM_BUS_ADDRESS environment variable
 *  -# On unix: the hardcoded string "unix:file=/var/run/dbus/system_bus_socket"
 *  -# On windows: the hardcoded string "autostart:"
 *
 *  \return non-zero on error
 */
int adbus_bind_address(adbus_BusType type, char* buf, size_t sz)
{
    if (!EnvironmentString(0, type, buf, sz))
        return -1;
    return 0;
}

// ----------------------------------------------------------------------------

/** Connects a BSD socket to the specified bus type.
 *  \ingroup adbus_Socket
 * 
 *  The address is looked up using adbus_connect_address().
 *  Supported addresses is given by adbus_sock_connect_s().
 *
 *  \return ADBUS_SOCK_INVALID on error
 *
 *  \sa adbus_connect_address()
 */
adbus_Socket adbus_sock_connect(adbus_BusType type)
{
    char buf[255];
    if (adbus_connect_address(type, buf, sizeof(buf)))
        return ADBUS_SOCK_INVALID;

    return adbus_sock_connect_s(buf, -1);
}

// ----------------------------------------------------------------------------

/** Binds a BSD socket to the specified bus type.
 *  \ingroup adbus_Socket
 * 
 *  The address is looked up using adbus_bind_address().
 *  Supported addresses are given by adbus_sock_bind_s().
 *
 *  \return ADBUS_SOCK_INVALID on error
 *
 *  \sa adbus_connect_address()
 */
adbus_Socket adbus_sock_bind(adbus_BusType type)
{
    char buf[255];
    if (adbus_bind_address(type, buf, sizeof(buf)))
        return ADBUS_SOCK_INVALID;

    return adbus_sock_bind_s(buf, -1);
}

// ----------------------------------------------------------------------------

/** Connects a BSD socket to the specified address.
 *  \ingroup adbus_Socket
 *
 *  Supported address types are:
 *  - TCP sockets of the form "tcp:host=<hostname>,port=<port number>"
 *  - On unix: unix sockets of the form "unix:file=<filename>"
 *  - On linux: abstract unix sockets of the form "unix:abstract=<filename>"
 *
 *  \return ADBUS_SOCK_INVALID on error
 *
 *  For example:
 *  \code
 *  adbus_Socket s = adbus_sock_connect_s("tcp:host=localhost,port=12345", -1);
 *  \endcode
 *
 *  \sa adbus_connect_address()
 */
adbus_Socket adbus_sock_connect_s(
        const char*     envstr,
        int             size)
{
    if (size < 0)
        size = strlen(envstr);

    char* str = adbusI_strndup(envstr, size);
    struct Fields f;
    memset(&f, 0, sizeof(struct Fields));

    ParseFields(&f, str, size);

    adbus_Socket sfd = ADBUS_SOCK_INVALID;

#ifdef _WIN32
    if (f.proto && strcmp(f.proto, "tcp") == 0 && f.host && f.port) {
        sfd = ConnectTcp(&f);
    }
#else
    if (f.proto && strcmp(f.proto, "tcp") == 0 && f.host && f.port) {
        sfd = ConnectTcp(&f);
    } else if (f.proto && strcmp(f.proto, "unix") == 0 && f.file) {
        sfd = ConnectUnix(&f);
    } else if (f.proto && strcmp(f.proto, "unix") == 0 && f.abstract) {
        sfd = ConnectAbstract(&f);
    }
#endif

    free(str);

    return sfd;
}


// ----------------------------------------------------------------------------

/** Binds a BSD socket to the specified address.
 *  \ingroup adbus_Socket
 *
 *  Supported address types are:
 *  - TCP sockets of the form "tcp:host=<hostname>,port=<port number>"
 *  - On unix: unix sockets of the form "unix:file=<filename>"
 *  - On linux: abstract unix sockets of the form "unix:abstract=<filename>"
 *
 *  For TCP: specifying a port of 0 will cause the underlying sockets API to
 *  choose a random available port. The chosen port can be found by using
 *  getsockname() on the returned socket.
 *
 *  \return ADBUS_SOCK_INVALID on error
 *
 *  For example:
 *  \code
 *  adbus_Socket s = adbus_sock_bind_s("tcp:host=localhost,port=12345", -1);
 *  \endcode
 *
 *  \sa adbus_connect_address()
 */
adbus_Socket adbus_sock_bind_s(
        const char*     envstr,
        int             size)
{
    if (size < 0)
        size = strlen(envstr);

    char* str = adbusI_strndup(envstr, size);
    struct Fields f;
    memset(&f, 0, sizeof(struct Fields));

    ParseFields(&f, str, size);

    adbus_Socket sfd = ADBUS_SOCK_INVALID;

#ifdef _WIN32
    if (f.proto && strcmp(f.proto, "tcp") == 0 && f.host && f.port) {
        sfd = BindTcp(&f);
    }
#else
    if (f.proto && strcmp(f.proto, "tcp") == 0 && f.host && f.port) {
        sfd = BindTcp(&f);
    } else if (f.proto && strcmp(f.proto, "unix") == 0 && f.file) {
        sfd = BindUnix(&f);
    } else if (f.proto && strcmp(f.proto, "unix") == 0 && f.abstract) {
        sfd = BindAbstract(&f);
    }
#endif

    free(str);

    return sfd;
}

// ----------------------------------------------------------------------------

static adbus_ssize_t Send(void* d, const char* b, size_t sz)
{ return send(*(adbus_Socket*) d, b, sz, 0); }

static uint8_t Rand(void* d)
{ (void) d; return (uint8_t) rand(); }

#define RECV_SIZE 1024
/** Run the client auth protocol for a blocking BSD socket.
 *  \ingroup adbus_Socket
 *
 *  This includes sending the leading null byte.
 *
 *  \return non-zero on error
 *
 *  For example:
 *  \code
 *  int main(int argc, char* argv[])
 *  {
 *      adbus_Socket s = adbus_sock_connect(ADBUS_DEFAULT_BUS);
 *      if (s == ADBUS_SOCK_INVALID || adbus_sock_cauth(s))
 *          return -1;
 *      ... use the socket
 *  }
 *  \endcode
 *
 */
int adbus_sock_cauth(adbus_Socket sock, adbus_Buffer* buffer)
{
    adbus_Auth* a = adbus_cauth_new(&Send, &Rand, &sock);
    int ret = 0;

    adbus_cauth_external(a);

    if (send(sock, "\0", 1, 0) != 1)
        return -1;

    if (adbus_cauth_start(a))
        return -1;

    while (!ret) {
        char* dest = adbus_buf_recvbuf(buffer, RECV_SIZE);
        int read = recv(sock, dest, RECV_SIZE, 0);
        if (read < 0)
            return -1;
        adbus_buf_recvd(buffer, RECV_SIZE, read);
        ret = adbus_auth_parse(a, buffer);
    }

    if (ret < 0)
        return -1;
    
    return 0;
}

