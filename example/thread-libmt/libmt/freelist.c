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

#include "freelist.h"

/* ------------------------------------------------------------------------- */

void MT_Freelist_Ref(MT_Freelist** s, MT_CreateCallback create, MT_FreeCallback free)
{
    if (*s == NULL) {
        *s = NEW(MT_Freelist);
        (*s)->create = create;
        (*s)->free = free;
    }
    MT_AtomicInt_Increment(&(*s)->ref);

#ifdef MT_FREELIST_THREAD
    MT_ThreadStorage_Ref(&(*s)->tls);
#endif
}

/* ------------------------------------------------------------------------- */

void MT_Freelist_Deref(MT_Freelist** s)
{
    if (MT_AtomicInt_Decrement(&(*s)->ref) == 0) {

#if defined MT_FREELIST_GLOBAL || defined MT_FREELIST_THREAD
        MT_Header *head, *next;

        head = (*s)->list;
        while (head != NULL) {
            next = head->next;
            if ((*s)->free) {
                (*s)->free(head);
            }
            head = next;
        }
#endif

#ifdef MT_FREELIST_THREAD
        MT_ThreadStorage_Deref(&(*s)->tls);
#endif

        free(*s);
        *s = NULL;
    }
}

/* ------------------------------------------------------------------------- */

MT_Header* MT_Freelist_Pop(MT_Freelist* s)
{
#if defined MT_FREELIST_GLOBAL
    MT_Header *head, *next;

    while ((head = s->list) != NULL) {
        /* Keep on trying to set the head pointer from head to head->next
         * until it succeeds.
         */
        next = head->next;
        if (MT_AtomicPtr_SetFrom(&s->list, head, next) == head) {
            LOG("Pop %p", head);
            return head;
        }
    }

    head = s->create();
    LOG("Pop new %p", head);
    return head;

#elif defined MT_FREELIST_THREAD
    MT_Header *head, *next;

    head = (MT_Header*) MT_ThreadStorage_Get(&s->tls);

    if (head) {
        next = head->next;
        MT_ThreadStorage_Set(&s->tls, next);
        LOG("Pop %p", head);
        return head;

    } else {
        head = s->create();
        head->next = (MT_Header*) MT_AtomicPtr_Set(&s->list, head);
        LOG("Pop new %p", head);
        return head;

    }

#else
    MT_Header* head = s->create();
    LOG("Pop new %p", head);
    return head;

#endif
}

/* ------------------------------------------------------------------------- */

void MT_Freelist_Push(MT_Freelist* s, MT_Header* h)
{
    LOG("Push %p", h);

#if defined MT_FREELIST_GLOBAL
    MT_Header *head;

    do { 
        head = s->list;
        h->next = head;
    } while (MT_AtomicPtr_SetFrom(&s->list, head, h) != head);

#elif defined MT_FREELIST_THREAD
    h->next = (MT_Header*) MT_ThreadStorage_Get(&s->tls);
    MT_ThreadStorage_Set(&s->tls, h);

#else
	s->free(h);

#endif
}



