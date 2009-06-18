namespace DBus{

  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------

  namespace detail{

    //-----------------------------------------------------------------------------

    template<unsigned int arity, class U, class Fn>
    MethodBase* CreateMethod(
        MemberFunction_tag<arity>,
        const Fn& function,
        U* data)
    {
      return new Target<arity>::MF<U, Fn>(function, data);
    }

    //-----------------------------------------------------------------------------
        
    template<unsigned int arity, class U, class Fn>
    MethodBase* CreateMethod(
        Function_tag<arity>,
        const Fn& function,
        U* data)
    {
      return new Target<arity>::F<U, Fn>(function, data);
    }

    //-----------------------------------------------------------------------------

  }

  template<class Fn, class U>
  MethodBase* ObjectInterface::addMethod<Fn, U>(const char* name, const Fn& function, U* data)
  {
    using namespace detail;
    MethodBase* ret = CreateMethod(arity(function), function, data);
    ret->setName(name);
    ret->setInterface(this);
    if (m_Methods[name])
      delete m_Methods[name];
    m_Methods[name] = ret;
    return ret;
  }

  //-----------------------------------------------------------------------------

  template<class T>
  Property<T>* ObjectInterface::addProperty<T>(const char* name)
  {
    Proprety<T>* ret = new Property<T>;
    ret->setName(name);
    ret->setInterface(this);
    if (m_Properties[name])
      delete m_Properties[name];
    m_Properties[name] = ret;
    return ret;
  }

  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------

#define ARG(x) const A ## x&  a ## x
#define MARSHALL(x) *sig << a ## x;

  template<DBUSCPP_DECLARE_TYPES>
  void Signal<DBUSCPP_TYPES>::trigger( DBUSCPP_REPEAT(ARG, DBUSCPP_NUM, COMMA) )
  {
    Marshaller* sig = m_Object->signalMessage(m_Name.c_str());

    DBUSCPP_REPEAT(MARSHALL, DBUSCPP_NUM, BLANK)

    sendMessage(sig); 
  }

#undef MARSHALL
#undef ARG

  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------

  template<class T>
  void Property<T>::introspect(std::string& out)
  {
    introspectProperty(out, DBusTypeString<T>());
  }

  //-----------------------------------------------------------------------------

  template<class T>
  void Property<T>::get(Message* m)
  {
    T data = m_Getter();
    Marshaller* ret = m_Object->returnMessage(m);
    *ret << data;
    sendMessage(ret);
  }

  //-----------------------------------------------------------------------------

  template<class T>
  void Property<T>::set(Message* m)
  {
    T data;
    *m >> data;
    m_Setter(data);
    Marshaller* ret = m_Object->returnMessage(m);
    sendMessage(ret);
  }

  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------

}
