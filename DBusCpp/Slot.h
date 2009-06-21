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

#include "Connection.h"
#include "Marshall.h"

#include "DBusClient/Marshaller.h"
#include "DBusClient/Message.h"

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
  
  namespace detail
  {
    template<int Arity>
    struct Target;
  }

}


#define NUM 0
#define SLOT_REPEAT(step,sep)
#include "Slot_t.h"
#undef SLOT_REPEAT
#undef NUM

#define NUM 1
#define SLOT_REPEAT(step,sep) step(0)
#include "Slot_t.h"
#undef SLOT_REPEAT
#undef NUM

#define NUM 2
#define SLOT_REPEAT(step,sep) step(0) sep step(1)
#include "Slot_t.h"
#undef SLOT_REPEAT
#undef NUM

#define NUM 3
#define SLOT_REPEAT(step,sep) step(0) sep step(1) sep step(2)
#include "Slot_t.h"
#undef SLOT_REPEAT
#undef NUM

#define NUM 4
#define SLOT_REPEAT(step,sep) step(0) sep step(1) sep step(2) sep step(3)
#include "Slot_t.h"
#undef SLOT_REPEAT
#undef NUM

#define NUM 5
#define SLOT_REPEAT(step,sep) step(0) sep step(1) sep step(2) sep step(3) sep step(4)
#include "Slot_t.h"
#undef SLOT_REPEAT
#undef NUM

#define NUM 6
#define SLOT_REPEAT(step,sep) step(0) sep step(1) sep step(2) sep step(3) sep step(4) sep step(5)
#include "Slot_t.h"
#undef SLOT_REPEAT
#undef NUM

#define NUM 7
#define SLOT_REPEAT(step,sep) step(0) sep step(1) sep step(2) sep step(3) sep step(4) sep step(5) sep step(6)
#include "Slot_t.h"
#undef SLOT_REPEAT
#undef NUM

#define NUM 8
#define SLOT_REPEAT(step,sep) step(0) sep step(1) sep step(2) sep step(3) sep step(4) sep step(5) sep step(6) sep step(7)
#include "Slot_t.h"
#undef SLOT_REPEAT
#undef NUM

#define NUM 9
#define SLOT_REPEAT(step,sep) step(0) sep step(1) sep step(2) sep step(3) sep step(4) sep step(5) sep step(6) sep step(7) sep step(8)
#include "Slot_t.h"
#undef SLOT_REPEAT
#undef NUM



namespace DBus
{
  //-----------------------------------------------------------------------------

  namespace detail
  {

    template<class U, class M, class R, class A0>
    Slot* createSlot(U* object, R (M::*function)(A0))
    { 
      typedef typename DBusBaseArgumentType<A0>::type BaseA0;
      return new Target<1>::MFSlot<U, R (M::*)(A0), BaseA0>(object, function); 
    }



    template<class U, class M, class R, class A0>
    MethodBase* createMethod(U* object, R (M::*function)(A0))
    { 
      typedef typename DBusBaseArgumentType<R>::type  BaseR;
      typedef typename DBusBaseArgumentType<A0>::type BaseA0;
      return new Target<1>::MFMethod<U, R (M::*)(A0), BaseR, BaseA0>(object, function); 
    }

    template<class U, class M, class A0>
    MethodBase* createMethod(U* object, void (M::*function)(A0))
    { 
      typedef typename DBusBaseArgumentType<A0>::type BaseA0;
      return new Target<1>::MFVoidMethod<U, void (M::*)(A0), BaseA0>(object, function); 
    }

  }

  //-----------------------------------------------------------------------------

  template<class U, class Fn>
  Slot* createSlot(U* object, Fn function)
  { return detail::createSlot(object, function); }

  //-----------------------------------------------------------------------------

  template<class U, class Fn>
  MethodBase* createMethod(U* object, Fn function)
  { return detail::createMethod(object, function); }

  //-----------------------------------------------------------------------------

}
