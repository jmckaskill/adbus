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
#include "interface.h"

#include "connection.h"
#include "misc.h"
#include "messages.h"

/** \struct adbus_Interface
 *  
 *  \brief Wrapper around a dbus interface that can be bound to multiple paths
 *  on multiple connections
 *
 *  Generally interfaces are created up front statically or with some kind of
 *  singleton so that they are reused. The interface then binds in all of the
 *  information in the introspection xml for that interface as well as the
 *  callbacks for properties and methods.
 *
 *  \warning Once an interface is bound to a connection it cannot be further
 *  modified.
 *
 *  \note In multithreaded programs, it's generally assumed that all the
 *  callbacks in an interface will be called on the thread that the bind
 *  occured on. This is required for the GetAll function which calls all of
 *  the property getters locally.
 *
 *  For example:
 *  \code
 *  adbus_Interface* i;
 *  adbus_Member* m;
 *
 *  i = adbus_iface_new("org.freedesktop.SampleInterface", -1);
 *
 *  m = adbus_iface_addmethod(i, "Frobate", -1);
 *  adbus_mbr_setmethod(m, &Frobate, NULL);
 *  adbus_mbr_argsig(m, "i", -1);
 *  adbus_mbr_argname(m, "foo", -1);
 *  adbus_mbr_retsig(m, "sa{us}", -1)
 *  adbus_mbr_retname(m, "bar", -1);
 *  adbus_mbr_retname(m, "baz", -1);
 *  adbus_mbr_annotate(m, "org.freedesktop.DBus.Deprecated", -1, "true", -1);
 *
 *  m = adbus_iface_addsignal(i, "Changed", -1);
 *  adbus_mbr_argsig(m, "b", -1);
 *  adbus_mbr_argname(m, "new_value", -1);
 *
 *  m = adbus_iface_addproperty(i, "Bar", -1);
 *  adbus_mbr_argsig(m, "y", -1);
 *  adbus_mbr_setgetter(m, &GetBar, NULL);
 *  adbus_mbr_setsetter(m, &SetBar, NULL);
 *  \endcode
 *
 *  On introspection this will generate the following xml:
 *
 *  \code
 *  <interface name="org.freedesktop.SampleInterface">
 *    <method name="Frobate">
 *      <arg name="foo" type="i" direction="in"/>
 *      <arg name="bar" type="s" direction="out"/>
 *      <arg name="baz" type="a{us}" direction="out"/>
 *      <annotation name="org.freedesktop.DBus.Deprecated" value="true"/>
 *    </method>
 *    <signal name="Changed">
 *      <arg name="new_value" type="b"/>
 *    </signal>
 *    <property name="Bar" type="y" access="readwrite"/>
 *  </interface>
 *  \endcode
 *
 *  For further examples and details on how to manipulate the members see
 *  adbus_Member.
 */

/** \struct adbus_Member
 *
 *  \brief Handle to an idividual member in an adbus_Interface.
 *
 *  \note If writing C code, it's easiest to pull out arguments in callbacks
 *  via the adbus_check_* functions (eg adbus_check_boolean()).
 *
 *  \section methods Methods
 *
 *  Methods are added to the interface using adbus_iface_addmethod(). The
 *  caller can then set:
 *
 *  - Arguments via adbus_mbr_argsig() and adbus_mbr_argname().
 *  - Return arguments via adbus_mbr_retsig() and adbus_mbr_retname(). 
 *  - Annotations via adbus_mbr_annotate().
 *  - Method callback via adbus_mbr_setmethod().
 *
 *  The caller must set a method callback.
 *
 *  When setting the method callback, you can also set a user data value which
 *  comes through to the callback as the adbus_CbData::user1 field. This way
 *  the same function can be reused for multiple methods.
 *
 *  User data can also be tied to the method so that it can be freed when the
 *  interface is eventually freed using adbus_mbr_addrelease().
 *
 *  For example:
 *  \code
 *  struct MultiplyData
 *  {
 *      int Multiplier;
 *  };
 *
 *  static int Multiply(adbus_CbData* d)
 *  {
 *      int32_t num = adbus_check_i32(d);
 *      adbus_check_end(d);
 *
 *      MultiplyData* mul = (MultiplyData*) d->user1;
 *
 *      if (d->ret)
 *          adbus_msg_i32(d->ret, num * mul->Multiplier);
 *
 *      return 0;
 *  }
 *
 *  MultiplyData* multiply1 = (MultiplyData*) calloc(sizeof(MultiplyData), 1);
 *  MultiplyData* multiply2 = (MultiplyData*) calloc(sizeof(MultiplyData), 1);
 *  multiply1->Multiplier = 1;
 *  multiply2->Multiplier = 2;
 *
 *  m = adbus_iface_addmethod(i, "Multiply1", -1);
 *  adbus_mbr_setmethod(m, &Multiply, multiply1);
 *  adbus_mbr_addrelease(m, &free, multiply1);
 *  adbus_mbr_argsig(m, "i", -1);
 *  adbus_mbr_retsig(m, "i", -1);
 *
 *  m = adbus_iface_addmethod(i, "Multiply2", -1);
 *  adbus_mbr_setmethod(m, &Multiply, multiply2);
 *  adbus_mbr_addrelease(m, &free, multiply2);
 *  adbus_mbr_argsig(m, "i", -1);
 *  adbus_mbr_retsig(m, "i", -1);
 *  \endcode
 *
 *  The method callback can use the user2 field supplied in the bind (see
 *  adbus_Bind) to distinguish between the method being called on multiple
 *  objects.
 *
 *  For example:
 *  \code
 *  class Multiplier
 *  {
 *  public:
 *      Multiplier(int num) : m_Num(num) {}
 *  private:
 *      static int Multiply(adbus_CbData* d);
 *      static void Free(void* d) {delete (Multiplier*) d;}
 *      int m_Num;
 *  };
 *
 *  int Multiplier::Multiply(adbus_CbData* d)
 *  {
 *      int num = adbus_check_i32(d);
 *      adbus_check_end(d);
 *
 *      Multiplier* s = (Multiplier*) d->user2;
 *      if (d->ret)
 *          adbus_check_i32(d->ret, s->m_Num * num);
 *
 *      return 0;
 *  }
 *  
 *  adbus_Interface* iface;
 *  adbus_Member* mbr;
 *
 *  iface = adbus_iface_new("org.freedesktop.SampleInterface", -1);
 *  mbr = adbus_iface_addmethod(iface, "Multiply", -1);
 *  adbus_mbr_setmethod(mbr, &Multiplier::Multiply, NULL);
 *  adbus_mbr_argsig(mbr, "i", -1);
 *  adbus_mbr_retsig(mbr, "i", -1);
 *
 *  Multiplier* mul3 = new Multiplier(3);
 *
 *  adbus_Bind bind;
 *  adbus_bind_init(&bind);
 *  bind.interface  = iface;
 *  bind.path       = "/Mul3";
 *  bind.cuser2     = mul3;
 *  bind.release[0] = &Multiply::Free;
 *  bind.ruser[0]   = mul3;
 *  adbus_conn_bind(connection, &bind);
 *  \endcode
 *
 *
 *
 *
 *
 *  \section signals Signals
 *
 *  Signals are added to an interface using adbus_mbr_addsignal(). The caller
 *  can then set:
 *
 *  - Signal arguments via adbus_mbr_argsig() and adbus_mbr_argname().
 *  - Annotations via adbus_mbr_annotate().
 *
 *  After binding the interface to a path, signals can be emitted by either
 *  manually building a signal message and sending or using the adbus_Signal
 *  utility module.
 *
 *  For example:
 *  \code
 *  // Create an interface with a single signal "Changed" which has a single
 *  // boolean argument
 *  adbus_Interface* iface;
 *  adbus_Member* mbr;
 *  iface = adbus_iface_new("org.freedesktop.SampleInterface", -1);
 *  mbr = adbus_iface_addsignal(iface, "Changed", -1);
 *  adbus_mbr_argsig(mbr, "b", -1);
 *
 *  // Bind this interface to the path "/"
 *  adbus_Bind bind;
 *  adbus_bind_init(&bind);
 *  bind.interface = iface;
 *  bind.path      = "/";
 *  adbus_conn_bind(connection, &bind);
 *
 *  // Create an adbus_Signal that we can use to emit the signal and bind it
 *  // to our changed signal at the path "/"
 *  adbus_Signal* sig = adbus_sig_new();
 *  adbus_sig_bind(sig, mbr, connection, "/", -1);
 *
 *  // Emit our signal
 *  adbus_MsgFactory* msg = adbus_sig_msg(sig);
 *  adbus_msg_boolean(msg, 1);
 *  adbus_sig_emit(sig);
 *  \endcode
 *
 *
 *
 *
 *
 *
 *  \section properties Properties
 *
 *  Properties are added to an interface using adbus_mbr_addproperty(). The
 *  caller can then set:
 *
 *  - Property get and set callbacks via adbus_mbr_setgetter() and
 *  adbus_mbr_setsetter().
 *  - Annotations via adbus_mbr_annotate().
 *
 *  The access property exported through the introspection xml is determined
 *  by looking at which of the getter and setter callbacks are set.
 *
 *  \note One of adbus_mbr_setgetter(), adbus_mbr_setsetter(), or both must be
 *  called with a valid callback.
 *
 *  For example:
 *  \code
 *  adbus_Bool sFoo = 0;
 *  int SetFoo(adbus_CbData* d)
 *  { 
 *      return adbus_iter_boolean(d->setprop, &sFoo); 
 *  }
 *
 *  int GetFoo(adbus_CbData* d)
 *  { 
 *      adbus_buf_boolean(d->getprop, sFoo);
 *      return 0;
 *  }
 *
 *  // Create an interface with a single read/write boolean property "Foo"
 *  adbus_Interface* iface;
 *  adbus_Member* mbr;
 *  iface = adbus_iface_new("org.freedesktop.SampleInterface", -1);
 *  mbr = adbus_iface_addproperty(iface, "Foo", -1, "b", -1);
 *  adbus_mbr_setgetter(mbr, &SetFoo, NULL);
 *  adbus_mbr_setsetter(mbr, &GetFoo, NULL);
 *  \endcode
 *
 *  \code
 *  int SetFoo(adbus_CbData* d)
 *  {
 *      sFoo = adbus_check_boolean(d);
 *      return 0;
 *  }
 *  \endcode
 *
 */


/* ------------------------------------------------------------------------- */

/** Creates a new interface.
 *  \relates adbus_Interface
 */
adbus_Interface* adbus_iface_new(
        const char*     name,
        int             size)
{
    adbus_Interface* i  = NEW(adbus_Interface);
    i->name.sz          = (size >= 0) ? size : strlen(name);
    i->name.str         = adbusI_strndup(name, i->name.sz);

    adbus_iface_ref(i);

    return i;  
}

/* ------------------------------------------------------------------------- */

/** Refs an interface.
 *  \relates adbus_Interface
 */
void adbus_iface_ref(adbus_Interface* interface)
{ adbusI_InterlockedIncrement(&interface->ref); }

/* ------------------------------------------------------------------------- */

static void FreeMember(adbus_Member* member);

/** Derefs an interface.
 *  \relates adbus_Interface
 */
void adbus_iface_deref(adbus_Interface* i)
{
    if (i && adbusI_InterlockedDecrement(&i->ref) == 0) {
        dh_Iter ii;

        for (ii = dh_begin(&i->members); ii != dh_end(&i->members); ++ii) {
            if (dh_exist(&i->members, ii)) {
                FreeMember(dh_val(&i->members, ii));
            }
        }
        dh_free(Member, &i->members);

        free((char*) i->name.str);
        free(i);
    }
}

/* ------------------------------------------------------------------------- */

static adbus_Member* AddMethod(
        adbus_Interface*    i,
        adbusI_MemberType   type,
        const char*         name,
        int                 size)
{
    dh_Iter mi;
    int added;

    adbus_Member* m = NEW(adbus_Member);
    m->interface    = i;
    m->type         = type;
    m->name.sz      = (size >= 0) ? size : strlen(name);
    m->name.str     = adbusI_strndup(name, m->name.sz);

    mi = dh_put(Member, &i->members, m->name, &added);
    if (!added) {
        FreeMember(dh_val(&i->members, mi));
    }

    dh_key(&i->members, mi) = m->name;
    dh_val(&i->members, mi) = m;

    return m;
}

/** Adds a new method.
 *  \relates adbus_Interface
 */
adbus_Member* adbus_iface_addmethod(adbus_Interface* i, const char* name, int size)
{ return AddMethod(i, ADBUSI_METHOD, name, size); }

/** Adds a new signal.
 *  \relates adbus_Interface
 */
adbus_Member* adbus_iface_addsignal(adbus_Interface* i, const char* name, int size)
{ return AddMethod(i, ADBUSI_SIGNAL, name, size); }

/** Adds a new property.
 *  \relates adbus_Interface
 */
adbus_Member* adbus_iface_addproperty(
        adbus_Interface*    i,
        const char*         name, 
        int                 namesize,
        const char*         sig, 
        int                 sigsize)
{ 
    adbus_Member* m = AddMethod(i, ADBUSI_PROPERTY, name, namesize); 
    m->propertyType = (sigsize >= 0)
                    ? adbusI_strndup(sig, sigsize)
                    : adbusI_strdup(sig);
    return m;
}

/* ------------------------------------------------------------------------- */

static void FreeMember(adbus_Member* m)
{
    if (m) {
        dh_Iter ii;
        size_t i;

        if (m->release[0]) {
            m->release[0](m->ruser[0]);
        }

        if (m->release[1]) {
            m->release[1](m->ruser[1]);
        }

        for (ii = dh_begin(&m->annotations); ii != dh_end(&m->annotations); ++ii) {
            if (dh_exist(&m->annotations, ii)) {
                free((char*) dh_key(&m->annotations, ii));
                free((char*) dh_val(&m->annotations, ii));
            }
        }
        dh_free(StringPair, &m->annotations);

        for (i = 0; i < dv_size(&m->arguments); i++) {
            free(dv_a(&m->arguments, i));
        }
        dv_free(String, &m->arguments);

        for (i = 0; i < dv_size(&m->returns); i++) {
            free(dv_a(&m->returns, i));
        }
        dv_free(String, &m->returns);

        ds_free(&m->argsig);
        ds_free(&m->retsig);

        free(m->propertyType);
        free((char*) m->name.str);

        free(m);
    }
}

/* ------------------------------------------------------------------------- */

static adbus_Member* GetMethod(
        adbus_Interface*      i,
        adbusI_MemberType     type,
        const char*           name,
        int                   size)
{
    dh_strsz_t mstr;
    dh_Iter mi;
    adbus_Member* mbr;

    mstr.str = name;
    mstr.sz  = (size >= 0) ? size : strlen(name);

    mi = dh_get(Member, &i->members, mstr);
    if (mi == dh_end(&i->members))
        return NULL;

    mbr = dh_val(&i->members, mi);
    if (mbr->type != type)
        return NULL;

    return mbr;
}

/** Gets a method.
 *  \relates adbus_Interface
 */
adbus_Member* adbus_iface_method(adbus_Interface* i, const char* name, int size)
{ return GetMethod(i, ADBUSI_METHOD, name, size); }

/** Gets a signal.
 *  \relates adbus_Interface
 */
adbus_Member* adbus_iface_signal(adbus_Interface* i, const char* name, int size)
{ return GetMethod(i, ADBUSI_SIGNAL, name, size); }

/** Gets a property.
 *  \relates adbus_Interface
 */
adbus_Member* adbus_iface_property(adbus_Interface* i, const char* name, int size)
{ return GetMethod(i, ADBUSI_PROPERTY, name, size); }

/* -------------------------------------------------------------------------
 * Member management
 * -------------------------------------------------------------------------
 */

/** Appends to the argument signature.
 *  \relates adbus_Member
 *  
 *  This merely appends to the full argument signature so multiple arguments
 *  can be added in one shot.
 *
 *  Can only be called on method and signal members.
 */
void adbus_mbr_argsig(adbus_Member* m, const char* sig, int size)
{
    assert(m->type == ADBUSI_METHOD || m->type == ADBUSI_SIGNAL);

    if (size >= 0) {
        ds_cat_n(&m->argsig, sig, size);
    } else {
        ds_cat(&m->argsig, sig);
    }
}

/* ------------------------------------------------------------------------- */

/** Sets the next argument name.
 *  \relates adbus_Member
 *
 *  Arguments do not require a name and the name can be set before or after
 *  the signature is set, but there cannot be more argument names than
 *  argument signatures by the time this interface is bound.
 *
 *  Can only be called on method and signal members.
 */
void adbus_mbr_argname(adbus_Member* m, const char* name, int size)
{
    char** pstr = dv_push(String, &m->arguments, 1);
    *pstr = (size >= 0) ? adbusI_strndup(name, size) : adbusI_strdup(name);

    assert(m->type == ADBUSI_METHOD || m->type == ADBUSI_SIGNAL);
}

/* ------------------------------------------------------------------------- */

/** Appends to the return argument signature.
 *  \relates adbus_Member
 *  
 *  This merely appends to the full argument signature so multiple arguments
 *  can be added in one shot.
 *
 *  Can only be called on method members.
 */
void adbus_mbr_retsig(adbus_Member* m, const char* sig, int size)
{
    assert(m->type == ADBUSI_METHOD);

    if (size >= 0) {
        ds_cat_n(&m->retsig, sig, size);
    } else {
        ds_cat(&m->retsig, sig);
    }
}

/* ------------------------------------------------------------------------- */

/** Sets the next return argument name.
 *  \relates adbus_Member
 *
 *  Arguments do not require a name and the name can be set before or after
 *  the signature is set, but there cannot be more argument names than
 *  argument signatures by the time this interface is bound.
 *
 *  Can only be called on method members.
 *
 */
void adbus_mbr_retname(adbus_Member* m, const char* name, int size)
{
    char** pstr = dv_push(String, &m->returns, 1);
    *pstr = (size >= 0) ? adbusI_strndup(name, size) : adbusI_strdup(name);

    assert(m->type == ADBUSI_METHOD);
}

/* ------------------------------------------------------------------------- */

/** Adds an annotation to the member.
 *  \relates adbus_Member
 */
void adbus_mbr_annotate(
        adbus_Member* m,
        const char*         name,
        int                 nameSize,
        const char*         value,
        int                 valueSize)
{
    int added;
    dh_Iter ii;

    name = (nameSize >= 0)
         ? adbusI_strndup(name, nameSize)
         : adbusI_strdup(name);

    value = (valueSize >= 0)
          ? adbusI_strndup(value, valueSize)
          : adbusI_strdup(value);

    ii = dh_put(StringPair, &m->annotations, name, &added);
    if (!added) {
        free((char*) dh_key(&m->annotations, ii));
        free((char*) dh_val(&m->annotations, ii));
    }

    dh_key(&m->annotations, ii) = (char*) name;
    dh_val(&m->annotations, ii) = (char*) value;
}

/* ------------------------------------------------------------------------- */

/** Adds a callback and user data to be called when the interface is freed.
 *  \relates adbus_Member
 *
 *  \warning The callback can be called on any thread.
 */
void adbus_mbr_addrelease(
        adbus_Member*           m,
        adbus_Callback          release,
        void*                   ruser)
{
    if (m->release[0]) {
        assert(!m->release[1]);
        m->release[1] = release;
        m->ruser[1] = ruser;
    } else {
        m->release[0] = release;
        m->ruser[0] = ruser;
    }
}

/* ------------------------------------------------------------------------- */

/** Sets the method callback.
 *  \relates adbus_Member
 *
 *  This must be set before the interface is bound.
 *
 *  Can only be called on method members.
 */
void adbus_mbr_setmethod(
        adbus_Member*           m,
        adbus_MsgCallback       callback,
        void*                   user1)
{
    m->methodCallback = callback;
    m->methodData     = user1;
}

/* ------------------------------------------------------------------------- */

/** Sets the get callback for properties.
 *  \relates adbus_Member
 *
 *  At least one of this or adbus_mbr_setsetter() must be called befor ethe
 *  interface is bound.
 *
 *  Can only be called on property members.
 */
void adbus_mbr_setgetter(
        adbus_Member*         m,
        adbus_MsgCallback     callback,
        void*                 user1)
{
    m->getPropertyCallback = callback;
    m->getPropertyData     = user1;
}

/* ------------------------------------------------------------------------- */

/** Sets the set callback for properties.
 *  \relates adbus_Member
 *
 *  At least one of this or adbus_mbr_setsetter() must be called befor ethe
 *  interface is bound.
 *
 *  Can only be called on property members.
 */
void adbus_mbr_setsetter(
        adbus_Member*         m,
        adbus_MsgCallback     callback,
        void*                 user1)
{
    m->setPropertyCallback = callback;
    m->setPropertyData     = user1;
}

/* ------------------------------------------------------------------------- */

static int DoCall(adbus_CbData* d)
{
    int ret;
    adbus_Member* mbr = (adbus_Member*) d->user1;
    /* d->user2 is already set to the bind userdata */
    d->user1 = mbr->methodData;

    if (d->ret) {
        adbus_msg_setsig(d->ret, ds_cstr(&mbr->retsig), ds_size(&mbr->retsig));
    }

    ret = adbus_dispatch(mbr->methodCallback, d);
    adbus_iface_deref(mbr->interface);
    return ret;
}

/** Calls a method member directly.
 *  \relates adbus_Member
 *
 *  \note This may proxy the method call to another thread.
 *
 *  This is useful when using the dbus binding for other purposes eg. bindings
 *  into an embedded scripting language.
 *
 *  The bind can be gotten from the return value of adbus_conn_bind() or by
 *  looking it up via adbus_conn_interface().
 */
int adbus_mbr_call(
        adbus_Member*         mbr,
        adbus_ConnBind*       bind,
        adbus_CbData*         d)
{
    if (!mbr->methodCallback) {
        return adbusI_methodError(d);
    }

    if (ds_cmp_n(&mbr->argsig, d->msg->signature, d->msg->signatureSize) != 0) {
        return adbus_error_argument(d);
    }

    adbus_iface_ref(mbr->interface);
    d->user1 = mbr;
    d->user2 = bind->cuser2;

    if (bind->proxy) {
        return bind->proxy(bind->puser, &DoCall, d);
    } else {
        return DoCall(d);
    }
}







/* -------------------------------------------------------------------------
 * Introspection callback (private)
 * -------------------------------------------------------------------------
 */

static void IntrospectArguments(adbus_Member* m, d_String* out)
{
    size_t argi = 0;
    const char* arg = ds_cstr(&m->argsig);
    while (arg && *arg) {
        const char* next = adbus_nextarg(arg);
        ds_cat(out, "\t\t\t<arg type=\"");
        ds_cat_n(out, arg, next - arg);
        if (argi < dv_size(&m->arguments)) {
            ds_cat(out, "\" name=\"");
            ds_cat(out, dv_a(&m->arguments, argi));
        }
        ds_cat(out, "\"/>\n");
        arg = next;
        argi++;
    }
    assert(argi >= dv_size(&m->arguments)); 

    argi = 0;
    arg = ds_cstr(&m->retsig);
    while (arg && *arg) {
        const char* next = adbus_nextarg(arg);
        ds_cat(out, "\t\t\t<arg type=\"");
        ds_cat_n(out, arg, next - arg);
        if (argi < dv_size(&m->returns)) {
            ds_cat(out, "\" name=\"");
            ds_cat(out, dv_a(&m->returns, argi));
        }
        ds_cat(out, "\" direction=\"out\"/>\n");
        arg = next;
        argi++;
    }
    assert(argi >= dv_size(&m->returns)); 
 
}

/* ------------------------------------------------------------------------- */

static void IntrospectAnnotations(adbus_Member* m, d_String* out)
{
    dh_Iter ai;
    for (ai = dh_begin(&m->annotations); ai != dh_end(&m->annotations); ++ai) {
        if (dh_exist(&m->annotations, ai)) {
            ds_cat(out, "\t\t\t<annotation name=\"");
            ds_cat(out, dh_key(&m->annotations, ai));
            ds_cat(out, "\" value=\"");
            ds_cat(out, dh_val(&m->annotations, ai));
            ds_cat(out, "\"/>\n");
        }
    }
}
/* ------------------------------------------------------------------------- */

static void IntrospectMember(adbus_Member* m, d_String* out)
{
    switch (m->type)
    {
        case ADBUSI_PROPERTY:
            {
                adbus_Bool read  = (m->getPropertyCallback != NULL);
                adbus_Bool write = (m->setPropertyCallback != NULL);

                ds_cat(out, "\t\t<property name=\"");
                ds_cat_n(out, m->name.str, m->name.sz);
                ds_cat(out, "\" type=\"");
                ds_cat(out, m->propertyType);
                ds_cat(out, "\" access=\"");
                if (read && write) {
                    ds_cat(out, "readwrite");
                } else if (read) {
                    ds_cat(out, "read");
                } else if (write) {
                    ds_cat(out, "write");
                } else {
                    assert(0);
                }

                if (dh_size(&m->annotations) == 0) {
                    ds_cat(out, "\"/>\n");
                } else {
                    ds_cat(out, "\">\n");
                    IntrospectAnnotations(m, out);
                    ds_cat(out, "\t\t</property>\n");
                }
            }
            break;
        case ADBUSI_METHOD:
            {
                ds_cat(out, "\t\t<method name=\"");
                ds_cat_n(out, m->name.str, m->name.sz);
                ds_cat(out, "\">\n");

                IntrospectAnnotations(m, out);
                IntrospectArguments(m, out);

                ds_cat(out, "\t\t</method>\n");
            }
            break;
        case ADBUSI_SIGNAL:
            {
                ds_cat(out, "\t\t<signal name=\"");
                ds_cat_n(out, m->name.str, m->name.sz);
                ds_cat(out, "\">\n");

                IntrospectAnnotations(m, out);
                IntrospectArguments(m, out);

                ds_cat(out, "\t\t</signal>\n");
            }
            break;
        default:
            assert(0);
            break;
    }
}

void adbusI_introspectInterface(adbus_Interface* i, d_String* out)
{
    dh_Iter mi;

    ds_cat(out, "\t<interface name=\"");
    ds_cat_n(out, i->name.str, i->name.sz);
    ds_cat(out, "\">\n");

    for (mi = dh_begin(&i->members); mi != dh_end(&i->members); ++mi) {
        if (dh_exist(&i->members, mi)) {
            IntrospectMember(dh_val(&i->members, mi), out);
        }
    }

    ds_cat(out, "\t</interface>\n");
}






/* -------------------------------------------------------------------------
 * Properties callbacks (private)
 * -------------------------------------------------------------------------
 */

int adbusI_proxiedGetProperty(adbus_CbData* d)
{
    int ret = 0;
    adbus_BufVariant v;

    adbus_Member* mbr = (adbus_Member*) d->user1;

    /* Check that we can read the property */
    if (!mbr->getPropertyCallback) {
        adbusI_propReadError(d);
        goto end;
    }

    d->getprop = adbus_msg_argbuffer(d->ret);
    d->user1 = mbr->getPropertyData;
    /* User2 is already set to the bind cuser2 */

    /* Get the property value */
    adbus_msg_beginvariant(d->ret, &v, mbr->propertyType, -1);
    ret = mbr->getPropertyCallback(d);

end:
    adbus_iface_deref(mbr->interface);
    return ret;
}


/* ------------------------------------------------------------------------- */

int adbusI_proxiedGetAllProperties(adbus_CbData* d)
{
    adbus_BufArray a;
    adbus_BufVariant v;
    dh_Iter mi;

    adbus_Interface* interface = (adbus_Interface*) d->user1;
    d_Hash(Member)* mbrs = &interface->members;
    
    /* Iterate over the properties and marshal up the values */
    adbus_msg_beginarray(d->ret, &a);

    for (mi = dh_begin(mbrs); mi != dh_end(mbrs); ++mi) {
        if (dh_exist(mbrs, mi)) {

            adbus_Member* mbr = dh_val(mbrs, mi);
            adbus_MsgCallback callback = mbr->getPropertyCallback;

            /* Check that it is a property */
            if (mbr->type != ADBUSI_PROPERTY) {
                continue;
            }

            /* Check that we can read the property */
            if (!callback) {
                continue;
            }

            /* Set the property entry */
            adbus_msg_arrayentry(d->ret, &a);
            adbus_msg_begindictentry(d->ret);
            adbus_msg_string(d->ret, mbr->name.str, mbr->name.sz);
            adbus_msg_beginvariant(d->ret, &v, mbr->propertyType, -1);

            d->getprop = adbus_msg_argbuffer(d->ret);
            d->user1 = mbr->getPropertyData;
            /* User2 is already set to the bind cuser2 */

            if (callback(d) || adbus_msg_type(d->ret) == ADBUS_MSG_ERROR)
                goto err;

            adbus_msg_endvariant(d->ret, &v);
            adbus_msg_enddictentry(d->ret);
        }
    }

    adbus_msg_endarray(d->ret, &a);
    adbus_msg_end(d->ret);
    adbus_iface_deref(interface);
    return 0;

err:
    adbus_iface_deref(interface);
    return -1;
}


/* ------------------------------------------------------------------------- */

int adbusI_proxiedSetProperty(adbus_CbData* d)
{
    int ret = -1;
    adbus_IterVariant v;

    adbus_Member* mbr = (adbus_Member*) d->user1;
    adbus_MsgCallback callback = mbr->setPropertyCallback;

    /* Check that we can write the property */
    if (!callback) {
        ret = adbusI_propWriteError(d);
        goto end;
    }

    /* Jump over the interface and member fields */
    adbus_iter_args(&d->setprop, d->msg);
    if (adbus_iter_string(&d->setprop, NULL, NULL))
        goto end;
    if (adbus_iter_string(&d->setprop, NULL, NULL))
        goto end;

    /* Check the property type */
    if (adbus_iter_beginvariant(&d->setprop, &v))
        goto end;

    if (strcmp(mbr->propertyType, v.sig) != 0) {
        ret = adbusI_propTypeError(d);
        goto end;
    }

    /* setprop is already setup */
    d->user1 = mbr->setPropertyData;
    /* user2 is already set to the bind cuser2 */

    /* Set the property */
    ret = mbr->setPropertyCallback(d);

end:
    adbus_iface_deref(mbr->interface);
    return ret;
}






