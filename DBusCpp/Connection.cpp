#include "Connection.h"

using namespace DBus;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

Connection::Connection()
{
  m_Parser = new Parser;

  m_BusFactory.setConnection(this);
  m_BusFactory.setService("org.freedesktop.DBus");
  m_BusFactory.setPath("/org/freedesktop/DBus");
  m_BusFactory.setInterface("org.freedesktop.DBus");
}

//-----------------------------------------------------------------------------

Connection::~Connection()
{
  Objects::iterator oi;
  for (oi = m_Objects.begin(); oi != m_Objects.end(); ++oi)
    delete *oi;
  delete m_Parser;
}

//-----------------------------------------------------------------------------

ObjectInterface* Connection::addObject(const char* name, int size)
{
  Objects::iterator oi = m_Objects.find(StringRef(name, size));
  if (oi != m_Interfaces.end())
    return *oi;
  Object* o = new Object(this);
  o->setName(name, size);
  m_Objects.insert(o);
  return i;
}

//-----------------------------------------------------------------------------

void Connection::removeObject(const char* name, int size)
{
  Objects::iterator oi = m_Objects.find(StringRef(name, size));
  if (oi == m_Interfaces.end())
    return;

  delete *oi;
  m_Objects.erase(oi);
}

//-----------------------------------------------------------------------------

void Connection::setSendCallback(SendCallback callback, void* callbackData)
{
  m_Parser->setCallback(callback, callbackData);
}

//-----------------------------------------------------------------------------

int Connection::appendInputData(uint8_t* data, size_t size)
{
  size_t used = 0;
  int err = 0;
  if (m_InputBuffer.empty())
  {
    err = m_Parser->parse(data, size, &used);
  }
  else
  {
    m_InputBuffer.insert(m_InputBuffer.end(), data, data + size);
    err = m_Parser->parse(&m_InputBuffer[0], m_InputBuffer.size(), &used);
  }

  if (err == Success)
  {
    m_InputBuffer.clear();
    return Success;
  }
  else if (err == NeedMoreData)
  {
    m_InputBuffer.erase(m_InputBuffer.begin(), m_InputBuffer.begin() + used);
    return Success;
  }
  else
  {
    return err;
  }
}

//-----------------------------------------------------------------------------

void Connection::dispatchToRegistration(Message* message)
{
  MessageType type      = DBusGetMessageType(message);
  const char* sender    = DBusGetSender(message);
  const char* path      = DBusGetPath(message);
  const char* interface = DBusGetInterface(message);
  const char* member    = DBusGetMember(message);

  MessageRegistrations::iterator ii = m_Registrations.begin();
  while (ii != m_Registrations.end())
  {
    if (type != ii->type)
      ++ii;
    else if (sender && !ii->service.empty() && ii->service != sender)
      ++ii;
    else if (path && !ii->path.empty() && ii->path != path)
      ++ii;
    else if (interface && !ii->interface.empty() && ii->interface != interface)
      ++ii;
    else if (member && !ii->member.empty() && ii->member != member)
      ++ii;
    else
    {
      ii->slot->triggered(message);
      if (ii->type == DBusMethodReturnMessage || ii->type == DBusErrorMessage)
        ii = m_Registrations.erase(ii);
      else
        ++ii;
    }
  }
}

//-----------------------------------------------------------------------------

class InvalidPathError : public DBus::Error
{
public:
  virtual const char* errorName()const
  { return "nz.co.foobar.DBus.InvalidPath"; }
  virtual const char* errorMessage()const
  { return "Path not found"; }
};

void Connection::dispatchToObject(Message* message)
{
  try
  {
    const char* path = DBusGetPath(message);
    if (!path)
      throw InvalidPathError();

    Objects::iterator oi = m_Objects.find(path);
    if (oi == m_Objects.end() || *oi == NULL)
      throw InvalidPathError();

    *oi->callMethod(message);
  }
  catch(DBus::Error& e)
  {
    if (type != DBusMethodCallMessage)
      return;
    DBusMarshaller* m;
    DBusClearMarshaller(m);
    setupMarshaller(m);
    DBusSetMessageType(m, DBusErrorMessage);
    DBusSetErrorName(m, e.errorName());
    DBusSetReplySerial(m, DBusGetSerial(message));
    const char* msg = e.errorMessage();
    if (msg && *msg != '\0')
      DBusAppendString(m, msg);
    DBusSendMessage(m);
  }
}

//-----------------------------------------------------------------------------

void Connection::parserCallback(void* connection, Message* message)
{
  Connection* c = reinterpret_cast<Connection*>(connection);
  MessageType type = DBusGetMessageType(message);
  {
    switch(type)
    {
    case DBusMethodCallMessage:
      return c->dispatchToObject(message, m_Objects);
    case DBusMethodReturnMessage:
    case DBusErrorMessage:
    case DBusSignalMessage:
      return c->dispatchToRegistration(message, m_Registrations);
    default:
      ASSERT(false);
      return;
    }
  }
}

//-----------------------------------------------------------------------------

void Connection::connectToBus()
{
  ASSERT(!m_Connected);
  MessageFactory& f = m_BusFactory;
  f.setName("Hello");
  f.setCallback(this, &Connection::onHello);
  f.call();
}

//-----------------------------------------------------------------------------

void Connection::onHello(const char* uniqueName)
{
  m_UniqueName = uniqueName;
  m_Connected  = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

Object::Object(Connection* c)
: m_Connection(c)
{
}

//-----------------------------------------------------------------------------

Object::~Object()
{
}

//-----------------------------------------------------------------------------

void Object::setName(const char* name, int size /* = -1 */)
{
  m_Name = std::string(name, size);
}

//-----------------------------------------------------------------------------

ObjectInterface* Object::addInterface(const char* name, int size)
{
  Interfaces::iterator ii = m_Interfaces.find(StringRef(name, size));
  if (ii != m_Interfaces.end() && *ii)
    return *ii;
  ObjectInterface* i = new ObjectInterface(this);
  i->setName(name, size);
  m_Interfaces.insert(i);
  return i;
}

//-----------------------------------------------------------------------------

void Object::removeInterface(const char* name, int size)
{
  m_Interfaces.erase(StringRef(name, size));
}

//-----------------------------------------------------------------------------

void Object::introspectInterfaces(std::string& out)const
{
  Interfaces::const_iterator ii;
  for (ii = m_Interfaces.begin(); ii != m_Interfaces.end(); ++ii)
    (*ii)->introspect(out);
}

//-----------------------------------------------------------------------------

DBusMarshaller* Object::marshaller()
{
  if (!m_Marshaller)
    m_Marshaller = DBusCreateMarshaller();
  DBusClearMarshaller(m_Marshaller);
  connection()->setupMarshaller(m_Marshaller);
  return m_Marshaller;
}

//-----------------------------------------------------------------------------

class InvalidMethodError : public DBus::Error
{
public:
  virtual const char* errorName()const
  { return "nz.co.foobar.DBus.InvalidMethod"; }
  virtual const char* errorMessage()const
  { return "No method found"; }
};

void Object::callMethod(Message* message)
{
  const char* interface = DBusGetInterface(message);
  if (interface)
  {
    Interfaces::const_iterator ii = m_Interfaces.find(StringRef(interface,-1));
    if (ii == m_Interfaces.end())
      throw InvalidMethodError();

    if (!(*ii)->callMethod(message))
      throw InvalidMethodError();
  }
  else
  {
    Interfaces::const_iterator ii;
    for (ii = m_Interfaces.begin(); ii != m_Interfaces.end(); ++ii)
    {
      if ((*ii)->callMethod(message))
        return;
    }
    throw InvalidMethodError();
  }
}

//-----------------------------------------------------------------------------
