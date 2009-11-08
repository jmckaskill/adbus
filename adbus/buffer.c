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
#include "buffer.h"
#include "misc.h"

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static struct StackEntry* StackTop(adbus_Buffer* m)
{
    return &kv_a(m->stack, kv_size(m->stack) - 1);
}

static struct StackEntry* StackPush(adbus_Buffer* m)
{
    return kv_push(Stack, m->stack, 1);
}

static void StackPop(adbus_Buffer* m)
{
    struct StackEntry* s = StackTop(m);
    if (s->type == VARIANT_STACK)
        free(s->d.variant.signature);
    kv_pop(Stack, m->stack, 1);
}

static size_t StackSize(adbus_Buffer* m)
{
    return kv_size(m->stack);
}

static void AlignData(adbus_Buffer* m, size_t align)
{
    size_t old = kv_size(m->data);
    size_t sz = ADBUSI_ALIGN(old, align);
    kv_push(u8, m->data, sz - old);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

adbus_Buffer* adbus_buf_new()
{
    adbus_Buffer* m = NEW(adbus_Buffer);
    m->data  = kv_new(u8);
    m->stack = kv_new(Stack);
    adbus_buf_reset(m);
    return m;
}

// ----------------------------------------------------------------------------

void adbus_buf_reset(adbus_Buffer* m)
{
    while(kv_size(m->stack) > 0)
        StackPop(m);

    kv_clear(u8, m->data);
    m->signature[0] = '\0';
    m->sigp = &m->signature[0];
}

// ----------------------------------------------------------------------------

void adbus_buf_free(adbus_Buffer* m)
{
    if (m) {
        // We need to pop off the stack entries to free the variant strings
        while(kv_size(m->stack) > 0)
            StackPop(m);

        kv_free(u8, m->data);
        kv_free(Stack, m->stack);

        free(m);
    }
}

// ----------------------------------------------------------------------------

void adbus_buf_set(adbus_Buffer* m,
                            const char*     sig,
                            int             sigsize,
                            const uint8_t*  data,
                            size_t          dataSize)
{
    if (sigsize < 0)
        sigsize = strlen(sig);

    ASSERT_RETURN(sigsize < 256);

    adbus_buf_reset(m);

    strncat(m->signature, sig, sigsize);
    m->sigp = m->signature;

    uint8_t* dest = kv_push(u8, m->data, dataSize);
    memcpy(dest, data, dataSize);
}

// ----------------------------------------------------------------------------

void adbus_buf_get(const adbus_Buffer* m,
                            const char**    sig,
                            size_t*         sigsize,
                            const uint8_t** data,
                            size_t*         dataSize)
{
    if (sig)
        *sig = m->signature;
    if (sigsize)
        *sigsize = strlen(m->signature);
    if (data)
        *data = &kv_a(m->data, 0);
    if (dataSize)
        *dataSize = kv_size(m->data);
}

// ----------------------------------------------------------------------------

int adbus_buf_appenddata(adbus_Buffer* m, const uint8_t* data, size_t size)
{
    if (StackSize(m) != 0) {
        struct StackEntry* s = StackTop(m);
        CHECK(s->type == ARRAY_STACK || s->type == VARIANT_STACK);
    }

    uint8_t* dest = kv_push(u8, m->data, size);
    memcpy(dest, data, size);

    return 0;
}

// ----------------------------------------------------------------------------

int adbus_buf_copy(adbus_Buffer* m, adbus_Iterator* i, int scope)
{
    adbus_Field field;
    while (!adbus_iter_isfinished(i, scope))
    {
        if (adbus_iter_next(i, &field))
            return -1;

        switch (field.type)
        {
        case ADBUS_UINT8:
            adbus_buf_uint8(m, field.u8);
            break;
        case ADBUS_BOOLEAN:
            adbus_buf_bool(m, field.b);
            break;
        case ADBUS_INT16:
            adbus_buf_int16(m, field.i16);
            break;
        case ADBUS_UINT16:
            adbus_buf_uint16(m, field.u16);
            break;
        case ADBUS_INT32:
            adbus_buf_int32(m, field.i32);
            break;
        case ADBUS_UINT32:
            adbus_buf_uint32(m, field.u32);
            break;
        case ADBUS_INT64:
            adbus_buf_int64(m, field.i64);
            break;
        case ADBUS_UINT64:
            adbus_buf_uint64(m, field.u64);
            break;
        case ADBUS_DOUBLE:
            adbus_buf_double(m, field.d);
            break;
        case ADBUS_STRING:
            adbus_buf_string(m, field.string, field.size);
            break;
        case ADBUS_OBJECT_PATH:
            adbus_buf_objectpath(m, field.string, field.size);
            break;
        case ADBUS_SIGNATURE:
            adbus_buf_signature(m, field.string, field.size);
            break;
        case ADBUS_ARRAY_BEGIN:
            adbus_buf_beginarray(m);
            break;
        case ADBUS_ARRAY_END:
            adbus_buf_endarray(m);
            break;
        case ADBUS_STRUCT_BEGIN:
            adbus_buf_beginstruct(m);
            break;
        case ADBUS_STRUCT_END:
            adbus_buf_endstruct(m);
            break;
        case ADBUS_MAP_BEGIN:
            adbus_buf_beginmap(m);
            break;
        case ADBUS_MAP_END:
            adbus_buf_endmap(m);
            break;
        case ADBUS_VARIANT_BEGIN:
            adbus_buf_beginvariant(m, field.string, field.size);
            break;
        case ADBUS_VARIANT_END:
            adbus_buf_endvariant(m);
            break;
        default:
            return -1;
        }
    }
    return adbus_iter_next(i, &field);
}

// ----------------------------------------------------------------------------

adbus_FieldType adbus_buf_expected(adbus_Buffer* m)
{
    if (m->sigp == NULL)
        return ADBUS_END_FIELD;
    else if (*m->sigp == 'a' && *(m->sigp+1) == '{')
        return ADBUS_MAP_BEGIN;
    else
        return (adbus_FieldType) *m->sigp;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void AppendField(adbus_Buffer* m);

static uint8_t* AppendFixedField(
        adbus_Buffer*     m,
        adbus_FieldType     fieldtype,
        size_t                      size)
{
    if (*m->sigp != (char) fieldtype)
        return NULL;

    AlignData(m, size);

    uint8_t* data = kv_push(u8, m->data, size);
    m->sigp++;
    AppendField(m);

    return data;
}

static int Append8(
        adbus_Buffer*     m,
        adbus_FieldType     fieldtype,
        uint8_t                     data)
{
    uint8_t* pdata = AppendFixedField(m, fieldtype, sizeof(uint8_t));
    if (!pdata)
        return 1;
    *pdata = data;
    return 0;
}

static int Append16(
        adbus_Buffer*     m,
        adbus_FieldType     fieldtype,
        uint16_t                    data)
{
    uint16_t* pdata = (uint16_t*) AppendFixedField(m, fieldtype, sizeof(uint16_t));
    if (!pdata)
        return 1;
    *pdata = data;
    return 0;
}

static int Append32(
        adbus_Buffer*     m,
        adbus_FieldType     fieldtype,
        uint32_t                    data)
{
    uint32_t* pdata = (uint32_t*) AppendFixedField(m, fieldtype, sizeof(uint32_t));
    if (!pdata)
        return 1;
    *pdata = data;
    return 0;
}

static int Append64(
        adbus_Buffer*     m,
        adbus_FieldType     fieldtype,
        uint64_t                    data)
{
    uint64_t* pdata = (uint64_t*) AppendFixedField(m, fieldtype, sizeof(uint64_t));
    if (!pdata)
        return 1;
    *pdata = data;
    return 0;
}

int adbus_buf_bool(adbus_Buffer* m, uint32_t data)
{ return Append32(m, ADBUS_BOOLEAN, data ? 1 : 0); }

int adbus_buf_uint8(adbus_Buffer* m, uint8_t data)
{ return Append8(m, ADBUS_UINT8, data); }

int adbus_buf_int16(adbus_Buffer* m, int16_t data)
{ return Append16(m, ADBUS_INT16, *(uint16_t*)&data); }

int adbus_buf_uint16(adbus_Buffer* m, uint16_t data)
{ return Append16(m, ADBUS_UINT16, data); }

int adbus_buf_int32(adbus_Buffer* m, int32_t data)
{ return Append32(m, ADBUS_INT32, *(uint32_t*)&data); }

int adbus_buf_uint32(adbus_Buffer* m, uint32_t data)
{ return Append32(m, ADBUS_UINT32, data); }

int adbus_buf_int64(adbus_Buffer* m, int64_t data)
{ return Append64(m, ADBUS_INT64, *(uint64_t*)&data); }

int adbus_buf_uint64(adbus_Buffer* m, uint64_t data)
{ return Append64(m, ADBUS_UINT64, data); }

int adbus_buf_double(adbus_Buffer* m, double data)
{
    // Take the safe route to avoid any possible aliasing problems
    // *(uint64_t*) &data won't compile with strict aliasing on gcc
    uint64_t d = 0;
    for (size_t i = 0; i < 8; ++i)
        ((uint8_t*) &d)[i] = ((uint8_t*) &data)[i];

    return Append64(m, ADBUS_DOUBLE, d);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static int AppendShortString(adbus_Buffer* m, const char* str, int size)
{
    if (size < 0)
        size = (int)strlen(str);

    CHECK(size < 256);

    uint8_t* psize  = kv_push(u8, m->data, 1 + size + 1);
    char* pstr      = (char*) psize + 1;

    *psize = (uint8_t) size;
    memcpy(pstr, str, size);
    pstr[size] = '\0';

    m->sigp++;

    AppendField(m);

    return 0;
}

static int AppendLongString(adbus_Buffer* m, const char* str, int size)
{
    if (size < 0)
        size = (int)strlen(str);

    AlignData(m, 4);

    uint8_t* pdata  = kv_push(u8, m->data, 4 + size + 1);
    uint32_t* psize = (uint32_t*) pdata;
    char* pstr      = (char*) pdata + 4;

    *psize = size;
    memcpy(pstr, str, size);
    pstr[size] = '\0';

    m->sigp++;

    AppendField(m);

    return 0;
}

// ----------------------------------------------------------------------------

int adbus_buf_string(adbus_Buffer* m, const char* str, int size)
{
    CHECK(*m->sigp == ADBUS_STRING);
    return AppendLongString(m, str, size);
}

// ----------------------------------------------------------------------------

int adbus_buf_objectpath(adbus_Buffer* m, const char* str, int size)
{
    CHECK(*m->sigp == ADBUS_OBJECT_PATH);
    return AppendLongString(m, str, size);
}

// ----------------------------------------------------------------------------

int adbus_buf_signature(adbus_Buffer* m, const char* str, int size)
{
    CHECK(*m->sigp == ADBUS_SIGNATURE);
    return AppendShortString(m, str, size);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int adbus_buf_append(adbus_Buffer* m, const char* sig, int size)
{
    if (size < 0)
        size = strlen(sig);

    CHECK(strlen(m->signature) + size < 256);
    CHECK(StackSize(m) == 0);
    CHECK(m->sigp == NULL || *m->sigp == '\0');

    strncat(m->signature, sig, size);

    return 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int adbus_buf_beginarray(adbus_Buffer* m)
{
    CHECK(*m->sigp == ADBUS_ARRAY_BEGIN);

    m->sigp++;

    AlignData(m, 4);
    size_t isize = kv_size(m->data);
    kv_push(u8, m->data, 4);
    AlignData(m, adbusI_alignment(*m->sigp));
    size_t idata = kv_size(m->data);

    struct StackEntry* s = StackPush(m);
    s->type = ARRAY_STACK;
    s->d.array.sizeIndex    = isize;
    s->d.array.dataBegin    = idata;
    s->d.array.sigBegin     = m->sigp;

    return 0;
}

// ----------------------------------------------------------------------------

static void AppendArrayChild(adbus_Buffer* m)
{
    struct StackEntry* s = StackTop(m);
    m->sigp = s->d.array.sigBegin;
}

// ----------------------------------------------------------------------------

int adbus_buf_endarray(adbus_Buffer* m)
{
    struct StackEntry* s = StackTop(m);
    uint32_t* psize = (uint32_t*) &kv_a(m->data, s->d.array.sizeIndex);
    *psize = (uint32_t)(kv_size(m->data) - s->d.array.dataBegin);

    CHECK(*psize < ADBUSI_MAXIMUM_ARRAY_LENGTH);

    m->sigp = adbusI_findArrayEnd(s->d.array.sigBegin);
    assert(m->sigp);

    StackPop(m);

    AppendField(m);
    return 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int adbus_buf_beginmap(adbus_Buffer* m)
{
    CHECK(*m->sigp == 'a' && *(m->sigp + 1) == '{');

    m->sigp += 2;

    AlignData(m, 4);
    size_t isize = kv_size(m->data);
    kv_push(u8, m->data, 4);
    AlignData(m, 8);
    size_t idata = kv_size(m->data);

    struct StackEntry* s = StackPush(m);
    s->type = MAP_STACK;
    s->d.map.sizeIndex  = isize;
    s->d.map.dataBegin  = idata;
    s->d.map.sigBegin   = m->sigp;
    s->d.map.haveKey    = 0;

    return 0;
}

// ----------------------------------------------------------------------------

static void AppendMapChild(adbus_Buffer* m)
{
    struct StackEntry* s = StackTop(m);
    if (s->d.map.haveKey) {
        m->sigp = s->d.map.sigBegin;
        s->d.map.haveKey = 0;
    } else {
        s->d.map.haveKey = 1;
    }
}

// ----------------------------------------------------------------------------

int adbus_buf_endmap(adbus_Buffer* m)
{
    struct StackEntry* s = StackTop(m);
    uint32_t* psize = (uint32_t*) &kv_a(m->data, s->d.map.sizeIndex);
    *psize = (uint32_t)(kv_size(m->data) - s->d.map.dataBegin);

    CHECK(*psize < ADBUSI_MAXIMUM_ARRAY_LENGTH);

    m->sigp = adbusI_findArrayEnd(s->d.map.sigBegin - 1);
    assert(m->sigp);

    StackPop(m);

    AppendField(m);
    return 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int adbus_buf_beginstruct(adbus_Buffer* m)
{
    CHECK(*m->sigp == ADBUS_STRUCT_BEGIN);

    m->sigp++;

    AlignData(m, 8);

    struct StackEntry* s = StackPush(m);
    s->type = STRUCT_STACK;
    return 0;
}

// ----------------------------------------------------------------------------

static void AppendStructChild(adbus_Buffer* m)
{
    // Nothing to do
    (void) m;
}

// ----------------------------------------------------------------------------

int adbus_buf_endstruct(adbus_Buffer* m)
{
    CHECK(*m->sigp == ADBUS_STRUCT_END);

    m->sigp++;

    StackPop(m);

    AppendField(m);
    return 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int adbus_buf_beginvariant(adbus_Buffer* m, const char* type, int typeSize)
{
    CHECK(*m->sigp == ADBUS_VARIANT_BEGIN);

    if (typeSize < 0)
        typeSize = strlen(type);

    CHECK(typeSize < 256);

    // Set the variant type in the output buffer
    AppendShortString(m, type, typeSize);

    // Setup stack entry
    struct StackEntry* s = StackPush(m);
    s->type = VARIANT_STACK;
    s->d.variant.oldSigp = m->sigp;

    m->sigp = type;
    return 0;
}

// ----------------------------------------------------------------------------

static void AppendVariantChild(adbus_Buffer* m)
{
    // Nothing to do
    (void) m;
}

// ----------------------------------------------------------------------------

int adbus_buf_endvariant(adbus_Buffer* m)
{
    CHECK(*m->sigp == '\0');
    struct StackEntry* s = StackTop(m);
    m->sigp = s->d.variant.oldSigp;

    StackPop(m);

    AppendField(m);
    return 0;
}

// ----------------------------------------------------------------------------

int ADBusAppendVariant(adbus_Buffer* m,
                       const char* sig,
                       int         sigsize,
                       const uint8_t* data,
                       size_t datasize)
{
    if (sigsize < 0)
        sigsize = strlen(sig);

    CHECK(*m->sigp == ADBUS_VARIANT_BEGIN);
    CHECK(sigsize < 256);

    m->sigp++;

    // Set the variant type in the output buffer
    AppendShortString(m, sig, sigsize);

    // Append the data
    AlignData(m, adbusI_alignment(*sig));
    uint8_t* dest = kv_push(u8, m->data, datasize);
    memcpy(dest, data, datasize);

    AppendField(m);
    return 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void AppendField(adbus_Buffer* m)
{
    if (StackSize(m) == 0)
        return;

    switch(StackTop(m)->type)
    {
    case ARRAY_STACK:
        AppendArrayChild(m);
        break;
    case VARIANT_STACK:
        AppendVariantChild(m);
        break;
    case STRUCT_STACK:
        AppendStructChild(m);
        break;
    case MAP_STACK:
        AppendMapChild(m);
        break;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
