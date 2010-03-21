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
#include "dmem/list.h"
#include "dmem/queue.h"
#include "dmem/string.h"
#include "dmem/vector.h"

#include <adbus.h>

struct Match;
struct MatchArgument;
struct Service;
struct ServiceOwner;


DLIST_INIT(Remote, adbus_Remote);
DLIST_INIT(Match, struct Match);

DVECTOR_INIT(char, char);

DVECTOR_INIT(Argument, adbus_Argument);



/* -------------------------------------------------------------------------- */

#define ARGS_MAX 64
struct Match
{
    d_List(Match)           hl;

    adbus_MessageType       type;
    uint32_t                reply;
    adbus_Bool              checkReply;
    const char*             path;
    size_t                  pathSize;
    const char*             interface;
    size_t                  interfaceSize;
    const char*             member;
    size_t                  memberSize;
    const char*             error;
    size_t                  errorSize;
    const char*             destination;
    size_t                  destinationSize;
    const char*             sender;
    size_t                  senderSize;

    size_t                  argumentsSize;
    adbus_Argument*         arguments;

    size_t                  size;
    char                    data[1];
};

/* -------------------------------------------------------------------------- */

struct ServiceOwner
{
    adbus_Remote*           remote;
    adbus_Bool              allowReplacement;
    adbus_Bool              reserved;
};

DVECTOR_INIT(ServiceOwner, struct ServiceOwner);

struct Service
{
    // The owner is the head [0] of the queue
    d_Vector(ServiceOwner)  queue;
    char*                   name;
};

DHASH_MAP_INIT_STR(Service, struct Service*);
DVECTOR_INIT(Service, struct Service*);

/* -------------------------------------------------------------------------- */

enum ParseState
{
    BEGIN,
    HEADER,
    DATA,
};

struct adbus_Remote
{
    d_List(Remote)          hl;

    adbus_Server*           server;
    d_String                unique;
    d_List(Match)           matches;

    adbus_SendMsgCallback   send;
    void*                   data;

    enum ParseState         parseState;
    adbus_Buffer*           msg;
    adbus_Buffer*           dispatch;
    adbus_Bool              native;
    size_t                  headerSize;
    size_t                  msgSize;
    size_t                  parsedMsgSize;

    // The first message on connecting a new remote needs to be a method call
    // to "Hello" on the bus, if not we kick the connection
    adbus_Bool              haveHello;

    d_Vector(Service)       services;
};

DHASH_MAP_INIT_STR(Remote, adbus_Remote*);

/* -------------------------------------------------------------------------- */

struct adbus_Server
{
    adbus_Connection*       busConnection;
    adbus_Interface*        busInterface;
    adbus_Signal*           nameOwnerChanged;
    adbus_Signal*           nameLost;
    adbus_Signal*           nameAcquired;
    adbus_Remote*           busRemote;
    adbus_Remote*           helloRemote;

    d_Hash(Service)         services;
    d_List(Remote)          remotes;

    unsigned int            nextRemote;
};

/* -------------------------------------------------------------------------- */

adbus_Remote* adbusI_serv_remote(adbus_Server* s, const char* name);
struct Match* adbusI_serv_newmatch(const char* mstr, size_t len);
void adbusI_serv_freematch(struct Match* m);
int  adbusI_serv_requestname(adbus_Server* s, adbus_Remote* r, const char* name, uint32_t flags);
int  adbusI_serv_releasename(adbus_Server* s, adbus_Remote* r, const char* name);
void adbusI_serv_freeservice(struct Service* s);
int  adbusI_serv_dispatch(adbus_Server* s, adbus_Message* m);
void adbusI_serv_initbus(adbus_Server* s);
void adbusI_serv_freebus(adbus_Server* s);
void adbusI_serv_ownerchanged(adbus_Server* s, const char* name, adbus_Remote* o, adbus_Remote* n);
void adbusI_serv_removeServiceFromRemote(adbus_Remote* r, struct Service* serv);


