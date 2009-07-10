// vim: ts=4 sw=4 sts=4 et
//
// Copyright (c) 2009 James R. McKaskill
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// ----------------------------------------------------------------------------

#pragma once

#include "ADBusLua.h"

#include "adbus/Connection.h"
#include "adbus/Marshaller.h"

#include "LuaInclude.h"


struct LADBusConnection
{
    struct ADBusConnection*    connection;
    struct ADBusMarshaller*    marshaller;
};

int LADBusCreateConnection(lua_State* L);
int LADBusFreeConnection(lua_State* L);

int LADBusSetConnectionSendCallback(lua_State* L);
int LADBusParse(lua_State* L);

int LADBusConnectToBus(lua_State* L);
int LADBusIsConnectedToBus(lua_State* L);
int LADBusUniqueServiceName(lua_State* L);

int LADBusNextSerial(lua_State* L);
int LADBusSendError(lua_State* L);
int LADBusSendReply(lua_State* L);


