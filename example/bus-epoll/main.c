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

#define _GNU_SOURCE

#include "dmem/list.h"
#include <adbus/adbus.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

struct Remote;
DLIST_INIT(Remote, struct Remote);

struct Server
{
    int             efd;
    int             fd;
    adbus_Server*   bus;
    d_List(Remote)  remotes;
    d_List(Remote)  disconnect;
};

struct Remote
{
    d_List(Remote)  hl;
    adbus_Bool      disconnected;
    adbus_Bool      sendblocked;
    int             fd;
    adbus_Auth*     auth;
    adbus_Remote*   remote;
    adbus_Buffer*  rx;
    adbus_Buffer*  tx;
};

void ServerRecv(struct Server* s)
{
    // Accept connections until it starts to fail (with EWOULDBLOCK)
    while (1) {
        int fd = accept4(s->fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0)
            return;

        struct Remote* r = calloc(1, sizeof(struct Remote));
        r->fd = fd;
        r->rx = adbus_buf_new();
        r->tx = adbus_buf_new();
        dl_insert_after(Remote, &s->remotes, r, &r->hl);

        struct epoll_event reg = {0};
        reg.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLRDHUP;
        reg.data.ptr = r;
        epoll_ctl(s->efd, EPOLL_CTL_ADD, r->fd, &reg);
    }
}

int Disconnect(struct Server* s, struct Remote* r)
{
    assert(!r->disconnected);
    r->disconnected = 1;
    dl_remove(Remote, r, &r->hl);
    dl_insert_after(Remote, &s->disconnect, r, &r->hl);
    return 0;
}

static adbus_ssize_t Send(void* d, const char* b, size_t sz)
{ return send(((struct Remote*) d)->fd, b, sz, 0); }

static adbus_ssize_t SendMsg(void* d, adbus_Message* m)
{
    struct Remote* r = (struct Remote*) d;
    adbus_buf_append(r->tx, m->data, m->size);
    return m->size;
}

static uint8_t Rand(void* d)
{ (void) d; return (uint8_t) rand(); }

#define RECV_SIZE 64 * 1024
void RemoteRecv(struct Server* s, struct Remote* r)
{
    if (r->disconnected)
        return;

    adbus_Buffer* b = r->rx;
    adbus_ssize_t recvd;
    do {
        char* dest = adbus_buf_recvbuf(b, RECV_SIZE);
        recvd = recv(r->fd, dest, RECV_SIZE, 0);
        adbus_buf_recvd(b, RECV_SIZE, recvd);
    } while (recvd == RECV_SIZE);

    if (recvd < 0 && recvd != EAGAIN) {
        Disconnect(s, r);
        return;
    }

    while (adbus_buf_size(b) > 0) {
        if (r->remote) {
            if (adbus_remote_parse(r->remote, b)) {
                Disconnect(s, r);
                return;
            }
            break;
        } else if (r->auth) {
            int ret = adbus_auth_parse(r->auth, b);
            if (ret < 0) {
                Disconnect(s, r);
                return;
            } else if (ret > 0) {
                adbus_auth_free(r->auth);
                r->auth = NULL;
                r->remote = adbus_serv_connect(s->bus, &SendMsg, r);
            } else {
                break;
            }
        } else {
            char* d = adbus_buf_data(b);
            if (*d != '\0') {
                Disconnect(s, r);
                return;
            }
            adbus_buf_remove(b, 0, 1);
            r->auth = adbus_sauth_new(&Send, &Rand, r);
            adbus_sauth_external(r->auth, NULL);
        }
    }
}

void RemoteSend(struct Server* s, struct Remote* r)
{
    const char* data = adbus_buf_data(r->tx);
    size_t size = adbus_buf_size(r->tx);
    if (!r->disconnected && r->remote && size > 0) {
        adbus_ssize_t sent = send(r->fd, data, size, 0);
        if (sent < 0) {
            if (errno == EAGAIN) {
                sent = 0;
            } else {
                Disconnect(s, r);
                return;
            }
        }

        adbus_buf_remove(r->tx, 0, sent);
    }
}

void DoDisconnect(struct Server* s)
{
    struct Remote* r;
    DL_FOREACH(Remote, r, &s->disconnect, hl) {
        epoll_ctl(s->efd, EPOLL_CTL_DEL, r->fd, NULL);
        close(r->fd);
        adbus_auth_free(r->auth);
        adbus_remote_disconnect(r->remote);
        adbus_buf_free(r->tx);
        adbus_buf_free(r->rx);
        free(r);
    }
    dl_clear(Remote, &s->disconnect);
}

static void error()
{
    fprintf(stderr, "%s\n", strerror(errno));
    abort();
}

#ifndef _WIN32
#   define closesocket(fd) close(fd)
#endif

#ifndef min
#   define min(x,y) ((y) < (x) ? (y) : (x))
#endif
#define UNIX_PATH_MAX 108
static adbus_Socket Abstract(const char* path)
{
    size_t psize = min(UNIX_PATH_MAX-1, strlen(path));
    struct sockaddr_un sa = {0};
    sa.sun_family = AF_UNIX;
    strncat(sa.sun_path+1, path, psize);

    adbus_Socket sfd = socket(
            AF_UNIX,
            SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK,
            0);

    if (sfd != ADBUS_SOCK_INVALID) {
        int err = bind(
                sfd,
                (const struct sockaddr*) &sa,
                sizeof(sa.sun_family) + psize);

        if (err) {
            closesocket(sfd);
            return ADBUS_SOCK_INVALID;
        }
    }

    return sfd;
}

static adbus_Socket Tcp(const char* port)
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

    int s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0) {
        return ADBUS_SOCK_INVALID;
    }

    /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully bind(2).
     If socket(2) (or bind(2)) fails, we (close the socket
     and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family,
                     rp->ai_socktype | SOCK_CLOEXEC | SOCK_NONBLOCK,
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

#define EVENT_NUM 4096
int main()
{
    struct Server server = {0};
    server.bus = adbus_serv_new();
    //server.fd = Abstract("/tmp/dbus-socket");
    server.fd = Tcp("12345");
    server.efd = epoll_create1(EPOLL_CLOEXEC);
    if (server.fd == ADBUS_SOCK_INVALID || server.efd == ADBUS_SOCK_INVALID)
        error();

    struct epoll_event serv_event = {0};
    serv_event.events = EPOLLIN | EPOLLET;
    serv_event.data.ptr = &server;
    if (epoll_ctl(server.efd, EPOLL_CTL_ADD, server.fd, &serv_event))
        error();

    if (listen(server.fd, SOMAXCONN))
        error();

    struct epoll_event events[EVENT_NUM];
    while (1) {
        int ready = epoll_wait(server.efd, events, EVENT_NUM, -1);
        if (ready < 0)
            continue;

        for (int i = 0; i < ready; i++) {
            struct epoll_event* e = &events[i];
            if (e->data.ptr == &server) {
                if (e->events & EPOLLIN) {
                    ServerRecv(&server);
                }
            } else {
                struct Remote* r = (struct Remote*) e->data.ptr;
                if (e->events & EPOLLERR) {
                    Disconnect(&server, r);
                    continue;
                }
                if (e->events & EPOLLIN) {
                    RemoteRecv(&server, r);
                }
                if ((e->events & EPOLLHUP) || (e->events & EPOLLRDHUP)) {
                    Disconnect(&server, r);
                    continue;
                }
                if (e->events & EPOLLOUT) {
                    RemoteSend(&server, r);
                }
            }
        }

        DoDisconnect(&server);
    }

    return 0;
}

