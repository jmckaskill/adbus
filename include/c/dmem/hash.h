/* The MIT License

   Copyright (c) 2008, by Attractive Chaos <attractivechaos@aol.co.uk>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

/*
  An example:

#include "dmem/hash.h"
DHASH_MAP_INIT_INT(32, char)
int main() {
    int ret, is_missing;
    dh_Iter k;
    d_Hash(32) *h = dh_new(32);
    k = dh_put(32, h, 5, &ret);
    if (!ret) dh_del(32, h, k);
    dh_value(h, k) = 10;
    k = dh_get(32, h, 10);
    is_missing = (k == dh_end(h));
    k = dh_get(32, h, 5);
    dh_del(32, h, k);
    for (k = dh_begin(h); k != dh_end(h); ++k)
        if (dh_exist(h, k)) dh_value(h, k) = 1;
    dh_free(32, h);
    return 0;
}
*/

/*
  2008-09-19 (0.2.3):

    * Corrected the example
    * Improved interfaces

  2008-09-11 (0.2.2):

    * Improved speed a little in dh_put()

  2008-09-10 (0.2.1):

    * Added dh_clear()
    * Fixed a compiling error

  2008-09-02 (0.2.0):

    * Changed to token concatenation which increases flexibility.

  2008-08-31 (0.1.2):

    * Fixed a bug in dh_get(), which has not been tested previously.

  2008-08-31 (0.1.1):

    * Added destructor
*/


#ifndef __AC_DHASH_H
#define __AC_DHASH_H

#define AC_VERSION_DHASH_H "0.2.2"

#include "common.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER 
#   pragma warning(disable:4127) // conditional expression is constant
#endif

typedef uint32_t khint_t;
typedef khint_t dh_Iter;

#define __ac_HASH_PRIME_SIZE 32
static const uint32_t __ac_prime_list[__ac_HASH_PRIME_SIZE] =
{
  0ul,          3ul,          11ul,         23ul,         53ul,
  97ul,         193ul,        389ul,        769ul,        1543ul,
  3079ul,       6151ul,       12289ul,      24593ul,      49157ul,
  98317ul,      196613ul,     393241ul,     786433ul,     1572869ul,
  3145739ul,    6291469ul,    12582917ul,   25165843ul,   50331653ul,
  100663319ul,  201326611ul,  402653189ul,  805306457ul,  1610612741ul,
  3221225473ul, 4294967291ul
};

#define __ac_isempty(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&2)
#define __ac_isdel(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&1)
#define __ac_iseither(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&3)
#define __ac_set_isdel_false(flag, i) (flag[i>>4]&=~(1ul<<((i&0xfU)<<1)))
#define __ac_set_isempty_false(flag, i) (flag[i>>4]&=~(2ul<<((i&0xfU)<<1)))
#define __ac_set_isboth_false(flag, i) (flag[i>>4]&=~(3ul<<((i&0xfU)<<1)))
#define __ac_set_isdel_true(flag, i) (flag[i>>4]|=1ul<<((i&0xfU)<<1))

static const double __ac_HASH_UPPER = 0.77;

#define DHASH_INIT(name, khkey_t, khval_t, dh_is_map, __hash_func, __hash_equal) \
    typedef struct {                                                    \
        khint_t n_buckets, size, n_occupied, upper_bound;               \
        uint32_t *flags;                                                \
        khkey_t *keys;                                                  \
        khval_t *vals;                                                  \
    } dh_##name##_t;                                                    \
    DMEM_INLINE void dh_free_##name(dh_##name##_t *h)                   \
    {                                                                   \
        if (h) {                                                        \
            free(h->keys);                                              \
            free(h->flags);                                             \
            free(h->vals);                                              \
        }                                                               \
    }                                                                   \
    DMEM_INLINE void dh_clear_##name(dh_##name##_t *h)                  \
    {                                                                   \
        if (h && h->flags) { \
            memset(h->flags, 0xaa, ((h->n_buckets>>4) + 1) * sizeof(uint32_t)); \
            h->size = h->n_occupied = 0;                                \
        }                                                               \
    }                                                                   \
    DMEM_INLINE khint_t dh_get_##name(dh_##name##_t *h, khkey_t key)    \
    {                                                                   \
        if (h->n_buckets) {                                             \
            khint_t inc, k, i, last;                                    \
            k = __hash_func(key); i = k % h->n_buckets;                 \
            inc = 1 + k % (h->n_buckets - 1); last = i;                 \
            while (!__ac_isempty(h->flags, i) && (__ac_isdel(h->flags, i) || !__hash_equal(h->keys[i], key))) { \
                if (i + inc >= h->n_buckets) i = i + inc - h->n_buckets; \
                else i += inc;                                          \
                if (i == last) return h->n_buckets;                     \
            }                                                           \
            return __ac_iseither(h->flags, i)? h->n_buckets : i;        \
        } else return 0;                                                \
    }                                                                   \
    DMEM_INLINE void dh_resize_##name(dh_##name##_t *h, khint_t new_n_buckets) \
    {                                                                   \
        uint32_t *new_flags = 0;                                        \
        khint_t j = 1;                                                  \
        {                                                               \
            khint_t t = __ac_HASH_PRIME_SIZE - 1;                       \
            while (__ac_prime_list[t] > new_n_buckets) --t;             \
            new_n_buckets = __ac_prime_list[t+1];                       \
            if (h->size >= (khint_t)(new_n_buckets * __ac_HASH_UPPER + 0.5)) j = 0; \
            else {                                                      \
                new_flags = (uint32_t*)malloc(((new_n_buckets>>4) + 1) * sizeof(uint32_t)); \
                memset(new_flags, 0xaa, ((new_n_buckets>>4) + 1) * sizeof(uint32_t)); \
                if (h->n_buckets < new_n_buckets) {                     \
                    h->keys = (khkey_t*)realloc(h->keys, new_n_buckets * sizeof(khkey_t)); \
                    if (dh_is_map)                                      \
                        h->vals = (khval_t*)realloc(h->vals, new_n_buckets * sizeof(khval_t)); \
                }                                                       \
            }                                                           \
        }                                                               \
        if (j) {                                                        \
            for (j = 0; j != h->n_buckets; ++j) {                       \
                if (__ac_iseither(h->flags, j) == 0) {                  \
                    khkey_t key = h->keys[j];                           \
                    khval_t val;                                        \
                    if (dh_is_map) val = h->vals[j];                    \
                    __ac_set_isdel_true(h->flags, j);                   \
                    while (1) {                                         \
                        khint_t inc, k, i;                              \
                        k = __hash_func(key);                           \
                        i = k % new_n_buckets;                          \
                        inc = 1 + k % (new_n_buckets - 1);              \
                        while (!__ac_isempty(new_flags, i)) {           \
                            if (i + inc >= new_n_buckets) i = i + inc - new_n_buckets; \
                            else i += inc;                              \
                        }                                               \
                        __ac_set_isempty_false(new_flags, i);           \
                        if (i < h->n_buckets && __ac_iseither(h->flags, i) == 0) { \
                            { khkey_t tmp = h->keys[i]; h->keys[i] = key; key = tmp; } \
                            if (dh_is_map) { khval_t tmp = h->vals[i]; h->vals[i] = val; val = tmp; } \
                            __ac_set_isdel_true(h->flags, i);           \
                        } else {                                        \
                            h->keys[i] = key;                           \
                            if (dh_is_map) h->vals[i] = val;            \
                            break;                                      \
                        }                                               \
                    }                                                   \
                }                                                       \
            }                                                           \
            if (h->n_buckets > new_n_buckets) {                         \
                h->keys = (khkey_t*)realloc(h->keys, new_n_buckets * sizeof(khkey_t)); \
                if (dh_is_map)                                          \
                    h->vals = (khval_t*)realloc(h->vals, new_n_buckets * sizeof(khval_t)); \
            }                                                           \
            free(h->flags);                                             \
            h->flags = new_flags;                                       \
            h->n_buckets = new_n_buckets;                               \
            h->n_occupied = h->size;                                    \
            h->upper_bound = (khint_t)(h->n_buckets * __ac_HASH_UPPER + 0.5); \
        }                                                               \
    }                                                                   \
    DMEM_INLINE khint_t dh_put_##name(dh_##name##_t *h, khkey_t key, int *added) \
    {                                                                   \
        khint_t x;                                                      \
        if (h->n_occupied >= h->upper_bound) {                          \
            if (h->n_buckets > (h->size<<1)) dh_resize_##name(h, h->n_buckets - 1); \
            else dh_resize_##name(h, h->n_buckets + 1);                 \
        }                                                               \
        {                                                               \
            khint_t inc, k, i, site, last;                              \
            x = site = h->n_buckets; k = __hash_func(key); i = k % h->n_buckets; \
            if (__ac_isempty(h->flags, i)) x = i;                       \
            else {                                                      \
                inc = 1 + k % (h->n_buckets - 1); last = i;             \
                while (!__ac_isempty(h->flags, i) && (__ac_isdel(h->flags, i) || !__hash_equal(h->keys[i], key))) { \
                    if (__ac_isdel(h->flags, i)) site = i;              \
                    if (i + inc >= h->n_buckets) i = i + inc - h->n_buckets; \
                    else i += inc;                                      \
                    if (i == last) { x = site; break; }                 \
                }                                                       \
                if (x == h->n_buckets) {                                \
                    if (__ac_isempty(h->flags, i) && site != h->n_buckets) x = site; \
                    else x = i;                                         \
                }                                                       \
            }                                                           \
        }                                                               \
        if (__ac_isempty(h->flags, x)) {                                \
            h->keys[x] = key;                                           \
            __ac_set_isboth_false(h->flags, x);                         \
            ++h->size; ++h->n_occupied;                                 \
            *added = 1;                                                 \
        } else if (__ac_isdel(h->flags, x)) {                           \
            h->keys[x] = key;                                           \
            __ac_set_isboth_false(h->flags, x);                         \
            ++h->size;                                                  \
            *added = 2;                                                 \
        } else *added = 0;                                              \
        return x;                                                       \
    }                                                                   \
    DMEM_INLINE void dh_del_##name(dh_##name##_t *h, khint_t x)         \
    {                                                                   \
        if (x != h->n_buckets && !__ac_iseither(h->flags, x)) {         \
            __ac_set_isdel_true(h->flags, x);                           \
            --h->size;                                                  \
        }                                                               \
    }

/* --- BEGIN OF HASH FUNCTIONS --- */

#define dh_uint32_hash_func(key) (uint32_t)(key)
#define dh_uint32_hash_equal(a, b) (a == b)
#define dh_uint64_hash_func(key) (uint32_t)((key)>>33^(key)^(key)<<11)
#define dh_uint64_hash_equal(a, b) (a == b)
DMEM_INLINE khint_t __ac_X31_hash_string(const char *s)
{
    khint_t h = *s;
    if (h) for (++s ; *s; ++s) h = (h << 5) - h + *s;
    return h;
}
#define dh_str_hash_func(key) __ac_X31_hash_string(key)
#define dh_str_hash_equal(a, b) (strcmp(a, b) == 0)

typedef struct dh_strsz_t
{
    const char* str;
    size_t      sz;
} dh_strsz_t;

DMEM_INLINE khint_t __ac_X31_hash_stringsz(dh_strsz_t s)
{
    size_t i;
    khint_t h;

    if (s.sz == 0)
        return 0;

    h = *s.str;
    for (i = 1; i < s.sz; i++)
        h = (h << 5) - h + s.str[i];
    return h;
}

#define dh_strsz_hash_func(key) __ac_X31_hash_stringsz(key)
#define dh_strsz_hash_equal(a, b) (a.sz == b.sz && memcmp(a.str, b.str, a.sz) == 0)

/* --- END OF HASH FUNCTIONS --- */

/* Other necessary macros... */

#define d_Hash(name) dh_##name##_t

#define dh_free(name, h) dh_free_##name(h)
#define dh_clear(name, h) dh_clear_##name(h)
#define dh_resize(name, h, s) dh_resize_##name(h, s)
#define dh_put(name, h, k, a) dh_put_##name(h, k, a)
#define dh_get(name, h, k) dh_get_##name(h, k)
#define dh_del(name, h, k) dh_del_##name(h, k)

#define dh_exist(h, x) (!__ac_iseither((h)->flags, (x)))
#define dh_key(h, x) ((h)->keys[x])
#define dh_val(h, x) ((h)->vals[x])
#define dh_value(h, x) ((h)->vals[x])
#define dh_begin(h) (khint_t)(0)
#define dh_end(h) ((h)->n_buckets)
#define dh_size(h) ((h)->size)
#define dh_n_buckets(h) ((h)->n_buckets)

/* More conenient interfaces */

#define DHASH_SET_INIT_UINT32(name)                                     \
    DHASH_INIT(name, uint32_t, char, 0, dh_uint32_hash_func, dh_uint32_hash_equal)

#define DHASH_MAP_INIT_UINT32(name, khval_t)                            \
    DHASH_INIT(name, uint32_t, khval_t, 1, dh_uint32_hash_func, dh_uint32_hash_equal)

#define DHASH_SET_INIT_UINT64(name)                                     \
    DHASH_INIT(name, uint64_t, char, 0, dh_uint64_hash_func, dh_uint64_hash_equal)

#define DHASH_MAP_INIT_UINT64(name, khval_t)                            \
    DHASH_INIT(name, uint64_t, khval_t, 1, dh_uint64_hash_func, dh_uint64_hash_equal)

typedef const char *dh_cstr_t;
#define DHASH_SET_INIT_STR(name)                                        \
    DHASH_INIT(name, dh_cstr_t, char, 0, dh_str_hash_func, dh_str_hash_equal)

#define DHASH_MAP_INIT_STR(name, khval_t)                               \
    DHASH_INIT(name, dh_cstr_t, khval_t, 1, dh_str_hash_func, dh_str_hash_equal)

#define DHASH_MAP_INIT_STRSZ(name, khval_t)                             \
    DHASH_INIT(name, dh_strsz_t, khval_t, 1, dh_strsz_hash_func, dh_strsz_hash_equal)

#endif /* __AC_DHASH_H */

