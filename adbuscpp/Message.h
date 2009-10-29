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

#include "adbus/Iterator.h"
#include "adbus/Marshaller.h"
#include "adbus/User.h"
#include <exception>
#include <string>
#include <map>
#include <vector>

// ----------------------------------------------------------------------------

namespace adbus
{
    class Variant;

    // ----------------------------------------------------------------------------

    class Error : public std::exception
    {
    public:
        Error()
        {}

        Error(const char* name, const char* message)
            : m_Name(name), m_Message(message)
        {}

        virtual ~Error() throw()
        {}

        virtual const char* errorName()const
        { return m_Name.c_str(); }
        virtual const char* errorMessage()const
        { return m_Message.c_str(); }

        virtual const char* what() const throw()
        { return errorMessage(); }
    private:
        std::string m_Name;
        std::string m_Message;
    };

    class MarshallError : public std::exception
    {
        virtual const char* what() const throw()
        { return "ADBus marshall error"; }
    };

    class ParseError : public std::exception
    {
    public:
        ParseError(int err):parseError(err){}
        virtual const char* what() const throw()
        { return "ADBus parse error"; }
        int parseError;
    };

    class InvalidArgument : public Error
    {
        virtual const char* errorName() const
        { return "nz.co.foobar.ADBus.InvalidArgument"; }
        virtual const char* errorMessage() const
        { return "Invalid arguments passed to a method call."; }
    };

    // ----------------------------------------------------------------------------

    inline void CheckForMarshallError(int err)
    {
        if (err)
            throw MarshallError();
    }
    inline void Iterate(struct ADBusIterator* i, struct ADBusField* field)
    {
        int err = ADBusIterate(i, field);
        if (err)
            throw ParseError(err);
    }
    inline void Iterate(struct ADBusIterator* i, struct ADBusField* field, enum ADBusFieldType expectedType)
    {
        int err = ADBusIterate(i, field);
        if (err)
            throw ParseError(err);
        if (field->type != expectedType)
            throw InvalidArgument();
    }

    template<class T>
    void AppendArgument(struct ADBusMarshaller* m, const T& t)
    {
        std::string type = ADBusTypeString((T*) NULL);
        ADBusAppendArguments(m, type.c_str(), type.size());
        t >> *m;
    }

    // ----------------------------------------------------------------------------

    class UserDataBase : public ADBusUser
    {
    public:
        UserDataBase();
        virtual ~UserDataBase(){}

        ADBusMessageCallback    chainedFunction;
    };

    template<class T>
    class UserData : public UserDataBase
    {
    public:
        UserData(const T& d)
        : data(d)
        {}

        T data;
    };

    // ----------------------------------------------------------------------------

    template<class T>
    struct ArrayReference
    {
      size_t    size;
      const T*  data;
    };

    // ----------------------------------------------------------------------------

    struct MessageEnd
    {
    };

    // ----------------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
// Type strings
// ----------------------------------------------------------------------------

#define ADBUSCPP_DECLARE_TYPE_STRING(TYPE, STRING)  \
    inline std::string ADBusTypeString(TYPE*)       \
    { return STRING; }

ADBUSCPP_DECLARE_TYPE_STRING(bool,         "b")
ADBUSCPP_DECLARE_TYPE_STRING(uint8_t,      "y")
ADBUSCPP_DECLARE_TYPE_STRING( int16_t,     "n")
ADBUSCPP_DECLARE_TYPE_STRING(uint16_t,     "q")
ADBUSCPP_DECLARE_TYPE_STRING( int32_t,     "i")
ADBUSCPP_DECLARE_TYPE_STRING(uint32_t,     "u")
ADBUSCPP_DECLARE_TYPE_STRING( int64_t,     "x")
ADBUSCPP_DECLARE_TYPE_STRING(uint64_t,     "t")
ADBUSCPP_DECLARE_TYPE_STRING(double,       "d")
ADBUSCPP_DECLARE_TYPE_STRING(const char*,  "s")
ADBUSCPP_DECLARE_TYPE_STRING(std::string,  "s")
ADBUSCPP_DECLARE_TYPE_STRING(adbus::Variant,  "v")

template<class T>
std::string ADBusTypeString(std::vector<T>*)
{ return "a" + ADBusTypeString((T*) NULL); }

template<class K, class V>
std::string ADBusTypeString(std::map<K,V>*)
{ return "a{" + ADBusTypeString((K*) NULL) + ADBusTypeString((V*) NULL) + "}"; }

template<class T>
std::string ADBusTypeString(adbus::ArrayReference<T>*)
{ return "a" + ADBusTypeString((T*) NULL); }

// ----------------------------------------------------------------------------
// Marshallers
// ----------------------------------------------------------------------------

inline void operator>>(bool b, ADBusMarshaller& m)
{ adbus::CheckForMarshallError(ADBusAppendBoolean(&m, b ? 1 : 0)); }

inline void operator>>(uint8_t data, ADBusMarshaller& m)
{ adbus::CheckForMarshallError(ADBusAppendUInt8(&m, data)); }

inline void operator>>(int16_t data, ADBusMarshaller& m)
{ adbus::CheckForMarshallError(ADBusAppendInt16(&m, data)); }

inline void operator>>(uint16_t data, ADBusMarshaller& m)
{ adbus::CheckForMarshallError(ADBusAppendUInt16(&m, data)); }

inline void operator>>(int32_t data, ADBusMarshaller& m)
{ adbus::CheckForMarshallError(ADBusAppendInt32(&m, data)); }

inline void operator>>(uint32_t data, ADBusMarshaller& m)
{ adbus::CheckForMarshallError(ADBusAppendUInt32(&m, data)); }

inline void operator>>(int64_t data, ADBusMarshaller& m)
{ adbus::CheckForMarshallError(ADBusAppendInt64(&m, data)); }

inline void operator>>(uint64_t data, ADBusMarshaller& m)
{ adbus::CheckForMarshallError(ADBusAppendUInt64(&m, data)); }

inline void operator>>(double data, ADBusMarshaller& m)
{ adbus::CheckForMarshallError(ADBusAppendDouble(&m, data)); }

inline void operator>>(const char* str, ADBusMarshaller& m)
{ adbus::CheckForMarshallError(ADBusAppendString(&m, str, -1)); }

inline void operator>>(const std::string& str, ADBusMarshaller& m)
{ adbus::CheckForMarshallError(ADBusAppendString(&m, str.c_str(), (int) str.size())); }

template<class T>
void operator>>(const std::vector<T>& vec, ADBusMarshaller& m)
{
    adbus::CheckForMarshallError(ADBusBeginArray(&m));
    for (size_t i = 0; i < vec.size(); ++i)
    {
        vec[i] >> m;
    }
    adbus::CheckForMarshallError(ADBusEndArray(&m));
}

template<class K, class V>
void operator>>(const std::map<K,V>& Map, ADBusMarshaller& m)
{
    adbus::CheckForMarshallError(ADBusBeginArray(&m));
    typedef typename std::map<K,V>::const_iterator iterator;
    iterator mi;
    for (mi = Map.begin(); mi != Map.end(); ++mi)
    {
        adbus::CheckForMarshallError(ADBusBeginDictEntry(&m));
        mi->first >> m;
        mi->second >> m;
        adbus::CheckForMarshallError(ADBusEndDictEntry(&m));
    }
    adbus::CheckForMarshallError(ADBusEndArray(&m));
}

template<class T>
void operator>>(const adbus::ArrayReference<T>& array, ADBusMarshaller& m)
{
    adbus::CheckForMarshallError(ADBusBeginArray(&m));
    ADBusAppendData(&m, (const uint8_t*) array.data, array.size * sizeof(T));
    adbus::CheckForMarshallError(ADBusEndArray(&m));
}


// ----------------------------------------------------------------------------
// Demarshallers
// ----------------------------------------------------------------------------

void operator<<(bool& data, ADBusIterator& i);
void operator<<(uint8_t& data, ADBusIterator& i);
void operator<<(int16_t& data, ADBusIterator& i);
void operator<<(uint16_t& data, ADBusIterator& i);
void operator<<(int32_t& data, ADBusIterator& i);
void operator<<(uint32_t& data, ADBusIterator& i);
void operator<<(int64_t& data, ADBusIterator& i);
void operator<<(uint64_t& data, ADBusIterator& i);
void operator<<(double& data, ADBusIterator& i);
void operator<<(const char*& str, ADBusIterator& i);
void operator<<(std::string& str, ADBusIterator& i);
void operator<<(adbus::MessageEnd& end, ADBusIterator& i);

template<class T>
void operator<<(std::vector<T>& vector, ADBusIterator& i)
{
    struct ADBusField field;
    adbus::Iterate(&i, &field, ADBusArrayBeginField);

    vector.clear();
    while(!ADBusIsScopeAtEnd(&i,field.scope))
    {
        vector.resize(vector.size() + 1);
        vector[vector.size() - 1] << i;
    }

    adbus::Iterate(&i, &field, ADBusArrayEndField);
}

template<class T>
void operator<<(adbus::ArrayReference<T>& array, ADBusIterator& i)
{
    struct ADBusField field;
    adbus::Iterate(&i, &field, ADBusArrayBeginField);

    array.size = field.size / sizeof(T);
    array.data = ADBusCurrentIteratorData(&i, NULL);

    int err = ADBusJumpToEndOfArray(&i, field.scope);
    if (err)
        throw adbus::ParseError(err);

    adbus::Iterate(&i, &field, ADBusArrayEndField);
}

