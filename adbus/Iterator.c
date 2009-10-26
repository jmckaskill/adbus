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

#include "Iterator.h"
#include "Iterator_p.h"
#include "Misc_p.h"
#include "Connection.h"
#include "CommonMessages.h"
#include "memory/kvector.h"

#include <assert.h>
#include <string.h>

// ----------------------------------------------------------------------------
// Helper Functions
// ----------------------------------------------------------------------------

static struct StackEntry* StackTop(struct ADBusIterator *i)
{
    return &kv_a(i->stack, kv_size(i->stack) - 1);
}

static struct StackEntry* PushStackEntry(struct ADBusIterator *i)
{
    return kv_push(Stack, i->stack, 1);
}

static void PopStackEntry(struct ADBusIterator *i)
{
    kv_pop(Stack, i->stack, 1);
}

// ----------------------------------------------------------------------------

static size_t DataRemaining(struct ADBusIterator *i)
{
    return i->dataEnd - i->data;
}

static const uint8_t* GetData(struct ADBusIterator *i, size_t size)
{
    assert(DataRemaining(i) >= size);
    const uint8_t* ret = i->data;
    i->data += size;
    return ret;
}

// ----------------------------------------------------------------------------


static uint8_t Get8BitData(struct ADBusIterator *i)
{
    return *GetData(i, sizeof(uint8_t));
}

static uint16_t Get16BitData(struct ADBusIterator *i)
{
    uint16_t data = *(uint16_t*)GetData(i, sizeof(uint16_t));
    if (i->alternateEndian)
        data = ENDIAN_CONVERT16(data);

    return data;
}

static uint32_t Get32BitData(struct ADBusIterator *i)
{
    uint32_t data = *(uint32_t*)GetData(i, sizeof(uint32_t));
    if (i->alternateEndian)
        data = ENDIAN_CONVERT32(data);

    return data;
}

static uint64_t Get64BitData(struct ADBusIterator *i)
{
    uint64_t data = *(uint64_t*)GetData(i, sizeof(uint64_t));
    if (i->alternateEndian)
        data = ENDIAN_CONVERT64(data);

    return data;
}

// ----------------------------------------------------------------------------

#ifndef NDEBUG
static uint IsAligned(struct ADBusIterator *i)
{
    return (((uintptr_t)(i->data)) % ADBusRequiredAlignment_(*i->signature)) == 0;
}
#endif

static void ProcessAlignment(struct ADBusIterator *i)
{
    size_t padding = ADBusRequiredAlignment_(*i->signature);
    i->data = ADBUS_ALIGN_VALUE_(uint8_t*, i->data, padding);
}






// ----------------------------------------------------------------------------
// Private API
// ----------------------------------------------------------------------------

static int ProcessField(struct ADBusIterator *i, struct ADBusField* f);

// ----------------------------------------------------------------------------
// Simple types
// ----------------------------------------------------------------------------

static int Process8Bit(struct ADBusIterator *i, struct ADBusField* f, uint8_t* fieldData)
{
    assert(IsAligned(i));

    if (DataRemaining(i) < 1)
        return ADBusInvalidData;

    f->type = (enum ADBusFieldType)(*i->signature);
    *fieldData = Get8BitData(i);

    i->signature += 1;

    return 0;
}

// ----------------------------------------------------------------------------

static int Process16Bit(struct ADBusIterator *i, struct ADBusField* f, uint16_t* fieldData)
{
    assert(IsAligned(i));

    if (DataRemaining(i) < 2)
        return ADBusInvalidData;

    f->type = (enum ADBusFieldType)(*i->signature);
    *fieldData = Get16BitData(i);

    i->signature += 1;

    return 0;
}

// ----------------------------------------------------------------------------

static int Process32Bit(struct ADBusIterator *i, struct ADBusField* f, uint32_t* fieldData)
{
    assert(IsAligned(i));

    if (DataRemaining(i) < 4)
        return ADBusInvalidData;

    f->type = (enum ADBusFieldType)(*i->signature);
    *fieldData = Get32BitData(i);

    i->signature += 1;

    return 0;
}

// ----------------------------------------------------------------------------

static int Process64Bit(struct ADBusIterator *i, struct ADBusField* f, uint64_t* fieldData)
{
    assert(IsAligned(i));

    if (DataRemaining(i) < 8)
        return ADBusInvalidData;

    f->type = (enum ADBusFieldType)(*i->signature);
    *fieldData = Get64BitData(i);

    i->signature += 1;

    return 0;
}

// ----------------------------------------------------------------------------

static int ProcessBoolean(struct ADBusIterator *i, struct ADBusField* f)
{
    int err = Process32Bit(i, f, &f->b);
    if (err)
        return err;

    if (f->b > 1)
        return ADBusInvalidData;

    f->type = ADBusBooleanField;
    f->b = f->b ? 1 : 0;

    return 0;
}

// ----------------------------------------------------------------------------
// String types
// ----------------------------------------------------------------------------

static int ProcessStringData(struct ADBusIterator *i, struct ADBusField* f)
{
    size_t size = f->size;
    if (DataRemaining(i) < size + 1)
        return ADBusInvalidData;

    const uint8_t* str = GetData(i, size + 1);
    if (ADBusHasNullByte_(str, size))
        return ADBusInvalidData;

    if (*(str + size) != '\0')
        return ADBusInvalidData;

    if (!ADBusIsValidUtf8_(str, size))
        return ADBusInvalidData;

    f->string = (const char*) str;

    i->signature += 1;

    return 0;
}

// ----------------------------------------------------------------------------

static int ProcessObjectPath(struct ADBusIterator *i, struct ADBusField* f)
{
    assert(IsAligned(i));
    if (DataRemaining(i) < 4)
        return ADBusInvalidData;

    f->type = ADBusObjectPathField;
    f->size = Get32BitData(i);

    int err = ProcessStringData(i,f);
    if (err)
        return err;

    if (!ADBusIsValidObjectPath_(f->string, f->size))
        return ADBusInvalidData;

    return 0;
}

// ----------------------------------------------------------------------------

static int ProcessString(struct ADBusIterator *i, struct ADBusField* f)
{
    assert(IsAligned(i));
    if (DataRemaining(i) < 4)
        return ADBusInvalidData;

    f->type = ADBusStringField;
    f->size = Get32BitData(i);

    return ProcessStringData(i,f);
}

// ----------------------------------------------------------------------------

static int ProcessSignature(struct ADBusIterator *i, struct ADBusField* f)
{
    assert(IsAligned(i));
    if (DataRemaining(i) < 1)
        return ADBusInvalidData;

    f->type = ADBusSignatureField;
    f->size = Get8BitData(i);

    return ProcessStringData(i,f);
}


// ----------------------------------------------------------------------------
// Root functions
// ----------------------------------------------------------------------------

static int NextRootField(struct ADBusIterator *i, struct ADBusField* f)
{
    if (i->signature == NULL || *i->signature == ADBusEndField)
    {
        f->type = ADBusEndField;
        return (i->dataEnd != i->data) ? -1 : 0;
    }

    ProcessAlignment(i);
    return ProcessField(i,f);
}

// ----------------------------------------------------------------------------

static uint IsRootAtEnd(struct ADBusIterator *i)
{
    return i->signature == NULL || *i->signature == ADBusEndField;
}

// ----------------------------------------------------------------------------
// Struct functions
// ----------------------------------------------------------------------------

static int ProcessStruct(struct ADBusIterator *i, struct ADBusField* f)
{
    assert(IsAligned(i));

    if (DataRemaining(i) == 0)
        return ADBusInvalidData;

    struct StackEntry* e = PushStackEntry(i);
    e->type = StructStackEntry;

    i->signature += 1; // skip over '('

    f->type = ADBusStructBeginField;
    f->scope = kv_size(i->stack);

    return 0;
}

// ----------------------------------------------------------------------------

static int NextStructField(struct ADBusIterator *i, struct ADBusField* f)
{
    if (*i->signature != ')')
    {
        ProcessAlignment(i);
        return ProcessField(i,f);
    }

    PopStackEntry(i);

    i->signature += 1; // skip over ')'

    f->type = ADBusStructEndField;

    return 0;
}

// ----------------------------------------------------------------------------

static uint IsStructAtEnd(struct ADBusIterator *i)
{
    return *i->signature == ')';
}

// ----------------------------------------------------------------------------
// Dict entry functions
// ----------------------------------------------------------------------------

static int ProcessDictEntry(struct ADBusIterator *i, struct ADBusField* f)
{
    assert(IsAligned(i));

    struct StackEntry* e = PushStackEntry(i);
    e->type = DictEntryStackEntry;
    e->data.dictEntryFields = 0;

    i->signature += 1; // skip over '{'

    f->type = ADBusDictEntryBeginField;
    f->scope = kv_size(i->stack);

    return 0;
}

// ----------------------------------------------------------------------------

static int NextDictEntryField(struct ADBusIterator *i, struct ADBusField* f)
{
    struct StackEntry* e = StackTop(i);
    if (*i->signature != '}')
    {
        ProcessAlignment(i);
        if (++(e->data.dictEntryFields) > 2)
            return ADBusInvalidData;
        else
            return ProcessField(i,f);
    }

    PopStackEntry(i);

    i->signature += 1; // skip over '}'

    f->type = ADBusDictEntryEndField;

    return 0;
}

// ----------------------------------------------------------------------------

static uint IsDictEntryAtEnd(struct ADBusIterator *i)
{
    return *i->signature == '}';
}

// ----------------------------------------------------------------------------
// Array functions
// ----------------------------------------------------------------------------

static int ProcessArray(struct ADBusIterator *i, struct ADBusField* f)
{
    assert(IsAligned(i));
    if (DataRemaining(i) < 4)
        return ADBusInvalidData;
    size_t size = Get32BitData(i);

    if (size > ADBusMaximumArrayLength || size > DataRemaining(i))
        return ADBusInvalidData;

    i->signature += 1; // skip over 'a'

    ProcessAlignment(i);

    struct StackEntry* e = PushStackEntry(i);
    e->type = ArrayStackEntry;
    e->data.array.dataEnd = i->data + size;
    e->data.array.typeBegin = i->signature;

    f->type   = ADBusArrayBeginField;
    f->size   = size;
    f->scope  = kv_size(i->stack);

    return 0;
}

// ----------------------------------------------------------------------------

static int NextArrayField(struct ADBusIterator *i, struct ADBusField* f)
{
    struct StackEntry* e = StackTop(i);
    if (i->data > e->data.array.dataEnd)
    {
        return ADBusInvalidData;
    }
    else if (i->data < e->data.array.dataEnd)
    {
        i->signature = e->data.array.typeBegin;
        ProcessAlignment(i);
        return ProcessField(i,f);
    }

    i->signature = ADBusFindArrayEnd_(e->data.array.typeBegin);
    if (i->signature == NULL)
        return 1;

    f->type = ADBusArrayEndField;
    PopStackEntry(i);
    return 0;
}

// ----------------------------------------------------------------------------

static uint IsArrayAtEnd(struct ADBusIterator *i)
{
    struct StackEntry* e = StackTop(i);
    return i->data >= e->data.array.dataEnd;
}

// ----------------------------------------------------------------------------
// Variant functions
// ----------------------------------------------------------------------------

static int ProcessVariant(struct ADBusIterator *i, struct ADBusField* f)
{
    assert(IsAligned(i));
    int err = ProcessSignature(i,f);
    if (err)
        return err;

    // ProcessSignature has alread filled out f->variantType and
    // has consumed the field in i->signature

    struct StackEntry* e = PushStackEntry(i);
    e->type = VariantStackEntry;
    e->data.variant.oldSignature = i->signature;
    e->data.variant.seenFirst = 0;

    f->type = ADBusVariantBeginField;
    f->scope = kv_size(i->stack);

    i->signature = f->string;

    return 0;
}

// ----------------------------------------------------------------------------

static int NextVariantField(struct ADBusIterator *i, struct ADBusField* f)
{
    struct StackEntry* e = StackTop(i);
    if (!e->data.variant.seenFirst)
    {
        e->data.variant.seenFirst = 1;
        ProcessAlignment(i);
        return ProcessField(i,f);
    }
    else if (*i->signature != '\0')
    {
        return ADBusInvalidData; // there's more than one root type in the variant type
    }

    i->signature = e->data.variant.oldSignature;

    PopStackEntry(i);

    f->type = ADBusVariantEndField;

    return 0;
}

// ----------------------------------------------------------------------------

static uint IsVariantAtEnd(struct ADBusIterator *i)
{
    return *i->signature == '\0';
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static int ProcessField(struct ADBusIterator *i, struct ADBusField* f)
{
    f->type = ADBusInvalidField;
    switch(*i->signature)
    {
        case ADBusUInt8Field:
            return Process8Bit(i,f,(uint8_t*)&f->u8);
        case ADBusBooleanField:
            return ProcessBoolean(i,f);
        case ADBusInt16Field:
            return Process16Bit(i,f,(uint16_t*)&f->i16);
        case ADBusUInt16Field:
            return Process16Bit(i,f,(uint16_t*)&f->u16);
        case ADBusInt32Field:
            return Process32Bit(i,f,(uint32_t*)&f->i32);
        case ADBusUInt32Field:
            return Process32Bit(i,f,(uint32_t*)&f->u32);
        case ADBusInt64Field:
            return Process64Bit(i,f,(uint64_t*)&f->i64);
        case ADBusUInt64Field:
            return Process64Bit(i,f,(uint64_t*)&f->u64);
        case ADBusDoubleField:
            return Process64Bit(i,f,(uint64_t*)&f->d);
        case ADBusStringField:
            return ProcessString(i,f);
        case ADBusObjectPathField:
            return ProcessObjectPath(i,f);
        case ADBusSignatureField:
            return ProcessSignature(i,f);
        case ADBusArrayBeginField:
            return ProcessArray(i,f);
        case ADBusStructBeginField:
            return ProcessStruct(i,f);
        case ADBusVariantBeginField:
            return ProcessVariant(i,f);
        case ADBusDictEntryBeginField:
            return ProcessDictEntry(i,f);
        default:
            return ADBusInvalidData;
    }
}

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

struct ADBusIterator* ADBusCreateIterator()
{
    struct ADBusIterator* i = NEW(struct ADBusIterator);
    i->stack = kv_new(Stack);
    return i;
}

// ----------------------------------------------------------------------------

void ADBusFreeIterator(struct ADBusIterator* i)
{
    if (i) {
        kv_free(Stack, i->stack);
        free(i);
    }
}

// ----------------------------------------------------------------------------

void ADBusResetIterator(struct ADBusIterator* i,
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

void ADBusSetIteratorEndianness(
        struct ADBusIterator*       i,
        enum ADBusEndianness        endianness)
{
    i->alternateEndian = ((char) endianness != ADBusNativeEndianness_);
}

// ----------------------------------------------------------------------------

const uint8_t* ADBusCurrentIteratorData(struct ADBusIterator* i, size_t* size)
{
    if (size)
        *size = i->dataEnd - i->data;
    return i->data;
}

// ----------------------------------------------------------------------------

const char* ADBusCurrentIteratorSignature(struct ADBusIterator *i, size_t* size)
{
    if (size)
        *size = strlen(i->signature);
    return i->signature;
}

// ----------------------------------------------------------------------------

uint ADBusIsScopeAtEnd(struct ADBusIterator *i, uint scope)
{
    if (kv_size(i->stack) < scope)
    {
        assert(0);
        return 1;
    }

    if (kv_size(i->stack) > scope)
        return 0;

    if (kv_size(i->stack) == 0)
        return IsRootAtEnd(i);

    switch(StackTop(i)->type)
    {
        case VariantStackEntry:
            return IsVariantAtEnd(i);
        case DictEntryStackEntry:
            return IsDictEntryAtEnd(i);
        case ArrayStackEntry:
            return IsArrayAtEnd(i);
        case StructStackEntry:
            return IsStructAtEnd(i);
        default:
            assert(0);
            return 1;
    }
}

// ----------------------------------------------------------------------------

int ADBusIterate(struct ADBusIterator *i, struct ADBusField* f)
{
    if (kv_size(i->stack) == 0)
    {
        return NextRootField(i,f);
    }
    else
    {
        switch(StackTop(i)->type)
        {
            case VariantStackEntry:
                return NextVariantField(i,f);
            case DictEntryStackEntry:
                return NextDictEntryField(i,f);
            case ArrayStackEntry:
                return NextArrayField(i,f);
            case StructStackEntry:
                return NextStructField(i,f);
            default:
                assert(0);
                return ADBusInternalError;
        }
    }
}

// ----------------------------------------------------------------------------

int ADBusJumpToEndOfArray(struct ADBusIterator* i, uint scope)
{
    if (kv_size(i->stack) < scope)
    {
        assert(0);
        return 1;
    }

    kv_pop(Stack, i->stack, kv_size(i->stack) - scope);

    struct StackEntry* e = StackTop(i);
    if (e->type != ArrayStackEntry)
    {
        assert(0);
        return 1;
    }

    i->data = e->data.array.dataEnd;
    return 0;
}




// ----------------------------------------------------------------------------
// Check functions
// ----------------------------------------------------------------------------

static void CheckField(
        struct ADBusCallDetails*    details,
        struct ADBusField*          field,
        enum ADBusFieldType         type)
{
    int err = ADBusIterate(details->arguments, field);
    if (err) {
        details->parseError = err;
        longjmp(details->jmpbuf, 0);
    }

    if (field->type != type) {
        ADBusSetupError(
                details,
                "nz.co.foobar.ADBus.Error.InvalidArgument", -1,
                "Invalid arguments passed to a method call.", -1);
        longjmp(details->jmpbuf, 0);
    }
}

void ADBusCheckMessageEnd(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusEndField);
}

uint ADBusCheckBoolean(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusBooleanField);
    return f.b;
}

uint8_t ADBusCheckUInt8(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusUInt8Field);
    return f.u8;
}

int16_t ADBusCheckInt16(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusInt16Field);
    return f.i16;
}

uint16_t ADBusCheckUInt16(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusUInt16Field);
    return f.u16;
}

int32_t ADBusCheckInt32(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusInt32Field);
    return f.i32;
}

uint32_t ADBusCheckUInt32(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusUInt32Field);
    return f.u32;
}

int64_t ADBusCheckInt64(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusInt64Field);
    return f.i64;
}

uint64_t ADBusCheckUInt64(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusUInt64Field);
    return f.u64;
}

double ADBusCheckDouble(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusDoubleField);
    return f.d;
}

const char* ADBusCheckString(struct ADBusCallDetails* d, size_t* size)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusStringField);
    if (size)
        *size = f.size;
    return f.string;
}

const char* ADBusCheckObjectPath(struct ADBusCallDetails* d, size_t* size)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusObjectPathField);
    if (size)
        *size = f.size;
    return f.string;
}

const char* ADBusCheckSignature(struct ADBusCallDetails* d, size_t* size)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusSignatureField);
    if (size)
        *size = f.size;
    return f.string;
}

uint ADBusCheckArrayBegin(struct ADBusCallDetails* d, const uint8_t** data, size_t* size)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusArrayBeginField);
    if (data)
        *data = f.data;
    if (size)
        *size = f.size;
    return f.scope;
}

void ADBusCheckArrayEnd(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusArrayEndField);
}

uint ADBusCheckStructBegin(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusStructBeginField);
    return f.scope;
}

void ADBusCheckStructEnd(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusStructEndField);
}

uint ADBusCheckDictEntryBegin(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusDictEntryBeginField);
    return f.scope;
}

void ADBusCheckDictEntryEnd(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusDictEntryEndField);
}

const char* ADBusCheckVariantBegin(struct ADBusCallDetails* d, size_t* size)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusVariantBeginField);
    if (size)
        *size = f.size;
    return f.string;
}

void ADBusCheckVariantEnd(struct ADBusCallDetails* d)
{
    struct ADBusField f;
    CheckField(d, &f, ADBusVariantEndField);
}




