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

#include <qdbusconnectioninterface.h>
#include <adbus.h>

/* ------------------------------------------------------------------------- */

QDBusConnectionInterface::QDBusConnectionInterface(const QDBusConnection& connection, QObject* parent)
: QDBusAbstractInterface("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", connection, parent)
{}

QDBusConnectionInterface::~QDBusConnectionInterface()
{}

/* ------------------------------------------------------------------------- */

QDBusReply<bool> QDBusConnectionInterface::isServiceRegistered(const QString& serviceName) const
{ return const_cast<QDBusConnectionInterface*>(this)->call("NameHasOwner", serviceName); }

/* ------------------------------------------------------------------------- */

QDBusReply<QDBusConnectionInterface::RegisterServiceReply>
QDBusConnectionInterface::registerService(
        const QString&              serviceName,
        ServiceQueueOptions         qoption,
        ServiceReplacementOptions   roption)
{
    uint32_t flags = 0;

    if (qoption == DontQueueService) {
        flags |= ADBUS_SERVICE_DO_NOT_QUEUE;
    } else if (qoption == ReplaceExistingService) {
        flags |= ADBUS_SERVICE_REPLACE_EXISTING;
    }

    if (roption == AllowReplacement) {
        flags |= ADBUS_SERVICE_ALLOW_REPLACEMENT;
    }

    return call("RequestName", serviceName, flags);
}

/* ------------------------------------------------------------------------- */

QDBusReply<QStringList> QDBusConnectionInterface::registeredServiceNames() const
{ return const_cast<QDBusConnectionInterface*>(this)->call("ListNames"); }

QDBusReply<QString> QDBusConnectionInterface::serviceOwner(const QString& name) const
{ return const_cast<QDBusConnectionInterface*>(this)->call("GetNameOwner", name); }

QDBusReply<uint> QDBusConnectionInterface::servicePid(const QString& serviceName) const
{ return const_cast<QDBusConnectionInterface*>(this)->call("GetConnectionUnixProcessId", serviceName); }

QDBusReply<uint> QDBusConnectionInterface::serviceUid(const QString& serviceName) const
{ return const_cast<QDBusConnectionInterface*>(this)->call("GetConnectionUnixUser", serviceName); }

QDBusReply<void> QDBusConnectionInterface::startService(const QString& name)
{ return call("StartServiceByName", name); }

QDBusReply<bool> QDBusConnectionInterface::unregisterService(const QString& serviceName)
{ return call("ReleaseName", serviceName); }

/* ------------------------------------------------------------------------- */

void QDBusConnectionInterface::connectNotify(const char* slot)
{ QDBusAbstractInterface::connectNotify(slot); }

void QDBusConnectionInterface::disconnectNotify(const char* slot)
{ QDBusAbstractInterface::disconnectNotify(slot); }


