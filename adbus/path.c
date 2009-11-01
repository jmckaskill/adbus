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

#include "connection.h"
#include "interface.h"
#include "misc.h"

// ----------------------------------------------------------------------------

void adbusI_freeObjectPath(struct ObjectPath* o)
{
    if (o) {
        khash_t(Bind)* h = o->interfaces;
        for (khiter_t ii = kh_begin(h); ii != kh_end(h); ++ii) {
            if (kh_exist(h, ii)) {
                adbus_user_free(kh_val(h, ii).user2);
            }
        }
        kh_free(Bind, h);
        kv_free(ObjectPtr, o->children);
        free((char*) o->h.string);
        free(o);
    }
}


// ----------------------------------------------------------------------------

static struct ObjectPath* DoAddObject(
        adbus_Connection* c,
        char*                   path,
        size_t                  size)
{
    int added;
    assert(path[size] == '\0');
    khiter_t ki = kh_put(ObjectPtr, c->objects, path, &added);
    if (!added) {
        return kh_val(c->objects, ki);
    }

    struct ObjectPath* o = NEW(struct ObjectPath);
    o->interfaces   = kh_new(Bind);
    o->children     = kv_new(ObjectPtr);
    o->h.connection = c;
    o->h.string     = adbusI_strndup(path, size);
    o->h.size       = size;

    kh_key(c->objects, ki) = o->h.string;
    kh_val(c->objects, ki) = o;

    adbus_path_bind(&o->h, c->introspectable, adbusI_puser_new(o));
    adbus_path_bind(&o->h, c->properties, adbusI_puser_new(o));

    // Setup the parent-child links
    if (strcmp(path, "/") != 0) {
        size_t parentSize;
        adbusI_parentPath(path, size, &parentSize);
        o->parent = DoAddObject(c, path, parentSize);

        struct ObjectPath** pchild = kv_push(ObjectPtr, o->parent->children, 1);
        *pchild = o;
    }

    return o;
}

// ----------------------------------------------------------------------------

adbus_Path* adbus_conn_path(
        adbus_Connection* c,
        const char*             path,
        int                     size)
{
    kstring_t* name = ks_new();
    adbusI_relativePath(name, path, size, NULL, 0);

    struct ObjectPath* o = DoAddObject(
            c,
            ks_data(name),
            ks_size(name));

    ks_free(name);
    return (adbus_Path*) o;
}

// ----------------------------------------------------------------------------

adbus_Path* adbus_path_relative(
        adbus_Path* path,
        const char*             relpath,
        int                     size)
{
    kstring_t* name = ks_new();
    adbusI_relativePath(name, path->string, path->size, relpath, size);

    struct ObjectPath* o = DoAddObject(
            path->connection,
            ks_data(name),
            ks_size(name));

    ks_free(name);
    return (adbus_Path*) o;
}

// ----------------------------------------------------------------------------

int adbus_path_bind(
        adbus_Path* path,
        adbus_Interface*  interface,
        adbus_User*       user2)
{
    struct ObjectPath* o = (struct ObjectPath*) path;

    int added;
    khiter_t bi = kh_put(Bind, o->interfaces, interface->name, &added);
    if (!added)
        return -1; // there was already an interface with that name

    kh_val(o->interfaces, bi).user2      = user2;
    kh_val(o->interfaces, bi).interface  = interface;
    kh_key(o->interfaces, bi)            = interface->name;

    return 0;
}

// ----------------------------------------------------------------------------

static void CheckRemoveObject(struct ObjectPath* p)
{
    // We have 2 builtin interfaces (introspectable and properties)
    // If these are the only two left and we have no children then we need
    // to prune this object
    if (kh_size(p->interfaces) > 2 || kv_size(p->children) > 0)
        return;

    // Remove it from its parent
    struct ObjectPath* parent = p->parent;
    for (size_t i = 0; i < kv_size(parent->children); ++i) {
        if (kv_a(parent->children, i) == p) {
            kv_remove(ObjectPtr, parent->children, i, 1);
            break;
        }
    }
    CheckRemoveObject(p->parent);

    // Remove it from the object lookup table
    adbus_Connection* c = p->h.connection;
    khiter_t oi = kh_get(ObjectPtr, c->objects, p->h.string);
    if (oi != kh_end(c->objects))
        kh_del(ObjectPtr, c->objects, oi);

    // Free the object
    adbusI_freeObjectPath(p);
}

// ----------------------------------------------------------------------------

int adbus_path_unbind(
        adbus_Path* path,
        adbus_Interface*  interface)
{
    struct ObjectPath* o = (struct ObjectPath*) path;

    khiter_t bi = kh_get(Bind, o->interfaces, interface->name);
    if (bi == kh_end(o->interfaces))
        return -1;
    if (interface != kh_val(o->interfaces, bi).interface)
        return -1;

    adbus_user_free(kh_val(o->interfaces, bi).user2);
    kh_del(Bind, o->interfaces, bi);

    CheckRemoveObject(o);

    return 0;
}

// ----------------------------------------------------------------------------

adbus_Interface* adbus_path_interface(
        adbus_Path*     path,
        const char*                 interface,
        int                         interfaceSize,
        const adbus_User**    user2)
{
    struct ObjectPath* o = (struct ObjectPath*) path;

    // The hash table lookup requires that the interface name is nul terminated
    // So if we cannot guarentee this (ie interfaceSize >= 0) then copy the
    // string out and free it after the lookup
    if (interfaceSize >= 0)
        interface = adbusI_strndup(interface, interfaceSize);

    khiter_t bi = kh_get(Bind, o->interfaces, interface);

    if (interfaceSize >= 0)
        free((char*) interface);

    if (bi == kh_end(o->interfaces))
        return NULL;

    if (user2)
        *user2 = kh_val(o->interfaces, bi).user2;

    return kh_val(o->interfaces, bi).interface;
}

// ----------------------------------------------------------------------------

adbus_Member* adbus_path_method(
        adbus_Path*     path,
        const char*                 method,
        int                         size,
        const adbus_User**    user2)
{
    struct ObjectPath* o = (struct ObjectPath*) path;
    adbus_Member* m;

    for (khiter_t bi = kh_begin(o->interfaces); bi != kh_end(o->interfaces); ++bi) {
        if (kh_exist(o->interfaces, bi)) {
            struct Bind* b = &kh_val(o->interfaces, bi);
            m = adbus_iface_method(b->interface, method, size);
            if (user2)
                *user2 = b->user2;
            if (m)
                return m;
        }
    }

    if (user2)
        *user2 = NULL;
    return NULL;
}






