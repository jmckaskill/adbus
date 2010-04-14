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
#include "qdbusmacros.h"
#include <QtCore/qobject.h>
#include <QtCore/qcoreevent.h>


/* ------------------------------------------------------------------------- */

struct QDBusProxyEvent : public QEvent
{
    QDBusProxyEvent() : QEvent(type) {}
    ~QDBusProxyEvent() {if (release) release(user);}

    static QEvent::Type type;

    adbus_Callback      cb;
    adbus_Callback      release;
    void*               user;
};

struct QDBusProxyMsgEvent : public QEvent
{
    QDBusProxyMsgEvent() : QEvent(type) {}
    ~QDBusProxyMsgEvent();

    static QEvent::Type type;

    adbus_MsgCallback   cb;
    adbus_Connection*   connection;
	adbus_MsgFactory*	ret;
    adbus_Message*      msg;
    void*               user1;
    void*               user2;
};

class QDBUS_EXPORT QDBusProxy : public QObject
{
    Q_OBJECT
public:
    QDBusProxy(adbus_Connection* connection);

    // Called on the connection thread - user is pointer to this QDBusProxy
    static void ProxyCallback(void* user, adbus_Callback cb, adbus_Callback release, void* cbuser);

    // Called on the local thread
    virtual bool event(QEvent* e);

    adbus_Connection* const     m_Connection;

public Q_SLOTS:
    // Called on the local thread. This detaches from the current thread and
    // deletes later on the connection thread.
    void destroyOnConnectionThread();

protected:
    // Called on any thread - should only be used to free local data. No
    // connection callbacks or qt events should come after this.
    virtual ~QDBusProxy();

    // Called on the connection thread - should be used to unregister from the
    // connection
    virtual void unregister() {}

private:
    static void Unregister(void* d);
    static void Delete(void* d);

    // For use only on the local thread
    adbus_MsgFactory*   m_Return;
};

