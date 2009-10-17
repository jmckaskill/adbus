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
#include "Interface_p.h"
#include "Misc_p.h"
#include "vector.h"

#include <assert.h>
#include <string.h>

#ifdef WIN32
#   define strdup _strdup
#endif

// ----------------------------------------------------------------------------
// Interface management
// ----------------------------------------------------------------------------

struct ADBusInterface* ADBusCreateInterface(
        const char*     name,
        int             size)
{
    struct ADBusInterface* i = NEW(struct ADBusInterface);

    i->name = (size >= 0)
            ? strndup(name, size)
            : strdup(name);
    i->members = kh_init(ADBusMemberPtr);

    return i;  
}

// ----------------------------------------------------------------------------

static void FreeMember(struct ADBusMember* member);

void ADBusFreeInterface(struct ADBusInterface* i)
{
    if (!i)
        return;

    for (khiter_t ii = kh_begin(i->members); ii != kh_end(i->members); ++ii) {
        if (kh_exist(i->members, ii))
            FreeMember(kh_val(i->members, ii));
    }
    kh_destroy(ADBusMemberPtr, i->members);

    free(i);
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
    m->name         = (size >= 0) ? strndup(name, size) : strdup(name);
    m->annotations  = kh_init(StringPair);

    int ret;
    khiter_t ki = kh_put(ADBusMemberPtr, i->members, m->name, &ret);
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
    kh_destroy(StringPair, m->annotations);

    for (size_t i = 0; i < arg_vector_size(&m->inArguments); ++i) {
        free(m->inArguments[i].name);
        free(m->inArguments[i].type);
    }

    for (size_t i = 0; i < arg_vector_size(&m->outArguments); ++i) {
        free(m->outArguments[i].name);
        free(m->outArguments[i].type);
    }

    arg_vector_free(&m->inArguments);
    arg_vector_free(&m->outArguments);

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
        name = strndup(name, size);

    khiter_t ii = kh_get(ADBusMemberPtr, i->members, name);

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

void ADBusAddArgument(struct ADBusMember* m,
        enum ADBusArgumentDirection direction,
        const char* name, int nameSize,
        const char* type, int typeSize)
{
    assert(type);

    if (name && nameSize < 0)
        nameSize = strlen(name);

    if (typeSize < 0)
        typeSize = strlen(type);

    struct Argument** pvec = (direction == ADBusInArgument)
                           ? &m->inArguments
                           : &m->outArguments;

    struct Argument* a = arg_vector_insert_end(pvec, 1);
    if (name && nameSize > 0)
        a->name = strndup(name, nameSize);
    a->type = strndup(type, typeSize);
}

// ----------------------------------------------------------------------------

void ADBusAddAnnotation(struct ADBusMember* m,
        const char* name, int nameSize,
        const char* value, int valueSize)
{
    assert(name && value);

    name = (nameSize >= 0)
         ? strndup(name, nameSize)
         : strdup(name);

    value = (valueSize >= 0)
          ? strndup(value, valueSize)
          : strdup(value);

    int ret;
    khiter_t ki = kh_put(StringPair, m->annotations, name, &ret);
    if (!ret) {
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
                    ? strndup(type, typeSize)
                    : strdup(type);
}

// ----------------------------------------------------------------------------

const char* ADBusPropertyType(
        struct ADBusMember*         m,
        size_t*                     size)
{
    if (size)
        *size = m->propertyType ? strlen(m->propertyType) : 0;
    return m->propertyType;
}

// ----------------------------------------------------------------------------

uint ADBusIsPropertyReadable(
        struct ADBusMember*         m)
{
    return (m->getPropertyCallback) ? 1 : 0;
}

// ----------------------------------------------------------------------------

uint ADBusIsPropertyWritable(
        struct ADBusMember*         m)
{
    return (m->setPropertyCallback) ? 1 : 0;
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

void ADBusCallMethod(
        struct ADBusMember*         m,
        struct ADBusCallDetails*    details)
{
    details->user1 = m->methodData;
    if (m->methodCallback)
        m->methodCallback(details);
}

// ----------------------------------------------------------------------------

void ADBusCallSetProperty(
        struct ADBusMember*         m,
        struct ADBusCallDetails*    details)
{
    details->user1 = m->setPropertyData;
    if (m->setPropertyCallback)
        m->setPropertyCallback(details);
}

// ----------------------------------------------------------------------------

void ADBusCallGetProperty(
        struct ADBusMember*         m,
        struct ADBusCallDetails*    details)
{
    details->user1 = m->getPropertyData;
    if (m->getPropertyCallback)
        m->getPropertyCallback(details);
}




