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

#include "misc.h"
#include "dmem/hash.h"
#include "dmem/vector.h"
#include "dmem/string.h"
#include "dmem/list.h"
#include <setjmp.h>
#include <stdint.h>



DVECTOR_INIT(char, char);

struct adbus_Connection
{
    /** \privatesection */
    volatile long               ref;

    d_Hash(ObjectPath)          paths;
    d_Hash(Remote)              remotes;

    // We keep free lists for all registrable services so that they can be
    // released in adbus_conn_free.

    d_IList(Bind)               binds;

    d_Hash(ServiceLookup)       services;

    uint32_t                    nextSerial;
    adbus_Bool                  connected;
    char*                       uniqueService;

    adbus_Callback              connectCallback;
    void*                       connectData;

    adbus_ConnectionCallbacks   callbacks;
    void*                       user;

    adbus_State*                state;
    adbus_Proxy*                bus;

    adbus_Interface*            introspectable;
    adbus_Interface*            properties;

    d_Vector(char)              parseBuffer;
    adbus_MsgFactory*           returnMessage;
};


ADBUSI_FUNC int adbusI_dispatchBind(adbus_CbData* d);



