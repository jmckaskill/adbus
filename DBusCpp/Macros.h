#pragma once

#define DBUSCPP_REPEATB(x,sep,step) sep  x(step)
#define DBUSCPP_REPEAT0(x,sep)
#define DBUSCPP_REPEAT1(x,sep) x(0)
#define DBUSCPP_REPEAT2(x,sep) DBUSCPP_REPEAT1(x,sep) DBUSCPP_REPEATB(x,sep,1) 
#define DBUSCPP_REPEAT3(x,sep) DBUSCPP_REPEAT2(x,sep) DBUSCPP_REPEATB(x,sep,2)
#define DBUSCPP_REPEAT4(x,sep) DBUSCPP_REPEAT3(x,sep) DBUSCPP_REPEATB(x,sep,3)
#define DBUSCPP_REPEAT5(x,sep) DBUSCPP_REPEAT4(x,sep) DBUSCPP_REPEATB(x,sep,4)
#define DBUSCPP_REPEAT6(x,sep) DBUSCPP_REPEAT5(x,sep) DBUSCPP_REPEATB(x,sep,5)
#define DBUSCPP_REPEAT7(x,sep) DBUSCPP_REPEAT6(x,sep) DBUSCPP_REPEATB(x,sep,6)
#define DBUSCPP_REPEAT8(x,sep) DBUSCPP_REPEAT7(x,sep) DBUSCPP_REPEATB(x,sep,7)
#define DBUSCPP_REPEAT9(x,sep) DBUSCPP_REPEAT8(x,sep) DBUSCPP_REPEATB(x,sep,8)
#define DBUSCPP_CAT2(x,y) x ## y
#define DBUSCPP_REPEAT(step,num,sep) DBUSCPP_CAT2(DBUSCPP_REPEAT, num) (step,sep)

// The preprocessor seems to be protecting against recursive macro calls by
// not allowing a macro to be called that is on the macro stack, so for the
// few situations where that is actually what we want to, we use REPEAT2
// instead
#define DBUSCPP_REPEAT2B(x,sep,step) sep  x(step)
#define DBUSCPP_REPEAT20(x,sep)
#define DBUSCPP_REPEAT21(x,sep) x(0)
#define DBUSCPP_REPEAT22(x,sep) DBUSCPP_REPEAT21(x,sep) DBUSCPP_REPEAT2B(x,sep,1) 
#define DBUSCPP_REPEAT23(x,sep) DBUSCPP_REPEAT22(x,sep) DBUSCPP_REPEAT2B(x,sep,2)
#define DBUSCPP_REPEAT24(x,sep) DBUSCPP_REPEAT23(x,sep) DBUSCPP_REPEAT2B(x,sep,3)
#define DBUSCPP_REPEAT25(x,sep) DBUSCPP_REPEAT24(x,sep) DBUSCPP_REPEAT2B(x,sep,4)
#define DBUSCPP_REPEAT26(x,sep) DBUSCPP_REPEAT25(x,sep) DBUSCPP_REPEAT2B(x,sep,5)
#define DBUSCPP_REPEAT27(x,sep) DBUSCPP_REPEAT26(x,sep) DBUSCPP_REPEAT2B(x,sep,6)
#define DBUSCPP_REPEAT28(x,sep) DBUSCPP_REPEAT27(x,sep) DBUSCPP_REPEAT2B(x,sep,7)
#define DBUSCPP_REPEAT29(x,sep) DBUSCPP_REPEAT28(x,sep) DBUSCPP_REPEAT2B(x,sep,8)
#define DBUSCPP_REPEAT2(step,num,sep) DBUSCPP_CAT2(DBUSCPP_REPEAT2, num) (step,sep)

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

#define DBUSCPP_DECLARE_TYPES_DEF      class A0 = Null_t, class A1 = Null_t, class A2 = Null_t, class A3 = Null_t, class A4 = Null_t, class A5 = Null_t, class A6 = Null_t, class A7 = Null_t, class A8 = Null_t

#define DBUSCPP_ARGS                   a0, a1, a2, a3, a4, a5, a6, a7, a8

#define DBUSCPP_CRARGS                 const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7, const A8& a8

#define DBUSCPP_CRARGS_DEF_NO_0_DEF    const A0& a0       , const A1& a1 = A1(), const A2& a2 = A2(), const A3& a3 = A3(), const A4& a4 = A4(), const A5& a5 = A5(), const A6& a6 = A6(), const A7& a7 = A7(), const A8& a8 = A8()
#define DBUSCPP_CRARGS_DEF             const A0& a0 = A0(), const A1& a1 = A1(), const A2& a2 = A2(), const A3& a3 = A3(), const A4& a4 = A4(), const A5& a5 = A5(), const A6& a6 = A6(), const A7& a7 = A7(), const A8& a8 = A8()

