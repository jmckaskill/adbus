/* vim: set et sts=4 ts=4 sw=4: */

#ifndef DMEM_VECTOR_H
#define DMEM_VECTOR_H

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#ifndef INLINE
#   define INLINE static inline
#endif

#ifdef NDEBUG
#   define DV_MEMSET(p,x,s)
#else
#   define DV_MEMSET(p,x,s) memset(p,x,s)
#endif

#define DVECTOR_INIT(name, type)                                            \
    typedef struct {                                                        \
        size_t  size;                                                       \
        size_t  alloc;                                                      \
        type*   data;                                                       \
    } dv_##name##_t;                                                        \
    INLINE void dv_init_##name(dv_##name##_t* v)                            \
    {                                                                       \
        memset(v, 0, sizeof(dv_##name##_t));                                \
    }                                                                       \
    INLINE void dv_free_##name(dv_##name##_t* v)                            \
    {                                                                       \
        if (v) {                                                            \
            free(v->data);                                                  \
        }                                                                   \
    }                                                                       \
    INLINE void dv_shred_##name(dv_##name##_t* v)                           \
    {                                                                       \
        type* end = v->data + v->size;                                      \
        type* allocend = v->data + v->alloc;                                \
        (void) end;                                                         \
        (void) allocend;                                                    \
        DV_MEMSET(end, '?', allocend - end);                                \
    }                                                                       \
    INLINE void dv_clear_##name(dv_##name##_t* v)                           \
    {                                                                       \
        v->size = 0;                                                        \
        dv_shred_##name(v);                                                 \
    }                                                                       \
    INLINE type* dv_release_##name(dv_##name##_t* v)                        \
    {                                                                       \
        type* ret = v->data;                                                \
        v->size = v->alloc = 0;                                             \
        v->data = NULL;                                                     \
        return ret;                                                         \
    }                                                                       \
    INLINE void dv_reserve_##name(dv_##name##_t* v, size_t sz)              \
    {                                                                       \
        if (v->alloc >= sz)                                                 \
            return;                                                         \
        v->alloc = ((v->alloc) + 16) * 3 / 2;                               \
        if (v->alloc < sz)                                                  \
            v->alloc = sz;                                                  \
                                                                            \
        v->data = (type*) realloc(v->data, sizeof(type) * v->alloc);        \
        dv_shred_##name(v);                                                 \
    }                                                                       \
    INLINE void dv_resize_##name(dv_##name##_t* v, size_t sz)               \
    {                                                                       \
        dv_reserve_##name(v, sz);                                           \
        v->size = sz;                                                       \
    }                                                                       \
    INLINE type* dv_push_##name(dv_##name##_t* v, size_t num)               \
    {                                                                       \
        type* b;                                                            \
        size_t old = v->size;                                               \
        dv_resize_##name(v, old + num);                                     \
        b = v->data + old;                                                  \
        DV_MEMSET(b, '?', num * sizeof(type));                              \
        return b;                                                           \
    }                                                                       \
    INLINE type* dv_insert_##name(dv_##name##_t* v,                         \
                                  size_t index,                             \
                                  size_t num)                               \
    {                                                                       \
        type *b;                                                            \
        type *e;                                                            \
        size_t old = v->size;                                               \
        size_t numafter = old - index;                                      \
        assert(index <= old);                                               \
        dv_resize_##name(v, old + num);                                     \
        b = v->data + index;                                                \
        e = v->data + index + num;                                          \
        if (numafter > 0)                                                   \
            memmove(e, b, numafter * sizeof(type));                         \
        DV_MEMSET(b, '?', num * sizeof(type));                              \
        return b;                                                           \
    }                                                                       \
    INLINE void dv_pop_##name(dv_##name##_t* v, size_t num)                 \
    {                                                                       \
        assert(num <= v->size);                                             \
        v->size -= num;                                                     \
        dv_shred_##name(v);                                                 \
    }                                                                       \
    INLINE void dv_remove_##name(dv_##name##_t* v,                          \
                                 size_t index,                              \
                                 size_t num)                                \
    {                                                                       \
        type* b = v->data + index;                                          \
        type* e = v->data + index + num;                                    \
        size_t numafter = v->size - (index + num);                          \
        assert(index + num <= v->size);                                     \
        if (numafter > 0)                                                   \
            memmove(b, e, numafter * sizeof(type));                         \
        v->size -= num;                                                     \
        dv_shred_##name(v);                                                 \
    }

#define d_Vector(name)                      dv_##name##_t
#define dv_init(name, pvec)                 dv_init_##name(pvec)
#define dv_free(name, pvec)                 dv_free_##name(pvec)
#define dv_reserve(name, pvec, sz)          dv_reserve_##name(pvec, sz)
#define dv_release(name, pvec)              dv_release_##name(pvec)
#define dv_clear(name, pvec)                dv_clear_##name(pvec)
#define dv_push(name, pvec, num)            dv_push_##name(pvec, num)
#define dv_pop(name, pvec, num)             dv_pop_##name(pvec, num)
#define dv_insert(name, pvec, index, num)   dv_insert_##name(pvec, index, num)
#define dv_remove(name, pvec, index, num)   dv_remove_##name(pvec, index, num)

#define dv_size(pvec)       ((pvec)->size)
#define dv_a(pvec, index)   ((pvec)->data[index])
#define dv_data(pvec)       ((pvec)->data)

#endif /* DMEM_VECTOR_H */

