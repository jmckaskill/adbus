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

#include "qdbusmessage.h"
#include "qdbusargument_p.h"
#include <QtCore/qshareddata.h>
#include <QtCore/qvector.h>
#include <adbus.h>

class QDBusArgumentList
{
public:
    QDBusArgumentList() : m_AppendMessage(false) {}

    bool    init(const QMetaMethod& method);
    void    setupMetacall(const QDBusMessage& msg);
    void**  metacallData() {return m_MetacallData.data();}
    void    getReply(adbus_MsgFactory** ret);
    void    bufferReturnArguments(adbus_MsgFactory* msg) const;
    void    bufferSignalArguments(adbus_MsgFactory* msg, void** args);
    void    finishMetacall() {m_Message = QDBusMessage();}

    struct Entry
    {
        Entry(QDBusArgumentDirection dir, QByteArray name_, QDBusArgumentType* type_)
            : direction(dir), name(name_), type(type_)
        {}

        QDBusArgumentDirection  direction;
        QByteArray              name;
        QDBusArgumentType*      type;
    };

    bool                m_AppendMessage;
    QList<Entry>        m_Args;
    QVector<void*>      m_MetacallData;
    QDBusMessage        m_Message;
};

class QDBusMessagePrivate : public QSharedData
{
public:
    static int          FromMessage(QDBusMessage& q, const adbus_Message* msg);
    static int          FromMessage(QDBusMessage& q, const adbus_Message* msg, const QDBusArgumentList& types);
    static bool         GetMessage(const QDBusMessage& q, adbus_MsgFactory* ret);
    static void         GetReply(const QDBusMessage& q, adbus_MsgFactory** ret, const QDBusArgumentList& types);
    static const QVariant& Argument(const QDBusMessage& q, int argi);

    static adbus_MsgFactory*    GetFactory();

    QDBusMessagePrivate() {reset();}

    void reset();
    void getHeaders(const adbus_Message* msg);

    adbus_MessageType   type;
    int                 flags;
    int64_t             serial;
    int64_t             replySerial;
    QString             signature;
    QString             path;
    QString             interface;
    QString             member;
    QString             error;
    QString             sender;
    QString             destination;
    QList<QVariant>     arguments;

    mutable bool        delayedReply;
    mutable QString     replyErrorName;
    mutable QString     replyErrorMsg;
};

