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

#include "server-service.h"

/* -------------------------------------------------------------------------- */

void adbusI_initServiceQueue(adbusI_ServiceQueueSet* s, adbusI_BusServer* bus)
{
    s->busServer = bus;
}

/* -------------------------------------------------------------------------- */

void adbusI_freeServiceQueue(adbusI_ServiceQueueSet* s)
{
    assert(dh_size(&s->queues) == 0);
    dh_free(ServiceQueue, &s->queues);
}

/* -------------------------------------------------------------------------- */

int adbusI_requestService(
        adbusI_ServiceQueueSet*   s,
        adbus_Remote*   r,
        const char*     name,
        uint32_t        flags)
{
    int added;
    dh_Iter ii = dh_put(ServiceQueue, &s->queues, name, &added);

    if (added) {
        adbusI_ServiceQueue** pqueue = &dh_val(&s->queues, ii);
        *pqueue = NEW(adbusI_ServiceQueue);
        (*pqueue)->name = adbusI_strdup(name);
        dh_key(&s->services, ii) = (*pqueue)->name;
    }

    adbusI_ServiceQueue* queue = dh_val(&s->queues, ii);

    if (dv_size(&queue->v) == 0) {
        // Empty queue - we immediately become the owner

        adbusI_ServiceQueue** remote_queue = dv_push(ServiceQueue, &r->services, 1);
        *remote_queue = queue;

        adbusI_ServiceOwner* o = dv_push(ServiceOwner, &queue->v, 1);
        o->remote = r;
        o->allowReplacement = flags & ADBUS_SERVICE_ALLOW_REPLACEMENT;
        adbusI_serv_ownerChanged(s->busServer, name, NULL, r);
        return ADBUS_SERVICE_SUCCESS;

    } else if (dv_a(&serv->queue, 0).remote == r) {
        // We are already the owner - update flags

        adbusI_ServiceOwner* o = &dv_a(&queue->v, 0);
        o->allowReplacement = flags & ADBUS_SERVICE_ALLOW_REPLACEMENT;
        return ADBUS_SERVICE_REQUEST_ALREADY_OWNER;

    } else if (flags & ADBUS_SERVICE_REPLACE_EXISTING && dv_a(&queue->v, 0).allowReplacement) {
        // We are replacing the existing owner

        adbusI_ServiceOwner* o = &dv_a(&serv->queue, 0);
        adbus_Remote* previous = o->remote;

        // Remove ourself, if we are already in the queue
        dv_remove(ServiceOwner, &queue->v, r);

        // Remove this service from the previous owner
        dv_remove(ServiceQueue, &previous->services, s);

        // Replace the existing owner
        o->remote = r;
        o->allowReplacement = flags & ADBUS_SERVICE_ALLOW_REPLACEMENT;
        adbusI_serv_ownerChanged(s, name, previous, r);
        return ADBUS_SERVICE_SUCCESS;

    } else if (!(flags & ADBUS_SERVICE_DO_NOT_QUEUE)) {
        // We are being added to the queue, or updating an existing queue entry

        // If we are already in the queue then we just update the entry,
        // otherwise we add a new entry
        adbusI_ServiceOwner* o = NULL;
        dv_find(&queue->v, &o, dv_a(&queue->v, i).remote == r);

        if (o == NULL) {
            adbusI_ServiceQueue** pqueue = dv_push(ServiceQueue, &r->services, 1);
            *pqueue = serv;

            o = dv_push(ServiceOwner, &queue->v, 1);
        }

        o->remote = r;
        o->allowReplacement = flags & ADBUS_SERVICE_ALLOW_REPLACEMENT;
        return ADBUS_SERVICE_REQUEST_IN_QUEUE;

    } else {
        // We don't want to queue and there is already an owner

        // If we already in the queue then we need to be removed
        dv_remove(ServiceOwner, &queue->v, r);
        dv_remove(ServiceQueue, &r->services, s);
        return ADBUS_SERVICE_REQUEST_FAILED;

    }

}

/* -------------------------------------------------------------------------- */

int adbusI_releaseService(
        adbusI_ServiceQueueSet* s,
        adbus_Remote*           r,
        const char*             name)
{
    dh_Iter ii = dh_get(ServiceQueue, &s->queues, name);
    if (ii == dh_end(&s->queues))
        return ADBUS_SERVICE_RELEASE_INVALID_NAME;

    adbusI_ServiceQueue* queue = dh_val(&s->queues, ii);
    adbus_Remote* owner = dv_a(&queue->v, 0);

    // Remove the queue from the remote
    dv_remove(ServiceQueue, &r->services, queue);

    // Remove the remote from the queue
    size_t queuesz = dv_size(&queue->v);
    dv_remove(ServiceQueue, &queue->v, r);

    if (dv_size(&queue->v) == queuesz)
        return ADBUS_SERVICE_RELEASE_NOT_OWNER;

    if (owner == r) {

        if (dv_size(&queue->v) > 0) {
            // Switch to the new owner
            owner = dv_a(&queue->v, 0);
        } else {
            // No new owner, remove the queue
            dv_free(ServiceOwner, &queue->v);
            free(queue->name);
            dh_del(ServiceQueue, &s->queues, ii);
            owner = NULL;
        }

        adbusI_serv_ownerChanged(s->busServer, name, r, owner);
        return ADBUS_SERVICE_SUCCESS;

    } else {
        return ADBUS_SERVICE_SUCCESS;
    }
}

/* -------------------------------------------------------------------------- */

adbus_Remote* adbusI_lookupRemote(adbusI_ServiceQueueSet* s, const char* name)
{
    dh_Iter ii = dh_get(ServiceQueue, &s->queues, name);
    return (ii != dh_end(&s->queues)) ? dh_val(&s->queues, ii).remote : NULL;
}
