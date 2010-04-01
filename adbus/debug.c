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

#include "debug.h"
#include "interface.h"
#include <inttypes.h>
#include <stdio.h>

#ifdef _WIN32
#   include <windows.h>
#else
#   include <unistd.h>
#endif

#ifdef __GNUC__
#   include <execinfo.h>
#endif

#undef interface

/* -------------------------------------------------------------------------- */

static void logerr(const char* str, size_t sz)
{ 
#if !defined _WIN32
    fprintf(stderr, "[adbus.so/%d] %.*s", (int) getpid(), (int) sz, str); 
#elif !defined _WIN32_WCE && !defined NDEBUG
    OutputDebugStringA(str);
#else
    (void) str;
    (void) sz;
#endif
}

int adbusI_loglevel = -1;

static int sEnableColors = 0;
static int sLogLevel = -1;
static adbus_LogCallback sLogFunction = &logerr;

void adbusI_initlog()
{
    if (sLogLevel == -1) {
        const char* logenv = getenv("ADBUS_DEBUG");
        const char* colenv = getenv("ADBUS_COLOR");
        sLogLevel = logenv ? strtol(logenv, NULL, 10) : 0;
        sEnableColors = colenv && strcmp(colenv, "1") == 0;
        adbusI_loglevel = sLogFunction ? sLogLevel : -1;
    }
}

void adbus_set_loglevel(int level)
{
    adbusI_initlog();
    sLogLevel = level;
    adbusI_loglevel = sLogFunction ? sLogLevel : -1;
}

void adbus_set_logger(adbus_LogCallback cb)
{ 
    adbusI_initlog();
    sLogFunction = cb; 
    adbusI_loglevel = sLogFunction ? sLogLevel : -1;
}

/* -------------------------------------------------------------------------- */

#define BLACK   (sEnableColors ? "\033[30m" : "")
#define RED     (sEnableColors ? "\033[31m" : "")
#define GREEN   (sEnableColors ? "\033[32m" : "")
#define YELLOW  (sEnableColors ? "\033[33m" : "")
#define BLUE    (sEnableColors ? "\033[34m" : "")
#define MAGENTA (sEnableColors ? "\033[35m" : "")
#define CYAN    (sEnableColors ? "\033[36m" : "")
#define WHITE   (sEnableColors ? "\033[37m" : "")
#define NORMAL  (sEnableColors ? "\033[m" : "")

/* -------------------------------------------------------------------------- */

#define LEADING   8
#define KEY_WIDTH 16

static void Header(d_String* s, const char* format, ...) ADBUSI_PRINTF(2, 3);

static void Header(d_String* s, const char* format, ...)
{
    int spaces;

    va_list ap;
    va_start(ap, format);
    ds_cat_char(s, '\n');
    ds_cat_char_n(s, ' ', LEADING);
    ds_cat(s, RED);

    spaces = ds_size(s) + KEY_WIDTH - 2; /* -2 for ': ' below */
    ds_cat_vf(s, format, ap);
    spaces -= ds_size(s);

    ds_cat_f(s, ":%s ", NORMAL);

    if (spaces > 0) {
        ds_cat_char_n(s, ' ', spaces);
    }
}

static void Number_(d_String* s, const char* field, int num)
{
    if (num >= 0) {
        Header(s, "%s", field);
        ds_cat_f(s, "%d", num);
    }
}

#define Number(s, field, num) Number_(s, field, (int) num)

static void String(d_String* s, const char* field, const char* value)
{
    if (value) {
        Header(s, "%s", field);
        ds_cat_f(s, "%s", value);
    }
}

static void Callback_(d_String* s, const char* field, void* cb, void* user)
{
    if (cb) {
        Header(s, "%s", field);

#ifdef __GNUC__
        {
            char** sym = backtrace_symbols(&cb, 1);
            ds_cat_f(s, "%s, %p", *sym, user);
            free(sym);
        }

#else
        ds_cat_f(s, "%p, %p", cb, user);
#endif
    }
}

#define Callback(s, field, cb, user) Callback_(s, field, (void*) (uintptr_t) cb, user)

/* -------------------------------------------------------------------------- */

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

/* -------------------------------------------------------------------------- */

static void InsertLeading(d_String* s, size_t begin, int spaces)
{
    while (begin < ds_size(s)) {
        const char* nl = (const char*) memchr(ds_cstr(s) + begin, '\n', ds_size(s) - begin);
        if (nl == NULL) {
            break;
        }

        begin = nl - ds_cstr(s) + 1;
        ds_insert_char_n(s, begin, ' ', spaces);
    }
}

static void Append(d_String* s, int spaces, const char* format, ...)
{
    size_t begin = ds_size(s);

    va_list ap;
    va_start(ap, format);

    ds_cat_vf(s, format, ap);
    InsertLeading(s, begin, spaces);
}

static int LogField(d_String* s, adbus_Iterator* i, int spaces);
static int LogArray(d_String* s, adbus_Iterator* i, int spaces)
{
    adbus_IterArray a;
    adbus_Bool first = 1;
    adbus_Bool map;

    if (adbus_iter_beginarray(i, &a))
        return -1;

    map = (*i->sig == ADBUS_DICTENTRY_BEGIN);
    Append(s, spaces + 2, map ? "{\n" : "[\n");

    while (adbus_iter_inarray(i, &a)) {
        if (!first)
            Append(s, spaces + 2, ",\n");
        first = 0;
        if (LogField(s, i, spaces + 2)) {
              return -1;
        }
    }

    Append(s, spaces, map ? "\n}" : "\n]");
    return adbus_iter_endarray(i, &a);
}

static int LogStruct(d_String* s, adbus_Iterator* i, int spaces)
{
    adbus_Bool first = 1;
    if (adbus_iter_beginstruct(i))
        return -1;
    ds_cat_f(s, "(");
    while (*i->sig != ADBUS_STRUCT_END) {
        if (!first)
            ds_cat(s, ", ");
        first = 0;
        if (LogField(s, i, spaces)) {
              return -1;
        }
    }
    ds_cat_f(s, ")");
    return adbus_iter_endstruct(i);
}

static int LogVariant(d_String* s, adbus_Iterator* i, int spaces)
{
    adbus_IterVariant v;
    if (adbus_iter_beginvariant(i, &v))
        return -1;
    ds_cat_f(s, "<%s>{", i->sig);
    if (LogField(s, i, spaces))
        return -1;
    if (*i->sig != '\0')
        return -1;
    ds_cat_f(s, "}");
    return adbus_iter_endvariant(i, &v);
}


static int LogField(d_String* s, adbus_Iterator* i, int spaces)
{
    adbus_Bool      b;
    uint8_t         u8;
    int16_t         i16;
    uint16_t        u16;
    int32_t         i32;
    uint32_t        u32;
    int64_t         i64;
    uint64_t        u64;
    double          d;
    const char*     string;
    size_t          size;

    switch (*i->sig)
    {
    case ADBUS_BOOLEAN:
        if (adbus_iter_bool(i, &b))
            return -1;
        ds_cat_f(s, "%s", b ? "true" : "false");
        break;
    case ADBUS_UINT8:
        if (adbus_iter_u8(i, &u8))
            return -1;
        ds_cat_f(s, "%" PRIu8, u8);
        break;
    case ADBUS_INT16:
        if (adbus_iter_i16(i, &i16))
            return -1;
        ds_cat_f(s, "%" PRIi16, i16);
        break;
    case ADBUS_UINT16:
        if (adbus_iter_u16(i, &u16))
            return -1;
        ds_cat_f(s, "%" PRIu16, u16);
        break;
    case ADBUS_INT32:
        if (adbus_iter_i32(i, &i32))
            return -1;
        ds_cat_f(s, "%" PRIi32, i32);
        break;
    case ADBUS_UINT32:
        if (adbus_iter_u32(i, &u32))
            return -1;
        ds_cat_f(s, "%" PRIu32, u32);
        break;
    case ADBUS_INT64:
        if (adbus_iter_i64(i, &i64))
            return -1;
        ds_cat_f(s, "%" PRIi64, i64);
        break;
    case ADBUS_UINT64:
        if (adbus_iter_u64(i, &u64))
            return -1;
        ds_cat_f(s, "%" PRIu64, u64);
        break;
    case ADBUS_DOUBLE:
        if (adbus_iter_double(i, &d))
            return -1;
        ds_cat_f(s, "%.15g", d);
        break;
    case ADBUS_STRING:
        if (adbus_iter_string(i, &string, &size))
            return -1;
        Append(s, spaces, "\"%*s\"", (int) size, string);
        break;
    case ADBUS_OBJECT_PATH:
        if (adbus_iter_objectpath(i, &string, &size))
            return -1;
        Append(s, spaces, "\"%*s\"", (int) size, string);
        break;
    case ADBUS_SIGNATURE:
        if (adbus_iter_signature(i, &string, &size))
            return -1;
        Append(s, spaces, "\"%*s\"", (int) size, string);
        break;
    case ADBUS_DICTENTRY_BEGIN:
        if (adbus_iter_begindictentry(i))
            return -1;
        if (LogField(s, i, spaces))
            return -1;
        ds_cat_f(s, " = ");
        if (LogField(s, i, spaces))
            return -1;
        if (*i->sig != ADBUS_DICTENTRY_END || adbus_iter_enddictentry(i))
            return -1;
        break;
    case ADBUS_ARRAY:
        return LogArray(s, i, spaces);
    case ADBUS_STRUCT_BEGIN:
        return LogStruct(s, i, spaces);
    case ADBUS_VARIANT:
        return LogVariant(s, i, spaces);
    default:
        assert(0);
        return -1;
    }
    return 0;
}

static int MsgSummary(d_String* s, const adbus_Message* m)
{
    String(s, "type", TypeString(m->type));
    Number(s, "flags", m->flags);
    Number(s, "length", m->size);
    Number(s, "serial", m->serial);
    Number(s, "reply_serial", m->replySerial);
    String(s, "sender", m->sender);
    String(s, "destination", m->destination);
    String(s, "path", m->path);
    String(s, "interface", m->interface);
    String(s, "member", m->member);
    String(s, "error", m->error);
    String(s, "signature", m->signature);

    {
        int argnum = 0;
        adbus_Iterator i;
        adbus_iter_args(&i, m);
        while (i.sig && *i.sig) {
            Header(s, "argument[%d]", argnum++);
            if (LogField(s, &i, KEY_WIDTH + LEADING))
                return -1;
        }
    }

    return 0;
}

void adbusI_logmsg(const adbus_Message* msg, const char* format, ...)
{
    va_list ap;
    d_String s;
    ZERO(s);
    va_start(ap, format);
    ds_cat_vf(&s, format, ap);
    MsgSummary(&s, msg);
    ds_cat(&s, "\n\n");
    sLogFunction(ds_cstr(&s), ds_size(&s));
    ds_free(&s);
}

/* -------------------------------------------------------------------------- */

static void BindString(d_String* s, const adbus_Bind* b)
{
    String(s, "path", b->path);

    if (b->interface) {
        Header(s, "interface");
        ds_cat_f(s, "\"%s\" (%p)", b->interface->name.str, (void*) b->interface);
    }

    if (b->cuser2) {
        Header(s, "cuser2");
        ds_cat_f(s, "%p", b->cuser2);
    }

    Callback(s, "release[0]", b->release[0], b->ruser[0]);
    Callback(s, "release[1]", b->release[1], b->ruser[1]);
}

void adbusI_logbind(const adbus_Bind* b, const char* format, ...)
{
    va_list ap;
    d_String s;
    ZERO(s);
    va_start(ap, format);
    ds_cat_vf(&s, format, ap);
    BindString(&s, b);
    ds_cat(&s, "\n\n");
    sLogFunction(ds_cstr(&s), ds_size(&s));
    ds_free(&s);
}

/* -------------------------------------------------------------------------- */

static void MatchString(d_String* s, const adbus_Match* m)
{
    size_t i;

    Number(s, "add to bus", m->addMatchToBusDaemon);
    String(s, "type", TypeString(m->type));
    Number(s, "reply_serial", m->replySerial);
    String(s, "sender", m->sender);
    String(s, "destination", m->destination);
    String(s, "path", m->path);
    String(s, "interface", m->interface);
    String(s, "member", m->member);
    String(s, "error", m->error);
    Callback(s, "callback", m->callback, m->cuser);
    Callback(s, "release[0]", m->release[0], m->ruser[0]);
    Callback(s, "release[1]", m->release[1], m->ruser[1]);

    for (i = 0; i < m->argumentsSize; ++i) {
        adbus_Argument* arg = &m->arguments[i];
        if (arg->value) {
            Header(s, "argument[%d]", (int) i);
            ds_cat_f(s, "\"%s\"", arg->value);
        }
    }
}

void adbusI_logmatch(const adbus_Match* m, const char* format, ...)
{
    va_list ap;
    d_String s;
    ZERO(s);
    va_start(ap, format);
    ds_cat_vf(&s, format, ap);
    MatchString(&s, m);
    ds_cat(&s, "\n\n");
    sLogFunction(ds_cstr(&s), ds_size(&s));
    ds_free(&s);
}

/* -------------------------------------------------------------------------- */

static void ReplyString(d_String* s, const adbus_Reply* r)
{
    Number(s, "serial", r->serial);
    String(s, "remote", r->remote);
    Callback(s, "callback", r->callback, r->cuser);
    Callback(s, "error", r->error, r->euser);
    Callback(s, "release[0]", r->release[0], r->ruser[0]);
    Callback(s, "release[1]", r->release[1], r->ruser[1]);
}

void adbusI_logreply(const adbus_Reply* r, const char* format, ...)
{
    va_list ap;
    d_String s;
    ZERO(s);
    va_start(ap, format);
    ds_cat_vf(&s, format, ap);
    ReplyString(&s, r);
    ds_cat(&s, "\n\n");
    sLogFunction(ds_cstr(&s), ds_size(&s));
    ds_free(&s);
}

/* -------------------------------------------------------------------------- */

#ifndef min
#   define min(x, y) ((x < y) ? (x) : (y))
#endif

#define IsPrintable(ch) (' ' <= ch && ch <= '~')

static void AppendData(d_String* s, const uint8_t* buf, size_t sz)
{
    size_t i, j;
    for (i = 0; i < sz; i += 16) {
        size_t spaces = 40;
        size_t end = min(i + 16, sz);

        ds_cat(s, NORMAL);
        ds_cat_char(s, '\n');
        ds_cat_char_n(s, ' ', LEADING);
        ds_cat_f(s, "%s0x%04x    ", CYAN, (int) i);
        for (j = i; j < end; j++) {
            spaces -= 2;
            ds_cat_f(s, "%s%02x", IsPrintable(buf[j]) ? NORMAL : RED, (int) buf[j]);
            if (j > 0 && j % 2) {
                spaces -= 1;
                ds_cat_char(s, ' ');
            }
        }

        ds_cat_char_n(s, ' ', spaces);

        for (j = i; j < end; j++) {
            if (IsPrintable(buf[j])) {
                ds_cat_f(s, "%s%c", NORMAL, (int) buf[j]);
            } else {
                ds_cat_f(s, "%s.", RED);
            }
        }
    }
}

void adbusI_logdata(const char* buf, int sz, const char* format, ...)
{
    if (sz > 0) {
        va_list ap;
        d_String s;
        ZERO(s);
        va_start(ap, format);

        ds_cat_vf(&s, format, ap);
        AppendData(&s, (const uint8_t*) buf, sz);
        ds_cat_f(&s, "%s\n\n", NORMAL);

        sLogFunction(ds_cstr(&s), ds_size(&s));
        ds_free(&s);
    }
}



/* -------------------------------------------------------------------------- */

void adbusI_log(const char* format, ...)
{
    d_String s;
    va_list ap;
    ZERO(s);
    va_start(ap, format);
    ds_cat_vf(&s, format, ap);
    ds_cat_char(&s, '\n');
    sLogFunction(ds_cstr(&s), ds_size(&s));
    ds_free(&s);
}
