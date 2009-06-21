// vim: ts=2 sw=2 sts=2 et
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

#include "MessageFactory.h"
#include "MessageFactory.inl"

#include "Connection.h"

#include "DBusClient/Marshaller.h"

#include <assert.h>

using namespace DBus;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

MessageFactory::MessageFactory()
  : m_Connection(NULL),
    m_Serial(0)
{
  m_Marshaller = DBusCreateMarshaller();
}

//-----------------------------------------------------------------------------

MessageFactory::~MessageFactory()
{
  DBusFreeMarshaller(m_Marshaller);
  delete m_Registration.slot;
  delete m_Registration.errorSlot;
}

//-----------------------------------------------------------------------------

void MessageFactory::reset()
{
  m_Connection = NULL;
  m_Service.clear();
  m_Path.clear();
  m_Interface.clear();
  m_Member.clear();
}

//-----------------------------------------------------------------------------

void MessageFactory::setConnection(Connection* connection)
{
  m_Connection = connection;
}

//-----------------------------------------------------------------------------

void MessageFactory::setService(const char* service, int size)
{
  if (size < 0)
    size = strlen(service);
  m_Service.clear();
  m_Service.insert(m_Service.end(), service, service + size);
}

//-----------------------------------------------------------------------------

void MessageFactory::setPath(const char* path, int size)
{
  if (size < 0)
    size = strlen(path);
  m_Path.clear();
  m_Path.insert(m_Path.end(), path, path + size);
}

//-----------------------------------------------------------------------------

void MessageFactory::setInterface(const char* interface, int size)
{
  if (size < 0)
    size = strlen(interface);
  m_Interface.clear();
  m_Interface.insert(m_Interface.end(), interface, interface + size);
}

//-----------------------------------------------------------------------------

void MessageFactory::setMember(const char* member, int size)
{
  if (size < 0)
    size = strlen(member);
  m_Member.clear();
  m_Member.insert(m_Member.end(), member, member + size);
}

//-----------------------------------------------------------------------------

uint32_t MessageFactory::connectSignal()
{
  if (!m_Connection || m_Interface.empty() || m_Member.empty() || !m_Registration.slot)
  {
    assert(false);
    return 0;
  }

  m_Registration.type = DBusSignalMessage;
  m_Registration.service = m_Service;
  m_Registration.path = m_Path;
  m_Registration.interface = m_Interface;
  m_Registration.member = m_Member;

  return m_Connection->addRegistration(&m_Registration);
}

//-----------------------------------------------------------------------------

int MessageFactory::setupMarshallerForCall(int flags)
{
  if (!m_Connection || m_Service.empty() || m_Path.empty() || m_Member.empty())
  {
    assert(false);
    return 1;
  }

  bool noReply = flags & DBusNoReplyExpectedFlag;
  m_Serial = 0;

  if ((m_Registration.slot || m_Registration.errorSlot) && !noReply)
  {
    m_Registration.type       = DBusMethodReturnMessage;
    m_Registration.service    = m_Service;
    m_Registration.path       = m_Path;
    m_Registration.interface  = m_Interface;
    m_Registration.member     = m_Member;

    m_Serial = m_Connection->addRegistration(&m_Registration);
  }


  m_Connection->setupMarshaller(m_Marshaller, m_Serial, flags);

  DBusSetMessageType(m_Marshaller, DBusMethodCallMessage);
  DBusSetPath(m_Marshaller, m_Path.c_str(), (int)m_Path.size());
  DBusSetDestination(m_Marshaller, m_Service.c_str(), (int)m_Service.size());
  if (!m_Interface.empty())
    DBusSetInterface(m_Marshaller, m_Interface.c_str(), (int)m_Interface.size());
  DBusSetMember(m_Marshaller, m_Member.c_str(), (int)m_Member.size());

  return 0;
}

//-----------------------------------------------------------------------------
