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
#include "misc.h"

/* -------------------------------------------------------------------------- */
static int Hello(adbus_CbData* d)
{
    adbus_Server* s = (adbus_Server*) d->user2;
    adbus_Remote* r = adbus_serv_caller(s);

    adbus_check_end(d);

    if (r->haveHello) {
        return adbus_error(d, "nz.co.foobar.adbus.AlreadyHaveHello", -1, NULL, -1);
    }

    r->haveHello = 1;
    adbusI_requestService(s, r, ds_cstr(&r->unique), 0);

    if (d->ret) {
        adbus_msg_string(d->ret, ds_cstr(&r->unique), ds_size(&r->unique));
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int RequestName(adbus_CbData* d)
{
    int ret;
    adbus_Server* s = (adbus_Server*) d->user2;
    adbus_Remote* r = adbus_serv_caller(s);

    const char* name = adbus_check_string(d, NULL);
    uint32_t flags   = adbus_check_u32(d);
    adbus_check_end(d);

    if (name[0] == ':' || !adbusI_isValidBusName(name, strlen(name))) {
        return adbus_errorf(d, "TODO", "TODO");
    }

    ret = adbusI_requestService(s, r, name, flags);
    if (d->ret) {
        adbus_msg_u32(d->ret, ret);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int ReleaseName(adbus_CbData* d)
{
    int ret;
    adbus_Server* s = (adbus_Server*) d->user2;
    adbus_Remote* r = adbus_serv_caller(s);

    const char* name = adbus_check_string(d, NULL);
    adbus_check_end(d);

    if (name[0] == ':' || !adbusI_isValidBusName(name, strlen(name))) {
        return adbus_errorf(d, "TODO", "TODO");
    }

    ret = adbusI_releaseService(s, r, name);
    if (d->ret) {
        adbus_msg_u32(d->ret, ret);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int ListNames(adbus_CbData* d)
{
    adbus_Server* s = (adbus_Server*) d->user2;

    adbus_check_end(d);

    if (d->ret) {
        adbus_BufArray a;
        dh_Iter ii;
        d_Hash(ServiceQueue)* h = &s->services.queues;

        adbus_msg_beginarray(d->ret, &a);
        for (ii = dh_begin(h); ii != dh_end(h); ii++) {
            if (dh_exist(h, ii)) {
                adbusI_ServiceQueue* queue = dh_val(h, ii);
                adbus_msg_arrayentry(d->ret, &a);
                adbus_msg_string(d->ret, queue->name, -1);
            }
        }
        adbus_msg_endarray(d->ret, &a);

    }
    return 0;
}

/* -------------------------------------------------------------------------- */
static int NameHasOwner(adbus_CbData* d)
{
    adbus_Server* s = (adbus_Server*) d->user2;
    adbus_Remote* r;

    const char* name = adbus_check_string(d, NULL);
    adbus_check_end(d);

    r = adbusI_lookupRemote(s, name);

    if (d->ret) {
        adbus_msg_bool(d->ret, r != NULL);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int GetNameOwner(adbus_CbData* d)
{
    adbus_Server* s = (adbus_Server*) d->user2;
    adbus_Remote* r;

    const char* name = adbus_check_string(d, NULL);
    adbus_check_end(d);

    r = adbusI_lookupRemote(s, name);

    if (r == NULL) {
        return adbus_errorf(d, "org.freedesktop.DBus.Error.NameHasNoOwner", "TODO");
    }

    if (d->ret) {
        adbus_msg_string(d->ret, ds_cstr(&r->unique), ds_size(&r->unique));
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int AddMatch(adbus_CbData* d)
{
    adbus_Server* s = (adbus_Server*) d->user2;
    adbus_Remote* r = adbus_serv_caller(s);

    size_t msize;
    const char* mstr = adbus_check_string(d, &msize);
    adbus_check_end(d);

    if (adbusI_serv_addMatch(&r->matches, mstr, msize)) {
        return adbus_errorf(d, "TODO", "TODO");
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int RemoveMatch(adbus_CbData* d)
{
    adbus_Server* s = (adbus_Server*) d->user2;
    adbus_Remote* r = adbus_serv_caller(s);

    size_t msize;
    const char* mstr = adbus_check_string(d, &msize);
    adbus_check_end(d);

    if (adbusI_serv_removeMatch(&r->matches, mstr, msize)) {
        return adbus_errorf(d, "TODO", "TODO");
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int SendToBus(void* d, adbus_Message* m)
{
    adbus_Server* s = (adbus_Server*) d;
    if (adbus_conn_dispatch(s->bus.connection, m))
        return -1;

    return (int) m->size;
}

static int SendToServer(void* d, adbus_Message* m)
{
    adbus_Server* s = (adbus_Server*) d;
    if (adbus_remote_dispatch(s->bus.remote, m))
        return -1;

    return (int) m->size;
}

void adbusI_serv_initBus(adbus_Server* s, adbus_Interface* i)
{
    /* Setup the org.freedesktop.DBus interface */
    adbus_Member *m, *changedsig, *acquiredsig, *lostsig;

    s->bus.interface = i;

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

    changedsig = m = adbus_iface_addsignal(i, "NameOwnerChanged", -1);
    adbus_mbr_argsig(m, "sss", -1);
    adbus_mbr_argname(m, "name", -1);
    adbus_mbr_argname(m, "old_owner", -1);
    adbus_mbr_argname(m, "new_owner", -1);

    acquiredsig = m = adbus_iface_addsignal(i, "NameAcquired", -1);
    adbus_mbr_argsig(m, "s", -1);

    lostsig = m = adbus_iface_addsignal(i, "NameLost", -1);
    adbus_mbr_argsig(m, "s", -1);

    {
        /* Setup the bus connection */
        adbus_ConnectionCallbacks cbs;
        ZERO(cbs);
        cbs.send_message = &SendToServer;
        s->bus.connection = adbus_conn_new(&cbs, s);
    }

    s->bus.nameOwnerChanged = adbus_sig_new(changedsig);
    s->bus.nameAcquired = adbus_sig_new(acquiredsig);
    s->bus.nameLost = adbus_sig_new(lostsig);
    s->bus.msg = adbus_msg_new();

    {
        /* Bind to / and /org/freedesktop/DBus */
        adbus_Bind b;
        adbus_bind_init(&b);
        b.interface = i;
        b.cuser2    = s;

        b.path      = "/";
        adbus_conn_bind(s->bus.connection, &b);
        
        b.path      = "/org/freedesktop/DBus";
        adbus_conn_bind(s->bus.connection, &b);
    }

    adbus_sig_bind(s->bus.nameOwnerChanged, s->bus.connection, "/org/freedesktop/DBus", -1);
    adbus_sig_bind(s->bus.nameAcquired, s->bus.connection, "/org/freedesktop/DBus", -1);
    adbus_sig_bind(s->bus.nameLost, s->bus.connection, "/org/freedesktop/DBus", -1);

    /* Hook up to the server */
    s->bus.remote = adbusI_serv_createRemote(s, &SendToBus, s, "org.freedesktop.DBus", 0);
    adbusI_requestService(s, s->bus.remote, "org.freedesktop.DBus", 0);
}

/* -------------------------------------------------------------------------- */
void adbusI_serv_freeBus(adbus_Server* s)
{
    adbus_sig_free(s->bus.nameOwnerChanged);
    adbus_sig_free(s->bus.nameAcquired);
    adbus_sig_free(s->bus.nameLost);
    adbus_conn_free(s->bus.connection);
    adbus_msg_free(s->bus.msg);
}

/* -------------------------------------------------------------------------- */
void adbusI_serv_ownerChanged(
        adbus_Server*       s,
        const char*         name,
        adbus_Remote*       oldowner,
        adbus_Remote*       newowner)
{
    adbus_MsgFactory* m;

    assert(!newowner || newowner->haveHello);
    if (oldowner && !oldowner->haveHello)
        oldowner = NULL;

    if (!oldowner && !newowner)
        return;

    m = adbus_sig_msg(s->bus.nameOwnerChanged);
    adbus_msg_string(m, name, -1);
    adbus_msg_string(m, oldowner ? ds_cstr(&oldowner->unique) : "", -1);
    adbus_msg_string(m, newowner ? ds_cstr(&newowner->unique) : "", -1);
    adbus_sig_emit(s->bus.nameOwnerChanged);

    if (oldowner) {
        m = adbus_sig_msg(s->bus.nameLost);
        adbus_msg_setdestination(m, ds_cstr(&oldowner->unique), -1);
        adbus_msg_string(m, name, -1);
        adbus_sig_emit(s->bus.nameLost);
    }

    if (newowner) {
        m = adbus_sig_msg(s->bus.nameAcquired);
        adbus_msg_setdestination(m, ds_cstr(&newowner->unique), -1);
        adbus_msg_string(m, name, -1);
        adbus_sig_emit(s->bus.nameAcquired);
    }

}

/* -------------------------------------------------------------------------- */
void adbusI_serv_invalidDestination(adbus_Server* s, const adbus_Message* msg)
{
    adbus_MsgFactory* m = s->bus.msg;

    adbus_msg_reset(m);
    adbus_msg_settype(m, ADBUS_MSG_ERROR);
    adbus_msg_setflags(m, ADBUS_MSG_NO_REPLY);

    adbus_msg_setdestination(m, msg->sender, msg->senderSize);
    adbus_msg_setreply(m, msg->serial);
    adbus_msg_seterror(m, "nz.co.foobar.adbus.InvalidDestination", -1);

    adbus_msg_setsig(m, "s", 1);
    adbus_msg_string_f(m, "Invalid destination %s", msg->destination);

    adbus_msg_send(m, s->bus.connection);
}

