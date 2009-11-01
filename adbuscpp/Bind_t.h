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

#define DEMARSHALL(NUM) A ## NUM a ## NUM; a ## NUM << *d->args;
#define ARGUMENT(NUM) a ## NUM
#define CLASS_DECL_LC_ITEM(NUM) , class A ## NUM
#define CLASS_LIST_LC_ITEM(NUM) , A ## NUM

// creates: , class A0, class A1 ...
#define CLASS_DECL_LC REPEAT( CLASS_DECL_LC_ITEM )

// creates: , A0, A1
#define CLASS_LIST_LC REPEAT( CLASS_LIST_LC_ITEM )

// creates: A0 a0; a0 << *d->args; A1 a1; a1 << *d->args; ...
#define DEMARSHALL_ARGUMENTS REPEAT( DEMARSHALL )

// creates: a0, a1, a2 ...
#define ARG_LIST REPEAT_SEPERATOR( ARGUMENT, COMMA_SEPERATOR ) 

template<class MF, class O CLASS_DECL_LC >
int CAT(MFCallback,NUM) (struct ADBusCallDetails* d)
{
    UserData<MF>* function = (UserData<MF>*) d->user1;
    UserData<O*>* object   = (UserData<O*>*) d->user2;
    MF mf = function->data;
    O* o  = object->data;
    // A0 a0;
    // a0 << *d->args;
    // A1 a1;
    // a1 << *d->args;
    // ...
    DEMARSHALL_ARGUMENTS;

    adbus::MessageEnd end;
    end << *d->args;

    // (o->*mf)( a0, a1, a2 ...);
    (o->*mf)( ARG_LIST );

    return 0;
}

template<class MF, class O CLASS_DECL_LC >
void CAT(CreateMFCallback,NUM) (MF          function,
                                O*          object,
                                ADBusUser** user1,
                                ADBusUser** user2)
{
    if (user1) {
        UserData<MF>* fdata = new UserData<MF>(function);
        fdata->chainedFunction = & CAT(MFCallback,NUM) <MF, O CLASS_LIST_LC>;
        *user1 = fdata;
    }

    if (user2) {
        UserData<O*>* odata = new UserData<O*>(object);
        *user2 = odata;
    }
}

template<class MF, class O, class R CLASS_DECL_LC >
int CAT(MFReturnCallback,NUM) (struct ADBusCallDetails* d)
{
    UserData<MF>* function  = (UserData<MF>*) d->user1;
    UserData<O*>* object    = (UserData<O*>*) d->user2;
    MF mf = function->data;
    O* o  = object->data;
    // A0 a0;
    // a0 << *details->arguments;
    // A1 a1;
    // a1 << *details->arguments;
    // ...
    DEMARSHALL_ARGUMENTS;

    adbus::MessageEnd end;
    end << *d->args;

    // R r = (o->*mf)( a0, a1, a2 ...);
    R r = (o->*mf)( ARG_LIST );
    if (d->retargs)
    {
        std::string type = ADBusTypeString((R*) NULL);
        ADBusAppendArguments(d->retargs, type.c_str(), (int) type.size());
        r >> *d->retargs;
    }

    return 0;
}

template<class MF, class O, class R CLASS_DECL_LC>
void CAT(CreateMFReturnCallback,NUM) (MF            function,
                                      O*            object,
                                      ADBusUser**   user1,
                                      ADBusUser**   user2)
{
    if (user1) {
        UserData<MF>* fdata = new UserData<MF>(function);
        fdata->chainedFunction = & CAT(MFReturnCallback,NUM) <MF, O, R CLASS_LIST_LC>;
        *user1 = fdata;
    }

    if (user2) {
        UserData<O*>* odata = new UserData<O*>(object);
        *user2 = odata;
    }
}


#undef DEMARSHALL
#undef ARGUMENT
#undef CLASS_DECL_LC_ITEM
#undef CLASS_LIST_LC_ITEM

#undef CLASS_DECL_LC
#undef CLASS_LIST_LC
#undef DEMARSHALL_ARGUMENTS
#undef ARG_LIST

#undef EMPTY_SEPERATOR
#undef COMMA_SEPERATOR
#undef REPEAT
#undef CAT
#undef CAT2

#undef NUM
#undef REPEAT_SEPERATOR
