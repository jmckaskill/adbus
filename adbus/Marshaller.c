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

#include "Marshaller.h"
#include "Marshaller_p.h"

#include "Misc_p.h"

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static struct StackEntry* StackTop(struct ADBusMarshaller* m)
{
    return m->stack + stack_vector_size(&m->stack) - 1;
}

static struct StackEntry* StackPush(struct ADBusMarshaller* m)
{
    return stack_vector_insert_end(&m->stack, 1);
}

static void StackPop(struct ADBusMarshaller* m)
{
    struct StackEntry* s = StackTop(m);
    if (s->type == VariantStackEntry)
        free(s->d.variant.signature);
    stack_vector_remove_end(&m->stack, 1);
}

static size_t StackSize(struct ADBusMarshaller* m)
{
    return stack_vector_size(&m->stack);
}

static void AlignData(struct ADBusMarshaller* m, size_t align)
{
    size_t padding = ADBUS_ALIGN_VALUE_(size_t, u8vector_size(&m->data), align)
                   - u8vector_size(&m->data);
    if (padding > 0)
        u8vector_insert_end(&m->data, padding);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

struct ADBusMarshaller* ADBusCreateMarshaller()
{
    struct ADBusMarshaller* ret = NEW(struct ADBusMarshaller);
    ADBusResetMarshaller(ret);
    return ret;
}

// ----------------------------------------------------------------------------

void ADBusResetMarshaller(struct ADBusMarshaller* m)
{
    while(stack_vector_size(&m->stack) > 0)
        StackPop(m);

    u8vector_clear(&m->data);
    m->signature[0] = '\0';
    m->sigp = &m->signature[0];
}

// ----------------------------------------------------------------------------

void ADBusFreeMarshaller(struct ADBusMarshaller* m)
{
    if (!m)
        return;

    ADBusResetMarshaller(m);

    u8vector_free(&m->data);
    stack_vector_free(&m->stack);

    free(m);
}

// ----------------------------------------------------------------------------

void ADBusSetMarshalledData(struct ADBusMarshaller* m,
                            const char*     sig,
                            int             sigsize,
                            const uint8_t*  data,
                            size_t          dataSize)
{
    if (sigsize < 0)
        sigsize = strlen(sig);

    ASSERT_RETURN(sigsize < 256);

    ADBusResetMarshaller(m);

    strncat(m->signature, sig, sigsize);
    m->sigp = m->signature;

    uint8_t* dest = u8vector_insert_end(&m->data, dataSize);
    memcpy(dest, data, dataSize);
}

// ----------------------------------------------------------------------------

void ADBusGetMarshalledData(const struct ADBusMarshaller* m,
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
        *data = m->data;
    if (dataSize)
        *dataSize = u8vector_size(&m->data);
}

// ----------------------------------------------------------------------------

int ADBusAppendData(struct ADBusMarshaller* m, const uint8_t* data, size_t size)
{
    if (StackSize(m) != 0)
    {
        struct StackEntry* s = StackTop(m);
        ASSERT_RETURN_ERR(s->type == ArrayStackEntry
                          || s->type == VariantStackEntry);
    }

    uint8_t* dest = u8vector_insert_end(&m->data, size);
    memcpy(dest, data, size);

    return 0;
}

// ----------------------------------------------------------------------------

enum ADBusFieldType ADBusNextMarshallerField(struct ADBusMarshaller* m)
{
    if (m->sigp)
        return (enum ADBusFieldType) *m->sigp;
    else
        return ADBusInvalidField;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void AppendField(struct ADBusMarshaller* m);

static uint8_t* AppendFixedField(
        struct ADBusMarshaller*     m,
        enum ADBusFieldType         fieldtype,
        size_t                      size)
{
    if (*m->sigp != (char) fieldtype)
        return NULL;

    AlignData(m, size);

    uint8_t* data = u8vector_insert_end(&m->data, size);
    m->sigp++;
    AppendField(m);

    return data;
}

static int Append8(
        struct ADBusMarshaller*     m,
        enum ADBusFieldType         fieldtype,
        uint8_t                     data)
{
    uint8_t* pdata = AppendFixedField(m, fieldtype, sizeof(uint8_t));
    if (!pdata)
        return 1;
    *pdata = data;
    return 0;
}

static int Append16(
        struct ADBusMarshaller*     m,
        enum ADBusFieldType         fieldtype,
        uint16_t                    data)
{
    uint16_t* pdata = (uint16_t*) AppendFixedField(m, fieldtype, sizeof(uint16_t));
    if (!pdata)
        return 1;
    *pdata = data;
    return 0;
}

static int Append32(
        struct ADBusMarshaller*     m,
        enum ADBusFieldType         fieldtype,
        uint32_t                    data)
{
    uint32_t* pdata = (uint32_t*) AppendFixedField(m, fieldtype, sizeof(uint32_t));
    if (!pdata)
        return 1;
    *pdata = data;
    return 0;
}

static int Append64(
        struct ADBusMarshaller*     m,
        enum ADBusFieldType         fieldtype,
        uint64_t                    data)
{
    uint64_t* pdata = (uint64_t*) AppendFixedField(m, fieldtype, sizeof(uint64_t));
    if (!pdata)
        return 1;
    *pdata = data;
    return 0;
}

int ADBusAppendBoolean(struct ADBusMarshaller* m, uint32_t data)
{ return Append32(m, ADBusBooleanField, data ? 1 : 0); }

int ADBusAppendUInt8(struct ADBusMarshaller* m, uint8_t data)
{ return Append8(m, ADBusUInt8Field, data); }

int ADBusAppendInt16(struct ADBusMarshaller* m, int16_t data)
{ return Append16(m, ADBusInt16Field, *(uint16_t*)&data); }

int ADBusAppendUInt16(struct ADBusMarshaller* m, uint16_t data)
{ return Append16(m, ADBusUInt16Field, data); }

int ADBusAppendInt32(struct ADBusMarshaller* m, int32_t data)
{ return Append32(m, ADBusInt32Field, *(uint32_t*)&data); }

int ADBusAppendUInt32(struct ADBusMarshaller* m, uint32_t data)
{ return Append32(m, ADBusUInt32Field, data); }

int ADBusAppendInt64(struct ADBusMarshaller* m, int64_t data)
{ return Append64(m, ADBusInt64Field, *(uint64_t*)&data); }

int ADBusAppendUInt64(struct ADBusMarshaller* m, uint64_t data)
{ return Append64(m, ADBusUInt64Field, data); }

int ADBusAppendDouble(struct ADBusMarshaller* m, double data)
{
    // Take the safe route to avoid any possible aliasing problems
    // *(uint64_t*) &data won't compile with strict aliasing on gcc
    uint64_t d = 0;
    for (size_t i = 0; i < 8; ++i)
        ((uint8_t*) &d)[i] = ((uint8_t*) &data)[i];

    return Append64(m, ADBusDoubleField, d);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static int AppendShortString(struct ADBusMarshaller* m, const char* str, int size)
{
    if (size < 0)
        size = (int)strlen(str);

    ASSERT_RETURN_ERR(size < 256);

    uint8_t* psize  = u8vector_insert_end(&m->data, 1 + size + 1);
    char* pstr      = (char*) psize + 1;

    *psize = (uint8_t) size;
    memcpy(pstr, str, size);
    pstr[size] = '\0';

    m->sigp++;

    AppendField(m);

    return 0;
}

static int AppendLongString(struct ADBusMarshaller* m, const char* str, int size)
{
    if (size < 0)
        size = (int)strlen(str);

    AlignData(m, 4);

    uint8_t* pdata  = u8vector_insert_end(&m->data, 4 + size + 1);
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

int ADBusAppendString(struct ADBusMarshaller* m, const char* str, int size)
{
    ASSERT_RETURN_ERR(*m->sigp == ADBusStringField);
    return AppendLongString(m, str, size);
}

// ----------------------------------------------------------------------------

int ADBusAppendObjectPath(struct ADBusMarshaller* m, const char* str, int size)
{
    ASSERT_RETURN_ERR(*m->sigp == ADBusObjectPathField);
    return AppendLongString(m, str, size);
}

// ----------------------------------------------------------------------------

int ADBusAppendSignature(struct ADBusMarshaller* m, const char* str, int size)
{
    ASSERT_RETURN_ERR(*m->sigp == ADBusSignatureField);
    return AppendShortString(m, str, size);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int ADBusBeginArgument(struct ADBusMarshaller* m, const char* sig, int size)
{
    if (size < 0)
        size = strlen(sig);

    ASSERT_RETURN_ERR(strlen(m->signature) + size < 256);
    ASSERT_RETURN_ERR(StackSize(m) == 0);
    ASSERT_RETURN_ERR(m->sigp == NULL || *m->sigp == '\0');

    strncat(m->signature, sig, size);

    return 0;
}

// ----------------------------------------------------------------------------

int ADBusEndArgument(struct ADBusMarshaller* m)
{
    ASSERT_RETURN_ERR(StackSize(m) == 0);
    return 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int ADBusBeginArray(struct ADBusMarshaller* m)
{
    ASSERT_RETURN_ERR(*m->sigp == ADBusArrayBeginField);

    m->sigp++;

    AlignData(m, 4);
    size_t isize = u8vector_size(&m->data);
    u8vector_insert_end(&m->data, 4);
    AlignData(m, ADBusRequiredAlignment_(*m->sigp));
    size_t idata = u8vector_size(&m->data);

    struct StackEntry* s = StackPush(m);
    s->type = ArrayStackEntry;
    s->d.array.sizeIndex    = isize;
    s->d.array.dataBegin    = idata;
    s->d.array.sigBegin     = m->sigp;

    return 0;
}

// ----------------------------------------------------------------------------

static void AppendArrayChild(struct ADBusMarshaller* m)
{
    struct StackEntry* s = StackTop(m);
    m->sigp = s->d.array.sigBegin;
}

// ----------------------------------------------------------------------------

int ADBusEndArray(struct ADBusMarshaller* m)
{
    struct StackEntry* s = StackTop(m);
    uint32_t* psize = (uint32_t*)&m->data[s->d.array.sizeIndex];
    *psize = (uint32_t)(u8vector_size(&m->data) - s->d.array.dataBegin);

    ASSERT_RETURN_ERR(*psize < ADBusMaximumArrayLength);

    m->sigp = ADBusFindArrayEnd_(s->d.array.sigBegin);
    assert(m->sigp);

    StackPop(m);

    AppendField(m);
    return 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int ADBusBeginStruct(struct ADBusMarshaller* m)
{
    ASSERT_RETURN_ERR(*m->sigp == ADBusStructBeginField);

    m->sigp++;

    AlignData(m, 8);

    struct StackEntry* s = StackPush(m);
    s->type = StructStackEntry;
    return 0;
}

// ----------------------------------------------------------------------------

static void AppendStructChild(struct ADBusMarshaller* m)
{
    // Nothing to do
    (void) m;
}

// ----------------------------------------------------------------------------

int ADBusEndStruct(struct ADBusMarshaller* m)
{
    ASSERT_RETURN_ERR(*m->sigp == ADBusStructEndField);

    m->sigp++;

    StackPop(m);

    AppendField(m);
    return 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int ADBusBeginDictEntry(struct ADBusMarshaller* m)
{
    ASSERT_RETURN_ERR(*m->sigp == ADBusDictEntryBeginField);

    m->sigp++;
    AlignData(m, 8);

    struct StackEntry* s = StackPush(m);
    s->type = DictEntryStackEntry;
    return 0;
}

// ----------------------------------------------------------------------------

static void AppendDictEntryChild(struct ADBusMarshaller* m)
{
    // Nothing to do
    (void) m;
}

// ----------------------------------------------------------------------------

int ADBusEndDictEntry(struct ADBusMarshaller* m)
{
    ASSERT_RETURN_ERR(*m->sigp == ADBusDictEntryEndField);

    m->sigp++;

    StackPop(m);

    AppendField(m);
    return 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int ADBusBeginVariant(struct ADBusMarshaller* m, const char* type, int typeSize)
{
    ASSERT_RETURN_ERR(*m->sigp == ADBusVariantBeginField);

    if (typeSize < 0)
        typeSize = strlen(type);

    ASSERT_RETURN_ERR(typeSize < 256);

    // Set the variant type in the output buffer
    AppendShortString(m, type, typeSize);

    // Setup stack entry
    struct StackEntry* s = StackPush(m);
    s->type = VariantStackEntry;
    s->d.variant.oldSigp = m->sigp;

    m->sigp = type;
    return 0;
}

// ----------------------------------------------------------------------------

static void AppendVariantChild(struct ADBusMarshaller* m)
{
    // Nothing to do
    (void) m;
}

// ----------------------------------------------------------------------------

int ADBusEndVariant(struct ADBusMarshaller* m)
{
    ASSERT_RETURN_ERR(*m->sigp == '\0');
    struct StackEntry* s = StackTop(m);
    m->sigp = s->d.variant.oldSigp;

    StackPop(m);

    AppendField(m);
    return 0;
}

// ----------------------------------------------------------------------------

int ADBusAppendVariant(struct ADBusMarshaller* m,
                       const char* sig,
                       int         sigsize,
                       const uint8_t* data,
                       size_t datasize)
{
    if (sigsize < 0)
        sigsize = strlen(sig);

    ASSERT_RETURN_ERR(*m->sigp == ADBusVariantBeginField);
    ASSERT_RETURN_ERR(sigsize < 256);

    m->sigp++;

    // Set the variant type in the output buffer
    AppendShortString(m, sig, sigsize);

    // Append the data
    AlignData(m, ADBusRequiredAlignment_(*sig));
    uint8_t* dest = u8vector_insert_end(&m->data, datasize);
    memcpy(dest, data, datasize);

    AppendField(m);
    return 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void AppendField(struct ADBusMarshaller* m)
{
    if (StackSize(m) == 0)
        return;

    switch(StackTop(m)->type)
    {
    case ArrayStackEntry:
        return AppendArrayChild(m);
    case VariantStackEntry:
        return AppendVariantChild(m);
    case DictEntryStackEntry:
        return AppendDictEntryChild(m);
    case StructStackEntry:
        return AppendStructChild(m);
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
