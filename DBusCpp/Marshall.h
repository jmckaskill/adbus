// vim: ts=2 sw=2 sts=2 et
//
// Copyright (c) 2009 James R. McKaskill
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// ----------------------------------------------------------------------------

#pragma once

#include "Macros.h"

#include "DBusClient/Marshaller.h"
#include "DBusClient/Message.h"

#include <string>
#include <vector>

// ----------------------------------------------------------------------------

namespace DBus{
  void CheckForError(int err);
}

// ----------------------------------------------------------------------------

template<class T>
struct DBusBaseArgumentType
{
  typedef T type;
};

#define DBUSCPP_DECLARE_BASE_TYPE(from, to) \
  template<> struct DBusBaseArgumentType<from>  \
  { typedef to type; };

// ----------------------------------------------------------------------------

template<class T>
const char* DBusTypeString();

#define DBUSCPP_DECLARE_TYPE_STRING(type, string) \
  DBUSCPP_DECLARE_BASE_TYPE(type const&, type) \
  template<> inline \
  const char* DBusTypeString<type>() \
  { return string; }

DBUSCPP_DECLARE_TYPE_STRING(bool,         "b")
DBUSCPP_DECLARE_TYPE_STRING(uint8_t,      "y")
DBUSCPP_DECLARE_TYPE_STRING( int16_t,     "n")
DBUSCPP_DECLARE_TYPE_STRING(uint16_t,     "q")
DBUSCPP_DECLARE_TYPE_STRING( int32_t,     "i")
DBUSCPP_DECLARE_TYPE_STRING(uint32_t,     "u")
DBUSCPP_DECLARE_TYPE_STRING( int64_t,     "x")
DBUSCPP_DECLARE_TYPE_STRING(uint64_t,     "t")
DBUSCPP_DECLARE_TYPE_STRING(double,       "d")
DBUSCPP_DECLARE_TYPE_STRING(const char*,  "s")
DBUSCPP_DECLARE_TYPE_STRING(std::string,  "s")

// ----------------------------------------------------------------------------

namespace DBus{

  template<class T>
  void BeginArgument(struct DBusMarshaller* m)
  { DBusBeginArgument(m, DBusTypeString<T>(), -1); }

  template<class T>
  void EndArgument(struct DBusMarshaller* m)
  { DBusEndArgument(m); }

  template<> inline
  void BeginArgument<Null_t>(struct DBusMarshaller* m)
  {}

  template<> inline
  void EndArgument<Null_t>(struct DBusMarshaller* m)
  {}

}

// ----------------------------------------------------------------------------

inline void operator<<(::DBus::Null_t, DBusMessage&)
{}

inline void operator<<(uint8_t& data, DBusMessage& m)
{ DBus::CheckForError(DBusTakeUInt8(&m, &data)); }

inline void operator<<(int16_t& data, DBusMessage& m)
{ DBus::CheckForError(DBusTakeInt16(&m, &data)); }

inline void operator<<(uint16_t& data, DBusMessage& m)
{ DBus::CheckForError(DBusTakeUInt16(&m, &data)); }

inline void operator<<(int32_t& data, DBusMessage& m)
{ DBus::CheckForError(DBusTakeInt32(&m, &data)); }

inline void operator<<(uint32_t& data, DBusMessage& m)
{ DBus::CheckForError(DBusTakeUInt32(&m, &data)); }

inline void operator<<(int64_t& data, DBusMessage& m)
{ DBus::CheckForError(DBusTakeInt64(&m, &data)); }

inline void operator<<(uint64_t& data, DBusMessage& m)
{ DBus::CheckForError(DBusTakeUInt64(&m, &data)); }

inline void operator<<(double& data, DBusMessage& m)
{ DBus::CheckForError(DBusTakeDouble(&m, &data)); }

inline void operator<<(const char*& str, DBusMessage& m)
{ DBus::CheckForError(DBusTakeString(&m, &str, NULL)); }

inline void operator<<(std::string& str, DBusMessage& m)
{ 
  const char* cstr;
  int size;
  DBus::CheckForError(DBusTakeString(&m, &cstr, &size)); 
  str.clear();
  str.insert(str.end(), cstr, cstr + size);
}

template<class T>
void operator<<(std::vector<T>& vector, DBusMessage& m)
{
  unsigned int scope;
  int err = DBusTakeArrayBegin(&m, &scope, NULL);
  vector.clear();
  while(!err && !DBusIsScopeAtEnd(&m,scope))
  {
    vector.resize(vector.size() + 1);
    vector[vector.size() - 1] << m;
  }
  err = err || DBusTakeArrayEnd(&m);
  DBus::CheckForError(err);
}

// ----------------------------------------------------------------------------

inline void operator>>(::DBus::Null_t, DBusMarshaller&)
{}

inline void operator>>(bool b, DBusMarshaller& m)
{ DBusAppendBoolean(&m, b ? 1 : 0); }

inline void operator>>(uint8_t data, DBusMarshaller& m)
{ DBusAppendUInt8(&m, data); }

inline void operator>>(int16_t data, DBusMarshaller& m)
{ DBusAppendInt16(&m, data); }

inline void operator>>(uint16_t data, DBusMarshaller& m)
{ DBusAppendUInt16(&m, data); }

inline void operator>>(int32_t data, DBusMarshaller& m)
{ DBusAppendInt32(&m, data); }

inline void operator>>(uint32_t data, DBusMarshaller& m)
{ DBusAppendUInt32(&m, data); }

inline void operator>>(int64_t data, DBusMarshaller& m)
{ DBusAppendInt64(&m, data); }

inline void operator>>(uint64_t data, DBusMarshaller& m)
{ DBusAppendUInt64(&m, data); }

inline void operator>>(double data, DBusMarshaller& m)
{ DBusAppendDouble(&m, data); }

inline void operator>>(const char* str, DBusMarshaller& m)
{ DBusAppendString(&m, str, -1); }

inline void operator>>(const std::string& str, DBusMarshaller& m)
{ DBusAppendString(&m, str.c_str(), str.size()); }

// ----------------------------------------------------------------------------

