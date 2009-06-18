#include "Interface.h"

#pragma once

using namespace DBus;

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
    delete *mi;
  Properties::iterator pi;
  for (pi = m_Properties.begin(); pi != m_Properties.end(); ++pi)
    delete *pi;
}

//-----------------------------------------------------------------------------

SignalBase* ObjectInterface::addSignal(const char* name, SignalBase* signal)
{
  m_Signals[signal];
  signal->setName(name);
  signal->setInterface(this);
  return signal;
}

//-----------------------------------------------------------------------------

DBusMarshaller* ObjectInterface::signalMessage(const std::string& name)
{
  DBusMarshaller* m = m_Object->marshaller();
  DBusSetPath(m, m_Object->name().c_str(), m_Object->name().size());
  DBusSetInterface(m, m_Name.c_str(), m_Name.size());
  DBusSetMember(m, name.c_str(), name.size());
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
    (*mi)->introspect(out);

  Properties::const_iterator pi;
  for (pi = m_Properties.begin(); pi != m_Properties.end(); ++pi)
    (*pi)->introspect(out);

  Signals::const_iterator si;
  for (si = m_Signals.begin(); si != m_Signals.end(); ++si)
    (*si)->introspect(out);

  out += "</interface>\n";
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
  for (ii = m_Arguments.begin(); ii != m_Arguments.end(); ++i)
  {
    out += "<argument name=\""
         + ii->name
         + "\" value=\""
         + ii->second
         + "\" direction=\""
         + ii->direction == In ? "in" : "out"
         + "\"/>\n";
  }
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
