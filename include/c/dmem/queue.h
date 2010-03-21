/* vim: set et sts=4 ts=4 sw=4: */

#ifndef DMEM_QUEUE_H
#define DMEM_QUEUE_H

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#ifndef INLINE
#   define INLINE static inline
#endif

#define d_Queue(name)                   dq_##name##_t
#define dq_init(name, pq)               dq_init_##name(pq)
#define dq_free(name, pq)               dq_free_##name(pq)
#define dq_clear(name, pq)              dq_clear_##name(pq)
#define dq_linearize(name, pq)          dq_linearize_##name(pq)
#define dq_push_back(name, pq, num)     dq_push_back_##name(pq, num)
#define dq_pop_back(name, pq, num)      dq_pop_back_##name(pq, num)
#define dq_pop_front(name, pq, num)     dq_pop_front_##name(pq, num)

#define dq_a(q, i)  ((q)->data[ ((q)->begin + i >= (q)->alloc)  \
                                ? (q)->begin + i - (q)->alloc   \
                                : (q)->begin + i                \
                              ])                                \

#define dq_size(q)  ( ((q)->end < (q)->begin)                   \
                    ? ((q)->end + (q)->alloc - (q)->begin)      \
                    : ((q)->end - (q)->begin))


#define DQUEUE_INIT(name, type)                                             \
    typedef struct {                                                        \
        type*   data;                                                       \
        size_t  alloc;                                                      \
        size_t  begin;                                                      \
        size_t  end;                                                        \
    } dq_##name##_t;                                                        \
    INLINE void dq_init_##name(dq_##name##_t* q)                            \
    {                                                                       \
        memset(q, 0, sizeof(dq_##name##_t));                                \
    }                                                                       \
    INLINE void dq_free_##name(dq_##name##_t* q)                            \
    {                                                                       \
        if (q) {                                                            \
            free(q->data);                                                  \
        }                                                                   \
    }                                                                       \
    INLINE void dq_clear_##name(dq_##name##_t* q)                           \
    {                                                                       \
        q->begin = 0;                                                       \
        q->end   = 0;                                                       \
    }                                                                       \
    INLINE void dq_resize_##name(dq_##name##_t* q, size_t sz)               \
    {                                                                       \
        size_t oldalloc = q->alloc;                                         \
        q->alloc = ((q->alloc) + 16) * 2;                                   \
        if (q->alloc <= sz)                                                 \
            q->alloc = sz + 1;                                              \
                                                                            \
        q->data = (type*) realloc(q->data, sizeof(type) * q->alloc);        \
        if (q->end < q->begin) {                                            \
            /* Resizing |bbb------aaa| to |bbb-------aaa-------|            \
             * Thus we need to move a to  |bbb--------------aaa|            \
             */                                                             \
            size_t asz  = oldalloc - q->begin;                              \
            type* asrc  = q->data + q->begin;                               \
            type* adest = q->data + q->alloc - asz;                         \
            memcpy(adest, asrc, asz * sizeof(type));                        \
            q->begin = q->alloc - asz;                                      \
        }                                                                   \
    }                                                                       \
    INLINE void dq_linearize_##name(dq_##name##_t* q)                       \
    {                                                                       \
        if (dq_size(q) == 0) {                                              \
            q->begin = 0;                                                   \
            q->end = 0;                                                     \
        } else if (q->end < q->begin) {                                     \
            dq_resize_##name(q, dq_size(q) * 2);                            \
            /* Moving |bbb---------aaa| to |---aaabbb---------| */          \
            size_t asz  = q->alloc - q->begin;                              \
            type* asrc  = q->data + q->alloc - asz;                         \
            type* adest = q->data + q->end;                                 \
            size_t bsz  = q->end;                                           \
            type* bsrc  = q->data;                                          \
            type* bdest = adest + asz;                                      \
            assert(bdest + bsz < asrc);                                     \
            memcpy(adest, asrc, asz * sizeof(type));                        \
            memcpy(bdest, bsrc, bsz * sizeof(type));                        \
            q->begin = q->end;                                              \
            q->end   = q->begin + asz + bsz;                                \
        }                                                                   \
    }                                                                       \
    INLINE type* dq_push_back_##name(dq_##name##_t* q, size_t num)          \
    {                                                                       \
        /* -1 so that when the queue is full q->end != q->begin*/           \
        size_t sz = dq_size(q);                                             \
        if ((int) (sz + num) >= (int) (q->alloc) - 1) {                     \
            dq_resize_##name(q, sz + num);                                  \
        }                                                                   \
        if (dq_size(q) == 0) {                                              \
            q->begin = 0;                                                   \
            q->end = 0;                                                     \
        }                                                                   \
        q->end += num;                                                      \
        if (q->end >= q->alloc) {                                           \
            q->end -= q->alloc;                                             \
        }                                                                   \
        if (&dq_a(q, sz) > &dq_a(q, sz + num)) {                            \
            dq_linearize_##name(q);                                         \
        }                                                                   \
        return &dq_a(q, sz);                                                \
    }                                                                       \
    INLINE void dq_pop_back_##name(dq_##name##_t* q, size_t num)            \
    {                                                                       \
        assert(num <= dq_size(q));                                          \
        if (q->end < num) {                                                 \
            q->end += q->alloc;                                             \
        }                                                                   \
        q->end -= num;                                                      \
    }                                                                       \
    INLINE void dq_pop_front_##name(dq_##name##_t* q, size_t num)           \
    {                                                                       \
        assert(num <= dq_size(q));                                          \
        q->begin += num;                                                    \
        if (q->begin >= q->alloc) {                                         \
            q->begin -= q->alloc;                                           \
        }                                                                   \
    }




#endif /* DMEM_QUEUE_H */

