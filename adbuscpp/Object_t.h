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
// #define REPEAT_SEPERATOR(x, sep) x(0) sep x(1)

#define EMPTY_SEPERATOR
#define COMMA_SEPERATOR ,
#define REPEAT(x) REPEAT_SEPERATOR(x, EMPTY_SEPERATOR)
#define CAT2(x,y) x ## y
#define CAT(x,y) CAT2(x,y)

#define CLASS_DECL_ITEM(NUM) class A ## NUM,
#define CLASS_LIST_ITEM(NUM) , A ## NUM

// creates: class A0, class A1, ...
#define CLASS_DECL_TRAILING_COMMA REPEAT( CLASS_DECL_ITEM )

// creates: , A0, A1 ...
#define CLASS_LIST_LEADING_COMMA REPEAT( CLASS_LIST_ITEM )

    template<CLASS_DECL_TRAILING_COMMA
             class MemFun, class M>
    void CAT(addMatch,NUM) (ADBusConnection* connection, Match* match, MemFun f, M* m)
    { 
        UserData<M*>* objectData = new UserData<M*>(m);
        UserData<MemFun>* functionData = new UserData<MemFun>(f);
        functionData->chainedFunction
            = & CAT(adbus::detail::MemberFunction,NUM) <MemFun, M CLASS_LIST_LEADING_COMMA>;

        addMatch(connection,
                 match,
                 &CallMethod,
                 functionData,
                 objectData);
    } 

#undef CLASS_DECL_ITEM
#undef CLASS_LIST_ITEM

#undef CLASS_DECL_TRAILING_COMMA
#undef CLASS_LIST_LEADING_COMMA

#undef EMPTY_SEPERATOR
#undef COMMA_SEPERATOR
#undef REPEAT
#undef CAT
#undef CAT2

#undef NUM
#undef REPEAT_SEPERATOR

