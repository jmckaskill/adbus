#include "MessageFactory.h"
#include "DBusClient/Marshaller.h"

using namespace DBus;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

uint32_t MessageFactor::connectSignal()
{
  if (!m_Connection || m_Interface.empty() || m_Member.empty() || !m_Registration.slot)
  {
    ASSERT(false);
    return 0;
  }

  m_Registration.type = DBusSignalMessage;
  m_Registration.service = m_Service;
  m_Registration.path = m_Path;
  m_Registration.interface = m_Interface;
  m_Registration.member = m_Member;

  return m_Connection->addRegistration(m_Registration);
}

//-----------------------------------------------------------------------------

uint32_t MessageFactory::setupMarshallerForCall(int flags)
{
  if (!m_Connection || m_Service.empty() || m_Path.empty() || m_Member.empty())
  {
    ASSERT(false);
    return 0;
  }

  bool noReply = flags & DBusNoReplyExpectedFlag;
  uint32_t serial = 0;

  if ((m_Registration.slot || m_Registration.errorSlot) && !noReply)
  {
    m_Registration.type       = DBusMethodCallReturnMessage;
    m_Registration.service    = m_Service;
    m_Registration.path       = m_Path;
    m_Registration.interface  = m_Interface;
    m_Registration.member     = m_Member;

    serial = m_Connection->addRegistration(m_Registration);
  }


  m_Connection->setupMarshaller(m_Marshaller, serial, flags);

  DBusSetMessageType(m_Marshaller, DBusMethodCallType);
  DBusSetService(m_Marshaller, m_Service.c_str(), m_Service.size());
  DBusSetPath(m_Marshaller, m_Path.c_str(), m_Path.size());
  DBusSetMember(m_Marshaller, m_Member.c_str(), m_Member.size());
  if (!m_Interface.empty())
    DBusSetInterface(m_Marshaller, m_Interface.c_str(), m_Interface.size());

  return serial;
}

//-----------------------------------------------------------------------------
