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

#include "qdbusabstractadaptor_p.hxx"
#include <qmetaobject.h>


#define QCLASSINFO_DBUS_INTROSPECTION   "D-Bus Introspection"

const char* QDBusAbstractAdaptorPrivate::IntrospectionXml(QDBusAbstractAdaptor* adaptor)
{
    const QMetaObject* meta = adaptor->metaObject();

    int begin = meta->classInfoOffset();
    int end = begin + meta->classInfoCount();
    for (int i = begin; i < end; ++i) {
        QMetaClassInfo info = meta->classInfo(i);
        if (strcmp(info.name(), QCLASSINFO_DBUS_INTROSPECTION) == 0) {
            return info.value();
        }
    }

    return NULL;
}



QDBusAbstractAdaptor::QDBusAbstractAdaptor(QObject *parent)
: QObject(*new QDBusAbstractAdaptorPrivate, parent)
{}

QDBusAbstractAdaptor::~QDBusAbstractAdaptor()
{}

bool QDBusAbstractAdaptor::autoRelaySignals() const
{
    Q_D(const QDBusAbstractAdaptor);
    return d->autoRelaySignals;
}

void QDBusAbstractAdaptor::setAutoRelaySignals(bool enable)
{
    Q_D(QDBusAbstractAdaptor);
    if (d->autoRelaySignals == enable)
        return;

    d->autoRelaySignals = enable;

    if (enable) {
        QObject* obj = parent();
        const QMetaObject* meta = metaObject();

        QByteArray sig;

        int begin = meta->methodOffset();
        int end = begin + meta->methodCount();
        for (int i = begin; i < end; ++i) {
            QMetaMethod method = meta->method(i);
            if (method.methodType() == QMetaMethod::Signal) {
                sig = "2";
                sig += method.signature();
                QObject::connect(obj, sig.constData(), this, sig.constData(), Qt::DirectConnection);
            }
        }
    } else {
        QObject::disconnect(this);
    }

}
