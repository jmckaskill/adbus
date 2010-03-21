/* vim: et sts=4 ts=4 sw=4: */
#include "dmem/istring.h"

void dis_cat_n(d_IString* s, const char* str, size_t sz)
{
    const char* end = str + sz;
    while (str < end) {
        const char* nl = (const char*) memchr(str, '\n', end - str);
        if (nl == NULL) {
            ds_cat_n(&s->str, str, end - str);
            break;
        } else {
            ds_cat_n(&s->str, str, nl - str);
            ds_cat_char(&s->str, '\n');
            ds_cat_s(&s->str, &s->indent);
            str = nl + 1;
        }
    }
}


