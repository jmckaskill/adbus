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

#include "client-bind.h"
#include "connection.h"
#include "interface.h"
#include "messages.h"

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



/* -------------------------------------------------------------------------- */

/** Initialises an adbus_Bind structure.
 *  \relates adbus_Bind
 */
void adbus_bind_init(adbus_Bind* b)
{
    memset(b, 0, sizeof(adbus_Bind));
    b->pathSize = -1;
}

/* -------------------------------------------------------------------------- */

adbus_ConnBind* adbusI_createBind(adbusI_ObjectTree* t, adbusI_ObjectNode* n, const adbus_Bind* bind)
{
    adbus_ConnBind* b;

    int added;
    dh_Iter bi = dh_put(Bind, &n->binds, bind->interface->name, &added);
    if (!added)
        return NULL;

    b           = NEW(adbus_ConnBind);
    b->node     = n;
    b->b        = *bind;
    b->b.path   = b->node->path.str;

    adbusI_refObjectNode(b->node);
    adbus_iface_ref(b->b.interface);

    dil_insert_after(Bind, &t->list, b, &b->hl);

    dh_val(&n->binds, bi) = b;
    dh_key(&n->binds, bi) = b->b.interface->name;

    return b;
}

/* -------------------------------------------------------------------------- */

static void FreeBind(adbus_ConnBind* b)
{
    /* Disconnect from node and tree */
    if (b->node) {
        d_Hash(Bind)* h = &b->node->binds;
        dh_Iter bi = dh_get(Bind, h, b->b.interface->name);
        if (bi != dh_end(h)) {
            dh_del(Bind, h, bi);
        }
    }
    dil_remove(Bind, b, &b->hl);

    /* Call release callbacks */
    if (b->b.release[0]) {
        if (b->b.relproxy) {
            b->b.relproxy(b->b.relpuser, NULL, b->b.release[0], b->b.ruser[0]);
        } else  {
            b->b.release[0](b->b.ruser[0]);
        }
    }

    if (b->b.release[1]) {
        if (b->b.relproxy) {
            b->b.relproxy(b->b.relpuser, NULL, b->b.release[1], b->b.ruser[1]);
        } else  {
            b->b.release[1](b->b.ruser[1]);
        }
    }

    /* Free data */
    adbusI_derefObjectNode(b->node);
    adbus_iface_deref(b->b.interface);
    free(b);
}

/* -------------------------------------------------------------------------- */

adbusI_ObjectNode* adbusI_getObjectNode(adbus_Connection* c, dh_strsz_t path)
{
    int added;
    dh_Iter ii;
   
    ii = dh_put(ObjectNode, &c->binds.lookup, path, &added);

    if (added) {
        dh_strsz_t parent;

        adbusI_ObjectNode* node = NEW(adbusI_ObjectNode);

        node->tree = &c->binds;

        node->path.str = adbusI_strndup(path.str, path.sz);
        node->path.sz  = path.sz;

        /* Setup node in the hashtable now as once we create the parent below,
         * ii becomes invalid.
         */
        dh_val(&c->binds.lookup, ii) = node;
        dh_key(&c->binds.lookup, ii) = node->path;

        {
            adbus_Bind b;
            adbus_bind_init(&b);
            b.cuser2 = node;

            b.interface = c->introspectable;
            node->introspectable = adbusI_createBind(&c->binds, node, &b);

            b.interface = c->properties;
            node->properties = adbusI_createBind(&c->binds, node, &b);
        }

        node->ref = 0;

        adbusI_parentPath(path, &parent);
        if (parent.str) {
            node->parent = adbusI_getObjectNode(c, parent);
            dl_insert_after(ObjectNode, &node->parent->children, node, &node->hl);
            adbusI_refObjectNode(node->parent);
        }

        return node;

    } else {
        return dh_val(&c->binds.lookup, ii);
    }
}

/* -------------------------------------------------------------------------- */

void adbusI_refObjectNode(adbusI_ObjectNode* n)
{ n->ref++; }

/* -------------------------------------------------------------------------- */

static void FreeObjectNode(adbusI_ObjectNode* n)
{
    assert(dh_size(&n->binds) == 0);
    assert(dl_isempty(&n->children));

    /* Disconnect from object tree */
    if (n->tree) {
        d_Hash(ObjectNode)* h = &n->tree->lookup;
        dh_Iter ii = dh_get(ObjectNode, h, n->path);
        if (ii != dh_end(h)) {
            dh_del(ObjectNode, h, ii);
        }
    }

    /* Disconnect from parent */
    if (n->parent) {
        dl_remove(ObjectNode, n, &n->hl);
        adbusI_derefObjectNode(n->parent);
        n->parent = NULL;
    }

    /* Free data */
    dh_free(Bind, &n->binds);
    free((char*) n->path.str);
    free(n);
}

void adbusI_derefObjectNode(adbusI_ObjectNode* n)
{
    if (n && --n->ref == 0) {
        assert(dh_size(&n->binds) == 2);

        /* Remove builtin interfaces */
        dh_clear(Bind, &n->binds);
        n->introspectable->node = NULL;
        n->properties->node = NULL;

        FreeBind(n->introspectable);
        FreeBind(n->properties);

        FreeObjectNode(n);
    }
}


/* -------------------------------------------------------------------------- */

adbus_ConnBind* adbus_conn_bind(
        adbus_Connection*       c,
        const adbus_Bind*       b)
{
    d_String pstr;
    dh_strsz_t path;
    adbusI_ObjectNode* node;
    adbus_ConnBind* ret;

    assert(!c->closed);

    ADBUSI_LOG_BIND_1(
			b,
			"bind (connection %s, %p)",
			adbus_conn_uniquename(c, NULL),
			(void*) c);

    ZERO(pstr);
    adbusI_sanitisePath(&pstr, b->path, b->pathSize);
    path.str = ds_cstr(&pstr);
    path.sz = ds_size(&pstr);

    assert(adbusI_isValidObjectPath(path.str, path.sz));

    node = adbusI_getObjectNode(c, path);
    ret = adbusI_createBind(&c->binds, node, b);

    ds_free(&pstr);

    return ret;
}


/* -------------------------------------------------------------------------- */

void adbus_conn_unbind(
        adbus_Connection*   c,
        adbus_ConnBind*     b)
{
    UNUSED(c);
    if (b) {
        ADBUSI_LOG_BIND_1(
				&b->b,
				"unbind (connection %s, %p)",
				adbus_conn_uniquename(c, NULL),
				(void*) c);

        FreeBind(b);
    }
}

/* -------------------------------------------------------------------------- */

void adbusI_freeObjectTree(adbusI_ObjectTree* t)
{
    dh_Iter ni;
    adbus_ConnBind* b;

    /* Free the object nodes */
    for (ni = dh_begin(&t->lookup); ni != dh_end(&t->lookup); ni++) {
        if (dh_exist(&t->lookup, ni)) {
            adbusI_ObjectNode* n = dh_val(&t->lookup, ni);

            /* Detach all object node links and free just the node */
            n->tree = NULL;
            dh_clear(Bind, &n->binds);
            dl_clear(ObjectNode, &n->children);
            n->parent = NULL;
            FreeObjectNode(n);
        }
    }
    dh_clear(ObjectNode, &t->lookup);

    /* Detach the binds from the object nodes.  We do this for all binds
     * first, as the FreeBind calls release callbacks which may try and unbind
     * other binds. 
     */
    DIL_FOREACH (Bind, b, &t->list, hl) {
        b->node = NULL;
    }

    /* Free the binds */
    DIL_FOREACH (Bind, b, &t->list, hl) {
        FreeBind(b);
    }

    assert(dh_size(&t->lookup) == 0);
    assert(dil_isempty(&t->list));

    dh_free(ObjectNode, &t->lookup);
}

/* -------------------------------------------------------------------------- */

int adbusI_dispatchMethod(adbus_Connection* c, adbus_CbData* d)
{
    adbus_ConnBind* bind;
    const adbus_Member* member;

    assert(d->ret);
    adbus_msg_reset(d->ret);

    if (d->msg->flags & ADBUS_MSG_NO_REPLY) {
        d->ret = NULL;
    }

    if (d->msg->interface) {
        /* If we know the interface, then we try and find the method on that
         * interface.
         */
        const adbus_Interface* interface = adbus_conn_interface(
                c,
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
        /* We don't know the interface, try and find the first method on any
         * interface with the member name.
         */
        member = adbus_conn_method(
                c,
                d->msg->path,
                d->msg->pathSize,
                d->msg->member,
                (int) d->msg->memberSize,
                &bind);

    }

    if (!member) {
        return adbusI_methodError(d);
    }

    return adbus_mbr_call(member, bind, d);
}

/* -------------------------------------------------------------------------- */

const adbus_Interface* adbus_conn_interface(
        adbus_Connection*   c,
        const char*         path,
        int                 pathSize,
        const char*         interface,
        int                 interfaceSize,
        adbus_ConnBind**    pbind)
{
    dh_strsz_t pstr;
    dh_strsz_t istr;
    dh_Iter ni, bi;
    adbusI_ObjectNode* node;
    adbus_ConnBind* bind;

    pstr.str = path;
    pstr.sz  = (pathSize >= 0) ? pathSize : strlen(path);

    istr.str = interface;
    istr.sz  = (interfaceSize >= 0) ? interfaceSize : strlen(interface);

    ni = dh_get(ObjectNode, &c->binds.lookup, pstr);
    if (ni == dh_end(&c->binds.lookup))
        return NULL;

    node = dh_val(&c->binds.lookup, ni);

    bi = dh_get(Bind, &node->binds, istr);
    if (bi == dh_end(&node->binds))
        return NULL;

    bind = dh_val(&node->binds, bi);

    if (pbind) {
        *pbind = bind;
    }

    return bind->b.interface;
}

/* -------------------------------------------------------------------------- */

const adbus_Member* adbus_conn_method(
        adbus_Connection*   c,
        const char*         path,
        int                 pathSize,
        const char*         method,
        int                 methodSize,
        adbus_ConnBind**    pbind)
{
    dh_strsz_t pstr;
    dh_strsz_t mstr;
    dh_Iter ni, bi;
    adbusI_ObjectNode* node;
    d_Hash(Bind)* binds;

    pstr.str = path;
    pstr.sz  = (pathSize >= 0) ? pathSize : strlen(path);

    mstr.str = method;
    mstr.sz  = (methodSize >= 0) ? methodSize : strlen(method);

    ni = dh_get(ObjectNode, &c->binds.lookup, pstr);
    if (ni == dh_end(&c->binds.lookup))
        return NULL;

    node = dh_val(&c->binds.lookup, ni);
    binds = &node->binds;

    for (bi = dh_begin(binds); bi != dh_end(binds); ++bi) {
        if (dh_exist(binds, bi)) {

            adbus_ConnBind* bind = dh_val(binds, bi);
            const adbus_Member* mbr = adbus_iface_method(bind->b.interface, mstr.str, mstr.sz);

            if (mbr) {
                if (pbind) {
                    *pbind = bind;
                }

                return mbr;
            }

        }
    }

    return NULL;
}

/* ------------------------------------------------------------------------- */

static void IntrospectInterfaces(adbusI_ObjectNode* node, d_String* out)
{
    dh_Iter bi;
    for (bi = dh_begin(&node->binds); bi != dh_end(&node->binds); ++bi) {
        if (dh_exist(&node->binds, bi)) {
            adbusI_introspectInterface(dh_val(&node->binds, bi)->b.interface, out);
        }
    }
}

/* ------------------------------------------------------------------------- */

static void IntrospectNode(adbusI_ObjectNode* node, d_String* out)
{
    size_t namelen;
    adbusI_ObjectNode* child;

    ds_cat(out,
           "<!DOCTYPE node PUBLIC \"-//freedesktop/DTD D-BUS Object Introspection 1.0//EN\"\n"
           "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
           "<node>\n");

    IntrospectInterfaces(node, out);

    namelen = node->path.sz;
    /* Now add the child objects */
    DL_FOREACH (ObjectNode, child, &node->children, hl) {
        /* Find the child tail ie ("bar" for "/foo/bar" or "foo" for "/foo") */
        const char* childstr = child->path.str;
        childstr += namelen;
        if (namelen > 1)
            childstr += 1; /* +1 for '/' separator when we are not the root node */

        ds_cat(out, "  <node name=\"");
        ds_cat(out, childstr);
        ds_cat(out, "\"/>\n");
    }

    ds_cat(out, "</node>\n");
}

/* ------------------------------------------------------------------------- */

int adbusI_introspect(adbus_CbData* d)
{
    adbusI_ObjectNode* node = (adbusI_ObjectNode*) d->user2;

    adbus_check_end(d);

    if (d->ret) {
        d_String out;
        ZERO(out);
        IntrospectNode(node, &out);

        adbus_msg_setsig(d->ret, "s", 1);
        adbus_msg_string(d->ret, ds_cstr(&out), ds_size(&out));
        adbus_msg_end(d->ret);

        ds_free(&out);
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

int adbusI_getProperty(adbus_CbData* d)
{
    dh_strsz_t mname, iname;
    adbus_ConnBind* bind;
    const adbus_Interface* interface;
    const adbus_Member* mbr;
    dh_Iter ii;

    adbusI_ObjectNode* node = (adbusI_ObjectNode*) d->user2;

    /* Unpack the message */
    iname.str = adbus_check_string(d, &iname.sz);
    mname.str = adbus_check_string(d, &mname.sz);
    adbus_check_end(d);

    /* Get the interface */
    ii = dh_get(Bind, &node->binds, iname); 
    if (ii == dh_end(&node->binds)) {
        return adbusI_interfaceError(d);
    }
    bind = dh_val(&node->binds, ii);
    interface = dh_val(&node->binds, ii)->b.interface;

    /* Get the property */
    mbr = adbus_iface_property(interface, mname.str, mname.sz);

    if (!mbr) {
        return adbusI_propertyError(d);
    }

    adbus_iface_ref(interface);
    d->user1 = (void*) mbr;
    d->user2 = bind->b.cuser2;

    return adbusI_proxiedDispatch(
            bind->b.proxy,
            bind->b.puser,
            &adbusI_proxiedGetProperty,
            d);
}

/* ------------------------------------------------------------------------- */

int adbusI_getAllProperties(adbus_CbData* d)
{
    dh_strsz_t iname;
    adbus_ConnBind* bind;
    const adbus_Interface* interface;
    dh_Iter ii;

    adbusI_ObjectNode* node = (adbusI_ObjectNode*) d->user2;

    /* Unpack the message */
    iname.str = adbus_check_string(d, &iname.sz);
    adbus_check_end(d);

    /* Get the interface */
    ii = dh_get(Bind, &node->binds, iname); 
    if (ii == dh_end(&node->binds)) {
        return adbusI_interfaceError(d);
    }
    bind = dh_val(&node->binds, ii);
    interface = dh_val(&node->binds, ii)->b.interface;

    adbus_iface_ref(interface);
    d->user1 = (void*) interface;
    d->user2 = bind->b.cuser2;

    return adbusI_proxiedDispatch(
            bind->b.proxy,
            bind->b.puser,
            &adbusI_proxiedGetAllProperties,
            d);
}

/* ------------------------------------------------------------------------- */

int adbusI_setProperty(adbus_CbData* d)
{
    dh_strsz_t mname, iname;
    adbus_ConnBind* bind;
    const adbus_Interface* interface;
    const adbus_Member* mbr;
    dh_Iter ii;

    adbusI_ObjectNode* node = (adbusI_ObjectNode*) d->user2;

    /* Unpack the message */
    iname.str = adbus_check_string(d, &iname.sz);
    mname.str = adbus_check_string(d, &mname.sz);

    /* Get the interface */
    ii = dh_get(Bind, &node->binds, iname); 
    if (ii == dh_end(&node->binds)) {
        return adbusI_interfaceError(d);
    }
    bind = dh_val(&node->binds, ii);
    interface = dh_val(&node->binds, ii)->b.interface;

    /* Get the property */
    mbr = adbus_iface_property(interface, mname.str, mname.sz);

    if (!mbr) {
        return adbusI_propertyError(d);
    }

    adbus_iface_ref(interface);
    d->user1 = (void*) mbr;
    d->user2 = bind->b.cuser2;
    
    return adbusI_proxiedDispatch(
            bind->b.proxy,
            bind->b.puser,
            &adbusI_proxiedSetProperty,
            d);
}

