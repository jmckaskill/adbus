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
#define CLASS_DECL_ITEM_TRAILING_COMMA(NUM) class A ## NUM,
#define CLASS_LIST_ITEM(NUM) A ## NUM
#define APPEND_ARGUMENT(NUM) adbus::AppendArgument<A ## NUM>(marshaller, a ## NUM);
#define CONST_REF_ARG(NUM) const A ## NUM & a ## NUM

// creates: template<class A0, class A1>
#define TEMPLATE_CLASS_DECL REPEAT(CLASS_DECL_ITEM, COMMA, template<, >)

// creates: class A0, class A1,
#define CLASS_DECL_TRAILING_COMMA REP(CLASS_DECL_ITEM_TRAILING_COMMA)

// creates: A0, A1
#define CLASS_LIST REPEAT(CLASS_LIST_ITEM, COMMA, EMPTY, EMPTY)

// creates: adbus::appendArgument<A0>(marshaller, a0); ...
#define APPEND_ARGUMENTS REP(APPEND_ARGUMENT)

// creates: const A0& a0, const A1& a0
#define CONST_REF_ARG_LIST REPEAT(CONST_REF_ARG, COMMA, EMPTY, EMPTY)

    template<CLASS_DECL_TRAILING_COMMA
             class MemFun, class M>
    void CAT(setCallback,NUM) (Object* object, M* mfObject, MemFun mf)
    { 
        setupMatch(ADBusMethodReturnMessage);
        object->CAT(addMatch,NUM) < CLASS_LIST > (&m_Match, mf, mfObject);
    } 

    template<CLASS_DECL_TRAILING_COMMA
             class MemFun, class M>
    void CAT(setErrorCallback,NUM) (Object* object, M* mfObject, MemFun mf)
    { 
        setupMatch(ADBusErrorMessage);
        object->CAT(addMatch,NUM) < CLASS_LIST > (&m_Match, mf, mfObject);
    } 

    // template<class A0, class A1>
    TEMPLATE_CLASS_DECL
    void call( CONST_REF_ARG_LIST ) // const A0& a0, const A1& a1
    {
        setupMessage();
        struct ADBusMarshaller* marshaller = ADBusArgumentMarshaller(m_Message);
        (void) marshaller;
        // adbus::AppendArgument<A0>(marshaller, a0);
        // adbus::AppendArgument<A1>(marshaller, a1);
        APPEND_ARGUMENTS;
        sendMessage();
    }

#undef CLASS_DECL_ITEM
#undef CLASS_DECL_ITEM_TRAILING_COMMA
#undef CLASS_LIST_ITEM
#undef APPEND_ARGUMENT
#undef CONST_REF_ARG

#undef TEMPLATE_CLASS_DECL
#undef CLASS_DECL_TRAILING_COMMA
#undef CLASS_LIST
#undef APPEND_ARGUMENTS
#undef CONST_REF_ARG_LIST

#undef EMPTY
#undef COMMA
#undef REP
#undef CAT
#undef CAT2

#undef REPEAT
#undef NUM

