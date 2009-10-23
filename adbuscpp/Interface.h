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
    class SignalBase;

    void CallMethod(struct ADBusCallDetails* details);

    // ----------------------------------------------------------------------------

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

        // M is the type of the object bound in
        // T is the type of the property
        template<class M, class T, class MemFun> Member& setGetter(MemFun f);
        template<class M, class T, class MemFun> Member& setSetter(MemFun f);

        // This now includes a whole bunch of method callback set functions
        // like:
        //
        // M is the type of the object bound in
        // A0, A1 ... are the base types of the arguments
        // arg0, arg1 .. are the names of the arguments given in the
        // introspection
        //
        // template<class M, class A0, class A1, class MemFun>
        // Member& setMethod2(MemFun f,
        //                    const std::string& arg0,
        //                    const std::string& arg1);
        //
        // template<class M, class R, class A0, class A1, class MemFun>
        // Member& setMethodReturn2(MemFun f,
        //                    const std::string& arg0,
        //                    const std::string& arg1);

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

        operator ADBusMember*()const{return m;}
    private:
        ADBusMember* m;
        ADBusMemberType m_Type;
    };

    // ----------------------------------------------------------------------------


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

        operator ADBusInterface*()const{return m_I;}

    private:
        ADBusInterface* m_I;
    };

    // ----------------------------------------------------------------------------
    // ----------------------------------------------------------------------------
    // ----------------------------------------------------------------------------

    template<class M, class T, class F>
    void SetPropertyCallback(
            struct ADBusCallDetails*    details)
    {
        UserData<F>* functionData = (UserData<F>*) details->user1;
        UserData<M*>* objectData = (UserData<M*>*) details->user2;
        F function = functionData->data;
        M* object = objectData->data;

        T value;
        value << *details->propertyIterator;
        (object->*function)(value);
    }

    template<class M, class T, class F>
    void GetPropertyCallback(
            struct ADBusCallDetails*    details)
    {
        UserData<F>* functionData = (UserData<F>*) details->user1;
        UserData<M*>* objectData = (UserData<M*>*) details->user2;
        F function = functionData->data;
        M* object = objectData->data;

        T value = (object->*function)();
        value >> *details->propertyMarshaller;
    }


    template<class M, class T, class MemFun>
    Member& Member::setGetter(MemFun f)
    {
        UserData<MemFun>* functionData = new UserData<MemFun>(f);
        functionData->chainedFunction = &GetPropertyCallback<M, T, MemFun>;
        setGetter(&CallMethod, functionData);
        return *this;
    }

    template<class M, class T, class MemFun>
    Member& Member::setSetter(MemFun f)
    {
        UserData<MemFun>* functionData = new UserData<MemFun>(f);
        functionData->chainedFunction = &SetPropertyCallback<M, T, MemFun>;
        setSetter(&CallMethod, functionData);
        return *this;
    }

    // ----------------------------------------------------------------------------

}

