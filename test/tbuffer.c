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

#include <assert.h>
#include <string.h>
#include <stdio.h>

static const char* HexString(const uint8_t* data, size_t sz)
{
    static char buf[4096];
    assert(sz < (sizeof(buf) - 1) / 3);
    for (size_t i = 0; i < sz; ++i) {
        snprintf(&buf[i*3], 4, "%02X ", data[i]);
    }
    buf[sz * 3 - 1] = '\0';
    return buf;
}


#define CAT2(x,y) x ## y
#define CAT(x,y) CAT2(x, y)
#define NAME(PRE) CAT(PRE ## _ , __LINE__)
#define T(SIG, ...)                                                          \
    static uint8_t NAME(cdata) [] = { __VA_ARGS__ };                            \
    const char* NAME(sig);                                                      \
    const uint8_t* NAME(data);                                                  \
    size_t NAME(sigsize), NAME(datasize);                                       \
    adbus_buf_get(b, &NAME(sig), &NAME(sigsize), &NAME(data), &NAME(datasize)); \
    printf("Test %d: Sig %d \"%s\", Data %d {%s}\n", __LINE__,                  \
            (int) NAME(sigsize), NAME(sig),                                     \
            (int) NAME(datasize), HexString(NAME(data), NAME(datasize)));       \
    assert(NAME(datasize) == sizeof(NAME(cdata)));                              \
    assert(NAME(sigsize) == (sizeof(SIG) - 1));                                 \
    NE(memcmp(NAME(data), NAME(cdata), NAME(datasize)));                     \
    NE(strncmp(NAME(sig), SIG, NAME(sigsize)));                              \
    adbus_buf_reset(b);

//fill
#define F 0x00 

#define NE(x) assert(x == 0)
#define E(x) assert(x != 0)

#define UINT8 0xDE
#define INT16 0x6EAD
#define INT16_LE 0xAD, 0x6E
#define UINT16 0xDEAD
#define UINT16_LE 0xAD, 0xDE
#define INT32 0x478945F2
#define INT32_LE 0xF2, 0x45, 0x89, 0x47
#define UINT32 0xD78A45C2
#define UINT32_LE 0xC2, 0x45, 0x8A, 0xD7
#define INT64 INT64_C(0x478A45C202050678)
#define INT64_LE 0x78, 0x06, 0x05, 0x02, 0xC2, 0x45, 0x8A, 0x47
#define UINT64 UINT64_C(0xD78A45C202050678)
#define UINT64_LE 0x78, 0x06, 0x05, 0x02, 0xC2, 0x45, 0x8A, 0xD7
#define DOUBLE 1.333e67
#define DOUBLE_LE 0x22, 0x88, 0x62, 0xD7, 0xDB, 0xA4, 0xDF, 0x4D
#define STR "Hello world."
#define STR_LE 'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '.', '\0'
#define STR2 "H2llo 6ld."
#define STR2_LE 'H', '2', 'l', 'l', 'o', ' ', '6', 'l', 'd', '.', '\0'
#define STR_NUL "Hello\0world."
#define STR_NUL_LE 'H', 'e', 'l', 'l', 'o', '\0', 'w', 'o', 'r', 'l', 'd', '.', '\0'
#define STR_UTF8 "Hello\xDEworld."
#define STR_UTF8_LE 'H', 'e', 'l', 'l', 'o', '\xDE', 'w', 'o', 'r', 'l', 'd', '.', '\0'
#define TRUE 1
#define FALSE 0
#define TRUE_LE 0x01, 0x00, 0x00, 0x00
#define FALSE_LE 0x00, 0x00, 0x00, 0x00

void TestBuffer()
{
    adbus_Buffer* b = adbus_buf_new();

    NE(adbus_buf_append(b, "b", -1));
    NE(adbus_buf_bool(b, TRUE));
    T("b", TRUE_LE);

    NE(adbus_buf_append(b, "b", -1));
    NE(adbus_buf_bool(b, FALSE));
    T("b", FALSE_LE);

    NE(adbus_buf_append(b, "y", -1));
    NE(adbus_buf_uint8(b, UINT8));
    T("y", UINT8);

    NE(adbus_buf_append(b, "n", -1));
    NE(adbus_buf_int16(b, INT16));
    T("n", INT16_LE);

    NE(adbus_buf_append(b, "q", -1));
    NE(adbus_buf_uint16(b, UINT16));
    T("q", UINT16_LE);

    NE(adbus_buf_append(b, "i", -1));
    NE(adbus_buf_int32(b, INT32));
    T("i", INT32_LE); 

    NE(adbus_buf_append(b, "u", -1));
    NE(adbus_buf_uint32(b, UINT32));
    T("u", UINT32_LE);

    NE(adbus_buf_append(b, "x", -1));
    NE(adbus_buf_int64(b, INT64));
    T("x", INT64_LE); 

    NE(adbus_buf_append(b, "t", -1));
    NE(adbus_buf_uint64(b, UINT64));
    T("t", UINT64_LE);

    NE(adbus_buf_append(b, "d", -1));
    NE(adbus_buf_double(b, DOUBLE));
    T("d", DOUBLE_LE);

    NE(adbus_buf_append(b, "s", -1));
    NE(adbus_buf_string(b, STR, -1));
    T("s", 12, 0, 0, 0, STR_LE);

#if 0 // TODO
    NE(adbus_buf_append(b, "s", -1));
    E(adbus_buf_string(b, STR_NUL, sizeof(STR_NUL) - 1));

    NE(adbus_buf_append(b, "s", -1));
    E(adbus_buf_string(b, STR_UTF8, -1));
#endif

    NE(adbus_buf_append(b, "o", -1));
    NE(adbus_buf_objectpath(b, STR, -1));
    T("o", 12, 0, 0, 0, STR_LE);

    NE(adbus_buf_append(b, "g", -1));
    NE(adbus_buf_signature(b, STR, -1));
    T("g", 12, STR_LE);

    NE(adbus_buf_append(b, "as", -1));
    NE(adbus_buf_beginarray(b));
    NE(adbus_buf_string(b, STR, -1));
    NE(adbus_buf_string(b, STR2, -1));
    NE(adbus_buf_endarray(b));
    T("as", 35, 0, 0, 0, 12, 0, 0, 0, STR_LE, F, F, F, 10, 0, 0, 0, STR2_LE);

    NE(adbus_buf_append(b, "(uy)", -1));
    NE(adbus_buf_beginstruct(b));
    NE(adbus_buf_uint32(b, UINT32));
    NE(adbus_buf_uint8(b, UINT8));
    NE(adbus_buf_endstruct(b));
    T("(uy)", UINT32_LE, UINT8);

    NE(adbus_buf_append(b, "a(uy)", -1));
    NE(adbus_buf_beginarray(b));
    NE(adbus_buf_beginstruct(b));
    NE(adbus_buf_uint32(b, UINT32));
    NE(adbus_buf_uint8(b, UINT8));
    NE(adbus_buf_endstruct(b));
    NE(adbus_buf_beginstruct(b));
    NE(adbus_buf_uint32(b, UINT32));
    NE(adbus_buf_uint8(b, UINT8));
    NE(adbus_buf_endstruct(b));
    NE(adbus_buf_endarray(b));
    T("a(uy)", 13, 0, 0, 0, F, F, F, F, UINT32_LE, UINT8, F, F, F, UINT32_LE, UINT8);

    NE(adbus_buf_append(b, "v", -1));
    NE(adbus_buf_beginvariant(b, "u", -1));
    NE(adbus_buf_uint32(b, UINT32));
    NE(adbus_buf_endvariant(b));
    T("v", 1, 'u', '\0', F, UINT32_LE);

    NE(adbus_buf_append(b, "a{ss}", -1));
    NE(adbus_buf_beginarray(b));
    NE(adbus_buf_endarray(b));
    T("a{ss}", 0, 0, 0, 0, F, F, F, F);

    NE(adbus_buf_append(b, "a{ss}", -1));
    NE(adbus_buf_beginarray(b));
    NE(adbus_buf_begindictentry(b));
    NE(adbus_buf_string(b, STR, -1));
    NE(adbus_buf_string(b, STR2, -1));
    NE(adbus_buf_enddictentry(b));
    NE(adbus_buf_endarray(b));
    T("a{ss}", 35, 0, 0, 0, F, F, F, F, 12, 0, 0, 0, STR_LE, F, F, F, 10, 0, 0, 0, STR2_LE);

    adbus_buf_free(b);
}

