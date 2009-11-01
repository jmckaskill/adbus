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

#include "Misc_p.h"

#include "CommonMessages.h"
#include "Marshaller.h"
#include "Match.h"
#include "Message.h"
#include "User.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#   include <windows.h>
#   include <crtdbg.h>
#else
#   include <sys/time.h>
#endif

// ----------------------------------------------------------------------------

uint RequiresServiceLookup(const char* name, int size)
{
    if (size < 0)
        size = strlen(name);

    return size > 0
        && *name != ':'
        && (strncmp(name, "org.freedesktop.DBus", size) != 0);
}

// ----------------------------------------------------------------------------

#ifdef WIN32
uint64_t TimerBegin()
{
    LARGE_INTEGER freq, begin;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&begin);
    return (begin.QuadPart * 1000000) / freq.QuadPart;
}
#else
uint64_t TimerBegin()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (((uint64_t) tv.tv_sec) * 1000000) + tv.tv_usec;
}
#endif

// ----------------------------------------------------------------------------

#ifdef WIN32
void TimerEnd(uint64_t begin, const char* what)
{
    uint64_t end = TimerBegin();
    _CrtDbgReport(_CRT_WARN, NULL, -1, "", "%s %u us\n", what, (uint) (end - begin));
}
#else
void TimerEnd(uint64_t begin, const char* what)
{
    uint64_t end = TimerBegin();
    fprintf(stderr, "%s %u us\n", what, (uint) (end - begin));
}
#endif

// ----------------------------------------------------------------------------

static const char sRequiredAlignment[256] =
{
    /*  0 \0*/ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /* 10 \n*/ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /* 20 SP*/ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /* 30 RS*/ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /* 40 ( */ 8, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /* 50 2 */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /* 60 < */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /* 70 F */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /* 80 P */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /* 90 Z */ 0, 0, 0, 0, 0,    0, 0, 4, 4, 0,
    /*100 d */ 8, 0, 0, 1, 0,    4, 0, 0, 0, 0,
    /*110 n */ 2, 4, 0, 2, 0,    4, 8, 4, 1, 0,
    /*120 x */ 8, 1, 0, 8, 0,    0, 0, 0, 0, 0,
    /*130   */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /*140   */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /*150   */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /*160   */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /*170   */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /*180   */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /*190   */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /*200   */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /*210   */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /*220   */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /*230   */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /*240   */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
    /*250   */ 0, 0, 0, 0, 0,    0
};


int RequiredAlignment(char ch)
{
    assert(sRequiredAlignment[(int)ch] > 0);
    return sRequiredAlignment[ch & 0x7F];
}

// ----------------------------------------------------------------------------

#ifdef ADBUS_LITTLE_ENDIAN
const char NativeEndianness = 'l';
#elif defined(ADBUS_BIG_ENDIAN)
const char NativeEndianness = 'B';
#else
#error Please define ADBUS_LITTLE_ENDIAN or ADBUS_BIG_ENDIAN
#endif

// ----------------------------------------------------------------------------

const uint8_t MajorProtocolVersion = 1;

// ----------------------------------------------------------------------------

uint IsValidObjectPath(const char* str, size_t len)
{
    if (!str || len == 0)
        return 0;
    if (str[0] != '/')
        return 0;
    if (len > 1 && str[len - 1] == '/')
        return 0;

    // Now we check for consecutive '/' and that
    // the path components only use [A-Z][a-z][0-9]_
    const char* slash = str;
    const char* cur = str;
    while (++cur < str + len)
    {
        if (*cur == '/')
        {
            if (cur - slash == 1)
                return 0;
            slash = cur;
            continue;
        }
        else if ( ('a' <= *cur && *cur <= 'z')
                || ('A' <= *cur && *cur <= 'Z')
                || ('0' <= *cur && *cur <= '9')
                || *cur == '_')
        {
            continue;
        }
        else
        {
            return 0;
        }
    }
    return 1;
}

// ----------------------------------------------------------------------------

uint IsValidInterfaceName(const char* str, size_t len)
{
    if (!str || len == 0 || len > 255)
        return 0;

    // Must not begin with a digit
    if (!(  ('a' <= str[0] && str[0] <= 'z')
                || ('A' <= str[0] && str[0] <= 'Z')
                || (str[0] == '_')))
    {
        return 0;
    }

    // Now we check for consecutive '.' and that
    // the components only use [A-Z][a-z][0-9]_
    const char* dot = str - 1;
    const char* cur = str;
    while (++cur < str + len)
    {
        if (*cur == '.')
        {
            if (cur - dot == 1)
                return 0;
            dot = cur;
            continue;
        }
        else if ( ('a' <= *cur && *cur <= 'z')
                || ('A' <= *cur && *cur <= 'Z')
                || ('0' <= *cur && *cur <= '9')
                || (*cur == '_'))
        {
            continue;
        }
        else
        {
            return 0;
        }
    }
    // Interface names must include at least one '.'
    if (dot == str - 1)
        return 0;

    return 1;
}

// ----------------------------------------------------------------------------

uint IsValidBusName(const char* str, size_t len)
{
    if (!str || len == 0 || len > 255)
        return 0;

    // Bus name must either begin with : or not a digit
    if (!(  (str[0] == ':')
                || ('a' <= str[0] && str[0] <= 'z')
                || ('A' <= str[0] && str[0] <= 'Z')
                || (str[0] == '_')
                || (str[0] == '-')))
    {
        return 0;
    }

    // Now we check for consecutive '.' and that
    // the components only use [A-Z][a-z][0-9]_
    const char* dot = str[0] == ':' ? str : str - 1;
    const char* cur = str;
    while (++cur < str + len)
    {
        if (*cur == '.')
        {
            if (cur - dot == 1)
                return 0;
            dot = cur;
            continue;
        }
        else if ( ('a' <= *cur && *cur <= 'z')
                || ('A' <= *cur && *cur <= 'Z')
                || ('0' <= *cur && *cur <= '9')
                || (*cur == '_')
                || (*cur == '-'))
        {
            continue;
        }
        else
        {
            return 0;
        }
    }
    // Bus names must include at least one '.'
    if (dot == str - 1)
        return 0;

    return 1;
}

// ----------------------------------------------------------------------------

uint IsValidMemberName(const char* str, size_t len)
{
    if (!str || len == 0 || len > 255)
        return 0;

    // Must not begin with a digit
    if (!(   ('a' <= str[0] && str[0] <= 'z')
          || ('A' <= str[0] && str[0] <= 'Z')
          || (str[0] == '_')))
    {
        return 0;
    }

    // We now check that we only use [A-Z][a-z][0-9]_
    const char* cur = str;
    while (++cur < str + len)
    {
        if ( ('a' <= *cur && *cur <= 'z')
          || ('A' <= *cur && *cur <= 'Z')
          || ('0' <= *cur && *cur <= '9')
          || (*cur == '_'))
        {
            continue;
        }
        else
        {
            return 0;
        }
    }

    return 1;
}

// ----------------------------------------------------------------------------

uint HasNullByte(const uint8_t* str, size_t len)
{
    return memchr(str, '\0', len) != NULL;
}

// ----------------------------------------------------------------------------

uint IsValidUtf8(const uint8_t* str, size_t len)
{
    if (!str)
        return 1;
    const uint8_t* s = str;
    const uint8_t* end = str + len;
    while (s < end)
    {
        if (s[0] < 0x80)
        {
            // 1 byte sequence (US-ASCII)
            s += 1;
        }
        else if (s[0] < 0xC0)
        {
            // Multi-byte data without start
            return 0;
        }
        else if (s[0] < 0xE0)
        {
            // 2 byte sequence
            if (end - s < 2)
                return 0;

            // Check the continuation bytes
            if (!(0x80 <= s[1] && s[1] < 0xC0))
                return 0;

            // Check for overlong encoding
            // 0x80 -> 0xC2 0x80
            if (s[0] < 0xC2)
                return 0;

            s += 2;
        }
        else if (s[0] < 0xF0)
        {
            // 3 byte sequence
            if (end - s < 3)
                return 0;

            // Check the continuation bytes
            if (!(  0x80 <= s[1] && s[1] < 0xC0
                        && 0x80 <= s[2] && s[2] < 0xC0))
                return 0;

            // Check for overlong encoding
            // 0x08 00 -> 0xE0 A0 90
            if (s[0] == 0xE0 && s[1] < 0xA0)
                return 0;

            // Code points [0xD800, 0xE000) are invalid
            // (UTF16 surrogate characters)
            // 0xD8 00 -> 0xED A0 80
            // 0xDF FF -> 0xED BF BF
            // 0xE0 00 -> 0xEE 80 80
            if (s[0] == 0xED && s[1] >= 0xA0)
                return 0;

            s += 3;
        }
        else if (s[0] < 0xF5)
        {
            // 4 byte sequence
            if (end - s < 4)
                return 0;

            // Check the continuation bytes
            if (!(  0x80 <= s[1] && s[1] < 0xC0
                        && 0x80 <= s[2] && s[2] < 0xC0
                        && 0x80 <= s[3] && s[3] < 0xC0))
                return 0;

            // Check for overlong encoding
            // 0x01 00 00 -> 0xF0 90 80 80
            if (s[0] == 0xF0 && s[1] < 0x90)
                return 0;

            s += 4;
        }
        else
        {
            // 4 byte above 0x10FFFF, 5 byte, and 6 byte sequences
            // restricted by RFC 3639
            // 0xFE-0xFF invalid
            return 0;
        }
    }

    return s == end ? 1 : 0;
}

// ----------------------------------------------------------------------------

const char* FindArrayEnd(const char* arrayBegin)
{
    int dictEntries = 0;
    int structs = 0;
    while (arrayBegin && *arrayBegin)
    {
        switch(*arrayBegin)
        {
        case ADBusUInt8Field:
        case ADBusBooleanField:
        case ADBusInt16Field:
        case ADBusUInt16Field:
        case ADBusInt32Field:
        case ADBusUInt32Field:
        case ADBusInt64Field:
        case ADBusUInt64Field:
        case ADBusDoubleField:
        case ADBusStringField:
        case ADBusObjectPathField:
        case ADBusSignatureField:
        case ADBusVariantBeginField:
            break;
        case ADBusArrayBeginField:
            arrayBegin++;
            arrayBegin = FindArrayEnd(arrayBegin);
            break;
        case ADBusStructBeginField:
            structs++;
            break;
        case ADBusStructEndField:
            structs--;
            break;
        case ADBusDictEntryBeginField:
            dictEntries++;
            break;
        case ADBusDictEntryEndField:
            dictEntries--;
            break;
        default:
            assert(0);
            break;
        }
        arrayBegin++;
    }
    return (structs == 0 && dictEntries == 0) ? arrayBegin : NULL;
}

// ----------------------------------------------------------------------------

struct UserPointer
{
    struct ADBusUser header;
    void* pointer;
};

static void FreeUserPointer(struct ADBusUser* data)
{
    free(data);
}

struct ADBusUser* CreateUserPointer(void* p)
{
    struct UserPointer* u = NEW(struct UserPointer);
    u->header.free = &FreeUserPointer;
    u->pointer = p;
    return &u->header;
}

void* GetUserPointer(const struct ADBusUser* data)
{
    return ((struct UserPointer*) data)->pointer;
}

// ----------------------------------------------------------------------------

void SanitisePath(
        kstring_t*  out,
        const char* path1,
        int         size1,
        const char* path2,
        int         size2)
{
    ks_clear(out);
    if (size1 < 0)
        size1 = strlen(path1);
    if (size2 < 0)
        size2 = strlen(path2);

    // Make sure it starts with a /
    if (size1 > 0 && path1[0] != '/')
        ks_cat(out, "/");
    if (size1 > 0)
        ks_cat_n(out, path1, size1);

    // Make sure it has a / seperator
    if (size2 > 0 && path2[0] != '/')
        ks_cat(out, "/");
    if (size2 > 0)
        ks_cat_n(out, path2, size2);

    // Remove repeating slashes
    for (size_t i = 1; i < ks_size(out); ++i) {
        if (ks_a(out, i) == '/' && ks_a(out, i-1) == '/') {
            ks_remove(out, i, 1);
        }
    }

    // Remove trailing /
    if (ks_size(out) > 1 && ks_a(out, ks_size(out) - 1) == '/')
        ks_remove_end(out, 1);
}

// ----------------------------------------------------------------------------

void ParentPath(
        char*   path,
        size_t  size,
        size_t* parentSize)
{
    // Path should have already been sanitised so double /'s should not happen
#ifndef NDEBUG
    kstring_t* sanitised = ks_new();
    SanitisePath(sanitised, path, size, NULL, 0);
    assert(path[size] == '\0');
    assert(ks_cmp(sanitised, path) == 0);
    ks_free(sanitised);
#endif

    --size;

    while (size > 1 && path[size] != '/') {
        --size;
    }

    path[size] = '\0';

    if (parentSize)
        *parentSize = size;
}

// ----------------------------------------------------------------------------

static void Append(kstring_t* s, const char* key, const char* value, int vsize)
{
    if (value) {
        ks_cat(s, key);
        ks_cat(s, "='");
        if (vsize >= 0)
            ks_cat_n(s, value, vsize);
        else
            ks_cat(s, value);
        ks_cat(s, "',");
    }
}

void AppendMatchString(
        struct ADBusMarshaller* mar,
        const struct ADBusMatch* m)
{
    kstring_t* mstr = ks_new();
    if (m->type == ADBusMethodCallMessage) {
        ks_cat(mstr, "type='method_call',");
    } else if (m->type == ADBusMethodReturnMessage) {
        ks_cat(mstr, "type='method_return',");
    } else if (m->type == ADBusErrorMessage) {
        ks_cat(mstr, "type='error',");
    } else if (m->type == ADBusSignalMessage) {
        ks_cat(mstr, "type='signal',");
    }

    // We only want to add the sender field if we are not going to have to do
    // a lookup conversion
    if (m->sender
     && !RequiresServiceLookup(m->sender, m->senderSize))
    {
        Append(mstr, "sender", m->sender, m->senderSize);
    }
    Append(mstr, "interface", m->interface, m->interfaceSize);
    Append(mstr, "member", m->member, m->memberSize);
    Append(mstr, "path", m->path, m->pathSize);
    Append(mstr, "destination", m->destination, m->destinationSize);

    for (size_t i = 0; i < m->argumentsSize; ++i) {
        struct ADBusMatchArgument* arg = &m->arguments[i];
        ks_printf(mstr, "arg%d='", arg->number);
        if (arg->valueSize < 0) {
            ks_cat(mstr, arg->value);
        } else {
            ks_cat_n(mstr, arg->value, arg->valueSize);
        }
        ks_cat(mstr, "',");
    }

    // Remove the trailing ','
    if (ks_size(mstr) > 0)
        ks_remove_end(mstr, 1);

    ADBusAppendArguments(mar, "s", -1);
    ADBusAppendString(mar, ks_cstr(mstr), (int) ks_size(mstr));
    ks_free(mstr);
}

// ----------------------------------------------------------------------------

int InvalidArgumentError(struct ADBusCallDetails* d)
{
    return ADBusError(
            d,
            "nz.co.foobar.ADBus.Error.InvalidArgument",
            "Invalid argument to the method '%s.%s' on %s",
            ADBusGetInterface(d->message, NULL),
            ADBusGetMember(d->message, NULL),
            ADBusGetPath(d->message, NULL));
}

int InvalidPathError(struct ADBusCallDetails* d)
{
    return ADBusError(
            d,
            "nz.co.foobar.ADBus.Error.InvalidPath",
            "The path '%s' does not exist.",
            ADBusGetPath(d->message, NULL));
}

int InvalidInterfaceError(struct ADBusCallDetails* d)
{
    return ADBusError(
            d,
            "nz.co.foobar.ADBus.Error.InvalidInterface",
            "The path '%s' does not export the interface '%s'.",
            ADBusGetPath(d->message, NULL),
            ADBusGetInterface(d->message, NULL),
            ADBusGetMember(d->message, NULL));
}

int InvalidMethodError(struct ADBusCallDetails* d)
{
    if (ADBusGetInterface(d->message, NULL)) {
        return ADBusError(
                d,
                "nz.co.foobar.ADBus.Error.InvalidMethod",
                "The path '%s' does not export the method '%s.%s'.",
                ADBusGetPath(d->message, NULL),
                ADBusGetInterface(d->message, NULL),
                ADBusGetMember(d->message, NULL));
    } else {
        return ADBusError(
                d,
                "nz.co.foobar.ADBus.Error.InvalidMethod",
                "The path '%s' does not export the method '%s'.",
                ADBusGetPath(d->message, NULL),
                ADBusGetMember(d->message, NULL));
    }
}

int InvalidPropertyError(struct ADBusCallDetails* d)
{
    return ADBusError(
            d,
            "nz.co.foobar.ADBus.Error.InvalidProperty",
            "The path '%s' does not export the property '%s.%s'.",
            ADBusGetPath(d->message, NULL),
            ADBusGetInterface(d->message, NULL),
            ADBusGetMember(d->message, NULL));
}

int PropWriteError(struct ADBusCallDetails* d)
{
    return ADBusError(
            d,
            "nz.co.foobar.ADBus.Error.ReadOnlyProperty",
            "The property '%s.%s' on '%s' is read only.",
            ADBusGetInterface(d->message, NULL),
            ADBusGetMember(d->message, NULL),
            ADBusGetPath(d->message, NULL));
}

int PropReadError(struct ADBusCallDetails* d)
{
    return ADBusError(
            d,
            "nz.co.foobar.ADBus.Error.WriteOnlyProperty",
            "The property '%s.%s' on '%s' is write only.",
            ADBusGetInterface(d->message, NULL),
            ADBusGetMember(d->message, NULL),
            ADBusGetPath(d->message, NULL));
}

int PropTypeError(struct ADBusCallDetails* d)
{ 
    return ADBusError(
            d,
            "nz.co.foobar.ADBus.Error.InvalidPropertyType",
            "Incorrect property type for '%s.%s' on %s.",
            ADBusGetInterface(d->message, NULL),
            ADBusGetMember(d->message, NULL),
            ADBusGetPath(d->message, NULL));
}


