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

// This is a multi included file
// arguments to this file are:
// #define NUM 2
// #define REPEAT_SEPERATOR(x, sep) x(0) sep x(1) ...

#define EMPTY_SEPERATOR
#define COMMA_SEPERATOR ,
#define REPEAT(x) REPEAT_SEPERATOR(x, EMPTY_SEPERATOR)
#define CAT2(x,y) x ## y 
#define CAT(x,y) CAT2(x,y)

#define DEMARSHALL(NUM) A ## NUM a ## NUM; a ## NUM << *details->arguments;
#define ARGUMENT(NUM) a ## NUM
#define CLASS_DECL_ITEM(NUM) , class A ## NUM

// creates: , class A0, class A1 ...
#define CLASS_DECL_LEADING_COMMA REPEAT( CLASS_DECL_ITEM )

// creates: A0 a0; a0 << *details->arguments; A1 a1; a1 << *details->arguments; ...
#define DEMARSHALL_ARGUMENTS REPEAT( DEMARSHALL )

// creates: a0, a1, a2 ...
#define ARG_LIST REPEAT_SEPERATOR( ARGUMENT, COMMA_SEPERATOR ) 

namespace adbus
{
    namespace detail
    {

        template<class F, class M CLASS_DECL_LEADING_COMMA >
        void CAT(MemberFunction,NUM) (struct ADBusCallDetails* details)
        {
            UserData<F>* function = (UserData<F>*) details->user1;
            UserData<M*>* object = (UserData<M*>*) details->user2;
            F f = function->data;
            M* m = object->data;
            // A0 a0;
            // a0 << *details->arguments;
            // A1 a1;
            // a1 << *details->arguments;
            // ...
            DEMARSHALL_ARGUMENTS;

            adbus::MessageEnd end;
            end << *details->arguments;

            // (m->*f)( a0, a1, a2 ...);
            (m->*f)( ARG_LIST );
        }

        template<class F, class M, class R CLASS_DECL_LEADING_COMMA >
        void CAT(MemberFunctionReturn,NUM) (struct ADBusCallDetails* details)
        {
            UserData<F>* function = (UserData<F>*) details->user1;
            UserData<M*>* object = (UserData<M*>*) details->user2;
            F f = function->data;
            M* m = object->data;
            // A0 a0;
            // a0 << *details->arguments;
            // A1 a1;
            // a1 << *details->arguments;
            // ...
            DEMARSHALL_ARGUMENTS;

            adbus::MessageEnd end;
            end << *details->arguments;

            // R r = (m->*f)( a0, a1, a2 ...);
            R r = (m->*f)( ARG_LIST );
            if (details->returnMessage)
            {
                struct ADBusMarshaller* m =
                        ADBusArgumentMarshaller(details->returnMessage);
                std::string type = ADBusTypeString((R*) NULL);
                ADBusAppendArguments(m, type.c_str(), type.size());
                r >> *m;
            }
        }

    }
}

#undef DEMARSHALL
#undef ARGUMENT
#undef CLASS_DECL_ITEM

#undef CLASS_DECL_LEADING_COMMA
#undef DEMARSHALL_ARGUMENTS
#undef ARG_LIST

#undef EMPTY_SEPERATOR
#undef COMMA_SEPERATOR
#undef REPEAT
#undef CAT
#undef CAT2

#undef NUM
#undef REPEAT_SEPERATOR
