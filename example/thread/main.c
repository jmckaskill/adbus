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

#include "main.h"
#include "client.h"
#include <stdio.h>

#define NEW(type) (type*) calloc(1, sizeof(type))

static MT_EventLoop* sMainLoop;
static int sPingersLeft;

static void PingerFinished();

/* -------------------------------------------------------------------------- */

void Pinger_Init(Pinger* p, adbus_Connection* c)
{
    p->connection = c;
    p->state = adbus_state_new();
    p->proxy = adbus_proxy_new(p->state);
    p->leftToReceive = 0;

    adbus_conn_ref(c);
    adbus_proxy_init(
            p->proxy,
            c, 
            "nz.co.foobar.adbus.PingServer",
            -1,
            "/",
            -1);
}

void Pinger_Destroy(Pinger* p)
{
    adbus_state_free(p->state);
    adbus_proxy_free(p->proxy);
    adbus_conn_deref(p->connection);
}

int Pinger_Run(Pinger* p)
{
    Pinger_AsyncPing(p);
    return p->leftToReceive > 0;
}

void Pinger_OnSend(Pinger* p)
{ p->leftToReceive++; }

void Pinger_OnReceive(Pinger* p)
{
    if (--p->leftToReceive == 0) {
        if (MT_Loop_Current() == sMainLoop) {
            PingerFinished();
        } else {
            MT_Loop_Exit(MT_Loop_Current(), 0);
        }
    }
}

void Pinger_AsyncPing(Pinger* p)
{
    adbus_Call f;
    adbus_proxy_method(p->proxy, &f, "Ping", -1);

    adbus_msg_appendsig(f.msg, "s", -1);
    adbus_msg_string(f.msg, "str", -1);

    f.callback  = &Pinger_AsyncReply;
    f.cuser     = p;
    f.error     = &Pinger_AsyncError;
    f.euser     = p;

    Pinger_OnSend(p);
    adbus_call_send(&f);
}

int Pinger_AsyncReply(adbus_CbData* d)
{
    /*Pinger* p = (Pinger*) d->user1; */
    const char* str = adbus_check_string(d, NULL);
    fprintf(stderr, "Reply %s\n", str);
    return 0;
}

int Pinger_AsyncError(adbus_CbData* d)
{
    /*Pinger* p = (Pinger*) d->user1; */
    fprintf(stderr, "Error %s\n", d->msg->error);
    MT_Loop_Exit(MT_Loop_Current(), 0);
    return 0;
}

/* -------------------------------------------------------------------------- */

void PingThread_Create(adbus_Connection* c)
{
    PingThread* s = NEW(PingThread);
    s->connection = c;

    adbus_conn_ref(c);

    s->thread = MT_Thread_Start(&PingThread_Run, s);
}

void PingThread_Run(void* u)
{
    PingThread* s = (PingThread*) u;
    MT_EventLoop* loop = MT_Loop_New();

    Pinger_Init(&s->pinger, s->connection);
    if (Pinger_Run(&s->pinger)) {
        MT_Loop_Run(loop);
    }

    MT_Loop_Free(loop);
    Pinger_Destroy(&s->pinger);

    s->finished.call = &PingThread_Finish;
    s->finished.user = s;

    MT_Loop_Post(sMainLoop, &s->finished);
}

void PingThread_Finish(void* u)
{
    PingThread* s = (PingThread*) u;
    adbus_conn_deref(s->connection);
    MT_Thread_Join(s->thread);
    free(s);
    PingerFinished();
}

/* -------------------------------------------------------------------------- */

static void PingerFinished()
{
    if (--sPingersLeft == 0) {
        MT_Loop_Exit(sMainLoop, 0);
    }
}

int main(void)
{
    sMainLoop = MT_Loop_New();
    adbus_Connection* c = MT_CreateDBusConnection(ADBUS_DEFAULT_BUS);

    if (c == NULL) {
        fprintf(stderr, "Failed to connect\n");
        return -1;
    }

    adbus_conn_ref(c);

    PingThread_Create(c);

    MT_Loop_Run(sMainLoop);

    adbus_conn_deref(c);
    MT_Loop_Free(sMainLoop);
    return 0;
}

