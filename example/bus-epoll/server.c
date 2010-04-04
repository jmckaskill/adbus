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

#include "server.h"
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#define NEW(t) (t*) calloc(1, sizeof(t))
#define ZERO(v) memset(&v, 0, sizeof(v))

/* -------------------------------------------------------------------------- */

struct Server* CreateServer(int efd, int fd)
{
    struct epoll_event event;
    adbus_Interface* iface = adbus_iface_new("org.freedesktop.DBus", -1);
    struct Server* s = NEW(struct Server);

    s->fd   = fd;
    s->efd  = efd;
    s->bus  = adbus_serv_new(iface);

    fcntl(fd, F_SETFL, O_NONBLOCK);

    ZERO(event);
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = s;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event))
        abort();

    return s;
}

/* -------------------------------------------------------------------------- */

void FreeServer(struct Server* s)
{
    if (s) {
        struct Remote* r;
        DIL_FOREACH (Remote, r, &s->remotes, hl) {
            RemoteDisconnect(r);
        }
        adbus_serv_free(s->bus);
        close(s->fd);
        free(s);
    }
}

/* -------------------------------------------------------------------------- */

void ServerRecv(struct Server* s)
{
    /* Accept connections until it starts to fail (with EWOULDBLOCK) */
    while (1) {
        struct Remote* remote;
        struct epoll_event reg;
        int fd = accept4(s->fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0)
            return;

        remote = CreateRemote(fd, s);
        dil_insert_after(Remote, &s->remotes, remote, &remote->hl);

        ZERO(reg);
        reg.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLRDHUP;
        reg.data.ptr = remote;
        if (epoll_ctl(s->efd, EPOLL_CTL_ADD, fd, &reg))
            abort();

    }
}

/* -------------------------------------------------------------------------- */

void ServerIdle(struct Server* s)
{
    struct Remote* r;
    DIL_FOREACH (Remote, r, &s->toflush, fl) {
        FlushRemote(r);
    }

    DIL_FOREACH (Remote, r, &s->tofree, fl) {
        FreeRemote(r);
    }
}







/* -------------------------------------------------------------------------- */

struct Remote* CreateRemote(int fd, struct Server* s)
{
    struct Remote* r = NEW(struct Remote);
    r->txbuf = adbus_buf_new();
    r->rxbuf = adbus_buf_new();
    r->server = s;
    r->fd = fd;
    return r;
}

/* -------------------------------------------------------------------------- */

void FreeRemote(struct Remote* r)
{
    if (r) {
        dil_remove(Remote, r, &r->hl);
        dil_remove(Remote, r, &r->fl);

        adbus_remote_disconnect(r->remote);
        adbus_auth_free(r->auth);
        adbus_buf_free(r->txbuf);
        adbus_buf_free(r->rxbuf);

        if (r->fd >= 0) {
            close(r->fd);
        }

        free(r);
    }
}

/* -------------------------------------------------------------------------- */

void RemoteDisconnect(struct Remote* r)
{
    dil_remove(Remote, r, &r->hl);
    dil_remove(Remote, r, &r->fl);

    adbus_remote_disconnect(r->remote);
    adbus_auth_free(r->auth);
    adbus_buf_free(r->txbuf);
    adbus_buf_free(r->rxbuf);

    close(r->fd);

    r->fd = -1;
    r->txbuf = NULL;
    r->rxbuf = NULL;
    r->remote = NULL;
    r->auth = NULL;

    dil_insert_after(Remote, &r->server->tofree, r, &r->hl);
}

/* -------------------------------------------------------------------------- */

void FlushRemote(struct Remote* r)
{
    int sent;
    size_t sz = adbus_buf_size(r->txbuf);
    if (sz == 0 || r->fd < 0)
        return;

    sent = send(r->fd, adbus_buf_data(r->txbuf), sz, 0);

    if (sent > 0) {
        adbus_buf_remove(r->txbuf, 0, sent);
    }

    /* Always remove from toflush - since if the send failed or did not fully
     * complete we need to wait for EPOLLOUT which will call us directly
     */
    dil_remove(Remote, r, &r->fl);
    r->txfull = (sent != sz);
}

/* -------------------------------------------------------------------------- */

int RemoteSendMsg(void* u, adbus_Message* m)
{
    struct Remote* r = (struct Remote*) u;
    adbus_buf_append(r->txbuf, m->data, m->size);

    if (r->fd >= 0 && !r->txfull && !r->fl.next && !r->fl.prev) {
        dil_insert_after(Remote, &r->server->toflush, r, &r->fl);
    }

    return m->size;
}

/* -------------------------------------------------------------------------- */

int RemoteSend(void* u, const char* data, size_t sz)
{
    struct Remote* r = (struct Remote*) u;
    return send(r->fd, data, sz, 0);
}

/* -------------------------------------------------------------------------- */

uint8_t RemoteRand(void* u)
{
    (void) u;
    return (uint8_t) rand();
}

/* -------------------------------------------------------------------------- */

#define RECV_SIZE 64 * 1024

void RemoteRecv(struct Remote* r)
{
    ssize_t recvd;
    if (r->fd < 0)
        return;

    do {
        char* dest = adbus_buf_recvbuf(r->rxbuf, RECV_SIZE);
        recvd = recv(r->fd, dest, RECV_SIZE, 0);
        adbus_buf_recvd(r->rxbuf, RECV_SIZE, recvd);
    } while (recvd == RECV_SIZE);

    while (adbus_buf_size(r->rxbuf) > 0) {
        if (r->remote) {
            if (adbus_remote_parse(r->remote, r->rxbuf)) {
                RemoteDisconnect(r);
                return;
            }
            break;

        } else if (r->auth) {
            adbus_Bool finished;
            char* data = adbus_buf_data(r->rxbuf);
            size_t size = adbus_buf_size(r->rxbuf);
            int used = adbus_auth_parse(r->auth, data, size, &finished);

            if (used < 0) {
                RemoteDisconnect(r);
                return;
            }

            adbus_buf_remove(r->rxbuf, 0, used);

            if (finished) {
                adbus_auth_free(r->auth);
                r->auth = NULL;
                r->remote = adbus_serv_connect(r->server->bus, &RemoteSendMsg, r);
            } else {
                break;
            }

        } else {
            char* d = adbus_buf_data(r->rxbuf);
            if (*d != '\0') {
                RemoteDisconnect(r);
                return;
            }
            adbus_buf_remove(r->rxbuf, 0, 1);
            r->auth = adbus_sauth_new(&RemoteSend, &RemoteRand, r);
            adbus_sauth_external(r->auth, NULL);
        }
    }

    if (recvd < 0 && recvd != EAGAIN) {
        RemoteDisconnect(r);
    }
}

