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
// #define NUM(x)  x ## 2
// #define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) tail

#define EMPTY
#define COMMA ,
#define REP(x) REPEAT(x, EMPTY, EMPTY, EMPTY)
#define CAT2(x,y) x ## y
#define CAT(x,y) CAT2(x,y)



// creates: template<class A0, class A1>
#define CLASS_DECL(x)           class A ## x
#define TEMPLATE_CLASS_DECLS    REPEAT(CLASS_DECL, COMMA, template<, >)

// creates: , class A0, class A1
#define CLASS_DECL_LC(x)        , class A ## x
#define CLASS_DECLS_LC          REP(CLASS_DECL_LC)

// creates: class A0, class A1,
#define CLASS_DECL_TC(x)        class A ## x,
#define CLASS_DECLS_TC          REP(CLASS_DECL_TC)

// creates: , A0, A1
#define CLASS_NAME_LC(x)        , A ## x
#define CLASS_NAMES_LC          REP(CLASS_NAME_LC)

// creates: A0, A1, 
#define CLASS_NAME_TC(x)        A ## x,
#define CLASS_NAMES_TC          REP(CLASS_NAME_TC)

// creates <A0, A1, ...>
#define CLASS_NAME(x)           A ## x
#define SB_CLASS_NAMES          REPEAT(CLASS_NAME, COMMA, <, >)

// creates: A0 a0; a0 << *d->args; ...
#define DEMARSHALL_ARG(x)       A ## x a ## x; if (a ## x << i) {return -1;}
#define DEMARSHALL_ARGS         REP(DEMARSHALL_ARG)

// creates: signature += adbus_type_string((A##x*) NULL);
#define APPEND_SIG(x)           signature += adbus_type_string((A##x*) NULL);
#define APPEND_SIGS             REP(APPEND_SIG)

// creates: a ## x >> b;
#define APPEND_ARG(x)           a ## x >> b;
#define APPEND_ARGS             REP(APPEND_ARG)

// creates: a0, a1, a2 ...
#define ARG(x)                  a ## x
#define ARGS                    REPEAT(ARG, COMMA, EMPTY, EMPTY)

// creates: , const A0& a0, const A1& a0
#define CONST_REF_ARG_LC(x)     , const A ## x & a ## x
#define CONST_REF_ARGS_LC       REP(CONST_REF_ARG_LC)

// creates: const A0& a0, const A1& a0
#define CONST_REF_ARG(x)        const A ## x & a ## x
#define CONST_REF_ARGS          REPEAT(CONST_REF_ARG, COMMA, EMPTY, EMPTY)

// creates: case 0: type = adbus_type_string((A0*) NULL); break; ...
#define TYPE_STRING_CASE(x)     case x: type = adbus_type_string((A ## x*) NULL); break;
#define TYPE_STRING_CASES       REP(TYPE_STRING_CASE)





/* -------------------------------------------------------------------------- */
#if ADBUSCPP_MULTI == ADBUSCPP_MULTI_BIND

    namespace detail
    {

        template<class MF, class O CLASS_DECLS_LC>
        int NUM(MFCallback) (adbus_CbData* d)
        {
            MF mf = GetUser<MF>(d->user1);
            O* o  = GetUser<O*>(d->user2);

            Iterator i;
            i.i = d->args;

            // A0 a0; if (a0 << i) {return -1;}
            // ...
            DEMARSHALL_ARGS;

            adbus::MessageEnd end;
            end << i;

            // (o->*mf)( a0, a1, a2 ...);
            try {
                (o->*mf)( ARGS );
            } catch (Error& e) {
                adbus_setup_error(d, e.name(), -1, e.message(), -1);
            }

            return 0;
        }

        template<class MF, class O, class R CLASS_DECLS_LC>
        int NUM(MFReturnCallback) (adbus_CbData* d)
        {
            MF mf = GetUser<MF>(d->user1);
            O* o  = GetUser<O*>(d->user2);

            Iterator i;
            i.i = d->args;

            // A0 a0; a0 << i;
            // ...
            DEMARSHALL_ARGS;

            adbus::MessageEnd end;
            end << i;

            // (o->*mf)( a0, a1, a2 ...);
            try {
                R r = (o->*mf)( ARGS );
                if (d->retmessage) {
                    struct Buffer b;
                    b.b = adbus_msg_buffer(d->retmessage);

                    std::string type = adbus_type_string((R*) NULL);
                    adbus_buf_append(b.b, type.c_str(), (int) type.size());
                    r >> b;
                }
            } catch (Error& e) {
                adbus_setup_error(d, e.name(), -1, e.message(), -1);
            }

            return 0;
        }

        template<class MF, class O CLASS_DECLS_LC>
        void NUM(CreateMFCallback) (MF function, O* object, adbus_Callback* callback, adbus_User** user1, adbus_User** user2)
        {
            if (callback)
                *callback = &NUM(MFCallback) <MF, O CLASS_NAMES_LC>;
            if (user1)
                *user1 = CreateUser<MF>(function);
            if (user2)
                *user2 = CreateUser<O*>(object);
        }


        template<class MF, class O, class R CLASS_DECLS_LC>
        void NUM(CreateMFReturnCallback) (MF function, O* object, adbus_Callback* callback, adbus_User** user1, adbus_User** user2)
        {
            if (callback)
                *callback = &NUM(MFReturnCallback) <MF, O, R CLASS_NAMES_LC>;
            if (user1)
                *user1 = CreateUser<MF>(function);
            if (user2)
                *user2 = CreateUser<O*>(object);
        }

    }

/* -------------------------------------------------------------------------- */
#elif ADBUSCPP_MULTI == ADBUSCPP_MULTI_MATCH

        template<CLASS_DECLS_TC class MF, class O>
        void NUM(setCallback) (MF function, O* object)
        {
            NUM(detail::CreateMFCallback) <MF, O CLASS_NAMES_LC> (
                    function,
                    object,
                    &this->callback,
                    &this->user1,
                    &this->user2);
        }


/* -------------------------------------------------------------------------- */
#elif ADBUSCPP_MULTI == ADBUSCPP_MULTI_PROXY


        template<CLASS_DECLS_TC class MF, class O>
        void NUM(setCallback) (MF function, O* object)
        {
            NUM(detail::CreateMFCallback) SB_CLASS_NAMES (function, object, &m_Callback, &m_User1, &m_User2);
        }

        TEMPLATE_CLASS_DECLS
        uint32_t call (const std::string& member CONST_REF_ARGS_LC)
        {
            adbus_Caller call;
            adbus_call_proxy(&call, m_Proxy, member.c_str(), (int) member.size());

            Buffer b;
            b.b = adbus_msg_buffer(call.msg);

            std::string signature;
            // signature += adbus_type_string((A0*) NULL); ...
            APPEND_SIGS;
            adbus_buf_append(b, signature.c_str(), (int) signature.size());

            // a0 >> b; ...
            APPEND_ARGS;

            if (m_Callback) {
                call.callback = m_Callback;
                call.user1 = m_User1;
                call.user2 = m_User2;
                m_User1 = m_User2 = NULL;
            }
            if (m_ErrorCallback) {
                call.errorCallback = m_ErrorCallback;
                call.errorUser1 = m_ErrorUser1;
                call.errorUser2 = m_ErrorUser2;
                m_ErrorUser1 = m_ErrorUser2 = NULL;
            }
            return adbus_call_send(&call);
        }

/* -------------------------------------------------------------------------- */
#elif ADBUSCPP_MULTI == ADBUSCPP_MULTI_CLASSES

    TEMPLATE_CLASS_DECLS
    class NUM(Signal)
    {
        ADBUSCPP_NOT_COPYABLE(NUM(Signal));
    public:
        NUM(Signal)() : m_Signal(NULL) 
        {
            std::string& signature = m_Signature;
            // signature += adbus_type_string((A0*) NULL);
            APPEND_SIGS;
            (void) signature;
        }

        ~NUM(Signal)() 
        {
            adbus_sig_free(m_Signal);
        }

        void bind(adbus_Path* path, adbus_Member* mbr)
        {
            adbus_sig_free(m_Signal);
            m_Signal = adbus_sig_new(path, mbr);
        }

        void trigger(CONST_REF_ARGS)
        {
            adbus_Caller call;
            adbus_call_signal(&call, m_Signal);

            Buffer b;
            b.b = adbus_msg_buffer(call.msg);
            adbus_buf_append(b, m_Signature.c_str(), (int) m_Signature.size());

            // a0 >> b; ...
            APPEND_ARGS

            adbus_call_send(&call);
        }

        void operator()(CONST_REF_ARGS)
        {
            trigger(ARGS);
        }

    private:
        adbus_Signal* m_Signal;
        std::string   m_Signature;
    };

    TEMPLATE_CLASS_DECLS
    class NUM(MemberBase)
    {
    public:
        NUM(MemberBase) (adbus_Member* m) : m_M(m), m_Arg(0) {}

    protected:
        void baseAddAnnotation(const std::string& name, const std::string& value)
        { adbus_mbr_addannotation(m_M, name.c_str(), (int) name.size(), value.c_str(), (int) value.size()); }

        void baseAddArgument(const std::string& name)
        {
            std::string type;
            switch (m_Arg++)
            { 
            TYPE_STRING_CASES
            default:
                assert(0);
                return;
            }

            adbus_mbr_addargument(
                    m_M,
                    name.c_str(),
                    (int) name.size(),
                    type.c_str(),
                    (int) type.size());
        }

        adbus_Member* m_M;
        int m_Arg;
    };


    TEMPLATE_CLASS_DECLS
    class NUM(SignalMember) : public NUM(MemberBase) SB_CLASS_NAMES
    {
    public:
        NUM(SignalMember) (adbus_Member* m) : NUM(MemberBase) SB_CLASS_NAMES (m) {}

        NUM(SignalMember)& addAnnotation(const std::string& name, const std::string& value)
        { this->baseAddAnnotation(name, value); return *this; }

        NUM(SignalMember)& addArgument(const std::string& name)
        { this->baseAddArgument(name); return *this; }
    };

    TEMPLATE_CLASS_DECLS
    class NUM(MethodMember) : public NUM(MemberBase) SB_CLASS_NAMES
    {
    public:
        NUM(MethodMember) (adbus_Member* m) : NUM(MemberBase) SB_CLASS_NAMES (m) {}

        NUM(MethodMember)& addAnnotation(const std::string& name, const std::string& value)
        { this->baseAddAnnotation(name, value); return *this; }

        NUM(MethodMember)& addArgument(const std::string& name)
        { this->baseAddArgument(name); return *this; }

    };


    template<class R  CLASS_DECLS_LC>
    class NUM(MethodReturnMember) : public NUM(MemberBase) SB_CLASS_NAMES
    {
    public:
        NUM(MethodReturnMember) (adbus_Member* m) : NUM(MemberBase) SB_CLASS_NAMES (m) {}

        NUM(MethodReturnMember)& addAnnotation(const std::string& name, const std::string& value)
        { this->baseAddAnnotation(name, value); return *this; }

        NUM(MethodReturnMember)& addArgument(const std::string& name)
        { this->baseAddArgument(name); return *this; }

        NUM(MethodReturnMember)& addReturn(const std::string& name)
        {
            std::string type = adbus_type_string((R*) NULL);
            adbus_mbr_addreturn(
                    this->m_M,
                    name.c_str(),
                    (int) name.size(),
                    type.c_str(),
                    (int) type.size());
            return *this;
        }
    };



/* -------------------------------------------------------------------------- */
#elif ADBUSCPP_MULTI == ADBUSCPP_MULTI_INTERFACES


        template<CLASS_DECLS_TC class MF>
        NUM(MethodMember) SB_CLASS_NAMES NUM(addMethod) (const std::string& name, MF function)
        {
            adbus_Member* mbr = adbus_iface_addmethod(m_I, name.c_str(), (int) name.size());

            adbus_Callback callback;
            adbus_User* user1;
            NUM(detail::CreateMFCallback) <MF, O CLASS_NAMES_LC> (function, (O*) NULL, &callback, &user1, NULL);
            adbus_mbr_setmethod(mbr, callback, user1);

            return NUM(MethodMember) SB_CLASS_NAMES (mbr);
        }

        template<class R, CLASS_DECLS_TC class MF>
        NUM(MethodReturnMember) <R CLASS_NAMES_LC> NUM(addReturnMethod) (const std::string& name, MF function)
        {
            adbus_Member* mbr = adbus_iface_addmethod(m_I, name.c_str(), (int) name.size());

            adbus_Callback callback;
            adbus_User* user1;
            NUM(detail::CreateMFReturnCallback) <MF, O, R CLASS_NAMES_LC> (function, (O*) NULL, &callback, &user1, NULL);
            adbus_mbr_setmethod(mbr, callback, user1);

            return NUM(MethodReturnMember) <R CLASS_NAMES_LC> (mbr);
        }

        TEMPLATE_CLASS_DECLS
        NUM(SignalMember) SB_CLASS_NAMES NUM(addSignal) (const std::string& name)
        {
            adbus_Member* mbr = adbus_iface_addsignal(m_I, name.c_str(), (int) name.size());
            return NUM(SignalMember) SB_CLASS_NAMES (mbr);
        }


#else
#error
#endif








#undef CLASS_DECL
#undef TEMPLATE_CLASS_DECLS

#undef CLASS_DECL_LC
#undef CLASS_DECLS_LC

#undef CLASS_DECL_TC
#undef CLASS_DECLS_TC

#undef CLASS_NAME_LC
#undef CLASS_NAMES_LC

#undef CLASS_NAME_TC
#undef CLASS_NAMES_TC

#undef CLASS_NAME
#undef SB_CLASS_NAMES

#undef DEMARSHALL_ARG
#undef DEMARSHALL_ARGS

#undef APPEND_ARG
#undef APPEND_ARGS

#undef ARG
#undef ARGS

#undef CONST_REF_ARG_LC
#undef CONST_REF_ARGS_LC

#undef CONST_REF_ARG
#undef CONST_REF_ARGS

#undef TYPE_STRING_CASE
#undef TYPE_STRING_CASES

#undef EMPTY
#undef COMMA
#undef REP
#undef CAT2
#undef CAT

#undef NUM
#undef REPEAT

