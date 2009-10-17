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


#pragma once

#include "Marshaller.h"

#include "vector.h"
#include "str.h"

VECTOR_INSTANTIATE(uint8_t, u8)

// ----------------------------------------------------------------------------

enum StackEntryType
{
    ArrayStackEntry,
    StructStackEntry,
    VariantStackEntry,
    DictEntryStackEntry,
};

struct ArrayStackData
{
    size_t      sizeIndex;
    size_t      dataBegin;
    const char* sigBegin;
};

struct VariantStackData
{
    char* signature;
    const char* oldSigp;
};

struct StackEntry
{
    enum StackEntryType type;

    union
    {
        struct ArrayStackData array;
        struct VariantStackData variant;
    }d;
};

VECTOR_INSTANTIATE(struct StackEntry, stack_)

// ----------------------------------------------------------------------------

struct ADBusMarshaller
{
    u8vector_t                  data;
    char                        signature[257];
    const char*                 sigp;
    stack_vector_t              stack;
};

