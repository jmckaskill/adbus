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
#include <stdio.h>
#include <limits.h>

class Main : public adbus::State
{
public:
    Main(adbus_Connection* c);

    void quit() {adbus_conn_block(m_Connection, ADBUS_UNBLOCK, &m_Block, -1);}
    int run()   {return adbus_conn_block(m_Connection, ADBUS_BLOCK, &m_Block, INT_MAX);}

private:
    static adbus::Interface<Main> CreateInterface();

    void NameRequested(uint32_t ret);
    void NameError(const char* err, const char* msg);

    adbus_Connection*   m_Connection;
    uintptr_t           m_Block;
};

adbus::Interface<Main> Main::CreateInterface()
{
    adbus::Interface<Main> i("nz.co.foobar.Test.Main");
    i.addMethod0("Quit", &Main::quit);
    return i;
}

Main::Main(adbus_Connection* c) 
    : m_Connection(c), m_Block(NULL) 
{
    static adbus::Interface<Main> interface = CreateInterface();

    bind(c, "/", interface, this);

    adbus::Proxy bus(this);
    bus.init(c, "org.freedesktop.DBus", "/");

    bus.method("RequestName")
        .arg("nz.co.foobar.adbus.SimpleCppTest")
        .arg(uint32_t(0))
        .setCallback1<uint32_t>(&Main::NameRequested, this)
        .setError(&Main::NameError, this)
        .send();
}

void Main::NameRequested(uint32_t ret)
{
    fprintf(stderr, "RequestName returned %d", (int) ret);
}

void Main::NameError(const char* err, const char* msg)
{
    fprintf(stderr, "Error %s: %s\n", err, msg);
    exit(-1);
}

int main()
{
    adbus_Connection* connection = adbus_sock_busconnect(ADBUS_DEFAULT_BUS, NULL);
    if (!connection)
        abort();

    Main m(connection);
    return m.run();
}

