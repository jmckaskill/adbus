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



/* ------------------------------------------------------------------------- */

char adbusI_nativeEndianness(void)
{
    int i = 1;
    int little = (*(char*)&i) == 1;
    return little ? 'l' : 'B';
}

/* ------------------------------------------------------------------------- */

const uint8_t adbusI_majorProtocolVersion = 1;

/* ------------------------------------------------------------------------- */

adbus_Bool adbusI_isValidObjectPath(const char* str, size_t len)
{
    const char *slash, *cur;

    if (!str || len == 0)
        return 0;
    if (str[0] != '/')
        return 0;
    if (len > 1 && str[len - 1] == '/')
        return 0;

    /* Now we check for consecutive '/' and that
     * the path components only use [A-Z][a-z][0-9]_
     */
    slash = str;
    cur = str;
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

/* ------------------------------------------------------------------------- */

adbus_Bool adbusI_isValidInterfaceName(const char* str, size_t len)
{
    const char *dot, *cur;

    if (!str || len == 0 || len > 255)
        return 0;

    /* Must not begin with a digit */
    if (!(  ('a' <= str[0] && str[0] <= 'z')
                || ('A' <= str[0] && str[0] <= 'Z')
                || (str[0] == '_')))
    {
        return 0;
    }

    /* Now we check for consecutive '.' and that
     * the components only use [A-Z][a-z][0-9]_
     */
    dot = str - 1;
    cur = str;
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
    /* Interface names must include at least one '.' */
    if (dot == str - 1)
        return 0;

    return 1;
}

/* ------------------------------------------------------------------------- */

adbus_Bool adbusI_isValidBusName(const char* str, size_t len)
{
    const char *dot, *cur;

    if (!str || len == 0 || len > 255)
        return 0;

    /* Bus name must either begin with : or not a digit */
    if (!(  (str[0] == ':')
                || ('a' <= str[0] && str[0] <= 'z')
                || ('A' <= str[0] && str[0] <= 'Z')
                || (str[0] == '_')
                || (str[0] == '-')))
    {
        return 0;
    }

    /* Now we check for consecutive '.' and that
     * the components only use [A-Z][a-z][0-9]_
     */
    dot = str[0] == ':' ? str : str - 1;
    cur = str;
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
    /* Bus names must include at least one '.' */
    if (dot == str - 1)
        return 0;

    return 1;
}

/* ------------------------------------------------------------------------- */

adbus_Bool adbusI_isValidMemberName(const char* str, size_t len)
{
    const char* cur;
    if (!str || len == 0 || len > 255)
        return 0;

    /* Must not begin with a digit */
    if (!(   ('a' <= str[0] && str[0] <= 'z')
          || ('A' <= str[0] && str[0] <= 'Z')
          || (str[0] == '_')))
    {
        return 0;
    }

    /* We now check that we only use [A-Z][a-z][0-9]_ */
    cur = str;
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

/* ------------------------------------------------------------------------- */

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
                const char *val, *end;

                val = adbus_nextarg(sig + 1);
                if (!val)
                    return NULL;

                end = adbus_nextarg(val);
                if (!end || *end != ADBUS_DICTENTRY_END)
                    return NULL;

                return end + 1;
            }

        default:
            return NULL;
    }
}

/* ------------------------------------------------------------------------- */

void adbusI_relativePath(
        d_String*  out,
        const char* path1,
        int         size1,
        const char* path2,
        int         size2)
{
    size_t i;

    ds_clear(out);
    if (size1 < 0)
        size1 = strlen(path1);
    if (size2 < 0)
        size2 = strlen(path2);

    /* Make sure it starts with a / */
    if (size1 > 0 && path1[0] != '/')
        ds_cat(out, "/");
    if (size1 > 0)
        ds_cat_n(out, path1, size1);

    /* Make sure it has a / separator */
    if (size2 > 0 && path2[0] != '/')
        ds_cat(out, "/");
    if (size2 > 0)
        ds_cat_n(out, path2, size2);

    /* Remove repeating slashes */
    for (i = 1; i < ds_size(out); ++i) {
        if (ds_a(out, i) == '/' && ds_a(out, i-1) == '/') {
            ds_erase(out, i, 1);
        }
    }

    /* Remove trailing / */
    if (ds_size(out) > 1 && ds_a(out, ds_size(out) - 1) == '/')
        ds_erase_end(out, 1);
}

/* ------------------------------------------------------------------------- */

void adbusI_sanitisePath(d_String* out, const char* path, int sz)
{ adbusI_relativePath(out, path, sz, NULL, 0); }


/* ------------------------------------------------------------------------- */

void adbusI_parentPath(
        dh_strsz_t  path,
        dh_strsz_t* parent)
{
    int size;

#ifndef NDEBUG
    {
        /* Path should have already been sanitised so double /'s should not happen */
        d_String sanitised;
        ZERO(sanitised);
        adbusI_sanitisePath(&sanitised, path.str, path.sz);
        assert(ds_cmp_n(&sanitised, path.str, path.sz) == 0);
        ds_free(&sanitised);
    }
#endif

    /* Search back until we find the first '/' that is not the last character */
    size = path.sz - 1;
    while (size > 0 && path.str[size - 1] != '/') {
        --size;
    }

    if (size <= 0) {
        /* Parent of "/" */
        parent->str = NULL;
    } else if (size == 1) {
        /* Parent of "/Foo" - "/" */
        parent->str = path.str;
        parent->sz  = size;
    } else {
        /* Parent of "/Foo/Bar" - "/Foo" (size is currently at "/Foo/") */
        parent->str = path.str;
        parent->sz  = size - 1;
    }
}

/* ------------------------------------------------------------------------- */

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
    size_t i;

    ds_cat(s, "");

    if (m->type)
        ds_cat_f(s, "type='%s',", TypeString(m->type));

    Append(s, "sender='%*s',", m->senderSize, m->sender);
    Append(s, "interface='%*s',", m->interfaceSize, m->interface);
    Append(s, "member='%*s',", m->memberSize, m->member);
    Append(s, "path='%*s',", m->pathSize, m->path);
    Append(s, "destination='%*s',", m->destinationSize, m->destination);

    for (i = 0; i < m->argumentsSize; ++i) {
        adbus_Argument* arg = &m->arguments[i];
        if (arg->value) {
            if (arg->size >= 0) {
                ds_cat_f(s, "arg%d='%*s',", (int) i, (int) arg->size, arg->value);
            } else {
                ds_cat_f(s, "arg%d='%s',", (int) i, arg->value);
            }
        }
    }

    /* Remove the trailing ',' */
    if (ds_size(s) > 0)
        ds_erase_end(s, 1);
}

/* -------------------------------------------------------------------------- */

static adbus_Bool StringMatches(
        const char* match,
        size_t      matchsz,
        const char* msg,
        size_t      msgsz)
{
    if (!match)
        return 1;

    if (!msg)
        return 0;

    if (matchsz != msgsz)
        return 0;

    return memcmp(match, msg, msgsz) == 0;
}


static adbus_Bool ArgsMatch(const adbus_Match* match, adbus_Message* msg)
{
    size_t i;

    if (match->argumentsSize == 0)
        return 1;

    if (adbus_parseargs(msg))
        return 0;

    if (msg->argumentsSize < match->argumentsSize)
        return 0;

    for (i = 0; i < match->argumentsSize; i++) {
        adbus_Argument* matcharg = &match->arguments[i];
        adbus_Argument* msgarg = &msg->arguments[i];

        if (!matcharg->value)
            continue;

        if (msgarg->value == NULL)
            return 0;

        if (msgarg->size != matcharg->size)
            return 0;

        if (memcmp(msgarg->value, matcharg->value, matcharg->size) != 0)
            return 0;

    }
    return 1;
}

adbus_Bool adbusI_matchesMessage(const adbus_Match* match, adbus_Message* msg)
{
    if (match->type == ADBUS_MSG_INVALID && match->type != msg->type) {
        return 0;
    } else if (match->replySerial >= 0 && (!msg->replySerial || (uint32_t) match->replySerial != *msg->replySerial)) {
        return 0;
    } else if (!StringMatches(match->path, match->pathSize, msg->path, msg->pathSize)) {
        return 0;
    } else if (!StringMatches(match->interface, match->interfaceSize, msg->interface, msg->interfaceSize)) {
        return 0;
    } else if (!StringMatches(match->member, match->memberSize, msg->member, msg->memberSize)) {
        return 0;
    } else if (!StringMatches(match->error, match->errorSize, msg->error, msg->errorSize)) {
        return 0;
    } else if (!StringMatches(match->destination, match->destinationSize, msg->destination, msg->destinationSize)) {
        return 0;
    } else if (!StringMatches(match->sender, match->senderSize, msg->sender, msg->senderSize)) {
        return 0;
    } else if (!ArgsMatch(match, msg)) {
        return 0;
    } else {
        return 1;
    }
}

