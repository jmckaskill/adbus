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

#include <QList>
#include <QMetaMethod>
#include <adbus.h>
#include <qdbusmetatype.h>
#include "qdbusargument_p.hxx"


struct QDBusArgumentType
{
    QDBusArgumentType() 
    :   isReturn(false),
        typeId(-1),
        marshall(NULL),
        demarshall(NULL)
    {}

    bool                    isReturn;
    int                     typeId;
    QByteArray              dbusSignature;
    QByteArray              cppSignature;
    QDBusMetaType::MarshallFunction   marshall;
    QDBusMetaType::DemarshallFunction demarshall;
};

class QDBusArgumentList
{
public:
    QDBusArgumentList();
    ~QDBusArgumentList();

    void init(const QDBusArgumentType& type);
    void init(const QList<QDBusArgumentType>& types);

    int SetProperty(adbus_Iterator* argiter);
    void GetProperty(adbus_Buffer* buffer);

    int GetArguments(adbus_Iterator* argiter);
    void GetReturns(adbus_Buffer* buf);

    void** Data() {return m_Args;}

private:
    QList<QDBusArgumentType> m_Types;
    void** m_Args;
};

int qDBusLookupParameters(const QMetaMethod& method, QList<QDBusArgumentType>* args);
int qDBusLookupCppSignature(const QByteArray& sig, QDBusArgumentType* arg);
int qDBusLookupDBusSignature(const QByteArray& sig, QDBusArgumentType* arg);
int qDBusLookupTypeId(int type, QDBusArgumentType* arg);

