#pragma once

#include "kpool.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#ifndef INLINE
#   define INLINE static inline
#endif

#define KVECTOR_INIT(name, type)                                            \
    typedef struct {                                                        \
        size_t  size;                                                       \
        size_t  alloc;                                                      \
        type*   data;                                                       \
    } kv_##name##_t;                                                        \
    INLINE void kv_pool_free_##name(void* v)                                \
    {                                                                       \
        free(((kv_##name##_t*) v)->data);                                   \
    }                                                                       \
    INLINE kv_##name##_t* kv_pool_new_##name(kpool_t* pool)                 \
    {                                                                       \
        kv_##name##_t* v = PNEW(pool, kv_##name##_t);                       \
        kp_register(pool, &kv_pool_free_##name, v);                         \
        return v;                                                           \
    }                                                                       \
    INLINE kv_##name##_t* kv_new_##name() {                                 \
        return NEW(kv_##name##_t);                                          \
    }                                                                       \
    INLINE void kv_free_##name(kv_##name##_t* v)                            \
    {                                                                       \
        if (v) {                                                            \
            free(v->data);                                                  \
            free(v);                                                        \
        }                                                                   \
    }                                                                       \
    INLINE void kv_clear_##name(kv_##name##_t* v)                           \
    {                                                                       \
        v->size = 0;                                                        \
    }                                                                       \
    INLINE type* kv_release_##name(kv_##name##_t* v)                        \
    {                                                                       \
        type* ret = v->data;                                                \
        v->size = v->alloc = 0;                                             \
        v->data = NULL;                                                     \
        return ret;                                                         \
    }                                                                       \
    INLINE void kv_resize_##name(kv_##name##_t* v, size_t sz)               \
    {                                                                       \
        v->size = sz;                                                       \
        if (v->alloc >= sz)                                                 \
            return;                                                         \
        v->alloc = ((v->alloc) + 16) * 3 / 2;                               \
        if (v->alloc < sz)                                                  \
            v->alloc = sz;                                                  \
                                                                            \
        v->data = (type*) realloc(v->data, sizeof(type) * v->alloc);        \
    }                                                                       \
    INLINE type* kv_push_##name(kv_##name##_t* v, size_t num)               \
    {                                                                       \
        size_t old = v->size;                                               \
        kv_resize_##name(v, old + num);                                     \
        memset(v->data + old, 0, num * sizeof(type));                       \
        return v->data + old;                                               \
    }                                                                       \
    INLINE type* kv_insert_##name(kv_##name##_t* v,                         \
                                  size_t index,                             \
                                  size_t num)                               \
    {                                                                       \
        size_t old = v->size;                                               \
        assert(index <= old);                                               \
        kv_resize_##name(v, old + num);                                     \
        type* b = v->data + index;                                          \
        type* e = v->data + index + num;                                    \
        size_t numafter = old - index;                                      \
        if (numafter > 0)                                                   \
            memmove(e, b, numafter * sizeof(type));                         \
        memset(b, 0, num * sizeof(type));                                   \
        return b;                                                           \
    }                                                                       \
    INLINE void kv_pop_##name(kv_##name##_t* v, size_t num)                 \
    {                                                                       \
        size_t old = v->size;                                               \
        (void) old;                                                         \
        assert(num <= old);                                                 \
        v->size -= num;                                                     \
    }                                                                       \
    INLINE void kv_remove_##name(kv_##name##_t* v,                          \
                                 size_t index,                              \
                                 size_t num)                                \
    {                                                                       \
        size_t old = v->size;                                               \
        assert(index + num <= old);                                         \
        type* b = v->data + index;                                          \
        type* e = v->data + index + num;                                    \
        size_t numafter = old - (index + num);                              \
        if (numafter > 0)                                                   \
            memmove(b, e, numafter * sizeof(type));                         \
        kv_resize_##name(v, old - num);                                     \
    }                                                                       \
    INLINE type* kv_at_resize_##name(kv_##name##_t* v, size_t index)        \
    {                                                                       \
        if (v->size <= index)                                               \
            kv_push_##name(v, v->size - index);                             \
        return v->data + index;                                             \
    }

#define kvector_t(name)                     kv_##name##_t
#define kv_pool_new(name, pool)             kv_pool_new_##name(pool)
#define kv_new(name)                        kv_new_##name()
#define kv_free(name, pvec)                 kv_free_##name(pvec)
#define kv_release(name, pvec)              kv_release_##name(pvec)
#define kv_clear(name, pvec)                kv_clear_##name(pvec)
#define kv_push(name, pvec, num)            kv_push_##name(pvec, num)
#define kv_pop(name, pvec, num)             kv_pop_##name(pvec, num)
#define kv_insert(name, pvec, index, num)   kv_insert_##name(pvec, index, num)
#define kv_remove(name, pvec, index, num)   kv_remove_##name(pvec, index, num)
#define kv_at_resize(name, pvec, index)     *kv_at_resize_##name(pvec, index)

#define kv_size(pvec)       ((pvec)->size)
#define kv_a(pvec, index)   ((pvec)->data[index])
