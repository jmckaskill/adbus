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

#include "adbus/Marshaller.h"
#include "adbus/Message.h"

#include <string>
#include <vector>

// ----------------------------------------------------------------------------

namespace adbus{
  void CheckForError(int err);
}

// ----------------------------------------------------------------------------

template<class T>
struct ADBusBaseArgumentType
{
  typedef T type;
};

#define ADBUSCPP_DECLARE_BASE_TYPE(from, to) \
  template<> struct ADBusBaseArgumentType<from>  \
  { typedef to type; };

// ----------------------------------------------------------------------------

template<class T>
const char* ADBusTypeString();

#define ADBUSCPP_DECLARE_TYPE_STRING(type, string) \
  template<> inline \
  const char* ADBusTypeString<type>() \
  { return string; }

ADBUSCPP_DECLARE_TYPE_STRING(adbus::Null_t, NULL)
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
ADBUSCPP_DECLARE_BASE_TYPE(const std::string&, std::string)


// ----------------------------------------------------------------------------

namespace adbus{

  template<class T>
  void BeginArgument(struct ADBusMarshaller* m)
  { ADBusBeginArgument(m, ADBusTypeString<T>(), -1); }

  template<class T>
  void EndArgument(struct ADBusMarshaller* m)
  { ADBusEndArgument(m); }

  template<> inline
  void BeginArgument<Null_t>(struct ADBusMarshaller* m)
  {}

  template<> inline
  void EndArgument<Null_t>(struct ADBusMarshaller* m)
  {}

}

// ----------------------------------------------------------------------------

inline void operator<<(::adbus::Null_t, ADBusMessage&)
{}

inline void operator<<(uint8_t& data, ADBusMessage& m)
{ adbus::CheckForError(ADBusTakeUInt8(&m, &data)); }

inline void operator<<(int16_t& data, ADBusMessage& m)
{ adbus::CheckForError(ADBusTakeInt16(&m, &data)); }

inline void operator<<(uint16_t& data, ADBusMessage& m)
{ adbus::CheckForError(ADBusTakeUInt16(&m, &data)); }

inline void operator<<(int32_t& data, ADBusMessage& m)
{ adbus::CheckForError(ADBusTakeInt32(&m, &data)); }

inline void operator<<(uint32_t& data, ADBusMessage& m)
{ adbus::CheckForError(ADBusTakeUInt32(&m, &data)); }

inline void operator<<(int64_t& data, ADBusMessage& m)
{ adbus::CheckForError(ADBusTakeInt64(&m, &data)); }

inline void operator<<(uint64_t& data, ADBusMessage& m)
{ adbus::CheckForError(ADBusTakeUInt64(&m, &data)); }

inline void operator<<(double& data, ADBusMessage& m)
{ adbus::CheckForError(ADBusTakeDouble(&m, &data)); }

inline void operator<<(const char*& str, ADBusMessage& m)
{ adbus::CheckForError(ADBusTakeString(&m, &str, NULL)); }

inline void operator<<(std::string& str, ADBusMessage& m)
{
  const char* cstr;
  int size;
  adbus::CheckForError(ADBusTakeString(&m, &cstr, &size));
  str.clear();
  str.insert(str.end(), cstr, cstr + size);
}

template<class T>
void operator<<(std::vector<T>& vector, ADBusMessage& m)
{
  unsigned int scope;
  int err = ADBusTakeArrayBegin(&m, &scope, NULL);
  vector.clear();
  while(!err && !ADBusIsScopeAtEnd(&m,scope))
  {
    vector.resize(vector.size() + 1);
    vector[vector.size() - 1] << m;
  }
  err = err || ADBusTakeArrayEnd(&m);
  adbus::CheckForError(err);
}

// ----------------------------------------------------------------------------

inline void operator>>(::adbus::Null_t, ADBusMarshaller&)
{}

inline void operator>>(bool b, ADBusMarshaller& m)
{ ADBusAppendBoolean(&m, b ? 1 : 0); }

inline void operator>>(uint8_t data, ADBusMarshaller& m)
{ ADBusAppendUInt8(&m, data); }

inline void operator>>(int16_t data, ADBusMarshaller& m)
{ ADBusAppendInt16(&m, data); }

inline void operator>>(uint16_t data, ADBusMarshaller& m)
{ ADBusAppendUInt16(&m, data); }

inline void operator>>(int32_t data, ADBusMarshaller& m)
{ ADBusAppendInt32(&m, data); }

inline void operator>>(uint32_t data, ADBusMarshaller& m)
{ ADBusAppendUInt32(&m, data); }

inline void operator>>(int64_t data, ADBusMarshaller& m)
{ ADBusAppendInt64(&m, data); }

inline void operator>>(uint64_t data, ADBusMarshaller& m)
{ ADBusAppendUInt64(&m, data); }

inline void operator>>(double data, ADBusMarshaller& m)
{ ADBusAppendDouble(&m, data); }

inline void operator>>(const char* str, ADBusMarshaller& m)
{ ADBusAppendString(&m, str, -1); }

inline void operator>>(const std::string& str, ADBusMarshaller& m)
{ ADBusAppendString(&m, str.c_str(), str.size()); }

// ----------------------------------------------------------------------------

