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
#include "misc.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>





static adbus_LogCallback sLog;
void adbus_set_logger(adbus_LogCallback cb)
{ sLog = cb; }

int adbusI_log_enabled()
{ return sLog != 0; }

void adbusI_klog(d_String* log)
{ 
    if (sLog)
        sLog(ds_cstr(log), ds_size(log)); 
}

void adbusI_dolog(const char* format, ...)
{
    if (!sLog)
        return;

    d_String log;
    ZERO(&log);

    va_list ap;
    va_start(ap, format);
    ds_cat_vf(&log, format, ap);
    va_end(ap);

    adbusI_klog(&log);

    ds_free(&log);
}

#ifdef _WIN32
#   define alloca _alloca
#endif

void adbusI_addheader(d_String* str, const char* format, ...)
{
    size_t begin = ds_size(str);
    va_list ap;
    va_start(ap, format);
    ds_insert_vf(str, 0, format, ap);
    va_end(ap);

    size_t hsize = ds_size(str) - begin;
    char* spaces = (char*) alloca(hsize);
    memset(spaces, ' ', hsize);

    size_t i = 0;
    while (i < ds_size(str) - 1) { // we don't care about the last char being a '\n'
        if (ds_a(str, i) == '\n') {
            ds_insert_n(str, i + 1, spaces, hsize);
        }
        ++i;
    }
}

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


int adbusI_alignment(char ch)
{
    assert(sRequiredAlignment[(int)ch] > 0);
    return sRequiredAlignment[ch & 0x7F];
}

// ----------------------------------------------------------------------------

char adbusI_nativeEndianness(void)
{
    int i = 1;
    int little = (*(char*)&i) == 1;
    return little ? 'l' : 'B';
}

// ----------------------------------------------------------------------------

const uint8_t adbusI_majorProtocolVersion = 1;

// ----------------------------------------------------------------------------

adbus_Bool adbusI_isValidObjectPath(const char* str, size_t len)
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

adbus_Bool adbusI_isValidInterfaceName(const char* str, size_t len)
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

adbus_Bool adbusI_isValidBusName(const char* str, size_t len)
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

adbus_Bool adbusI_isValidMemberName(const char* str, size_t len)
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

adbus_Bool adbusI_hasNullByte(const char* str, size_t len)
{
    return memchr(str, '\0', len) != NULL;
}

// ----------------------------------------------------------------------------

#if 0
adbus_Bool adbusI_isValidUtf8(const char* str, size_t len)
{
    if (!str)
        return 1;
    const char* s = str;
    const char* end = str + len;
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
#endif

// ----------------------------------------------------------------------------

const char* adbus_nextarg(const char* sig)
{
    switch(*sig)
    {
        case ADBUS_BOOLEAN:
        case ADBUS_UINT8:
        case ADBUS_INT16:
        case ADBUS_UINT16:
        case ADBUS_INT32:
        case ADBUS_UINT32:
        case ADBUS_INT64:
        case ADBUS_UINT64:
        case ADBUS_DOUBLE:
        case ADBUS_STRING:
        case ADBUS_OBJECT_PATH:
        case ADBUS_SIGNATURE:
        case ADBUS_VARIANT_BEGIN:
            return sig+1;

        case ADBUS_ARRAY_BEGIN:
            return adbus_nextarg(sig + 1);

        case ADBUS_STRUCT_BEGIN:
            {
                sig++;
                while (sig && *sig != ADBUS_STRUCT_END) {
                    sig = adbus_nextarg(sig);
                }

                if (!sig || *sig != ADBUS_STRUCT_END)
                    return NULL;

                return sig + 1;
            }

        case ADBUS_DICTENTRY_BEGIN:
            {
                const char* val = adbus_nextarg(sig + 1);
                if (!val)
                    return NULL;

                const char* end = adbus_nextarg(val);
                if (!end || *end != ADBUS_DICTENTRY_END)
                    return NULL;

                return end + 1;
            }

        default:
            return NULL;
    }
}

// ----------------------------------------------------------------------------

void adbusI_relativePath(
        d_String*  out,
        const char* path1,
        int         size1,
        const char* path2,
        int         size2)
{
    ds_clear(out);
    if (size1 < 0)
        size1 = strlen(path1);
    if (size2 < 0)
        size2 = strlen(path2);

    // Make sure it starts with a /
    if (size1 > 0 && path1[0] != '/')
        ds_cat(out, "/");
    if (size1 > 0)
        ds_cat_n(out, path1, size1);

    // Make sure it has a / seperator
    if (size2 > 0 && path2[0] != '/')
        ds_cat(out, "/");
    if (size2 > 0)
        ds_cat_n(out, path2, size2);

    // Remove repeating slashes
    for (size_t i = 1; i < ds_size(out); ++i) {
        if (ds_a(out, i) == '/' && ds_a(out, i-1) == '/') {
            ds_remove(out, i, 1);
        }
    }

    // Remove trailing /
    if (ds_size(out) > 1 && ds_a(out, ds_size(out) - 1) == '/')
        ds_remove_end(out, 1);
}

// ----------------------------------------------------------------------------

void adbusI_parentPath(
        dh_strsz_t  path,
        dh_strsz_t* parent)
{
    // Path should have already been sanitised so double /'s should not happen
#ifndef NDEBUG
    d_String sanitised;
    ZERO(&sanitised);
    adbusI_relativePath(&sanitised, path.str, path.sz, NULL, 0);
    assert(ds_cmp_n(&sanitised, path.str, path.sz) == 0);
    ds_free(&sanitised);
#endif

    int size = path.sz - 1;

    // Search back until we find the first '/' that is not the last character
    while (size > 0 && path.str[size - 1] != '/') {
        --size;
    }

    if (size <= 0) {
        // Parent of "/"
        parent->str = NULL;
    } else if (size == 1) {
        // Parent of "/Foo" - "/"
        parent->str = path.str;
        parent->sz  = size;
    } else {
        // Parent of "/Foo/Bar" - "/Foo" (size is currently at "/Foo/")
        parent->str = path.str;
        parent->sz  = size - 1;
    }
}

// ----------------------------------------------------------------------------

static void Append(d_String* s, const char* format, int vsize, const char* value)
{
    if (value) {
        if (vsize < 0)
            vsize = strlen(value);
        ds_cat_f(s, format, vsize, value);
    }
}

static const char* TypeString(adbus_MessageType type)
{
    if (type == ADBUS_MSG_METHOD) {
        return "method_call";
    } else if (type == ADBUS_MSG_RETURN) {
        return "method_return";
    } else if (type == ADBUS_MSG_ERROR) {
        return "error";
    } else if (type == ADBUS_MSG_SIGNAL) {
        return "signal";
    } else {
        return "unknown";
    }
}

void adbusI_matchString(d_String* s, const adbus_Match* m)
{
    ds_cat(s, "");

    if (m->type)
        ds_cat_f(s, "type='%s',", TypeString(m->type));

    Append(s, "sender='%*s',", m->senderSize, m->sender);
    Append(s, "interface='%*s',", m->interfaceSize, m->interface);
    Append(s, "member='%*s',", m->memberSize, m->member);
    Append(s, "path='%*s',", m->pathSize, m->path);
    Append(s, "destination='%*s',", m->destinationSize, m->destination);

    for (size_t i = 0; i < m->argumentsSize; ++i) {
        adbus_Argument* arg = &m->arguments[i];
        if (arg->value) {
            if (arg->size >= 0) {
                ds_cat_f(s, "arg%d='%*s',", i, arg->size, arg->value);
            } else {
                ds_cat_f(s, "arg%d='%s',", i, arg->value);
            }
        }
    }

    // Remove the trailing ','
    if (ds_size(s) > 0)
        ds_remove_end(s, 1);
}

// ----------------------------------------------------------------------------

int adbus_error_argument(adbus_CbData* d)
{
    if (d->msg->interface) {
        return adbus_errorf(
                d,
                "nz.co.foobar.adbus.InvalidArgument",
                "Invalid argument to the method '%s.%s' on %s",
                d->msg->interface,
                d->msg->member,
                d->msg->path);
    } else {
        return adbus_errorf(
                d,
                "nz.co.foobar.adbus.InvalidArgument",
                "Invalid argument to the method '%s' on %s",
                d->msg->member,
                d->msg->path);
    }
}

int adbusI_pathError(adbus_CbData* d)
{
    return adbus_errorf(
            d,
            "nz.co.foobar.adbus.InvalidPath",
            "The path '%s' does not exist.",
            d->msg->path);
}

int adbusI_interfaceError(adbus_CbData* d)
{
    return adbus_errorf(
            d,
            "nz.co.foobar.adbus.InvalidInterface",
            "The path '%s' does not export the interface '%s'.",
            d->msg->path,
            d->msg->interface,
            d->msg->member);
}

int adbusI_methodError(adbus_CbData* d)
{
    if (d->msg->interface) {
        return adbus_errorf(
                d,
                "nz.co.foobar.adbus.InvalidMethod",
                "The path '%s' does not export the method '%s.%s'.",
                d->msg->path,
                d->msg->interface,
                d->msg->member);
    } else {
        return adbus_errorf(
                d,
                "nz.co.foobar.adbus.InvalidMethod",
                "The path '%s' does not export the method '%s'.",
                d->msg->path,
                d->msg->member);
    }
}

int adbusI_propertyError(adbus_CbData* d)
{
    return adbus_errorf(
            d,
            "nz.co.foobar.adbus.InvalidProperty",
            "The path '%s' does not export the property '%s.%s'.",
            d->msg->path,
            d->msg->interface,
            d->msg->member);
}

int adbusI_propWriteError(adbus_CbData* d)
{
    return adbus_errorf(
            d,
            "nz.co.foobar.adbus.ReadOnlyProperty",
            "The property '%s.%s' on '%s' is read only.",
            d->msg->interface,
            d->msg->member,
            d->msg->path);
}

int adbusI_propReadError(adbus_CbData* d)
{
    return adbus_errorf(
            d,
            "nz.co.foobar.adbus.WriteOnlyProperty",
            "The property '%s.%s' on '%s' is write only.",
            d->msg->interface,
            d->msg->member,
            d->msg->path);
}

int adbusI_propTypeError(adbus_CbData* d)
{ 
    return adbus_errorf(
            d,
            "nz.co.foobar.adbus.InvalidPropertyType",
            "Incorrect property type for '%s.%s' on %s.",
            d->msg->interface,
            d->msg->member,
            d->msg->path);
}


