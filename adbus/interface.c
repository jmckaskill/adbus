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
#include "interface.h"

#include "connection.h"
#include "misc.h"

#include "memory/kvector.h"

#include <assert.h>
#include <string.h>

// ----------------------------------------------------------------------------
// Interface management
// ----------------------------------------------------------------------------

adbus_Interface* adbus_iface_new(
        const char*     name,
        int             size)
{
    adbus_Interface* i = NEW(adbus_Interface);

    i->name = (size >= 0)
            ? adbusI_strndup(name, size)
            : adbusI_strdup(name);
    i->members = kh_new(MemberPtr);

    return i;  
}

// ----------------------------------------------------------------------------

static void FreeMember(adbus_Member* member);

void adbus_iface_free(adbus_Interface* i)
{
    if (i) {
        for (khiter_t ii = kh_begin(i->members); ii != kh_end(i->members); ++ii) {
            if (kh_exist(i->members, ii))
                FreeMember(kh_val(i->members, ii));
        }
        kh_free(MemberPtr, i->members);

        free(i);
    }
}

// ----------------------------------------------------------------------------

adbus_Member* AddMethod(
        adbus_Interface*    i,
        adbusI_MemberType   type,
        const char*         name,
        int                 size)
{
    assert(name);

    adbus_Member* m = NEW(adbus_Member);
    m->interface    = i;
    m->type         = type;
    m->name         = (size >= 0) ? adbusI_strndup(name, size) : adbusI_strdup(name);
    m->annotations  = kh_new(StringPair);
    m->inArguments  = kv_new(Argument);
    m->outArguments = kv_new(Argument);

    int ret;
    khiter_t ki = kh_put(MemberPtr, i->members, m->name, &ret);
    if (!ret) {
        FreeMember(kh_val(i->members, ki));
    }

    kh_key(i->members, ki) = m->name;
    kh_val(i->members, ki) = m;

    return m;
}

adbus_Member* adbus_iface_addmethod(adbus_Interface* i, const char* name, int size)
{ return AddMethod(i, ADBUSI_METHOD, name, size); }

adbus_Member* adbus_iface_addsignal(adbus_Interface* i, const char* name, int size)
{ return AddMethod(i, ADBUSI_SIGNAL, name, size); }

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

// ----------------------------------------------------------------------------

static void FreeMember(adbus_Member* m)
{
    if (!m)
        return;

    for (khiter_t ii = kh_begin(m->annotations); ii != kh_end(m->annotations); ++ii) {
        if (kh_exist(m->annotations, ii)) {
            free((char*) kh_key(m->annotations, ii));
            free(kh_val(m->annotations, ii));
        }
    }
    kh_free(StringPair, m->annotations);

    for (size_t i = 0; i < kv_size(m->inArguments); ++i) {
        free(kv_a(m->inArguments, i).name);
        free(kv_a(m->inArguments, i).type);
    }
    kv_free(Argument, m->inArguments);

    for (size_t i = 0; i < kv_size(m->outArguments); ++i) {
        free(kv_a(m->outArguments, i).name);
        free(kv_a(m->outArguments, i).type);
    }
    kv_free(Argument, m->outArguments);

    free(m->propertyType);
    free(m->name);

    adbus_user_free(m->methodData);
    adbus_user_free(m->getPropertyData);
    adbus_user_free(m->setPropertyData);

    free(m);
}

// ----------------------------------------------------------------------------

static adbus_Member* GetMethod(
        adbus_Interface*      i,
        adbusI_MemberType        type,
        const char*                 name,
        int                         size)
{
    // We need to guarantee name is nul terminated
    if (size >= 0)
        name = adbusI_strndup(name, size);

    khiter_t ii = kh_get(MemberPtr, i->members, name);

    if (size >= 0)
        free((char*) name);

    if (ii == kh_end(i->members))
        return NULL;

    adbus_Member* m = kh_val(i->members, ii);
    if (m->type == type)
        return m;
    else
        return NULL;
}

adbus_Member* adbus_iface_method(adbus_Interface* i, const char* name, int size)
{ return GetMethod(i, ADBUSI_METHOD, name, size); }

adbus_Member* adbus_iface_signal(adbus_Interface* i, const char* name, int size)
{ return GetMethod(i, ADBUSI_SIGNAL, name, size); }

adbus_Member* adbus_iface_property(adbus_Interface* i, const char* name, int size)
{ return GetMethod(i, ADBUSI_PROPERTY, name, size); }

// ----------------------------------------------------------------------------
// Member management
// ----------------------------------------------------------------------------

static void AddArgument(
        kvector_t(Argument)*        args,
        const char*                 name,
        int                         nameSize,
        const char*                 type,
        int                         typeSize)
{
    if (name && nameSize < 0)
        nameSize = strlen(name);

    if (typeSize < 0)
        typeSize = strlen(type);

    struct Argument* a = kv_push(Argument, args, 1);
    if (name && nameSize > 0)
        a->name = adbusI_strndup(name, nameSize);
    a->type = adbusI_strndup(type, typeSize);
}

// ----------------------------------------------------------------------------

void adbus_mbr_addargument(
        adbus_Member*         m,
        const char*                 name,
        int                         nameSize,
        const char*                 type,
        int                         typeSize)
{
    if (m->type == ADBUSI_METHOD)
        AddArgument(m->inArguments, name, nameSize, type, typeSize);
    else if (m->type == ADBUSI_SIGNAL)
        AddArgument(m->outArguments, name, nameSize, type, typeSize);
}

// ----------------------------------------------------------------------------

void adbus_mbr_addreturn(
        adbus_Member*         m,
        const char*                 name,
        int                         nameSize,
        const char*                 type,
        int                         typeSize)
{
    if (m->type == ADBUSI_METHOD)
        AddArgument(m->outArguments, name, nameSize, type, typeSize);
}

// ----------------------------------------------------------------------------

void adbus_mbr_addannotation(
        adbus_Member* m,
        const char*         name,
        int                 nameSize,
        const char*         value,
        int                 valueSize)
{
    name = (nameSize >= 0)
         ? adbusI_strndup(name, nameSize)
         : adbusI_strdup(name);

    value = (valueSize >= 0)
          ? adbusI_strndup(value, valueSize)
          : adbusI_strdup(value);

    int added;
    khiter_t ki = kh_put(StringPair, m->annotations, name, &added);
    if (!added) {
        free((char*) kh_key(m->annotations, ki));
        free(kh_val(m->annotations, ki));
    }

    kh_key(m->annotations, ki) = name;
    kh_val(m->annotations, ki) = (char*) value;
}

// ----------------------------------------------------------------------------

void adbus_mbr_setmethod(
        adbus_Member*         m,
        adbus_Callback        callback,
        adbus_User*           user)
{
    adbus_user_free(m->methodData);
    m->methodCallback = callback;
    m->methodData     = user;
}

// ----------------------------------------------------------------------------

void adbus_mbr_setgetter(
        adbus_Member*         m,
        adbus_Callback        callback,
        adbus_User*           user)
{
    adbus_user_free(m->getPropertyData);
    m->getPropertyCallback = callback;
    m->getPropertyData     = user;
}

// ----------------------------------------------------------------------------

void adbus_mbr_setsetter(
        adbus_Member*         m,
        adbus_Callback        callback,
        adbus_User*           user)
{
    adbus_user_free(m->setPropertyData);
    m->setPropertyCallback = callback;
    m->setPropertyData     = user;
}







// ----------------------------------------------------------------------------
// Introspection callback (private)
// ----------------------------------------------------------------------------

static void IntrospectArguments(adbus_Member* m, kstring_t* out)
{
    for (size_t i = 0; i < kv_size(m->inArguments); ++i)
    {
        struct Argument* a = &kv_a(m->inArguments, i);
        ks_cat(out, "\t\t\t<arg type=\"");
        ks_cat(out, a->type);
        if (a->name)
        {
            ks_cat(out, "\" name=\"");
            ks_cat(out, a->name);
        }
        ks_cat(out, "\" direction=\"in\"/>\n");
    }

    for (size_t j = 0; j < kv_size(m->outArguments); ++j)
    {
        struct Argument* a = &kv_a(m->outArguments, j);
        ks_cat(out, "\t\t\t<arg type=\"");
        ks_cat(out, a->type);
        if (a->name)
        {
            ks_cat(out, "\" name=\"");
            ks_cat(out, a->name);
        }
        ks_cat(out, "\" direction=\"out\"/>\n");
    }

}

// ----------------------------------------------------------------------------

static void IntrospectAnnotations(adbus_Member* m, kstring_t* out)
{
    for (khiter_t ai = kh_begin(m->annotations); ai != kh_end(m->annotations); ++ai) {
        if (kh_exist(m->annotations, ai)) {
            ks_cat(out, "\t\t\t<annotation name=\"");
            ks_cat(out, kh_key(m->annotations, ai));
            ks_cat(out, "\" value=\"");
            ks_cat(out, kh_val(m->annotations, ai));
            ks_cat(out, "\"/>\n");
        }
    }
}
// ----------------------------------------------------------------------------

static void IntrospectMember(adbus_Member* m, kstring_t* out)
{
    switch (m->type)
    {
        case ADBUSI_PROPERTY:
            {
                ks_cat(out, "\t\t<property name=\"");
                ks_cat(out, m->name);
                ks_cat(out, "\" type=\"");
                ks_cat(out, m->propertyType);
                ks_cat(out, "\" access=\"");
                adbus_Bool read  = (m->getPropertyCallback != NULL);
                adbus_Bool write = (m->setPropertyCallback != NULL);
                if (read && write) {
                    ks_cat(out, "readwrite");
                } else if (read) {
                    ks_cat(out, "read");
                } else if (write) {
                    ks_cat(out, "write");
                } else {
                    assert(0);
                }

                if (kh_size(m->annotations) == 0)
                {
                    ks_cat(out, "\"/>\n");
                }
                else
                {
                    ks_cat(out, "\">\n");
                    IntrospectAnnotations(m, out);
                    ks_cat(out, "\t\t</property>\n");
                }
            }
            break;
        case ADBUSI_METHOD:
            {
                ks_cat(out, "\t\t<method name=\"");
                ks_cat(out, m->name);
                ks_cat(out, "\">\n");

                IntrospectAnnotations(m, out);
                IntrospectArguments(m, out);

                ks_cat(out, "\t\t</method>\n");
            }
            break;
        case ADBUSI_SIGNAL:
            {
                ks_cat(out, "\t\t<signal name=\"");
                ks_cat(out, m->name);
                ks_cat(out, "\">\n");

                IntrospectAnnotations(m, out);
                IntrospectArguments(m, out);

                ks_cat(out, "\t\t</signal>\n");
            }
            break;
        default:
            assert(0);
            break;
    }
}

// ----------------------------------------------------------------------------

static void IntrospectInterfaces(struct ObjectPath* p, kstring_t* out)
{
    for (khiter_t bi = kh_begin(p->interfaces); bi != kh_end(p->interfaces); ++bi) {
        if (kh_exist(p->interfaces, bi)) {
            adbus_Interface* i = kh_val(p->interfaces, bi).interface;
            ks_cat(out, "\t<interface name=\"");
            ks_cat(out, i->name);
            ks_cat(out, "\">\n");

            for (khiter_t mi = kh_begin(i->members); mi != kh_end(i->members); ++mi) {
                if (kh_exist(i->members, mi)) {
                    IntrospectMember(kh_val(i->members, mi), out);
                }
            }

            ks_cat(out, "\t</interface>\n");
        }
    }
}

// ----------------------------------------------------------------------------

static void IntrospectNode(struct ObjectPath* p, kstring_t* out)
{
    ks_cat(out,
           "<!DOCTYPE node PUBLIC \"-//freedesktop/DTD D-BUS Object Introspection 1.0//EN\"\n"
           "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
           "<node>\n");

    IntrospectInterfaces(p, out);

    size_t namelen = p->h.size;
    // Now add the child objects
    for (size_t i = 0; i < kv_size(p->children); ++i) {
        // Find the child tail ie ("bar" for "/foo/bar" or "foo" for "/foo")
        const char* child = kv_a(p->children, i)->h.string;
        child += namelen;
        if (namelen > 1)
            child += 1; // +1 for '/' when p is not the root node

        ks_cat(out, "\t<node name=\"");
        ks_cat(out, child);
        ks_cat(out, "\"/>\n");
    }

    ks_cat(out, "</node>\n");
}

// ----------------------------------------------------------------------------

int adbusI_introspect(adbus_CbData* d)
{
    adbus_check_end(d);

    // If no reply is wanted, we're done
    if (!d->retmessage) {
        return 0;
    }

    struct ObjectPath* p = (struct ObjectPath*) adbusI_puser_get(d->user2);

    kstring_t* out = ks_new();
    IntrospectNode(p, out);

    adbus_buf_append(d->retargs, "s", -1);
    adbus_buf_string(d->retargs, ks_cstr(out), ks_size(out));

    ks_free(out);

    return 0;
}






// ----------------------------------------------------------------------------
// Properties callbacks (private)
// ----------------------------------------------------------------------------

int adbusI_getProperty(adbus_CbData* d)
{
    adbus_Path* path = (adbus_Path*) adbusI_puser_get(d->user2);

    // Unpack the message
    const char* iname  = adbus_check_string(d, NULL);
    const char* mname  = adbus_check_string(d, NULL);
    adbus_check_end(d);

    // Get the interface
    adbus_Interface* interface = adbus_path_interface(
            path,
            iname,
            -1,
            &d->user2);

    if (!interface) {
        return adbusI_interfaceError(d);
    }

    // Get the property
    adbus_Member* mbr = adbus_iface_property(interface, mname, -1);

    if (!mbr) {
        return adbusI_propertyError(d);
    }

    // Check that we can read the property
    adbus_Callback callback = mbr->getPropertyCallback;
    if (!callback) {
        return PropReadError(d);
    }

    // If no reply is wanted we are finished
    if (!d->retmessage)
        return 0;

    // Get the property value

    adbus_buf_append(d->retargs, "v", -1);
    adbus_buf_beginvariant(d->retargs, mbr->propertyType, -1);

    d->propertyMarshaller = d->retargs;
    d->user1 = mbr->getPropertyData;
    int err = callback(d);
    if (err || adbus_msg_type(d->retmessage) == ADBUS_MSG_ERROR)
        return err;

    adbus_buf_endvariant(d->retargs);

    return err;
}

// ----------------------------------------------------------------------------

int adbusI_getAllProperties(adbus_CbData* d)
{
    adbus_Path* path = (adbus_Path*) adbusI_puser_get(d->user2);

    // Unpack the message
    const char* iname = adbus_check_string(d, NULL);
    adbus_check_end(d);

    // Get the interface
    adbus_Interface* interface = adbus_path_interface(
            path,
            iname,
            -1,
            &d->user2);

    if (!interface) {
        return adbusI_interfaceError(d);
    }

    // If no reply is wanted we are finished
    if (!d->retmessage)
        return 0;

    // Iterate over the properties and marshall up the values
    adbus_buf_append(d->retargs, "a{sv}", -1);
    adbus_buf_beginmap(d->retargs);

    khash_t(MemberPtr)* mbrs = interface->members;
    for (khiter_t mi = kh_begin(mbrs); mi != kh_end(mbrs); ++mi) {
        if (kh_exist(mbrs, mi)) {
            adbus_Member* mbr = kh_val(mbrs, mi);
            // Check that it is a property
            if (mbr->type != ADBUSI_PROPERTY) {
                continue;
            }

            // Check that we can read the property
            adbus_Callback callback = mbr->getPropertyCallback;
            if (!callback) {
                continue;
            }

            // Set the property entry
            adbus_buf_string(d->retargs, mbr->name, -1);
            adbus_buf_beginvariant(d->retargs, mbr->propertyType, -1);

            d->user1 = mbr->getPropertyData;
            d->propertyMarshaller = d->retargs;
            int err = callback(d);
            if (err || adbus_msg_type(d->retmessage) == ADBUS_MSG_ERROR)
                return err;

            adbus_buf_endvariant(d->retargs);
        }
    }

    adbus_buf_endmap(d->retargs);
    return 0;
}

// ----------------------------------------------------------------------------

int adbusI_setProperty(adbus_CbData* d)
{
    adbus_Path* path = (adbus_Path*) adbusI_puser_get(d->user2);

    // Unpack the message
    const char* iname = adbus_check_string(d, NULL);
    const char* mname = adbus_check_string(d, NULL);

    // Get the interface
    adbus_Interface* interface = adbus_path_interface(
            path,
            iname,
            -1,
            &d->user2);

    if (!interface) {
        return adbusI_interfaceError(d);
    }

    // Get the property
    adbus_Member* mbr = adbus_iface_property(interface, mname, -1);

    if (!mbr) {
        return adbusI_propertyError(d);
    }

    // Check that we can write the property
    adbus_Callback callback = mbr->setPropertyCallback;
    if (!callback) {
        return PropWriteError(d);
    }

    // Get the property type
    const char* sig = adbus_check_variantbegin(d, NULL);

    // Check the property type
    if (strcmp(mbr->propertyType, sig) != 0) {
        return PropTypeError(d);
    }

    // Set the property
    d->user1 = mbr->setPropertyData;
    d->propertyIterator = d->args;
    return callback(d);
}





