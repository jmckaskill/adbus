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

#include <adbus/adbuscpp.h>

#include <sys/socket.h>
#include <string.h>
#include <stdio.h>

class Socket : public adbus::User
{
public:
    Socket(adbus_Socket sock):s(sock){}
    adbus_Socket s;
};

static void Send(adbus_Message* m, const adbus_User* u)
{
    const char* msg = adbus_msg_summary(m, NULL);
    fprintf(stderr, "Sending\n%s\n\n", msg);

    Socket* s = (Socket*) u;
    size_t size;
    const uint8_t* data = adbus_msg_data(m, &size);
    send(s->s, data, size, 0);
}


class Quitter;
static adbus::Interface<Quitter>* Interface();

class Quitter
{
public:
    Quitter() :m_Quit(false) {}

    void bind(adbus::Path p)
    { p.bind(Interface(), this); }

    void quit()
    {
        m_Quit = true;
    }
    
    bool m_Quit;
};

static adbus::Interface<Quitter>* Interface()
{
    static adbus::Interface<Quitter>* i = NULL;
    if (i == NULL) {
        i = new adbus::Interface<Quitter>("nz.co.foobar.Test.Quit");
        i->addMethod0("Quit", &Quitter::quit);

    }

    return i;
}



int main()
{
    adbus_Socket s = adbus_sock_connect(ADBUS_SESSION_BUS);
    if (s == ADBUS_SOCK_INVALID)
        return 1;

    adbus_Message* msg = adbus_msg_new();
    adbus_Stream* stream = adbus_stream_new();;
    adbus::Connection c;
    c.setSender(&Send, new Socket(s));

    Quitter q;
    q.bind(c.path("/"));

    // Once we are all setup, we connect to the bus
    c.connectToBus();
    c.requestName("nz.co.foobar.adbus.Test");

    char buf[64 * 1024];
    while(!q.m_Quit) {
        int recvd = recv(s, buf, sizeof(buf), 0);
        if (recvd < 0)
            return 2;

        const uint8_t* data = (const uint8_t*) buf;
        size_t size = recvd;
        while (size > 0) {
            if (adbus_stream_parse(stream, msg, &data, &size))
                return 3;

            const char* summary = adbus_msg_summary(msg, NULL);
            fprintf(stderr, "Received\n%s\n\n", summary);

            if (c.dispatch(msg))
                return 4;
        }
    }

    return 0;
}

