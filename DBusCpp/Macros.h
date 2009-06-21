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

namespace DBus{
  struct Null_t{};
}

#define DBUSCPP_NON_COPYABLE(classname) \
    classname(const classname&); \
    classname& operator=(const classname&)

#define DBUSCPP_REPEAT(step,sep) \
  step(0) sep \
  step(1) sep \
  step(2) sep \
  step(3) sep \
  step(4) sep \
  step(5) sep \
  step(6) sep \
  step(7) sep \
  step(8) sep

#define DBUSCPP_TYPE(x)           A ## x
#define DBUSCPP_CRTYPE(x)         const DBUSCPP_TYPE(x) &
#define DBUSCPP_ARG(x)            a ## x
#define DBUSCPP_CRARG(x)          DBUSCPP_CRTYPE(x) DBUSCPP_ARG(x)
#define DBUSCPP_CRARG_DEF(x)      DBUSCPP_CRTYPE(x) DBUSCPP_ARG(x) = DBUSCPP_TYPE(x)()
#define DBUSCPP_DECL_TYPE(x)      class DBUSCPP_TYPE(x)
#define DBUSCPP_DECL_TYPE_DEF(x)  class DBUSCPP_TYPE(x) DBUSCPP_DEF_TYPE

#define DBUSCPP_COMMA ,
#define DBUSCPP_BLANK
#define DBUSCPP_COLON ;

#define DBUSCPP_NUM 9

#define DBUSCPP_TYPES                  A0, A1, A2, A3, A4, A5, A6, A7, A8

#define DBUSCPP_CRTYPES                const A0&, const A1&, const A2&, const A3&, const A4&, const A5&, const A6&, const A7&, const A8&

#define DBUSCPP_DECLARE_TYPES          class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8

#define DBUSCPP_DECLARE_TYPES_DEF      class A0 = ::DBus::Null_t, class A1 = ::DBus::Null_t, class A2 = ::DBus::Null_t, class A3 = ::DBus::Null_t, class A4 = ::DBus::Null_t, class A5 = ::DBus::Null_t, class A6 = ::DBus::Null_t, class A7 = ::DBus::Null_t, class A8 = ::DBus::Null_t

#define DBUSCPP_ARGS                   a0, a1, a2, a3, a4, a5, a6, a7, a8

#define DBUSCPP_CRARGS                 const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7, const A8& a8

#define DBUSCPP_CRARGS_DEF             const A0& a0 = ::DBus::Null_t(), const A1& a1 = ::DBus::Null_t(), const A2& a2 = ::DBus::Null_t(), const A3& a3 = ::DBus::Null_t(), const A4& a4 = ::DBus::Null_t(), const A5& a5 = ::DBus::Null_t(), const A6& a6 = ::DBus::Null_t(), const A7& a7 = ::DBus::Null_t(), const A8& a8 = ::DBus::Null_t()

