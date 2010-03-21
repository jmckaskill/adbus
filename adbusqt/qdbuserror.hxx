/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDBus module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDBUSERROR_H
#define QDBUSERROR_H

#include <QtDBus/qdbusmacros.h>
#include <QtCore/qstring.h>

QT_BEGIN_HEADER

struct DBusError;

QT_BEGIN_NAMESPACE

QT_MODULE(DBus)

class QDBusMessage;

class QDBUS_EXPORT QDBusError
{
public:
    enum ErrorType {
        NoError = 0,
        Other = 1,
        Failed,
        NoMemory,
        ServiceUnknown,
        NoReply,
        BadAddress,
        NotSupported,
        LimitsExceeded,
        AccessDenied,
        NoServer,
        Timeout,
        NoNetwork,
        AddressInUse,
        Disconnected,
        InvalidArgs,
        UnknownMethod,
        TimedOut,
        InvalidSignature,
        UnknownInterface,
        InternalError,
        UnknownObject,
        InvalidService,
        InvalidObjectPath,
        InvalidInterface,
        InvalidMember,

#ifndef Q_QDOC
        // don't use this one!
        LastErrorType = InvalidMember
#endif
    };

    QDBusError(const DBusError *error = 0);
    QDBusError(const QDBusMessage& msg);
    QDBusError(ErrorType error, const QString &message);
    QDBusError(const QDBusError &other);
    QDBusError &operator=(const QDBusError &other);

    ErrorType type() const;
    QString name() const;
    QString message() const;
    bool isValid() const;

    static QString errorString(ErrorType error);

private:
    ErrorType code;
    QString msg;
    QString nm;
    void *unused;
};

#ifndef QT_NO_DEBUG_STREAM
QDBUS_EXPORT QDebug operator<<(QDebug, const QDBusError &);
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif
