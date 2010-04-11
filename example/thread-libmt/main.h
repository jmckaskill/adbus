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

#include <libmt.h>
#include <adbus.h>


typedef struct Pinger Pinger;
typedef struct PingThread PingThread;

struct Pinger
{
    adbus_Connection*   connection;
    adbus_State*        state;
    adbus_Proxy*        proxy;
    int                 asyncPingsLeft;
    int                 leftToReceive;
};

struct PingThread
{
    MT_MainLoop*        loop;
    adbus_Connection*   connection;
    MT_Message          finished;
    MT_Thread           thread;
    Pinger              pinger;
};

void Pinger_Init(Pinger* p, adbus_Connection* c);
int  Pinger_Run(Pinger* p);
void Pinger_Destroy(Pinger* p);
void Pinger_AsyncPing(Pinger* p);
int  Pinger_AsyncReply(adbus_CbData* d);
int  Pinger_AsyncError(adbus_CbData* d);
void Pinger_OnSend(Pinger* p);
void Pinger_OnReceive(Pinger* p);


void PingThread_Create(adbus_Connection* c);
void PingThread_Run(void* u);
void PingThread_Join(MT_Message* m);
void PingThread_Free(MT_Message* m);


