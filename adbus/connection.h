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

#include <adbus/adbus.h>
#include "memory/khash.h"
#include "memory/kvector.h"
#include "memory/kstring.h"
#include <setjmp.h>
#include <stdint.h>


// ----------------------------------------------------------------------------

struct Bind
{
    adbus_Interface*  interface;
    adbus_User*       user2;
};

// ----------------------------------------------------------------------------

struct ObjectPath;

KVECTOR_INIT(ObjectPtr, struct ObjectPath*)
KHASH_MAP_INIT_STR(ObjectPtr, struct ObjectPath*)
KHASH_MAP_INIT_STR(Bind, struct Bind)

struct ObjectPath
{
    adbus_Path      h;
    khash_t(Bind)*              interfaces;
    kvector_t(ObjectPtr)*       children;
    struct ObjectPath*          parent;
};

// ----------------------------------------------------------------------------

struct Service
{
    uint32_t                signalMatch;
    uint32_t                methodMatch;
    char*                   serviceName;
    char*                   uniqueName;
    int                     refCount;
};

struct Match
{
    adbus_Match       m;
    struct Service*         service;
};

// ----------------------------------------------------------------------------

KVECTOR_INIT(Match, struct Match)
KHASH_MAP_INIT_STR(Service, struct Service*)

struct adbus_Connection
{
    kvector_t(Match)*           registrations;
    khash_t(ObjectPtr)*         objects;
    uint32_t                    nextSerial;
    uint32_t                    nextMatchId;
    adbus_Bool                  connected;
    char*                       uniqueService;

    adbus_ConnectCallback     connectCallback;
    adbus_User*           connectCallbackData;

    adbus_SendCallback           sendCallback;
    adbus_User*           sendCallbackData;

    adbus_Callback        receiveMessageCallback;
    adbus_User*           receiveMessageData;

    adbus_Callback        sendMessageCallback;
    adbus_User*           sendMessageData;

    adbus_Message*        returnMessage;

    adbus_Proxy*          bus;

    adbus_Iterator*       dispatchIterator;

    adbus_Interface*      introspectable;
    adbus_Interface*      properties;

    khash_t(Service)*           services;

    adbus_Stream*           parseStream;
    adbus_Message*          parseMessage;
};


void adbusI_freeMatchData(struct Match* m);
void adbusI_freeObjectPath(struct ObjectPath* o);
void adbusI_freeService(struct Service* s);


