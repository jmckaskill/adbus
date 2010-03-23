/* vim: set et sts=4 ts=4 sw=4: */

#ifndef DMEM_LIST_H
#define DMEM_LIST_H

#include "common.h"

#include <stdlib.h>
#include <stddef.h>

#define d_List(name)                            dl_##name##_t
#define dl_init(name, head)                     dl_init_##name(head)
#define dl_clear(name, head)                    dl_init_##name(head)
#define dl_remove(name, v, h)                   dl_remove_##name(v, h)
#define dl_insert_before(name, head, v, h)      dl_insert_before_##name(head, v, h)
#define dl_insert_after(name, head, v, h)       dl_insert_after_##name(head, v, h)
#define dl_isempty(head)                        ((head)->next == NULL)

#define DL_FOREACH(name, i, head, field)                                \
    for (i = (head)->next; i != NULL; i = i->field.next)

#define DLIST_INIT(name, type)                                          \
    typedef struct {                                                    \
        type* next;                                                     \
        type* prev;                                                     \
    } dl_##name##_t;                                                    \
    DMEM_INLINE void dl_init_##name(dl_##name##_t* head)                \
    {                                                                   \
        head->next = head->prev = NULL;                                 \
    }                                                                   \
    DMEM_INLINE type* dl_type_##name(dl_##name##_t* h, ptrdiff_t off)   \
    {                                                                   \
        char* ctype = ((char*) h) - off;                                \
        return (type*) ctype;                                           \
    }                                                                   \
    DMEM_INLINE dl_##name##_t* dl_handle_##name(type* v, ptrdiff_t off) \
    {                                                                   \
        char* chandle = ((char*) v) + off;                              \
        return (dl_##name##_t*) chandle;                                \
    }                                                                   \
    DMEM_INLINE void dl_remove_##name(type* v, dl_##name##_t* h)        \
    {                                                                   \
        ptrdiff_t off = ((char*) h) - ((char*) v);                      \
        if (h->next)                                                    \
            dl_handle_##name(h->next, off)->prev = h->prev;             \
        if (h->prev)                                                    \
            dl_handle_##name(h->prev, off)->next = h->next;             \
        h->next = NULL;                                                 \
        h->prev = NULL;                                                 \
    }                                                                   \
    DMEM_INLINE void dl_insert_before_##name(                           \
            dl_##name##_t*  head,                                       \
            type*           v,                                          \
            dl_##name##_t*  h)                                          \
    {                                                                   \
        ptrdiff_t off = ((char*) h) - ((char*) v);                      \
        h->prev = head->prev;                                           \
        h->next = dl_type_##name(head, off);                            \
        head->prev = v;                                                 \
        if (h->prev)                                                    \
            dl_handle_##name(h->prev, off)->next = v;                   \
    }                                                                   \
    DMEM_INLINE void dl_insert_after_##name(                            \
            dl_##name##_t*  head,                                       \
            type*           v,                                          \
            dl_##name##_t*  h)                                          \
    {                                                                   \
        ptrdiff_t off = ((char*) h) - ((char*) v);                      \
        h->prev = dl_type_##name(head, off);                            \
        h->next = head->next;                                           \
        head->next = v;                                                 \
        if (h->next)                                                    \
            dl_handle_##name(h->next, off)->prev = v;                   \
    }

/* This is a doubly linked list that holds onto the next value when iterating.
 * This is so that you can remove items from the list whilst iterating over
 * the list. However the iteration is thus non-reentrant.
 * To iterate over the list:
 *
 * struct SomeType;
 * DILIST_INIT(SomeName, struct SomeType)
 * struct SomeType
 * {
 *      d_IList(SomeName)   lh;
 * };
 *
 * d_IList(SomeName) list;
 * dil_insert_after(SomeName, &list, calloc(sizeof(struct SomeType)));
 *
 * SomeType* i;
 * DIL_FOREACH(SomeName, i, &list, lh) {
 *    dowork(i);
 *    dil_remove(SomeName, i);
 * }
 */

#define d_IList(name)                           dil_##name##_t
#define dil_init(name, head)                    dil_init_##name(head)
#define dil_clear(name, head)                   dil_init_##name(head)
#define dil_remove(name, v, h)                  dil_remove_##name(v, h)
#define dil_insert_before(name, head, v, h)     dil_insert_before_##name(head, v, h)
#define dil_insert_after(name, head, v, h)      dil_insert_after_##name(head, v, h)
#define dil_isempty(head)                       ((head)->next == NULL)
#define dil_setiter(head, v)                    ((head)->iter = v)
#define dil_getiter(head)                       ((head)->iter)

#define DIL_FOREACH(name, i, head, field)                               \
    for ((head)->iter = NULL, i = (head)->next;                         \
         (head)->iter = (i ? i->field.next : NULL), i != NULL;          \
         i = (head)->iter)

#define DILIST_INIT(name, type)                                         \
    typedef struct {                                                    \
        type* next;                                                     \
        type* prev;                                                     \
        type* iter;                                                     \
    } dil_##name##_t;                                                   \
    DMEM_INLINE void dil_init_##name(dil_##name##_t* head)              \
    {                                                                   \
        head->next = head->prev = head->iter = NULL;                    \
    }                                                                   \
    DMEM_INLINE type* dil_type_##name(dil_##name##_t* h, ptrdiff_t off) \
    {                                                                   \
        char* ctype = ((char*) h) - off;                                \
        return (type*) ctype;                                           \
    }                                                                   \
    DMEM_INLINE dil_##name##_t* dil_handle_##name(type* v, ptrdiff_t off) \
    {                                                                   \
        char* chandle = ((char*) v) + off;                              \
        return (dil_##name##_t*) chandle;                               \
    }                                                                   \
    DMEM_INLINE void dil_clear_##name(dil_##name##_t* head)             \
    {                                                                   \
        head->next = head->prev = head->iter = NULL;                    \
    }                                                                   \
    DMEM_INLINE void dil_remove_##name(type* v, dil_##name##_t* h)      \
    {                                                                   \
        ptrdiff_t off = ((char*) h) - ((char*) v);                      \
        type** iter = (type**) h->iter;                                 \
        if (iter && *iter == v)                                         \
            *iter = NULL;                                               \
        if (h->next)                                                    \
            dil_handle_##name(h->next, off)->prev = h->prev;            \
        if (h->prev)                                                    \
            dil_handle_##name(h->prev, off)->next = h->next;            \
        h->next = NULL;                                                 \
        h->prev = NULL;                                                 \
        h->iter = NULL;                                                 \
    }                                                                   \
    DMEM_INLINE void dil_insert_before_##name(                          \
            dil_##name##_t*  head,                                      \
            type*           v,                                          \
            dil_##name##_t*  h)                                         \
    {                                                                   \
        ptrdiff_t off = ((char*) h) - ((char*) v);                      \
        h->prev = head->prev;                                           \
        h->next = dil_type_##name(head, off);                           \
        h->iter = (type*) &head->iter;                                  \
        head->prev = v;                                                 \
        if (h->prev)                                                    \
            dil_handle_##name(h->prev, off)->next = v;                  \
    }                                                                   \
    DMEM_INLINE void dil_insert_after_##name(                           \
            dil_##name##_t*  head,                                      \
            type*           v,                                          \
            dil_##name##_t*  h)                                         \
    {                                                                   \
        ptrdiff_t off = ((char*) h) - ((char*) v);                      \
        h->prev = dil_type_##name(head, off);                           \
        h->next = head->next;                                           \
        h->iter = (type*) &head->iter;                                  \
        head->next = v;                                                 \
        if (h->next)                                                    \
            dil_handle_##name(h->next, off)->prev = v;                  \
    }

#endif /* DMEM_LIST_H */

