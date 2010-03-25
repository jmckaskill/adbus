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

/* -------------------------------------------------------------------------- */
enum
{
    AUTH_NOT_TRIED,
    AUTH_NOT_SUPPORTED,
    AUTH_BEGIN
};

typedef int (*adbusI_AuthDataCallback)(adbus_Auth* a, d_String* data);

struct adbus_Auth
{
    /** \privatesection */
    adbus_Bool              server;
    int                     cookie;
    int                     external;
    adbus_SendCallback      send;
    adbus_RandCallback      rand;
    adbus_ExternalCallback  externalcb;
    void*                   data;
    adbusI_AuthDataCallback onData;
    d_String                id;
    d_String                okCmd;
    adbus_Bool              okSent;
    adbus_Buffer*           buf;
    adbus_Bool              successful;
};


