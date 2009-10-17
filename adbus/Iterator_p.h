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

#include "Common.h"
#include "vector.h"

enum StackEntryType
{
  InvalidStackEntry,
  VariantStackEntry,
  DictEntryStackEntry,
  ArrayStackEntry,
  StructStackEntry,
};

struct ArrayStackData
{
  const char* typeBegin;
  const uint8_t* dataEnd;
};

struct VariantStackData
{
  const char* oldSignature;
  unsigned int seenFirst;
};

struct StackEntry
{
  enum StackEntryType type;
  union
  {
    struct ArrayStackData   array;
    struct VariantStackData variant;
    size_t                  dictEntryFields;
  } data;
};

VECTOR_INSTANTIATE(struct StackEntry, stack_)

struct ADBusIterator
{
  const uint8_t*            data;
  const uint8_t*            dataEnd;
  const char*               signature;

  uint                      alternateEndian;

  stack_vector_t            stack;
};

