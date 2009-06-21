// ----------------------------------------------------------------------------
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
// ----------------------------------------------------------------------------
//

#pragma once

#include "Marshall.h"
#include "Slot.h"

#include "adbus/Common.h"

namespace adbus{

  //-----------------------------------------------------------------------------

  template<class U, class Fn>
  void MessageFactory::setCallback(U* object, Fn function)
  {
    delete this->m_Registration.slot;
    this->m_Registration.slot = createSlot(object, function);
  }

  //-----------------------------------------------------------------------------

  template<class U, class Fn>
  void MessageFactory::setErrorCallback(U* object, Fn function)
  {
    delete this->m_Registration.errorSlot;
    this->m_Registration.errorSlot = createSlot(object, function);
  }

  //-----------------------------------------------------------------------------

#define MARSHALL_STATEMENT(x) \
  BeginArgument<A ## x>(m_Marshaller);  \
  a ## x >> *m_Marshaller;  \
  EndArgument<A ## x>(m_Marshaller);

#define MARSHALL_STATEMENTS ADBUSCPP_REPEAT(MARSHALL_STATEMENT, ADBUSCPP_BLANK)

  template<ADBUSCPP_DECLARE_TYPES>
  uint32_t MessageFactory::callWithFlags(int flags, ADBUSCPP_CRARGS)
  {
    if (setupMarshallerForCall(flags))
      return 0;

    MARSHALL_STATEMENTS;

    ADBusSendMessage(m_Marshaller);
    return m_Serial;
  }

#undef MARSHALL_STATEMENT
#undef MARSHALL_STATEMENTS

  //-----------------------------------------------------------------------------

}
