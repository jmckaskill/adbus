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

#include "Interface.h"

#include "CommonMessages.h"
#include "Connection_p.h"
#include "Interface_p.h"
#include "Iterator.h"
#include "Misc_p.h"
#include "memory/kvector.h"

#include <assert.h>
#include <string.h>

// ----------------------------------------------------------------------------
// Interface management
// ----------------------------------------------------------------------------

struct ADBusInterface* ADBusCreateInterface(
        const char*     name,
        int             size)
{
    struct ADBusInterface* i = NEW(struct ADBusInterface);

    i->name = (size >= 0)
            ? strndup_(name, size)
            : strdup_(name);
    i->members = kh_new(MemberPtr);

    return i;  
}

// ----------------------------------------------------------------------------

static void FreeMember(struct ADBusMember* member);

void ADBusFreeInterface(struct ADBusInterface* i)
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

struct ADBusMember* ADBusAddMember(
        struct ADBusInterface*  i,
        enum ADBusMemberType    type,
        const char*             name,
        int                     size)
{
    assert(name);

    struct ADBusMember* m = NEW(struct ADBusMember);
    m->interface    = i;
    m->type         = type;
    m->name         = (size >= 0) ? strndup_(name, size) : strdup_(name);
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

static void FreeMember(struct ADBusMember* m)
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

    ADBusUserFree(m->methodData);
    ADBusUserFree(m->getPropertyData);
    ADBusUserFree(m->setPropertyData);

    free(m);
}

// ----------------------------------------------------------------------------

struct ADBusMember* ADBusGetInterfaceMember(
        struct ADBusInterface*      i,
        enum ADBusMemberType        type,
        const char*                 name,
        int                         size)
{
    // We need to guarantee name is nul terminated
    if (size >= 0)
        name = strndup_(name, size);

    khiter_t ii = kh_get(MemberPtr, i->members, name);

    if (size >= 0)
        free((char*) name);

    if (ii == kh_end(i->members))
        return NULL;

    struct ADBusMember* m = kh_val(i->members, ii);
    if (m->type == type)
        return m;
    else
        return NULL;
}

// ----------------------------------------------------------------------------
// Member management
// ----------------------------------------------------------------------------

void ADBusAddArgument(
        struct ADBusMember*         m,
        enum ADBusArgumentDirection direction,
        const char*                 name,
        int                         nameSize,
        const char*                 type,
        int                         typeSize)
{
    if (name && nameSize < 0)
        nameSize = strlen(name);

    if (typeSize < 0)
        typeSize = strlen(type);

    kvector_t(Argument)* pvec = (direction == ADBusInArgument)
                              ? m->inArguments
                              : m->outArguments;

    struct Argument* a = kv_push(Argument, pvec, 1);
    if (name && nameSize > 0)
        a->name = strndup_(name, nameSize);
    a->type = strndup_(type, typeSize);
}

// ----------------------------------------------------------------------------

void ADBusAddAnnotation(
        struct ADBusMember* m,
        const char*         name,
        int                 nameSize,
        const char*         value,
        int                 valueSize)
{
    name = (nameSize >= 0)
         ? strndup_(name, nameSize)
         : strdup_(name);

    value = (valueSize >= 0)
          ? strndup_(value, valueSize)
          : strdup_(value);

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

void ADBusSetMethodCallback(
        struct ADBusMember*         m,
        ADBusMessageCallback        callback,
        struct ADBusUser*           user)
{
    ADBusUserFree(m->methodData);
    m->methodCallback = callback;
    m->methodData     = user;
}

// ----------------------------------------------------------------------------

void ADBusSetPropertyType(
        struct ADBusMember*         m,
        const char*                 type,
        int                         typeSize)
{
    m->propertyType = (typeSize >= 0)
                    ? strndup_(type, typeSize)
                    : strdup_(type);
}

// ----------------------------------------------------------------------------

void ADBusSetPropertyGetCallback(
        struct ADBusMember*         m,
        ADBusMessageCallback        callback,
        struct ADBusUser*           user)
{
    ADBusUserFree(m->getPropertyData);
    m->getPropertyCallback = callback;
    m->getPropertyData     = user;
}

// ----------------------------------------------------------------------------

void ADBusSetPropertySetCallback(
        struct ADBusMember*         m,
        ADBusMessageCallback        callback,
        struct ADBusUser*           user)
{
    ADBusUserFree(m->setPropertyData);
    m->setPropertyCallback = callback;
    m->setPropertyData     = user;
}







// ----------------------------------------------------------------------------
// Introspection callback (private)
// ----------------------------------------------------------------------------

static void IntrospectArguments(struct ADBusMember* m, kstring_t* out)
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

static void IntrospectAnnotations(struct ADBusMember* m, kstring_t* out)
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

static void IntrospectMember(struct ADBusMember* m, kstring_t* out)
{
    switch (m->type)
    {
        case ADBusPropertyMember:
            {
                ks_cat(out, "\t\t<property name=\"");
                ks_cat(out, m->name);
                ks_cat(out, "\" type=\"");
                ks_cat(out, m->propertyType);
                ks_cat(out, "\" access=\"");
                uint read  = (m->getPropertyCallback != NULL);
                uint write = (m->setPropertyCallback != NULL);
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
        case ADBusMethodMember:
            {
                ks_cat(out, "\t\t<method name=\"");
                ks_cat(out, m->name);
                ks_cat(out, "\">\n");

                IntrospectAnnotations(m, out);
                IntrospectArguments(m, out);

                ks_cat(out, "\t\t</method>\n");
            }
            break;
        case ADBusSignalMember:
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
            struct ADBusInterface* i = kh_val(p->interfaces, bi).interface;
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

    size_t namelen = p->h.pathSize;
    // Now add the child objects
    for (size_t i = 0; i < kv_size(p->children); ++i) {
        // Find the child tail ie ("bar" for "/foo/bar" or "foo" for "/foo")
        const char* child = kv_a(p->children, i)->h.path;
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

int IntrospectCallback(struct ADBusCallDetails* d)
{
    ADBusCheckMessageEnd(d);

    // If no reply is wanted, we're done
    if (!d->retmessage) {
        return 0;
    }

    struct ObjectPath* p = (struct ObjectPath*) GetUserPointer(d->user2);

    kstring_t* out = ks_new();
    IntrospectNode(p, out);

    ADBusAppendArguments(d->retargs, "s", -1);
    ADBusAppendString(d->retargs, ks_cstr(out), ks_size(out));

    ks_free(out);

    return 0;
}






// ----------------------------------------------------------------------------
// Properties callbacks (private)
// ----------------------------------------------------------------------------

int GetPropertyCallback(struct ADBusCallDetails* d)
{
    struct ADBusObjectPath* path = (struct ADBusObjectPath*) GetUserPointer(d->user2);

    // Unpack the message
    const char* iname  = ADBusCheckString(d, NULL);
    const char* mname  = ADBusCheckString(d, NULL);
    ADBusCheckMessageEnd(d);

    // Get the interface
    struct ADBusInterface* interface = ADBusGetBoundInterface(
            path,
            iname,
            -1,
            &d->user2);

    if (!interface) {
        return InvalidInterfaceError(d);
    }

    // Get the property
    struct ADBusMember* mbr = ADBusGetInterfaceMember(
            interface,
            ADBusPropertyMember,
            mname,
            -1);

    if (!mbr) {
        return InvalidPropertyError(d);
    }

    // Check that we can read the property
    ADBusMessageCallback callback = mbr->getPropertyCallback;
    if (!callback) {
        return PropReadError(d);
    }

    // If no reply is wanted we are finished
    if (!d->retmessage)
        return 0;

    // Get the property value

    ADBusAppendArguments(d->retargs, "v", -1);
    ADBusBeginVariant(d->retargs, mbr->propertyType, -1);

    d->propertyMarshaller = d->retargs;
    d->user1 = mbr->getPropertyData;
    int err = callback(d);

    ADBusEndVariant(d->retargs);

    return err;
}

// ----------------------------------------------------------------------------

int GetAllPropertiesCallback(struct ADBusCallDetails* d)
{
    struct ADBusObjectPath* path = (struct ADBusObjectPath*) GetUserPointer(d->user2);

    // Unpack the message
    const char* iname = ADBusCheckString(d, NULL);
    ADBusCheckMessageEnd(d);

    // Get the interface
    struct ADBusInterface* interface = ADBusGetBoundInterface(
            path,
            iname,
            -1,
            &d->user2);

    if (!interface) {
        return InvalidInterfaceError(d);
    }

    // If no reply is wanted we are finished
    if (!d->retmessage)
        return 0;

    // Iterate over the properties and marshall up the values
    ADBusAppendArguments(d->retargs, "a{sv}", -1);
    ADBusBeginArray(d->retargs);

    khash_t(MemberPtr)* mbrs = interface->members;
    for (khiter_t mi = kh_begin(mbrs); mi != kh_end(mbrs); ++mi) {
        if (kh_exist(mbrs, mi)) {
            struct ADBusMember* mbr = kh_val(mbrs, mi);
            // Check that it is a property
            if (mbr->type != ADBusPropertyMember) {
                continue;
            }

            // Check that we can read the property
            ADBusMessageCallback callback = mbr->getPropertyCallback;
            if (!callback) {
                continue;
            }

            // Set the property entry
            ADBusBeginDictEntry(d->retargs);
            ADBusAppendString(d->retargs, mbr->name, -1);
            ADBusBeginVariant(d->retargs, mbr->propertyType, -1);

            d->user1 = mbr->getPropertyData;
            d->propertyMarshaller = d->retargs;
            int err = callback(d);
            if (err)
                return err;

            ADBusEndVariant(d->retargs);
            ADBusEndDictEntry(d->retargs);
        }
    }

    ADBusEndArray(d->retargs);
    return 0;
}

// ----------------------------------------------------------------------------

int SetPropertyCallback(struct ADBusCallDetails* d)
{
    struct ADBusObjectPath* path = (struct ADBusObjectPath*) GetUserPointer(d->user2);

    // Unpack the message
    const char* iname = ADBusCheckString(d, NULL);
    const char* mname = ADBusCheckString(d, NULL);

    // Get the interface
    struct ADBusInterface* interface = ADBusGetBoundInterface(
            path,
            iname,
            -1,
            &d->user2);

    if (!interface) {
        return InvalidInterfaceError(d);
    }

    // Get the property
    struct ADBusMember* mbr = ADBusGetInterfaceMember(
            interface,
            ADBusPropertyMember,
            mname,
            -1);

    if (!mbr) {
        return InvalidPropertyError(d);
    }

    // Check that we can write the property
    ADBusMessageCallback callback = mbr->setPropertyCallback;
    if (!callback) {
        return PropWriteError(d);
    }

    // Get the property type
    const char* sig = ADBusCheckVariantBegin(d, NULL);

    // Check the property type
    if (strcmp(mbr->propertyType, sig) != 0) {
        return PropTypeError(d);
    }

    // Set the property
    d->user1 = mbr->setPropertyData;
    d->propertyIterator = d->args;
    return callback(d);
}





