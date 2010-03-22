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
#include "connection.h"
#include "interface.h"

/** \struct adbus_Bind
 *  \brief Data structure used to bind interfaces to a particular path.
 *
 *  \note An adbus_Bind structure should always be initialised using
 *  adbus_bind_init().
 *
 *  \note The proxy and relproxy fields should almost always be initialised
 *  using adbus_conn_getproxy().
 *
 *  Client code registers for paths by:
 *
 *  -#  Creating an interface (see adbus_Interface). This registers all of the
 *  methods, properties, and signals on that interface and all of the
 *  callbacks for those. This is equivalent to defining an abstract base class
 *  or interface.
 *  -# The interface is then bound to a path using adbus_conn_bind(). The bind
 *  details what path to bind to and also binds in a user pointer so that the
 *  method callbacks can distinguish which object is being called.
 *
 *  For example:
 *  \code
 *  int ExampleMethod(adbus_CbData* d)
 *  {
 *      Object* o = (Object*) d->user2;
 *      o->ExampleMethod();
 *      return 0;
 *  }
 *
 *  adbus_Interface* CreateInterface()
 *  {
 *      adbus_Interface* i = adbus_iface_new("com.example.Interface", -1);
 *      adbus_Member* m = adbus_iface_addmethod(i, "Example", -1);
 *      adbus_mbr_setmethod(m, &ExampleMethod, NULL);
 *      return i;
 *  }
 *
 *  void Bind(adbus_Connection* c, Object* o, const char* path)
 *  {
 *      adbus_Interface* interface = CreateInterface();
 *
 *      adbus_Bind b;
 *      adbus_bind_init(&b);
 *      b.path      = path;
 *      b.interface = interface;
 *      b.cuser2    = o;
 *
 *      adbus_state_bind(o->state, c, &b);
 *
 *      adbus_iface_free(interface);
 *  }
 *  \endcode
 *
 *  \note If writing C code directly, it's \b strongly suggested to use
 *  adbus_state_bind() since it vastly simplifies the management of unbinding
 *  and performs all of the thread jumping required in multithreaded
 *  applications.
 *
 *  \sa adbus_Connection, adbus_conn_bind(), adbus_state_bind(),
 *  adbus_Interface, adbus_CbData
 */

/** \var adbus_Bind::path
 *  Path to bind the interface to.
 */

/** \var adbus_Bind::pathSize
 *  Length of adbus_Bind::path or -1 if null terminated (default).
 */

/** \var adbus_Bind::interface
 *  Interface to bind.
 *
 *  The interface will be internally ref'd via adbus_iface_ref() after the call to
 *  adbus_conn_bind() or adbus_state_bind() so the calling code can "free" the
 *  interface if it wants to.
 */

/** \var adbus_Bind::cuser2
 *  User pointer available as adbus_CbData::user2 in all the method and
 *  property callbacks.
 *
 *  This user data can be freed on unbind by setting adbus_Bind::release and
 *  adbus_Bind::ruser.
 */

/** \var adbus_Bind::proxy
 *  Callback to a proxy function called to proxy method and property sets/gets
 *  to the correct thread.  
 *
 *  If this value is non-zero the proxy will be used to proxy all calls for
 *  this bind. In general it should be set be defaulted using
 *  adbus_conn_getproxy().
 */

/** \var adbus_Bind::puser
 *  User data for adbus_Bind::proxy.
 */

/** \var adbus_Bind::release
 *  Release callbacks called when the interface is unbound from this path.
 */

/** \var adbus_Bind::ruser
 *  User data for adbus_Bind::release
 */

/** \var adbus_Bind::relproxy
 *  Proxy function for the release callbacks.
 *
 *  In general this should be defaulted using adbus_conn_getproxy().
 */



// ----------------------------------------------------------------------------

/** Initialises an adbus_Bind structure.
 *  \relates adbus_Bind
 */
void adbus_bind_init(adbus_Bind* b)
{
    memset(b, 0, sizeof(adbus_Bind));
    b->pathSize = -1;
}

// ----------------------------------------------------------------------------

static adbus_ConnBind* DoBind(struct ObjectPath* path, const adbus_Bind* bind)
{
    int added;
    dh_Iter bi = dh_put(Bind, &path->interfaces, bind->interface->name, &added);
    if (!added)
        return NULL;

    adbus_iface_ref(bind->interface);

    adbus_ConnBind* b   = NEW(adbus_ConnBind);
    b->path             = path;
    b->interface        = bind->interface;
    b->cuser2           = bind->cuser2;
    b->proxy            = bind->proxy;
    b->puser            = bind->puser;
    b->release[0]       = bind->release[0];
    b->release[1]       = bind->release[1];
    b->ruser[0]         = bind->ruser[0];
    b->ruser[1]         = bind->ruser[1];
    b->relproxy         = bind->relproxy;
    b->relpuser         = bind->relpuser;

    dh_val(&path->interfaces, bi) = b;
    dh_key(&path->interfaces, bi) = b->interface->name;

    dil_insert_after(Bind, &path->connection->binds, b, &b->fl);

    return b;
}

// ----------------------------------------------------------------------------

void adbusI_freeObjectPath(struct ObjectPath* o)
{
    // Disconnect from connection
    if (o->connection) {
        d_Hash(ObjectPath)* h = &o->connection->paths;
        dh_Iter oi = dh_get(ObjectPath, h, o->path);
        if (oi != dh_end(h)) {
            dh_del(ObjectPath, h, oi);
        }
    }

    // Disconnect from binds
    d_Hash(Bind)* h = &o->interfaces;
    if (dh_size(h) > 0) {
        for (dh_Iter bi = dh_begin(h); bi != dh_end(h); bi++) {
            if (dh_exist(h, bi)) {
                dh_val(h, bi)->path = NULL;
            }
        }
    }

    dh_free(Bind, &o->interfaces);
    dv_free(ObjectPath, &o->children);
    free((char*) o->path.str);
    free(o);
}

// ----------------------------------------------------------------------------

static void CheckRemoveObject(struct ObjectPath* o)
{
    // We have 2 built in interfaces (introspectable and properties)
    // If these are the only two left and we have no children then we need
    // to prune this object
    if (dh_size(&o->interfaces) > 2 || dv_size(&o->children) > 0)
        return;

    // Remove the built in interfaces.
    d_Hash(Bind)* h = &o->interfaces;
    for (dh_Iter ii = dh_begin(h); ii != dh_end(h); ii++) {
        if (dh_exist(h, ii)) {
            adbus_ConnBind* b = dh_val(h, ii);
            b->path = NULL;
            adbusI_freeBind(b);
            dh_del(Bind, h, ii);
        }
    }

    // Remove it from its parent
    struct ObjectPath* parent = o->parent;
    if (parent) {
        for (size_t i = 0; i < dv_size(&parent->children); ++i) {
            if (dv_a(&parent->children, i) == o) {
                dv_remove(ObjectPath, &parent->children, i, 1);
                break;
            }
        }
        CheckRemoveObject(o->parent);
    }

    // Free the object
    adbusI_freeObjectPath(o);
}

// ----------------------------------------------------------------------------

void adbusI_freeBind(adbus_ConnBind* bind)
{
    struct ObjectPath* o = bind->path;
    if (o) {
        dh_Iter bi = dh_get(Bind, &o->interfaces, bind->interface->name);
        if (bi != dh_end(&o->interfaces)) {
            dh_del(Bind, &o->interfaces, bi);
        }
        CheckRemoveObject(o);
    }

    if (bind->release[0]) {
        if (bind->relproxy) {
            bind->relproxy(bind->relpuser, bind->release[0], bind->ruser[0]);
        } else {
            bind->release[0](bind->ruser[0]);
        }
    }

    if (bind->release[1]) {
        if (bind->relproxy) {
            bind->relproxy(bind->relpuser, bind->release[1], bind->ruser[1]);
        } else {
            bind->release[1](bind->ruser[1]);
        }
    }

    adbus_iface_deref(bind->interface);
    dil_remove(Bind, bind, &bind->fl);
    free(bind);
}

// ----------------------------------------------------------------------------

static struct ObjectPath* GetObject(
        adbus_Connection*       c,
        dh_strsz_t              path)
{
    int added;
    assert(adbusI_isValidObjectPath(path.str, path.sz));
    dh_Iter ki = dh_put(ObjectPath, &c->paths, path, &added);
    if (!added) {
        return dh_val(&c->paths, ki);
    }

    struct ObjectPath* o = NEW(struct ObjectPath);
    o->connection        = c;
    o->path.str          = adbusI_strndup(path.str, path.sz);
    o->path.sz           = path.sz;

    dh_key(&c->paths, ki) = o->path;
    dh_val(&c->paths, ki) = o;

    // Bind the builtin interfaces

    adbus_Bind b;
    adbus_bind_init(&b);
    b.cuser2     = o;
    b.interface  = c->introspectable;
    DoBind(o, &b);

    adbus_bind_init(&b);
    b.cuser2    = o;
    b.interface = c->properties;
    DoBind(o, &b);

    // Setup the parent-child links
    dh_strsz_t parent;
    adbusI_parentPath(path, &parent);
    if (parent.str) {
        o->parent = GetObject(c, parent);

        struct ObjectPath** pchild = dv_push(ObjectPath, &o->parent->children, 1);
        *pchild = o;
    }

    return o;
}

// ----------------------------------------------------------------------------

adbus_ConnBind* adbus_conn_bind(
        adbus_Connection*       c,
        const adbus_Bind*       b)
{
    if (ADBUS_TRACE_BIND) {
        adbusI_logbind("bind", b);
    }

    size_t psz = (b->pathSize >= 0) ? (size_t) b->pathSize : strlen(b->path);
    dh_strsz_t path = {b->path, psz};
    return DoBind(GetObject(c, path), b);
}


// ----------------------------------------------------------------------------

void adbus_conn_unbind(
        adbus_Connection*   c,
        adbus_ConnBind*     b)
{
    UNUSED(c);
    adbusI_freeBind(b);
}

// ----------------------------------------------------------------------------

int adbusI_dispatchBind(adbus_CbData* d)
{
    // should have been checked by parser
    assert(d->msg->path);
    assert(d->msg->member);

    adbus_Member* member;
    adbus_ConnBind* bind;

    if (d->msg->interface) {
        // If we know the interface, then we try and find the method on that
        // interface
        adbus_Interface* interface = adbus_conn_interface(
                d->connection,
                d->msg->path,
                d->msg->pathSize,
                d->msg->interface,
                (int) d->msg->interfaceSize,
                &bind);

        if (!interface) {
            return adbusI_interfaceError(d);
        }

        member = adbus_iface_method(
                interface,
                d->msg->member,
                (int) d->msg->memberSize);

    } else {
        // We don't know the interface, try and find the first method on any
        // interface with the member name
        member = adbus_conn_method(
                d->connection,
                d->msg->path,
                d->msg->pathSize,
                d->msg->member,
                (int) d->msg->memberSize,
                &bind);

    }

    if (!member)
        return adbusI_methodError(d);

    return adbus_mbr_call(member, bind, d);
}

// ----------------------------------------------------------------------------

adbus_Interface* adbus_conn_interface(
        adbus_Connection*   c,
        const char*         path,
        int                 pathSize,
        const char*         interface,
        int                 interfaceSize,
        adbus_ConnBind**    bind)
{
    if (pathSize < 0)
        pathSize = strlen(path);
    if (interfaceSize < 0)
        interfaceSize = strlen(interface);

    dh_strsz_t pstr = {path, pathSize};
    dh_strsz_t istr = {interface, interfaceSize};

    dh_Iter oi = dh_get(ObjectPath, &c->paths, pstr);
    if (oi == dh_end(&c->paths))
        return NULL;

    struct ObjectPath* o = dh_val(&c->paths, oi);

    dh_Iter bi = dh_get(Bind, &o->interfaces, istr);
    if (bi == dh_end(&o->interfaces))
        return NULL;

    adbus_ConnBind* b = dh_val(&o->interfaces, bi);

    if (bind)
        *bind = b;

    return b->interface;
}

// ----------------------------------------------------------------------------

adbus_Member* adbus_conn_method(
        adbus_Connection*   c,
        const char*         path,
        int                 pathSize,
        const char*         method,
        int                 methodSize,
        adbus_ConnBind**    bind)
{
    if (pathSize < 0)
        pathSize = strlen(path);
    if (methodSize < 0)
        methodSize = strlen(method);

    dh_strsz_t pstr = {path, pathSize};

    dh_Iter oi = dh_get(ObjectPath, &c->paths, pstr);
    if (oi == dh_end(&c->paths))
        return NULL;

    struct ObjectPath* o = dh_val(&c->paths, oi);
    d_Hash(Bind)* h = &o->interfaces;

    for (dh_Iter bi = dh_begin(h); bi != dh_end(h); ++bi) {
        if (dh_exist(h, bi)) {
            adbus_ConnBind* b = dh_val(h, bi);
            adbus_Member* m = adbus_iface_method(b->interface, method, methodSize);
            if (m) {
                if (bind)
                    *bind = b;
                return m;
            }
        }
    }

    return NULL;
}





