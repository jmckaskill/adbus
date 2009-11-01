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

#include "Match.h"

#include "Connection_p.h"
#include "Iterator.h"
#include "Misc_p.h"
#include "Proxy.h"

#include <string.h>

// ----------------------------------------------------------------------------

void ADBusInitMatchArguments(struct ADBusMatchArgument* args, size_t num)
{
    memset(args, 0, sizeof(struct ADBusMatchArgument) * num);
    for (size_t i = 0; i < num; ++i) {
        args->valueSize = -1;
    }
}

// ----------------------------------------------------------------------------

void ADBusInitMatch(struct ADBusMatch* pmatch)
{
    memset(pmatch, 0, sizeof(struct ADBusMatch));
    pmatch->replySerial     = -1;
    pmatch->senderSize      = -1;
    pmatch->destinationSize = -1;
    pmatch->interfaceSize   = -1;
    pmatch->pathSize        = -1;
    pmatch->memberSize      = -1;
    pmatch->errorNameSize   = -1;
}

// ----------------------------------------------------------------------------

void FreeMatchData(struct Match* m)
{
    if (m) {
        free((char*) m->m.sender);
        free((char*) m->m.destination);
        free((char*) m->m.interface);
        free((char*) m->m.member);
        free((char*) m->m.errorName);
        free((char*) m->m.path);
        ADBusUserFree(m->m.user1);
        ADBusUserFree(m->m.user2);
        for (size_t i = 0; i < m->m.argumentsSize; ++i) {
            free((char*) m->m.arguments[i].value);
        }
        free(m->m.arguments);
    }
}

void FreeService(struct Service* s)
{
    if (s) {
        free(s->serviceName);
        free(s->uniqueName);
        free(s);
    }
}

// ----------------------------------------------------------------------------

static void CloneString(const char* from, int fsize, const char** to, int* tsize)
{
    if (from) {
        if (fsize < 0)
            fsize = strlen(from);
        *to = strndup_(from, fsize);
        *tsize = fsize;
    }
}

static void CloneMatch(const struct ADBusMatch* from,
                       struct Match* to)
{
    memset(to, 0, sizeof(struct Match));
    to->m.type                  = from->type;
    to->m.addMatchToBusDaemon   = from->addMatchToBusDaemon;
    to->m.removeOnFirstMatch    = from->removeOnFirstMatch;
    to->m.replySerial           = from->replySerial;
    to->m.callback              = from->callback;
    to->m.user1                 = from->user1;
    to->m.user2                 = from->user2;
    to->m.id                    = from->id;

#define STRING(NAME) CloneString(from->NAME, from->NAME ## Size, &to->m.NAME, &to->m.NAME ## Size)
    STRING(sender);
    STRING(destination);
    STRING(interface);
    STRING(member);
    STRING(errorName);
    if (from->path) {
        kstring_t* sanitised = ks_new();
        SanitisePath(sanitised, from->path, from->pathSize, NULL, 0);
        to->m.pathSize = ks_size(sanitised);
        to->m.path     = ks_release(sanitised);
        ks_free(sanitised);
    }
    to->m.arguments = NEW_ARRAY(struct ADBusMatchArgument, from->argumentsSize);
    to->m.argumentsSize = from->argumentsSize;
    for (size_t i = 0; i < from->argumentsSize; ++i) {
        to->m.arguments[i].number = from->arguments[i].number;
        STRING(arguments[i].value);
    }
#undef STRING
}

// ----------------------------------------------------------------------------

static int GetNameOwner(struct ADBusCallDetails* d)
{
    struct Service* s = (struct Service*) GetUserPointer(d->user1);

    s->methodMatch = 0;

    const char* unique = ADBusCheckString(d, NULL);
    ADBusCheckMessageEnd(d);

    free(s->uniqueName);
    s->uniqueName = strdup_(unique);

    return 0;
}

static int NameOwnerChanged(struct ADBusCallDetails* d)
{
    struct Service* s = (struct Service*) GetUserPointer(d->user1);

    ADBusCheckString(d, NULL);
    ADBusCheckString(d, NULL);
    const char* to = ADBusCheckString(d, NULL);
    ADBusCheckMessageEnd(d);

    free(s->uniqueName);
    s->uniqueName = strdup_(to);

    return 0;
}

// ----------------------------------------------------------------------------

static struct Service* RefService(struct ADBusConnection* c, const char* servname)
{
    struct Service* s = NULL;

    int added = 0;
    khiter_t si = kh_put(Service, c->services, servname, &added);
    if (!added) {
        s = kh_val(c->services, si);
        s->refCount++;
    } else {
        s = NEW(struct Service);
        s->refCount = 1;
        s->serviceName = strdup_(servname);

        kh_key(c->services, si) = s->serviceName;
        kh_val(c->services, si) = s;
    }
    return s;
}

// ----------------------------------------------------------------------------

static void UnrefService(struct ADBusConnection* c, struct Service* s)
{
    s->refCount--;
    if (s->refCount > 0)
        return;

    khiter_t si = kh_get(Service, c->services, s->serviceName);
    if (si != kh_end(c->services))
        kh_del(Service, c->services, si);

    if (s->methodMatch)
        ADBusRemoveMatch(c, s->methodMatch);
    if (s->signalMatch)
        ADBusRemoveMatch(c, s->signalMatch);
    FreeService(s);
}

// ----------------------------------------------------------------------------

static void AddServiceCallbacks(
        struct ADBusConnection* c,
        struct Service*         s)
{
    // Add the NameOwnerChanged match
    struct ADBusMatchArgument arg0;
    ADBusInitMatchArguments(&arg0, 1);
    arg0.number = 0;
    arg0.value = s->serviceName;

    struct ADBusMatch match;
    ADBusInitMatch(&match);
    match.type = ADBusSignalMessage;
    match.addMatchToBusDaemon = 1;
    match.sender = "org.freedesktop.DBus";
    match.path = "/org/freedesktop/DBus";
    match.interface = "org.freedesktop.DBus";
    match.member = "NameOwnerChanged";
    match.arguments = &arg0;
    match.argumentsSize = 1;
    match.callback = &NameOwnerChanged;
    match.user1 = CreateUserPointer(s);

    s->signalMatch = ADBusAddMatch(c, &match);

    // Call GetNameOwner - do this after adding the NameOwnerChanged match to
    // avoid a race condition. Setup the match for the reply first
    struct ADBusFactory f;
    ADBusProxyFactory(c->bus, &f);
    f.member    = "GetNameOwner";
    f.callback  = &GetNameOwner;
    f.user1     = CreateUserPointer(s);

    ADBusAppendArguments(f.args, "s", -1);
    ADBusAppendString(f.args, s->serviceName, -1);

    s->methodMatch = ADBusCallFactory(&f);
}


// ----------------------------------------------------------------------------

uint32_t ADBusAddMatch(
        struct ADBusConnection*     c,
        const struct ADBusMatch*    reg)
{
    struct Match* match = kv_push(Match, c->registrations, 1);
    CloneMatch(reg, match);
    if (match->m.id == 0) {
        match->m.id = ADBusNextMatchId(c);
    }

    if (match->m.addMatchToBusDaemon) {
        struct ADBusFactory f;
        ADBusProxyFactory(c->bus, &f);
        f.member = "AddMatch";
        AppendMatchString(f.args, &match->m);

        ADBusCallFactory(&f);
    }

    uint32_t id = match->m.id;

    // If the sender field is a service name we have to track the service name
    // assignments
    if (match->m.sender
     && RequiresServiceLookup(match->m.sender, match->m.senderSize))
    {
        struct Service* s = RefService(c, match->m.sender);
        match->service = s;
        // WARNING: AddServiceCallbacks will call ADBusAddMatch which may cause
        // match to be a dangling pointer
        if (s->signalMatch == 0) {
            AddServiceCallbacks(c, s);
        }
    }

    return id;
}

// ----------------------------------------------------------------------------

void ADBusRemoveMatch(struct ADBusConnection* c, uint32_t id)
{
    for (size_t i = 0; i < kv_size(c->registrations); ++i) {
        if (kv_a(c->registrations, i).m.id == id) {
            struct Match* match = &kv_a(c->registrations, i);
            struct Service* s = match->service;

            if (match->m.addMatchToBusDaemon) {
                struct ADBusFactory f;
                ADBusProxyFactory(c->bus, &f);
                f.member = "RemoveMatch";
                AppendMatchString(f.args, &match->m);

                ADBusCallFactory(&f);
            }

            FreeMatchData(match);
            kv_remove(Match, c->registrations, i, 1);

            // Remove the service after removing the match - as removing the
            // service will remove more matches
            if (s) {
                UnrefService(c, s);
            }

            return;
        }
    }

}

// ----------------------------------------------------------------------------

uint32_t ADBusNextMatchId(struct ADBusConnection* c)
{
    if (c->nextMatchId == UINT32_MAX)
        c->nextSerial = 1;
    return c->nextMatchId++;
}



