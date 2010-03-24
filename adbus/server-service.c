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

#include "server-service.h"
#include "server-remote.h"
#include "server-bus.h"

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
    adbusI_ServiceQueue* queue;
    adbusI_ServiceOwner* owner = NULL;
    adbus_Remote* previous;

    int added;
    dh_Iter ii = dh_put(ServiceQueue, &s->queues, name, &added);

    if (added) {
        queue = NEW(adbusI_ServiceQueue);
        queue->name = adbusI_strdup(name);
        dh_key(&s->queues, ii) = queue->name;
        dh_val(&s->queues, ii) = queue;
    } else {
        queue = dh_val(&s->queues, ii);
    }


    if (dv_size(&queue->v) == 0) {
        /* Empty queue - we immediately become the owner */

        adbusI_ServiceQueue** remote_queue = dv_push(ServiceQueue, &r->services, 1);
        *remote_queue = queue;

        owner = dv_push(ServiceOwner, &queue->v, 1);
        owner->remote = r;
        owner->allowReplacement = flags & ADBUS_SERVICE_ALLOW_REPLACEMENT;
        adbusI_serv_ownerChanged(s->busServer, name, NULL, r);
        return ADBUS_SERVICE_SUCCESS;

    } else if (dv_a(&queue->v, 0).remote == r) {
        /* We are already the owner - update flags */

        owner = &dv_a(&queue->v, 0);
        owner->allowReplacement = flags & ADBUS_SERVICE_ALLOW_REPLACEMENT;
        return ADBUS_SERVICE_REQUEST_ALREADY_OWNER;

    } else if (flags & ADBUS_SERVICE_REPLACE_EXISTING && dv_a(&queue->v, 0).allowReplacement) {
        /* We are replacing the existing owner */

        owner = &dv_a(&queue->v, 0);
        previous = owner->remote;

        /* Remove ourself, if we are already in the queue */
        dv_remove(ServiceOwner, &queue->v, ENTRY->remote == r);

        /* Remove this service from the previous owner */
        dv_remove(ServiceQueue, &previous->services, *ENTRY == queue);

        /* Replace the existing owner */
        owner->remote = r;
        owner->allowReplacement = flags & ADBUS_SERVICE_ALLOW_REPLACEMENT;
        adbusI_serv_ownerChanged(s->busServer, name, previous, r);
        return ADBUS_SERVICE_SUCCESS;

    } else if (!(flags & ADBUS_SERVICE_DO_NOT_QUEUE)) {
        /* We are being added to the queue, or updating an existing queue entry */

        /* If we are already in the queue then we just update the entry,
         * otherwise we add a new entry 
         */
        dv_find(ServiceOwner, &queue->v, &owner, ENTRY->remote == r);

        if (owner == NULL) {
            adbusI_ServiceQueue** pqueue = dv_push(ServiceQueue, &r->services, 1);
            *pqueue = queue;

            owner = dv_push(ServiceOwner, &queue->v, 1);
        }

        owner->remote = r;
        owner->allowReplacement = flags & ADBUS_SERVICE_ALLOW_REPLACEMENT;
        return ADBUS_SERVICE_REQUEST_IN_QUEUE;

    } else {
        /* We don't want to queue and there is already an owner */

        /* If we already in the queue then we need to be removed */
        dv_remove(ServiceOwner, &queue->v, ENTRY->remote == r);
        dv_remove(ServiceQueue, &r->services, *ENTRY == queue);
        return ADBUS_SERVICE_REQUEST_FAILED;

    }

}

/* -------------------------------------------------------------------------- */

int adbusI_releaseService(
        adbusI_ServiceQueueSet* s,
        adbus_Remote*           r,
        const char*             name)
{
    adbusI_ServiceOwner* owner;
    adbusI_ServiceQueue* queue;

    dh_Iter ii = dh_get(ServiceQueue, &s->queues, name);
    if (ii == dh_end(&s->queues))
        return ADBUS_SERVICE_RELEASE_INVALID_NAME;

    queue = dh_val(&s->queues, ii);
    owner = &dv_a(&queue->v, 0);

    /* Remove the queue from the remote */
    dv_remove(ServiceQueue, &r->services, *ENTRY == queue);

    /* Remove the remote from the queue */
    {
        size_t queuesz = dv_size(&queue->v);
        dv_remove(ServiceOwner, &queue->v, ENTRY->remote == r);

        if (dv_size(&queue->v) == queuesz)
            return ADBUS_SERVICE_RELEASE_NOT_OWNER;
    }

    if (owner->remote == r) {

        if (dv_size(&queue->v) > 0) {
            /* Switch to the new owner */
            adbusI_serv_ownerChanged(s->busServer, name, r, dv_a(&queue->v, 0).remote);
        } else {
            /* No new owner, remove the queue */
            dh_del(ServiceQueue, &s->queues, ii);
            adbusI_serv_ownerChanged(s->busServer, name, r, NULL);
            dv_free(ServiceOwner, &queue->v);
            free(queue->name);
            free(queue);
        }

        return ADBUS_SERVICE_SUCCESS;

    } else {
        return ADBUS_SERVICE_SUCCESS;
    }
}

/* -------------------------------------------------------------------------- */

adbus_Remote* adbusI_lookupRemote(adbusI_ServiceQueueSet* s, const char* name)
{
    adbusI_ServiceQueue* queue;
    dh_Iter ii = dh_get(ServiceQueue, &s->queues, name);
    if (ii == dh_end(&s->queues))
        return NULL;

    queue = dh_val(&s->queues, ii);
    return dv_a(&queue->v, 0).remote;
}

