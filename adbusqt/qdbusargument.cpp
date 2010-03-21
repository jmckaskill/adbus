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

#include "qdbusargument_p.hxx"
#include "qdbusmetatype_p.hxx"
#include "qsharedfunctions_p.hxx"
#include <qvariant.h>

// Setting up an argument

/* ------------------------------------------------------------------------- */

int QDBusArgumentPrivate::Demarshall(adbus_Iterator* iter, QDBusMetaType::DemarshallFunction func, void* data)
{
    QDBusArgumentPrivate d;
    d.m_Iterator = *iter;

    QDBusArgument arg(&d);
    func(arg, data);
    *iter = d.m_Iterator;
    arg.d = NULL;

    return d.parseError;
}

/* ------------------------------------------------------------------------- */

void QDBusArgumentPrivate::Marshall(adbus_Buffer* buf, QDBusMetaType::MarshallFunction func, const void* data)
{
    QDBusArgumentPrivate d;
    d.m_Buffer = buf;

    QDBusArgument arg(&d);
    func(arg, data);
    arg.d = NULL;
    d.m_Buffer = NULL;
}

/* ------------------------------------------------------------------------- */

void QDBusArgumentPrivate::Marshall(adbus_Buffer* buf, const QVariant& variant)
{
    int typeId = variant.userType();
    QDBusArgumentType type;
    if (typeId <= 0 || qDBusLookupTypeId(typeId, &type) || !type.marshall)
        return;

    QDBusArgumentPrivate d;
    d.m_Buffer = buf;

    QDBusArgument arg(&d);
    type.marshall(arg, variant.data());
    arg.d = NULL;
    d.m_Buffer = NULL;
}

/* ------------------------------------------------------------------------- */

QDBusArgumentPrivate::QDBusArgumentPrivate(const QDBusArgumentPrivate& other)
: QSharedData()
{
    m_Buffer = adbus_buf_new();

    adbus_Iterator iter;
    adbus_iter_buffer(&iter, other.m_Buffer);
    if (!iter.data)
        return;

    adbus_buf_appendvalue(m_Buffer, &iter);
}

/* ------------------------------------------------------------------------- */

adbus_Iterator* QDBusArgumentPrivate::iterator() const
{
    if (m_Iterator.data == NULL)
        adbus_iter_buffer(&m_Iterator, m_Buffer);
    return &m_Iterator;
}

/* ------------------------------------------------------------------------- */

adbus_Buffer* QDBusArgumentPrivate::buffer()
{
    m_Iterator.data = NULL;
    if (!barrays.empty())
        adbus_buf_checkarrayentry(m_Buffer, &barrays.back());
    return m_Buffer;
}

/* ------------------------------------------------------------------------- */

void QDBusArgumentPrivate::startArgument(const char* sig)
{
    if (barrays.empty())
        adbus_buf_appendsig(m_Buffer, sig, -1);
}







/* ------------------------------------------------------------------------- */

QDBusArgument::QDBusArgument(QDBusArgumentPrivate* priv)
{ qCopySharedData(d, priv); }

/* ------------------------------------------------------------------------- */

QDBusArgument::QDBusArgument()
{
    qCopySharedData(d, new QDBusArgumentPrivate);
    d->m_Buffer = adbus_buf_new();
}

/* ------------------------------------------------------------------------- */

QDBusArgument::~QDBusArgument()
{ qDestructSharedData(d); }

/* ------------------------------------------------------------------------- */

QDBusArgument::QDBusArgument(const QDBusArgument& other)
{ qCopySharedData(d, other.d); }

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator=(const QDBusArgument& other)
{ qAssignSharedData(d, other.d); return *this; }







// Marshalling stuff

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginArray(int type)
{
    // Only want to set the signature for the top array, as that includes the
    // entire signature for the argument.
    qDetachSharedData(d);
    adbus_Buffer* b = d->buffer();
    if (d->barrays.empty()) {
        adbus_buf_appendsig(b, "a", 1);
        adbus_buf_appendsig(b, QDBusMetaType::typeToSignature(type), -1);
    }

    d->barrays.push_back(adbus_BufArray());
    adbus_buf_beginarray(b, &d->barrays.back());
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::endArray()
{
    qDetachSharedData(d);
    adbus_Buffer* b = d->buffer();
    adbus_buf_endarray(b, &d->barrays.back());
    d->barrays.pop_back();
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginMap(int kid, int vid)
{
    // Only want to set the signature for the top array, as that includes the
    // entire signature for the argument.
    qDetachSharedData(d);
    adbus_Buffer* b = d->buffer();
    if (d->barrays.empty()) {
        adbus_buf_appendsig(b, "a{", 2);
        adbus_buf_appendsig(b, QDBusMetaType::typeToSignature(kid), -1);
        adbus_buf_appendsig(b, QDBusMetaType::typeToSignature(vid), -1);
        adbus_buf_appendsig(b, "}", 1);
    }

    d->barrays.push_back(adbus_BufArray());
    adbus_buf_beginarray(b, &d->barrays.back());
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::endMap()
{ endArray(); }

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginMapEntry()
{ 
    qDetachSharedData(d);
    adbus_buf_begindictentry(d->buffer()); 
}

void QDBusArgument::endMapEntry()
{ 
    qDetachSharedData(d);
    adbus_buf_enddictentry(d->buffer()); 
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginStructure()
{ 
    qDetachSharedData(d);
    adbus_buf_beginstruct(d->buffer()); 
}

void QDBusArgument::endStructure()
{ 
    qDetachSharedData(d);
    adbus_buf_endstruct(d->buffer()); 
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(uchar arg)
{
    qDetachSharedData(d);
    d->startArgument("y");
    adbus_buf_u8(d->buffer(), arg);
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(bool arg)
{
    qDetachSharedData(d);
    d->startArgument("b");
    adbus_buf_bool(d->buffer(), arg ? 1 : 0);
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(short arg)
{
    qDetachSharedData(d);
    d->startArgument("n");
    adbus_buf_i16(d->buffer(), arg);
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(ushort arg)
{
    qDetachSharedData(d);
    d->startArgument("q");
    adbus_buf_u16(d->buffer(), arg);
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(int arg)
{
    qDetachSharedData(d);
    d->startArgument("i");
    adbus_buf_i32(d->buffer(), arg);
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(uint arg)
{
    qDetachSharedData(d);
    d->startArgument("u");
    adbus_buf_u32(d->buffer(), arg);
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(qlonglong arg)
{
    qDetachSharedData(d);
    d->startArgument("x");
    adbus_buf_i64(d->buffer(), arg);
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(qulonglong arg)
{
    qDetachSharedData(d);
    d->startArgument("t");
    adbus_buf_u64(d->buffer(), arg);
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(double arg)
{
    qDetachSharedData(d);
    d->startArgument("d");
    adbus_buf_double(d->buffer(), arg);
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(const QString& arg)
{
    qDetachSharedData(d);
    d->startArgument("s");
    QByteArray utf8 = arg.toUtf8();
    adbus_buf_string(d->buffer(), utf8.constData(), utf8.size());
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(const QDBusObjectPath& arg)
{
    qDetachSharedData(d);
    d->startArgument("o");
    QByteArray str8 = arg.path().toAscii();
    adbus_buf_objectpath(d->buffer(), str8.constData(), str8.size());
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(const QDBusSignature& arg)
{
    qDetachSharedData(d);
    d->startArgument("g");
    QByteArray str8 = arg.signature().toAscii();
    adbus_buf_signature(d->buffer(), str8.constData(), str8.size());
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(const QStringList& arg)
{
    qDetachSharedData(d);
    d->startArgument("as");
    adbus_Buffer* b = d->buffer();
    adbus_BufArray a;
    adbus_buf_beginarray(b, &a);
    for (int i = 0; i < arg.size(); i++) {
        QByteArray utf8 = arg[i].toUtf8();
        adbus_buf_arrayentry(b, &a);
        adbus_buf_string(b, utf8.constData(), utf8.size());
    }
    adbus_buf_endarray(b, &a);
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(const QByteArray& arg)
{
    qDetachSharedData(d);
    d->startArgument("ay");
    adbus_Buffer* b = d->buffer();
    adbus_BufArray a;
    adbus_buf_beginarray(b, &a);
    adbus_buf_append(b, arg.constData(), arg.size());
    adbus_buf_endarray(b, &a);
    return *this;
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::appendVariant(const QVariant& variant)
{
    qDetachSharedData(d);
    int typeId = variant.userType();
    if (typeId == qMetaTypeId<QDBusArgument>()) {
        // We want to pull the actual dbus data out

        const QDBusArgument* arg = (const QDBusArgument*) variant.constData();
        if (arg) {
            adbus_Iterator iter = *arg->d->iterator();
            if (iter.data)
                adbus_buf_appendvalue(d->buffer(), &iter);
        }

    } else {
        // Look up the marshalling function

        QDBusArgumentType type; 
        if (!qDBusLookupTypeId(typeId, &type) && type.marshall) {
            type.marshall(*this, variant.constData());
        }
    }
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(const QDBusVariant& arg)
{
    qDetachSharedData(d);
    d->startArgument("v");
    QVariant variant = arg.variant();

    const char* sig = QDBusMetaType::typeToSignature(variant.userType());

    adbus_Buffer* b = d->buffer();
    adbus_BufVariant v;
    adbus_buf_beginvariant(b, &v, sig ? sig : "", -1);

    if (sig)
        appendVariant(variant);

    adbus_buf_endvariant(b, &v);
    return *this;
}

/* ------------------------------------------------------------------------- */




// Demarshalling stuff

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginArray() const
{ 
    qDetachSharedData(d);
    if (d->parseError) return;

    d->iarrays.push_back(adbus_IterArray());
    d->parseError = adbus_iter_beginarray(d->iterator(), &d->iarrays.back()); 
}

/* ------------------------------------------------------------------------- */

bool QDBusArgument::atEnd() const
{
    qDetachSharedData(d);
    if (d->parseError) return true;
    return adbus_iter_inarray(d->iterator(), &d->iarrays.back()) == 0;
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::endArray() const
{
    qDetachSharedData(d);
    if (d->parseError) return;
    d->parseError = adbus_iter_endarray(d->iterator(), &d->iarrays.back());
    d->iarrays.pop_back();
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginMap() const
{ beginArray(); }

void QDBusArgument::endMap() const
{ endArray(); }

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginMapEntry() const
{
    qDetachSharedData(d);
    if (d->parseError) return;
    d->parseError = adbus_iter_begindictentry(d->iterator());
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::endMapEntry() const
{
    qDetachSharedData(d);
    if (d->parseError) return;
    d->parseError = adbus_iter_enddictentry(d->iterator());
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginStructure() const
{
    qDetachSharedData(d);
    if (d->parseError) return;
    d->parseError = adbus_iter_beginstruct(d->iterator());
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::endStructure() const
{
    qDetachSharedData(d);
    if (d->parseError) return;
    d->parseError = adbus_iter_endstruct(d->iterator());
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(uchar& arg) const
{
    qDetachSharedData(d);
    const uint8_t* data = NULL;
    if (d->parseError) return *this;
    d->parseError = adbus_iter_u8(d->iterator(), &data);
    if (d->parseError) return *this;
    arg = *data;
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(bool& arg) const
{
    qDetachSharedData(d);
    const adbus_Bool* data = NULL;
    if (d->parseError) return *this;
    d->parseError = adbus_iter_bool(d->iterator(), &data);
    if (d->parseError) return *this;
    arg = *data == 0;
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(short& arg) const
{
    qDetachSharedData(d);
    const int16_t* data = NULL;
    if (d->parseError) return *this;
    d->parseError = adbus_iter_i16(d->iterator(), &data);
    if (d->parseError) return *this;
    arg = *data;
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(ushort& arg) const
{
    qDetachSharedData(d);
    const uint16_t* data = NULL;
    if (d->parseError) return *this;
    d->parseError = adbus_iter_u16(d->iterator(), &data);
    if (d->parseError) return *this;
    arg = *data;
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(int& arg) const
{
    qDetachSharedData(d);
    const int32_t* data = NULL;
    if (d->parseError) return *this;
    d->parseError = adbus_iter_i32(d->iterator(), &data);
    if (d->parseError) return *this;
    arg = *data;
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(uint& arg) const
{
    qDetachSharedData(d);
    const uint32_t* data = NULL;
    if (d->parseError) return *this;
    d->parseError = adbus_iter_u32(d->iterator(), &data);
    if (d->parseError) return *this;
    arg = *data;
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(qlonglong& arg) const
{
    qDetachSharedData(d);
    const int64_t* data = NULL;
    if (d->parseError) return *this;
    d->parseError = adbus_iter_i64(d->iterator(), &data);
    if (d->parseError) return *this;
    arg = *data;
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(qulonglong& arg) const
{
    qDetachSharedData(d);
    const uint64_t* data = NULL;
    if (d->parseError) return *this;
    d->parseError = adbus_iter_u64(d->iterator(), &data);
    if (d->parseError) return *this;
    arg = *data;
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(double& arg) const
{
    qDetachSharedData(d);
    const double* data = NULL;
    if (d->parseError) return *this;
    d->parseError = adbus_iter_double(d->iterator(), &data);
    if (d->parseError) return *this;
    arg = *data;
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(QString& arg) const
{
    qDetachSharedData(d);
    const char* str = NULL;
    size_t sz = 0;
    if (d->parseError) return *this;
    d->parseError = adbus_iter_string(d->iterator(), &str, &sz);
    if (d->parseError) return *this;
    arg = QString::fromUtf8(str, sz);
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(QDBusObjectPath& arg) const
{
    qDetachSharedData(d);
    const char* str = NULL;
    size_t sz = 0;
    if (d->parseError) return *this;
    d->parseError = adbus_iter_objectpath(d->iterator(), &str, &sz);
    if (d->parseError) return *this;
    arg.setPath(QString::fromUtf8(str, sz));
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(QDBusSignature& arg) const
{
    qDetachSharedData(d);
    const char* str = NULL;
    size_t sz = 0;
    if (d->parseError) return *this;
    d->parseError = adbus_iter_signature(d->iterator(), &str, &sz);
    if (d->parseError) return *this;
    arg.setSignature(QString::fromUtf8(str, sz));
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(QStringList& arg) const
{
    qDetachSharedData(d);
    beginArray();
    arg.clear();
    while (!atEnd()) {
        arg.push_back(QString());
        *this >> arg.back();
    }
    endArray();
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(QByteArray& arg) const
{
    qDetachSharedData(d);
    if (d->parseError) return *this;

    adbus_IterArray a = {};
    d->parseError = adbus_iter_beginarray(d->iterator(), &a);
    if (d->parseError) return *this;
    d->parseError = adbus_iter_endarray(d->iterator(), &a);
    if (d->parseError) return *this;

    arg.clear();
    arg.append(a.data, a.size);

    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(QDBusVariant& variant) const
{
    // Try and demarshall the argument, but if we cant chuck the argument into
    // the variant as a QDBusArgument

    qDetachSharedData(d);
    if (d->parseError) return *this;

    adbus_Iterator* i = d->iterator();
    adbus_IterVariant v = {};
    d->parseError = adbus_iter_beginvariant(i, &v);
    if (d->parseError) return *this;

    QDBusArgumentType type;
    if (!qDBusLookupDBusSignature(v.sig, &type) && type.typeId > 0 && type.demarshall) {
        // Use the demarshall function
        void* value = QMetaType::construct(type.typeId);
        type.demarshall(*this, value);
        if (!d->parseError)
            variant.setVariant(QVariant(type.typeId, value));
        QMetaType::destroy(type.typeId, value);

    } else {
        // We couldn't find the signature. Copy the data into a new
        // QDBusArgument.
        QDBusArgument arg;
        d->parseError = adbus_buf_appendvalue(arg.d->buffer(), i);
        if (!d->parseError) {
            variant.setVariant(qVariantFromValue<QDBusArgument>(arg));
        }
    }
    return *this;
}



