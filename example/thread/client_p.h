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

#ifdef _WIN32
#	include <winsock2.h>
#endif

#include "client.h"


#define NEW(type) (type*) calloc(1, sizeof(type))

typedef struct MTI_Client MTI_Client;
typedef struct MTI_ClientMessage MTI_ClientMessage;
typedef struct MTI_ProxyMessage MTI_ProxyMessage;

struct MTI_Client
{
#ifdef _WIN32
    WSAEVENT            handle;
#endif

    adbus_Socket        sock;
    adbus_Connection*   connection;
    MT_AtomicInt        connected;
    adbus_Buffer*       txbuf;
    MT_MainLoop*        loop;
};


struct MTI_ClientMessage
{
    MT_Header           header;
    MT_Message          msgHeader;
    adbus_Connection*   connection;
	adbus_Buffer*		msgBuffer;
    adbus_Message       msg;
    adbus_MsgFactory*   ret;
    void*               user1;
    void*               user2;
    adbus_Bool          hasReturn;
    adbus_MsgCallback   cb;
};

struct MTI_ProxyMessage
{
    MT_Header           header;
    MT_Message          msgHeader;
    adbus_Callback      callback;
    adbus_Callback      release;
    adbus_Bool          releaseCalled;
    void*               user;
};

MT_Header* MTI_ClientMessage_New(void);
void MTI_ClientMessage_Free(MT_Header* h);

MT_Header* MTI_ProxyMessage_New(void);
void MTI_ProxyMessage_Free(MT_Header* h);

int     MTI_Client_SendFlush(MTI_Client* s, size_t req);
void    MTI_Client_OnIdle(void* u);
int     MTI_Client_SendMsg(void* u, adbus_Message* m);
int     MTI_Client_Send(void* u, const char* buf, size_t sz);
int     MTI_Client_Recv(void* u, char* buf, size_t sz);
uint8_t MTI_Client_Rand(void* u);
void    MTI_Client_Disconnect(MTI_Client* s);
int     MTI_Client_DispatchExisting(MTI_Client* s);
void    MTI_Client_OnReceive(void* u);
int     MTI_Client_Block(void* u, adbus_BlockType type, uintptr_t* block, int timeoutms);
void    MTI_Client_Connected(void* u);
void    MTI_Client_ConnectionProxy(void* u, adbus_Callback cb, adbus_Callback release, void* cbuser);
void    MTI_Client_GetProxy(void* u, adbus_ProxyMsgCallback* msgcb, void** msguser, adbus_ProxyCallback* cb, void** cbuser);
void    MTI_Client_Free(void* u);;

void    MTI_Client_Proxy(void* u, adbus_Callback cb, adbus_Callback release, void* cbuser);
int     MTI_Client_MsgProxy(void* u, adbus_MsgCallback msgcb, adbus_CbData* d);
