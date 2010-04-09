/* vim: set et sts=4 ts=4 sw=4: */

#ifndef DMEM_VECTOR_H
#define DMEM_VECTOR_H

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "common.h"

#ifdef NDEBUG
#   define DV_MEMSET(p,x,s)
#else
#   define DV_MEMSET(p,x,s) memset(p,x,s)
#endif

typedef struct dv_base_t dv_base_t;

struct dv_base_t
{
  size_t size;
  size_t alloc;
  void*  data;
};

#ifdef __cplusplus
extern "C" {
#endif

#define DMEM_VECTOR_CHECK 0

#if DMEM_VECTOR_CHECK
    void dv_free_base(dv_base_t* v);
    void dv_clear_base(dv_base_t* v);
    void dv_expand_base(dv_base_t* v, size_t typesz, size_t incr);
    void dv_shrink_base(dv_base_t* v, size_t typesz, size_t decr);
#else
    DMEM_INLINE void dv_free_base(dv_base_t* v)
    { if (v) free(v->data); }
    DMEM_INLINE void dv_clear_base(dv_base_t* v)
    { v->size = 0; }
    DMEM_INLINE void dv_shrink_base(dv_base_t* v, size_t typesz, size_t decr)
    { (void) typesz; v->size -= decr; }
    DMEM_INLINE void dv_expand_base(dv_base_t* v, size_t typesz, size_t incr)
    {
        v->size += incr;
        if (v->alloc >= v->size)
            return;

        v->alloc = ((v->alloc) + 16) * 3 / 2;

        if (v->alloc < v->size)
            v->alloc = v->size;

        v->data = realloc(v->data, v->alloc * typesz);
    }
#endif

#ifdef __cplusplus
}
#endif

#define DVECTOR_INIT(name, type)                                            \
    typedef struct {                                                        \
        size_t  size;                                                       \
        size_t  alloc;                                                      \
        type*   data;                                                       \
    } dv_##name##_t;                                                        \
    typedef type dv_##name##_value_t;                                       \
    DMEM_INLINE void dv_init_##name(dv_##name##_t* v)                       \
    {                                                                       \
        memset(v, 0, sizeof(dv_##name##_t));                                \
    }                                                                       \
    DMEM_INLINE void dv_free_##name(dv_##name##_t* v)                       \
    { dv_free_base((dv_base_t*) v); }                                       \
    DMEM_INLINE void dv_clear_##name(dv_##name##_t* v)                      \
    { dv_clear_base((dv_base_t*) v); }                                      \
    DMEM_INLINE type* dv_release_##name(dv_##name##_t* v)                   \
    {                                                                       \
        type* ret = v->data;                                                \
        v->size = v->alloc = 0;                                             \
        v->data = NULL;                                                     \
        return ret;                                                         \
    }                                                                       \
    DMEM_INLINE type* dv_push_##name(dv_##name##_t* v, size_t num)          \
    {                                                                       \
        size_t old = v->size;                                               \
        dv_expand_base((dv_base_t*) v, sizeof(type), num);                  \
        return v->data + old;                                               \
    }                                                                       \
    DMEM_INLINE type* dv_insert_##name(dv_##name##_t* v,                    \
                                  size_t index,                             \
                                  size_t num)                               \
    {                                                                       \
        type* d;                                                            \
        size_t begin = index;                                               \
        size_t end = index + num;                                           \
        size_t oldsize = v->size;                                           \
        assert(begin <= v->size);                                           \
        dv_expand_base((dv_base_t*) v, sizeof(type), num);                  \
        d = v->data;                                                        \
        memmove(&d[end], &d[begin], (oldsize - begin) * sizeof(type));      \
        DV_MEMSET(&d[begin], '?', num * sizeof(type));                      \
        return &d[begin];                                                   \
    }                                                                       \
    DMEM_INLINE void dv_pop_##name(dv_##name##_t* v, size_t num)            \
    {                                                                       \
        assert(num <= v->size);                                             \
        dv_shrink_base((dv_base_t*) v, sizeof(type), num);                  \
    }                                                                       \
    DMEM_INLINE void dv_erase_##name(dv_##name##_t* v,                      \
                                 size_t index,                              \
                                 size_t num)                                \
    {                                                                       \
        type* d = v->data;                                                  \
        size_t begin = index;                                               \
        size_t end = index + num;                                           \
        assert(end <= v->size);                                             \
        memmove(&d[begin], &d[end], (v->size - end) * sizeof(type));        \
        dv_shrink_base((dv_base_t*) v, sizeof(type), num);                  \
    }                                                                       \


#define d_Vector(name)                      dv_##name##_t
#define dv_init(name, pvec)                 dv_init_##name(pvec)
#define dv_free(name, pvec)                 dv_free_##name(pvec)
#define dv_release(name, pvec)              dv_release_##name(pvec)
#define dv_clear(name, pvec)                dv_clear_##name(pvec)
#define dv_push(name, pvec, num)            dv_push_##name(pvec, num)
#define dv_pop(name, pvec, num)             dv_pop_##name(pvec, num)
#define dv_insert(name, pvec, index, num)   dv_insert_##name(pvec, index, num)
#define dv_erase(name, pvec, index, num)    dv_erase_##name(pvec, index, num)

#define dv_remove(name, pvec, test)                                         \
    do {                                                                    \
        size_t _index;                                                      \
        dv_##name##_value_t* ENTRY;                                         \
        for (_index = 0; _index < dv_size(pvec);) {                         \
            ENTRY = &dv_a(pvec, _index);                                    \
            if (test) {                                                     \
                dv_erase(name, pvec, _index, 1);                            \
            } else {                                                        \
                _index++;                                                   \
            }                                                               \
        }                                                                   \
	DMEM_MSC_WARNING(push)													\
	DMEM_MSC_WARNING(disable:4127)											\
    } while (0)																\
	DMEM_MSC_WARNING(pop)

#define dv_find(name, pvec, result, test)                                   \
    do {                                                                    \
        size_t _index;                                                      \
        dv_##name##_value_t* ENTRY;                                         \
        for (_index = 0; _index < dv_size(pvec); _index++) {                \
            ENTRY = &dv_a(pvec, _index);                                    \
            if (test) {                                                     \
                *result = &dv_a(pvec, _index);                              \
                break;                                                      \
            }                                                               \
        }                                                                   \
	DMEM_MSC_WARNING(push)													\
	DMEM_MSC_WARNING(disable:4127)											\
    } while (0)																\
	DMEM_MSC_WARNING(pop)

DMEM_INLINE size_t dv_a_check(size_t sz, size_t i)
{
    (void) i;
    (void) sz;
    assert(sz > i);
    return 0;
}

#define dv_size(pvec)       ((pvec)->size)
#define dv_a(pvec, index)   ((dv_a_check((pvec)->size, index) + (pvec)->data)[index])
#define dv_data(pvec)       ((pvec)->data)
#define dv_reserved(pvec)   ((pvec)->alloc)

#endif /* DMEM_VECTOR_H */

