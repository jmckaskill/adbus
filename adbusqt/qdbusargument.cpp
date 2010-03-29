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
#include "qsharedfunctions_p.h"
#include <QtCore/qvariant.h>
#include <QtCore/qhash.h>
#include <QtCore/qmutex.h>

/* ------------------------------------------------------------------------- */

typedef QHash<int, QDBusArgumentType*> IntTypeHash;
typedef QHash<QByteArray, QDBusArgumentType*> StringTypeHash;

static QMutex* sMutex;
static IntTypeHash *sTypeId;
static StringTypeHash *sDBus;

static void Init()
{
    if (!sMutex)
        sMutex = new QMutex;
    if (!sTypeId)
        sTypeId = new IntTypeHash;
    if (!sDBus)
        sDBus = new StringTypeHash;
}

/* ------------------------------------------------------------------------- */

void QDBusMetaType::registerMarshallOperators(
        int                                 typeId,
        QDBusMetaType::MarshallFunction     marshall,
        QDBusMetaType::DemarshallFunction   demarshall)
{
    Init();

    QDBusArgumentType* type = new QDBusArgumentType;
    type->m_TypeId          = typeId;
    type->m_Marshall        = marshall;
    type->m_Demarshall      = demarshall;

    adbus_Buffer* buf = adbus_buf_new();
    type->marshall(buf, QVariant(typeId, (const void*) NULL), true);
    type->m_DBusSignature = adbus_buf_sig(buf, NULL);
    adbus_buf_free(buf);

    {
        QMutexLocker lock(sMutex);

        Q_ASSERT(!sTypeId->contains(typeId));
        Q_ASSERT(!sDBus->contains(type->m_DBusSignature));

        sTypeId->insert(typeId, type);
        sDBus->insert(type->m_DBusSignature, type);
    }
}

/* ------------------------------------------------------------------------- */

const char* QDBusMetaType::typeToSignature(int typeId)
{
    QDBusArgumentType* type = QDBusArgumentType::Lookup(typeId);
    return type ? type->m_DBusSignature.constData() : NULL;
}

/* ------------------------------------------------------------------------- */

QDBusArgumentType* QDBusArgumentType::Lookup(int type)
{
    Init();
    QMutexLocker lock(sMutex);
    return sTypeId->value(type);
}

/* ------------------------------------------------------------------------- */

QDBusArgumentType* QDBusArgumentType::Lookup(const QByteArray& sig)
{
    Init();
    QMutexLocker lock(sMutex);
    return sDBus->value(sig);
}

/* ------------------------------------------------------------------------- */

static int RegisterBuiltInTypes()
{
    qDBusRegisterMetaType<uchar>();
    qDBusRegisterMetaType<bool>();
    qDBusRegisterMetaType<short>();
    qDBusRegisterMetaType<ushort>();
    qDBusRegisterMetaType<int>();
    qDBusRegisterMetaType<uint>();
    qDBusRegisterMetaType<qlonglong>();
    qDBusRegisterMetaType<qulonglong>();
    qDBusRegisterMetaType<QString>();
    qDBusRegisterMetaType<QDBusVariant>();
    qDBusRegisterMetaType<QDBusObjectPath>();
    qDBusRegisterMetaType<QDBusSignature>();
    qDBusRegisterMetaType<QStringList>();
    qDBusRegisterMetaType<QByteArray>();
    return 0;
}

Q_CONSTRUCTOR_FUNCTION(RegisterBuiltInTypes)









/* ------------------------------------------------------------------------- */

void QDBusArgumentType::marshall(adbus_Buffer* b, const QVariant& variant, bool appendsig) const
{
    Q_ASSERT(variant.userType() == m_TypeId);
    marshall(b, variant.data(), appendsig);
}

void QDBusArgumentType::marshall(adbus_Buffer* b, const void* data, bool appendsig) const
{
    QDBusArgument arg = QDBusArgumentPrivate::Create(b, appendsig);
    m_Marshall(arg, data);
}

int QDBusArgumentType::demarshall(adbus_Iterator* i, QVariant& variant) const
{
    Q_ASSERT(variant.userType() == m_TypeId);
    return demarshall(i, (void*) variant.data());
}

int QDBusArgumentType::demarshall(adbus_Iterator* i, void* data) const
{
    QDBusArgument arg = QDBusArgumentPrivate::Create(i);
    m_Demarshall(arg, data);
    return QDBusArgumentPrivate::ParseError(arg);
}




































/* ------------------------------------------------------------------------- */

QDBusArgument QDBusArgumentPrivate::Create(adbus_Buffer* b, bool appendsig)
{
    QDBusArgumentPrivate* d = new QDBusArgumentPrivate;
    d->err = 0;
    d->buf = b;
    d->depth = appendsig ? 1 : 0;
    return QDBusArgument(d);
}

/* ------------------------------------------------------------------------- */

QDBusArgument QDBusArgumentPrivate::Create(adbus_Iterator* i)
{
    QDBusArgumentPrivate* d = new QDBusArgumentPrivate;
    d->err = 0;
    d->iter = i;
    d->depth = 0;
    return QDBusArgument(d);
}

/* ------------------------------------------------------------------------- */

bool QDBusArgumentPrivate::canIterate() const
{ return iter != NULL && !err; }

bool QDBusArgumentPrivate::canBuffer()
{ return buf != NULL; }

void QDBusArgumentPrivate::appendSignature(const char* sig)
{
    if (shouldAppendSignature()) {
        adbus_buf_appendsig(buf, sig, -1);
    }
}

void QDBusArgumentPrivate::appendSignature(int typeId)
{
    if (shouldAppendSignature()) {
        adbus_buf_appendsig(buf, QDBusMetaType::typeToSignature(typeId), -1);
    }
}







/* ------------------------------------------------------------------------- */

QDBusArgument::QDBusArgument() : d(NULL)
{}

/* ------------------------------------------------------------------------- */

QDBusArgument::QDBusArgument(QDBusArgumentPrivate* priv)
{ qCopySharedData(d, priv); }

/* ------------------------------------------------------------------------- */

QDBusArgument::~QDBusArgument()
{ qDestructSharedData(d); }

/* ------------------------------------------------------------------------- */

QDBusArgument::QDBusArgument(const QDBusArgument& other)
{ qCopySharedData(d, other.d); }









// Marshalling stuff

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginArray(int type)
{
    if (d->canBuffer()) {
        d->appendSignature("a");
        d->appendSignature(type);
        d->barrays.push_back(adbus_BufArray());
        adbus_buf_beginarray(d->buf, &d->barrays.back());
    }
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::endArray()
{
    if (d->canBuffer()) {
        adbus_buf_endarray(d->buf, &d->barrays.back());
        d->barrays.pop_back();
    }
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginMap(int kid, int vid)
{
    if (d->canBuffer()) {
        d->appendSignature("a{");
        d->appendSignature(kid);
        d->appendSignature(vid);
        d->appendSignature("}");
        d->barrays.push_back(adbus_BufArray());
        adbus_buf_beginarray(d->buf, &d->barrays.back());
    }
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::endMap()
{ endArray(); }

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginMapEntry()
{ 
    if (d->canBuffer()) {
        adbus_buf_begindictentry(d->buf); 
    }
}

void QDBusArgument::endMapEntry()
{ 
    if (d->canBuffer()) {
        adbus_buf_enddictentry(d->buf); 
    }
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginStructure()
{ 
    if (d->canBuffer()) {
        adbus_buf_beginstruct(d->buf); 
    }
}

void QDBusArgument::endStructure()
{ 
    if (d->canBuffer()) {
        adbus_buf_endstruct(d->buf); 
    }
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(uchar arg)
{
    if (d->canBuffer()) {
        d->appendSignature("y");
        adbus_buf_u8(d->buf, arg);
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(bool arg)
{
    if (d->canBuffer()) {
        d->appendSignature("b");
        adbus_buf_bool(d->buf, arg ? 1 : 0);
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(short arg)
{
    if (d->canBuffer()) {
        d->appendSignature("n");
        adbus_buf_i16(d->buf, arg);
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(ushort arg)
{
    if (d->canBuffer()) {
        d->appendSignature("q");
        adbus_buf_u16(d->buf, arg);
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(int arg)
{
    if (d->canBuffer()) {
        d->appendSignature("i");
        adbus_buf_i32(d->buf, arg);
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(uint arg)
{
    if (d->canBuffer()) {
        d->appendSignature("u");
        adbus_buf_u32(d->buf, arg);
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(qlonglong arg)
{
    if (d->canBuffer()) {
        d->appendSignature("x");
        adbus_buf_i64(d->buf, arg);
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(qulonglong arg)
{
    if (d->canBuffer()) {
        d->appendSignature("t");
        adbus_buf_u64(d->buf, arg);
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(double arg)
{
    if (d->canBuffer()) {
        d->appendSignature("d");
        adbus_buf_double(d->buf, arg);
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(const QString& arg)
{
    if (d->canBuffer()) {
        QByteArray utf8 = arg.toUtf8();
        d->appendSignature("s");
        adbus_buf_string(d->buf, utf8.constData(), utf8.size());
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(const QDBusObjectPath& arg)
{
    if (d->canBuffer()) {
        QByteArray utf8 = arg.path().toUtf8();
        d->appendSignature("o");
        adbus_buf_objectpath(d->buf, utf8.constData(), utf8.size());
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(const QDBusSignature& arg)
{
    if (d->canBuffer()) {
        QByteArray utf8 = arg.signature().toUtf8();
        d->appendSignature("g");
        adbus_buf_signature(d->buf, utf8.constData(), utf8.size());
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(const QStringList& arg)
{
    if (d->canBuffer()) {
        d->appendSignature("as");

        adbus_Buffer* b = d->buf;
        adbus_BufArray a;
        adbus_buf_beginarray(b, &a);
        for (int i = 0; i < arg.size(); i++) {
            QByteArray utf8 = arg[i].toUtf8();
            adbus_buf_arrayentry(b, &a);
            adbus_buf_string(b, utf8.constData(), utf8.size());
        }
        adbus_buf_endarray(b, &a);
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(const QByteArray& arg)
{
    if (d->canBuffer()) {
        d->appendSignature("ay");
        adbus_Buffer* b = d->buf;
        adbus_BufArray a;
        adbus_buf_beginarray(b, &a);
        adbus_buf_append(b, arg.constData(), arg.size());
        adbus_buf_endarray(b, &a);
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::appendVariant(const QVariant& variant)
{
    if (d->canBuffer()) {
        QDBusArgumentType* type = QDBusArgumentType::Lookup(variant.userType());
        type->marshall(d->buf, variant, d->shouldAppendSignature());
    }
}

/* ------------------------------------------------------------------------- */

QDBusArgument& QDBusArgument::operator<<(const QDBusVariant& arg)
{
    if (d->canBuffer()) {
        QVariant variant = arg.variant();
        QDBusArgumentType* type = QDBusArgumentType::Lookup(variant.userType());
        
        adbus_Buffer* b = d->buf;
        adbus_BufVariant v;
        adbus_buf_beginvariant(b, &v, type->m_DBusSignature.constData(), -1);
        
        type->marshall(b, variant, false);

        adbus_buf_endvariant(b, &v);
    }
    return *this;
}

/* ------------------------------------------------------------------------- */




// Demarshalling stuff

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginArray() const
{ 
    if (d->canIterate()) {
        d->iarrays.push_back(adbus_IterArray());
        d->err = adbus_iter_beginarray(d->iter, &d->iarrays.back());
    }
}

/* ------------------------------------------------------------------------- */

bool QDBusArgument::atEnd() const
{
    if (d->canIterate()) {
        return adbus_iter_inarray(d->iter, &d->iarrays.back()) == 0;
    } else {
        return true;
    }
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::endArray() const
{
    if (d->canIterate()) {
        d->err = adbus_iter_endarray(d->iter, &d->iarrays.back());
        d->iarrays.pop_back();
    }
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginMap() const
{ beginArray(); }

void QDBusArgument::endMap() const
{ endArray(); }

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginMapEntry() const
{
    if (d->canIterate()) {
        d->err = adbus_iter_begindictentry(d->iter);
    }
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::endMapEntry() const
{
    if (d->canIterate()) {
        d->err = adbus_iter_enddictentry(d->iter);
    }
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::beginStructure() const
{
    if (d->canIterate()) {
        d->err = adbus_iter_beginstruct(d->iter);
    }
}

/* ------------------------------------------------------------------------- */

void QDBusArgument::endStructure() const
{
    if (d->canIterate()) {
        d->err = adbus_iter_endstruct(d->iter);
    }
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(uchar& arg) const
{
    if (d->canIterate()) {
        const uint8_t* data = NULL;
        d->err = adbus_iter_u8(d->iter, &data);
        if (!d->err) {
            arg = *data;
        }
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(bool& arg) const
{
    if (d->canIterate()) {
        const adbus_Bool* data = NULL;
        d->err = adbus_iter_bool(d->iter, &data);
        if (!d->err) {
            arg = (*data == 0);
        }
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(short& arg) const
{
    if (d->canIterate()) {
        const int16_t* data = NULL;
        d->err = adbus_iter_i16(d->iter, &data);
        if (!d->err) {
            arg = *data;
        }
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(ushort& arg) const
{
    if (d->canIterate()) {
        const uint16_t* data = NULL;
        d->err = adbus_iter_u16(d->iter, &data);
        if (!d->err) {
            arg = *data;
        }
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(int& arg) const
{
    if (d->canIterate()) {
        const int32_t* data = NULL;
        d->err = adbus_iter_i32(d->iter, &data);
        if (!d->err) {
            arg = *data;
        }
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(uint& arg) const
{
    if (d->canIterate()) {
        const uint32_t* data = NULL;
        d->err = adbus_iter_u32(d->iter, &data);
        if (!d->err) {
            arg = *data;
        }
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(qlonglong& arg) const
{
    if (d->canIterate()) {
        const int64_t* data = NULL;
        d->err = adbus_iter_i64(d->iter, &data);
        if (!d->err) {
            arg = *data;
        }
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(qulonglong& arg) const
{
    if (d->canIterate()) {
        const uint64_t* data = NULL;
        d->err = adbus_iter_u64(d->iter, &data);
        if (!d->err) {
            arg = *data;
        }
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(double& arg) const
{
    if (d->canIterate()) {
        const double* data = NULL;
        d->err = adbus_iter_double(d->iter, &data);
        if (!d->err) {
            arg = *data;
        }
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(QString& arg) const
{
    if (d->canIterate()) {
        const char* str = NULL;
        size_t sz = 0;
        d->err = adbus_iter_string(d->iter, &str, &sz);
        if (!d->err) {
            arg = QString::fromUtf8(str, (int) sz);
        }
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(QDBusObjectPath& arg) const
{
    if (d->canIterate()) {
        const char* str = NULL;
        size_t sz = 0;
        d->err = adbus_iter_objectpath(d->iter, &str, &sz);
        if (!d->err) {
            arg.setPath(QString::fromUtf8(str, (int) sz));
        }
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(QDBusSignature& arg) const
{
    if (d->canIterate()) {
        const char* str = NULL;
        size_t sz = 0;
        d->err = adbus_iter_signature(d->iter, &str, &sz);
        if (!d->err) {
            arg.setSignature(QString::fromUtf8(str, (int) sz));
        }
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(QStringList& arg) const
{
    if (d->canIterate()) {
        beginArray();
        arg.clear();
        while (!atEnd()) {
            arg.push_back(QString());
            *this >> arg.back();
        }
        endArray();
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(QByteArray& arg) const
{
    if (d->canIterate()) {
        adbus_IterArray a;
        d->err = adbus_iter_beginarray(d->iter, &a) 
              || adbus_iter_endarray(d->iter, &a);

        if (!d->err) {
            arg.clear();
            arg.append(a.data, (int) a.size);
        }
    }
    return *this;
}

/* ------------------------------------------------------------------------- */

const QDBusArgument& QDBusArgument::operator>>(QDBusVariant& arg) const
{
    if (d->canIterate()) {
        adbus_IterVariant v;
        d->err = adbus_iter_beginvariant(d->iter, &v);
        if (!d->err)
            return *this;

        QVariant variant;
        QDBusArgumentType* type = QDBusArgumentType::Lookup(v.sig);
        d->err = type->demarshall(d->iter, variant);

        if (!d->err)
            return *this;

        arg.setVariant(variant);
    }
    return *this;
}



