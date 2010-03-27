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

/* -------------------------------------------------------------------------- */

adbus_LogCallback sLogFunction;

void adbus_set_logger(adbus_LogCallback cb)
{ sLogFunction = cb; }

/* -------------------------------------------------------------------------- */

static void PrintStringField(
        d_String* str,
        const char* field,
        const char* what)
{
    if (field)
        ds_cat_f(str, "%-15s \"%s\"\n", what,  field);
}

static int LogField(d_String* str, adbus_Iterator* i);
static int LogArray(d_String* str, adbus_Iterator* i)
{
    adbus_IterArray a;
    adbus_Bool first = 1;
    adbus_Bool map;

    if (adbus_iter_beginarray(i, &a))
        return -1;

    map = (*i->sig == ADBUS_DICTENTRY_BEGIN);
    ds_cat(str, map ? "{" : "[");

    while (adbus_iter_inarray(i, &a)) {
        if (!first)
            ds_cat(str, ", ");
        first = 0;
        if (LogField(str, i)) {
              return -1;
        }
    }

    ds_cat_f(str, map ? "}" : "]");
    return adbus_iter_endarray(i, &a);
}

static int LogStruct(d_String* str, adbus_Iterator* i)
{
    adbus_Bool first = 1;
    if (adbus_iter_beginstruct(i))
        return -1;
    ds_cat_f(str, "(");
    while (*i->sig != ADBUS_STRUCT_END) {
        if (!first)
            ds_cat(str, ", ");
        first = 0;
        if (LogField(str, i)) {
              return -1;
        }
    }
    ds_cat_f(str, ")");
    return adbus_iter_endstruct(i);
}

static int LogVariant(d_String* str, adbus_Iterator* i)
{
    adbus_IterVariant v;
    if (adbus_iter_beginvariant(i, &v))
        return -1;
    ds_cat_f(str, "<%s>{", i->sig);
    if (LogField(str, i))
        return -1;
    if (*i->sig != '\0')
        return -1;
    ds_cat_f(str, "}");
    return adbus_iter_endvariant(i, &v);
}


static int LogField(d_String* str, adbus_Iterator* i)
{
    const adbus_Bool*     b;
    const uint8_t*        u8;
    const int16_t*        i16;
    const uint16_t*       u16;
    const int32_t*        i32;
    const uint32_t*       u32;
    const int64_t*        i64;
    const uint64_t*       u64;
    const double*         d;
    const char*     string;
    size_t          size;

    switch (*i->sig)
    {
    case ADBUS_BOOLEAN:
        if (adbus_iter_bool(i, &b))
            return -1;
        ds_cat_f(str, "%s", *b ? "true" : "false");
        break;
    case ADBUS_UINT8:
        if (adbus_iter_u8(i, &u8))
            return -1;
        ds_cat_f(str, "%" PRIu8, *u8);
        break;
    case ADBUS_INT16:
        if (adbus_iter_i16(i, &i16))
            return -1;
        ds_cat_f(str, "%" PRIi16, *i16);
        break;
    case ADBUS_UINT16:
        if (adbus_iter_u16(i, &u16))
            return -1;
        ds_cat_f(str, "%" PRIu16, *u16);
        break;
    case ADBUS_INT32:
        if (adbus_iter_i32(i, &i32))
            return -1;
        ds_cat_f(str, "%" PRIi32, *i32);
        break;
    case ADBUS_UINT32:
        if (adbus_iter_u32(i, &u32))
            return -1;
        ds_cat_f(str, "%" PRIu32, *u32);
        break;
    case ADBUS_INT64:
        if (adbus_iter_i64(i, &i64))
            return -1;
        ds_cat_f(str, "%" PRIi64, *i64);
        break;
    case ADBUS_UINT64:
        if (adbus_iter_u64(i, &u64))
            return -1;
        ds_cat_f(str, "%" PRIu64, *u64);
        break;
    case ADBUS_DOUBLE:
        if (adbus_iter_double(i, &d))
            return -1;
        ds_cat_f(str, "%.15g", *d);
        break;
    case ADBUS_STRING:
        if (adbus_iter_string(i, &string, &size))
            return -1;
        ds_cat_f(str, "\"%*s\"", (int) size, string);
        break;
    case ADBUS_OBJECT_PATH:
        if (adbus_iter_objectpath(i, &string, &size))
            return -1;
        ds_cat_f(str, "\"%*s\"", (int) size, string);
        break;
    case ADBUS_SIGNATURE:
        if (adbus_iter_signature(i, &string, &size))
            return -1;
        ds_cat_f(str, "\"%*s\"", (int) size, string);
        break;
    case ADBUS_DICTENTRY_BEGIN:
        if (LogField(str, i))
            return -1;
        ds_cat_f(str, " = ");
        if (LogField(str, i))
            return -1;
        if (*i->sig != ADBUS_DICTENTRY_END)
            return -1;
        break;
    case ADBUS_ARRAY_BEGIN:
        return LogArray(str, i);
    case ADBUS_STRUCT_BEGIN:
        return LogStruct(str, i);
    case ADBUS_VARIANT_BEGIN:
        return LogVariant(str, i);
    default:
        assert(0);
        return -1;
    }
    return 0;
}

static int MsgSummary(d_String* str, const adbus_Message* m)
{
    if (m->type == ADBUS_MSG_METHOD) {
        ds_cat_f(str, "Method call: ");
    } else if (m->type == ADBUS_MSG_RETURN) {
        ds_cat_f(str, "Return: ");
    } else if (m->type == ADBUS_MSG_ERROR) {
        ds_cat_f(str, "Error: ");
    } else if (m->type == ADBUS_MSG_SIGNAL) {
        ds_cat_f(str, "Signal: ");
    } else {
        ds_cat_f(str, "Unknown (%d): ", (int) m->type);
    }

    ds_cat_f(str, "Flags %d, Length %d, Serial %d\n",
            (int) m->flags,
            (int) m->size,
            (int) m->serial);

    if (m->replySerial) {
        ds_cat_f(str, "%-15s %u\n", "Reply serial", *m->replySerial);
    }

    PrintStringField(str, m->sender, "Sender");
    PrintStringField(str, m->destination, "Destination");
    PrintStringField(str, m->path, "Path");
    PrintStringField(str, m->interface, "Interface");
    PrintStringField(str, m->member, "Member");
    PrintStringField(str, m->error, "Error");
    PrintStringField(str, m->signature, "Signature");

    {
        int argnum = 0;
        adbus_Iterator i;
        adbus_iter_args(&i, m);
        while (i.sig && *i.sig) {
            ds_cat_f(str, "Argument %2d     ", argnum++);
            if (LogField(str, &i))
                return -1;
            ds_cat_char(str, '\n');
        }
    }

    return 0;
}

void adbusI_logmsg(const char* header, const adbus_Message* msg)
{
    d_String str;
    ZERO(str);
    ds_cat_f(&str, "%s ", header);
    MsgSummary(&str, msg);
    sLogFunction(ds_cstr(&str), ds_size(&str));
    ds_free(&str);
}

/* -------------------------------------------------------------------------- */

static void Append(d_String* s, const char* field, const char* value, int vsize)
{
    if (value) {
        if (vsize < 0)
            vsize = strlen(value);
        ds_cat_f(s, "%-15s \"%*s\"\n", field, vsize, value);
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


/* -------------------------------------------------------------------------- */

static void BindString(d_String* s, const adbus_Bind* b)
{
    Append(s, "Path", b->path, b->pathSize);
    if (b->interface) {
        ds_cat_f(s, "%-15s %p %s\n", "Interface", (void*) b->interface, b->interface->name.str);
    } else {
        ds_cat_f(s, "%-15s %p\n", "Interface", (void*) b->interface);
    }

    ds_cat_f(s, "%-15s %p\n", "User2", b->cuser2);
    if (b->release[0])
        ds_cat_f(s, "%-15s %p %p\n", "Release 0", (void*) (uintptr_t) b->release[0], b->ruser[0]);
    if (b->release[1])
        ds_cat_f(s, "%-15s %p %p\n", "Release 1", (void*) (uintptr_t) b->release[1], b->ruser[1]);
}

void adbusI_logbind(const char* header, const adbus_Bind* b)
{
    d_String str;
    ZERO(str);
    ds_cat_f(&str, "%s ", header);
    BindString(&str, b);
    sLogFunction(ds_cstr(&str), ds_size(&str));
    ds_free(&str);
}

/* -------------------------------------------------------------------------- */

static void MatchString(d_String* s, const adbus_Match* m)
{
    size_t i;
    if (m->addMatchToBusDaemon)
        ds_cat_f(s, "Add to bus\n");
    if (m->callback)
        ds_cat_f(s, "%-15s %p %p\n", "Callback", (void*) (uintptr_t) m->callback, m->cuser);
    if (m->release[0])
        ds_cat_f(s, "%-15s %p %p\n", "Release 0", (void*) (uintptr_t) m->release[0], m->ruser[0]);
    if (m->release[1])
        ds_cat_f(s, "%-15s %p %p\n", "Release 1", (void*) (uintptr_t) m->release[1], m->ruser[1]);

    if (m->type)
        ds_cat_f(s, "%-15s %s\n", "Type", TypeString(m->type));
    if (m->replySerial >= 0)
        ds_cat_f(s, "%-15s %u\n", "Reply serial", (unsigned int) m->replySerial);

    Append(s, "Sender", m->sender, m->senderSize);
    Append(s, "Destination", m->destination, m->destinationSize);
    Append(s, "Interface", m->interface, m->interfaceSize);
    Append(s, "Path", m->path, m->pathSize);
    Append(s, "Member", m->member, m->memberSize);
    Append(s, "Error", m->error, m->errorSize);

    for (i = 0; i < m->argumentsSize; ++i) {
        adbus_Argument* arg = &m->arguments[i];
        if (arg->value) {
            if (arg->size >= 0) {
                ds_cat_f(s, "Argument %-2d     \"%*s\"\n", (int) i, (int) arg->size, arg->value);
            } else {
                ds_cat_f(s, "Argument %-2d     \"%s\"\n", (int) i, arg->value);
            }
        }
    }
}

void adbusI_logmatch(const char* header, const adbus_Match* m)
{
    d_String str;
    ZERO(str);
    ds_cat_f(&str, "%s ", header);
    MatchString(&str, m);
    sLogFunction(ds_cstr(&str), ds_size(&str));
    ds_free(&str);
}

/* -------------------------------------------------------------------------- */

static void ReplyString(d_String* s, const adbus_Reply* r)
{
    ds_cat_f(s, "%-15s %u\n", "Serial", (unsigned int) r->serial);
    Append(s, "Remote", r->remote, r->remoteSize);
    if (r->callback)
        ds_cat_f(s, "%-15s %p %p\n", "Callback", (void*) (uintptr_t) r->callback, r->cuser);
    if (r->error)
        ds_cat_f(s, "%-15s %p %p\n", "Error", (void*) (uintptr_t) r->error, r->euser);
    if (r->release[0])
        ds_cat_f(s, "%-15s %p %p\n", "Release 0", (void*) (uintptr_t) r->release[0], r->ruser[0]);
    if (r->release[1])
        ds_cat_f(s, "%-15s %p %p\n", "Release 1", (void*) (uintptr_t) r->release[1], r->ruser[1]);
}

void adbusI_logreply(const char* header, const adbus_Reply* r)
{
    d_String str;
    ZERO(str);
    ds_cat_f(&str, "%s ", header);
    ReplyString(&str, r);
    sLogFunction(ds_cstr(&str), ds_size(&str));
    ds_free(&str);
}

/* -------------------------------------------------------------------------- */

void adbusI_log(const char* format, ...)
{
    d_String str;
    va_list ap;
    ZERO(str);
    va_start(ap, format);
    ds_set_vf(&str, format, ap);
    sLogFunction(ds_cstr(&str), ds_size(&str));
    ds_free(&str);
}
