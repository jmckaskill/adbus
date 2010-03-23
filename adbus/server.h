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

struct adbus_Server
{
    adbusI_RemoteSet        remotes;
    adbusI_ServiceQueueSet  services;
    adbusI_BusServer        bus;
    adbus_Remote*           lastCaller;
};

ADBUS_API adbus_Server* adbus_serv_new(adbus_Interface* bus);
ADBUS_API void adbus_serv_free(adbus_Server* s);

// Message must have gone through the server parse functions to ensure it has
// the correct sender field before calling this.
ADBUSI_FUNC int adbusI_serv_dispatch(adbus_Server* s, adbus_Remote* from, adbus_Message* m);

