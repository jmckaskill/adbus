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

#ifdef __linux__
#   define _GNU_SOURCE
#endif

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

#ifndef _WIN32
#   define closesocket(x) close(x)
#endif

/* -------------------------------------------------------------------------- */

Server* Server_New(adbus_Socket sock)
{
    Server* s   = NEW(Server);
    s->sock     = sock;
    s->bus      = adbus_serv_new(adbus_iface_new("org.freedesktop.DBus", -1));
    s->reg      = MT_Current_RegisterHandle(sock, &Server_OnConnect, s);

#if !defined _WIN32
    fcntl(sock, F_SETFL, O_CLOEXEC);
    fcntl(sock, F_SETFL, O_NONBLOCK);
#endif

    return s;
}

/* -------------------------------------------------------------------------- */

void Server_Free(struct Server* s)
{
    if (s) {
        Remote* r;
        DIL_FOREACH (Remote, r, &s->remotes, hl) {
            Remote_Free(r);
        }
        MT_Current_Unregister(s->reg);
        adbus_serv_free(s->bus);
        closesocket(s->sock);
        free(s);
    }
}

/* -------------------------------------------------------------------------- */

void Server_OnConnect(void* u)
{
    Server* s = (Server*) u;
    Remote* remote;
    adbus_Socket sock;

    /* Accept connections until it starts to fail (with EWOULDBLOCK) */
    while (1) {
        sock = accept(s->sock, NULL, NULL);

        if (sock == ADBUS_SOCK_INVALID)
            return;

#if !defined _WIN32
        fcntl(sock, F_SETFL, O_CLOEXEC);
        fcntl(sock, F_SETFL, O_NONBLOCK);
#endif

        remote = Remote_New(sock, s->bus);
        dil_insert_after(Remote, &s->remotes, remote, &remote->hl);
    }
}
















/* -------------------------------------------------------------------------- */

Remote* Remote_New(adbus_Socket sock, adbus_Server* s)
{
    Remote* r   = NEW(Remote);
    r->txbuf    = adbus_buf_new();
    r->rxbuf    = adbus_buf_new();
    r->txfull   = 0;
    r->bus      = s;
    r->sock     = sock;
    r->idle     = MT_Current_RegisterIdle(&Remote_OnIdle, r);

    r->reg = MT_Current_RegisterSocket(
            sock,
            &Remote_ReadyRecv,
            &Remote_ReadySend,
            &Remote_OnClose,
            r);

    MT_Current_Disable(r->reg, MT_LOOP_WRITE);
    r->readySendEnabled = 0;

    return r;
}

/* -------------------------------------------------------------------------- */

void Remote_Free(Remote* r)
{
    if (r) {
        dil_remove(Remote, r, &r->hl);

        adbus_remote_disconnect(r->remote);
        adbus_auth_free(r->auth);
        adbus_buf_free(r->txbuf);
        adbus_buf_free(r->rxbuf);

        MT_Current_Unregister(r->reg);
        MT_Current_Unregister(r->idle);

        closesocket(r->sock);

        free(r);
    }
}

/* -------------------------------------------------------------------------- */

void Remote_OnClose(void* u)
{ 
    Remote* r = (Remote*) u;
    Remote_Free(r); 
}

/* -------------------------------------------------------------------------- */

void Remote_OnIdle(void* u)
{
    Remote* r = (Remote*) u;
    if (!r->txfull) {
        Remote_FlushTxBuffer(r);
    }
}

/* -------------------------------------------------------------------------- */

void Remote_ReadySend(void* u)
{
    Remote* r = (Remote*) u;
    r->txfull = 0;
    Remote_FlushTxBuffer(r);
}

/* -------------------------------------------------------------------------- */

void Remote_FlushTxBuffer(Remote* r)
{
    size_t sz = adbus_buf_size(r->txbuf);

    if (sz != 0) {
        int sent = send(r->sock, adbus_buf_data(r->txbuf), sz, 0);

        if (sent == sz) {
            adbus_buf_reset(r->txbuf);
        } else if (sent > 0) {
            adbus_buf_remove(r->txbuf, 0, sent);
        }

        r->txfull = (sent != sz);

        if (!r->txfull && r->readySendEnabled) {
            r->readySendEnabled = 0;
            MT_Current_Disable(r->reg, MT_LOOP_WRITE);

        } else if (r->txfull && !r->readySendEnabled) {
            r->readySendEnabled = 1;
            MT_Current_Enable(r->reg, MT_LOOP_WRITE);

        }
    }
}

/* -------------------------------------------------------------------------- */

int Remote_SendMsg(void* u, adbus_Message* m)
{
    Remote* r = (Remote*) u;
    adbus_buf_append(r->txbuf, m->data, m->size);
    return m->size;
}

/* -------------------------------------------------------------------------- */

int Remote_Send(void* u, const char* data, size_t sz)
{
    Remote* r = (Remote*) u;
    return send(r->sock, data, sz, 0);
}

/* -------------------------------------------------------------------------- */

uint8_t Remote_Rand(void* u)
{
    (void) u;
    return (uint8_t) rand();
}

/* -------------------------------------------------------------------------- */

#define RECV_SIZE 64 * 1024

void Remote_ReadyRecv(void* u)
{
    Remote* r = (Remote*) u;
    int recvd;

    do {
        char* dest = adbus_buf_recvbuf(r->rxbuf, RECV_SIZE);
        recvd = recv(r->sock, dest, RECV_SIZE, 0);
        adbus_buf_recvd(r->rxbuf, RECV_SIZE, recvd);
    } while (recvd == RECV_SIZE);

#ifndef _WIN32
    if (recvd < 0 && errno == EAGAIN) {
        recvd = 0;
    }
#endif

    while (adbus_buf_size(r->rxbuf) > 0) {
        if (r->remote) {
            if (adbus_remote_parse(r->remote, r->rxbuf)) {
                Remote_Free(r);
                return;
            }
            break;

        } else if (r->auth) {
            adbus_Bool finished;
            char* data = adbus_buf_data(r->rxbuf);
            size_t size = adbus_buf_size(r->rxbuf);
            int used = adbus_auth_parse(r->auth, data, size, &finished);

            if (used < 0) {
                Remote_Free(r);
                return;
            }

            adbus_buf_remove(r->rxbuf, 0, used);

            if (finished) {
                adbus_auth_free(r->auth);
                r->auth = NULL;
                r->remote = adbus_serv_connect(r->bus, &Remote_SendMsg, r);
            } else {
                break;
            }

        } else {
            char* d = adbus_buf_data(r->rxbuf);
            if (*d != '\0') {
                Remote_Free(r);
                return;
            }
            adbus_buf_remove(r->rxbuf, 0, 1);
            r->auth = adbus_sauth_new(&Remote_Send, &Remote_Rand, r);
            adbus_sauth_external(r->auth, NULL);
        }
    }

    if (recvd < 0) {
        Remote_Free(r);
    }
}

