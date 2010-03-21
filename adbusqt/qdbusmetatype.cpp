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


#include "qdbusmetatype_p.hxx"

#include <QMutex>
#include <QHash>

typedef QHash<QByteArray, QDBusArgumentType> TypeSigHash;
typedef QHash<int, QDBusArgumentType> TypeIdHash;

static QMutex* sTypesLock;
static TypeSigHash* sTypesFromDBusSignature;
static TypeSigHash* sTypesFromCppSignature;
static TypeIdHash* sTypesFromId;

/* ------------------------------------------------------------------------- */

int qDBusLookupTypeId(int typeId, QDBusArgumentType* arg)
{
    if (!sTypesLock)
        return -1;

    QMutexLocker locker(sTypesLock);
    if (!sTypesFromId)
        return -1;

    TypeIdHash::iterator ii = sTypesFromId->find(typeId);
    if (ii == sTypesFromId->end())
        return -1;

    if (arg)
        *arg = ii.value();

    return 0;
}

/* ------------------------------------------------------------------------- */

const char* QDBusMetaType::typeToSignature(int typeId)
{
    QDBusArgumentType arg;
    if (qDBusLookupTypeId(typeId, &arg))
        return NULL;
    else
        return arg.dbusSignature.constData();
}

/* ------------------------------------------------------------------------- */

int qDBusLookupDBusSignature(const QByteArray& sig, QDBusArgumentType* arg)
{
    if (!sTypesLock)
        return -1;

    QMutexLocker lock(sTypesLock);
    if (!sTypesFromDBusSignature)
        return -1;

    TypeSigHash::iterator ii = sTypesFromDBusSignature->find(sig);
    if (ii == sTypesFromDBusSignature->end())
        return -1;

    if (arg)
        *arg = ii.value();
    
    return 0;
}

/* ------------------------------------------------------------------------- */

int qDBusLookupCppSignature(const QByteArray& cppsig, QDBusArgumentType* arg)
{
    if (!sTypesLock)
        return -1;

    QMutexLocker lock(sTypesLock);
    if (!sTypesFromCppSignature)
        return -1;

    TypeSigHash::iterator ii = sTypesFromCppSignature->find(cppsig);
    if (ii == sTypesFromCppSignature->end())
        return -1;

    if (arg)
        *arg = ii.value();

    return 0;
}

/* ------------------------------------------------------------------------- */

int qDBusLookupParameters(const QMetaMethod& method, QList<QDBusArgumentType>* args)
{
    const char* returnSig = method.typeName();
    if (returnSig && *returnSig) {
        args->append(QDBusArgumentType());
        if (qDBusLookupCppSignature(returnSig, &args->back()))
            return -1;
    } else {
        // Inject a invalid type as the first so that the void** array created
        // by QDBusArgumentList has a NULL as its first void*
        args->append(QDBusArgumentType());
    }

    QList<QByteArray> types = method.parameterTypes();
    for (int i = 0; i < types.size(); i++)
    {
        args->append(QDBusArgumentType());
        if (qDBusLookupCppSignature(types[i], &args->back()))
            return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

void QDBusMetaType::registerMarshallOperators(int typeId, MarshallFunction marshall, DemarshallFunction demarshall)
{
    if (!sTypesLock)
        sTypesLock = new QMutex(QMutex::Recursive);

    QMutexLocker lock(sTypesLock);
    if (!sTypesFromCppSignature)
        sTypesFromCppSignature = new TypeSigHash;
    if (!sTypesFromDBusSignature)
        sTypesFromDBusSignature = new TypeSigHash;
    if (!sTypesFromId)
        sTypesFromId = new TypeIdHash;


    QDBusArgumentType arg;
    arg.typeId = typeId;
    arg.marshall = marshall;
    arg.demarshall = demarshall;

    // Run the marshall function with a default constructed type to try and
    // figure out the dbus signature.
    void* defaultArg = QMetaType::construct(typeId);
    if (!defaultArg)
        return;
    adbus_Buffer* buf = adbus_buf_new();
    QDBusArgumentPrivate::Marshall(buf, marshall, defaultArg);
    arg.dbusSignature = adbus_buf_sig(buf, NULL);
    adbus_buf_free(buf);
    QMetaType::destroy(typeId, defaultArg);

    // Add base type (ie bool)
    arg.cppSignature = QMetaType::typeName(typeId);
    arg.isReturn = false;
    sTypesFromCppSignature->insert(arg.cppSignature, arg);
    sTypesFromDBusSignature->insert(arg.dbusSignature, arg);
    sTypesFromId->insert(typeId, arg);

    // Add const ref type (ie const bool&)
    arg.cppSignature = "const ";
    arg.cppSignature += QMetaType::typeName(typeId);
    arg.cppSignature += "&";
    arg.isReturn = false;
    sTypesFromCppSignature->insert(arg.cppSignature, arg);

    // Ad ref type (ie bool&)
    arg.cppSignature = QMetaType::typeName(typeId);
    arg.cppSignature += "&";
    arg.isReturn = true;
    sTypesFromCppSignature->insert(arg.cppSignature, arg);
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

static void** ConstructArguments(const QList<QDBusArgumentType>& types)
{
    void** ret = (void**) malloc(sizeof(void*) * types.size());

    for (int i = 0; i < types.size(); i++)
    {
        if (types[i].typeId > 0) {
            ret[i] = QMetaType::construct(types[i].typeId);
        } else {
            ret[i] = NULL;
        }
    }

    return ret;
}

/* ------------------------------------------------------------------------- */

QDBusArgumentList::QDBusArgumentList()
: m_Args(NULL)
{
}

/* ------------------------------------------------------------------------- */

void QDBusArgumentList::init(const QDBusArgumentType& type)
{
    Q_ASSERT(m_Types.isEmpty() && !m_Args);
    m_Types.append(type);
    m_Args = ConstructArguments(m_Types);
}

/* ------------------------------------------------------------------------- */

void QDBusArgumentList::init(const QList<QDBusArgumentType>& types)
{
    Q_ASSERT(m_Types.isEmpty() && !m_Args);
    m_Types = types;
    m_Args = ConstructArguments(m_Types);
}

/* ------------------------------------------------------------------------- */

QDBusArgumentList::~QDBusArgumentList()
{
    for (int i = 0; i < m_Types.size(); i++)
    {
        if (m_Args[i] && m_Types[i].typeId > 0) {
            QMetaType::destroy(m_Types[i].typeId, m_Args[i]);
        }
    }

    free(m_Args);
}

/* ------------------------------------------------------------------------- */

int QDBusArgumentList::SetProperty(adbus_Iterator* iter)
{
    Q_ASSERT(m_Types.size() == 1);

    const QDBusArgumentType* t = &m_Types[0];
    if (t->typeId <= 0 || !t->demarshall)
        return -1;

    if (QDBusArgumentPrivate::Demarshall(iter, t->demarshall, m_Args[0]))
        return -1;

    return 0;
}

/* ------------------------------------------------------------------------- */

void QDBusArgumentList::GetProperty(adbus_Buffer* buf)
{
    Q_ASSERT(m_Types.size() == 1);
    const QDBusArgumentType* t = &m_Types[0];
    if (t->typeId > 0 && t->marshall)
        QDBusArgumentPrivate::Marshall(buf, t->marshall, m_Args[0]);
}

/* ------------------------------------------------------------------------- */

int QDBusArgumentList::GetArguments(adbus_Iterator* iter)
{
    for (int i = 0; i < m_Types.size(); i++)
    {
        const QDBusArgumentType* t = &m_Types[i];
        if (!t->isReturn && t->typeId > 0 && t->demarshall) {
            if (QDBusArgumentPrivate::Demarshall(iter, t->demarshall, m_Args[i]))
                return -1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

void QDBusArgumentList::GetReturns(adbus_Buffer* buf)
{
    for (int i = 0; i < m_Types.size(); i++)
    {
        const QDBusArgumentType* t = &m_Types[i];
        if (t->isReturn && t->typeId > 0 && t->marshall) {
            QDBusArgumentPrivate::Marshall(buf, t->marshall, m_Args[i]);
        }
    }
}
















/* ------------------------------------------------------------------------- */

// TODO
void QDBusObjectPath::check()
{}

void QDBusSignature::check()
{}


