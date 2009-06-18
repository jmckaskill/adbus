#pragma once

namespace DBus{

  //-----------------------------------------------------------------------------

  class ObjectInterface
  {
    NON_COPYABLE(ObjectInterface);
  public:
    ObjectInterface(Object* object);
    ~ObjectInterface();

    SignalBase* addSignal(const char* name, SignalBase* signal);

    template<class Fn, class U>
    MethodBase* addMethod(const char* name, const Fn& function, U* data = (void*)NULL);

    template<class T>
    Property<T>* addProperty(const char* name);


    DBusMarshaller* returnMessage(DBusMarshaller* request);
    DBusMarshaller* signalMessage(const char* name);

    void introspect(std::string& out)const;

    Object* object()const{return m_Object;}

    void setName(const char* name){m_Name = name;}
    const std::string& name()const{return m_Name;}

    bool callMethod(Message* message);

  private:
    typedef std::set<MethodBase*>    Methods;
    typedef std::set<SignalBase*>    Signals;
    typedef std::set<PropertyBase*>  Properties;
    std::string m_Name;
    Methods     m_Methods;
    Signals     m_Signals;
    Properties  m_Properties;
    Object*     m_Object;
  };

  //-----------------------------------------------------------------------------

  class InterfaceComponent
  {
    NON_COPYABLE(InterfaceComponent);
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
    ObjectInterface* interface()const{return m_Interface;}

    void setName(const char* name){m_Name = name;}
    void setInterface(ObjectInterface* interface){m_Interface = interface;}

  private:

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

    AnnotationsMap m_Annotations;
    Arguments m_Args;

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

    virtual MethodBase* clone();

  protected:
    MethodBase(){}

  private:
    friend class ObjectInterface;
    virtual void triggered(Message* m)=0;

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
    virtual void introspect(std::string& out)const;
  };

  //-----------------------------------------------------------------------------

#define ARG(x) const A ## x&  a ## x = A ## x()

  template<DBUSCPP_DECLARE_TYPES_DEF>
  class Signal : public SignalBase
  {
  public:
    void trigger( DBUSCPP_REPEAT(ARG, DBUSCPP_NUM, COMMA) );
  };

#undef ARG

  //-----------------------------------------------------------------------------

  class PropertyBase : public InterfaceComponent
  {
  protected:
    PropertyBase():InterfaceComponent(Property){}
    virtual void get(Message* m)=0;
    virtual void set(Message* m)=0;

    void introspectProperty(std::string& out, const char* typeString)const;
  };

  //-----------------------------------------------------------------------------

  template<class T>
  class Property : public PropertyBase
  {
  public:
    Property<T>* setSetter(const boost::function<void (T)>& setter){m_Setter = setter; return this;}
    Property<T>* setGetter(const boost::function<T ()>& getter){m_Getter = getter; return this;}

  private:
    virtual void introspect(std::string& out)const;
    virtual void get(Message* m);
    virtual void set(Message* m);

    boost::function<void (T)>  m_Setter;
    boost::function<T ()>      m_Getter;
  };

  //-----------------------------------------------------------------------------

}

