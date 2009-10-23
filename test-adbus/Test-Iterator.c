// vim: ts=4 sw=4 sts=4 et
//
// Copyright (c) 2009 James R. McKaskill
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// ----------------------------------------------------------------------------

#ifndef NDEBUG

#include "Tests.h"

#include "adbus/Iterator.h"
#include "adbus/vector.h"

#include <assert.h>
#include <string.h>

VECTOR_INSTANTIATE(uint8_t, u8)

static struct ADBusField        sField;
static struct ADBusIterator*    sIterator;
static u8vector_t               sData;
static enum ADBusEndianness     sEndianness;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void TestField(enum ADBusFieldType type)
{
    memset(&sField, 0xDE, sizeof(struct ADBusField));
    int err = ADBusIterate(sIterator, &sField);
    assert(!err);
    assert(sField.type == type);
}

static void TestInvalidData()
{
    memset(&sField, 0xDE, sizeof(struct ADBusField));
    int err = ADBusIterate(sIterator, &sField);
    assert(err);
}

static void ResetIterator(const char* sig, const uint8_t* data, size_t size)
{
    u8vector_clear(&sData);
    uint8_t* dest = u8vector_insert_end(&sData, size);
    memcpy(dest, data, size);
    ADBusResetIterator(sIterator, sig, -1, sData, size);
    ADBusSetIteratorEndianness(sIterator, sEndianness);
}

static void Setup(enum ADBusEndianness endianness)
{
    sIterator = ADBusCreateIterator();
    sEndianness = endianness;
}

static void Cleanup()
{
    ADBusFreeIterator(sIterator);
    sIterator = NULL;
    u8vector_free(&sData);
}

// ----------------------------------------------------------------------------


static void TestEnd()
{ TestField(ADBusEndField); }

static void TestBoolean(uint32_t b)
{
    TestField(ADBusBooleanField);
    assert(sField.b == b);
}

static void TestUInt8(uint8_t v)
{
    TestField(ADBusUInt8Field);
    assert(sField.u8 == v);
}

static void TestUInt16(uint16_t v)
{
    TestField(ADBusUInt16Field);
    assert(sField.u16 == v);
}

static void TestInt16(int16_t v)
{
    TestField(ADBusInt16Field);
    assert(sField.i16 == v);
}

static void TestUInt32(uint32_t v)
{
    TestField(ADBusUInt32Field);
    assert(sField.u32 == v);
}

static void TestInt32(int32_t v)
{
    TestField(ADBusInt32Field);
    assert(sField.i32 == v);
}

static void TestUInt64(uint64_t v)
{
    TestField(ADBusUInt64Field);
    assert(sField.u64 == v);
}

static void TestInt64(int64_t v)
{
    TestField(ADBusInt64Field);
    assert(sField.i64 == v);
}

static void TestDouble(double v)
{
    TestField(ADBusDoubleField);
    assert(sField.d == v);
}

static void TestString(const char* str)
{
    TestField(ADBusStringField);
    assert(strcmp(sField.string, str) == 0);
    assert(strlen(str) == sField.size);
}

static void TestObjectPath(const char* str)
{
    TestField(ADBusObjectPathField);
    assert(strcmp(sField.string, str) == 0);
    assert(strlen(str) == sField.size);
}

static void TestSignature(const char* str)
{
    TestField(ADBusSignatureField);
    assert(strcmp(sField.string, str) == 0);
    assert(strlen(str) == sField.size);
}

static void TestArrayBegin()
{ TestField(ADBusArrayBeginField); }
static void TestArrayEnd()
{ TestField(ADBusArrayEndField); }

static void TestStructBegin()
{ TestField(ADBusStructBeginField); }
static void TestStructEnd()
{ TestField(ADBusStructEndField); }

static void TestDictEntryBegin()
{ TestField(ADBusDictEntryBeginField); }
static void TestDictEntryEnd()
{ TestField(ADBusDictEntryEndField); }

static void TestVariantBegin(const char* type)
{
    TestField(ADBusVariantBeginField);
    assert(strcmp(sField.string, type) == 0);
    assert(sField.size == strlen(type));
}
static void TestVariantEnd()
{ TestField(ADBusVariantEndField); }

// ----------------------------------------------------------------------------

#define CAT2(x,y) x ## y
#define CAT(x,y) CAT2(x, y)
#define DATA_NAME CAT(data, __LINE__)
#define RESET(SIG, ...) static uint8_t DATA_NAME [] = { __VA_ARGS__ }; ResetIterator(SIG, DATA_NAME, sizeof(DATA_NAME))

#define FILL 0xCC

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void TestIteratorLittleEndian()
{
    Setup(ADBusLittleEndian);

    // Fixed fields
    RESET("y", 0x08);
    TestUInt8(0x08);
    TestEnd();
    TestEnd();

    RESET("yy", 0x88, 0x23);
    TestUInt8(0x88);
    TestUInt8(0x23);
    TestEnd();

    RESET("q", 0x34, 0x56);
    TestUInt16(0x5634);
    TestEnd();

    RESET("yq", 0x12, FILL, 0x34, 0x56);
    TestUInt8(0x12);
    TestUInt16(0x5634);
    TestEnd();

    RESET("n", 0x34, 0x56);
    TestInt16(0x5634);
    TestEnd();

    RESET("yn", 0x12, FILL, 0x34, 0xA6);
    TestUInt8(0x12);
    TestInt16(0xA634 - 0x10000);
    TestEnd();

    RESET("u", 0x12, 0x34, 0x56, 0x78);
    TestUInt32(0x78563412);
    TestEnd();

    RESET("yu", 0x11, FILL, FILL, FILL, 0x12, 0x34, 0x56, 0x78);
    TestUInt8(0x11);
    TestUInt32(0x78563412);
    TestEnd();

    RESET("i", 0x12, 0x34, 0x56, 0x78);
    TestInt32(0x78563412);
    TestEnd();

    RESET("yi", 0x11, FILL, FILL, FILL, 0x12, 0x34, 0x56, 0xC8);
    TestUInt8(0x11);
    TestInt32(-(int32_t)(0xFFFFFFFF - 0xC8563412 + 1));
    TestEnd();

    RESET("t", 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88);
    TestUInt64(UINT64_C(0x8877665544332211));
    TestEnd();

    RESET("yt", 0x99, FILL, FILL, FILL, FILL, FILL, FILL, FILL,
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88);
    TestUInt8(0x99);
    TestUInt64(UINT64_C(0x8877665544332211));
    TestEnd();

    RESET("x", 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88);
    TestInt64(INT64_C(0x8877665544332211));
    TestEnd();

    RESET("yx", 0x99, FILL, FILL, FILL, FILL, FILL, FILL, FILL,
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xE8);
    TestUInt8(0x99);
    TestInt64(-(int64_t)(UINT64_C(0xFFFFFFFFFFFFFFFF) - UINT64_C(0xE877665544332211) + 1));
    TestEnd();

    RESET("d", 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88);
    uint64_t u64Value = UINT64_C(0x8877665544332211);
    TestDouble(*(double*)&u64Value);
    TestEnd();

    RESET("yd", 0x99, FILL, FILL, FILL, FILL, FILL, FILL, FILL,
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xE8);
    TestUInt8(0x99);
    uint64_t u64Value2 = UINT64_C(0xE877665544332211);
    TestDouble(*(double*)&u64Value2);
    TestEnd();

    // Boolean
    RESET("b", 0x01, 0x00, 0x00, 0x00);
    TestBoolean(1);
    TestEnd();

    RESET("b", 0x00, 0x00, 0x00, 0x00);
    TestBoolean(0);
    TestEnd();

    RESET("b", 0x02, 0x00, 0x00, 0x00);
    TestInvalidData();

    RESET("b", 0x01, 0x00, 0x00);
    TestInvalidData();

    RESET("b", 0x01, 0x00, 0x00, 0x00, FILL);
    TestBoolean(1);
    TestInvalidData();

    RESET("yb", 0x11, FILL, FILL, FILL, 0x01, 0x00, 0x00, 0x00);
    TestUInt8(0x11);
    TestBoolean(1);
    TestEnd();

    // Strings
    RESET("s", 12, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '.', 0);
    TestString("Hello world.");
    TestEnd();

    // Embedded zero
    RESET("s", 12, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', 0, 'w', 'o', 'r', 'l', 'd', '.', 0);
    TestInvalidData();

    // Missing null terminator
    RESET("s", 12, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '.');
    TestInvalidData();

    // Length off by one
    RESET("s", 13, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '.', 0);
    TestInvalidData();

    // Invalid utf8 - TODO more cases
    RESET("s", 12, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', 0xDE, 'w', 'o', 'r', 'l', 'd', '.', 0);
    TestInvalidData();

    // Struct
    RESET("(yu)", 0x11, FILL, FILL, FILL, 0x11, 0x22, 0x33, 0x44);
    TestStructBegin();
    TestUInt8(0x11);
    TestUInt32(0x44332211);
    TestStructEnd();
    TestEnd();

    // Variant
    RESET("v", 0x04,  '(',  'y',  'u',  ')', 0x00, FILL, FILL,
               0x11, FILL, FILL, FILL, 0x11, 0x22, 0x33, 0x44);
    TestVariantBegin("(yu)");
    TestStructBegin();
    TestUInt8(0x11);
    TestUInt32(0x44332211);
    TestStructEnd();
    TestVariantEnd();
    TestEnd();

    // Array
    RESET("a(yu)", 0x00, 0x00, 0x00, 0x00, FILL, FILL, FILL, FILL);
    TestArrayBegin();
    TestArrayEnd();
    TestEnd();

    RESET("a(yu)", 0x10, 0x00, 0x00, 0x00, FILL, FILL, FILL, FILL,
                   0x11, FILL, FILL, FILL, 0x11, 0x22, 0x33, 0x44,
                   0x99, FILL, FILL, FILL, 0x99, 0xAA, 0xBB, 0xCC);
    TestArrayBegin();
    TestStructBegin();
    TestUInt8(0x11);
    TestUInt32(0x44332211);
    TestStructEnd();
    TestStructBegin();
    TestUInt8(0x99);
    TestUInt32(0xCCBBAA99);
    TestStructEnd();
    TestArrayEnd();
    TestEnd();

    RESET("a(yq)", 0x0C, 0x00, 0x00, 0x00, FILL, FILL, FILL, FILL,
                   0x11, FILL, 0x11, 0x22, FILL, FILL, FILL, FILL,
                   0x99, FILL, 0x99, 0xAA);
    TestArrayBegin();
    TestStructBegin();
    TestUInt8(0x11);
    TestUInt16(0x2211);
    TestStructEnd();
    TestStructBegin();
    TestUInt8(0x99);
    TestUInt16(0xAA99);
    TestStructEnd();
    TestArrayEnd();
    TestEnd();

    Cleanup();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void TestIterator()
{
    TestIteratorLittleEndian();
}

#endif