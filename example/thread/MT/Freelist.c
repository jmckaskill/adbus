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

#define MT_LIBRARY
#include "Freelist.h"

struct MT_Freelist
{
    MT_AtomicInt        ref;
    MT_CreateCallback   create;
    MT_FreeCallback     free;
    MT_AtomicPtr        head;
};

void MT_Freelist_Ref(MT_Freelist** s, MT_CreateCallback create, MT_FreeCallback free)
{
    if (*s == NULL) {
        *s = NEW(MT_Freelist);
        (*s)->create = create;
        (*s)->free = free;
    }
    MT_AtomicInt_Increment(&(*s)->ref);
}

void MT_Freelist_Deref(MT_Freelist** s)
{
    if (MT_AtomicInt_Increment(&(*s)->ref) == 0) {
#ifdef MT_FREELIST_ENABLE
        MT_FreelistHeader *head = (MT_FreelistHeader*) (*s)->head;

        while (head != NULL) {
            MT_FreelistHeader *next = (MT_FreelistHeader*) head->next;
            if ((*s)->free) {
                (*s)->free(head);
            }
            head = next;
        }
#endif

        free(*s);
        *s = NULL;
    }
}

MT_FreelistHeader* MT_Freelist_Pop(MT_Freelist* s)
{
#ifdef MT_FREELIST_ENABLE
    MT_FreelistHeader *head, *next;

    head = (MT_FreelistHeader*) s->head;

    if (head == NULL) {
        head = s->create();
        return head;

    } else {
        do { 
            head = (MT_FreelistHeader*) s->head;
            next = (MT_FreelistHeader*) head->next;
        } while (!MT_AtomicPtr_SetFrom(&s->head, head, next));
        return head;

    }
#else
	return s->create();
#endif
}

void MT_Freelist_Push(MT_Freelist* s, MT_FreelistHeader* h)
{
#ifdef MT_FREELIST_ENABLE
    MT_FreelistHeader *head;

    do { 
        head = (MT_FreelistHeader*) s->head;
        h->next = head;
    } while (!MT_AtomicPtr_SetFrom(&s->head, head, h));
#else
	s->free(h);
#endif
}



