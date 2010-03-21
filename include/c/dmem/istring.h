/* vim: set et sts=4 ts=4 sw=4: */

#ifndef DMEM_ISTRING_H
#define DMEM_ISTRING_H

#include "dmem/string.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct d_IString d_IString;

struct d_IString
{
    d_String str;
    d_String indent;
    d_String format;
};


INLINE void dis_init(d_IString* s)
{ memset(s, 0, sizeof(d_IString)); }

INLINE void dis_free(d_IString* s)
{ 
    ds_free(&s->str);
    ds_free(&s->indent);
}

INLINE char* dis_release(d_IString* s)
{
    ds_clear(&s->indent);
    return ds_release(&s->str);
}

INLINE void dis_clear(d_IString* s)
{
    ds_clear(&s->indent);
    ds_clear(&s->str);
}

INLINE const char* dis_cstr(d_IString* s)
{ return ds_cstr(&s->str); }

INLINE size_t dis_size(d_IString* s)
{ return ds_size(&s->str); }

INLINE void dis_indent(d_IString* s)
{ ds_cat_n(&s->indent, "    ", 4); }

INLINE void dis_unindent(d_IString* s)
{ ds_remove_end(&s->indent, 4); }

extern void dis_cat_n(d_IString* s, const char* str, size_t sz);

INLINE void dis_cat(d_IString* s, const char* str)
{ dis_cat_n(s, str, strlen(str)); }

INLINE void dis_cat_f(d_IString* s, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    ds_set_vf(&s->format, format, ap);
    dis_cat_n(s, ds_cstr(&s->format), ds_size(&s->format));
}

#ifdef __cplusplus
}
#endif

#endif /* DMEM_ISTRING_H */

