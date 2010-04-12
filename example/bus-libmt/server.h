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

#include <adbus.h>
#include <dmem/list.h>
#include <libmt.h>

typedef struct Remote Remote;
typedef struct Server Server;

DILIST_INIT(Remote, Remote);

struct Server
{
    MT_LoopRegistration*    reg;
    adbus_Socket            sock;
    adbus_Server*           bus;
    d_IList(Remote)         remotes;
};

Server* Server_New(adbus_Socket sock);
void    Server_Free(Server* s);
void    Server_OnConnect(void* u);


struct Remote
{
    d_IList(Remote)         hl;
    MT_LoopRegistration*    reg;
    MT_LoopRegistration*    idle;
    adbus_Socket            sock;
    adbus_Auth*             auth;
    adbus_Server*           bus;
    adbus_Remote*           remote;
    adbus_Buffer*           txbuf;
    adbus_Buffer*           rxbuf;
    adbus_Bool              txfull;
    adbus_Bool              readySendEnabled;
};

Remote* Remote_New(adbus_Socket sock, adbus_Server* s);
void    Remote_Free(Remote* r);
void    Remote_FlushTxBuffer(Remote* r);
int     Remote_SendMsg(void* u, adbus_Message* m);
int     Remote_Send(void* u, const char* data, size_t sz);
uint8_t Remote_Rand(void* u);
void    Remote_OnIdle(void* u);
void    Remote_ReadySend(void* u);
void    Remote_ReadyRecv(void* u);
void    Remote_OnClose(void* u);


