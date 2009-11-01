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

#include "Bind.h"
#include "Interface.h"
#include "Message.h"

#include "adbus/Connection.h"
#include "adbus/Match.h"
#include "adbus/ObjectPath.h"

#include <string>
#include <vector>

namespace adbus
{

    class ObjectPath
    {
    public:
        ObjectPath()
        { m_Path = NULL; }

        ObjectPath(ADBusObjectPath* p)
        { m_Path = p; }

        ObjectPath(ADBusConnection* c, const std::string& p)
        { m_Path = ADBusGetObjectPath(c, p.c_str(), (int) p.size()); }

        ObjectPath operator/(const std::string& p) const
        { return ObjectPath(ADBusRelativePath(m_Path, p.c_str(), (int) p.size())); }

        ADBusConnection* connection() const
        {return m_Path->connection;}

        bool isValid() const
        {return m_Path != NULL;}

        std::string pathString() const
        {return std::string(m_Path->path, m_Path->pathSize);}

        const char* path() const
        {return m_Path->path;}

        operator ADBusObjectPath*() const
        {return m_Path;}
    private:
        ADBusObjectPath*    m_Path;
    };

    /** Something about Object.
     *
     * \ingroup adbuscpp
     */

    class Object
    {
        ADBUSCPP_NON_COPYABLE(Object);
    public:
        Object();
        virtual ~Object();

        void reset();

        template<class O>
        void bind(
                ADBusConnection*    connection,
                const std::string&  path,
                ADBusInterface*     interface,
                O*                  object);

        template<class O>
        void bind(
                ADBusObjectPath*    path,
                ADBusInterface*     interface,
                O*                  object);

        void unbind(
                ADBusConnection*    connection,
                const std::string&  path,
                ADBusInterface*     interface);

        void unbind(
                ADBusObjectPath*    path,
                ADBusInterface*     interface);

        uint32_t addMatch(ADBusConnection* connection, ADBusMatch* match);
        void addMatch(ADBusConnection* connection, uint32_t matchId);
        void removeMatch(ADBusConnection* connection, uint32_t matchId);

#ifdef DOC

        template<class A0 ..., class MF, class O>
        void addMatchX(ADBusConnection* c, ADBusMatch* m, MF mf, O* m);

#else

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

#endif

        operator struct ADBusObject*()const {return m_Object;}

    private:
        void doBind(
                ADBusObjectPath*    path,
                ADBusInterface*     interface,
                struct ADBusUser*   user2);

        struct ADBusObject*     m_Object;
    };

    template<class O>
    void Object::bind(
            ADBusConnection*    connection,
            const std::string&  path,
            ADBusInterface*     interface,
            O*                  object)
    {
        ADBusObjectPath* o = ADBusGetObjectPath(
                connection,
                path.c_str(),
                (int) path.size());

        UserData<O*>* odata = new UserData<O*>(object);
        doBind(o, interface, odata);
    }

    template<class O>
    void Object::bind(
            ADBusObjectPath*    path,
            ADBusInterface*     interface,
            O*                  object)
    {
        UserData<O*>* odata = new UserData<O*>(object);
        doBind(path, interface, odata);
    }

}


