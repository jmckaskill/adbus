#include "kstring.h"

#include <stdarg.h>
#include <stdio.h>

#ifdef WIN32
int ks_vprintf(kstring_t* s, const char* format, va_list ap)
{
    va_list aq;
    va_copy(aq, ap);
    int nchars = _vscprintf(format, aq);
    va_end(aq);

    kv_pop(s, 1);
    char* dest = kv_push(kstring, s, nchars + 1);
    nchars = _vsnprintf(dest, nchars + 1, format, ap);

    return nchars;
}

#else
int ks_vprintf(kstring_t* s, const char* format, va_list ap)
{
    // Need to copy out ap incase we need to try again
    va_list aq;
    va_copy(aq, ap);
    kv_pop(kstring, s, 1); // remove nul
    char* dest = kv_push(kstring, s, 128 + 1);
    int nchars = vsnprintf(dest, 128 + 1, format, ap);
    if (nchars > 128) {
        // nchars now holds the number of characters needed
        kv_pop(kstring, s, 128 + 1);
        char* dest = kv_push(kstring, s, nchars + 1);
        nchars = vsnprintf(dest, nchars + 1, format, aq);
    } else {
        kv_pop(kstring, s, 128 - nchars);
    }
    va_end(aq);
    return nchars;
}
#endif
