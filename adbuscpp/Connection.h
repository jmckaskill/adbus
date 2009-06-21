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
#include "Marshall.h"
#include "MessageFactory.h"

#include "adbus/Marshaller.h"
#include "adbus/Message.h"
#include "adbus/Parser.h"

#include <boost/function.hpp>

#include <map>
#include <set>
#include <string>
#include <vector>

struct ADBusMarshaller;
struct ADBusMessage;

namespace adbus{

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
    ADBUSCPP_NON_COPYABLE(Connection);
  public:
    Connection();
    ~Connection();

    void setSendCallback(ADBusSendCallback callback, void* callbackData);
    int  appendInputData(uint8_t* data, size_t size);

    void connectToBus();

    void addService(const char* name, int size = -1);
    void removeService(const char* name, int size = -1);
    const std::string& uniqueName()const{return m_UniqueName;}
    bool isConnected()const{return m_Connected;}

    void setupMarshaller(ADBusMarshaller* marshaller, uint32_t serial = 0, int flags = 0);
    ADBusMarshaller* returnMessage(ADBusMessage* request);

    uint32_t addRegistration(MessageRegistration* registration);

    Object* addObject(const char* name, int size = -1);
    void removeObject(const char* name, int size = -1);

    std::string introspectObject(const std::string& objectName)const;

  private:
    friend class RegistrationHandle;
    typedef std::map<uint32_t, MessageRegistration> Registrations;
    typedef std::map<std::string, Object*>   Objects;

    static void parserCallback(void* connection, ADBusMessage* message);
    void dispatchSignal(ADBusMessage* message);
    void dispatchMethodCall(ADBusMessage* message);
    void dispatchMethodReturn(ADBusMessage* message);
    void onHello(const char* uniqueName);

    ADBusSendCallback      m_Callback;
    void*                 m_CallbackData;
    ADBusParser*           m_Parser;
    std::vector<uint8_t>  m_InputBuffer;
    Registrations         m_Returns;
    Registrations         m_Signals;
    MessageFactory        m_BusFactory;
    Objects               m_Objects;
    ADBusMarshaller*       m_ErrorMarshaller;
    uint32_t              m_NextSerial;
    bool                  m_Connected;
    std::string           m_UniqueName;
  };

  //-----------------------------------------------------------------------------

  class Object
  {
    ADBUSCPP_NON_COPYABLE(Object);
  public:
    ObjectInterface* addInterface(const char* name, int size = -1);
    void removeInterface(const char* name, int size = -1);

    std::string introspect()const;
    void introspectInterfaces(std::string& out)const;

    const std::string& name()const{return m_Name;}

    Connection* connection()const{return m_Connection;}

  private:
    friend class Connection;
    friend class ObjectInterface;

    Object(Connection* connection);
    ~Object();

    void callMethod(ADBusMessage* message);
    void setName(const char* name, int size = -1);

    ADBusMarshaller* marshaller();

    typedef std::map<std::string, ObjectInterface*> Interfaces;

    std::string m_Name;
    Interfaces m_Interfaces;
    Connection* m_Connection;
    ADBusMarshaller* m_Marshaller;
  };

  //-----------------------------------------------------------------------------

  class ObjectInterface
  {
    ADBUSCPP_NON_COPYABLE(ObjectInterface);
  public:
    ObjectInterface(Object* object);
    ~ObjectInterface();

    SignalBase* addSignal(const char* name, SignalBase* signal);

    template<class Fn, class U>
    MethodBase* addMethod(const char* name, Fn function, U* data);

    template<class T>
    Property<T>* addProperty(const char* name);


    ADBusMarshaller* returnMessage(ADBusMessage* request);
    ADBusMarshaller* signalMessage(const char* name, int size);

    void introspect(std::string& out)const;

    Object* object()const{return m_Object;}

    void setName(const char* name, int size = -1);
    const std::string& name()const{return m_Name;}

    bool callMethod(ADBusMessage* message);

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
    ADBUSCPP_NON_COPYABLE(InterfaceComponent);
  public:
    InterfaceComponent* addAnnotation(const char* key, const char* value);

    virtual void introspect(std::string& out)const=0;

  protected:
    InterfaceComponent();
    virtual ~InterfaceComponent(){}

    void introspectArguments(std::string& out)const;
    void introspectAnnotations(std::string& out)const;

    const std::string& name()const{return m_Name;}
    ObjectInterface*   interface()const{return m_Interface;}

    void setName(const char* name, int size = -1);
    void setInterface(ObjectInterface* interface){m_Interface = interface;}

    typedef std::vector<std::pair<std::string, std::string> >  Arguments;
    Arguments   m_InArguments;
    Arguments   m_OutArguments;

  private:
    friend class ObjectInterface;

    typedef std::map<std::string, std::string>  Annotations;

    Annotations m_Annotations;

    std::string m_Name;
    ObjectInterface* m_Interface;
  };

  //-----------------------------------------------------------------------------

  class MethodBase : public InterfaceComponent
  {
  public:
    MethodBase* addArgument(const char* name, const char* type);

    MethodBase* addReturn(const char* name, const char* type);

  protected:
    MethodBase(){}

  private:
    friend class ObjectInterface;
    virtual const char* argumentTypeString(int i)const=0;
    virtual void triggered(ADBusMessage* m)=0;

    virtual void introspect(std::string& out)const;
  };

  //-----------------------------------------------------------------------------

  class SignalBase : public InterfaceComponent
  {
  public:
    SignalBase* addArgument(const char* name, const char* type);

  protected:
    SignalBase(){}

  private:
    friend class ObjectInterface;
    virtual void introspect(std::string& out)const;
    virtual const char* argumentTypeString(int i)const=0;
  };

  //-----------------------------------------------------------------------------

  template<ADBUSCPP_DECLARE_TYPES_DEF>
  class Signal : public SignalBase
  {
  public:
    void trigger(ADBUSCPP_CRARGS_DEF);
  private:
    virtual const char* argumentTypeString(int i)const;
  };

  //-----------------------------------------------------------------------------

  class PropertyBase : public InterfaceComponent
  {
  protected:
    friend class ObjectInterface;
    PropertyBase(){}
    virtual void get(ADBusMessage* m)=0;
    virtual void set(ADBusMessage* m)=0;

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
    virtual void get(ADBusMessage* m);
    virtual void set(ADBusMessage* m);

    boost::function1<void, T> m_Setter;
    boost::function0<T>       m_Getter;
  };

  //-----------------------------------------------------------------------------

}
