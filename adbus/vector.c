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

#include "vector.h"

#include <assert.h>
#include <string.h>
#include <stdint.h>

// Size needs to be a multiple of 8 so that when we put it at the head of the
// vector the beginning of the vector is still 8 byte aligned
struct vector_header
{
    size_t size;
    size_t alloc;
};

// ----------------------------------------------------------------------------

static struct vector_header* get_header(uint8_t** pvec)
{
    struct vector_header** pheader = (struct vector_header**) pvec;
    return *pheader ? *pheader - 1 : NULL;
}

static const struct vector_header* get_cheader(const uint8_t** pvec)
{
    return (const struct vector_header*) get_header((uint8_t**) pvec);
}

// ----------------------------------------------------------------------------

#ifndef vector_alloc_grow
#define vector_alloc_grow(size) (((size)+16)*3/2)
#endif

static void grow(uint8_t** pvec, size_t member_size, size_t size)
{
    size_t alloc = 0;
    struct vector_header* header = get_header(pvec);
    if (header) {
        header->size = size;
        alloc = header->alloc;
    }

    if (size <= alloc)
        return;

    if (vector_alloc_grow(alloc) < size)
        alloc = size;
    else
        alloc = vector_alloc_grow(alloc);

    size_t alloc_size = alloc * member_size
                      + sizeof(struct vector_header);
    header = (struct vector_header*) realloc(header, alloc_size);

    header->size  = size;
    header->alloc = alloc;
    *pvec = (uint8_t*) (header + 1);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#ifdef NDEBUG
#   define vector_assert(x) do {}while(0)
#else
void vector_assert(const uint8_t** pvec)
{
    const struct vector_header* header = get_cheader(pvec);
    assert(!header || header->size <= header->alloc);
}
#endif

// ----------------------------------------------------------------------------

size_t vector_size(const uint8_t** pvec)
{
    const struct vector_header* header = get_cheader(pvec);
    return header ? header->size : 0;
}

// ----------------------------------------------------------------------------

void vector_require(uint8_t** pvec, size_t member_size, size_t min_size)
{
    size_t size = vector_size((const uint8_t**) pvec);
    if (size <= min_size)
        return;

    vector_insert_end(pvec, member_size, min_size - size);
}

// ----------------------------------------------------------------------------

uint8_t* vector_insert_end(uint8_t** pvec, size_t member_size, size_t number)
{
    assert(number > 0);

    size_t size = vector_size((const uint8_t**) pvec);
    grow(pvec, member_size, size + number);

    uint8_t* begin = *pvec + (size * member_size);
    memset(begin, 0, member_size * number);

    return begin;
}

// ----------------------------------------------------------------------------

uint8_t* vector_insert(uint8_t** pvec, size_t member_size, size_t index, size_t number)
{
    assert(number > 0);
    assert(vector_size((const uint8_t**) pvec) >= index);

    size_t old_size = vector_size((const uint8_t**) pvec);
    grow(pvec, member_size, old_size + number);

    uint8_t* begin = *pvec + (index * member_size);
    uint8_t* end   = begin + (number * member_size);
    size_t size = vector_size((const uint8_t**) pvec) * member_size;

    //      b              b e
    // [....|....] to [....| |....] (size is of latter)
    memmove(end, begin, (*pvec + size) - end);
    memset(begin, 0, end - begin);
    return begin;
}

// ----------------------------------------------------------------------------

void vector_remove_end(uint8_t** pvec, size_t number)
{
    struct vector_header* header = get_header(pvec);
    assert(header);
    assert(0 < number && number <= header->size);

    header->size -= number;
}

// ----------------------------------------------------------------------------

void vector_remove(uint8_t** pvec, size_t member_size, size_t index, size_t number)
{
    struct vector_header* header = get_header(pvec);
    assert(header);
    assert(0 < number);
    assert(index + number <= header->size);

    uint8_t* rm_begin  = *pvec + (index * member_size);
    uint8_t* rm_end    = rm_begin + (number * member_size);
    uint8_t* vector_end   = *pvec + (header->size * member_size);

    assert( rm_begin <  rm_end
         && rm_end   <= vector_end);

    //     rb re  ve        rb
    // [....| |....] to [....|....]
    if ((vector_end - rm_end) > 0)
        memmove(rm_begin, rm_end, vector_end - rm_end);

    header->size -= number;
}

// ----------------------------------------------------------------------------

void vector_clear(uint8_t** pvec)
{
    struct vector_header* header = get_header(pvec);
    if (header)
        header->size = 0;
}

// ----------------------------------------------------------------------------

void vector_free(uint8_t** pvec)
{
    free(get_header(pvec));
    *pvec = NULL;
}







