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

#include "Connection.h"


using namespace adbus;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

Connection::Connection()
{
    m_FreeConnection = true;
    m_C = ADBusCreateConnection();
    m_Buf = ADBusCreateStreamBuffer();
    m_Message = ADBusCreateMessage();
}

// ----------------------------------------------------------------------------

Connection::Connection(ADBusConnection* connection)
{
    m_FreeConnection = false;
    m_C = connection;
}

// ----------------------------------------------------------------------------

Connection::~Connection()
{
    if (m_FreeConnection)
        ADBusFreeConnection(m_C);
    ADBusFreeStreamBuffer(m_Buf);
}

// ----------------------------------------------------------------------------

void Connection::setSendCallback(ADBusSendCallback callback, ADBusUser* data)
{
    ADBusSetSendCallback(m_C, callback, data);
}

// ----------------------------------------------------------------------------

void Connection::dispatch(struct ADBusMessage* message)
{
    ADBusDispatch(m_C, message);
}

// ----------------------------------------------------------------------------

void Connection::parse(const uint8_t* data, size_t size)
{
    while (size > 0) {
        int err = ADBusParse(m_Buf, m_Message, &data, &size);
        if (err == ADBusNeedMoreData)
            break;
        else if (err == ADBusIgnoredData)
            continue;
        else if (err)
            throw ParseError();

        ADBusDispatch(m_C, m_Message);
    }
}

// ----------------------------------------------------------------------------

void Connection::connectToBus(ADBusConnectionCallback callback, ADBusUser* user)
{
    ADBusConnectToBus(m_C, callback, user);
}

// ----------------------------------------------------------------------------

bool Connection::isConnectedToBus() const
{
    return ADBusIsConnectedToBus(m_C) != 0;
}

// ----------------------------------------------------------------------------

std::string Connection::uniqueName() const
{
    size_t size;
    const char* str = ADBusGetUniqueServiceName(m_C, &size);
    return std::string(str, str + size);
}

// ----------------------------------------------------------------------------

uint32_t Connection::nextSerial() const
{
    return ADBusNextSerial(m_C);
}

// ----------------------------------------------------------------------------

void Connection::sendMessage(ADBusMessage* message)
{
    ADBusSendMessage(m_C, message);
}


