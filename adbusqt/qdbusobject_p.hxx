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

#include <QObject>
#include <QList>
#include <QEvent>
#include "qdbusmetatype_p.hxx"
#include "qdbusconnection.hxx"
#include "dmem/list.h"


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
    adbus_Message       msg;
    void*               user1;
    void*               user2;
    bool                ret;
};

class QDBusProxy : public QObject
{
    Q_OBJECT
public:
    QDBusProxy();
    ~QDBusProxy();

    // Called on a non local thread
    static void ProxyCallback(void* user, adbus_Callback cb, adbus_Callback release, void* cbuser);
    static int  ProxyMsgCallback(void* user, adbus_MsgCallback cb, adbus_CbData* d);

    // Called on the local thread
    virtual bool event(QEvent* e);

private:
    // For use only on the local thread
    adbus_MsgFactory*   m_Msg;

};


/* ------------------------------------------------------------------------- */

class QDBusObject;
struct QDBusMatchData;
struct QDBusReplyData;
struct QDBusBindData;
DILIST_INIT(QDBusMatchData, QDBusMatchData)
DILIST_INIT(QDBusReplyData, QDBusReplyData)
DILIST_INIT(QDBusBindData, QDBusBindData)

struct QDBusUserData
{
    // Only used for user datas bound into the interfaces - called on the
    // connection thread.
    static void Free(void* u) {delete (QDBusUserData*) u;}

    QDBusUserData()
    : argList(), methodIndex(-1), errorIndex(-1), object(NULL)
    {}

    QDBusUserData()
    { free(methodArguments); }

    QDBusObject*                owner;
    void**                      methodArguments;
    int                         methodIndex;
    int                         errorIndex;
    QObject*                    object;
    adbus_Connection*           connection;
};

struct QDBusMatchData : public QDBusUserData
{
    QDBusMatchData() : connMatch(NULL) {adbus_match_init(&match);}
    ~QDBusMatchData() {dil_remove(QDBusMatchData, this, &hl);}

    d_IList(QDBusMatchData)     hl;
    QByteArray                  sender;
    QByteArray                  path;
    QByteArray                  interface;
    QByteArray                  member;
    QByteArray                  slot;
    adbus_Match                 match;
    adbus_ConnMatch*            connMatch;
};

struct QDBusBindData : public QDBusUserData
{
    QDBusBindData() : connBind(NULL) {adbus_bind_init(&bind);}
    ~QDBusBindData() {dil_remove(QDBusBindData, this, &hl);}

    d_IList(QDBusBindData)    hl;
    QByteArray                path;
    adbus_Bind                bind;
    adbus_ConnBind*           connBind;
};

struct QDBusReplyData : public QDBusUserData
{
    QDBusReplyData() : connReply(NULL) {adbus_reply_init(&reply);}
    ~QDBusReplyData() {dil_remove(QDBusReplyData, this, &hl);}

    d_IList(QDBusReplyData)   hl;
    QByteArray                remote;
    adbus_Reply               reply;
    adbus_ConnReply*          connReply;
};

/* ------------------------------------------------------------------------- */

class QDBusObject : public QDBusProxy
{
    Q_OBJECT
public:
    // Public API functions should all be called on the local thread

    QDBusObject(const QDBusConnection& connection, QObject* tracked);

    bool addReply(const QByteArray& remote, uint32_t serial, QObject* receiver, const char* returnMethod, const char* errorMethod);
    bool addMatch(const QByteArray& service, const QByteArray& path, const QByteArray& interface, const QByteArray& name, QObject* receiver, const char* slot);
    void removeMatch(const QByteArray& service, const QByteArray& path, const QByteArray& interface, const QByteArray& name, QObject* receiver, const char* slot);
    bool bindFromMetaObject(const QByteArray& path, QObject* object, QDBusConnection::RegisterOptions options);
    bool bindFromXml(const QByteArray& path, QObject* object, const char* xml);
    void unbind(const QByteArray& path);

    virtual bool event(QEvent* e);
    virtual bool eventFilter(QObject* object, QEvent* event);

    // Callbacks called on the local thread
    static int ReplyCallback(adbus_CbData* d);
    static int ErrorCallback(adbus_CbData* d);
    static int MatchCallback(adbus_CbData* d);
    static int MethodCallback(adbus_CbData* d);
    static int GetPropertyCallback(adbus_CbData* d);
    static int SetPropertyCallback(adbus_CbData* d);

    // Callbacks called on the connection thread
    static void Delete(void* u);
    static void Unregister(void* u);
    static void DoBind(void* u);
    static void FreeDoBind(void* u);
    static void DoAddMatch(void* u);
    static void DoAddReply(void* u);
    static void DoRemoveMatch(void* u);

    adbus_Message*      m_CurrentMessage;
    QDBusArgumentList*  m_CurrentArguments;

public slots:
    void destroy();

private:
    ~QDBusObject() {}

    QDBusConnection           m_QConnection;
    adbus_Connection* const   m_Connection;
    QObject* const            m_Tracked;

    // These lists are manipulated on the local thread and on the connection
    // thread when the object gets destroyed.
    d_IList(QDBusMatchData) m_Matches;
    d_IList(QDBusBindData)  m_Binds;
    d_IList(QDBusReplyData) m_Replies;
};


