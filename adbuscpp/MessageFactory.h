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

#include <string>

#include "adbus/Common.h"

struct ADBusMarshaller;

namespace adbus{

  class Connection;
  class Slot;

  //-----------------------------------------------------------------------------

  struct MessageRegistration
  {
    MessageRegistration() :type(ADBusInvalidMessage), slot(NULL), errorSlot(NULL){}

    ADBusMessageType type;
    std::string     service;
    std::string     path;
    std::string     interface;
    std::string     member;
    Slot*           slot;
    Slot*           errorSlot;
  };

  //-----------------------------------------------------------------------------

  class MessageFactory
  {
    ADBUSCPP_NON_COPYABLE(MessageFactory);
  public:
    MessageFactory();
    ~MessageFactory();
    void reset();
    void setConnection(Connection* connection);
    void setService(const char* service, int size = -1);
    void setPath(const char* path, int size = -1);
    void setInterface(const char* interface, int size = -1);
    void setMember(const char* member, int size = -1);

    template<class U, class Fn>
    void setCallback(U* object, Fn function);

    template<class U, class Fn>
    void setErrorCallback(U* object, Fn function);

    uint32_t call()
    { return callWithFlags(0, Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t()); }

    template<class A0>
    uint32_t call(const A0& a0)
    { return callWithFlags(0, a0, Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t()); }

    template<class A0,class A1>
    uint32_t call(const A0& a0, const A1& a1)
    { return callWithFlags(0, a0, a1, Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t()); }

    template<class A0,class A1,class A2>
    uint32_t call(const A0& a0, const A1& a1, const A2& a2)
    { return callWithFlags(0, a0, a1, a2, Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t()); }

    template<class A0,class A1,class A2,class A3>
    uint32_t call(const A0& a0, const A1& a1, const A2& a2, const A3& a3)
    { return callWithFlags(0, a0, a1, a2, a3, Null_t(), Null_t(), Null_t(), Null_t(), Null_t()); }

    template<class A0,class A1,class A2,class A3,class A4>
    uint32_t call(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4)
    { return callWithFlags(0, a0, a1, a2, a3, a4, Null_t(), Null_t(), Null_t(), Null_t()); }

    template<class A0,class A1,class A2,class A3,class A4,class A5>
    uint32_t call(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5)
    { return callWithFlags(0, a0, a1, a2, a3, a4, a5, Null_t(), Null_t(), Null_t()); }

    template<class A0,class A1,class A2,class A3,class A4,class A5,class A6>
    uint32_t call(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6)
    { return callWithFlags(0, a0, a1, a2, a3, a4, a5, a6, Null_t(), Null_t()); }

    template<class A0,class A1,class A2,class A3,class A4,class A5,class A6,class A7>
    uint32_t call(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7)
    { return callWithFlags(0, a0, a1, a2, a3, a4, a5, a6, a7, Null_t()); }

    template<class A0,class A1,class A2,class A3,class A4,class A5,class A6,class A7,class A8>
    uint32_t call(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7, const A8& a8)
    { return callWithFlags(0, a0, a1, a2, a3, a4, a5, a6, a7, a8); }

    uint32_t callNoReply()
    { return callWithFlags(ADBusNoReplyExpectedFlag, Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t()); }

    template<class A0>
    uint32_t callNoReply(const A0& a0)
    { return callWithFlags(ADBusNoReplyExpectedFlag, a0, Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t()); }

    template<class A0,class A1>
    uint32_t callNoReply(const A0& a0, const A1& a1)
    { return callWithFlags(ADBusNoReplyExpectedFlag, a0, a1, Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t()); }

    template<class A0,class A1,class A2>
    uint32_t callNoReply(const A0& a0, const A1& a1, const A2& a2)
    { return callWithFlags(ADBusNoReplyExpectedFlag, a0, a1, a2, Null_t(), Null_t(), Null_t(), Null_t(), Null_t(), Null_t()); }

    template<class A0,class A1,class A2,class A3>
    uint32_t callNoReply(const A0& a0, const A1& a1, const A2& a2, const A3& a3)
    { return callWithFlags(ADBusNoReplyExpectedFlag, a0, a1, a2, a3, Null_t(), Null_t(), Null_t(), Null_t(), Null_t()); }

    template<class A0,class A1,class A2,class A3,class A4>
    uint32_t callNoReply(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4)
    { return callWithFlags(ADBusNoReplyExpectedFlag, a0, a1, a2, a3, a4, Null_t(), Null_t(), Null_t(), Null_t()); }

    template<class A0,class A1,class A2,class A3,class A4,class A5>
    uint32_t callNoReply(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5)
    { return callWithFlags(ADBusNoReplyExpectedFlag, a0, a1, a2, a3, a4, a5, Null_t(), Null_t(), Null_t()); }

    template<class A0,class A1,class A2,class A3,class A4,class A5,class A6>
    uint32_t callNoReply(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6)
    { return callWithFlags(ADBusNoReplyExpectedFlag, a0, a1, a2, a3, a4, a5, a6, Null_t(), Null_t()); }

    template<class A0,class A1,class A2,class A3,class A4,class A5,class A6,class A7>
    uint32_t callNoReply(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7)
    { return callWithFlags(ADBusNoReplyExpectedFlag, a0, a1, a2, a3, a4, a5, a6, a7, Null_t()); }

    template<class A0,class A1,class A2,class A3,class A4,class A5,class A6,class A7,class A8>
    uint32_t callNoReply(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7, const A8& a8)
    { return callWithFlags(ADBusNoReplyExpectedFlag, a0, a1, a2, a3, a4, a5, a6, a7, a8); }

    template<ADBUSCPP_DECLARE_TYPES>
    uint32_t callWithFlags(int flags, ADBUSCPP_CRARGS);

    uint32_t connectSignal();


  private:
    int setupMarshallerForCall(int flags = 0);

    Connection* m_Connection;
    std::string m_Service;
    std::string m_Path;
    std::string m_Interface;
    std::string m_Member;
    MessageRegistration m_Registration;
    ADBusMarshaller* m_Marshaller;
    uint32_t m_Serial;
  };

  //-----------------------------------------------------------------------------

}

