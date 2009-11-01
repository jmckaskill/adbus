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

#include "Object.h"

#include "Match.h"
#include "ObjectPath.h"

#include "memory/kvector.h"

// ----------------------------------------------------------------------------

struct Bind
{
    struct ADBusObjectPath*     path;
    struct ADBusInterface*      interface;
};

struct Match
{
    struct ADBusConnection*     connection;
    uint32_t                    id;
};

KVECTOR_INIT(Bind, struct Bind);
KVECTOR_INIT(Match, struct Match);

struct ADBusObject
{
    kvector_t(Match)*   matches;
    kvector_t(Bind)*    binds;
};

// ----------------------------------------------------------------------------

struct ADBusObject* ADBusCreateObject()
{
    struct ADBusObject* o = NEW(struct ADBusObject);
    o->matches = kv_new(Match);
    o->binds   = kv_new(Bind);

    return o;
}

// ----------------------------------------------------------------------------

void ADBusFreeObject(struct ADBusObject* o)
{
    if (o) {
        ADBusResetObject(o);
        kv_free(Match, o->matches);
        kv_free(Bind, o->binds);
        free(o);
    }
}

// ----------------------------------------------------------------------------

void ADBusResetObject(struct ADBusObject* o)
{
    for (size_t i = 0; i < kv_size(o->matches); ++i){
        struct Match* m = &kv_a(o->matches, i);
        ADBusRemoveMatch(m->connection, m->id);
    }

    for (size_t j = 0; j < kv_size(o->binds); ++j) {
        struct Bind* b = &kv_a(o->binds, j);
        ADBusUnbindInterface(b->path, b->interface);
    }

    kv_clear(Match, o->matches);
    kv_clear(Bind, o->binds);
}

// ----------------------------------------------------------------------------

int ADBusBindObject(
        struct ADBusObject*         o,
        struct ADBusObjectPath*     path,
        struct ADBusInterface*      interface,
        struct ADBusUser*           user2)
{
    int err = ADBusBindInterface(path, interface, user2);
    if (err)
        return err;

    struct Bind* b = kv_push(Bind, o->binds, 1);
    b->path = path;
    b->interface = interface;

    return 0;
}

// ----------------------------------------------------------------------------

int ADBusUnbindObject(
        struct ADBusObject*         o,
        struct ADBusObjectPath*     path,
        struct ADBusInterface*      interface)
{
    size_t i;
    for (i = 0; i < kv_size(o->binds); ++i) {
        struct Bind* b = &kv_a(o->binds, i);
        if (b->path == path && b->interface == interface) {
            break;
        }
    }

    assert(i != kv_size(o->binds));

    kv_remove(Bind, o->binds, i, 1);
    return ADBusUnbindInterface(path, interface);
}

// ----------------------------------------------------------------------------

uint32_t ADBusAddObjectMatch(
        struct ADBusObject*         o,
        struct ADBusConnection*     connection,
        const struct ADBusMatch*    match)
{
    uint32_t id = ADBusAddMatch(connection, match);

    struct Match* m = kv_push(Match, o->matches, 1);
    m->connection = connection;
    m->id = id;

    return id;
}

// ----------------------------------------------------------------------------

void ADBusAddObjectMatchId(
        struct ADBusObject*         o,
        struct ADBusConnection*     connection,
        uint32_t                    id)
{
    struct Match* m = kv_push(Match, o->matches, 1);
    m->connection = connection;
    m->id = id;
}

// ----------------------------------------------------------------------------

void ADBusRemoveObjectMatch(
        struct ADBusObject*         o,
        struct ADBusConnection*     connection,
        uint32_t                    id)
{
    size_t i;
    for (i = 0; i < kv_size(o->matches); ++i) {
        struct Match* m = &kv_a(o->matches, i);
        if (m->id == id && m->connection == connection) {
            break;
        }
    }

    assert(i != kv_size(o->matches));

    kv_remove(Match, o->matches, i, 1);
    ADBusRemoveMatch(connection, id);
}



