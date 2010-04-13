/* vim: et sts=4 ts=4 sw=4: */

#ifdef _MSC_VER
#   define _CRT_SECURE_NO_DEPRECATE
#endif

#include "dmem/string.h"

#include <stdarg.h>
#include <stdio.h>

#ifndef va_copy
#   ifdef _MSC_VER
#       define va_copy(d,s) d = s
#   elif defined __GNUC__
#       define va_copy(d,s)	__builtin_va_copy(d,s)
#   else
#       error
#   endif
#endif

#ifdef _MSC_VER 
int ds_insert_vf(d_String* s, size_t index, const char* format, va_list ap)
{
    va_list aq;
    int nchars, nchars2;
    char after, *dest;

    if (index > ds_size(s)) {
        assert(0);
        return -1;
    }

    if (dv_size(s) == 0) {
        dv_push(dstring, s, 1);
        dv_a(s, 0) = '\0';
    }

    va_copy(aq, ap);
    nchars = _vscprintf(format, aq);

    /* Always give the vsnprintf calls an extra byte so that they can write
     * their nul terminator, but we replace it with after at the end
     */
    after = dv_a(s, index);
    dest = dv_insert(dstring, s, index, nchars);
    nchars2 = _vsnprintf(dest, nchars + 1, format, ap);
    assert(nchars2 == nchars);
    dv_a(s, index + nchars) = after;

    return nchars;
}

#else
int ds_insert_vf(d_String* s, size_t index, const char* format, va_list ap)
{
    va_list aq;
    int nchars;
    char after, *dest;

    if (index > ds_size(s)) {
        assert(0);
        return -1;
    }

    if (dv_size(s) == 0) {
        dv_push(dstring, s, 1);
        dv_a(s, 0) = '\0';
    }

    /* Need to copy out ap incase we need to try again */
    va_copy(aq, ap);
    /* Always give the vsnprintf calls an extra byte so that they can write
     * their nul terminator, but we replace it with after at the end
     */
    after = dv_a(s, index);
    dest = dv_insert(dstring, s, index, 128);
    nchars = vsnprintf(dest, 128 + 1, format, ap);
    if (nchars > 128) {
        /* nchars now holds the number of characters needed */
        dest = dv_insert(dstring, s, index + 128, nchars - 128) - 128;
        nchars = vsnprintf(dest, nchars + 1, format, aq);
    } else {
        size_t beg = index + nchars;
        size_t end = index + 128;
        dv_erase(dstring, s, beg, end - beg);
    }

    dv_a(s, index + nchars) = after;
    return nchars;
}
#endif
