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


#include "server.h"
#include <sys/epoll.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define EVENT_NUM 4096
int main()
{
    int ready, i;
    struct epoll_event events[EVENT_NUM];
    struct Server* server;
    adbus_Socket sock;
    int efd = epoll_create(EPOLL_CLOEXEC);

    sock = adbus_sock_bind(ADBUS_DEFAULT_BUS);
    if (sock == ADBUS_SOCK_INVALID)
        abort();

    server = CreateServer(efd, sock);

    if (listen(sock, SOMAXCONN))
        abort();

    while (1) {
        ServerIdle(server);
        ready = epoll_wait(efd, events, EVENT_NUM, -1);
        if (ready < 0)
            continue;

        for (i = 0; i < ready; i++) {
            struct epoll_event* e = &events[i];
            if (e->data.ptr == server) {
                if (e->events & EPOLLIN) {
                    ServerRecv(server);
                }
            } else {
                struct Remote* r = (struct Remote*) e->data.ptr;
                if (e->events & EPOLLIN) {
                    RemoteRecv(r);
                }
                if (e->events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                    RemoteDisconnect(r);
                    continue;
                }
                if (e->events & EPOLLOUT) {
                    FlushRemote(r);
                }
            }
        }

    }

    return 0;
}

