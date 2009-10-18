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

#include "Tests.h"

#include "adbus/vector.h"
#include "adbus/str.h"

#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <wchar.h>

VECTOR_INSTANTIATE(char, c)
VECTOR_INSTANTIATE(wchar_t, wc)
static void DoTestWCharVector(wcvector_t* vec, const wchar_t* str)
{
    assert(wcslen(str) == wcvector_size(vec));
    assert(wcsncmp(str, *vec, wcvector_size(vec)) == 0);
}

static void DoTestCharVector(cvector_t* vec, const char* str)
{
    assert(strlen(str) == cvector_size(vec));
    assert(strncmp(str, *vec, cvector_size(vec)) == 0);
}

#define TEST(STR) DoTestCharVector(&vec, STR)

static void TestCharVector()
{
    char* dest;
    cvector_t vec = NULL;

    dest = cvector_insert_end(&vec, 3);
    memcpy(dest, "abc", 3);
    TEST("abc");

    dest = cvector_insert_end(&vec, 2);
    memcpy(dest, "de", 2);
    TEST("abcde");

    dest = cvector_insert(&vec, 1, 3);
    memcpy(dest, "fgh", 3);
    TEST("afghbcde");

    cvector_remove_end(&vec, 4);
    TEST("afgh");

    cvector_remove(&vec, 1, 2);
    TEST("ah");

    cvector_free(&vec);
    assert(vec == NULL);
}

#undef TEST
#define TEST(STR) DoTestWCharVector(&vec, STR)

static void TestWCharVector()
{
    // Use wchar_t so that the member size is > 1
    wchar_t* dest;
    wcvector_t vec = NULL;

    dest = wcvector_insert_end(&vec, 3);
    memcpy(dest, L"abc", 3 * sizeof(wchar_t));
    TEST(L"abc");

    dest = wcvector_insert_end(&vec, 2);
    memcpy(dest, L"de", 2 * sizeof(wchar_t));
    TEST(L"abcde");

    dest = wcvector_insert(&vec, 1, 3);
    memcpy(dest, L"fgh", 3 * sizeof(wchar_t));
    TEST(L"afghbcde");

    wcvector_remove_end(&vec, 4);
    TEST(L"afgh");

    wcvector_remove(&vec, 1, 2);
    TEST(L"ah");

    wcvector_free(&vec);
    assert(vec == NULL);
}

#undef TEST

static void DoTestString(str_t* string, const char* str)
{
    size_t sz = strlen(str);
    size_t stringz = str_size(string);
    assert(sz == stringz);
    assert(strcmp(str, *string) == 0);
}

#define TEST(STR) DoTestString(&str, STR)

static void TestString()
{
    str_t str = NULL;

    str_append(&str, "abc");
    TEST("abc");

    str_append(&str, "de");
    TEST("abcde");

    str_append(&str, "fghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
    TEST("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");

    str_remove(&str, 3, 2);
    TEST("abcfghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");

    str_remove_end(&str, 26);
    TEST("abcfghijklmnopqrstuvwxyz");

    str_insert_n(&str, 3, "defg", 2);
    TEST("abcdefghijklmnopqrstuvwxyz");

    str_insert(&str, 3, "de");
    TEST("abcdedefghijklmnopqrstuvwxyz");

    str_free(&str);
    assert(str == NULL);
}

#undef TEST

void TestVector()
{
    TestCharVector();
    TestWCharVector();
    TestString();
}
