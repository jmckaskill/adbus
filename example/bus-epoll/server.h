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

#pragma once

#include "adbus.h"
#include "dmem/list.h"

struct Remote;
DILIST_INIT(Remote, struct Remote);

struct Server
{
    int             efd;
    int             fd;
    adbus_Server*   bus;
    d_IList(Remote) remotes;
    d_IList(Remote) toflush;
    d_IList(Remote) tofree;
};

struct Server* CreateServer(int efd, int fd);
void FreeServer(struct Server* s);
void ServerRecv(struct Server* s);
void ServerIdle(struct Server* s);


struct Remote
{
    d_IList(Remote) hl;
    d_IList(Remote) fl;
    int             fd;
    adbus_Auth*     auth;
    adbus_Remote*   remote;
    adbus_Buffer*   txbuf;
    adbus_Buffer*   rxbuf;
    struct Server*  server;
    adbus_Bool      txfull;
};

struct Remote* CreateRemote(int fd, struct Server* s);
void    FreeRemote(struct Remote* r);
void    FlushRemote(struct Remote* r);
int     RemoteSendMsg(void* u, adbus_Message* m);
int     RemoteSend(void* u, const char* data, size_t sz);
uint8_t RemoteRand(void* u);
void    RemoteRecv(struct Remote* r);
void    RemoteDisconnect(struct Remote* r);


