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

#ifndef _WIN32
#   define _POSIX_SOURCE
#endif

#include "internal.h"

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
#   define closesocket(x) close(x)
#endif

/** \defgroup adbus_Socket adbus_Socket
 *  \brief Helper functions for connecting BSD sockets.
 */

/** \defgroup adbus_Misc adbus_Misc
 *  \brief Miscellaneous functions
 */

/* ------------------------------------------------------------------------- */

struct Fields
{
    const char* proto;
    const char* path;
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

    f->proto = bstr;
    *p = '\0';
    bstr = p + 1;

    while (bstr < estr) {
        char *bkey, *ekey, *bval, *eval;

        bkey = bstr;
        ekey = (char*) memchr(bstr, '=', estr - bstr);
        if (!ekey)
            return;

        bstr = ekey + 1;

        bval = bstr;
        eval = (char*) memchr(bstr, ',', estr - bstr);
        if (eval) {
            bstr = eval + 1;
            *eval = '\0';
        } else {
            bstr = estr;
            eval = estr;
        }

        if (strncmp(bkey, "path", ekey - bkey) == 0) {
            f->path = bval;
        } else if (strncmp(bkey, "abstract", ekey - bkey) == 0) {
            f->abstract = bval;
        } else if (strncmp(bkey, "host", ekey - bkey) == 0) {
            f->host = bval;
        } else if (strncmp(bkey, "port", ekey - bkey) == 0) {
            f->port = bval;
        }
    }
}

/* ------------------------------------------------------------------------- */

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

    if (getaddrinfo(f->host, f->port, &hints, &result) != 0)
        return ADBUS_SOCK_INVALID;

    /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family,
#ifdef SOCK_CLOEXEC
                     rp->ai_socktype | SOCK_CLOEXEC,
#else
                     rp->ai_socktype,
#endif
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

/* ------------------------------------------------------------------------- */

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

    if (getaddrinfo(NULL, f->port, &hints, &result) != 0) {
        return ADBUS_SOCK_INVALID;
    }

    /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family,
#ifdef SOCK_CLOEXEC
                     rp->ai_socktype | SOCK_CLOEXEC,
#else
                     rp->ai_socktype,
#endif
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

/* ------------------------------------------------------------------------- */

#ifndef _WIN32
#define UNIX_PATH_MAX 108
static adbus_Socket ConnectAbstract(struct Fields* f)
{
    struct sockaddr_un sa;
    adbus_Socket sfd;
    int err;

    size_t psize = min(UNIX_PATH_MAX-1, strlen(f->abstract));

    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    strncat(sa.sun_path+1, f->abstract, psize);

    sfd = socket(
            AF_UNIX,
#ifdef SOCK_CLOEXEC
            SOCK_STREAM | SOCK_CLOEXEC,
#else
            SOCK_STREAM,
#endif
            0);

    err = connect(sfd,
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
    struct sockaddr_un sa;
    adbus_Socket sfd;
    int err;

    size_t psize = min(UNIX_PATH_MAX-1, strlen(f->abstract));

    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    strncat(sa.sun_path+1, f->abstract, psize);

    sfd = socket(
            AF_UNIX,
#ifdef SOCK_CLOEXEC
            SOCK_STREAM | SOCK_CLOEXEC,
#else
            SOCK_STREAM,
#endif
            0);

    err = bind(sfd,
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
    struct sockaddr_un sa;
    adbus_Socket sfd;
    int err;

    size_t psize = min(UNIX_PATH_MAX, strlen(f->path));

    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    strncat(sa.sun_path, f->path, psize);

    sfd = socket(
            AF_UNIX,
#ifdef SOCK_CLOEXEC
            SOCK_STREAM | SOCK_CLOEXEC,
#else
            SOCK_STREAM,
#endif
            0);

    err = connect(sfd,
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
    struct sockaddr_un sa;
    adbus_Socket sfd;
    int err;

    size_t psize = min(UNIX_PATH_MAX, strlen(f->path));

    memset(&sa, 0, sizeof(struct sockaddr_un));
    sa.sun_family = AF_UNIX;
    strncat(sa.sun_path, f->path, psize);

    sfd = socket(
            AF_UNIX,
#ifdef SOCK_CLOEXEC
            SOCK_STREAM | SOCK_CLOEXEC,
#else       
            SOCK_STREAM,
#endif
            0);

    err = bind(sfd,
               (const struct sockaddr*) &sa,
               sizeof(sa.sun_family) + psize);
    if (err) {
        closesocket(sfd);
        return ADBUS_SOCK_INVALID;
    }

    return sfd;
}

#endif

/* ------------------------------------------------------------------------- */

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
    void* view;
    HANDLE map = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY, 0, sz - 1, name);
    if (map == INVALID_HANDLE_VALUE)
        return 0;

    if (GetLastError() != ERROR_ALREADY_EXISTS)
    {
        CloseHandle(map);
        return 0;
    }

    view = MapViewOfFile(map, FILE_MAP_READ, 0, 0, sz - 1);
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
        if (CopyEnv("DBUS_STARTER_ADDRESS", buf, sz))
            return 1;

        if (CopyEnv("DBUS_SESSION_BUS_ADDRESS", buf, sz))
            return 1;

#ifdef _WIN32
        if (connect && CopySharedMem(L"Local\\DBUS_SESSION_BUS_ADDRESS", buf, sz))
            return 1;
#endif

        if (CopyString("autostart:", buf, sz))
            return 1;

    } else if (type == ADBUS_SESSION_BUS) {
        if (CopyEnv("DBUS_SESSION_BUS_ADDRESS", buf, sz))
            return 1;

#ifdef _WIN32
        if (connect && CopySharedMem(L"Local\\DBUS_SESSION_BUS_ADDRESS", buf, sz))
            return 1;
#endif

        if (CopyString("autostart:", buf, sz))
            return 1;

    } else if (type == ADBUS_SYSTEM_BUS) {
        if (CopyEnv("DBUS_SYSTEM_BUS_ADDRESS", buf, sz))
            return 1;

#ifndef _WIN32
        if (CopyString("unix:path=/var/run/dbus/system_bus_socket", buf, sz))
            return 1;
#endif

        if (CopyString("autostart:", buf, sz))
            return 1;

    }
    return 0;
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
 *  -# On unix: the hardcoded string "unix:path=/var/run/dbus/system_bus_socket"
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
 *  -# On unix: the hardcoded string "unix:path=/var/run/dbus/system_bus_socket"
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

/* ------------------------------------------------------------------------- */

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

/* ------------------------------------------------------------------------- */

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

/* ------------------------------------------------------------------------- */

/** Connects a BSD socket to the specified address.
 *  \ingroup adbus_Socket
 *
 *  Supported address types are:
 *  - TCP sockets of the form "tcp:host=<hostname>,port=<port number>"
 *  - On unix: unix sockets of the form "unix:path=<filename>"
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
    char* str;
    struct Fields f;
    adbus_Socket sfd = ADBUS_SOCK_INVALID;

#ifdef _WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata))
        return ADBUS_SOCK_INVALID;
#endif

    ZERO(f);

    if (size < 0)
        size = strlen(envstr);
    str = adbusI_strndup(envstr, size);

    ParseFields(&f, str, size);


    if (f.proto && strcmp(f.proto, "tcp") == 0 && f.host && f.port) {
        sfd = ConnectTcp(&f);

#ifndef _WIN32
    } else if (f.proto && strcmp(f.proto, "unix") == 0 && f.path) {
        sfd = ConnectUnix(&f);
    } else if (f.proto && strcmp(f.proto, "unix") == 0 && f.abstract) {
        sfd = ConnectAbstract(&f);
#endif

    }

    free(str);

    return sfd;
}


/* ------------------------------------------------------------------------- */

/** Binds a BSD socket to the specified address.
 *  \ingroup adbus_Socket
 *
 *  Supported address types are:
 *  - TCP sockets of the form "tcp:host=<hostname>,port=<port number>"
 *  - On unix: unix sockets of the form "unix:path=<filename>"
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
    char* str;
    struct Fields f;
    adbus_Socket sfd = ADBUS_SOCK_INVALID;

#ifdef _WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata))
        return ADBUS_SOCK_INVALID;
#endif

    ZERO(f);

    if (size < 0)
        size = strlen(envstr);
    str = adbusI_strndup(envstr, size);

    ParseFields(&f, str, size);

    if (f.proto && strcmp(f.proto, "tcp") == 0 && f.host && f.port) {
        sfd = BindTcp(&f);

#ifndef _WIN32
    } else if (f.proto && strcmp(f.proto, "unix") == 0 && f.path) {
        sfd = BindUnix(&f);
    } else if (f.proto && strcmp(f.proto, "unix") == 0 && f.abstract) {
        sfd = BindAbstract(&f);
#endif

    }

    free(str);

    return sfd;
}

/* ------------------------------------------------------------------------- */

struct SocketData
{
    adbus_Socket sock;
    adbus_Connection* connection;
    adbus_Bool connected;
    adbus_Buffer* txbuf;
};

static void FlushTxBuffer(struct SocketData* s)
{
    size_t sz = adbus_buf_size(s->txbuf);
    if (sz > 0) {
        int sent = send(s->sock, adbus_buf_data(s->txbuf), sz, 0); 
        if (sent > 0) {
            adbus_buf_remove(s->txbuf, 0, sent);
        }
    }
}

static int SendMsg(void* d, adbus_Message* m)
{ 
    struct SocketData* s = (struct SocketData*) d;
    adbus_buf_append(s->txbuf, m->data, m->size);
    if (adbus_buf_size(s->txbuf) > 16 * 1024) {
        FlushTxBuffer(s);
    }
    return (int) m->size;
}

static int Send(void* d, const char* buf, size_t sz)
{ 
    struct SocketData* s = (struct SocketData*) d;
    return (int) send(s->sock, buf, sz, 0); 
}

static int Recv(void* d, char* buf, size_t sz)
{
    struct SocketData* s = (struct SocketData*) d;
    int recvd = (int) recv(s->sock, buf, sz, 0); 
    return (recvd == 0) ? -1 : recvd;
}

static uint8_t Rand(void* d)
{ (void) d; return (uint8_t) rand(); }

static void Close(void* d)
{
    struct SocketData* s = (struct SocketData*) d;
    FlushTxBuffer(s);
#ifdef _WIN32
    closesocket(s->sock); 
#else
    close(s->sock);
#endif
    adbus_buf_free(s->txbuf);
    free(s);
}

static int Block(void* d, adbus_BlockType type, void** block, int timeoutms)
{
    struct SocketData* s = (struct SocketData*) d;
    /* This block function doesn't support timeouts yet */
    (void) timeoutms;
    assert(timeoutms == -1);

    if (type == ADBUS_BLOCK) {
        *block = NULL;
        while (!*block) {
            FlushTxBuffer(s);
            if (adbus_conn_parsecb(s->connection)) {
                return -1;
            }
        }

    } else if (type == ADBUS_UNBLOCK) {
        /* Just set it to something */
        *block = s;

    } else if (type == ADBUS_WAIT_FOR_CONNECTED) {
        *block = NULL;
        while (!*block && !s->connected) {
            FlushTxBuffer(s);
            if (adbus_conn_parsecb(s->connection)) {
                return -1;
            }
        }
    }

    return 0;
}

static void Connected(void* d)
{ 
    struct SocketData* s = (struct SocketData*) d;
    s->connected = 1;
}

adbus_Connection* adbus_sock_busconnect_s(
        const char*     envstr,
        int             size,
        adbus_Socket*   sockret)
{
    struct SocketData* d = NEW(struct SocketData);
    adbus_ConnectionCallbacks cbs;
    adbus_Auth* auth = NULL;
    adbus_Bool authenticated = 0;
    int recvd = 0, used = 0;
    char buf[256];
    void* handle = NULL;

    adbusI_initlog();

    d->txbuf = adbus_buf_new();
    d->sock = adbus_sock_connect_s(envstr, size);
    if (d->sock == ADBUS_SOCK_INVALID)
        goto err;

    auth = adbus_cauth_new(&Send, &Rand, d);
    adbus_cauth_external(auth);

    ZERO(cbs);
    cbs.send_message    = &SendMsg;
    cbs.recv_data       = &Recv;
    cbs.release         = &Close;
    cbs.block           = &Block;

    d->connection = adbus_conn_new(&cbs, d);

    if (send(d->sock, "\0", 1, 0) != 1)
        goto err;

    if (adbus_cauth_start(auth))
        goto err;

    while (!authenticated) {
        recvd = recv(d->sock, buf, 256, 0);
        if (recvd < 0)
            goto err;

        used = adbus_auth_parse(auth, buf, recvd, &authenticated);
        if (used < 0)
            goto err;

    }

    adbus_conn_connect(d->connection, &Connected, d);

    if (adbus_conn_parse(d->connection, buf + used, recvd - used))
        goto err;

    if (sockret)
        *sockret = d->sock;

    if (adbus_conn_block(d->connection, ADBUS_WAIT_FOR_CONNECTED, &handle, -1))
        goto err;

    adbus_auth_free(auth);
    return d->connection;

err:
    adbus_auth_free(auth);
    if (d->connection) {
        adbus_conn_free(d->connection);
    } else {
        Close(d);
    }
    return NULL;
}

/* ------------------------------------------------------------------------- */

adbus_Connection* adbus_sock_busconnect(
        adbus_BusType   type,
        adbus_Socket*   psock)
{
    char buf[255];
    if (adbus_connect_address(type, buf, sizeof(buf)))
        return NULL;

    return adbus_sock_busconnect_s(buf, -1, psock);
}

