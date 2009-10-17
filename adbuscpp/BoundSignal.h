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

#include "Object.h"
#include "Interface.h"

namespace adbus
{
    class BoundSignalBase
    {
        ADBUSCPP_NON_COPYABLE(BoundSignalBase);
    public:
        BoundSignalBase();
        ~BoundSignalBase();
        void bind(Member signal, Object* object);
        bool isBound()const{return m_Object != NULL;}

    protected:
        void setupMessage();
        void sendMessage();
        ADBusMessage*   m_Message;

    private:
        Object*         m_Object;
        Member          m_Signal;
    };

    // This now defines a whole bunch of BoundSignal types like:
    //
    // template<class A0, class A1>
    // class BoundSignal2 : public BoundSignalBase
    // {
    //      ADBUSCPP_NON_COPYABLE(BoundSignal2);
    // public:
    //      void emit(const A0& a0, const A1& a1);
    // };

#define NUM 0
#define REPEAT(x, sep, lead, tail)
#include "BoundSignal_t.h"

#define NUM 1
#define REPEAT(x, sep, lead, tail) lead x(0) tail
#include "BoundSignal_t.h"

#define NUM 2
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) tail
#include "BoundSignal_t.h"

#define NUM 3
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) tail
#include "BoundSignal_t.h"

#define NUM 4
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) tail
#include "BoundSignal_t.h"

#define NUM 5
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) tail
#include "BoundSignal_t.h"

#define NUM 6
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) tail
#include "BoundSignal_t.h"

#define NUM 7
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) tail
#include "BoundSignal_t.h"

#define NUM 8
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) sep x(7) tail
#include "BoundSignal_t.h"

#define NUM 9
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) sep x(7) sep x(8) tail
#include "BoundSignal_t.h"


}
