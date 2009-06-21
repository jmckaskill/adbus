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

namespace adbus
{
namespace detail
{
  template<unsigned int Arity, class R>
  struct FunctionTag
  {};
  template<unsigned int Arity, class R>
  struct MemberFunctionTag{};

  template<unsigned int Arity>
  struct FunctionVoidTag
  {};
  template<unsigned int Arity>
  struct MemberFunctionVoidTag{};


#if 0
  template<class R, class M>
  MemberFunctionTag<0,R> arity(R (M::*)()){return Return<R>::memberFunctionTag<0>();}

  template<class R, class M, class A0>
  MemberFunctionTag<1,R> arity(R (M::*)(A0)){return Return<R>::memberFunctionTag<1>();}

  template<class R, class M, class A0, class A1>
  MemberFunctionTag<2,R> arity(R (M::*)(A0, A1)){return Return<R>::memberFunctionTag<2>();}

  template<class R, class M, class A0, class A1, class A2>
  MemberFunctionTag<3,R> arity(R (M::*)(A0, A1, A2)){return Return<R>::memberFunctionTag<3>();}

  template<class R, class M, class A0, class A1, class A2, class A3>
  MemberFunctionTag<4,R> arity(R (M::*)(A0, A1, A2, A3)){return Return<R>::memberFunctionTag<4>();}

  template<class R, class M, class A0, class A1, class A2, class A3, class A4>
  MemberFunctionTag<5,R> arity(R (M::*)(A0, A1, A2, A3, A4)){return Return<R>::memberFunctionTag<5>();}

  template<class R, class M, class A0, class A1, class A2, class A3, class A4, class A5>
  MemberFunctionTag<6,R> arity(R (M::*)(A0, A1, A2, A3, A4, A5)){return Return<R>::memberFunctionTag<6>();}

  template<class R, class M, class A0, class A1, class A2, class A3, class A4, class A5, class A6>
  MemberFunctionTag<7,R> arity(R (M::*)(A0, A1, A2, A3, A4, A5, A6)){return Return<R>::memberFunctionTag<7>();}

  template<class R, class M, class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
  MemberFunctionTag<8,R> arity(R (M::*)(A0, A1, A2, A3, A4, A5, A6, A7)){return Return<R>::memberFunctionTag<8>();}

  template<class R, class M, class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
  MemberFunctionTag<9,R> arity(R (M::*)(A0, A1, A2, A3, A4, A5, A6, A7, A8)){return Return<R>::memberFunctionTag<9>();}

  template<class M>
  MemberFunctionVoidTag<0> arity(void (M::*)()){return MemberFunctionVoidTag<0>();}

  template<class M, class A0>
  MemberFunctionVoidTag<1> arity(void (M::*)(A0)){return MemberFunctionVoidTag<1>();}

  template<class M, class A0, class A1>
  MemberFunctionVoidTag<2> arity(void (M::*)(A0, A1)){return MemberFunctionVoidTag<2>();}

  template<class M, class A0, class A1, class A2>
  MemberFunctionVoidTag<3> arity(void (M::*)(A0, A1, A2)){return MemberFunctionVoidTag<3>();}

  template<class M, class A0, class A1, class A2, class A3>
  MemberFunctionVoidTag<4> arity(void (M::*)(A0, A1, A2, A3)){return MemberFunctionVoidTag<4>();}

  template<class M, class A0, class A1, class A2, class A3, class A4>
  MemberFunctionVoidTag<5> arity(void (M::*)(A0, A1, A2, A3, A4)){return MemberFunctionVoidTag<5>();}

  template<class M, class A0, class A1, class A2, class A3, class A4, class A5>
  MemberFunctionVoidTag<6> arity(void (M::*)(A0, A1, A2, A3, A4, A5)){return MemberFunctionVoidTag<6>();}

  template<class M, class A0, class A1, class A2, class A3, class A4, class A5, class A6>
  MemberFunctionVoidTag<7> arity(void (M::*)(A0, A1, A2, A3, A4, A5, A6)){return MemberFunctionVoidTag<7>();}

  template<class M, class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
  MemberFunctionVoidTag<8> arity(void (M::*)(A0, A1, A2, A3, A4, A5, A6, A7)){return MemberFunctionVoidTag<8>();}

  template<class M, class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
  MemberFunctionVoidTag<9> arity(void (M::*)(A0, A1, A2, A3, A4, A5, A6, A7, A8)){return MemberFunctionVoidTag<9>();}



  template<class R, class M>
  FunctionTag<0,R> arity(R (*)()){return Return<R>::functionTag<0>();}

  template<class R, class A0>
  FunctionTag<1,R> arity(R (*)(A0)){return Return<R>::functionTag<1>();}

  template<class R, class A0, class A1>
  FunctionTag<2,R> arity(R (*)(A0, A1)){return Return<R>::functionTag<2>();}

  template<class R, class A0, class A1, class A2>
  FunctionTag<3,R> arity(R (*)(A0, A1, A2)){return Return<R>::functionTag<3>();}

  template<class R, class A0, class A1, class A2, class A3>
  FunctionTag<4,R> arity(R (*)(A0, A1, A2, A3)){return Return<R>::functionTag<4>();}

  template<class R, class A0, class A1, class A2, class A3, class A4>
  FunctionTag<5,R> arity(R (*)(A0, A1, A2, A3, A4)){return Return<R>::functionTag<5>();}

  template<class R, class A0, class A1, class A2, class A3, class A4, class A5>
  FunctionTag<6,R> arity(R (*)(A0, A1, A2, A3, A4, A5)){return Return<R>::functionTag<6>();}

  template<class R, class A0, class A1, class A2, class A3, class A4, class A5, class A6>
  FunctionTag<7,R> arity(R (*)(A0, A1, A2, A3, A4, A5, A6)){return Return<R>::functionTag<7>();}

  template<class R, class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
  FunctionTag<8,R> arity(R (*)(A0, A1, A2, A3, A4, A5, A6, A7)){return Return<R>::functionTag<8>();}

  template<class R, class A0, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
  FunctionTag<9,R> arity(R (*)(A0, A1, A2, A3, A4, A5, A6, A7, A8)){return Return<R>::functionTag<9>();}
#endif


}

}
