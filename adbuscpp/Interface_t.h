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
#define COMMA_SEPERATOR
#define REPEAT(x) REPEAT_SEPERATOR(x, EMPTY_SEPERATOR)
#define CAT2(x,y) x ## y
#define CAT(x,y) CAT2(x,y)

#define CLASS_DECL_LC_ITEM(NUM) , class A ## NUM
#define CLASS_LIST_LC_ITEM(NUM) , A ## NUM
#define ARGNAME_LC_ITEM(NUM) , const std::string& arg ## NUM
#define ADD_ARGUMENT_ITEM(NUM) addArgument<A ## NUM>(arg ## NUM);

// creates: , class A0, class A1 ...
#define CLASS_DECL_LC REPEAT( CLASS_DECL_LC_ITEM )

// creates: , const std::string& arg0, const std::string& arg1 ...
#define ARGNAME_LIST_LC REPEAT( ARGNAME_LC_ITEM )

// creates: , A0, A1 ...
#define CLASS_LIST_LC REPEAT( CLASS_LIST_LC_ITEM )

// creates: addArgument<A0>(arg0); addArgument<A1>(arg1); ...
#define ADD_ARGUMENTS REPEAT( ADD_ARGUMENT_ITEM )

    template<class O CLASS_DECL_LC, class MF>
    Member& CAT(setMethod,NUM) (MF f ARGNAME_LIST_LC )
    { 
        // addArgument<A0>(arg0);
        // addArgument<A1>(arg1);
        // ...
        ADD_ARGUMENTS;

        struct ADBusUser* user1;
        CAT(detail::CreateMFCallback, NUM) <MF, O CLASS_LIST_LC> (
                f, (O*) NULL, &user1, NULL);

        setMethod(&detail::CallMethod, user1);
        return *this;
    }

    template<class O, class R CLASS_DECL_LC, class MF>
    Member& CAT(setMethodReturn,NUM) (MF f, const std::string& ret ARGNAME_LIST_LC )
    { 
        addReturn<R>(ret);
        // addArgument<A0>(arg0);
        // addArgument<A1>(arg1);
        // ...
        ADD_ARGUMENTS;

        struct ADBusUser* user1;
        CAT(detail::CreateMFReturnCallback,NUM) <MF, O, R CLASS_LIST_LC> (
                f, (O*) NULL, &user1, NULL);

        setMethod(&detail::CallMethod, user1);
        return *this;
    }

#undef CLASS_DECL_LC_ITEM
#undef CLASS_LIST_LC_ITEM
#undef ARGNAME_LC_ITEM
#undef ADD_ARGUMENT_ITEM

#undef CLASS_DECL_LC
#undef CLASS_LIST_LC
#undef ARGNAME_LIST_LC
#undef ADD_ARGUMENTS

#undef EMPTY_SEPERATOR
#undef COMMA_SEPERATOR
#undef REPEAT
#undef CAT2
#undef CAT

#undef NUM
#undef REPEAT_SEPERATOR

