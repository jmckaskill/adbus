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

#include "Bind.h"
#include "Common.h"
#include "Message.h"

#include "adbus/Interface.h"
#include "adbus/User.h"

#include <string>


namespace adbus
{

    // ----------------------------------------------------------------------------

    /** Something about member.
     *
     * \ingroup adbuscpp
     */

    class Member
    {
    public:
        Member(ADBusMember* member, ADBusMemberType type);

        Member& addAnnotation(const std::string& name, const std::string& value);

        Member& addArgument(const std::string& name, const std::string& type);
        Member& addReturn(const std::string& name, const std::string& type);

        template<class T> Member& addArgument(const std::string& name)
        { return addArgument(name, ADBusTypeString((T*) NULL)); }
        template<class T> Member& addReturn(const std::string& name)
        { return addReturn(name, ADBusTypeString((T*) NULL)); }

        Member& setMethod(ADBusMessageCallback callback, ADBusUser* user1);
        Member& setGetter(ADBusMessageCallback callback, ADBusUser* user1);
        Member& setSetter(ADBusMessageCallback callback, ADBusUser* user1);

        // O is the type of the object bound in
        // T is the type of the property
        template<class O, class T, class MF> Member& setGetter(MF f);
        template<class O, class T, class MF> Member& setSetter(MF f);

#ifdef DOC
        // This now includes a whole bunch of method callback set functions
        // like:
        //
        // M is the type of the object bound in
        // A0, A1 ... are the base types of the arguments
        // arg0, arg1 .. are the names of the arguments given in the
        // introspection

        template<class O, class A0 ..., class MF>
        Member& setMethodX(MF f, const std::string& arg0 ...);

        template<class O, class R, class A0 ..., class MF>
        Member& setMethodReturnX(MF f, const std::string& arg0 ...);

#else

#define NUM 0
#define REPEAT_SEPERATOR(x, sep)
#include "Interface_t.h"

#define NUM 1
#define REPEAT_SEPERATOR(x, sep) x(0)
#include "Interface_t.h"

#define NUM 2
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1)
#include "Interface_t.h"

#define NUM 3
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) sep x(2)
#include "Interface_t.h"

#define NUM 4
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) sep x(2) sep x(3)
#include "Interface_t.h"

#define NUM 5
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) sep x(2) sep x(3) sep x(4)
#include "Interface_t.h"

#define NUM 6
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5)
#include "Interface_t.h"

#define NUM 7
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6)
#include "Interface_t.h"

#define NUM 8
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) sep x(7)
#include "Interface_t.h"

#define NUM 9
#define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) sep x(7) sep x(8)
#include "Interface_t.h"

#endif

        operator ADBusMember*()const {return m_Member;}
    private:
        ADBusMember*    m_Member;
        ADBusMemberType m_Type;
    };

    // ----------------------------------------------------------------------------

    /** Something about interface.
     *
     * \ingroup adbuscpp
     */

    class Interface
    {
        ADBUSCPP_NON_COPYABLE(Interface);
    public:
        Interface(const std::string& name);
        ~Interface();

        Member addMethod(const std::string& name);
        Member addSignal(const std::string& name);
        Member addProperty(const std::string& name, const std::string& type);
        template<class T> Member addProperty(const std::string& name)
        { return addProperty(name, ADBusTypeString((T*) NULL)); }

        Member method(const std::string& name);
        Member signal(const std::string& name);
        Member property(const std::string& name);

        operator ADBusInterface*()const {return m_Interface;}

    private:
        ADBusInterface* m_Interface;
    };

    // ----------------------------------------------------------------------------
    // ----------------------------------------------------------------------------
    // ----------------------------------------------------------------------------

    template<class O, class T, class MF>
    int SetPropertyCallback(struct ADBusCallDetails* d)
    {
        UserData<MF>* functionData = (UserData<MF>*) d->user1;
        UserData<O*>* objectData   = (UserData<O*>*) d->user2;
        MF function = functionData->data;
        O* object   = objectData->data;

        T value;
        value << *d->propertyIterator;
        (object->*function)(value);
        return 0;
    }

    template<class O, class T, class MF>
    int GetPropertyCallback(struct ADBusCallDetails* d)
    {
        UserData<MF>* functionData = (UserData<MF>*) d->user1;
        UserData<O*>* objectData   = (UserData<O*>*) d->user2;
        MF function = functionData->data;
        O* object   = objectData->data;

        T value = (object->*function)();
        value >> *d->propertyMarshaller;
        return 0;
    }


    template<class O, class T, class MF>
    Member& Member::setGetter(MF f)
    {
        UserData<MF>* functionData = new UserData<MF>(f);
        functionData->chainedFunction = &GetPropertyCallback<O, T, MF>;
        setGetter(&detail::CallMethod, functionData);
        return *this;
    }

    template<class O, class T, class MF>
    Member& Member::setSetter(MF f)
    {
        UserData<MF>* functionData = new UserData<MF>(f);
        functionData->chainedFunction = &SetPropertyCallback<O, T, MF>;
        setSetter(&detail::CallMethod, functionData);
        return *this;
    }

    // ----------------------------------------------------------------------------

}

