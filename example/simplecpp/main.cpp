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

#include <adbuscpp.h>
#include <memory>

class Main : public adbus::State
{
public:
    Main(adbus_Connection* c);

    void quit() {adbus_conn_block(m_Connection, ADBUS_UNBLOCK, &m_Block, -1);}
    int run()   {return adbus_conn_block(m_Connection, ADBUS_BLOCK, &m_Block, -1);}

private:
    static adbus::Interface<Main>* Interface();

    adbus_Connection*   m_Connection;
    void*               m_Block;
};

adbus::Interface<Main>* Main::Interface()
{
    static std::auto_ptr<adbus::Interface<Main> > i;
    if (!i.get()) {
        i.reset(new adbus::Interface<Main>("nz.co.foobar.Test.Main"));

        i->addMethod0("Quit", &Main::quit);
    }

    return i.get();
}

Main::Main(adbus_Connection* c) 
    : m_Connection(c), m_Block(NULL) 
{
    bind(c, "/", Interface(), this);

    adbus::Proxy bus(this);
    bus.init(c, "org.freedesktop.DBus", "/");
    bus.call("RequestName", "nz.co.foobar.adbus.SimpleCppTest", uint32_t(0));
}

int main()
{
    adbus_Connection* connection = adbus_sock_busconnect(ADBUS_DEFAULT_BUS, NULL);
    if (!connection)
        abort();

    Main m(connection);
    return m.run();
}

