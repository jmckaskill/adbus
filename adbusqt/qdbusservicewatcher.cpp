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

#include "qdbusservicewatcher_p.h"


/* ------------------------------------------------------------------------- */

QDBusServiceWatcher::QDBusServiceWatcher(QObject* parent)
: QObject(parent)
{} 

/* ------------------------------------------------------------------------- */

QDBusServiceWatcher::~QDBusServiceWatcher()
{}

/* ------------------------------------------------------------------------- */

void QDBusServiceWatcher::setConnection(const QDBusConnection& connection)
{
    Q_D(QDBusServiceWatcher);
    d->connection = connection;

    if (d->object) {
        d->object->destroyOnConnectionThread();
    }

    d->object = new QDBusObject(connection, this);

    d->object->addMatch(
            "org.freedesktop.DBus",
            "/org/freedesktop/DBus",
            "org.freedesktop.DBus",
            "NameOwnerChanged",
            this,
            SLOT(_q_serviceOwnerChanged(QString,QString,QString)));
}

/* ------------------------------------------------------------------------- */

QStringList QDBusServiceWatcher::watchedServices() const
{
    Q_D(const QDBusServiceWatcher);
    return d->services;
}

/* ------------------------------------------------------------------------- */

void QDBusServiceWatcher::setWatchedServices(const QStringList& services)
{
    Q_D(QDBusServiceWatcher);
    d->services = services;
}

/* ------------------------------------------------------------------------- */

void QDBusServiceWatcher::addWatchedService(const QString& newService)
{
    Q_D(QDBusServiceWatcher);
    d->services += newService;
}

/* ------------------------------------------------------------------------- */

bool QDBusServiceWatcher::removeWatchedService(const QString& service)
{
    Q_D(QDBusServiceWatcher);
    return d->services.removeAll(service) != 0;
}

/* ------------------------------------------------------------------------- */

QDBusConnection QDBusServiceWatcher::connection() const
{
    Q_D(const QDBusServiceWatcher);
    return d->connection;
}

/* ------------------------------------------------------------------------- */

QDBusServiceWatcher::WatchMode QDBusServiceWatcher::watchMode() const
{
    Q_D(const QDBusServiceWatcher);
    return d->watchMode;
}

/* ------------------------------------------------------------------------- */

void QDBusServiceWatcher::setWatchMode(WatchMode watchMode)
{
    Q_D(QDBusServiceWatcher);
    d->watchMode = watchMode;
}

/* ------------------------------------------------------------------------- */

void QDBusServiceWatcherPrivate::_q_serviceOwnerChanged(
        QString service,
        QString oldOwner,
        QString newOwner)
{
    Q_Q(QDBusServiceWatcher);

    if (!services.contains(service))
        return;

    if (watchMode & QDBusServiceWatcher::WatchForOwnerChange) 
    {
        emit q->serviceOwnerChanged(service, oldOwner, newOwner);
    }
   
    if (    oldOwner.isEmpty() 
        &&  !newOwner.isEmpty()
        &&  (watchMode & QDBusServiceWatcher::WatchForRegistration))
    {
        emit q->serviceRegistered(service);
    }
   
    if (    !oldOwner.isEmpty()
        &&  newOwner.isEmpty()
        &&  (watchMode & QDBusServiceWatcher::WatchForUnregistration)) 
    {
        emit q->serviceUnregistered(service);
    }
}






