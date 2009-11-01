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

#include "Common.h"
#include "Object.h"

#include "adbus/Message.h"
#include "adbus/Proxy.h"

#include <string>

namespace adbus
{

    /** \addtogroup adbuscpp
     *
     * @{
     */

    class Proxy
    {
        ADBUSCPP_NON_COPYABLE(Proxy);
    public:
        Proxy();
        ~Proxy();

        void bind(
                struct ADBusConnection* connection,
                const std::string&      service,
                const std::string&      path);

        void bind(
                struct ADBusConnection* connection,
                const std::string&      service,
                const std::string&      path,
                const std::string&      interface);

        bool isBound()const {return m_Proxy != NULL;}

#ifdef DOC

        template<class A0 ..., class MF, class O>
        void setCallbackX(Object* object, O* o, MF mf);

        template<class A0 ..., class MF, class O>
        void setErrorCallbackX(Object* object, O* o, MF mf);

        template<class A0 ...>
        void call(const std::string& member, const A0& a0 ...);

#else

#define NUM 0
#define REPEAT(x, sep, lead, tail)
#include "Proxy_t.h"

#define NUM 1
#define REPEAT(x, sep, lead, tail) lead x(0) tail
#include "Proxy_t.h"

#define NUM 2
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) tail
#include "Proxy_t.h"

#define NUM 3
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) tail
#include "Proxy_t.h"

#define NUM 4
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) tail
#include "Proxy_t.h"

#define NUM 5
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) tail
#include "Proxy_t.h"

#define NUM 6
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) tail
#include "Proxy_t.h"

#define NUM 7
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) tail
#include "Proxy_t.h"

#define NUM 8
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) sep x(7) tail
#include "Proxy_t.h"

#define NUM 9
#define REPEAT(x, sep, lead, tail) lead x(0) sep x(1) sep x(2) sep x(3) sep x(4) sep x(5) sep x(6) sep x(7) sep x(8) tail
#include "Proxy_t.h"

#endif

    private:
        struct ADBusFactory m_Factory;
        struct ADBusProxy*  m_Proxy;
    };


    /** @} */


}
