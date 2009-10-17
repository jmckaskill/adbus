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



ADBUSI_FUNC size_t  vector_size(const uint8_t** pvec);
ADBUSI_FUNC void    vector_require(uint8_t** pvec, size_t member_size, size_t min_size);
ADBUSI_FUNC uint8_t*   vector_insert_end(uint8_t** pvec, size_t member_size, size_t number);
ADBUSI_FUNC uint8_t*   vector_insert(uint8_t** pvec, size_t member_size, size_t index, size_t number);
ADBUSI_FUNC void    vector_remove_end(uint8_t** pvec, size_t number);
ADBUSI_FUNC void    vector_remove(uint8_t** pvec, size_t member_size, size_t index, size_t number);
ADBUSI_FUNC void    vector_clear(uint8_t** pvec);
ADBUSI_FUNC void    vector_free(uint8_t** pvec);


#define VECTOR_CAT2(x,y) x ## y
#define VECTOR_CAT(x,y) VECTOR_CAT2(x,y)


#define VECTOR_TYPE(P)          VECTOR_CAT(P, vector_t)

#define VECTOR_SIZE(P)          VECTOR_CAT(P, vector_size)

#define VECTOR_CLEAR(P)         VECTOR_CAT(P, vector_clear)
#define VECTOR_FREE(P)          VECTOR_CAT(P, vector_free)

#define VECTOR_REQUIRE(P)       VECTOR_CAT(P, vector_require)
#define VECTOR_INSERT(P)        VECTOR_CAT(P, vector_insert)
#define VECTOR_INSERT_END(P)    VECTOR_CAT(P, vector_insert_end)

#define VECTOR_REMOVE(P)        VECTOR_CAT(P, vector_remove)
#define VECTOR_REMOVE_END(P)    VECTOR_CAT(P, vector_remove_end)



#define VECTOR_INSTANTIATE(T, P)                                                \
                                                                                \
    typedef T* VECTOR_TYPE(P);                                                  \
                                                                                \
                                                                                \
    static inline size_t VECTOR_SIZE(P) (const VECTOR_TYPE(P)* pvec)            \
    { return vector_size((const uint8_t**) pvec); }                             \
                                                                                \
                                                                                \
    static inline void VECTOR_CLEAR(P) (VECTOR_TYPE(P)* pvec)                   \
    { vector_clear((uint8_t**) pvec); }                                         \
                                                                                \
    static inline void VECTOR_FREE(P) (VECTOR_TYPE(P)* pvec)                    \
    { vector_free((uint8_t**) pvec); }                                          \
                                                                                \
                                                                                \
    static inline void VECTOR_REQUIRE(P) (                                      \
            VECTOR_TYPE(P)* pvec,                                               \
            size_t          min_size)                                           \
    { return vector_require((uint8_t**) pvec, sizeof(T), min_size); }           \
                                                                                \
    static inline T* VECTOR_INSERT(P) (                                         \
            VECTOR_TYPE(P)*    pvec,                                            \
            size_t          index,                                              \
            size_t          number)                                             \
    { return (T*) vector_insert((uint8_t**) pvec, sizeof(T), index, number); }  \
                                                                                \
    static inline T* VECTOR_INSERT_END(P) (                                     \
            VECTOR_TYPE(P)*    pvec,                                            \
            size_t          number)                                             \
    { return (T*) vector_insert_end((uint8_t**) pvec, sizeof(T), number); }     \
                                                                                \
                                                                                \
    static inline void VECTOR_REMOVE(P) (                                       \
            VECTOR_TYPE(P)*    pvec,                                            \
            size_t          index,                                              \
            size_t          number)                                             \
    { vector_remove((uint8_t**) pvec, sizeof(T), index, number); }              \
                                                                                \
    static inline void VECTOR_REMOVE_END(P) (                                   \
            VECTOR_TYPE(P)*    pvec,                                            \
            size_t          number)                                             \
    { vector_remove_end((uint8_t**) pvec, number); }












