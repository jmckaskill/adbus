/* vim: set et sts=4 ts=4 sw=4: */

#ifndef DMEM_STRING_H
#define DMEM_STRING_H

#include "common.h"

#include "dmem/vector.h"
#include <string.h>
#include <stdarg.h>


#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
#   define DS_BOOL    bool 
#   define DS_FALSE   false
#   define DS_TRUE    true
#else
#   define DS_BOOL    int 
#   define DS_FALSE   0 
#   define DS_TRUE    1 
#endif

DVECTOR_INIT(dstring, char)

typedef d_Vector(dstring) d_String;

/* ------------------------------------------------------------------------- */

DMEM_INLINE void ds_init(d_String* s)
{ dv_init(dstring, s); }

DMEM_INLINE void ds_free(d_String* s)
{ dv_free(dstring, s); }

DMEM_INLINE char* ds_release(d_String* s)
{ 
    return dv_release(dstring, s);
}

DMEM_INLINE void ds_clear(d_String* s)
{ dv_clear(dstring, s); }

DMEM_INLINE char ds_a(d_String* s, size_t i)
{ return dv_a(s, i); }

DMEM_INLINE char* ds_data(d_String* s)
{ return &dv_a(s, 0); }

DMEM_INLINE const char* ds_cstr(const d_String* s)
{ 
    if (dv_size(s) > 0)
        return &dv_a(s, 0); 
    else
        return "";
}

DMEM_INLINE size_t ds_size(const d_String* s)
{ 
    if (dv_size(s) > 0)
        return dv_size(s) - 1;
    else
        return 0;
}

/* ------------------------------------------------------------------------- */

int ds_insert_vf(d_String* s, size_t index, const char* format, va_list ap);

DMEM_INLINE int ds_insert_f(d_String* s, size_t index, const char* format, ...)
{
    int ret;
    va_list ap;

    va_start(ap, format);
    ret = ds_insert_vf(s, index, format, ap);
    va_end(ap);
    return ret;
}

/* ------------------------------------------------------------------------- */

DMEM_INLINE int ds_cat_vf(d_String* s, const char* format, va_list ap)
{ return ds_insert_vf(s, ds_size(s), format, ap); }

DMEM_INLINE int ds_cat_f(d_String* s, const char* format, ...)
{
    int ret;
    va_list ap;

    va_start(ap, format);
    ret = ds_insert_vf(s, ds_size(s), format, ap);
    va_end(ap);
    return ret;
}

/* ------------------------------------------------------------------------- */

DMEM_INLINE int ds_set_vf(d_String* s, const char* format, va_list ap)
{
    ds_clear(s);
    return ds_insert_vf(s, 0, format, ap);
}

DMEM_INLINE int ds_set_f(d_String* s, const char* format, ...)
{
    int ret;
    va_list ap;

    ds_clear(s);
    va_start(ap, format);
    ret = ds_insert_vf(s, 0, format, ap);
    va_end(ap);
    return ret;
}

/* ------------------------------------------------------------------------- */

DMEM_INLINE void ds_set_n(d_String* s, const char* r, size_t n)
{
    char* dest;

    dv_clear(dstring, s);
    dest = dv_push(dstring, s, n + 1);
    memcpy(dest, r, n);
    dv_a(s, dv_size(s) - 1) = '\0';
}

DMEM_INLINE void ds_set_s(d_String* s, const d_String* r)
{ ds_set_n(s, ds_cstr(r), ds_size(r)); }

DMEM_INLINE void ds_set(d_String* s, const char* r)
{ ds_set_n(s, r, strlen(r)); }

/* ------------------------------------------------------------------------- */

DMEM_INLINE void ds_cat_n(d_String* s, const char* r, size_t n)
{
    char* dest = (dv_size(s) == 0)
               ? dv_push(dstring, s, n + 1)
               : dv_push(dstring, s, n) - 1;
    memcpy(dest, r, n);
    dv_a(s, dv_size(s) - 1) = '\0';
}

DMEM_INLINE void ds_cat_s(d_String* s, const d_String* r)
{ ds_cat_n(s, ds_cstr(r), ds_size(r)); }

DMEM_INLINE void ds_cat(d_String* s, const char* r)
{ ds_cat_n(s, r, strlen(r)); }

DMEM_INLINE void ds_cat_char(d_String* s, int ch)
{ 
    char* dest = (dv_size(s) == 0)
               ? dv_push(dstring, s, 1 + 1)
               : dv_push(dstring, s, 1) - 1;
    dest[0] = (unsigned char) ch;
    dest[1] = '\0';
}

/* ------------------------------------------------------------------------- */

DMEM_INLINE void ds_insert_n(d_String* s, size_t index, const char* r, size_t n)
{
    assert(index <= ds_size(s));
    if (index == ds_size(s)) {
        ds_cat_n(s, r, n);
    } else {
        char* dest = dv_insert(dstring, s, index, n);
        memcpy(dest, r, n);
    }
}

DMEM_INLINE void ds_insert_s(d_String* s, size_t index, const d_String* r)
{ ds_insert_n(s, index, ds_cstr(r), ds_size(r)); }

DMEM_INLINE void ds_insert(d_String* s, size_t index, const char* r)
{ ds_insert_n(s, index, r, strlen(r)); }

DMEM_INLINE void ds_insert_char(d_String* s, size_t index, int ch)
{ 
    assert(index <= ds_size(s));
    if (index == ds_size(s)) {
        ds_cat_char(s, ch);
    } else {
        char* dest = dv_insert(dstring, s, index, 1);
        *dest = (unsigned char) ch;
    }
}

/* ------------------------------------------------------------------------- */

DMEM_INLINE void ds_erase(d_String* s, size_t index, size_t n)
{ 
    assert(n <= ds_size(s));
    dv_erase(dstring, s, index, n); 
}

DMEM_INLINE void ds_erase_end(d_String* s, size_t n)
{
    assert(n <= ds_size(s));
    if (n > 0) {
        dv_pop(dstring, s, n);
        dv_a(s, dv_size(s) - 1) = '\0';
    }
}

/* ------------------------------------------------------------------------- */

DMEM_INLINE int ds_cmp_n(const d_String* s, const char* r, size_t n)
{
    if (ds_size(s) != n)
        return (int) (ds_size(s) - n);
    else
        return memcmp(ds_cstr(s), r, n);
}

DMEM_INLINE int ds_cmp_s(const d_String* s, const d_String* r)
{ return ds_cmp_n(s, ds_cstr(r), ds_size(r)); }

DMEM_INLINE int ds_cmp(const d_String* s, const char* r)
{ return strcmp(ds_cstr(s), r); }

/* ------------------------------------------------------------------------- */

DMEM_INLINE DS_BOOL ds_begins_with_n(const d_String* s, const char* r, size_t n)
{
    if (ds_size(s) < n)
        return DS_FALSE;
    else
        return memcmp(ds_cstr(s), r, n) == 0;
}

DMEM_INLINE DS_BOOL ds_begins_with_s(const d_String* s, const d_String* r)
{ return ds_begins_with_n(s, ds_cstr(r), ds_size(r)); }

DMEM_INLINE DS_BOOL ds_begins_with(const d_String* s, const char* r)
{ return ds_begins_with_n(s, r, strlen(r)); }

/* ------------------------------------------------------------------------- */

DMEM_INLINE DS_BOOL ds_ends_with_n(const d_String* s, const char* r, size_t n)
{
    if (ds_size(s) < n)
        return DS_FALSE;
    else
      return memcmp(ds_cstr(s) + ds_size(s) - n, r, n) == 0;
}

DMEM_INLINE DS_BOOL ds_ends_with_s(const d_String* s, const d_String* r)
{ return ds_ends_with_n(s, ds_cstr(r), ds_size(r)); }

DMEM_INLINE DS_BOOL ds_ends_with(const d_String* s, const char* r)
{ return ds_ends_with_n(s, r, strlen(r)); }

#ifdef __cplusplus
}
#endif


#endif /* DMEM_STRING_H */

