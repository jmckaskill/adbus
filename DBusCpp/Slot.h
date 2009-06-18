#pragma once

#include "Arity.h"

namespace DBus
{

  //-----------------------------------------------------------------------------

  class Slot
  {
  public:
    virtual ~Slot(){}
    virtual Slot* clone()=0;

    virtual void triggered(DBusMessage* message)=0;
  };

  //-----------------------------------------------------------------------------

  class CallbackObject
  {
  public:
    CallbackObject();
    ~CallbackObject();

    void connect(Slot* slot);
    void disconnect(Slot* slot);

  private:
    std::vector<Slot*> m_Slots;
  };

  //-----------------------------------------------------------------------------

  namespace detail
  {

    template<class U, class Fn>
    class SlotBase : public DBus::Slot
    {
    public:
      SlotBase(U* object, Fn function)
        : m_Object(object),
          m_Function(function)
      {
        ((CallbackObject*)m_Object)->connect(this);
      }
      ~SlotBase()
      {
        ((CallbackObject*)m_Object)->disconnect(this);
      }
    protected:
      U* m_Object;
      Fn m_Function;
    };

  }

  //-----------------------------------------------------------------------------

}


#define NUM 0
#include "Slot_t.h"
#undef NUM

#define NUM 1
#include "Slot_t.h"
#undef NUM

#define NUM 2
#include "Slot_t.h"
#undef NUM

#define NUM 3
#include "Slot_t.h"
#undef NUM

#define NUM 4
#include "Slot_t.h"
#undef NUM

#define NUM 5
#include "Slot_t.h"
#undef NUM

#define NUM 6
#include "Slot_t.h"
#undef NUM

#define NUM 7
#include "Slot_t.h"
#undef NUM

#define NUM 8
#include "Slot_t.h"
#undef NUM

#define NUM 9
#include "Slot_t.h"
#undef NUM


namespace DBus
{
  //-----------------------------------------------------------------------------

  namespace detail
  {
    template<class U, class Fn, unsigned int Arity, class R>
    static void createSlot(U* object, Fn function, MemberFunctionTag<Arity,R>)
    { return new Target<Arity>::Return<R>::MF(object, function); }

    template<class U, class Fn, unsigned int Arity, class R>
    static void createSlot(U* object, Fn function, FunctionTag<Arity,R>)
    { return new Target<Arity>::Return<R>::F(object, function); }
  }

  //-----------------------------------------------------------------------------

  template<class U, class Fn>
  Slot* createSlot(U* object, Fn function)
  { return detail::createSlot(object, function, detail::arity(function)); }

  //-----------------------------------------------------------------------------

}
