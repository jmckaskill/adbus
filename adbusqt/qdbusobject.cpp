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

#include "qdbusobject_p.hxx"
#include "qdbusmessage_p.hxx"
#include "qdbuserror_p.hxx"
#include "qdbusconnection_p.hxx"
#include <QCoreApplication>
#include <QMetaObject>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QThread>
#include <QDomDocument>
#include <QDomElement>

/* ------------------------------------------------------------------------- */

QDBusProxy::QDBusProxy()
: m_Msg(NULL)
{}

QDBusProxy::~QDBusProxy()
{ adbus_msg_free(m_Msg); }

/* ------------------------------------------------------------------------- */

QEvent::Type QDBusProxyEvent::type = (QEvent::Type) QEvent::registerEventType();

// Called on the connection thread
void QDBusProxy::ProxyCallback(void* user, adbus_Callback cb, void* cbuser)
{
    QDBusProxy* s = (QDBusProxy*) user;

    if (QThread::currentThread() == s->thread()) {
        cb(cbuser);

    } else {
        QDBusProxyEvent* e = new QDBusProxyEvent;
        e->cb = cb;
        e->user = cbuser;

        QCoreApplication::postEvent(s, e);
    }
}

/* ------------------------------------------------------------------------- */

QEvent::Type QDBusProxyMsgEvent::type = (QEvent::Type)QEvent::registerEventType();

QDBusProxyMsgEvent::~QDBusProxyMsgEvent()
{
    adbus_conn_deref(connection);
    adbus_freedata(&msg);
}

// Called on the connection thread
int QDBusProxy::ProxyMsgCallback(void* user, adbus_MsgCallback cb, adbus_CbData* d)
{
    QDBusProxy* s = (QDBusProxy*) user;

    if (QThread::currentThread() == s->thread()) {
        return adbus_dispatch(cb, d);

    } else {
        QDBusProxyMsgEvent* e = new QDBusProxyMsgEvent;
        e->cb = cb;
        e->connection = d->connection;
        e->user1 = d->user1;
        e->user2 = d->user2;

        adbus_clonedata(d->msg, &e->msg);
        adbus_conn_ref(e->connection);

        QCoreApplication::postEvent(s, e);

        // We will send the return on the other thread
        d->ret = NULL;
        return 0;
    }
}

/* ------------------------------------------------------------------------- */

// Called on the local thread
bool QDBusProxy::event(QEvent* event)
{
    if (event->type() == QDBusProxyEvent::type) {
        QDBusProxyEvent* e = (QDBusProxyEvent*) event;
        e->cb(e->user);
        return true;

    } else if (event->type() == QDBusProxyMsgEvent::type) {
        QDBusProxyMsgEvent* e = (QDBusProxyMsgEvent*) event;

        adbus_CbData d = {};
        d.connection = e->connection;
        d.msg   = &e->msg;
        d.user1 = e->user1;
        d.user2 = e->user2;

        if (e->ret) {
            if (!m_Msg) {
                m_Msg = adbus_msg_new();
            }
            d.ret = m_Msg;
            adbus_msg_reset(d.ret);
        }

        adbus_dispatch(e->cb, &d);

        if (d.ret) {
            adbus_msg_send(d.ret, d.connection);
        }

        return true;

    } else {
        return QObject::event(event);
    }
}














/* ------------------------------------------------------------------------- */

QDBusObject::QDBusObject(const QDBusConnection& connection, QObject* tracked)
:   m_QConnection(connection),
    m_Connection(QDBusConnectionPrivate::Connection(connection)),
    m_Tracked(tracked)
{
    // Filter events in order to get the ThreadChange event
    m_Tracked->installEventFilter(this);
    connect(m_Tracked, SIGNAL(destroyed()), this, SLOT(destroy()), Qt::DirectConnection);
}

void QDBusObject::Delete(void* u)
{
    QDBusObject* d = (QDBusObject*) u;

    // Remove all pending binds, matches, and replies
    QDBusUserData* i;
    DIL_FOREACH(QDBusUserData, i, &d->m_Matches, hl) {
        adbus_conn_removematch(d->m_Connection, ((QDBusMatchData*) d)->connMatch);
    }
    Q_ASSERT(dil_isempty(&d->m_Matches));

    DIL_FOREACH(QDBusUserData, i, &d->m_Replies, hl) {
        adbus_conn_removereply(d->m_Connection, ((QDBusReplyData*) d)->connReply);
    }
    Q_ASSERT(dil_isempty(&d->m_Replies));

    DIL_FOREACH(QDBusUserData, i, &d->m_Binds, hl) {
        adbus_conn_unbind(d->m_Connection, ((QDBusBindData*) d)->connBind);
    }
    Q_ASSERT(dil_isempty(&d->m_Binds));

    delete d;
}

void QDBusObject::destroy()
{
    if (m_Tracked) {
        QDBusConnectionPrivate::RemoveObject(m_QConnection, m_Tracked);
    }

    // Kill all incoming events from the connection thread
    setParent(NULL);
    moveToThread(NULL);

    // Delete the object on the proxy thread - this ensures that it receives all
    // of our messages up to this point safely and removing services can only be
    // done on the connection thread.
    adbus_conn_proxy(m_Connection, &QDBusObject::Delete, this);
}

/* ------------------------------------------------------------------------- */

int QDBusObject::ReplyCallback(adbus_CbData* d)
{
    QDBusUserData* data = (QDBusUserData*) d->user1;

    adbus_Iterator argiter = {};
    adbus_iter_args(&argiter, d->msg);

    if (data->argList.GetArguments(&argiter))
        return -1;

    data->object->qt_metacall(QMetaObject::InvokeMetaMethod, data->methodIndex, data->argList.Data());

    return 0;
}

/* ------------------------------------------------------------------------- */

int QDBusObject::ErrorCallback(adbus_CbData* d)
{
    QDBusUserData* data = (QDBusUserData*) d->user1;

    DBusError dbusError = {d->msg};
    QDBusError err(&dbusError);

    QDBusMessage msg;
    if (QDBusMessagePrivate::Copy(d->msg, &msg))
        return -1;

    // Error methods are void (QDBusError, QDBusMessage)
    void* args[] = {0, &err, &msg};
    data->object->qt_metacall(QMetaObject::InvokeMetaMethod, data->errorIndex, args);

    return 0;
}

/* ------------------------------------------------------------------------- */

int QDBusObject::MethodCallback(adbus_CbData* d)
{
    QDBusUserData* method = (QDBusUserData*) d->user1;
    QDBusUserData* bind   = (QDBusUserData*) d->user2;

    adbus_Iterator argiter = {};
    adbus_iter_args(&argiter, d->msg);

    if (method->argList.GetArguments(&argiter))
        return -1;

    bind->object->qt_metacall(QMetaObject::InvokeMetaMethod, method->methodIndex, method->argList.Data());

    if (d->ret) {
        method->argList.GetReturns(adbus_msg_argbuffer(d->ret));
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

int QDBusObject::GetPropertyCallback(adbus_CbData* d)
{
    QDBusUserData* prop = (QDBusUserData*) d->user1;
    QDBusUserData* bind = (QDBusUserData*) d->user2;

    bind->object->qt_metacall(QMetaObject::ReadProperty, prop->methodIndex, prop->argList.Data());
    prop->argList.GetProperty(d->getprop);

    return 0;
}

/* ------------------------------------------------------------------------- */

int QDBusObject::SetPropertyCallback(adbus_CbData* d)
{
    QDBusUserData* prop = (QDBusUserData*) d->user1;
    QDBusUserData* bind = (QDBusUserData*) d->user2;

    if (prop->argList.SetProperty(&d->setprop))
        return -1;

    bind->object->qt_metacall(QMetaObject::WriteProperty, prop->methodIndex, prop->argList.Data());

    return 0;
}

/* ------------------------------------------------------------------------- */

void QDBusObject::DoAddReply(void* u)
{
    QDBusReplyData* d = (QDBusReplyData*) u;
    dil_insert_after(QDBusUserData, &d->owner->m_Replies, d, &d->hl);
    d->connReply = adbus_conn_addreply(d->connection, &d->reply);
}

bool QDBusObject::addReply(const QByteArray& remote, uint32_t serial, QObject* receiver, const char* returnMethod, const char* errorMethod)
{
    int returnIndex = -1;
    int errorIndex = -1;
    const QMetaObject* meta = receiver->metaObject();

    if (returnMethod && *returnMethod) {
        // If the method is set it must be a valid slot
        if (returnMethod[0] != '1') {
            return false;
        }
        returnMethod++;
        returnIndex = meta->indexOfSlot(returnMethod);
        if (returnIndex < 0) {
            returnIndex = meta->indexOfSlot(QMetaObject::normalizedSignature(returnMethod).constData());
            if (returnIndex < 0) {
                return false;
            }
        }
    }

    if (errorMethod && *errorMethod) {
        if (errorMethod[0] != '1') {
            return false;
        }
        errorMethod++;
        errorIndex = meta->indexOfSlot(errorMethod);
        if (errorIndex < 0) {
            errorIndex = meta->indexOfSlot(QMetaObject::normalizedSignature(errorMethod).constData());
            if (errorIndex < 0) {
                return false;
            }
        }
    }

    if (returnIndex < 0 && errorIndex < 0)
        return false;

    QList<QDBusArgumentType> types;
    if (returnIndex >= 0 && qDBusLookupParameters(meta->method(returnIndex), &types))
        return false;

    QDBusReplyData* d = new QDBusReplyData;
    d->argList.init(types);
    d->methodIndex = returnIndex;
    d->errorIndex = errorIndex;
    d->object = receiver;
    d->owner = this;

    d->remote = remote;
    d->reply.remote = d->remote.constData();
    d->reply.remoteSize = d->remote.size();

    d->reply.serial = serial;

    d->reply.release[0] = &QDBusReplyData::Free;
    d->reply.ruser[0] = d;

    d->reply.cuser = d;
    d->reply.euser = d;
    
    d->reply.proxy = &ProxyMsgCallback;
    d->reply.puser = this;

    if (returnIndex >= 0)
        d->reply.callback = &ReplyCallback;

    if (errorIndex >= 0)
        d->reply.error = &ErrorCallback;

    adbus_conn_proxy(m_Connection, &DoAddReply, d);

    return true;
}

/* ------------------------------------------------------------------------- */

void QDBusObject::DoAddMatch(void* u)
{
    QDBusMatchData* d = (QDBusMatchData*) u;
    dil_insert_after(QDBusUserData, &d->owner->m_Matches, d, &d->hl);
    d->connMatch = adbus_conn_addmatch(d->connection, &d->match);
}

bool QDBusObject::addMatch(
        const QByteArray& service,
        const QByteArray& path,
        const QByteArray& interface,
        const QByteArray& name,
        QObject* receiver,
        const char* slot)
{
    if (!slot || !*slot)
        return false;

    const QMetaObject* meta = receiver->metaObject();
    QByteArray normalized = QMetaObject::normalizedSignature(slot);
    int methodIndex = meta->indexOfMethod(normalized.constData());

    if (methodIndex < 0)
        return false;

    QMetaMethod metaMethod = meta->method(methodIndex);
    if (metaMethod.methodType() != QMetaMethod::Slot && metaMethod.methodType() != QMetaMethod::Signal)
        return false;

    QList<QDBusArgumentType> types;
    if (qDBusLookupParameters(metaMethod, &types))
        return false;

    QDBusMatchData* d = new QDBusMatchData;
    d->argList.init(types);
    d->methodIndex = methodIndex;
    d->object = receiver;
    d->owner = this;

    d->sender = service;
    d->match.sender = d->sender.constData();
    d->match.senderSize = d->sender.size();

    d->path = path;
    d->match.path = d->path.constData();
    d->match.pathSize = d->path.size();

    if (!interface.isEmpty()) {
        d->interface = interface;
        d->match.interface = d->interface.constData();
        d->match.interfaceSize = d->interface.size();
    }

    d->member = name;
    d->match.member = d->member.constData();
    d->match.memberSize = d->member.size();

    d->match.release[0] = &QDBusUserData::Free;
    d->match.ruser[0] = d;

    d->match.callback = &ReplyCallback;
    d->match.cuser = d;
    
    d->match.proxy = &ProxyMsgCallback;
    d->match.puser = this;

    adbus_conn_proxy(m_Connection, &DoAddMatch, d);

    return true;
}

/* ------------------------------------------------------------------------- */

// Signals and methods both come through QMetaObject::method
static adbus_Member* AddMethod(adbus_Interface* iface, QMetaMethod method, int methodIndex)
{
    QMetaMethod::MethodType type = method.methodType();
    QMetaMethod::Access access = method.access();

    const char* name = method.signature();
    const char* nend = strchr(name, '(');
    if (nend == NULL)
        return NULL;

    QList<QDBusArgumentType> arguments;
    if (qDBusLookupParameters(method, &arguments))
        return NULL;

    QList<QByteArray> names = method.parameterNames();
    if (names.size() != arguments.size())
        return NULL;

    switch (type)
    {
    case QMetaMethod::Method:
    case QMetaMethod::Slot:
        {
            if (access < QMetaMethod::Public)
                return NULL;

            adbus_Member* mbr = adbus_iface_addmethod(iface, name, int(nend - name));

            for (int i = 0; i < arguments.size(); i++) {
                QDBusArgumentType* a = &arguments[i];
                if (a->isReturn) {
                    adbus_mbr_retsig(mbr, a->dbusSignature.constData(), a->dbusSignature.size());
                    adbus_mbr_retsig(mbr, names[i].constData(), names[i].size());
                } else {
                    adbus_mbr_argsig(mbr, a->dbusSignature.constData(), a->dbusSignature.size());
                    adbus_mbr_argsig(mbr, names[i].constData(), names[i].size());
                }
            }

            QDBusUserData* d = new QDBusUserData;
            d->argList.init(arguments);
            d->methodIndex = methodIndex;

            adbus_mbr_setmethod(mbr, &QDBusObject::MethodCallback, d);
            adbus_mbr_addrelease(mbr, &QDBusUserData::Free, d);

            return mbr;
        }

    case QMetaMethod::Signal:
        {
            if (access != QMetaMethod::Protected)
                return NULL;

            adbus_Member* mbr = adbus_iface_addmethod(iface, name, int(nend - name));

            for (int i = 0; i < arguments.size(); i++) {
                QDBusArgumentType* a = &arguments[i];
                if (a->isReturn)
                    continue;

                adbus_mbr_argsig(mbr, a->dbusSignature.constData(), a->dbusSignature.size());
                adbus_mbr_argsig(mbr, names[i].constData(), names[i].size());
            }

            return mbr;
        }

    default:
        return NULL;
    }
}

/* ------------------------------------------------------------------------- */

static adbus_Member* AddProperty(adbus_Interface* iface, QMetaProperty prop, int propertyIndex)
{
    QDBusArgumentType arg;
    if (qDBusLookupCppSignature(prop.typeName(), &arg))
        return NULL;

    if (!prop.isValid() && !prop.isWritable())
        return NULL;

    QDBusUserData* d = new QDBusUserData;
    d->argList.init(arg);
    d->methodIndex = propertyIndex;

    adbus_Member* mbr = adbus_iface_addproperty(iface, prop.name(), -1, arg.dbusSignature.constData(), arg.dbusSignature.size());
    if (prop.isValid()) {
        adbus_mbr_setgetter(mbr, &QDBusObject::GetPropertyCallback, d);
    }
    if (prop.isWritable()) {
        adbus_mbr_setsetter(mbr, &QDBusObject::SetPropertyCallback, d);
    }
    adbus_mbr_addrelease(mbr, &QDBusUserData::Free, d);

    return mbr;
}

/* ------------------------------------------------------------------------- */

void QDBusObject::DoBind(void* u)
{
    QDBusBindData* d = (QDBusBindData*) u;
    dil_insert_after(QDBusUserData, &d->owner->m_Binds, d, &d->hl);
    d->connBind = adbus_conn_bind(d->connection, &d->bind);
    adbus_iface_deref(d->bind.interface);
}

bool QDBusObject::bindFromMetaObject(const QByteArray& path, QObject* object, QDBusConnection::RegisterOptions options)
{
    // See if we have to export anything
    if ((options & QDBusConnection::ExportAllContents) == 0)
        return true;

    const QMetaObject* meta;
    for (meta = object->metaObject(); meta != NULL; meta = meta->superClass()) {
        adbus_Interface* iface = adbus_iface_new(meta->className(), -1);

        int mbegin = meta->methodOffset();
        int mend = mbegin + meta->methodCount();
        for (int mi = mbegin; mi < mend; mi++)
            AddMethod(iface, meta->method(mi), mi);

        int pbegin = meta->propertyOffset();
        int pend = pbegin + meta->propertyCount();
        for (int pi = pbegin; pi < pend; pi++)
            AddProperty(iface, meta->property(pi), pi);

        QDBusBindData* d = new QDBusBindData;
        d->owner = this;
        d->object = object;

        d->path = path;
        d->bind.path = d->path.constData();
        d->bind.pathSize = d->path.size();

        // interface is freed in DoBind
        d->bind.interface = iface;
        d->bind.cuser2 = d;

        d->bind.release[0] = &QDBusBindData::Free;
        d->bind.ruser[0] = d; 

        d->bind.proxy = &ProxyMsgCallback;
        d->bind.puser = this;

        adbus_conn_proxy(m_Connection, &DoBind, d);
    }

    return true;
}

/* ------------------------------------------------------------------------- */

static int GetAttribute(const QDomElement& element, const char* field, QByteArray* data)
{
    QDomNode attr = element.attributes().namedItem(field);
    if (!attr.isAttr())
        return -1;

    *data = attr.toAttr().value().toUtf8();
    return 0;
}

static void GetAnnotations(adbus_Member* mbr, const QDomElement& xmlMember)
{
    for (QDomElement anno = xmlMember.firstChildElement("annotation");
         !anno.isNull();
         anno = anno.nextSiblingElement("annotation"))
    {
        QByteArray annoName, annoValue;
        if (GetAttribute(anno, "name", &annoName) || GetAttribute(anno, "value", &annoValue))
            continue;

        adbus_mbr_annotate(mbr,
                           annoName.constData(),
                           annoName.size(),
                           annoValue.constData(),
                           annoValue.size());

    }
}

bool QDBusObject::bindFromXml(const QByteArray& path, QObject* object, const char* xml)
{
    // Since the various callback use InvokeMetaMethod directly we need to at
    // least check the xml against the meta object. In fact its much easier to
    // pull the argument names and signatures out of the meta object.
    const QMetaObject* meta = object->metaObject();

    QDomDocument doc;
    if (!doc.setContent(QByteArray(xml), false))
        return false;

    QDomElement xmlInterface = doc.documentElement();
    if (xmlInterface.tagName() != "interface")
        return false;

    QByteArray ifaceName;
    if (GetAttribute(xmlInterface, "name", &ifaceName))
        return false;

    adbus_Interface* iface = adbus_iface_new(ifaceName.constData(), ifaceName.size());

    QDomElement member;
    for (QDomElement xmlMember = xmlInterface.firstChildElement();
         !xmlMember.isNull();
         xmlMember = xmlMember.nextSiblingElement())
    {
        QByteArray name;
        if (GetAttribute(xmlMember, "name", &name))
            continue;

        // Ignore the arguments and signatures given by the xml (its safer
        // and easier to just pull the types out of QMetaObject). We need
        // to pull out the annotations out of the xml.
        QString tag = xmlMember.tagName();
        if (tag == "method") {

            int methodIndex = meta->indexOfSlot(name.constData());
            if (methodIndex < 0)
                methodIndex = meta->indexOfMethod(name.constData());
            if (methodIndex < 0)
                continue;

            QMetaMethod metaMethod = meta->method(methodIndex);

            adbus_Member* mbr = AddMethod(iface, metaMethod, methodIndex);
            GetAnnotations(mbr, xmlMember);

        } else if (tag == "signal") {

            int sigIndex = meta->indexOfSignal(name.constData());
            if (sigIndex < 0)
                continue;

            QMetaMethod metaMethod = meta->method(sigIndex);

            adbus_Member* mbr = AddMethod(iface, metaMethod, sigIndex);
            GetAnnotations(mbr, xmlMember);

        } else if (tag == "property") {

            int propIndex = meta->indexOfProperty(name.constData());
            if (propIndex < 0)
                continue;

            QMetaProperty metaProperty = meta->property(propIndex);

            adbus_Member* mbr = AddProperty(iface, metaProperty, propIndex);
            GetAnnotations(mbr, xmlMember);
        }
    }

    QDBusBindData* d = new QDBusBindData;
    d->object = object;
    d->owner = this;

    d->path = path;
    d->bind.path = d->path.constData();
    d->bind.pathSize = d->path.size();

    // interface is freed in DoBind
    d->bind.interface = iface;
    d->bind.cuser2 = d;

    d->bind.release[0] = &QDBusUserData::Free;
    d->bind.ruser[0] = d; 

    d->bind.proxy = &ProxyMsgCallback;
    d->bind.puser = this;

    adbus_conn_proxy(m_Connection, &DoBind, d);

    return true;
}

/* ------------------------------------------------------------------------- */

static QEvent::Type sThreadChangeCompleteEvent = (QEvent::Type) QEvent::registerEventType();

bool QDBusObject::event(QEvent* e)
{
    if (e->type() == sThreadChangeCompleteEvent) {
        setParent(NULL);
        return true;
    } else {
        return QDBusProxy::event(e);
    }
}


bool QDBusObject::eventFilter(QObject* object, QEvent* event)
{
    Q_ASSERT(object == m_Tracked);

    if (event->type() == QEvent::ThreadChange) {
        // We get the thread change event before the actual thread change occurs.
        // We want to move with the object (ie whilst the lock in
        // QObject::moveToThread is still held), so that we don't get events on
        // the wrong thread. The only of doing this is to insert ourselves as a
        // child of the tracked object, and then remove ourselves after we've
        // moved.
        setParent(m_Tracked);
        // Post ourselves the thread change event which we will catch in
        // QDBusObject::event on the new thread.
        QCoreApplication::postEvent(this, new QEvent(sThreadChangeCompleteEvent));
    }

    return false;
}


