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
#include <qdbusargument.h>
#include <qdbusmetatype.h>
#include <qshareddata.h>

class QDBusArgumentPrivate : public QSharedData
{
public:
    static int Demarshall(adbus_Iterator* iter, QDBusMetaType::DemarshallFunction func, void* data);
    static void Marshall(adbus_Buffer* buf, QDBusMetaType::MarshallFunction func, const void* data);
    static void Marshall(adbus_Buffer* buf, const QVariant& variant);

    QDBusArgumentPrivate() 
        :   parseError(0),
            m_Buffer(NULL)
    { m_Iterator.data = NULL; }

    ~QDBusArgumentPrivate()
    { adbus_buf_free(m_Buffer); }

    QDBusArgumentPrivate(const QDBusArgumentPrivate& other);

    // Grabbing the buffer will reset the iterator
    // Grabbing the iterator will set it up to iterate over the buffer if reset

    adbus_Iterator* iterator() const;
    adbus_Buffer*   buffer();
    void            startArgument(const char* sig);

    int                 parseError;
    QList<adbus_IterArray> iarrays;

    QList<adbus_BufArray> barrays;

    adbus_Buffer*           m_Buffer;
    mutable adbus_Iterator  m_Iterator;
};



