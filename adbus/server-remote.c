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

/** Adds a new remote to the server
 *  \relates adbus_Server
 *
 *  \note This should be called after the remote has successfully
 *  authenticated.
 */
adbus_Remote* adbus_serv_connect(
        adbus_Server*           s,
        adbus_SendMsgCallback   send,
        void*                   data)
{
    adbus_Remote* r = NEW(adbus_Remote);
    r->server       = s;
    r->send         = send;
    r->data         = data;
    r->msg          = adbus_buf_new();
    r->dispatch     = adbus_buf_new();

    if (s->busRemote == NULL) {
        ds_set(&r->unique, "org.freedesktop.DBus");
    } else {
        ds_set_f(&r->unique, ":1.%u", s->nextRemote++);
    }

    dl_insert_after(Remote, &s->remotes, r, &r->hl);

    return r;
}

/** Removes a remote from the server
 *  \relates adbus_Server
 */
void adbus_remote_disconnect(adbus_Remote* r)
{
    if (r == NULL)
        return;

    adbus_Server* s = r->server;

    dl_remove(Remote, r, &r->hl);

    // Free the matches
    struct Match* m = r->matches.next;
    while (m) {
        struct Match* next = m->hl.next;
        adbusI_serv_freematch(m);
        m = next;
    }
    dl_clear(Match, &r->matches);

    while (dv_size(&r->services) > 0) {
        adbusI_serv_releasename(s, r, dv_a(&r->services, 0)->name);
    }
    dv_free(Service, &r->services);


    adbus_buf_free(r->msg);
    adbus_buf_free(r->dispatch);
    ds_free(&r->unique);
    free(r);
}

/* -------------------------------------------------------------------------- */
adbus_Remote* adbusI_serv_remote(adbus_Server* s, const char* name)
{
    dh_Iter ii = dh_get(Service, &s->services, name);
    if (ii == dh_end(&s->services))
        return NULL;

    struct Service* serv = dh_val(&s->services, ii);
    return dv_a(&serv->queue, 0).remote;
}