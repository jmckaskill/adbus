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

#include "MessageFactory.h"

#include "Connection.h"

#include "adbus/Marshaller.h"

#include <assert.h>

using namespace adbus;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

MessageFactory::MessageFactory()
:   m_Connection(NULL)
{
    m_Message = ADBusCreateMessage();
}

//-----------------------------------------------------------------------------

MessageFactory::~MessageFactory()
{
    ADBusFreeMessage(m_Message);
}

//-----------------------------------------------------------------------------

void MessageFactory::reset()
{
    m_Connection = NULL;
    m_Destination.clear();
    m_Path.clear();
    m_Interface.clear();
    m_Member.clear();
    m_Match = Match();
    m_Flags = 0;
}

//-----------------------------------------------------------------------------

void MessageFactory::setupMatch(enum ADBusMessageType type)
{
    assert(m_Connection);
    if (m_Match.replySerial == 0xFFFFFFFF)
        m_Match.replySerial = m_Connection->nextSerial();

    m_Match.type = type;
}

//-----------------------------------------------------------------------------

void MessageFactory::setupMessage()
{
    ADBusResetMessage(m_Message);
    ADBusSetMessageType(m_Message, ADBusMethodCallMessage);
    ADBusSetFlags(m_Message, m_Flags);
    ADBusSetSerial(m_Message, m_Match.replySerial);

    if (!m_Destination.empty())
        ADBusSetDestination(m_Message, m_Destination.c_str(), m_Destination.size());
    if (!m_Interface.empty())
        ADBusSetInterface(m_Message, m_Interface.c_str(), m_Interface.size());
    if (!m_Path.empty())
        ADBusSetPath(m_Message, m_Path.c_str(), m_Path.size());
    if (!m_Member.empty())
        ADBusSetMember(m_Message, m_Member.c_str(), m_Member.size());
}

//-----------------------------------------------------------------------------

void MessageFactory::sendMessage()
{
    m_Connection->sendMessage(m_Message);
}



