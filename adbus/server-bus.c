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

#define ADBUS_LIBRARY
#include "server.h"

/* -------------------------------------------------------------------------- */
static int Hello(adbus_CbData* d)
{
    adbus_check_end(d);

    adbus_Server* s = (adbus_Server*) d->user2;
    adbus_Remote* r = s->helloRemote;

    if (r == NULL || r->haveHello) {
        return adbus_error(d, "nz.co.foobar.adbus.AlreadyHaveHello", -1, NULL, -1);
    }

    r->haveHello = 1;
    adbusI_serv_requestname(s, r, d->msg->sender, 0);

    if (d->ret) {
        adbus_msg_setsig(d->ret, "s", -1);
        adbus_msg_string(d->ret, d->msg->sender, -1);
        adbus_msg_end(d->ret);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int RequestName(adbus_CbData* d)
{
    const char* name = adbus_check_string(d, NULL);
    uint32_t flags   = adbus_check_u32(d);
    adbus_check_end(d);

    adbus_Server* s = (adbus_Server*) d->user2;
    adbus_Remote* r = adbusI_serv_remote(s, d->msg->sender);

    if (name[0] == ':' || !adbusI_isValidBusName(name, strlen(name))) {
        return adbus_errorf(d, "TODO", "TODO");
    }

    int ret = adbusI_serv_requestname(s, r, name, flags);
    if (d->ret) {
        adbus_msg_setsig(d->ret, "u", -1);
        adbus_msg_u32(d->ret, ret);
        adbus_msg_end(d->ret);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int ReleaseName(adbus_CbData* d)
{
    const char* name = adbus_check_string(d, NULL);
    adbus_check_end(d);

    adbus_Server* s = (adbus_Server*) d->user2;
    adbus_Remote* r = adbusI_serv_remote(s, d->msg->sender);

    if (name[0] == ':' || !adbusI_isValidBusName(name, strlen(name))) {
        return adbus_errorf(d, "TODO", "TODO");
    }

    int ret = adbusI_serv_releasename(s, r, name);
    if (d->ret) {
        adbus_msg_setsig(d->ret, "u", -1);
        adbus_msg_u32(d->ret, ret);
        adbus_msg_end(d->ret);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int ListNames(adbus_CbData* d)
{
    adbus_check_end(d);

    adbus_Server* s = (adbus_Server*) d->user2;

    if (d->ret) {
        adbus_BufArray a;
        adbus_msg_setsig(d->ret, "as", -1);
        adbus_msg_beginarray(d->ret, &a);
        dh_Iter ii;
        for (ii = dh_begin(&s->services); ii != dh_end(&s->services); ii++) {
            if (dh_exist(&s->services, ii)) {
                struct Service* serv = dh_val(&s->services, ii);
                adbus_msg_arrayentry(d->ret, &a);
                adbus_msg_string(d->ret, serv->name, -1);
            }
        }
        adbus_msg_endarray(d->ret, &a);
        adbus_msg_end(d->ret);

    }
    return 0;
}

/* -------------------------------------------------------------------------- */
static int NameHasOwner(adbus_CbData* d)
{
    const char* name = adbus_check_string(d, NULL);
    adbus_check_end(d);

    adbus_Server* s = (adbus_Server*) d->user2;
    adbus_Remote* r = adbusI_serv_remote(s, name);

    if (d->ret) {
        adbus_msg_setsig(d->ret, "b", -1);
        adbus_msg_bool(d->ret, r != NULL);
        adbus_msg_end(d->ret);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int GetNameOwner(adbus_CbData* d)
{
    const char* name = adbus_check_string(d, NULL);
    adbus_check_end(d);

    adbus_Server* s = (adbus_Server*) d->user2;
    adbus_Remote* r = adbusI_serv_remote(s, name);

    if (r == NULL) {
        return adbus_errorf(d, "org.freedesktop.DBus.Error.NameHasNoOwner", "TODO");
    }

    if (d->ret) {
        adbus_msg_setsig(d->ret, "s", -1);
        adbus_msg_string(d->ret, ds_cstr(&r->unique), ds_size(&r->unique));
        adbus_msg_end(d->ret);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int AddMatch(adbus_CbData* d)
{
    size_t msize;
    const char* mstr = adbus_check_string(d, &msize);
    adbus_check_end(d);

    struct Match* m = adbusI_serv_newmatch(mstr, msize);
    if (m == NULL) {
        return adbus_errorf(d, "TODO", "TODO");
    }

    adbus_Server* s = (adbus_Server*) d->user2;
    adbus_Remote* r = adbusI_serv_remote(s, d->msg->sender);

    dl_insert_after(Match, &r->matches, m, &m->hl);
    return 0;
}

/* -------------------------------------------------------------------------- */
static int RemoveMatch(adbus_CbData* d)
{
    size_t msize;
    const char* mstr = adbus_check_string(d, &msize);
    adbus_check_end(d);

    adbus_Server* s = (adbus_Server*) d->user2;
    adbus_Remote* r = adbusI_serv_remote(s, d->msg->sender);

    struct Match* m;
    DL_FOREACH(Match, m, &r->matches, hl) {
        if (msize == m->size && memcmp(m->data, mstr, msize) == 0) {
            dl_remove(Match, m, &m->hl);
            adbusI_serv_freematch(m);
            break;
        }
    }

    if (m == NULL) {
        adbus_errorf(d, "TODO", "TODO");
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static adbus_ssize_t SendToBus(void* d, adbus_Message* m)
{
    adbus_Server* s = (adbus_Server*) d;
    if (adbus_conn_dispatch(s->busConnection, m))
        return -1;

    return m->size;
}

static adbus_ssize_t SendToServer(void* d, adbus_Message* m)
{
    adbus_Server* s = (adbus_Server*) d;
    if (adbus_remote_dispatch(s->busRemote, m))
        return -1;

    return m->size;
}

void adbusI_serv_initbus(adbus_Server* s)
{
    // Setup the org.freedesktop.DBus interface
    adbus_Interface* i = s->busInterface;
    adbus_Member* m;

    m = adbus_iface_addmethod(i, "Hello", -1);
    adbus_mbr_setmethod(m, &Hello, NULL);
    adbus_mbr_retsig(m, "s", -1);
    adbus_mbr_retname(m, "unique_id", -1);

    m = adbus_iface_addmethod(i, "RequestName", -1);
    adbus_mbr_setmethod(m, &RequestName, NULL);
    adbus_mbr_argsig(m, "su", -1);
    adbus_mbr_argname(m, "name", -1);
    adbus_mbr_argname(m, "flags", -1);
    adbus_mbr_retsig(m, "u", -1);

    m = adbus_iface_addmethod(i, "ReleaseName", -1);
    adbus_mbr_setmethod(m, &ReleaseName, NULL);
    adbus_mbr_argsig(m, "s", -1);
    adbus_mbr_argname(m, "name", -1);
    adbus_mbr_retsig(m, "u", -1);

    m = adbus_iface_addmethod(i, "ListNames", -1);
    adbus_mbr_setmethod(m, &ListNames, NULL);
    adbus_mbr_retsig(m, "as", -1);

    m = adbus_iface_addmethod(i, "NameHasOwner", -1);
    adbus_mbr_setmethod(m, &NameHasOwner, NULL);
    adbus_mbr_argsig(m, "s", -1);
    adbus_mbr_retsig(m, "b", -1);

    m = adbus_iface_addmethod(i, "GetNameOwner", -1);
    adbus_mbr_setmethod(m, &GetNameOwner, NULL);
    adbus_mbr_argsig(m, "s", -1);
    adbus_mbr_retsig(m, "s", -1);

    m = adbus_iface_addmethod(i, "AddMatch", -1);
    adbus_mbr_setmethod(m, &AddMatch, NULL);
    adbus_mbr_argsig(m, "s", -1);
    adbus_mbr_argname(m, "match_string", -1);

    m = adbus_iface_addmethod(i, "RemoveMatch", -1);
    adbus_mbr_setmethod(m, &RemoveMatch, NULL);
    adbus_mbr_argsig(m, "s", -1);
    adbus_mbr_argname(m, "match_string", -1);

    adbus_Member* changedsig = adbus_iface_addsignal(i, "NameOwnerChanged", -1);
    adbus_mbr_argsig(changedsig, "sss", -1);
    adbus_mbr_argname(changedsig, "name", -1);
    adbus_mbr_argname(changedsig, "old_owner", -1);
    adbus_mbr_argname(changedsig, "new_owner", -1);

    adbus_Member* acquiredsig = adbus_iface_addsignal(i, "NameAcquired", -1);
    adbus_mbr_argsig(acquiredsig, "s", -1);

    adbus_Member* lostsig = adbus_iface_addsignal(i, "NameLost", -1);
    adbus_mbr_argsig(acquiredsig, "s", -1);

    // Setup the bus connection
    adbus_ConnectionCallbacks cbs = {};
    cbs.send_message = &SendToServer;

    s->busConnection = adbus_conn_new(&cbs, s);
    s->nameOwnerChanged = adbus_sig_new(changedsig);
    s->nameAcquired = adbus_sig_new(acquiredsig);
    s->nameLost = adbus_sig_new(lostsig);

    // Bind to / and /org/freedesktop/DBus
    adbus_Bind b;
    adbus_bind_init(&b);
    b.interface = i;
    b.cuser2    = s;

    b.path      = "/";
    adbus_conn_bind(s->busConnection, &b);
    
    b.path      = "/org/freedesktop/DBus";
    adbus_conn_bind(s->busConnection, &b);

    adbus_sig_bind(s->nameOwnerChanged, s->busConnection, "/org/freedesktop/DBus", -1);
    adbus_sig_bind(s->nameAcquired, s->busConnection, "/org/freedesktop/DBus", -1);
    adbus_sig_bind(s->nameLost, s->busConnection, "/org/freedesktop/DBus", -1);

    // Hook up to the server
    s->busRemote = adbus_serv_connect(s, &SendToBus, s);

    // Manual hello
    s->busRemote->haveHello = 1;
    adbusI_serv_requestname(s, s->busRemote, ds_cstr(&s->busRemote->unique), 0);
}

/* -------------------------------------------------------------------------- */
void adbusI_serv_freebus(adbus_Server* s)
{
    adbus_sig_free(s->nameOwnerChanged);
    adbus_sig_free(s->nameAcquired);
    adbus_sig_free(s->nameLost);
    adbus_conn_free(s->busConnection);
}

/* -------------------------------------------------------------------------- */
void adbusI_serv_ownerchanged(
        adbus_Server*   s,
        const char*     name,
        adbus_Remote*   oldowner,
        adbus_Remote*   newowner)
{
    adbus_MsgFactory* m;

    assert(!newowner || newowner->haveHello);
    if (oldowner && !oldowner->haveHello)
        oldowner = NULL;

    m = adbus_sig_msg(s->nameOwnerChanged);
    adbus_msg_setsig(m, "sss", -1);
    adbus_msg_string(m, name, -1);
    adbus_msg_string(m, oldowner ? ds_cstr(&oldowner->unique) : "", -1);
    adbus_msg_string(m, newowner ? ds_cstr(&newowner->unique) : "", -1);
    adbus_sig_emit(s->nameOwnerChanged);

    if (oldowner) {
        m = adbus_sig_msg(s->nameLost);
        adbus_msg_setdestination(m, ds_cstr(&oldowner->unique), -1);
        adbus_msg_setsig(m, "s", -1);
        adbus_msg_string(m, name, -1);
        adbus_sig_emit(s->nameLost);
    }

    if (newowner) {
        m = adbus_sig_msg(s->nameAcquired);
        adbus_msg_setdestination(m, ds_cstr(&newowner->unique), -1);
        adbus_msg_setsig(m, "s", -1);
        adbus_msg_string(m, name, -1);
        adbus_sig_emit(s->nameAcquired);
    }

}

