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
#include "iterator.h"
#include "misc.h"

#include "memory/kvector.h"

#include <assert.h>
#include <string.h>

// ----------------------------------------------------------------------------
// Helper Functions
// ----------------------------------------------------------------------------

static struct StackEntry* StackTop(adbus_Iterator *i)
{
    return &kv_a(i->stack, kv_size(i->stack) - 1);
}

static struct StackEntry* PushStackEntry(adbus_Iterator *i)
{
    return kv_push(Stack, i->stack, 1);
}

static void PopStackEntry(adbus_Iterator *i)
{
    kv_pop(Stack, i->stack, 1);
}

// ----------------------------------------------------------------------------

static size_t DataRemaining(adbus_Iterator *i)
{
    return i->dataEnd - i->data;
}

static const uint8_t* GetData(adbus_Iterator *i, size_t size)
{
    assert(DataRemaining(i) >= size);
    const uint8_t* ret = i->data;
    i->data += size;
    return ret;
}

// ----------------------------------------------------------------------------


static uint8_t Get8BitData(adbus_Iterator *i)
{
    return *GetData(i, sizeof(uint8_t));
}

static uint16_t Get16BitData(adbus_Iterator *i)
{
    uint16_t data = *(uint16_t*)GetData(i, sizeof(uint16_t));
    if (i->alternateEndian)
        data = ADBUSI_FLIP16(data);

    return data;
}

static uint32_t Get32BitData(adbus_Iterator *i)
{
    uint32_t data = *(uint32_t*)GetData(i, sizeof(uint32_t));
    if (i->alternateEndian)
        data = ADBUSI_FLIP32(data);

    return data;
}

static uint64_t Get64BitData(adbus_Iterator *i)
{
    uint64_t data = *(uint64_t*)GetData(i, sizeof(uint64_t));
    if (i->alternateEndian)
        data = ADBUSI_FLIP64(data);

    return data;
}

// ----------------------------------------------------------------------------

#ifndef NDEBUG
static adbus_Bool IsAligned(adbus_Iterator *i)
{
    return (((uintptr_t)(i->data)) % adbusI_alignment(*i->signature)) == 0;
}
#endif

static void ProcessAlignment(adbus_Iterator *i)
{
    size_t padding = adbusI_alignment(*i->signature);
    i->data = (const uint8_t*) ADBUSI_ALIGN(i->data, padding);
}






// ----------------------------------------------------------------------------
// Private API
// ----------------------------------------------------------------------------

static int ProcessField(adbus_Iterator *i, adbus_Field* f);

// ----------------------------------------------------------------------------
// Simple types
// ----------------------------------------------------------------------------

static int Process8Bit(adbus_Iterator *i, adbus_Field* f, uint8_t* fieldData)
{
    assert(IsAligned(i));

    CHECK(DataRemaining(i) >= 1);

    f->type = (adbus_FieldType)(*i->signature);
    *fieldData = Get8BitData(i);

    i->signature += 1;

    return 0;
}

// ----------------------------------------------------------------------------

static int Process16Bit(adbus_Iterator *i, adbus_Field* f, uint16_t* fieldData)
{
    assert(IsAligned(i));

    CHECK(DataRemaining(i) >= 2);

    f->type = (adbus_FieldType)(*i->signature);
    *fieldData = Get16BitData(i);

    i->signature += 1;

    return 0;
}

// ----------------------------------------------------------------------------

static int Process32Bit(adbus_Iterator *i, adbus_Field* f, uint32_t* fieldData)
{
    assert(IsAligned(i));

    CHECK(DataRemaining(i) >= 4);

    f->type = (adbus_FieldType)(*i->signature);
    *fieldData = Get32BitData(i);

    i->signature += 1;

    return 0;
}

// ----------------------------------------------------------------------------

static int Process64Bit(adbus_Iterator *i, adbus_Field* f, uint64_t* fieldData)
{
    assert(IsAligned(i));

    CHECK(DataRemaining(i) >= 8);

    f->type = (adbus_FieldType)(*i->signature);
    *fieldData = Get64BitData(i);

    i->signature += 1;

    return 0;
}

// ----------------------------------------------------------------------------

static int ProcessBoolean(adbus_Iterator *i, adbus_Field* f)
{
    CHECK(!Process32Bit(i, f, &f->b));

    CHECK(f->b == 0 || f->b == 1);

    f->type = ADBUS_BOOLEAN;
    f->b = f->b ? 1 : 0;

    return 0;
}

// ----------------------------------------------------------------------------
// String types
// ----------------------------------------------------------------------------

static int ProcessStringData(adbus_Iterator *i, adbus_Field* f)
{
    size_t size = f->size;
    CHECK(DataRemaining(i) >= size + 1);

    const uint8_t* str = GetData(i, size + 1);
    CHECK(!adbusI_hasNullByte(str, size));

    CHECK(*(str + size) == '\0');

    CHECK(adbusI_isValidUtf8(str, size));

    f->string = (const char*) str;

    i->signature += 1;

    return 0;
}

// ----------------------------------------------------------------------------

static int ProcessObjectPath(adbus_Iterator *i, adbus_Field* f)
{
    assert(IsAligned(i));
    CHECK(DataRemaining(i) >= 4);

    f->type = ADBUS_OBJECT_PATH;
    f->size = Get32BitData(i);

    CHECK(!ProcessStringData(i,f));

    CHECK(adbusI_isValidObjectPath(f->string, f->size));

    return 0;
}

// ----------------------------------------------------------------------------

static int ProcessString(adbus_Iterator *i, adbus_Field* f)
{
    assert(IsAligned(i));
    CHECK(DataRemaining(i) >= 4);

    f->type = ADBUS_STRING;
    f->size = Get32BitData(i);

    return ProcessStringData(i,f);
}

// ----------------------------------------------------------------------------

static int ProcessSignature(adbus_Iterator *i, adbus_Field* f)
{
    assert(IsAligned(i));
    CHECK(DataRemaining(i) >= 1);

    f->type = ADBUS_SIGNATURE;
    f->size = Get8BitData(i);

    return ProcessStringData(i,f);
}


// ----------------------------------------------------------------------------
// Root functions
// ----------------------------------------------------------------------------

static int NextRootField(adbus_Iterator *i, adbus_Field* f)
{
    if (i->signature == NULL || *i->signature == ADBUS_END_FIELD)
    {
        f->type = ADBUS_END_FIELD;
        return 0;
    }

    ProcessAlignment(i);
    return ProcessField(i,f);
}

// ----------------------------------------------------------------------------

static adbus_Bool IsRootAtEnd(adbus_Iterator *i)
{
    return i->signature == NULL || *i->signature == ADBUS_END_FIELD;
}

// ----------------------------------------------------------------------------
// Struct functions
// ----------------------------------------------------------------------------

static int ProcessStruct(adbus_Iterator *i, adbus_Field* f)
{
    assert(IsAligned(i));

    struct StackEntry* e = PushStackEntry(i);
    e->type = STRUCT_STACK;

    i->signature += 1; // skip over '('

    f->type = ADBUS_STRUCT_BEGIN;
    f->scope = kv_size(i->stack);

    return 0;
}

// ----------------------------------------------------------------------------

static int NextStructField(adbus_Iterator *i, adbus_Field* f)
{
    if (*i->signature != ')')
    {
        ProcessAlignment(i);
        return ProcessField(i,f);
    }

    PopStackEntry(i);

    i->signature += 1; // skip over ')'

    f->type = ADBUS_STRUCT_END;

    return 0;
}

// ----------------------------------------------------------------------------

static adbus_Bool IsStructAtEnd(adbus_Iterator *i)
{
    return *i->signature == ')';
}

// ----------------------------------------------------------------------------
// Map functions
// ----------------------------------------------------------------------------

static int ProcessMap(adbus_Iterator* i, adbus_Field* f)
{
    assert(IsAligned(i));
    CHECK(DataRemaining(i) >= 4);
    size_t size = Get32BitData(i);

    CHECK(size <= ADBUSI_MAXIMUM_ARRAY_LENGTH);
    CHECK(size <= DataRemaining(i));

    i->signature += 2; // skip over 'a{'

    i->data = (const uint8_t*) ADBUSI_ALIGN(i->data, 8);

    struct StackEntry* e = PushStackEntry(i);
    e->type = MAP_STACK;
    e->data.map.dataEnd = i->data + size;
    e->data.map.typeBegin = i->signature;
    e->data.map.haveKey = 0;

    f->type   = ADBUS_MAP_BEGIN;
    f->data   = i->data;
    f->size   = size;
    f->scope  = kv_size(i->stack);

    return 0;
}

// ----------------------------------------------------------------------------

static int NextMapField(adbus_Iterator* i, adbus_Field* f)
{
    struct StackEntry* e = StackTop(i);
    CHECK(i->data <= e->data.map.dataEnd);

    if (i->data < e->data.map.dataEnd)
    {
        if (e->data.map.haveKey) {
            e->data.map.haveKey = 0;
            i->signature = e->data.map.typeBegin;
            ProcessAlignment(i);
            return ProcessField(i,f);
        } else {
            e->data.map.haveKey = 1;
            ProcessAlignment(i);
            return ProcessField(i,f);
        }
    }

    i->signature = adbusI_findArrayEnd(e->data.map.typeBegin - 1);
    CHECK(i->signature != NULL);

    f->type = ADBUS_MAP_END;
    PopStackEntry(i);
    return 0;
}

// ----------------------------------------------------------------------------

static adbus_Bool IsMapAtEnd(adbus_Iterator* i)
{
    struct StackEntry* e = StackTop(i);
    return i->data >= e->data.map.dataEnd;
}

// ----------------------------------------------------------------------------
// Array functions
// ----------------------------------------------------------------------------

static int ProcessArray(adbus_Iterator *i, adbus_Field* f)
{
    assert(IsAligned(i));
    CHECK(DataRemaining(i) >= 4);
    size_t size = Get32BitData(i);

    CHECK(size <= ADBUSI_MAXIMUM_ARRAY_LENGTH);
    CHECK(size <= DataRemaining(i));

    i->signature += 1; // skip over 'a'

    ProcessAlignment(i);

    struct StackEntry* e = PushStackEntry(i);
    e->type = ARRAY_STACK;
    e->data.array.dataEnd = i->data + size;
    e->data.array.typeBegin = i->signature;

    f->type   = ADBUS_ARRAY_BEGIN;
    f->data   = i->data;
    f->size   = size;
    f->scope  = kv_size(i->stack);

    return 0;
}

// ----------------------------------------------------------------------------

static int NextArrayField(adbus_Iterator *i, adbus_Field* f)
{
    struct StackEntry* e = StackTop(i);
    CHECK(i->data <= e->data.array.dataEnd);

    if (i->data < e->data.array.dataEnd)
    {
        i->signature = e->data.array.typeBegin;
        ProcessAlignment(i);
        return ProcessField(i,f);
    }

    i->signature = adbusI_findArrayEnd(e->data.array.typeBegin);
    CHECK(i->signature != NULL);

    f->type = ADBUS_ARRAY_END;
    PopStackEntry(i);
    return 0;
}

// ----------------------------------------------------------------------------

static adbus_Bool IsArrayAtEnd(adbus_Iterator *i)
{
    struct StackEntry* e = StackTop(i);
    return i->data >= e->data.array.dataEnd;
}

// ----------------------------------------------------------------------------
// Variant functions
// ----------------------------------------------------------------------------

static int ProcessVariant(adbus_Iterator *i, adbus_Field* f)
{
    assert(IsAligned(i));
    CHECK(!ProcessSignature(i,f));

    // ProcessSignature has alread filled out f->variantType and
    // has consumed the field in i->signature

    struct StackEntry* e = PushStackEntry(i);
    e->type = VARIANT_STACK;
    e->data.variant.oldSignature = i->signature;
    e->data.variant.seenFirst = 0;

    f->type = ADBUS_VARIANT_BEGIN;
    f->scope = kv_size(i->stack);

    i->signature = f->string;

    return 0;
}

// ----------------------------------------------------------------------------

static int NextVariantField(adbus_Iterator *i, adbus_Field* f)
{
    struct StackEntry* e = StackTop(i);
    if (!e->data.variant.seenFirst)
    {
        e->data.variant.seenFirst = 1;
        ProcessAlignment(i);
        return ProcessField(i,f);
    }

    // there's more than one root type in the variant type
    CHECK(*i->signature == '\0');

    i->signature = e->data.variant.oldSignature;

    PopStackEntry(i);

    f->type = ADBUS_VARIANT_END;

    return 0;
}

// ----------------------------------------------------------------------------

static adbus_Bool IsVariantAtEnd(adbus_Iterator *i)
{
    return *i->signature == '\0';
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static int ProcessField(adbus_Iterator *i, adbus_Field* f)
{
    f->type = ADBUS_END_FIELD;
    switch(*i->signature)
    {
        case ADBUS_UINT8:
            return Process8Bit(i,f,(uint8_t*)&f->u8);
        case ADBUS_BOOLEAN:
            return ProcessBoolean(i,f);
        case ADBUS_INT16:
            return Process16Bit(i,f,(uint16_t*)&f->i16);
        case ADBUS_UINT16:
            return Process16Bit(i,f,(uint16_t*)&f->u16);
        case ADBUS_INT32:
            return Process32Bit(i,f,(uint32_t*)&f->i32);
        case ADBUS_UINT32:
            return Process32Bit(i,f,(uint32_t*)&f->u32);
        case ADBUS_INT64:
            return Process64Bit(i,f,(uint64_t*)&f->i64);
        case ADBUS_UINT64:
            return Process64Bit(i,f,(uint64_t*)&f->u64);
        case ADBUS_DOUBLE:
            return Process64Bit(i,f,(uint64_t*)&f->d);
        case ADBUS_STRING:
            return ProcessString(i,f);
        case ADBUS_OBJECT_PATH:
            return ProcessObjectPath(i,f);
        case ADBUS_SIGNATURE:
            return ProcessSignature(i,f);
        case ADBUS_ARRAY_BEGIN:
            if (*(i->signature + 1) == '{')
                return ProcessMap(i, f);
            else
                return ProcessArray(i,f);
        case ADBUS_STRUCT_BEGIN:
            return ProcessStruct(i,f);
        case ADBUS_VARIANT_BEGIN:
            return ProcessVariant(i,f);
        default:
            assert(0);
            return -1;
    }
}

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

adbus_Iterator* adbus_iter_new(void)
{
    adbus_Iterator* i = NEW(adbus_Iterator);
    i->stack = kv_new(Stack);
    return i;
}

// ----------------------------------------------------------------------------

void adbus_iter_free(adbus_Iterator* i)
{
    if (i) {
        kv_free(Stack, i->stack);
        free(i);
    }
}

// ----------------------------------------------------------------------------

void adbus_iter_reset(adbus_Iterator* i,
        const char*     sig,
        int             sigsize,
        const uint8_t*  data,
        size_t          datasize)
{
    (void) sigsize;

    i->data = data;
    i->dataEnd = data + datasize;
    i->alternateEndian = 0;

    i->signature = sig;
    kv_clear(Stack, i->stack);
}

// ----------------------------------------------------------------------------

void adbus_iter_setnonnative(
        adbus_Iterator*       i)
{
    i->alternateEndian = 1;
}

// ----------------------------------------------------------------------------

void ADBusGetIteratorData(
        const adbus_Iterator* i,
        const char**                sig,
        size_t*                     sigsize,
        const uint8_t**             data,
        size_t*                     datasize)
{
    if (sig)
        *sig = i->signature;
    if (sigsize)
        *sigsize = strlen(i->signature);
    if (data)
        *data = i->data;
    if (datasize)
        *datasize = i->dataEnd - i->data;
}

// ----------------------------------------------------------------------------

int adbus_iter_isfinished(adbus_Iterator *i, int scope)
{
    CHECK(kv_size(i->stack) >= scope);

    if ((int) kv_size(i->stack) > scope)
        return 0;

    if (kv_size(i->stack) == 0)
        return IsRootAtEnd(i);

    switch(StackTop(i)->type)
    {
        case VARIANT_STACK:
            return IsVariantAtEnd(i);
        case ARRAY_STACK:
            return IsArrayAtEnd(i);
        case STRUCT_STACK:
            return IsStructAtEnd(i);
        case MAP_STACK:
            return IsMapAtEnd(i);
        default:
            assert(0);
            return 1;
    }
}

// ----------------------------------------------------------------------------

int adbus_iter_next(adbus_Iterator *i, adbus_Field* f)
{
#ifndef NDEBUG
    memset(f, 0xDE, sizeof(adbus_Field));
#endif
    if (kv_size(i->stack) == 0)
    {
        return NextRootField(i,f);
    }
    else
    {
        switch(StackTop(i)->type)
        {
            case VARIANT_STACK:
                return NextVariantField(i,f);
            case ARRAY_STACK:
                return NextArrayField(i,f);
            case STRUCT_STACK:
                return NextStructField(i,f);
            case MAP_STACK:
                return NextMapField(i,f);
            default:
                assert(0);
                return -1;
        }
    }
}

// ----------------------------------------------------------------------------

int adbus_iter_arrayjump(adbus_Iterator* i, int scope)
{
    CHECK(kv_size(i->stack) >= scope);

    kv_pop(Stack, i->stack, kv_size(i->stack) - scope);

    struct StackEntry* e = StackTop(i);
    CHECK(e->type == ARRAY_STACK);

    i->data = e->data.array.dataEnd;
    return 0;
}




// ----------------------------------------------------------------------------
// Check functions
// ----------------------------------------------------------------------------

static void CheckField(
        adbus_CbData*    d,
        adbus_Field*          field,
        adbus_FieldType         type)
{
    if (adbus_iter_next(d->args, field)) {
        longjmp(d->jmpbuf, ADBUSI_PARSE_ERROR);
    }

    if (field->type != type) {
        adbusI_argumentError(d);
    }
}

void adbus_check_end(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_END_FIELD);
}

adbus_Bool adbus_check_bool(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_BOOLEAN);
    return f.b;
}

uint8_t adbus_check_uint8(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_UINT8);
    return f.u8;
}

int16_t adbus_check_int16(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_INT16);
    return f.i16;
}

uint16_t adbus_check_uint16(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_UINT16);
    return f.u16;
}

int32_t adbus_check_int32(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_INT32);
    return f.i32;
}

uint32_t adbus_check_uint32(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_UINT32);
    return f.u32;
}

int64_t adbus_check_int64(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_INT64);
    return f.i64;
}

uint64_t adbus_check_uint64(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_UINT64);
    return f.u64;
}

double adbus_check_double(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_DOUBLE);
    return f.d;
}

const char* adbus_check_string(adbus_CbData* d, size_t* size)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_STRING);
    if (size)
        *size = f.size;
    return f.string;
}

const char* adbus_check_objectpath(adbus_CbData* d, size_t* size)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_OBJECT_PATH);
    if (size)
        *size = f.size;
    return f.string;
}

const char* adbus_check_signature(adbus_CbData* d, size_t* size)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_SIGNATURE);
    if (size)
        *size = f.size;
    return f.string;
}

int adbus_check_arraybegin(adbus_CbData* d, const uint8_t** data, size_t* size)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_ARRAY_BEGIN);
    if (data)
        *data = f.data;
    if (size)
        *size = f.size;
    return f.scope;
}

void adbus_check_arrayend(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_ARRAY_END);
}

int adbus_check_structbegin(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_STRUCT_BEGIN);
    return f.scope;
}

void adbus_check_structend(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_STRUCT_END);
}

int adbus_check_mapbegin(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_MAP_BEGIN);
    return f.scope;
}

void adbus_check_mapend(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_MAP_END);
}

const char* adbus_check_variantbegin(adbus_CbData* d, size_t* size)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_VARIANT_BEGIN);
    if (size)
        *size = f.size;
    return f.string;
}

void adbus_check_variantend(adbus_CbData* d)
{
    adbus_Field f;
    CheckField(d, &f, ADBUS_VARIANT_END);
}




