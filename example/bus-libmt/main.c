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


#include "server.h"
#include <libmt.h>
#include <stdio.h>

#ifndef _WIN32
#   include <sys/types.h>
#   include <sys/socket.h>
#endif

#ifdef _MSC_VER
#   define putenv _putenv
#endif


int main()
{
    Server* server;
    adbus_Socket sock;
    MT_MainLoop* loop;
   
    loop = MT_Loop_New();
    MT_SetCurrent(loop);

    server = Server_New(ADBUS_DEFAULT_BUS, adbus_iface_new("org.freedesktop.DBus", -1));
    if (!server)
        abort();
    if (listen(sock, SOMAXCONN))
        abort();

    MT_Current_Run();

    Server_Free(server);
    MT_Loop_Free(loop);

    return 0;
}

