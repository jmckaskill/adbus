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

#include "Server.h"
#include <dmem/string.h>


Server::Server(adbus_Interface* i)
{
    adbus_Interface* iface = i ? i : adbus_iface_new("org.freedesktop.DBus", -1);

    m_EventLoop = NULL;
    m_Socket = ADBUS_SOCK_INVALID;
    m_Server = adbus_serv_new(iface);
    m_Event = INVALID_HANDLE_VALUE;
    m_Auto = INVALID_HANDLE_VALUE;
}

static void SetAutoAddress(HANDLE map, const char* str)
{
    if (map == INVALID_HANDLE_VALUE)
        return;

    void* view = MapViewOfFile(map, FILE_MAP_WRITE, 0, 0, strlen(str) + 1);
    if (view)
        CopyMemory(view, str, strlen(str) + 1);

    UnmapViewOfFile(view);
}

void Server::Init(EventLoop* loop)
{
    char buf[255];
    if (adbus_bind_address(ADBUS_SESSION_BUS, buf, sizeof(buf)))
        return;

    const char* address = buf;
    if (strcmp(buf, "autostart:") == 0) {
        m_Auto = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, L"Local\\DBUS_SESSION_BUS_ADDRESS");
        address = "tcp:host=localhost,port=0";
        SetAutoAddress(m_Auto, "");
    }

    m_EventLoop = loop;
    m_Socket = adbus_sock_bind_s(address, -1);
    if (m_Socket == ADBUS_SOCK_INVALID)
        abort();

    m_Event = WSACreateEvent();
    WSAEventSelect(m_Socket, m_Event, FD_ACCEPT);
    m_EventLoop->RegisterHandle(this, m_Event);

    listen(m_Socket, SOMAXCONN);

    struct sockaddr_in addr;
    int sz = sizeof(sockaddr_in);
    if (!getsockname(m_Socket, (struct sockaddr*) &addr, &sz)) {
        d_String s = {};
        ds_set_f(&s, "tcp:host=localhost,port=%d", (int) ntohs(addr.sin_port));
        SetAutoAddress(m_Auto, ds_cstr(&s));
        ds_free(&s);
    }
}

Server::~Server()
{
    SetAutoAddress(m_Auto, "");
    CloseHandle(m_Auto);

    for (size_t i = 0; i < m_Remotes.size(); i++) {
        delete m_Remotes[i];
    }

    if (m_EventLoop)
      m_EventLoop->UnregisterHandle(this, m_Event);
    adbus_serv_free(m_Server);
    WSACloseEvent(m_Event);
    closesocket(m_Socket);
}

void Server::Connect(Remote* r, HANDLE event)
{
    m_Remotes.push_back(r);
    m_EventLoop->RegisterHandle(r, event);
}

void Server::Disconnect(Remote* remote, HANDLE event)
{
    for (size_t i = 0; i < m_Remotes.size(); i++)
    {
        if (m_Remotes[i] == remote) {
            m_EventLoop->UnregisterHandle(remote, event);
            delete remote;
            m_Remotes.erase(m_Remotes.begin() + i);
            return;
        }
    }
    assert(0);
}

void Server::OnEvent(HANDLE event)
{
    (void) event;

    WSANETWORKEVENTS events;
    if (WSAEnumNetworkEvents(m_Socket, m_Event, &events))
        abort();

    if (events.lNetworkEvents & FD_ACCEPT) {
        SOCKET sock = accept(m_Socket, NULL, NULL);
        if (sock != INVALID_SOCKET) {
            new Remote(this, sock);
        }
    }
}



Remote::Remote(Server* server, SOCKET socket)
{
    m_Server  = server;
    m_Auth    = NULL;
    m_Remote  = NULL;
    m_Buffer  = adbus_buf_new();
    m_Socket  = socket;
    m_Event   = WSACreateEvent();
    WSAEventSelect(m_Socket, m_Event, FD_READ | FD_CLOSE);
    m_Server->Connect(this, m_Event);
}

Remote::~Remote()
{
    adbus_auth_free(m_Auth);
    adbus_remote_disconnect(m_Remote);
    adbus_buf_free(m_Buffer);
    WSACloseEvent(m_Event);
    closesocket(m_Socket);
}

int Remote::SendMsg(void* d, const adbus_Message* m)
{
    Remote* r = (Remote*) d;
    return (int) send(r->m_Socket, m->data, (int) m->size, 0);
}

int Remote::Send(void* d, const char* b, size_t sz)
{
    Remote* r = (Remote*) d;
    return (int) send(r->m_Socket, b, (int) sz, 0);
}

uint8_t Remote::Rand(void* d)
{
    (void) d;
    return (uint8_t) rand();
}

void Remote::Disconnect()
{
    adbus_remote_disconnect(m_Remote);
    m_Remote = NULL;
    m_Server->Disconnect(this, m_Event);
}

#define RECV_SIZE 64 * 1024
void Remote::OnEvent(HANDLE event)
{
    (void) event;

    WSANETWORKEVENTS events;
    if (WSAEnumNetworkEvents(m_Socket, m_Event, &events))
      return Disconnect();

    if (events.lNetworkEvents & FD_READ) {
        int read;
        do {
            char* dest = adbus_buf_recvbuf(m_Buffer, RECV_SIZE);
            read = recv(m_Socket, dest, RECV_SIZE, 0);
            adbus_buf_recvd(m_Buffer, RECV_SIZE, read);
        } while (read == RECV_SIZE);

        while (adbus_buf_size(m_Buffer) > 0) {
            if (m_Remote) {
                if (adbus_remote_parse(m_Remote, m_Buffer)) {
                    return Disconnect();
                }
                break;
            } else if (m_Auth) {
                adbus_Bool finished;
                char* data = adbus_buf_data(m_Buffer);
                size_t size = adbus_buf_size(m_Buffer);

                int used = adbus_auth_parse(m_Auth, data, size, &finished);

                if (used < 0) {
                    return Disconnect();
                }

                adbus_buf_remove(m_Buffer, 0, used);

                if (finished) {
                    adbus_auth_free(m_Auth);
                    m_Auth = NULL;
                    m_Remote = adbus_serv_connect(m_Server->DBusServer(), &SendMsg, this);
                } else {
                    break;
                }

            } else {
                char* d = adbus_buf_data(m_Buffer);
                if (*d != '\0') {
                    return Disconnect();
                }
                adbus_buf_remove(m_Buffer, 0, 1);
                m_Auth = adbus_sauth_new(&Send, &Rand, this);
                adbus_sauth_external(m_Auth, NULL);
            }
        }

        if (read < 0) {
            return Disconnect();
        }
    }

    if (events.lNetworkEvents & FD_CLOSE) {
        return Disconnect();
    }
}

