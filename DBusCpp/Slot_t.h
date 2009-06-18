namespace DBus
{
namespace detail
{

#define DEMARSHALL(x) \
    Arg ## x a ## x;  \
    *message >> a ## x;

  template<>
  struct Target<NUM>
  {

    template<class R>
    struct Return
    {

      //-----------------------------------------------------------------------------

      template<class U, class Fn, DBUSCPP_DECLARE_TYPES>
      class F : public SlotBase<U,Fn>
      {
        NON_COPYABLE(F);
      public:
        F(U* object, Fn function):SlotBase<U,Fn>(object,function){}

      private:
        virtual void triggered(DBusMessage* message)
        {
          DBUSCPP_REPEAT( DEMARSHALL, NUM, BLANK )
          DBusMessage* ret = m_Object->returnMessage(message);
          *ret << m_Function(m_Data, DBUSCPP_REPEAT(DBUSCPP_ARG, NUM, COMMA) );
          sendMessage(ret);
        }

        virtual Slot* clone()
        { return new F<U,Fn,DBUS_CPP_TYPES>(m_Data, m_Function); }
      };

      //-----------------------------------------------------------------------------

      template<class U, class Fn, DBUSCPP_DECLARE_TYPES>
      class MF : public SlotBase<U,Fn>
      {
        NON_COPYABLE(MF);
      public:
        MF(U* object, Fn function):SlotBase<U,Fn>(object,function){}

      private:
        virtual void triggered(DBusMessage* message)
        {
          DBUSCPP_REPEAT( DEMARSHALL, NUM, BLANK )
          DBusMessage* ret = m_Object->returnMessage(message);
          *ret << (m_Object->*m_Function)( DBUSCPP_REPEAT(DBUSCPP_ARG, NUM, COMMA) );
          sendMessage(ret);
        }

        virtual Slot* clone()
        { return new MF<U,Fn,DBUS_CPP_TYPES>(m_Data, m_Function); }
      }; 

      //-----------------------------------------------------------------------------

    };



    template<>
    struct Return<void>
    {

      //-----------------------------------------------------------------------------

      template<class U, class Fn, DBUSCPP_DECLARE_TYPES>
      class F : public SlotBase<U,Fn>
      {
        NON_COPYABLE(F);
      public:
        F(U* object, Fn function):SlotBase<U,Fn>(object,function){}

      private:
        virtual void triggered(DBusMessage* message)
        {
          DBUSCPP_REPEAT( DEMARSHALL, NUM, BLANK )
          m_Function(m_Data, DBUSCPP_REPEAT(DBUSCPP_ARG, NUM, COMMA) );
          DBusMessage* ret = m_Object->returnMessage(message);
          sendMessage(ret);
        }
      };

      //-----------------------------------------------------------------------------

      template<class U, class Fn, DBUSCPP_DECLARE_TYPES>
      class MF : public SlotBase<U,Fn>
      {
        NON_COPYABLE(MF);
      public:
        MF(U* object, Fn function):SlotBase<U,Fn>(object,function){}

      private:
        virtual void triggered(DBusMessage* message)
        {
          DBUSCPP_REPEAT( DEMARSHALL, NUM, BLANK )
          (m_Object->*m_Function)( DBUSCPP_REPEAT(DBUSCPP_ARG, NUM, COMMA) );
          DBusMessage* ret = m_Object->returnMessage(message);
          sendMessage(ret);
        }

        virtual Slot* clone()
        { return new MF<U,Fn,DBUS_CPP_TYPES>(m_Data, m_Function); }
      }; 

      //-----------------------------------------------------------------------------

    };

  };

#undef DEMARSHALL

}
}
