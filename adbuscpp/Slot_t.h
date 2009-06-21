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

namespace adbus
{
namespace detail
{



  template<>
  struct Target<NUM>
  {

#define ARG_LEADING_COMMA(x) , a ## x
#define ARG(x) a ## x
#define DEMARSHALL_STATEMENT(x) A ## x ARG(x); ARG(x) << *message;
#define TYPE_STRING_CASE(x) case x: { return ADBusTypeString<A ## x>(); }

#define ARGS_LEADING_COMMA          SLOT_REPEAT(ARG_LEADING_COMMA, ADBUSCPP_BLANK)
#define ARGS                        SLOT_REPEAT(ARG, ADBUSCPP_COMMA)
#define DEMARSHALL_STATEMENTS       SLOT_REPEAT(DEMARSHALL_STATEMENT, ADBUSCPP_BLANK)
#define TYPE_STRING_CASES           SLOT_REPEAT(TYPE_STRING_CASE, ADBUSCPP_BLANK)

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------

    template<class U, class Fn, ADBUSCPP_DECLARE_TYPES_DEF>
    class FSlot : public Slot
    {
      ADBUSCPP_NON_COPYABLE(FSlot);
    public:
      FSlot(U* object, Fn function):m_Object(object),m_Function(function){}

    private:
      virtual void triggered(ADBusMessage* message)
      {
        DEMARSHALL_STATEMENTS;
        m_Function(m_Object ARGS_LEADING_COMMA );
      }

      virtual Slot* clone()
      { return new FSlot<U, Fn, ADBUSCPP_TYPES>(m_Object, m_Function); }

      U* m_Object;
      Fn m_Function;
    };

    //-----------------------------------------------------------------------------

    template<class U, class Fn, ADBUSCPP_DECLARE_TYPES_DEF>
    class MFSlot : public Slot
    {
      ADBUSCPP_NON_COPYABLE(MFSlot);
    public:
      MFSlot(U* object, Fn function):m_Object(object),m_Function(function){}

    private:
      virtual void triggered(ADBusMessage* message)
      {
        DEMARSHALL_STATEMENTS;
        (m_Object->*m_Function)( ARGS );
      }

      virtual Slot* clone()
      { return new MFSlot<U, Fn, ADBUSCPP_TYPES>(m_Object, m_Function); }

      U* m_Object;
      Fn m_Function;
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------

    template<class U, class Fn, class R, ADBUSCPP_DECLARE_TYPES_DEF>
    class FMethod : public MethodBase
    {
      ADBUSCPP_NON_COPYABLE(FMethod);
    public:
      FMethod(U* object, Fn function):m_Object(object),m_Function(function){}

      virtual const char* argumentTypeString(int i)const
      {
        switch(i)
        {
        case -1:
          return ADBusTypeString<R>();
        TYPE_STRING_CASES
        default:
          return NULL;
        }
      }

    private:
      virtual void triggered(ADBusMessage* message)
      {
        DEMARSHALL_STATEMENTS;
        ADBusMarshaller* ret = interface()->returnMessage(message);
        ADBusBeginArgument(ret, ADBusTypeString<R>(), -1);
        m_Function(m_Object ARGS_LEADING_COMMA ) >> *ret;
        ADBusEndArgument(ret);
        ADBusSendMessage(ret);
      }

      U* m_Object;
      Fn m_Function;
    };

    //-----------------------------------------------------------------------------

    template<class U, class Fn, class R, ADBUSCPP_DECLARE_TYPES_DEF>
    class MFMethod : public MethodBase
    {
      ADBUSCPP_NON_COPYABLE(MFMethod);
    public:
      MFMethod(U* object, Fn function):m_Object(object),m_Function(function){}

      virtual const char* argumentTypeString(int i)const
      {
        switch(i)
        {
        case -1:
          return ADBusTypeString<R>();
        TYPE_STRING_CASES
        default:
          return NULL;
        }
      }

    private:
      virtual void triggered(ADBusMessage* message)
      {
        DEMARSHALL_STATEMENTS;
        ADBusMarshaller* ret = interface()->returnMessage(message);
        ADBusBeginArgument(ret, ADBusTypeString<R>(), -1);
        (m_Object->*m_Function)( ARGS ) >> *ret;
        ADBusEndArgument(ret);
        ADBusSendMessage(ret);
      }

      U* m_Object;
      Fn m_Function;
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------

    template<class U, class Fn, ADBUSCPP_DECLARE_TYPES_DEF>
    class FVoidMethod : public MethodBase
    {
      ADBUSCPP_NON_COPYABLE(FVoidMethod);
    public:
      FVoidMethod(U* object, Fn function):m_Object(object),m_Function(function){}

      virtual const char* argumentTypeString(int i)const
      {
        switch(i)
        {
        TYPE_STRING_CASES
        default:
          return NULL;
        }
      }

    private:
      virtual void triggered(ADBusMessage* message)
      {
        DEMARSHALL_STATEMENTS;
        m_Function(m_Object ARGS_LEADING_COMMA );
        ADBusMarshaller* ret = interface()->returnMessage(message);
        ADBusSendMessage(ret);
      }

      U* m_Object;
      Fn m_Function;
    };

    //-----------------------------------------------------------------------------

    template<class U, class Fn, ADBUSCPP_DECLARE_TYPES_DEF>
    class MFVoidMethod : public MethodBase
    {
      ADBUSCPP_NON_COPYABLE(MFVoidMethod);
    public:
      MFVoidMethod(U* object, Fn function):m_Object(object),m_Function(function){}

      virtual const char* argumentTypeString(int i)const
      {
        switch(i)
        {
        TYPE_STRING_CASES
        default:
          return NULL;
        }
      }

    private:
      virtual void triggered(ADBusMessage* message)
      {
        DEMARSHALL_STATEMENTS;
        (m_Object->*m_Function)( ARGS );
        ADBusMarshaller* ret = interface()->returnMessage(message);
        ADBusSendMessage(ret);
      }

      U* m_Object;
      Fn m_Function;
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------

  };

#undef ARG_LEADING_COMMA
#undef ARG
#undef DEMARSHALL_STATEMENT
#undef TYPE_STRING_CASE

#undef ARGS_LEADING_COMMA
#undef ARGS
#undef DEMARSHALL_STATEMENTS
#undef TYPE_STRING_CASES

}
}
