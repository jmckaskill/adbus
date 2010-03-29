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
#include "qdbusargument.hxx"
#include "qdbusmessage.hxx"
#include "qdbusmetatype.hxx"
#include <QtCore/qshareddata.h>


class QDBusArgumentType
{
public:

    static QDBusArgumentType* Lookup(int type);
    static QDBusArgumentType* Lookup(const QByteArray& sig);

    int  demarshall(adbus_Iterator* iter, QVariant& variant) const;
    void marshall(adbus_Buffer* buf, const QVariant& variant, bool appendsig) const;

    int  demarshall(adbus_Iterator* iter, void* data) const;
    void marshall(adbus_Buffer* buf, const void* data, bool appendsig) const;

    int                                 m_TypeId;
    QByteArray                          m_DBusSignature;
    QDBusMetaType::MarshallFunction     m_Marshall;
    QDBusMetaType::DemarshallFunction   m_Demarshall;

private:
    friend class QDBusMetaType;
    QDBusArgumentType() : m_TypeId(-1) {}
};

class QDBusArgumentPrivate : public QSharedData
{
public:
    friend class QDBusArgumentType;
    friend class QDBusMetaType;

    static QDBusArgument Create(adbus_Buffer* b, bool appendsig);
    static QDBusArgument Create(adbus_Iterator* i);

    static int ParseError(const QDBusArgument& arg) {return arg.d->err;}

    ~QDBusArgumentPrivate(){}

    void appendSignature(const char* sig);
    void appendSignature(int typeId);
    bool shouldAppendSignature() const {return depth == 0;}

    bool canIterate() const;
    bool canBuffer();

    int                     err;

    QList<adbus_BufArray>   barrays;
    adbus_Buffer*           buf;

    QList<adbus_IterArray>  iarrays;
    adbus_Iterator*         iter;
    int                     depth;
};



