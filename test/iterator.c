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

#define __STDC_FORMAT_MACROS

#include <adbus.h>
#include <stdio.h>
#include <dmem/vector.h>
#include <inttypes.h>

void error()
{
    printf("error\n");
    exit(-1);
}

char* ReadData(const char* filename, size_t* sz)
{
    FILE* f = fopen(filename, "rb");
    if (!f)
        abort();

    char* buf = NULL;
    size_t bufsize = 64 * 1024;
    while (!feof(f)) {
        buf = (char*) realloc(buf, bufsize);
        *sz += fread(buf + *sz, 1, bufsize - *sz, f);
        if (ferror(f)) {
            printf("File error\n");
            exit(-1);
        }
    }

    return buf;
}

static char* begin = NULL;
#define DIFF(p1, p2) ((int) ((char*) (p1) - (char*) (p2)))

void print_iter(adbus_Iterator* i)
{
    printf("ITER '%s' %d/%d\n",
            i->sig,
            DIFF(i->data, begin),
            (int) i->size);
}

void print_array(adbus_IterArray* a)
{
    printf("ARRAY '%.*s' %d/%d\n",
            (int) a->sigsz,
            a->sig,
            DIFF(a->data, begin),
            (int) a->size);
}

void print_variant(adbus_IterVariant* v)
{
    printf("VARIANT '%s' '%s' %d/%d\n",
            v->origsig,
            v->sig,
            DIFF(v->data, begin),
            (int) v->size);
}

DVECTOR_INIT(Array, adbus_IterArray);
DVECTOR_INIT(Variant, adbus_IterVariant);

int main(int argc, char* argv[])
{
    if (argc != 4)
        abort();

    adbus_Iterator i = {};
    begin = ReadData(argv[1], &i.size);
    i.data = begin;
    i.sig = argv[2];

    char* cmd = argv[3];
    printf("DATA '%s' %d\n", i.sig, (int) i.size);
    print_iter(&i);
    while (*cmd) {
        switch (*cmd++) 
        {
        case ADBUS_UINT8:
            {
                const uint8_t* v;
                if (adbus_iter_u8(&i, &v)) {
                    error();
                } else {
                    printf("U8 %d %d\n", DIFF(v, begin), (int) *v);
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_BOOLEAN:
            {
                const adbus_Bool* v;
                if (adbus_iter_bool(&i, &v)) {
                    error();
                } else {
                    printf("BOOL %d %d\n", DIFF(v, begin), (int) *v);
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_INT16:
            {
                const int16_t* v;
                if (adbus_iter_i16(&i, &v)) {
                    error();
                } else {
                    printf("I16 %d %d\n", DIFF(v, begin), (int) *v);
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_UINT16:
            {
                const uint16_t* v;
                if (adbus_iter_u16(&i, &v)) {
                    error();
                } else {
                    printf("U16 %d %d\n", DIFF(v, begin), (int) *v);
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_INT32:
            {
                const int32_t* v;
                if (adbus_iter_i32(&i, &v)) {
                    error();
                } else {
                    printf("I32 %d %d\n", DIFF(v, begin), (int) *v);
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_UINT32:
            {
                const uint32_t* v;
                if (adbus_iter_u32(&i, &v)) {
                    error();
                } else {
                    printf("U32 %d %u\n", DIFF(v, begin), (int) *v);
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_INT64:
            {
                const int64_t* v;
                if (adbus_iter_i64(&i, &v)) {
                    error();
                } else {
                    printf("I64 %d %"PRId64"\n", DIFF(v, begin), *v);
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_UINT64:
            {
                const uint64_t* v;
                if (adbus_iter_u64(&i, &v)) {
                    error();
                } else {
                    printf("U64 %d %"PRIu64"\n", DIFF(v, begin), *v);
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_DOUBLE:
            {
                const double* v;
                if (adbus_iter_double(&i, &v)) {
                    error();
                } else {
                    printf("DOUBLE %d %.30f\n", DIFF(v, begin), *v);
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_STRING:
            {
                size_t sz;
                const char* str;
                if (adbus_iter_string(&i, &str, &sz)) {
                    error();
                } else {
                    printf("STRING %d %d '%s'\n", DIFF(str, begin), (int) sz, str);
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_OBJECT_PATH:
            {
                size_t sz;
                const char* str;
                if (adbus_iter_objectpath(&i, &str, &sz)) {
                    error();
                } else {
                    printf("PATH %d %d '%s'\n", DIFF(str, begin), (int) sz, str);
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_SIGNATURE:
            {
                size_t sz;
                const char* str;
                if (adbus_iter_signature(&i, &str, &sz)) {
                    error();
                } else {
                    printf("SIGNATURE %d %d '%s'\n", DIFF(str, begin), (int) sz, str);
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_STRUCT_BEGIN:
            {
                if (adbus_iter_beginstruct(&i)) {
                    error();
                } else {
                    printf("STRUCT BEGIN\n");
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_STRUCT_END:
            {
                if (adbus_iter_endstruct(&i)) {
                    error();
                } else {
                    printf("STRUCT END\n");
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_DICTENTRY_BEGIN:
            {
                if (adbus_iter_begindictentry(&i)) {
                    error();
                } else {
                    printf("DICT ENTRY BEGIN\n");
                    print_iter(&i);
                }
            }
            break;

        case ADBUS_DICTENTRY_END:
            {
                if (adbus_iter_enddictentry(&i)) {
                    error();
                } else {
                    printf("DICT ENTRY END\n");
                    print_iter(&i);
                }
            }
            break;


        case ADBUS_ARRAY_BEGIN:
            {
                static d_Vector(Array) arrays = {};

                adbus_IterArray* a;
                switch (*cmd++) 
                {
                case 'B':
                    a = dv_push(Array, &arrays, 1);
                    if (adbus_iter_beginarray(&i, a)) {
                        error();
                    } else {
                        printf("ARRAY BEGIN\n");
                        print_array(a);
                        print_iter(&i);
                    }
                    break;
                case '?':
                    a = &dv_a(&arrays, dv_size(&arrays) - 1);
                    printf("ARRAY IN %d\n",
                            adbus_iter_inarray(&i, a));
                    print_array(a);
                    print_iter(&i);
                    break;
                case 'E':
                    a = &dv_a(&arrays, dv_size(&arrays) - 1);
                    if (adbus_iter_endarray(&i, a)) {
                        error();
                    } else {
                        printf("ARRAY END\n");
                        print_array(a);
                        print_iter(&i);
                    }
                    break;
                default:
                    abort();
                    break;
                }
            }
            break;

        case ADBUS_VARIANT_BEGIN:
            {
                static d_Vector(Variant) variants = {};

                adbus_IterVariant* v;
                switch (*cmd++) 
                {
                case 'B':
                    v = dv_push(Variant, &variants, 1);
                    if (adbus_iter_beginvariant(&i, v)) {
                        error();
                    } else {
                        printf("VARIANT BEGIN\n");
                        print_variant(v);
                        print_iter(&i);
                    }
                    break;
                case 'E':
                    v = &dv_a(&variants, dv_size(&variants) - 1);
                    if (adbus_iter_endvariant(&i, v)) {
                        error();
                    } else {
                        printf("VARIANT END\n");
                        print_variant(v);
                        print_iter(&i);
                    }
                    break;
                default:
                    abort();
                    break;
                }
            }
            break;

        }
    }


    free(begin);
    return 0;
}


