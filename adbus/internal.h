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

#ifndef __STDC_LIMIT_MACROS
#   define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_CONSTANT_MACROS
#   define __STDC_CONSTANT_MACROS
#endif

#if defined _MSC_VER && !defined _CRT_SECURE_NO_DEPRECATE
#   define _CRT_SECURE_NO_DEPRECATE
#endif

#define ADBUS_LIBRARY

#include <adbus.h>
#include "dmem/hash.h"
#include "dmem/list.h"
#include "dmem/string.h"
#include "dmem/vector.h"

#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && defined(__ELF__)
#   define ADBUSI_FUNC __attribute__((visibility("hidden"))) extern
#   define ADBUSI_DATA __attribute__((visibility("hidden"))) extern
#else
#   define ADBUSI_FUNC
#   define ADBUSI_DATA extern
#endif

typedef struct adbusI_TrackedRemote adbusI_TrackedRemote;
typedef struct adbusI_RemoteTracker adbusI_RemoteTracker;
typedef struct adbusI_BusServer adbusI_BusServer;
typedef struct adbusI_ServerParser adbusI_ServerParser;
typedef struct adbusI_RemoteSet adbusI_RemoteSet;
typedef struct adbusI_ServiceOwner adbusI_ServiceOwner;
typedef struct adbusI_ServiceQueue adbusI_ServiceQueue;
typedef struct adbusI_ServiceQueueSet adbusI_ServiceQueueSet;
typedef struct adbusI_ServerMatch adbusI_ServerMatch;
typedef struct adbusI_ServerMatchList adbusI_ServerMatchList;
typedef struct adbusI_Header adbusI_Header;
typedef struct adbusI_ExtendedHeader adbusI_ExtendedHeader;
typedef struct adbusI_ConnMatchList adbusI_ConnMatchList;
typedef struct adbusI_ReplySet adbusI_ReplySet;
typedef struct adbusI_ObjectTree adbusI_ObjectTree;
typedef struct adbusI_ObjectNode adbusI_ObjectNode;
typedef struct adbusI_ConnBusData adbusI_ConnBusData;
typedef struct adbusI_StateData adbusI_StateData;
typedef struct adbusI_StateConn adbusI_StateConn;
typedef struct adbusI_ConnMsg adbusI_ConnMsg;

#include "debug.h"
#include "misc.h"

