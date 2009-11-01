#pragma once

#include "kvector.h"
#include <string.h>
#include <stdarg.h>

#ifndef INLINE
#   define INLINE static inline
#endif

#ifndef __cplusplus
    typedef unsigned char bool;
#   define false '\0'
#   define true (!false)
#endif

KVECTOR_INIT(kstring, char)

typedef kvector_t(kstring) kstring_t;

// ----------------------------------------------------------------------------

INLINE kstring_t* ks_pool_new(kpool_t* pool)
{
    kstring_t* s = kv_pool_new(kstring, pool);
    kv_push(kstring, s, 1);
    return s;
}

INLINE kstring_t* ks_new()
{
    kstring_t* s = kv_new(kstring);
    kv_push(kstring, s, 1);
    return s;
}

INLINE void ks_free(kstring_t* s)
{ kv_free(kstring, s); }

INLINE char* ks_release(kstring_t* s)
{
    char* ret = kv_release(kstring, s);
    kv_push(kstring, s, 1);
    return ret;
}

INLINE void ks_clear(kstring_t* s)
{ kv_remove(kstring, s, 0, kv_size(s) - 1); }

INLINE char ks_a(kstring_t* s, size_t i)
{ return kv_a(s, i); }

INLINE char* ks_data(kstring_t* s)
{ return &kv_a(s, 0); }

INLINE const char* ks_cstr(const kstring_t* s)
{ return &kv_a(s, 0); }

INLINE size_t ks_size(const kstring_t* s)
{ return kv_size(s) - 1; }

// ----------------------------------------------------------------------------

int ks_vprintf(kstring_t* s, const char* format, va_list ap);

INLINE int ks_printf(kstring_t* s, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    int ret = ks_vprintf(s, format, ap);
    va_end(ap);
    return ret;
}

// ----------------------------------------------------------------------------

INLINE void ks_set_n(kstring_t* s, const char* r, size_t n)
{
    kv_clear(kstring, s);
    char* dest = kv_push(kstring, s, n);
    memcpy(dest, r, n);
    kv_push(kstring, s, 1);
}

INLINE void ks_set_s(kstring_t* s, const kstring_t* r)
{ ks_set_n(s, ks_cstr(r), ks_size(r)); }

INLINE void ks_set(kstring_t* s, const char* r)
{ ks_set_n(s, r, strlen(r)); }

// ----------------------------------------------------------------------------

INLINE void ks_cat_n(kstring_t* s, const char* r, size_t n)
{
    kv_pop(kstring, s, 1);
    char* dest = kv_push(kstring, s, n);
    memcpy(dest, r, n);
    kv_push(kstring, s, 1);
}

INLINE void ks_cat_s(kstring_t* s, const kstring_t* r)
{ ks_cat_n(s, ks_cstr(r), ks_size(r)); }

INLINE void ks_cat(kstring_t* s, const char* r)
{ ks_cat_n(s, r, strlen(r)); }

INLINE void ks_cat_char(kstring_t* s, int ch)
{ ks_cat_n(s, (const char*) &ch, 1); }

// ----------------------------------------------------------------------------

INLINE void ks_insert_n(kstring_t* s, size_t index, const char* r, size_t n)
{
    char* dest = kv_insert(kstring, s, index, n);
    memcpy(dest, r, n);
}

INLINE void ks_insert_s(kstring_t* s, size_t index, const kstring_t* r)
{ ks_insert_n(s, index, ks_cstr(r), ks_size(r)); }

INLINE void ks_insert(kstring_t* s, size_t index, const char* r)
{ ks_insert_n(s, index, r, strlen(r)); }

INLINE void ks_insert_char(kstring_t* s, size_t index, int ch)
{ ks_insert_n(s, index, (const char*) &ch, 1); }

// ----------------------------------------------------------------------------

INLINE void ks_remove(kstring_t* s, size_t index, size_t n)
{ kv_remove(kstring, s, index, n); }

INLINE void ks_remove_end(kstring_t* s, size_t n)
{
    kv_pop(kstring, s, n + 1);
    kv_push(kstring, s, 1);
}

// ----------------------------------------------------------------------------

INLINE int ks_cmp_n(const kstring_t* s, const char* r, size_t n)
{
    if (ks_size(s) != n)
        return (int) (ks_size(s) - n);
    else
        return memcmp(ks_cstr(s), r, n);
}

INLINE int ks_cmp_s(const kstring_t* s, const kstring_t* r)
{ return ks_cmp_n(s, ks_cstr(r), ks_size(r)); }

INLINE int ks_cmp(const kstring_t* s, const char* r)
{ return strcmp(ks_cstr(s), r); }

// ----------------------------------------------------------------------------

INLINE bool ks_begins_with_n(const kstring_t* s, const char* r, size_t n)
{
    if (ks_size(s) < n)
        return false;
    else
        return memcmp(ks_cstr(s), r, n) == 0;
}

INLINE bool ks_begins_with_s(const kstring_t* s, const kstring_t* r)
{ return ks_begins_with_n(s, ks_cstr(r), ks_size(r)); }

INLINE bool ks_begins_with(const kstring_t* s, const char* r)
{ return ks_begins_with_n(s, r, strlen(r)); }

// ----------------------------------------------------------------------------

INLINE bool ks_ends_with_n(const kstring_t* s, const char* r, size_t n)
{
    if (ks_size(s) < n)
        return false;
    else
      return memcmp(ks_cstr(s) + ks_size(s) - n, r, n) == 0;
}

INLINE bool ks_ends_with_s(const kstring_t* s, const kstring_t* r)
{ return ks_ends_with_n(s, ks_cstr(r), ks_size(r)); }

INLINE bool ks_ends_with(const kstring_t* s, const char* r)
{ return ks_ends_with_n(s, r, strlen(r)); }


