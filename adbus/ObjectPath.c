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

#include "ObjectPath.h"
#include "Connection_p.h"
#include "Interface_p.h"
#include "Misc_p.h"

// ----------------------------------------------------------------------------

void FreeObjectPath(struct ObjectPath* o)
{
    if (o) {
        khash_t(Bind)* h = o->interfaces;
        for (khiter_t ii = kh_begin(h); ii != kh_end(h); ++ii) {
            if (kh_exist(h, ii)) {
                ADBusUserFree(kh_val(h, ii).user2);
            }
        }
        kh_free(Bind, h);
        kv_free(ObjectPtr, o->children);
        free((char*) o->h.path);
        free(o);
    }
}


// ----------------------------------------------------------------------------

static struct ObjectPath* DoAddObject(
        struct ADBusConnection* c,
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
    o->h.path       = strndup_(path, size);
    o->h.pathSize   = size;

    kh_key(c->objects, ki) = o->h.path;
    kh_val(c->objects, ki) = o;

    ADBusBindInterface(&o->h, c->introspectable, CreateUserPointer(o));
    ADBusBindInterface(&o->h, c->properties, CreateUserPointer(o));

    // Setup the parent-child links
    if (strcmp(path, "/") != 0) {
        size_t parentSize;
        ParentPath(path, size, &parentSize);
        o->parent = DoAddObject(c, path, parentSize);

        struct ObjectPath** pchild = kv_push(ObjectPtr, o->parent->children, 1);
        *pchild = o;
    }

    return o;
}

// ----------------------------------------------------------------------------

struct ADBusObjectPath* ADBusGetObjectPath(
        struct ADBusConnection* c,
        const char*             path,
        int                     size)
{
    kstring_t* name = ks_new();
    SanitisePath(name, path, size, NULL, 0);

    struct ObjectPath* o = DoAddObject(
            c,
            ks_data(name),
            ks_size(name));

    ks_free(name);
    return (struct ADBusObjectPath*) o;
}

// ----------------------------------------------------------------------------

struct ADBusObjectPath* ADBusRelativePath(
        struct ADBusObjectPath* path,
        const char*             relpath,
        int                     size)
{
    kstring_t* name = ks_new();
    SanitisePath(name, path->path, path->pathSize, relpath, size);

    struct ObjectPath* o = DoAddObject(
            path->connection,
            ks_data(name),
            ks_size(name));

    ks_free(name);
    return (struct ADBusObjectPath*) o;
}

// ----------------------------------------------------------------------------

int ADBusBindInterface(
        struct ADBusObjectPath* path,
        struct ADBusInterface*  interface,
        struct ADBusUser*       user2)
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
    struct ADBusConnection* c = p->h.connection;
    khiter_t oi = kh_get(ObjectPtr, c->objects, p->h.path);
    if (oi != kh_end(c->objects))
        kh_del(ObjectPtr, c->objects, oi);

    // Free the object
    FreeObjectPath(p);
}

// ----------------------------------------------------------------------------

int ADBusUnbindInterface(
        struct ADBusObjectPath* path,
        struct ADBusInterface*  interface)
{
    struct ObjectPath* o = (struct ObjectPath*) path;

    khiter_t bi = kh_get(Bind, o->interfaces, interface->name);
    if (bi == kh_end(o->interfaces))
        return -1;
    if (interface != kh_val(o->interfaces, bi).interface)
        return -1;

    ADBusUserFree(kh_val(o->interfaces, bi).user2);
    kh_del(Bind, o->interfaces, bi);

    CheckRemoveObject(o);

    return 0;
}

// ----------------------------------------------------------------------------

struct ADBusInterface* ADBusGetBoundInterface(
        struct ADBusObjectPath*     path,
        const char*                 interface,
        int                         interfaceSize,
        const struct ADBusUser**    user2)
{
    struct ObjectPath* o = (struct ObjectPath*) path;

    // The hash table lookup requires that the interface name is nul terminated
    // So if we cannot guarentee this (ie interfaceSize >= 0) then copy the
    // string out and free it after the lookup
    if (interfaceSize >= 0)
        interface = strndup_(interface, interfaceSize);

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

struct ADBusMember* ADBusGetBoundMember(
        struct ADBusObjectPath*     path,
        enum ADBusMemberType        type,
        const char*                 member,
        int                         memberSize,
        const struct ADBusUser**    user2)
{
    struct ObjectPath* o = (struct ObjectPath*) path;
    struct ADBusMember* m;

    for (khiter_t bi = kh_begin(o->interfaces); bi != kh_end(o->interfaces); ++bi) {
        if (kh_exist(o->interfaces, bi)) {
            struct Bind* b = &kh_val(o->interfaces, bi);
            m = ADBusGetInterfaceMember(b->interface, type, member, memberSize);
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






