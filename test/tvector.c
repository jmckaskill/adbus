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

#include "memory/kvector.h"
#include "memory/kstring.h"

#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <wchar.h>

KVECTOR_INIT(char, char)
KVECTOR_INIT(wchar_t, wchar_t)
static void DoTestWCharVector(kvector_t(wchar_t)* vec, const wchar_t* str)
{
    assert(wcslen(str) == kv_size(vec));
    assert(wcsncmp(str, &kv_a(vec, 0), kv_size(vec)) == 0);
}

static void DoTestCharVector(kvector_t(char)* vec, const char* str)
{
    assert(strlen(str) == kv_size(vec));
    assert(strncmp(str, &kv_a(vec, 0), kv_size(vec)) == 0);
}

#define TEST(STR) DoTestCharVector(vec, STR)

static void TestCharVector()
{
    char* dest;
    kvector_t(char)* vec = kv_new(char);

    dest = kv_push(char, vec, 3);
    memcpy(dest, "abc", 3);
    TEST("abc");

    dest = kv_push(char, vec, 2);
    memcpy(dest, "de", 2);
    TEST("abcde");

    dest = kv_insert(char, vec, 1, 3);
    memcpy(dest, "fgh", 3);
    TEST("afghbcde");

    kv_pop(char, vec, 4);
    TEST("afgh");

    kv_remove(char, vec, 1, 2);
    TEST("ah");

    kv_free(char, vec);
}

#undef TEST
#define TEST(STR) DoTestWCharVector(vec, STR)

static void TestWCharVector()
{
    // Use wchar_t so that the member size is > 1
    wchar_t* dest;
    kvector_t(wchar_t)* vec = kv_new(wchar_t);

    dest = kv_push(wchar_t, vec, 3);
    memcpy(dest, L"abc", 3 * sizeof(wchar_t));
    TEST(L"abc");

    dest = kv_push(wchar_t, vec, 2);
    memcpy(dest, L"de", 2 * sizeof(wchar_t));
    TEST(L"abcde");

    dest = kv_insert(wchar_t, vec, 1, 3);
    memcpy(dest, L"fgh", 3 * sizeof(wchar_t));
    TEST(L"afghbcde");

    kv_pop(wchar_t, vec, 4);
    TEST(L"afgh");

    kv_remove(wchar_t, vec, 1, 2);
    TEST(L"ah");

    kv_free(wchar_t, vec);
}

#undef TEST

static void DoTestString(kstring_t* string, const char* str)
{
    size_t sz = strlen(str);
    size_t stringz = ks_size(string);
    assert(sz == stringz);
    assert(ks_cmp(string, str) == 0);
}

#define TEST(STR) DoTestString(str, STR)

static void TestString()
{
    kstring_t* str = ks_new();

    ks_cat(str, "abc");
    TEST("abc");

    ks_cat(str, "de");
    TEST("abcde");

    ks_cat(str, "fghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
    TEST("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");

    ks_remove(str, 3, 2);
    TEST("abcfghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");

    ks_remove_end(str, 26);
    TEST("abcfghijklmnopqrstuvwxyz");

    ks_insert_n(str, 3, "defg", 2);
    TEST("abcdefghijklmnopqrstuvwxyz");

    ks_insert(str, 3, "de");
    TEST("abcdedefghijklmnopqrstuvwxyz");

    ks_free(str);
}

#undef TEST

void TestVector()
{
    TestCharVector();
    TestWCharVector();
    TestString();
}

#endif
