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
#include "misc.h"

/** \struct adbus_Server
 *  \brief Bus server
 *
 *  The provided bus server is a minimal single threaded server. The server by
 *  default provides the following members of the "org.freedesktop.DBus"
 *  interface on the "/" and "/org/freedesktop/DBus" paths:
 *  - Hello
 *  - RequestName
 *  - ReleaseName
 *  - ListNames
 *  - NameHasOwner
 *  - GetNameOwner
 *  - AddMatch
 *  - RemoveMatch
 *  - NameOwnerChanged
 *  - NameAcquired
 *  - NameLost
 *
 *  The call to adbus_serv_new() takes a fresh "org.freedesktop.DBus"
 *  adbus_Interface, so that the user API can add application specific or
 *  platform specific members.
 *
 *  The overall workflow is:
 *  -# Remote connects to server.
 *  -# Run the server side of the auth protocol (perhaps using the adbus_Auth
 *  module).
 *  -# Call adbus_serv_connect() to get an adbus_Remote for that remote.
 *  -# Receive incoming data on the remote and call adbus_remote_dispatch() or
 *  adbus_remote_parse() to dispatch.
 *  -# When the remote disconnects call adbus_remote_disconnect() to cleanup.
 *
 */

/** \struct adbus_Remote
 *  \brief Handle used by adbus_Server to refer to a specific remote
 */

/** Creates a new server using the provided fresh "org.freedesktop.DBus"
 *  interface.
 *  \relates adbus_Server
 */
adbus_Server* adbus_serv_new(adbus_Interface* bus)
{
    adbus_Server* s = NEW(adbus_Server);
    s->busInterface = bus;
    adbus_iface_ref(bus);

    adbusI_serv_initbus(s);

    return s;
}

/** Frees the server
 *  \relates adbus_Server
 */
void adbus_serv_free(adbus_Server* s)
{
    if (s == NULL)
        return;

    adbus_Remote* r = s->remotes.next;
    while (r) {
        adbus_Remote* next = r->hl.next;
        adbus_remote_disconnect(r);
        r = next;
    }
    dl_clear(Remote, &s->remotes);

    for (dh_Iter ii = dh_begin(&s->services); ii != dh_end(&s->services); ++ii) {
        if (dh_exist(&s->services, ii)) {
            adbusI_serv_freeservice(dh_val(&s->services, ii));
        }
    }
    dh_clear(Service, &s->services);

    dh_free(Service, &s->services);
    adbusI_serv_freebus(s);
    adbus_iface_deref(s->busInterface);
    free(s);
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

/* -------------------------------------------------------------------------- */
static int IsArgKey(const char* beg, const char* end, int* num)
{
    if (end - beg < 3 || strncmp(beg, "arg", 3) != 0)
    {
        return 0;
    }
    // arg0 - arg9
    else if (   end - beg == 4
            &&  '0' <= beg[3] && beg[3] <= '9')
    {
        *num = (beg[3] - '0');
        return 1;
    }
    // arg10 - arg63
    else if (   end - beg == 5
            &&  '0' <= beg[3] && beg[3] <= '9'
            &&  '0' <= beg[4] && beg[4] <= '9')
    {
        *num = (beg[3] - '0') * 10
             + (beg[4] - '0');
        return *num <= 63;
    }
    else
    {
        return 0;
    }
}

struct Match* adbusI_serv_newmatch(const char* mstr, size_t len)
{
    d_Vector(Argument) args;
    ZERO(&args);

    struct Match* m = (struct Match*) calloc(1, sizeof(struct Match) + len + 1);
    m->size = len;
    memcpy(m->data, mstr, len);

    const char* line = m->data;
    const char* end  = m->data + len;
    while (line < end) {
        // Look for a key/value pair "<key>='<value>',"
        // Comma is optional for the last key/value pair
        const char* keyb = line;
        const char* keye = (const char*) memchr(keyb, '=', end - keyb);
        // keye needs to be followed by '
        if (!keye || keye + 1 >= end || keye[1] != '\'')
            goto error;

        const char* valb = keye+2;
        const char* vale = (const char*) memchr(valb, '\'', end - valb);
        // vale is either the last character or followed by a ,
        if (!vale || (vale + 1 != end && vale[1] != ',' ))
            goto error;

        line = vale + 2;

#define MATCH(BEG, END, STR)                        \
        (   END - BEG == sizeof(STR) - 1            \
         && memcmp(BEG, STR, END - BEG) == 0)

        int argnum;
        if (MATCH(keyb, keye, "type")) {
            if (MATCH(valb, vale, "signal")) {
                m->type = ADBUS_MSG_SIGNAL;
            } else if (MATCH(valb, vale, "method_call")) {
                m->type = ADBUS_MSG_METHOD;
            } else if (MATCH(valb, vale, "method_return")) {
                m->type = ADBUS_MSG_RETURN;
            } else if (MATCH(valb, vale, "error")) {
                m->type = ADBUS_MSG_ERROR;
            } else {
                goto error;
            }

        } else if (MATCH(keyb, keye, "sender")) {
            m->sender           = valb;
            m->senderSize       = vale - valb;

        } else if (MATCH(keyb, keye, "interface")) {
            m->interface        = valb;
            m->interfaceSize    = vale - valb;

        } else if (MATCH(keyb, keye, "member")) {
            m->member           = valb;
            m->memberSize       = vale - valb;

        } else if (MATCH(keyb, keye, "path")) {
            m->path             = valb;
            m->pathSize         = vale - valb;

        } else if (MATCH(keyb, keye, "destination")) {
            m->destination      = valb;
            m->destinationSize  = vale - valb;

        } else if (IsArgKey(keyb, keye, &argnum)) {
            int toadd = argnum + 1 - (int) dv_size(&args);
            if (toadd > 0) {
                adbus_Argument* a = dv_push(Argument, &args, toadd);
                adbus_arg_init(a, toadd);
            }

            adbus_Argument* a = &dv_a(&args, argnum);
            a->value = valb;
            a->size  = (int) (vale - valb);
        }

    }

    m->argumentsSize = dv_size(&args);
    m->arguments     = dv_release(Argument, &args);

    return m;

error:
    dv_free(Argument, &args);
    free(m);
    return NULL;
}

/* -------------------------------------------------------------------------- */
void adbusI_serv_freematch(struct Match* m)
{
    if (m) {
        free(m->arguments);
        free(m);
    }
}

/* -------------------------------------------------------------------------- */
void adbusI_serv_freeservice(struct Service* s)
{
    if (s) {
        dv_free(ServiceOwner, &s->queue);
        free(s);
    }
}

/* -------------------------------------------------------------------------- */
int adbusI_serv_requestname(
        adbus_Server*   s,
        adbus_Remote*   r,
        const char*     name,
        uint32_t        flags)
{
    int added;
    dh_Iter ii = dh_put(Service, &s->services, name, &added);

    if (added) {
        struct Service** pserv = &dh_val(&s->services, ii);
        *pserv = NEW(struct Service);
        (*pserv)->name = adbusI_strdup(name);
        dh_key(&s->services, ii) = (*pserv)->name;
    }

    struct Service* serv = dh_val(&s->services, ii);

    if (dv_size(&serv->queue) == 0) {
        struct Service** pserv = dv_push(Service, &r->services, 1);
        *pserv = serv;

        struct ServiceOwner* o = dv_push(ServiceOwner, &serv->queue, 1);
        o->remote = r;
        o->allowReplacement = flags & ADBUS_SERVICE_ALLOW_REPLACEMENT;
        adbusI_serv_ownerchanged(s, name, NULL, r);
        return ADBUS_SERVICE_SUCCESS;

    } else if (dv_a(&serv->queue, 0).remote == r) {
        struct ServiceOwner* o = &dv_a(&serv->queue, 0);
        o->allowReplacement = flags & ADBUS_SERVICE_ALLOW_REPLACEMENT;
        return ADBUS_SERVICE_REQUEST_ALREADY_OWNER;

    } else if (flags & ADBUS_SERVICE_REPLACE_EXISTING && dv_a(&serv->queue, 0).allowReplacement) {
        // We are replacing the owner, so we need to be removed from the queue
        adbusI_serv_releasename(s, r, serv->name);

        struct ServiceOwner* o = &dv_a(&serv->queue, 0);
        adbus_Remote* old = o->remote;
        adbusI_serv_removeServiceFromRemote(old, serv);
        o->remote = r;
        o->allowReplacement = flags & ADBUS_SERVICE_ALLOW_REPLACEMENT;
        adbusI_serv_ownerchanged(s, name, old, r);
        return ADBUS_SERVICE_SUCCESS;

    } else if (!(flags & ADBUS_SERVICE_DO_NOT_QUEUE)) {
        // If we are already in the queue then we just update the entry,
        // otherwise we add a new entry
        struct ServiceOwner* o = NULL;
        for (size_t i = 0; i < dv_size(&serv->queue); i++) {
            if (dv_a(&serv->queue, i).remote == r) {
                o = &dv_a(&serv->queue, i);
                break;
            }
        }

        if (o == NULL) {
            struct Service** pserv = dv_push(Service, &r->services, 1);
            *pserv = serv;

            o = dv_push(ServiceOwner, &serv->queue, 1);
        }

        o->remote = r;
        o->allowReplacement = flags & ADBUS_SERVICE_ALLOW_REPLACEMENT;
        return ADBUS_SERVICE_REQUEST_IN_QUEUE;

    } else {
        // If we already in the queue then we need to be removed
        adbusI_serv_releasename(s, r, serv->name);
        return ADBUS_SERVICE_REQUEST_FAILED;

    }

}

/* -------------------------------------------------------------------------- */
int adbusI_serv_releasename(
        adbus_Server*       s,
        adbus_Remote*       r,
        const char*         name)
{
    dh_Iter ii = dh_get(Service, &s->services, name);
    if (ii == dh_end(&s->services))
        return ADBUS_SERVICE_RELEASE_INVALID_NAME;

    // We have bidirectional links with the remote having a list of services
    // it is queued for (or owns) and the service having its list of remotes
    // queued.  First we remove the service from the remote list via
    // removeServiceFromRemote, then we remove the service from the queue. We
    // need to special case if the remote currently owns the service, since we
    // need to then either shift ownership to the next in the queue or remove
    // the service.

    struct Service* serv = dh_val(&s->services, ii);
    adbusI_serv_removeServiceFromRemote(r, serv);

    if (dv_a(&serv->queue, 0).remote == r) {
        dv_remove(ServiceOwner, &serv->queue, 0, 1);

        if (dv_size(&serv->queue) == 0) {
            adbusI_serv_ownerchanged(s, serv->name, r, NULL);
            adbusI_serv_freeservice(serv);
            dh_del(Service, &s->services, ii);
        } else {
            adbusI_serv_ownerchanged(s, serv->name, r, dv_a(&serv->queue, 0).remote);
        }

        return ADBUS_SERVICE_SUCCESS;
    }

    for (size_t i = 1; i < dv_size(&serv->queue); i++) {
        if (dv_a(&serv->queue, 0).remote == r) {
            dv_remove(ServiceOwner, &serv->queue, i, 1);
            return ADBUS_SERVICE_SUCCESS;
        }
    }

    return ADBUS_SERVICE_RELEASE_NOT_OWNER;
}

/* -------------------------------------------------------------------------- */
void adbusI_serv_removeServiceFromRemote(adbus_Remote* r, struct Service* serv)
{
    for (size_t j = 0; j < dv_size(&r->services); ++j) {
        if (dv_a(&r->services, j) == serv) {
            dv_remove(Service, &r->services, j, 1);
            break;
        }
    }
}


