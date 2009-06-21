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
#include "Slot.h"

#include <assert.h>

namespace adbus{

  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------

  template<class Fn, class U>
  MethodBase* ObjectInterface::addMethod(const char* name, Fn function, U* data)
  {
    MethodBase* ret = createMethod(data, function);
    ret->setName(name);
    ret->setInterface(this);

    assert(FindUsingKey(m_Methods, name, -1) == m_Methods.end());

    m_Methods[ret->name()] = ret;
    return ret;
  }

  //-----------------------------------------------------------------------------

  template<class T>
  Property<T>* ObjectInterface::addProperty(const char* name)
  {
    Property<T>* ret = new Property<T>;
    ret->setName(name);
    ret->setInterface(this);

    assert(FindUsingKey(m_Properties, name, -1) == m_Properties.end());

    m_Properties[ret->name()] = ret;

    return ret;
  }

  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------

#define MARSHALL_STATEMENT(x) a ## x >> *sig;
#define MARSHALL_STATEMENTS ADBUSCPP_REPEAT(MARSHALL_STATEMENT, ADBUSCPP_BLANK)

  template<ADBUSCPP_DECLARE_TYPES>
  void Signal<ADBUSCPP_TYPES>::trigger(ADBUSCPP_CRARGS)
  {
    ADBusMarshaller* sig = interface()->signalMessage(m_Name.c_str(), m_Name.size());

    MARSHALL_STATEMENTS;

    ADBusSendMessage(sig);
  }

#undef MARSHALL_STATEMENT
#undef MARSHALL_STATEMENTS

  //-----------------------------------------------------------------------------

#define TYPE_STRING_CASE(x) case x: { return ADBusTypeString<A ## x>(); }
#define TYPE_STRING_CASES   ADBUSCPP_REPEAT(TYPE_STRING_CASE, ADBUSCPP_BLANK)

  template<ADBUSCPP_DECLARE_TYPES>
  const char* Signal<ADBUSCPP_TYPES>::argumentTypeString(int i)const
  {
    switch(i)
    {
    TYPE_STRING_CASES
    default:
      return NULL;
    }
  }

#undef TYPE_STRING_CASES
#undef TYPE_STRING_CASE

  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------

  template<class T>
  void Property<T>::introspect(std::string& out)const
  {
    introspectProperty(out, ADBusTypeString<T>());
  }

  //-----------------------------------------------------------------------------

  template<class T>
  void Property<T>::get(ADBusMessage* m)
  {
    T data = m_Getter();
    ADBusMarshaller* ret = interface()->returnMessage(m);
    data >> *ret;
    ADBusSendMessage(ret);
  }

  //-----------------------------------------------------------------------------

  template<class T>
  void Property<T>::set(ADBusMessage* m)
  {
    T data;
    data << *m;
    m_Setter(data);
    ADBusMarshaller* ret = interface()->returnMessage(m);
    ADBusSendMessage(ret);
  }

  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------

  template<class Container>
  typename Container::iterator
  FindUsingKey(Container& c, const char* str, int size)
  {
    if (size < 0)
      size = strlen(str);
    typename Container::iterator begin = c.begin();
    typename Container::iterator end   = c.end();
    while (begin != end)
    {
      const std::string& key = begin->first;
      if (key.size() == size && strncmp(key.c_str(), str, size) == 0)
        break;
      ++begin;
    }
    return begin;
  }

  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------
  //-----------------------------------------------------------------------------

}

