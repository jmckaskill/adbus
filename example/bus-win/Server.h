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

#include <adbus.h>

#include <string>
#include <vector>

#include <winsock2.h>

class Remote;

#ifndef NOT_COPYABLE
#   define NOT_COPYABLE(x) x(const x&); x& operator=(const x&)
#endif

class Callback
{
public:
    virtual void OnEvent(HANDLE event) = 0;
};

class EventLoop
{
public:
    virtual void RegisterHandle(Callback* cb, HANDLE event) = 0;
    virtual void UnregisterHandle(Callback* cb, HANDLE event) = 0;
};

class Server : public Callback
{
  NOT_COPYABLE(Server);
public:
  Server(adbus_Interface* i = NULL);
  ~Server();

  void Init(EventLoop* loop);

  void Connect(Remote* r, HANDLE event);
  void Disconnect(Remote* r, HANDLE event);
  void OnEvent(HANDLE event);

  adbus_Server* DBusServer() {return m_Server;}

private:
  std::vector<Remote*>        m_Remotes;
  EventLoop*                  m_EventLoop;
  SOCKET                      m_Socket;
  WSAEVENT                    m_Event;
  adbus_Server*               m_Server;
  HANDLE                      m_Auto;
};


class Remote : public Callback
{
  NOT_COPYABLE(Remote);
public:
  Remote(Server* server, SOCKET socket);
  ~Remote();

  void OnEvent(HANDLE event);
  void Disconnect();

private:
  static adbus_ssize_t SendMsg(void* d, adbus_Message* m);
  static adbus_ssize_t Send(void* d, const char* b, size_t sz);
  static uint8_t       Rand(void* d);

  Server*                     m_Server;
  adbus_Auth*                 m_Auth;
  adbus_Remote*               m_Remote;
  adbus_Buffer*               m_Buffer;
  SOCKET                      m_Socket;
  WSAEVENT                    m_Event;
};
