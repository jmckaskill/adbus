/* vim: ts=4 sw=4 sts=4 et
 *
 * Copyright (c) 2009 James R. McKaskill
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 */

#pragma once

#include "Common.h"
#include "Object.h"

#include "adbus/Marshaller.h"
#include "adbus/Message.h"

#include <string>

struct ADBusMessage;

namespace adbus{

    class Connection;

    //-----------------------------------------------------------------------------

    class MessageFactory
    {
        ADBUSCPP_NON_COPYABLE(MessageFactory);
    public:
        MessageFactory();
        ~MessageFactory();
        void reset();
        void setConnection(Connection* connection)      {m_Connection = connection;}
        void setDestination(const std::string& dest)    {m_Destination = dest;}
        void setPath(const std::string& path)           {m_Path = path;}
        void setInterface(const std::string& interface) {m_Interface = interface;}
        void setMember(const std::string& member)       {m_Member = member;}
        void setFlag(uint8_t flag)                      {m_Flags |= flag;}

        void setNoReply()                               {setFlag(ADBusNoReplyExpectedFlag);}
        void setNoAutostart()                           {setFlag(ADBusNoAutoStartFlag);}

        // This now includes a whole bunch of method callback set functions
        // like:
        //
        // template<class A0, class A1, class A2, class U, class MemFun>
        // void setCallback2(Object* object, U* mfObject, MemFun mf);
        //
        // template<class A0, class A1, class A2, class U, class MemFun>
        // void setErrorCallback2(Object* object, U* mfObject, MemFun mf);
        //
        // template<class A0, class A1, class A2>
        // void call(const A0& a0, const A1& a1, const A2& a2);

#define NUM 0
#define REPEAT(x, sep, lead, tail)
#include "MessageFactory_t.h"

#define NUM 1
#define REPEAT(x, sep, lead, tail) lead x(0) tail
#include "MessageFactory_t.h"

#define NUM 2
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) tail
#include "MessageFactory_t.h"

#define NUM 3
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) tail
#include "MessageFactory_t.h"

#define NUM 4
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) tail
#include "MessageFactory_t.h"

#define NUM 5
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) tail
#include "MessageFactory_t.h"

#define NUM 6
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) tail
#include "MessageFactory_t.h"

#define NUM 7
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) tail
#include "MessageFactory_t.h"

#define NUM 8
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) sep x(7) tail
#include "MessageFactory_t.h"

#define NUM 9
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) sep x(7) sep x(8) tail
#include "MessageFactory_t.h"

    private:
        void setupMatch(enum ADBusMessageType type);
        void setupMessage();
        void sendMessage();
        Connection*         m_Connection;
        Match               m_Match;
        uint8_t             m_Flags;
        std::string         m_Destination;
        std::string         m_Path;
        std::string         m_Interface;
        std::string         m_Member;
        ADBusMessage*       m_Message;
    };

    //-----------------------------------------------------------------------------

}

