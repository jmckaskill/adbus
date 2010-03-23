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

#include "internal.h"

/* The match lookup is simply a linked list of matches which we run through
 * checking the message against each match. Every message runs through this
 * list.
 */

DILIST_INIT(Match, adbus_ConnMatch);

struct adbus_ConnMatch
{
    d_IList(Match)          hl;
    adbus_Match             m;
    adbus_State*            state;
    adbus_Proxy*            proxy;
    adbusI_TrackedRemote*   sender;
};

struct adbusI_ConnMatchList
{
    d_IList(Match)          matches;
};

ADBUSI_FUNC void adbusI_freeMatch(adbus_ConnMatch* m);
ADBUSI_FUNC int adbusI_dispatchMatch(adbusI_MatchSet* s, adbus_CbData* d);
