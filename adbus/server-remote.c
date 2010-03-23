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

#include "server-remote.h"
#include "server.h"

adbus_Remote* adbusI_serv_createRemote(
        adbus_Server*           s,
        adbus_SendMsgCallback   send,
        void*                   user,
        const char*             unique,
        adbus_Bool              needhello)
{
    adbus_Remote* r = NEW(adbus_Remote);
    r->server       = s;
    r->send         = send;
    r->user         = user;
    r->haveHello    = !needhello;

    adbusI_remote_initParser(&r->parser);

    if (unique) {
        ds_set(&r->unique, unique);
    } else {
        ds_set_f(&r->unique, ":1.%u", s->remotes.nextRemote++);
    }

    dl_insert_after(Remote, &s->remotes.async, r, &r->hl);

    return r;
}

/** Adds a new remote to the server
 *  \relates adbus_Server
 *
 *  \note This should be called after the remote has successfully
 *  authenticated.
 */
adbus_Remote* adbus_serv_connect(
        adbus_Server*           s,
        adbus_SendMsgCallback   send,
        void*                   user)
{
    return adbusI_serv_createRemote(s, send, user, NULL, 1);
}

/** Removes a remote from the server
 *  \relates adbus_Server
 */
void adbus_remote_disconnect(adbus_Remote* r)
{
    if (r) {
        adbus_Server* s = r->server;

        dl_remove(Remote, r, &r->hl);
        adbusI_serv_freeMatches(&r->matches);
        adbusI_remote_freeParser(&r->parser);

        while (dv_size(&r->services) > 0) {
            adbusI_releaseService(&s->services, r, dv_a(&r->services, 0)->name);
        }
        dv_free(ServiceQueue, &r->services);

        ds_free(&r->unique);
        free(r);
    }
}

void adbus_remote_setsynchronous(adbus_Remote* r, adbus_Bool sync)
{
    dl_remove(Remote, r, &r->hl);
    if (sync) {
        dl_insert_after(Remote, &r->server->remotes.sync, r, &r->hl);
    } else {
        dl_insert_after(Remote, &r->server->remotes.async, r, &r->hl);
    }
}

