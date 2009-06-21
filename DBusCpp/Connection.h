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

#pragma once

#include "Macros.h"
#include "MessageFactory.h"

#include "DBusClient/Marshaller.h"
#include "DBusClient/Message.h"
#include "DBusClient/Parser.h"

#include <boost/function.hpp>

#include <map>
#include <set>
#include <string>
#include <vector>

struct DBusMarshaller;
struct DBusMessage;

namespace DBus{

  class InterfaceComponent;
  class MethodBase;
  class Object;
  class ObjectInterface;
  class PropertyBase;
  class SignalBase;
  class Slot;
  template<class T> class Property;

  //-----------------------------------------------------------------------------

  class Error
  {
  public:
    virtual const char* errorName()const=0;
    virtual const char* errorMessage()const=0;
  };

  //-----------------------------------------------------------------------------

  template<class Container> 
  typename Container::iterator
  FindUsingKey(Container& c, const char* str, int size);
  
  //-----------------------------------------------------------------------------

  class Connection
  {
    DBUSCPP_NON_COPYABLE(Connection);
  public:
    Connection();
    ~Connection();

    void setSendCallback(DBusSendCallback callback, void* callbackData);
    int  appendInputData(uint8_t* data, size_t size);

    void connectToBus();

    void addService(const char* name, int size = -1);
    void removeService(const char* name, int size = -1);
    const std::string& uniqueName()const{return m_UniqueName;}
    bool isConnected()const{return m_Connected;}

    void setupMarshaller(DBusMarshaller* marshaller, uint32_t serial = 0, int flags = 0);
    DBusMarshaller* returnMessage(DBusMessage* request);

    uint32_t addRegistration(MessageRegistration* registration);

    Object* addObject(const char* name, int size = -1);
    void removeObject(const char* name, int size = -1);

  private:
    friend class RegistrationHandle;
    typedef std::map<uint32_t, MessageRegistration> Registrations;
    typedef std::map<std::string, Object*>   Objects;

    static void parserCallback(void* connection, DBusMessage* message);
    void dispatchSignal(DBusMessage* message);
    void dispatchMethodCall(DBusMessage* message);
    void dispatchMethodReturn(DBusMessage* message);
    void onHello(const char* uniqueName);

    DBusSendCallback      m_Callback;
    void*                 m_CallbackData;
    DBusParser*           m_Parser;
    std::vector<uint8_t>  m_InputBuffer;
    Registrations         m_Returns;
    Registrations         m_Signals;
    MessageFactory        m_BusFactory;
    Objects               m_Objects;
    DBusMarshaller*       m_ErrorMarshaller;
    uint32_t              m_NextSerial;
    bool                  m_Connected;
    std::string           m_UniqueName;
  };

  //-----------------------------------------------------------------------------

  class Object
  {
    DBUSCPP_NON_COPYABLE(Object);
  public:
    ObjectInterface* addInterface(const char* name, int size = -1);
    void removeInterface(const char* name, int size = -1);

    void introspectInterfaces(std::string& out)const;

    const std::string& name()const{return m_Name;}

    Connection* connection()const{return m_Connection;}

  private:
    friend class Connection;
    friend class ObjectInterface;

    Object(Connection* connection);
    ~Object();

    void callMethod(DBusMessage* message);
    void setName(const char* name, int size = -1);

    DBusMarshaller* marshaller();

    typedef std::map<std::string, ObjectInterface*> Interfaces;

    std::string m_Name;
    Interfaces m_Interfaces;
    Connection* m_Connection;
    DBusMarshaller* m_Marshaller;
  };

  //-----------------------------------------------------------------------------

  class ObjectInterface
  {
    DBUSCPP_NON_COPYABLE(ObjectInterface);
  public:
    ObjectInterface(Object* object);
    ~ObjectInterface();

    SignalBase* addSignal(const char* name, SignalBase* signal);

    template<class Fn, class U>
    MethodBase* addMethod(const char* name, Fn function, U* data);

    template<class T>
    Property<T>* addProperty(const char* name);


    DBusMarshaller* returnMessage(DBusMessage* request);
    DBusMarshaller* signalMessage(const char* name, int size);

    void introspect(std::string& out)const;

    Object* object()const{return m_Object;}

    void setName(const char* name, int size = -1);
    const std::string& name()const{return m_Name;}

    bool callMethod(DBusMessage* message);

  private:
    typedef std::map<std::string, MethodBase*>    Methods;
    typedef std::map<std::string, SignalBase*>    Signals;
    typedef std::map<std::string, PropertyBase*>  Properties;
    std::string m_Name;
    Methods     m_Methods;
    Signals     m_Signals;
    Properties  m_Properties;
    Object*     m_Object;
  };

  //-----------------------------------------------------------------------------

  class InterfaceComponent
  {
    DBUSCPP_NON_COPYABLE(InterfaceComponent);
  public:
    InterfaceComponent* addAnnotation(const char* key, const char* value);

    virtual void introspect(std::string& out)const=0;

  protected:
    InterfaceComponent();
    virtual ~InterfaceComponent(){}

    enum Direction
    {
      In,
      Out,
    };

    void addArgument(const char* name, const char* type, Direction dir);

    void introspectArguments(std::string& out)const;
    void introspectAnnotations(std::string& out)const;

    const std::string& name()const{return m_Name;}
    ObjectInterface*   interface()const{return m_Interface;}

    void setName(const char* name, int size = -1);
    void setInterface(ObjectInterface* interface){m_Interface = interface;}

  private:
    friend class ObjectInterface;

    struct Argument
    {
      std::string name;
      std::string type;
      Direction   direction;
      bool operator<(const Argument& other)const
      { return name < other.name; }
    };

    typedef std::map<std::string, std::string>  Annotations;
    typedef std::set<Argument>                  Arguments;

    Annotations m_Annotations;
    Arguments   m_Arguments;

    std::string m_Name;
    ObjectInterface* m_Interface;
  };

  //-----------------------------------------------------------------------------

  class MethodBase : public InterfaceComponent
  {
  public:
    MethodBase* addArgument(const char* name, const char* type)
    { InterfaceComponent::addArgument(name, type, In); return this; }

    MethodBase* addReturn(const char* name, const char* type)
    { InterfaceComponent::addArgument(name, type, Out); return this; }

  protected:
    MethodBase(){}

  private:
    friend class ObjectInterface;
    virtual void triggered(DBusMessage* m)=0;

    virtual void introspect(std::string& out)const;
  };

  //-----------------------------------------------------------------------------

  class SignalBase : public InterfaceComponent
  {
  public:
    SignalBase* addArgument(const char* name, const char* type)
    { InterfaceComponent::addArgument(name, type, Out); return this; }

  protected:
    SignalBase(){}

  private:
    friend class ObjectInterface;
    virtual void introspect(std::string& out)const;
  };

  //-----------------------------------------------------------------------------

  template<DBUSCPP_DECLARE_TYPES_DEF>
  class Signal : public SignalBase
  {
  public:
    void trigger(DBUSCPP_CRARGS_DEF);
  };

  //-----------------------------------------------------------------------------

  class PropertyBase : public InterfaceComponent
  {
  protected:
    friend class ObjectInterface;
    PropertyBase(){}
    virtual void get(DBusMessage* m)=0;
    virtual void set(DBusMessage* m)=0;

    void introspectProperty(std::string& out, const char* typeString)const;
  };

  //-----------------------------------------------------------------------------

  template<class T>
  class Property : public PropertyBase
  {
  public:
    Property<T>* setSetter(boost::function1<void, T> setter){m_Setter = setter;}
    Property<T>* setGetter(boost::function0<T> getter){m_Getter = getter;}

  private:
    friend class ObjectInterface;
    virtual void introspect(std::string& out)const;
    virtual void get(DBusMessage* m);
    virtual void set(DBusMessage* m);

    boost::function1<void, T> m_Setter;
    boost::function0<T>       m_Getter;
  };

  //-----------------------------------------------------------------------------

}
