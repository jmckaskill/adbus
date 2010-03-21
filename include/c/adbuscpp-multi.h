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

// Disable some warnings due to the empty switch statement in MemberBase::baseAddArgument
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4065) // switch statement contains 'default' but no 'case' labels
#   pragma warning(disable: 4702) // unreachable code
#endif

#define CAT2(x,y) x##y
#define CAT(x,y) CAT2(x,y)

#define NUM(x) CAT(x,0)
#define REPEAT(x, sep, lead, tail)
#include "adbuscpp-include.h"

#ifdef _MSC_VER
#   pragma warning(pop)
#endif


#define NUM(x) CAT(x,1)
#define REPEAT(x, sep, lead, tail) lead x(0) tail
#include "adbuscpp-include.h"

#define NUM(x) CAT(x,2)
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) tail
#include "adbuscpp-include.h"

#define NUM(x) CAT(x,3)
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) tail
#include "adbuscpp-include.h"

#define NUM(x) CAT(x,4)
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) tail
#include "adbuscpp-include.h"

#define NUM(x) CAT(x,5)
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) tail
#include "adbuscpp-include.h"

#define NUM(x) CAT(x,6)
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) tail
#include "adbuscpp-include.h"

#define NUM(x) CAT(x,7)
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) tail
#include "adbuscpp-include.h"

#define NUM(x) CAT(x,8)
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) sep x(7) tail
#include "adbuscpp-include.h"

#define NUM(x) CAT(x,9)
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) sep x(7) sep x(8) tail
#include "adbuscpp-include.h"

#undef ADBUSCPP_MULTI
#undef CAT2
#undef CAT

