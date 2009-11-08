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

#ifndef ADBUSCPP_H
#define ADBUSCPP_H

#include <adbus/adbus.h>

#include <map>
#include <string>
#include <vector>

#include <assert.h>


#define ADBUSCPP_MULTI_MATCH        0
#define ADBUSCPP_MULTI_PROXY        1
#define ADBUSCPP_MULTI_CLASSES      2
#define ADBUSCPP_MULTI_INTERFACES   3
#define ADBUSCPP_MULTI_BIND         4

#define ADBUSCPP_NOT_COPYABLE(x) x(const x&); x& operator=(const x&)


namespace adbus
{
    class Buffer
    {
    public:
        adbus_Buffer* b;

        operator adbus_Buffer*() const {return b;}
    };

    inline int operator>>(bool v, Buffer& b)
    { return adbus_buf_bool(b, v); }
    inline int operator>>(uint8_t v, Buffer& b)
    { return adbus_buf_uint8(b, v); }
    inline int operator>>(int16_t v, Buffer& b)
    { return adbus_buf_int16(b, v); }
    inline int operator>>(uint16_t v, Buffer& b)
    { return adbus_buf_uint16(b, v); }
    inline int operator>>(int32_t v, Buffer& b)
    { return adbus_buf_int32(b, v); }
    inline int operator>>(uint32_t v, Buffer& b)
    { return adbus_buf_uint32(b, v); }
    inline int operator>>(int64_t v, Buffer& b)
    { return adbus_buf_int64(b, v); }
    inline int operator>>(uint64_t v, Buffer& b)
    { return adbus_buf_uint64(b, v); }
    inline int operator>>(double v, Buffer& b)
    { return adbus_buf_double(b, v); }
    inline int operator>>(const std::string& s, Buffer& b)
    { return adbus_buf_string(b, s.c_str(), (int) s.size()); }

    template<class T>
    inline int operator>>(const std::vector<T>& v, Buffer& b)
    {
        if (adbus_buf_beginarray(b))
            return -1;
        for (size_t i = 0; i < v.size(); ++i) {
            if (v[i] >> b)
                return -1;
        }
        return adbus_buf_endarray(b);
    }

    template<class K, class V>
    inline int operator>>(const std::map<K,V>& map, Buffer& b)
    {
        if (adbus_buf_beginmap(b))
            return -1;
        typename std::map<K,V>::const_iterator ii;
        for (ii = map.begin(); ii != map.end(); ++ii) {
            if (ii->first >> b)
                return -1;
            if (ii->second >> b)
                return -1;
        }
        return adbus_buf_endmap(b);
    }



    struct MessageEnd{};

    class Iterator
    {
    public:
        struct adbus_Field f;
        adbus_Iterator* i;

        operator adbus_Iterator*() const {return i;}
    };

    inline int Iterate(Iterator& i, adbus_FieldType type)
    {
        if (adbus_iter_next(i, &i.f) || i.f.type != type)
            return -1;
        return 0;
    }

    template<class T, class F>
    inline int Iterate(Iterator& i, adbus_FieldType type, T& to, const F& from)
    {
        if (adbus_iter_next(i, &i.f) || i.f.type != type)
            return -1;
        to = from;
        return 0;
    }

    inline int operator<<(bool& v, Iterator& i)
    { return Iterate(i, ADBUS_BOOLEAN, v, i.f.b); }
    inline int operator<<(uint8_t& v, Iterator& i)
    { return Iterate(i, ADBUS_UINT8, v, i.f.u8);}
    inline int operator<<(int16_t& v, Iterator& i)
    { return Iterate(i, ADBUS_INT16, v, i.f.i16);}
    inline int operator<<(uint16_t& v, Iterator& i)
    { return Iterate(i, ADBUS_UINT16, v, i.f.u16);}
    inline int operator<<(int32_t& v, Iterator& i)
    { return Iterate(i, ADBUS_INT32, v, i.f.i32);}
    inline int operator<<(uint32_t& v, Iterator& i)
    { return Iterate(i, ADBUS_UINT32, v, i.f.u32);}
    inline int operator<<(int64_t& v, Iterator& i)
    { return Iterate(i, ADBUS_INT64, v, i.f.i64);}
    inline int operator<<(uint64_t& v, Iterator& i)
    { return Iterate(i, ADBUS_UINT64, v, i.f.u64);}
    inline int operator<<(double& v, Iterator& i)
    { return Iterate(i, ADBUS_DOUBLE, v, i.f.d);}
    inline int operator<<(std::string& v, Iterator& i)
    { return Iterate(i, ADBUS_STRING, v, i.f.string);}
    inline int operator<<(MessageEnd& v, Iterator& i)
    { (void) v; return Iterate(i, ADBUS_END_FIELD); }

    template<class T>
    inline int operator<<(std::vector<T>& v, Iterator& i)
    {
        int scope;
        if (Iterate(i, ADBUS_ARRAY_BEGIN, scope, i.f.scope))
            return -1;
        while (!adbus_iter_isfinished(i, scope)) {
            v.resize(v.size() + 1);
            if (v[v.size() - 1] << i)
                return -1;
        }
        return Iterate(i, ADBUS_ARRAY_END);
    }

    template<class K, class V>
    inline int operator<<(std::map<K,V>& map, Iterator& i)
    {
        int scope;
        if (Iterate(i, ADBUS_MAP_BEGIN, scope, i.f.scope))
            return -1;
        while (!adbus_iter_isfinished(i, scope)) {
            K key;
            V val;
            if (key << i || val << i)
                return -1;
            map[key] = val;
        }
        return Iterate(i, ADBUS_MAP_END);
    }

    template<class T>
    class Array
    {
    public:
        Array() : data(NULL), size(0) {}

        int operator>>(Buffer& b) const
        {
            if (adbus_buf_beginarray(b))
                return -1;
            if (adbus_buf_appenddata(b, (const uint8_t*) data, size))
                return -1;
            return adbus_buf_endarray(b);
        }

        int operator<<(Iterator& i)
        {
            int scope;
            if (Iterate(i, ADBUS_ARRAY_BEGIN, scope, i.f.scope))
                return -1;
            data = i.f.data;
            size = i.f.size / sizeof(T);
            if (adbus_iter_arrayjump(i, scope))
                return -1;
            return Iterate(i, ADBUS_ARRAY_END);
        }

        const T*    data;
        size_t      size;
    };


    class Variant
    {
        ADBUSCPP_NOT_COPYABLE(Variant);
    public:
        Variant() : m_Iter(adbus_iter_new()), m_Buf(adbus_buf_new()) {}

        ~Variant() { adbus_iter_free(m_Iter); adbus_buf_free(m_Buf); }

        int operator>>(Buffer& b) const
        { 
            const char* sig;
            size_t sigsize;
            adbus_buf_get(m_Buf, &sig, &sigsize, NULL, NULL);
            if (adbus_buf_beginvariant(b, sig, sigsize))
                return -1;
            if (adbus_buf_copy(b, iterator().i, 0))
                return -1;
            return adbus_buf_endvariant(b);
        }

        int operator<<(Iterator& i)
        { 
            adbus_buf_reset(m_Buf);
            if (Iterate(i, ADBUS_VARIANT_BEGIN))
                return -1;
            if (adbus_buf_append(m_Buf, i.f.string, i.f.size))
                return -1;
            if (adbus_buf_copy(m_Buf, i, i.f.scope))
                return -1;
            return Iterate(i, ADBUS_VARIANT_END);
        }

        Iterator iterator() const
        {
            const uint8_t* data;
            const char* sig;
            size_t sigsize, datasize;
            adbus_buf_get(m_Buf, &sig, &sigsize, &data, &datasize);
            adbus_iter_reset(m_Iter, sig, sigsize, data, datasize);
            Iterator i;
            i.i = m_Iter;
            return i;
        }

        Buffer buffer()
        { 
            Buffer b;
            b.b = m_Buf;
            return b;
        }

    private:
        mutable adbus_Iterator* m_Iter;
        adbus_Buffer*           m_Buf;
    };

    template<class T> inline
    int operator>>(const T& t, Variant& v)
    { return t >> v.buffer(); }



    class User : public adbus_User
    {
    public:
        User() {free = &Free;}
        virtual ~User() {}
    private:
        static void Free(adbus_User* user)
        { delete (User*) user;}
    };

    template<class T>
    class TUser : public User
    {
    public:
        TUser(const T& d) : data(d) {}

        T data;
    };

    template<class T> inline
    TUser<T>* CreateUser(const T& data)
    { return new TUser<T>(data); }

    template<class T> inline 
    const T& GetUser(const adbus_User* user)
    { return ((TUser<T>*) user)->data; }

    class Error
    {
    public:
        Error(const char* name, const char* msg) : m_Name(name), m_Message(msg) {}
        virtual const char* name() const {return m_Name.c_str();}
        virtual const char* message() const {return m_Message.c_str();}
    private:
        std::string m_Name;
        std::string m_Message;
    };

#ifndef DOC
#define ADBUSCPP_MULTI ADBUSCPP_MULTI_BIND
#include "adbuscpp-multi.h"
#endif

    class Match : public adbus_Match
    {
    public:
        Match() {reset();}

        void reset() {adbus_match_init(this);}

#ifdef DOC
        template<class A0 ..., class MF, class O>
        void setCallbackX(O* o, MF mf);
#else
#define ADBUSCPP_MULTI ADBUSCPP_MULTI_MATCH
#include "adbuscpp-multi.h"
#endif
    };


    namespace detail
    {
        template<class MF, class O, class T>
        static int GetCallback(adbus_CbData* d)
        {
            MF mf = GetUser<MF>(d->user1);
            O* o  = GetUser<O*>(d->user2);
            try {
                T t = (o->*mf)();

                struct Buffer b;
                b.b = d->propertyMarshaller;
                t >> b; 
            } catch (Error& e) {
                adbus_setup_error(d, e.name(), -1, e.message(), -1);
            }
            return 0;
        }

        template<class MF, class O, class T>
        static int SetCallback(adbus_CbData* d)
        {
            MF mf = GetUser<MF>(d->user1);
            O* o  = GetUser<O*>(d->user2);
            try {
                Iterator i;
                i.i = d->propertyIterator;

                T t;
                if (t << i)
                    return -1;

                (o->*mf)(t);

            } catch (Error& e) {
                adbus_setup_error(d, e.name(), -1, e.message(), -1);
            }
            return 0;
        }
    }


    template<class O, class T>
    class PropertyMember 
    {
    public:
        PropertyMember(adbus_Member* m):m_M(m){}

        PropertyMember<O,T>& addAnnotation(const std::string& name, const std::string& value) 
        {
            adbus_mbr_addannotation(m_M, name.c_str(), (int) name.size(), value.c_str(), (int) value.size());
            return *this;
        }

        template<class MF>
        PropertyMember<O,T>& setGetter(MF function)
        {
            adbus_mbr_setgetter(
                    m_M,
                    &detail::GetCallback<MF,O,T>,
                    CreateUser<MF>(function));
            return *this;
        }

        template<class MF>
        PropertyMember<O,T>& setSetter(MF function)
        {
            adbus_mbr_setsetter(
                    m_M,
                    &detail::SetCallback<MF,O,T>,
                    CreateUser<MF>(function));
            return *this;
        }

    private:
        adbus_Member* m_M;
    };



#ifdef DOC

    template<class A0...>
    class SignalX
    {
        ADBUSCPP_NOT_COPABLE(SignalX<A0...>);
    public:
        SignalX();

        void bind(adbus_Path* path, adbus_Member* mbr);

        void trigger(const A0& a0...);
    private:
        adbus_Signal*       m_Signal;
    };


    template<class A0...>
    class SignalMemberX
    {
    public:
        SignalMemberX(adbus_Member* m);

        SignalMemberX<A0...>& addAnnotation(const std::string& name, const std::string& value);
        SignalMemberX<A0...>& addArgument(const std::string& name);

    private:
        adbus_Member* m_M;
        int m_Args;
    };


    template<class R, class A0...>
    class MethodMemberX
    {
    public:
        MethodMemberX(adbus_Member* m);
        ~MethodMember();

        MethodMemberX<R,A0...>& addAnnotation(const std::string& name, const std::string& value);
        MethodMemberX<R,A0...>& addArgument(const std::string& name);
        MethodMemberX<R,A0...>& addReturn(const std::string& name);

    private:
        adbus_Member* m_M;
        int m_Args;
        int m_Rets;
    };

#else
#define ADBUSCPP_MULTI ADBUSCPP_MULTI_CLASSES
#include "adbuscpp-multi.h"
#endif



    template<class O>
    class Interface
    {
        ADBUSCPP_NOT_COPYABLE(Interface);
    public:
        Interface(const std::string& name) : m_I(adbus_iface_new(name.c_str(), (int) name.size())) {}
        ~Interface() {adbus_iface_free(m_I);}

#ifdef DOC
        template<class A0...>
        MethodMemberX<A0...> addMethodX(const std::string& name);

        template<class A0...>
        SignalMemberX<A0...> addSignalX(const std::string& name);
#else
#define ADBUSCPP_MULTI ADBUSCPP_MULTI_INTERFACES
#include "adbuscpp-multi.h"
#endif

        template<class T>
        PropertyMember<O, T> addProperty(const std::string& name)
        {
            std::string type = adbus_type_string((T*) NULL);
            return PropertyMember<O, T>(adbus_iface_addproperty(
                        m_I,
                        name.c_str(), (int) name.size(),
                        type.c_str(), (int) type.size()));
        }

        adbus_Member* property(const std::string& name) {return adbus_iface_property(m_I, name.c_str(), (int) name.size());}
        adbus_Member* signal(const std::string& name) {return adbus_iface_signal(m_I, name.c_str(), (int) name.size());}
        adbus_Member* method(const std::string& name) {return adbus_iface_method(m_I, name.c_str(), (int) name.size());}

        operator adbus_Interface*() const {return m_I;}
    private:
        adbus_Interface* m_I;
    };

    class Connection;

    class Path
    {
    public:
        Path()
            : m_P(NULL) {}

        Path(adbus_Connection* c, const std::string& p)
            : m_P(adbus_conn_path(c, p.c_str(), (int) p.size())) {}

        Path(adbus_Path* p) 
            : m_P(p) {}

        template<class O>
        void bind(Interface<O>* i, O* object)
        {adbus_path_bind(m_P, *i, CreateUser<O*>(object));}

        std::string string() const
        {return std::string(m_P->string, m_P->string + m_P->size);}

        const char* c_str() const
        {return m_P->string;}

        Connection connection() const;

        bool isValid() const
        {return m_P != NULL;}

        Path operator/(const std::string& p) const
        {return adbus_path_relative(m_P, p.c_str(), (int) p.size());}

        operator adbus_Path*() const
        {return m_P;}

    private:
        adbus_Path* m_P;
    };


    class Connection
    {
    public:
        Connection() : m_C(adbus_conn_new()), m_Free(true) {}
        Connection(adbus_Connection* c) : m_C(c), m_Free(false) {}
        ~Connection() {if (m_Free) adbus_conn_free(m_C);}

        uint32_t matchId() const {return adbus_conn_matchid(m_C);}
        uint32_t addMatch(const adbus_Match* m) {return adbus_conn_addmatch(m_C, m);}
        void removeMatch(uint32_t id) {adbus_conn_removematch(m_C, id);}

        uint32_t serial() const {return adbus_conn_serial(m_C);}

        Path path(const std::string& p) const {return Path(m_C, p);}

        void setSender(adbus_SendCallback callback, adbus_User* data)
        {adbus_conn_setsender(m_C, callback, data);}

        int parse(const uint8_t* data, size_t size)
        {return adbus_conn_parse(m_C, data, size);}

        int dispatch(adbus_Message* msg)
        {return adbus_conn_dispatch(m_C, msg);}

        void connectToBus(adbus_ConnectCallback callback = NULL, adbus_User* data = NULL)
        {adbus_conn_connect(m_C, callback, data);}

        uint32_t requestName(const std::string& name, uint32_t flags = 0, adbus_NameCallback callback = NULL, adbus_User* user = NULL)
        {return adbus_conn_requestname(m_C, name.c_str(), (int) name.size(), flags, callback, user);}

        uint32_t releaseName(const std::string& name, adbus_NameCallback callback, adbus_User* user)
        {return adbus_conn_releasename(m_C, name.c_str(), (int) name.size(), callback, user);}

        operator adbus_Connection*(){return m_C;}
    private:
        adbus_Connection* m_C;
        bool m_Free;
    };

    inline Connection Path::connection() const
    {return Connection(m_P->connection);}

    class Object
    {
        ADBUSCPP_NOT_COPYABLE(Object);
    public:
        Object() : m_O(adbus_obj_new()){}
        ~Object() {adbus_obj_free(m_O);}

        template<class O>
        int bind(adbus_Path* p, Interface<O>* i, O* object) {return adbus_obj_bind(m_O, p, *i, CreateUser<O*>(object));} 
        int unbind(adbus_Path* p, adbus_Interface* i) {return adbus_obj_unbind(m_O, p, i);}

        void addMatchId(adbus_Connection* c, uint32_t id) {return adbus_obj_addmatchid(m_O, c, id);}
        uint32_t addMatch(adbus_Connection* c, const adbus_Match* m) {return adbus_obj_addmatch(m_O, c, m);}
        void removeMatch(adbus_Connection* c, uint32_t id) {return adbus_obj_removematch(m_O, c, id);}

        operator adbus_Object*(){return m_O;}
    private:
        adbus_Object* m_O;
    };

    class Proxy
    {
        ADBUSCPP_NOT_COPYABLE(Proxy);
    public:
        Proxy(Connection& c, const std::string& service, const std::string& path, const std::string& interface = std::string())
        { 
            m_User1 = m_User2 = NULL;
            if (!interface.empty()) {
                m_Proxy = adbus_proxy_new(c,
                        service.c_str(), (int) service.size(),
                        path.c_str(), (int) path.size(),
                        interface.c_str(), (int) interface.size());
            } else {
                m_Proxy = adbus_proxy_new(c,
                        service.c_str(), (int) service.size(),
                        path.c_str(), (int) path.size(),
                        NULL, 0);
            }
        }

        ~Proxy()
        { 
            adbus_proxy_free(m_Proxy);
            adbus_user_free(m_User1);
            adbus_user_free(m_User2);
            adbus_user_free(m_ErrorUser1);
            adbus_user_free(m_ErrorUser2);
        }

        template<class MF, class O>
        void setErrorCallback(O* o, MF mf)
        {
            detail::CreateMFCallback1<std::string>(mf, o, &m_ErrorCallback, &m_ErrorUser1, &m_ErrorUser2);
        }

#ifdef DOC
        template<class A0 ..., class MF, class O>
        void setCallbackX(O* o, MF mf);

        template<class A0 ...>
        uint32_t call(const std::string& member, const A0& a0 ...);
#else
#define ADBUSCPP_MULTI ADBUSCPP_MULTI_PROXY
#include "adbuscpp-multi.h"
#endif
    private:
        adbus_Proxy*        m_Proxy;
        adbus_Callback      m_Callback;
        adbus_User*         m_User1;
        adbus_User*         m_User2;
        adbus_Callback      m_ErrorCallback;
        adbus_User*         m_ErrorUser1;
        adbus_User*         m_ErrorUser2;
    };







}


inline const char* adbus_type_string(bool*)    {return "b";}
inline const char* adbus_type_string(uint8_t*) {return "y";}
inline const char* adbus_type_string(int16_t*) {return "n";}
inline const char* adbus_type_string(uint16_t*){return "q";}
inline const char* adbus_type_string(int32_t*) {return "i";}
inline const char* adbus_type_string(uint32_t*){return "u";}
inline const char* adbus_type_string(int64_t*) {return "x";}
inline const char* adbus_type_string(uint64_t*){return "t";}
inline const char* adbus_type_string(double*)  {return "d";}
inline const char* adbus_type_string(std::string*)   {return "s";}
inline const char* adbus_type_string(adbus::Variant*){return "v";}

template<class T> inline
std::string adbus_type_string(std::vector<T>*)
{return std::string("a") + adbus_type_string((T*) NULL);}

template<class K, class V> inline
std::string adbus_type_string(std::map<K,V>*)
{return std::string("a{") + adbus_type_string((K*) NULL) + adbus_type_string((V*) NULL) + "}";}

template<class T> inline
std::string adbus_type_string(adbus::Array<T>*)
{return std::string("a") + adbus_type_string((T*) NULL);}




#endif /* ADBUSCPP_H */
