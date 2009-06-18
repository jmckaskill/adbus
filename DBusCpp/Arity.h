#pragma once

namespace DBus
{
namespace detail
{

  template<unsigned int Arity, class R>
  struct FunctionTag{};

  template<unsigned int Arity, class R>
  struct MemberFunctionTag{};

#define MFARG(NUM)  \
  template<class R, \
           class M, \
           DBUSCPP_REPEAT( DBUSCPP_DECL_TYPE, NUM, DBUSCPP_COMMA )  \
          > \
  MemberFunctionTag<NUM,R> arity(R (M::*)( REPEAT(DBUSCPP_TYPE, NUM, DBUSCPP_COMMA) ))  \
  {return MemberFunction_tag<NUM>();}

  template<class R, class M>
  MemberFunctionTag<0,R> arity(R (M::*)()){return MemberFunctionTag<0,R>();}

  MFARG(1)
  MFARG(2)
  MFARG(3)
  MFARG(4)
  MFARG(5)
  MFARG(6)
  MFARG(7)
  MFARG(8)
  MFARG(9)
#undef MFARG

#define FARG(NUM) \
  template<class R, \
           DBUSCPP_REPEAT(DBUSCPP_DECL_TYPE, NUM, DBUSCPP_COMMA)  \
          > \
  FunctionTag<NUM,R> arity(R (*)( REPEAT(DBUSCPP_TYPE, NUM, DBUSCPP_COMMA) ))  \
  {return FunctionTag<NUM,R>();}

  template<class R>
  FunctionTag<0,R> arity(R (*)()){return FunctionTag<0,R>();}

  FARG(1)
  FARG(2)
  FARG(3)
  FARG(4)
  FARG(5)
  FARG(6)
  FARG(7)
  FARG(8)
  FARG(9)
#undef FARG

}

}
