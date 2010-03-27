/* vim: set et sts=4 ts=4 sw=4: */

#include "dmem/vector.h"

#if DMEM_VECTOR_CHECK

#define DUMMY_NULL ((void*) 22)

void dv_free_base(dv_base_t* v)
{
    if (v) {
        free(v->data == DUMMY_NULL ? NULL : v->data);
    }
}

void dv_clear_base(dv_base_t* v)
{
    v->size = 0;
    v->alloc = 0;
    free(v->data == DUMMY_NULL ? NULL : v->data);
    v->data = DUMMY_NULL;
}

void dv_expand_base(dv_base_t* v, size_t typesz, size_t incr)
{
    size_t newsz = v->size + incr;
    void* newdata = malloc(newsz * typesz);
    memcpy(newdata, v->data, v->size * typesz);
    memset(newdata + (v->size * typesz), '?', incr * typesz);
    free(v->data == DUMMY_NULL ? NULL : v->data);
    v->data = newdata;
    v->size = newsz;
    v->alloc = newsz;
}

void dv_shrink_base(dv_base_t* v, size_t typesz, size_t decr)
{
    size_t newsz = v->size - decr;
    void* newdata = malloc(newsz * typesz);
    assert(v->size >= decr);
    memcpy(newdata, v->data, newsz * typesz);
    free(v->data == DUMMY_NULL ? NULL : v->data);
    v->data = newdata;
    v->size = newsz;
    v->alloc = newsz;
}
#endif

