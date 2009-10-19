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

#include "adbus/Connection.h"

#include <exception>
#include <string>

namespace adbus{

    class Connection;
    class Object;
    class BoundSignalBase;

    // ----------------------------------------------------------------------------

    class ParseError : public std::exception
    {
    public:
        virtual const char* what()const throw()
        { return "ADBus parse error";}
    };

    // ----------------------------------------------------------------------------

    class Connection
    {
        ADBUSCPP_NON_COPYABLE(Connection);
    public:
        Connection();
        Connection(ADBusConnection* connection);
        ~Connection();

        void setSendCallback(ADBusSendCallback callback, ADBusUser* user);
        void parse(const uint8_t* data, size_t size);
        void dispatch(struct ADBusMessage* message);

        void connectToBus() { connectToBus(NULL, NULL); }
        void connectToBus(ADBusConnectionCallback callback, ADBusUser* user);

        void requestServiceName(const std::string&    name,
                                uint32_t              flags = 0);
        void requestServiceName(const std::string&    name,
                                uint32_t              flags,
                                ADBusServiceCallback  callback,
                                ADBusUser*            user);

        bool isConnectedToBus()const;
        std::string uniqueName()const;

        uint32_t nextSerial() const;
        void sendMessage(ADBusMessage* message);

        ADBusConnection* connection(){return m_C;}

    private:
        friend class Object;
        friend class BoundSignalBase;
        ADBusConnection*    m_C;
        ADBusStreamBuffer*  m_Buf;
        ADBusMessage*       m_Message;
        bool                m_FreeConnection;
    };

    // ----------------------------------------------------------------------------

    inline void Connection::requestServiceName(const std::string& name,
                                               uint32_t flags)
    {
      requestServiceName(name, flags, NULL, NULL);
    }

    // ----------------------------------------------------------------------------

    inline void Connection::requestServiceName(const std::string& name,
                                               uint32_t flags,
                                               ADBusServiceCallback callback,
                                               ADBusUser* user)
    {
        ADBusRequestServiceName(m_C,
                                name.c_str(),
                                name.size(),
                                flags,
                                callback,
                                user);
    }

    // ----------------------------------------------------------------------------


}
