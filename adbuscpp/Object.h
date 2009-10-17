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
#include "Connection.h"
#include "Interface.h"

#include <string>
#include <vector>

namespace adbus
{
    struct Match
    {
        Match()
        :   type(ADBusInvalidMessage),
            addMatchToBusDaemon(0),
            removeOnFirstMatch(0),
            replySerial(0xFFFFFFFF)
        {}

        enum ADBusMessageType type;

        bool            addMatchToBusDaemon;
        bool            removeOnFirstMatch;

        uint32_t        replySerial;

        std::string     sender;
        std::string     destination;
        std::string     interface;
        std::string     path;
        std::string     member;
        std::string     errorName;
    };


    class Object
    {
        ADBUSCPP_NON_COPYABLE(Object);
    public:
        Object();
        virtual ~Object();

        const std::string& path()const{return m_Path;}
        std::string childPath(const std::string& child);

        Connection* connection()const{return m_Connection;}

        virtual void registerObject(Connection* connection, const std::string& path);
        virtual void deregisterObject();

        template<class M> 
        void bind(Interface* interface, M* object);

        void addMatch(Match*                match,
                      ADBusMessageCallback  function,
                      struct ADBusUser*     user1,
                      struct ADBusUser*     user2);

        // This now includes a whole bunch of match callback functions like:
        //
        // template<class MemFun, class M>
        // void addMatch0(Match* m, MemFun mf, M* m);
        //
        // template<class A0, class MemFun, class M>
        // void addMatch1(Match* m, MemFun mf, M* m);
        //
        // template<class A0, class A1, class MemFun, class M>
        // void addMatch2(Match* m, MemFun mf, M* m);

#define NUM 0
#define REPEAT_SEPERATOR(x, sep)
#include "Object_t.h"

#define NUM 1
#define REPEAT_SEPERATOR(x, sep) x(0)
#include "Object_t.h"

#define NUM 2
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1)
#include "Object_t.h"

#define NUM 3
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) sep x(2)
#include "Object_t.h"

#define NUM 4
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) sep x(2) sep x(3)
#include "Object_t.h"

#define NUM 5
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) sep x(2) sep x(3) sep x(4)
#include "Object_t.h"

#define NUM 6
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5)
#include "Object_t.h"

#define NUM 7
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6)
#include "Object_t.h"

#define NUM 8
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) sep x(7)
#include "Object_t.h"

#define NUM 9
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) sep x(7) sep x(8)
#include "Object_t.h"

    private:
        void doBind(Interface* interface, struct ADBusUser* user2);

        friend class BoundSignalBase;
        ADBusObject*            m_Object;
        Connection*             m_Connection;
        std::string             m_Path;
        std::vector<uint32_t>   m_Matches;
    };

    template<class M>
    void Object::bind(Interface* interface, M* object)
    {
        UserData<M*>* objectData = new UserData<M*>(object);
        doBind(interface, objectData);
    }


}

