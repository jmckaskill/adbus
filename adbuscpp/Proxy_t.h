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
// #define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) tail

#define EMPTY
#define COMMA ,
#define REP(x) REPEAT(x, EMPTY, EMPTY, EMPTY)
#define CAT2(x,y) x ## y
#define CAT(x,y) CAT2(x,y)

#define CLASS_DECL_ITEM(NUM) class A ## NUM
#define CLASS_DECL_ITEM_TC(NUM) class A ## NUM,
#define CLASS_LIST_ITEM_LC(NUM) , A ## NUM
#define APPEND_ARGUMENT(NUM) adbus::AppendArgument<A ## NUM>(m_Factory.args, a ## NUM);
#define CONST_REF_ARG_LC(NUM) , const A ## NUM & a ## NUM

// creates: template<class A0, class A1>
#define TEMPLATE_CLASS_DECL REPEAT(CLASS_DECL_ITEM, COMMA, template<, >)

// creates: class A0, class A1,
#define CLASS_DECL_TC REP(CLASS_DECL_ITEM_TC)

// creates: , A0, A1
#define CLASS_LIST_LC REP(CLASS_LIST_ITEM_LC)

// creates: adbus::appendArgument<A0>(marshaller, a0); ...
#define APPEND_ARGUMENTS REP(APPEND_ARGUMENT)

// creates: const A0& a0, const A1& a0
#define CONST_REF_ARG_LIST_LC REP(CONST_REF_ARG_LC)

    template<CLASS_DECL_TC class MF, class O>
    void CAT(setCallback,NUM) (Object* object, O* o, MF mf)
    {
        CAT(detail::CreateMFCallback,NUM) <MF, O CLASS_LIST_LC> (
                mf, o, &m_Factory.user1, &m_Factory.user2);

        m_Factory.callback = &detail::CallMethod;
    } 

    template<CLASS_DECL_TC class MF, class O>
    void CAT(setErrorCallback,NUM) (Object* object, O* o, MF mf)
    { 
        CAT(detail::CreateMFCallback,NUM) <MF, O CLASS_LIST_LC> (
                mf, o, &m_Factory.errorUser1, &m_Factory.errorUser2);

        m_Factory.callback = &detail::CallMethod;
    } 

    // template<class A0, class A1>
    TEMPLATE_CLASS_DECL
    void call(const std::string& member CONST_REF_ARG_LIST_LC ) // const A0& a0, const A1& a1
    {
        // adbus::AppendArgument<A0>(m_Factory.args, a0);
        // adbus::AppendArgument<A1>(m_Factory.args, a1);
        APPEND_ARGUMENTS;
        ADBusCallFactory(&m_Factory);
        ADBusProxyFactory(m_Proxy, &m_Factory);
    }

#undef CLASS_DECL_ITEM
#undef CLASS_DECL_ITEM_TC
#undef CLASS_LIST_ITEM_LC
#undef APPEND_ARGUMENT
#undef CONST_REF_ARG_LC

#undef TEMPLATE_CLASS_DECL
#undef CLASS_DECL_TC
#undef CLASS_LIST_LC
#undef APPEND_ARGUMENTS
#undef CONST_REF_ARG_LIST_LC

#undef EMPTY
#undef COMMA
#undef REP
#undef CAT
#undef CAT2

#undef REPEAT
#undef NUM

