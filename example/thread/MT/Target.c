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

#pragma once

void MT_Target_Init(MT_Target* t)
{ 
	t->loop = MT_Current();
}

void MT_Target_InitToLoop(MT_Target* t, MT_EventLoop* loop)
{ 
	t->loop = loop; 
}

void MT_Target_Destroy(MT_Target* t)
{
    MT_Message* m = t->first;
    for (m = t->first; m != NULL; m = (MT_Message*) m->targetNext) {
        /* Disable the calling of this message */
        m->target = NULL;
        m->call = NULL;
    }
}

void MT_Message_Post(MT_Message* m, MT_Target* t)
{
    MT_Message* prevlast;

	m->target = t;
    /* Grab last pointer */
    prevlast = MT_AtomicPtr_Set(&t->last, t);
    /* Release to target thread */
    MT_AtomicPtr_Set(&prevlast->targetNext, m);

    MT_Loop_Post(t->loop, m);
}
