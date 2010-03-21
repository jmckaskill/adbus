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
#include <stdio.h>

/* -------------------------------------------------------------------------- */
static adbus_Bool StringMatches(
        const char* match,
        size_t      matchsz,
        const char* msg,
        size_t      msgsz)
{
    if (!match)
        return 1;

    if (!msg)
        return 0;

    if (matchsz != msgsz)
        return 0;

    return memcmp(match, msg, msgsz) == 0;
}


static adbus_Bool ArgsMatch(
        struct Match*   match,
        adbus_Message*  msg)
{
    if (msg->argumentsSize < match->argumentsSize)
        return 0;

    for (size_t i = 0; i < match->argumentsSize; i++) {
        adbus_Argument* matcharg = &match->arguments[i];
        adbus_Argument* msgarg = &msg->arguments[i];

        if (!matcharg->value)
            continue;

        if (msgarg->value == NULL)
            return 0;

        if (msgarg->size != matcharg->size)
            return 0;

        if (memcmp(msgarg->value, matcharg->value, matcharg->size) != 0)
            return 0;

    }
    return 1;
}

static int RemoteDispatchMatch(adbus_Remote* r, adbus_Message* msg)
{
    struct Match* match;
    DL_FOREACH(Match, match, &r->matches, hl) {
        if (match->type == ADBUS_MSG_INVALID && match->type != msg->type) {
            continue;
        } else if (match->checkReply && (!msg->replySerial || match->reply != *msg->replySerial)) {
            continue;
        } else if (!StringMatches(match->path, match->pathSize, msg->path, msg->pathSize)) {
            continue;
        } else if (!StringMatches(match->interface, match->interfaceSize, msg->interface, msg->interfaceSize)) {
            continue;
        } else if (!StringMatches(match->member, match->memberSize, msg->member, msg->memberSize)) {
            continue;
        } else if (!StringMatches(match->error, match->errorSize, msg->error, msg->errorSize)) {
            continue;
        } else if (!StringMatches(match->destination, match->destinationSize, msg->destination, msg->destinationSize)) {
            continue;
        } else if (!StringMatches(match->sender, match->senderSize, msg->sender, msg->senderSize)) {
            continue;
        } else if (match->argumentsSize > 0) {
            if (adbus_parseargs(msg))
                return -1;
            if (!ArgsMatch(match, msg))
                continue;
        }

        return r->send(r->data, msg) != (adbus_ssize_t) msg->size;
    }
    return 0;
}

/* -------------------------------------------------------------------------- */
int adbusI_serv_dispatch(adbus_Server* s, adbus_Message* m)
{
    adbus_Remote* direct = NULL;
    if (m->destination) {
        dh_Iter ii = dh_get(Service, &s->services, m->destination);
        if (ii != dh_end(&s->services)) {
            struct Service* serv = dh_val(&s->services, ii);
            if (dv_size(&serv->queue) > 0) {
                direct = dv_a(&serv->queue, 0).remote;
            }
        }
    } else if (m->type == ADBUS_MSG_METHOD) {
        direct = s->busRemote;
    }

    adbus_Remote* r;
    DL_FOREACH(Remote, r, &s->remotes, hl) {
        if (r != direct && RemoteDispatchMatch(r, m)) {
            return -1;
        }
    }

    if (direct && direct->send(direct->data, m) != (adbus_ssize_t) m->size)
        return -1;

    return 0;
}


