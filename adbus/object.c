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

#include <adbus/adbus.h>

#include "memory/kvector.h"

// ----------------------------------------------------------------------------

struct Bind
{
    adbus_Path*     path;
    adbus_Interface*      interface;
};

struct Match
{
    adbus_Connection*     connection;
    uint32_t                    id;
};

KVECTOR_INIT(Bind, struct Bind);
KVECTOR_INIT(Match, struct Match);

struct adbus_Object
{
    kvector_t(Match)*   matches;
    kvector_t(Bind)*    binds;
};

// ----------------------------------------------------------------------------

adbus_Object* adbus_obj_new(void)
{
    adbus_Object* o = NEW(adbus_Object);
    o->matches = kv_new(Match);
    o->binds   = kv_new(Bind);

    return o;
}

// ----------------------------------------------------------------------------

void adbus_obj_free(adbus_Object* o)
{
    if (o) {
        adbus_obj_reset(o);
        kv_free(Match, o->matches);
        kv_free(Bind, o->binds);
        free(o);
    }
}

// ----------------------------------------------------------------------------

void adbus_obj_reset(adbus_Object* o)
{
    for (size_t i = 0; i < kv_size(o->matches); ++i){
        struct Match* m = &kv_a(o->matches, i);
        adbus_conn_removematch(m->connection, m->id);
    }

    for (size_t j = 0; j < kv_size(o->binds); ++j) {
        struct Bind* b = &kv_a(o->binds, j);
        adbus_path_unbind(b->path, b->interface);
    }

    kv_clear(Match, o->matches);
    kv_clear(Bind, o->binds);
}

// ----------------------------------------------------------------------------

int adbus_obj_bind(
        adbus_Object*         o,
        adbus_Path*     path,
        adbus_Interface*      interface,
        adbus_User*           user2)
{
    int err = adbus_path_bind(path, interface, user2);
    if (err)
        return err;

    struct Bind* b = kv_push(Bind, o->binds, 1);
    b->path = path;
    b->interface = interface;

    return 0;
}

// ----------------------------------------------------------------------------

int adbus_obj_unbind(
        adbus_Object*         o,
        adbus_Path*     path,
        adbus_Interface*      interface)
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
    return adbus_path_unbind(path, interface);
}

// ----------------------------------------------------------------------------

uint32_t adbus_obj_addmatch(
        adbus_Object*         o,
        adbus_Connection*     connection,
        const adbus_Match*    match)
{
    uint32_t id = adbus_conn_addmatch(connection, match);

    struct Match* m = kv_push(Match, o->matches, 1);
    m->connection = connection;
    m->id = id;

    return id;
}

// ----------------------------------------------------------------------------

void adbus_obj_addmatchid(
        adbus_Object*         o,
        adbus_Connection*     connection,
        uint32_t                    id)
{
    struct Match* m = kv_push(Match, o->matches, 1);
    m->connection = connection;
    m->id = id;
}

// ----------------------------------------------------------------------------

void adbus_obj_removematch(
        adbus_Object*         o,
        adbus_Connection*     connection,
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
    adbus_conn_removematch(connection, id);
}



