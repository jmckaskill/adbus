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

#include "Connection.h"
#include "Connection.inl"

#include "MessageFactory.inl"

#include <assert.h>

using namespace DBus;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

Connection::Connection()
  : m_Callback(NULL),
    m_CallbackData(NULL),
    m_ErrorMarshaller(NULL),
    m_NextSerial(1),
    m_Connected(false)
{
  m_Parser = DBusCreateParser();
  DBusSetParserCallback(m_Parser, &Connection::parserCallback, (void*)this);

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
    delete oi->second;
  DBusFreeParser(m_Parser);
}

//-----------------------------------------------------------------------------

Object* Connection::addObject(const char* name, int size)
{
  Objects::iterator oi = FindUsingKey(m_Objects, name, size);

  if (oi != m_Objects.end())
    return oi->second;
  Object* o = new Object(this);
  o->setName(name, size);
  m_Objects[o->name()] = o;
  return o;
}

//-----------------------------------------------------------------------------

void Connection::removeObject(const char* name, int size)
{
  Objects::iterator oi = FindUsingKey(m_Objects, name, size);

  if (oi == m_Objects.end())
    return;

  delete oi->second;
  m_Objects.erase(oi);
}

//-----------------------------------------------------------------------------

void Connection::setSendCallback(DBusSendCallback callback, void* callbackData)
{
  m_Callback = callback;
  m_CallbackData = callbackData;
}

//-----------------------------------------------------------------------------

int Connection::appendInputData(uint8_t* data, size_t size)
{
  size_t used = 0;
  int err = 0;

  m_InputBuffer.insert(m_InputBuffer.end(), data, data + size);

  while (!m_InputBuffer.empty())
  {
    err = DBusParse(m_Parser, &m_InputBuffer[0], m_InputBuffer.size(), &used);
    if (err)
      break;
    m_InputBuffer.erase(m_InputBuffer.begin(), m_InputBuffer.begin() + used);
  }

  if (err == DBusNeedMoreData)
    err = DBusSuccess;

  return err;
}

//-----------------------------------------------------------------------------

void Connection::dispatchSignal(DBusMessage* message)
{
  DBusMessageType type  = DBusGetMessageType(message);
  const char* sender    = DBusGetSender(message, NULL);
  const char* path      = DBusGetPath(message, NULL);
  const char* interface = DBusGetInterface(message, NULL);
  const char* member    = DBusGetMember(message, NULL);

  Registrations::iterator ii;
  for (ii = m_Signals.begin(); ii != m_Signals.end(); ++ii)
  {
    if (ii->second.type != type)
      continue;
    else if (sender && !ii->second.service.empty() && ii->second.service != sender)
      continue;
    else if (path && !ii->second.path.empty() && ii->second.path != path)
      continue;
    else if (interface && !ii->second.interface.empty() && ii->second.interface != interface)
      continue;
    else if (member && !ii->second.member.empty() && ii->second.member != member)
      continue;
    else
    {
      ii->second.slot->triggered(message);
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

void Connection::dispatchMethodCall(DBusMessage* message)
{
  try
  {
    int pathLen;
    const char* path = DBusGetPath(message, &pathLen);
    if (!path)
      throw InvalidPathError();

    Objects::iterator oi = FindUsingKey(m_Objects, path, pathLen);

    if (oi == m_Objects.end() || oi->second == NULL)
      throw InvalidPathError();

    oi->second->callMethod(message);
  }
  catch(DBus::Error& e)
  {
    DBusMarshaller* m = m_ErrorMarshaller;
    DBusClearMarshaller(m);
    setupMarshaller(m);
    DBusSetMessageType(m, DBusErrorMessage);
    DBusSetErrorName(m, e.errorName(), -1);
    DBusSetReplySerial(m, DBusGetSerial(message));
    const char* msg = e.errorMessage();
    if (msg && *msg != '\0')
      DBusAppendString(m, msg, -1);
    DBusSendMessage(m);
  }
}

//-----------------------------------------------------------------------------

void Connection::dispatchMethodReturn(DBusMessage* message)
{
  uint32_t serial = DBusGetReplySerial(message);
  Registrations::iterator ii = m_Returns.find(serial);

  // MethodReturn and Error messages can not be responded to, so any errors we
  // should just ignore
  if (ii == m_Returns.end() || ii->second.type != DBusMethodReturnMessage)
    return;

  DBusMessageType type = DBusGetMessageType(message);
  if (type == DBusMethodReturnMessage && ii->second.slot)
  {
    ii->second.slot->triggered(message);
  }
  else if (type == DBusErrorMessage && ii->second.errorSlot)
  {
    ii->second.errorSlot->triggered(message);
  }

  delete ii->second.slot;
  delete ii->second.errorSlot;

  m_Returns.erase(ii);
}

//-----------------------------------------------------------------------------

void Connection::parserCallback(void* connection, DBusMessage* message)
{
  Connection* c = reinterpret_cast<Connection*>(connection);
  DBusMessageType type = DBusGetMessageType(message);
  {
    switch(type)
    {
    case DBusMethodCallMessage:
      return c->dispatchMethodCall(message);
    case DBusMethodReturnMessage:
    case DBusErrorMessage:
      return c->dispatchMethodReturn(message);
    case DBusSignalMessage:
      return c->dispatchSignal(message);
    default:
      assert(false);
      return;
    }
  }
}

//-----------------------------------------------------------------------------

void Connection::connectToBus()
{
  assert(!m_Connected);
  MessageFactory& f = m_BusFactory;
  f.setMember("Hello");
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

void Connection::setupMarshaller(DBusMarshaller* marshaller, uint32_t serial, int flags)
{
  DBusClearMarshaller(marshaller);
  DBusSetSendCallback(marshaller, m_Callback, m_CallbackData);
  if (serial == 0)
    serial = m_NextSerial++;
  DBusSetSerial(marshaller, serial);
  DBusSetFlags(marshaller, flags);
}

//-----------------------------------------------------------------------------

uint32_t Connection::addRegistration(MessageRegistration* registration)
{
  MessageRegistration copy = *registration;
  if (copy.slot)
    copy.slot = copy.slot->clone();
  if (copy.errorSlot)
    copy.slot = copy.errorSlot->clone();
  uint32_t serial = m_NextSerial++;
  if (copy.type == DBusMethodReturnMessage)
    m_Returns[serial] = copy;
  else if (copy.type == DBusSignalMessage)
    m_Signals[serial] = copy;
  else
    assert(false);
  return serial;
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
  if (size < 0)
    size = strlen(name);
  m_Name.clear();
  m_Name.insert(m_Name.end(), name, name + size);
}

//-----------------------------------------------------------------------------

ObjectInterface* Object::addInterface(const char* name, int size)
{
  Interfaces::iterator ii = FindUsingKey(m_Interfaces, name, size);

  if (ii != m_Interfaces.end() && ii->second)
    return ii->second;

  ObjectInterface* i = new ObjectInterface(this);
  i->setName(name, size);
  m_Interfaces[i->name()] = i;
  return i;
}

//-----------------------------------------------------------------------------

void Object::removeInterface(const char* name, int size)
{
  Interfaces::iterator ii = FindUsingKey(m_Interfaces, name, size);

  if (ii != m_Interfaces.end())
    m_Interfaces.erase(ii);
}

//-----------------------------------------------------------------------------

void Object::introspectInterfaces(std::string& out)const
{
  Interfaces::const_iterator ii;
  for (ii = m_Interfaces.begin(); ii != m_Interfaces.end(); ++ii)
    ii->second->introspect(out);
}

//-----------------------------------------------------------------------------

DBusMarshaller* Object::marshaller()
{
  if (!m_Marshaller)
    m_Marshaller = DBusCreateMarshaller();
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

void Object::callMethod(DBusMessage* message)
{
  int interfaceSize;
  const char* interface = DBusGetInterface(message, &interfaceSize);
  if (interface)
  {
    Interfaces::iterator ii = FindUsingKey(m_Interfaces, interface, interfaceSize);

    if (ii == m_Interfaces.end())
      throw InvalidMethodError();

    if (!ii->second->callMethod(message))
      throw InvalidMethodError();
  }
  else
  {
    Interfaces::const_iterator ii;
    for (ii = m_Interfaces.begin(); ii != m_Interfaces.end(); ++ii)
    {
      if (ii->second->callMethod(message))
        return;
    }
    throw InvalidMethodError();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

ObjectInterface::ObjectInterface(Object* object)
{
  m_Object = object;
}

//-----------------------------------------------------------------------------

ObjectInterface::~ObjectInterface()
{
  // Signals are not owned by us
  Methods::iterator mi;
  for (mi = m_Methods.begin(); mi != m_Methods.end(); ++mi)
    delete mi->second;
  Properties::iterator pi;
  for (pi = m_Properties.begin(); pi != m_Properties.end(); ++pi)
    delete pi->second;
}

//-----------------------------------------------------------------------------

SignalBase* ObjectInterface::addSignal(const char* name, SignalBase* signal)
{
  signal->setName(name);
  signal->setInterface(this);
  m_Signals[signal->name()] = signal;
  return signal;
}

//-----------------------------------------------------------------------------

DBusMarshaller* ObjectInterface::signalMessage(const char* name, int size)
{
  DBusMarshaller* m = m_Object->marshaller();
  DBusSetPath(m, m_Object->name().c_str(), (int)m_Object->name().size());
  DBusSetInterface(m, m_Name.c_str(), (int)m_Name.size());
  DBusSetMember(m, name, size);
  return m;
}

//-----------------------------------------------------------------------------

DBusMarshaller* ObjectInterface::returnMessage(DBusMessage* request)
{
  DBusMarshaller* m = m_Object->marshaller();
  DBusSetReplySerial(m, DBusGetSerial(request));
  return m;
}

//-----------------------------------------------------------------------------

void ObjectInterface::introspect(std::string& out)const
{
  out += "<interface name=\""
       + name()
       + "\">\n";

  Methods::const_iterator mi;
  for (mi = m_Methods.begin(); mi != m_Methods.end(); ++mi)
    mi->second->introspect(out);

  Properties::const_iterator pi;
  for (pi = m_Properties.begin(); pi != m_Properties.end(); ++pi)
    pi->second->introspect(out);

  Signals::const_iterator si;
  for (si = m_Signals.begin(); si != m_Signals.end(); ++si)
    si->second->introspect(out);

  out += "</interface>\n";
}

//-----------------------------------------------------------------------------

void ObjectInterface::setName(const char* name, int size)
{
  if (size < 0)
    size = strlen(name);
  m_Name.clear();
  m_Name.insert(m_Name.end(), name, name + size);
}

//-----------------------------------------------------------------------------

bool ObjectInterface::callMethod(DBusMessage* message)
{
  int memberSize;
  const char* member = DBusGetMember(message, &memberSize);
  Methods::iterator ii = FindUsingKey(m_Methods, member, memberSize);
  if (ii == m_Methods.end())
    return false;

  ii->second->triggered(message);

  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

InterfaceComponent::InterfaceComponent()
{
  m_Interface = NULL;
}

//-----------------------------------------------------------------------------

void InterfaceComponent::addArgument(const char* name, const char* type, Direction dir)
{
  Argument arg;
  arg.name = name;
  arg.type = type;
  arg.direction = dir;
  m_Arguments.insert(arg);
}

//-----------------------------------------------------------------------------

InterfaceComponent* InterfaceComponent::addAnnotation(const char* key, const char* value)
{
  m_Annotations[key] = value;
  return this;
}

//-----------------------------------------------------------------------------

void InterfaceComponent::introspectAnnotations(std::string& out)const
{
  Annotations::const_iterator ii;
  for (ii = m_Annotations.begin(); ii != m_Annotations.end(); ++ii)
  {
    out += "<annotation name=\""
         + ii->first
         + "\" value=\""
         + ii->second
         + "\"/>\n";
  }
}

//-----------------------------------------------------------------------------

void InterfaceComponent::introspectArguments(std::string& out)const
{
  Arguments::const_iterator ii;
  for (ii = m_Arguments.begin(); ii != m_Arguments.end(); ++ii)
  {
    out += "<argument name=\""
         + ii->name
         + "\" type=\""
         + ii->type
         + "\" direction=\""
         + (ii->direction == In ? "in" : "out")
         + "\"/>\n";
  }
}

//-----------------------------------------------------------------------------

void InterfaceComponent::setName(const char* name, int size)
{
  if (size < 0)
    size = strlen(name);
  m_Name.clear();
  m_Name.insert(m_Name.end(), name, name + size);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void MethodBase::introspect(std::string& out)const
{
  out += "<method name=\""
       + name()
       + "\"\n";

  introspectArguments(out);
  introspectAnnotations(out);

  out += "</method>\n";
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void SignalBase::introspect(std::string& out)const
{
  out += "<signal name=\""
       + name()
       + "\"\n";

  introspectArguments(out);
  introspectAnnotations(out);

  out += "</signal>\n";
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void PropertyBase::introspectProperty(std::string& out, const char* typeString)const
{
  out += "<property name\""
       + name()
       + "\" type=\""
       + typeString
       + ">\n";

  introspectAnnotations(out);

  out += "</property>\n";
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

