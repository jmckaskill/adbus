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

#define DECL_ARG(x)             A##x a##x;
#define DECL_ARGS               REP(DECL_ARG)

#define DEMARSHALL_POINTER_ARG(x)   if (*(o->a##x) << i) {return -1;}
#define DEMARSHALL_POINTER_ARGS     REP(DEMARSHALL_POINTER_ARG)

// creates: a0 << *d->args; ...
#define DEMARSHALL_ARG(x)       if (a ## x << i) {return -1;}
#define DEMARSHALL_ARGS         REP(DEMARSHALL_ARG)

#define ASSIGN_ARG(x)           o->a##x = a##x;
#define ASSIGN_ARGS             REP(ASSIGN_ARG)

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
            MF mf = *(MF*) d->user1;
            O* o  =  (O*)  d->user2;

            Iterator i;
            adbus_iter_args(i, d->msg);

            try {
                // A0 a0; ...
                DECL_ARGS;

                // if (a0 << i) {return -1;} ...
                DEMARSHALL_ARGS;

                // (o->*mf)( a0, a1, a2 ...);
                (o->*mf)( ARGS );

            } catch (Error& e) {
                adbus_error(d, e.name(), -1, e.message(), -1);
            }

            return 0;
        }

        template<class MF, class O, class R CLASS_DECLS_LC>
        int NUM(MFReturnCallback) (adbus_CbData* d)
        {
            MF mf = *(MF*) d->user1;
            O* o  =  (O*)  d->user2;

            Iterator i;
            adbus_iter_args(i, d->msg);

            try {
                // A0 a0; ...
                DECL_ARGS;

                // if (a0 << i) {return -1;} ...
                DEMARSHALL_ARGS;

                // (o->*mf)( a0, a1, a2 ...);
                R r = (o->*mf)( ARGS );

                if (d->ret) {
                    Buffer b;
                    b.b = adbus_msg_argbuffer(d->ret);

                    std::string type = adbus_type_string((R*) NULL);
                    adbus_buf_appendsig(b.b, type.c_str(), (int) type.size());
                    r >> b;
                }

            } catch (Error& e) {
                adbus_error(d, e.name(), -1, e.message(), -1);
            }

            return 0;
        }

        template<class MF, class O CLASS_DECLS_LC>
        int NUM(MFMatchCallback) (adbus_CbData* d)
        {
            MF mf = ((std::pair<MF,O*>*) d->user1)->first;
            O* o  = ((std::pair<MF,O*>*) d->user1)->second;

            Iterator i;
            adbus_iter_args(i, d->msg);

            try {
                // A0 a0; ...
                DECL_ARGS;

                // if (a0 << i) {return -1;} ...
                DEMARSHALL_ARGS;

                // (o->*mf)( a0, a1, a2 ...);
                (o->*mf)( ARGS );

            } catch (Error& e) {
                (void) e;
            }

            return 0;
        }

        TEMPLATE_CLASS_DECLS
        struct NUM(GetReturnsData)
        {
            // A0 a0; ...
            DECL_ARGS;
        };

        TEMPLATE_CLASS_DECLS inline
        int NUM(GetReturnsCallback) (adbus_CbData* d)
        {
            typedef NUM(GetReturnsData) SB_CLASS_NAMES Data;
            Data* o = (Data*) d->user1;
            (void) o;

            Iterator i = d->msg;

            try {
                // if (*(o->a0) << i) {return -1;}
                DEMARSHALL_POINTER_ARGS;

            } catch (Error& e) {
                (void) e;
            }

            return 0;
        }

    }

/* -------------------------------------------------------------------------- */
#elif ADBUSCPP_MULTI == ADBUSCPP_MULTI_MATCH

        template<CLASS_DECLS_TC class MF, class O>
        void NUM(setCallback) (MF function, O* object)
        {
            assert(!cuser);
            this->callback      = &NUM(detail::MFMatchCallback) <MF, O CLASS_NAMES_LC>;
            this->cuser         = CreateUser2<MF,O*>(function, object);
            this->release[0]    = &free;
            this->ruser[0]      = this->cuser;
        }


/* -------------------------------------------------------------------------- */
#elif ADBUSCPP_MULTI == ADBUSCPP_MULTI_CALL

        template<CLASS_DECLS_TC class MF, class O>
        Call& NUM(setCallback) (MF function, O* object)
        {
            assert(!cuser);
            this->callback      = &NUM(detail::MFMatchCallback) <MF, O CLASS_NAMES_LC>;
            this->cuser         = CreateUser2<MF, O*>(function, object);
            this->release[0]    = &free;
            this->ruser[0]      = this->cuser;
            return *this;
        }

        TEMPLATE_CLASS_DECLS
        void block(CONST_REF_ARGS)  // A0* a0, ...
        {
            assert(!cuser);
            if (NUM() > 0) {
                typedef NUM(detail::GetReturnsData) SB_CLASS_NAMES Data;

                this->callback  = &NUM(detail::GetReturnsCallback) SB_CLASS_NAMES;
                this->cuser     = CreateUser<Data>();

                Data* o = (Data*) this->cuser;
                (void) o;

                // o->a0 = a0;
                ASSIGN_ARGS;
            }

            adbus_call_block(this);
        }




/* -------------------------------------------------------------------------- */
#elif ADBUSCPP_MULTI == ADBUSCPP_MULTI_PROXY

        template<CLASS_DECLS_TC class MF, class O>
        void NUM(connect) (const char* signal, MF function, O* object)
        {
            Match m;
            m.NUM(setCallback) <CLASS_NAMES_TC MF, O> (function, object);
            adbus_proxy_signal(m_Proxy, &m, signal, -1);
        }
        
        TEMPLATE_CLASS_DECLS
        Call method(const char* method CONST_REF_ARGS_LC)
        {
            Call call;
            adbus_proxy_method(m_Proxy, &call, method, -1);

            Buffer b = adbus_msg_argbuffer(call.msg);

            // signature += adbus_type_string((A0*) NULL); ...
            std::string signature;
            APPEND_SIGS;
            adbus_buf_appendsig(b, signature.c_str(), (int) signature.size());

            // a0 >> b; ...
            APPEND_ARGS;

            return call;
        }

/* -------------------------------------------------------------------------- */
#elif ADBUSCPP_MULTI == ADBUSCPP_MULTI_CLASSES

    TEMPLATE_CLASS_DECLS
    class NUM(Signal)
    {
        ADBUSCPP_NOT_COPYABLE(NUM(Signal));
    public:
        NUM(Signal)(adbus_Member* mbr) : m_Signal(adbus_sig_new(mbr))
        {}

        ~NUM(Signal)() 
        { adbus_sig_free(m_Signal); }

        void bind(adbus_Connection* c, const char* p)
        { adbus_sig_bind(m_Signal, c, p, -1); }

        void bind(adbus_Connection* c, const std::string& p)
        { adbus_sig_bind(m_Signal, c, p.c_str(), (int) p.size()); }

        void bind(adbus_Connection* c, const Path& p)
        { adbus_sig_bind(m_Signal, c, p.c_str(), (int) p.size()); }

        void bind(const BindPath& p)
        { adbus_sig_bind(m_Signal, p.connection(), p.c_str(), (int) p.size()); }

        void trigger(CONST_REF_ARGS)
        {
            adbus_MsgFactory* m = adbus_sig_msg(m_Signal);

            Buffer b = adbus_msg_argbuffer(m);

            // a0 >> b; ...
            APPEND_ARGS;

            adbus_sig_emit(m_Signal);
        }

        void reset()
        { adbus_sig_reset(m_Signal); }

        void operator()(CONST_REF_ARGS)
        { trigger(ARGS); }

    private:
        adbus_Signal* m_Signal;
    };



/* -------------------------------------------------------------------------- */
#elif ADBUSCPP_MULTI == ADBUSCPP_MULTI_INTERFACES


        template<CLASS_DECLS_TC class MF>
        detail::MethodMember NUM(addMethod) (const std::string& name, MF function)
        {
            adbus_Member* mbr = adbus_iface_addmethod(m_I, name.c_str(), (int) name.size());

            adbus_MsgCallback callback = &NUM(detail::MFCallback) <MF, O CLASS_NAMES_LC>;
            void* cuser = CreateUser<MF>(function);
            adbus_mbr_setmethod(mbr, callback, cuser);
            adbus_mbr_addrelease(mbr, &free, cuser);

            std::string signature;
            // signature += adbus_type_string((A0*) NULL);
            APPEND_SIGS;
            adbus_mbr_argsig(mbr, signature.c_str(), (int) signature.size());

            return detail::MethodMember(mbr);
        }

        template<class R, CLASS_DECLS_TC class MF>
        detail::MethodMember NUM(addReturnMethod) (const std::string& name, MF function)
        {
            adbus_Member* mbr = adbus_iface_addmethod(m_I, name.c_str(), (int) name.size());

            adbus_MsgCallback callback = &NUM(detail::MFReturnCallback) <MF, O, R CLASS_NAMES_LC>;
            void* cuser = CreateUser<MF>(function);
            adbus_mbr_setmethod(mbr, callback, cuser);
            adbus_mbr_addrelease(mbr, &free, cuser);

            std::string signature;
            // signature += adbus_type_string((A0*) NULL);
            APPEND_SIGS;
            adbus_mbr_argsig(mbr, signature.c_str(), (int) signature.size());

            std::string retsig = adbus_type_string((R*) NULL);
            adbus_mbr_retsig(mbr, retsig.c_str(), (int) retsig.size());

            return detail::MethodMember(mbr);
        }

        TEMPLATE_CLASS_DECLS
        detail::SignalMember NUM(addSignal) (const std::string& name)
        {
            adbus_Member* mbr = adbus_iface_addsignal(m_I, name.c_str(), (int) name.size());

            std::string signature;
            // signature += adbus_type_string((A0*) NULL);
            APPEND_SIGS;
            adbus_mbr_argsig(mbr, signature.c_str(), (int) signature.size());

            return detail::SignalMember(mbr);
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

#undef DECL_ARG
#undef DECL_ARGS

#undef DEMARSHALL_POINTER_ARG
#undef DEMARSHALL_POINTER_ARGS

#undef DEMARSHALL_ARG
#undef DEMARSHALL_ARGS

#undef ASSIGN_ARG
#undef ASSIGN_ARGS

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

#undef NUM
#undef REPEAT

