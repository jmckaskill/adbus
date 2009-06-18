namespace DBus{

  //-----------------------------------------------------------------------------
  
  namespace detail
  {

  }

  template<class U, class Fn>
  void MessageFactory::setCallback<U,Fn>(U* object, Fn function)
  {
    delete m_Registration.slot;
    m_Registration.slot = Slot::create(object, function);
  }

  //-----------------------------------------------------------------------------

  template<class U, class Fn>
  void MessageFactory::setErrorCallback<U,Fn>(U* object, Fn function)
  {
    delete m_ErrorRegistration.slot;
    m_ErrorRegistration.slot = Slot::create(object, function);
  }

  //-----------------------------------------------------------------------------

#define MARSHALL(x) *m_Marshaller << a ## x;

  template<DBUSCPP_DECLARE_TYPES>
  uint32_t MessageFactory::call<DBUSCPP_TYPES>(DBUSCPP_CRARGS)
  {
    if (setupMarshallerForCall())
      return 0;

    REPEAT(MARSHALL, DBUSCPP_NUM, DBUSCPP_BLANK)

    DBusSendMessage(m_Marshaller);
    return m_Serial;
  }

  template<DBUSCPP_DECLARE_TYPES>
  uint32_t MessageFactory::callNoReply<DBUSCPP_TYPES>(DBUSCPP_CRARGS)
  {
    if (setupMarshallerForCall(DBusReplyExpectedFlag))
      return 0;

    REPEAT(MARSHALL, DBUSCPP_NUM, DBUSCPP_BLANK)

    DBusSendMessage(m_Marshaller);
    return 0;
  }
#undef MARSHALL

  //-----------------------------------------------------------------------------

}
