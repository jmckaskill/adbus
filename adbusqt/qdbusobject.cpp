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
#include "qdbusconnection_p.hxx"
#include <QtCore/qcoreapplication.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qthread.h>
#include <QtXml/qdom.h>

/* ------------------------------------------------------------------------- */

QDBusProxy::QDBusProxy()
: m_Msg(NULL)
{}

QDBusProxy::~QDBusProxy()
{ adbus_msg_free(m_Msg); }

/* ------------------------------------------------------------------------- */

QEvent::Type QDBusProxyEvent::type = (QEvent::Type) QEvent::registerEventType();

// Called on the connection thread
void QDBusProxy::ProxyCallback(void* user, adbus_Callback cb, adbus_Callback release, void* cbuser)
{
    QDBusProxy* s = (QDBusProxy*) user;

    if (QThread::currentThread() == s->thread()) {
        cb(cbuser);

    } else {
        QDBusProxyEvent* e = new QDBusProxyEvent;
        e->cb = cb;
        e->user = cbuser;
        e->release = release;

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
        if (e->cb) {
            e->cb(e->user);
        }
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

void QDBusObject::Unregister(void* u)
{
    QDBusObject* d = (QDBusObject*) u;

    // Remove all pending binds, matches, and replies
    QDBusMatchData* m;
    DIL_FOREACH(QDBusMatchData, m, &d->m_Matches, hl) {
        adbus_conn_removematch(d->m_Connection, m->connMatch);
    }

    QDBusReplyData* r;
    DIL_FOREACH(QDBusReplyData, r, &d->m_Replies, hl) {
        adbus_conn_removereply(d->m_Connection, r->connReply);
    }

    QDBusBindData* b;
    DIL_FOREACH(QDBusBindData, b, &d->m_Binds, hl) {
        adbus_conn_unbind(d->m_Connection, b->connBind);
    }
}

void QDBusObject::Delete(void* u)
{
    QDBusObject* d = (QDBusObject*) u;

    QDBusMatchData* m;
    DIL_FOREACH(QDBusMatchData, m, &d->m_Matches, hl) {
        delete m;
    }

    QDBusReplyData* r;
    DIL_FOREACH(QDBusReplyData, r, &d->m_Replies, hl) {
        delete r;
    }

    QDBusBindData* b;
    DIL_FOREACH(QDBusBindData, b, &d->m_Binds, hl) {
        delete b;
    }

    delete d;
}

void QDBusObject::destroy()
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (m_Tracked) {
        QDBusConnectionPrivate::RemoveObject(m_QConnection, m_Tracked);
    }

    // Kill all incoming events from the connection thread and stop new ones
    // from coming in. The data in those messages will still be freed since
    // the dtor is still called, which calls the supplied release callback.
    setParent(NULL);
    moveToThread(NULL);

    // Delete the object on the proxy thread - this ensures that it receives all
    // of our messages up to this point safely and removing services can only be
    // done on the connection thread.
    adbus_conn_proxy(m_Connection, &QDBusObject::Unregister, &QDBusObject::Delete, this);
}

/* ------------------------------------------------------------------------- */

int QDBusObject::MatchCallback(adbus_CbData* d)
{
    QDBusMatchData* data = (QDBusMatchData*) d->user1;
    QDBusMessage& msg    = data->owner->m_CurrentMessage;

    /* We can get messages after the match has been removed, since we have to
     * just across to the connection thread to remove the match.
     */
    if (data->methodIndex >= 0) {
        adbus_Iterator argiter = {};
        adbus_iter_args(&argiter, d->msg);

        /* Check that the message could be parsed correctly */
        if (QDBusMessagePrivate::FromMessage(msg, d->msg))
            return -1;

        /* Check that we can convert the arguments correctly */
        if (data->arguments.setupMetacall(&msg)) {
            data->object->qt_metacall(
                    QMetaObject::InvokeMetaMethod,
                    data->methodIndex,
                    data->arguments.metacallData());
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

int QDBusObject::ReplyCallback(adbus_CbData* d)
{
    QDBusReplyData* data = (QDBusReplyData*) d->user1;
    QDBusMessage& msg    = data->owner->m_CurrentMessage;

    /* We always add the reply callback, even if the user didn't set a reply
     * callback, so that we can remove the reply data.
     */
    if (data->methodIndex >= 0) {
        adbus_Iterator argiter = {};
        adbus_iter_args(&argiter, d->msg);

        /* Check that the message could be parsed correctly */
        if (QDBusMessagePrivate::FromMessage(msg, d->msg, data->arguments))
            return -1;

        /* Check that we can convert the arguments correctly */
        if (data->arguments.setupMetacall(&msg)) {
            data->object->qt_metacall(
                    QMetaObject::InvokeMetaMethod,
                    data->methodIndex,
                    data->arguments.metacallData());
        }
    }

    dil_remove(QDBusReplyData, data, &data->hl);
    delete data;

    return 0;
}

/* ------------------------------------------------------------------------- */

int QDBusObject::ErrorCallback(adbus_CbData* d)
{
    QDBusReplyData* data = (QDBusReplyData*) d->user1;
    QDBusMessage& msg    = data->owner->m_CurrentMessage;

    /* We always add the reply callback, even if the user didn't set a reply
     * callback, so that we can remove the reply data.
     */
    if (data->errorIndex >= 0) {
        /* Check that the message could be parsed correctly */
        if (QDBusMessagePrivate::FromMessage(msg, d->msg))
            return -1;

        QDBusError err(msg);

        // Error methods are void (QDBusError, QDBusMessage)
        void* args[] = {0, &err, &msg};

        data->object->qt_metacall(
                QMetaObject::InvokeMetaMethod,
                data->errorIndex,
                args);
    }

    dil_remove(QDBusReplyData, data, &data->hl);
    delete data;

    return 0;
}

/* ------------------------------------------------------------------------- */

int QDBusObject::MethodCallback(adbus_CbData* d)
{
    QDBusMethodData* method = (QDBusMethodData*) d->user1;
    QDBusBindData* bind     = (QDBusBindData*) d->user2;
    QDBusMessage&  msg      = bind->owner->m_CurrentMessage;

    Q_ASSERT(method->methodIndex >= 0);

    adbus_Iterator argiter = {};
    adbus_iter_args(&argiter, d->msg);

    /* Check that the message could be parsed correctly */
    if (QDBusMessagePrivate::FromMessage(msg, d->msg, method->arguments))
        return -1;

    /* Check that we can convert the arguments correctly */
    if (method->arguments.setupMetacall(&msg)) {
        bind->object->qt_metacall(
                QMetaObject::InvokeMetaMethod,
                method->methodIndex,
                method->arguments.metacallData());

    } else if (d->ret) {
        return adbus_error_argument(d);
    }


    QDBusMessagePrivate::GetReply(msg, &d->ret, method->arguments);

    return 0;
}

/* ------------------------------------------------------------------------- */

int QDBusObject::GetPropertyCallback(adbus_CbData* d)
{
    QDBusPropertyData* prop = (QDBusPropertyData*) d->user1;
    QDBusUserData* bind     = (QDBusUserData*) d->user2;

    Q_ASSERT(prop->propIndex >= 0);

    bind->object->qt_metacall(
            QMetaObject::ReadProperty,
            prop->propIndex,
            &prop->data);

    prop->type->marshall(d->getprop, prop->data);

    return 0;
}

/* ------------------------------------------------------------------------- */

int QDBusObject::SetPropertyCallback(adbus_CbData* d)
{
    QDBusPropertyData* prop = (QDBusPropertyData*) d->user1;
    QDBusUserData* bind     = (QDBusUserData*) d->user2;

    Q_ASSERT(prop->propIndex >= 0);

    if (prop->type->demarshall(&d->setprop, prop->data))
        return -1;

    bind->object->qt_metacall(
            QMetaObject::WriteProperty,
            prop->propIndex,
            &prop->data);

    return 0;
}

/* ------------------------------------------------------------------------- */

void QDBusObject::DoAddReply(void* u)
{
    QDBusReplyData* d = (QDBusReplyData*) u;
    d->connReply = adbus_conn_addreply(d->connection, &d->reply);
}

bool QDBusObject::addReply(const QByteArray& remote, uint32_t serial, QObject* receiver, const char* returnMethod, const char* errorMethod)
{
    Q_ASSERT(QThread::currentThread() == thread());
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

    QDBusReplyData* d = new QDBusReplyData;

    if (returnIndex >= 0 && !d->arguments.init(meta->method(returnIndex))) {
        delete d;
        return false;
    }

    d->methodIndex = returnIndex;
    d->errorIndex = errorIndex;
    d->object = receiver;
    d->owner = this;

    d->remote = remote;
    d->reply.remote = d->remote.constData();
    d->reply.remoteSize = d->remote.size();

    d->reply.serial = serial;

    /* Always add the reply callbacks even if returnIndex is invalid, since
     * the reply callback also removes and frees the reply data.
     */

    d->reply.callback = &ReplyCallback;
    d->reply.cuser = d;

    d->reply.error = &ErrorCallback;
    d->reply.euser = d;
    
    d->reply.proxy = &ProxyMsgCallback;
    d->reply.puser = this;

    dil_insert_after(QDBusReplyData, &m_Replies, d, &d->hl);
    adbus_conn_proxy(m_Connection, &DoAddReply, NULL, d);

    return true;
}

/* ------------------------------------------------------------------------- */

void QDBusObject::DoAddMatch(void* u)
{
    QDBusMatchData* d = (QDBusMatchData*) u;
    d->connMatch = adbus_conn_addmatch(d->connection, &d->match);
}

void QDBusObject::ReleaseMatch(void* u)
{
    QDBusMatchData* d = (QDBusMatchData*) u;
    d->connMatch = NULL;
}

bool QDBusObject::addMatch(
        const QByteArray& service,
        const QByteArray& path,
        const QByteArray& interface,
        const QByteArray& name,
        QObject* receiver,
        const char* slot)
{
    Q_ASSERT(QThread::currentThread() == thread());
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

    QDBusMatchData* d = new QDBusMatchData;

    if (!d->arguments.init(metaMethod)) {
        delete d;
        return false;
    }

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

    d->match.callback = &MatchCallback;
    d->match.cuser = d;
    
    d->match.proxy = &ProxyMsgCallback;
    d->match.puser = this;

    d->match.release[0] = &ReleaseMatch;
    d->match.ruser[0] = d;

    dil_insert_after(QDBusMatchData, &m_Matches, d, &d->hl);
    adbus_conn_proxy(m_Connection, &DoAddMatch, NULL, d);

    return true;
}

/* ------------------------------------------------------------------------- */

void QDBusObject::DoRemoveMatch(void* u)
{
    QDBusMatchData* d = (QDBusMatchData*) u;
    adbus_conn_removematch(d->owner->m_Connection, d->connMatch);
    d->connMatch = NULL;
}

void QDBusObject::removeMatch(
        const QByteArray&   service,
        const QByteArray&   path,
        const QByteArray&   interface,
        const QByteArray&   name,
        QObject*            receiver,
        const char*         slot)
{
    // 1. Find the match data.
    // 2. Set the methodIndex to -1 so MatchCallback will not call the
    // callback.
    // 3. Send a request to the connection thread to remove the match
    // 4. (On connection thread) Unset connMatch so we don't remove it on destruction
    // 5. Leave it in m_Matches for the data to be freed at destruction time
    
    // Ideally we would want to remove it from m_Matches, but this is a bit
    // tricky, we would have to send the request to the connection thread to
    // remove it, get a response back and then remove it.

    Q_ASSERT(QThread::currentThread() == thread());
    QDBusMatchData* d;
    DIL_FOREACH (QDBusMatchData, d, &m_Matches, hl) {
        if (    d->sender == service 
            &&  d->path == path
            &&  d->interface == interface
            &&  d->member == name
            &&  d->object == receiver 
            &&  d->slot == slot)
        {
            d->methodIndex = -1;
            adbus_conn_proxy(m_Connection, &DoRemoveMatch, NULL, d);
            return;
        }
    }
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

    QList<QByteArray> names = method.parameterNames();

    switch (type)
    {
    case QMetaMethod::Method:
    case QMetaMethod::Slot:
        {
            if (access < QMetaMethod::Public)
                return NULL;

            QDBusMethodData* d = new QDBusMethodData;

            if (!d->arguments.init(method)) {
                delete d;
                return NULL;
            }

            d->methodIndex = methodIndex;

            adbus_Member* mbr = adbus_iface_addmethod(iface, name, int(nend - name));

            for (int i = 0; i < d->arguments.m_Args.size(); i++) {

                QDBusArgumentList::Entry* e = &d->arguments.m_Args[i];
                const QByteArray& dbus = e->type->m_DBusSignature;

                if (e->inarg) {
                    adbus_mbr_argsig(mbr, dbus.constData(), dbus.size());
                    adbus_mbr_argname(mbr, e->name.constData(), e->name.size());
                } else {
                    adbus_mbr_retsig(mbr, dbus.constData(), dbus.size());
                    adbus_mbr_retname(mbr, e->name.constData(), e->name.size());
                }
            }

            adbus_mbr_setmethod(mbr, &QDBusObject::MethodCallback, d);
            adbus_mbr_addrelease(mbr, &QDBusUserData::Free, d);

            return mbr;
        }

#if 0
        // TODO
    case QMetaMethod::Signal:
        {
            if (access != QMetaMethod::Protected)
                return NULL;

            adbus_Member* mbr = adbus_iface_addsignal(iface, name, int(nend - name));

            for (int i = 0; i < arguments.size(); i++) {
                QDBusArgumentType* a = &arguments[i];
                if (a->isReturn)
                    continue;

                adbus_mbr_argsig(mbr, a->dbusSignature.constData(), a->dbusSignature.size());
                adbus_mbr_argname(mbr, names[i].constData(), names[i].size());
            }

            return mbr;
        }
#endif

    default:
        return NULL;
    }
}

/* ------------------------------------------------------------------------- */

static adbus_Member* AddProperty(adbus_Interface* iface, QMetaProperty prop, int propertyIndex)
{
    QDBusArgumentType* type = QDBusArgumentType::Lookup(prop.type());
    if (!type)
        return NULL;

    if (!(prop.isValid() || prop.isWritable()))
        return NULL;

    QDBusPropertyData* d = new QDBusPropertyData;
    d->propIndex    = propertyIndex;
    d->type         = type;
    d->data         = QMetaType::construct(prop.type());

    adbus_Member* mbr = adbus_iface_addproperty(
            iface,
            prop.name(),
            -1,
            type->m_DBusSignature.constData(),
            type->m_DBusSignature.size());

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
    d->connBind = adbus_conn_bind(d->connection, &d->bind);
}

void QDBusObject::FreeDoBind(void* u)
{
    QDBusBindData* d = (QDBusBindData*) u;
    adbus_iface_deref(d->bind.interface);
}

void QDBusObject::ReleaseBind(void* u)
{
    QDBusBindData* d = (QDBusBindData*) u;
    d->connBind = NULL;
}

bool QDBusObject::bindFromMetaObject(const QByteArray& path, QObject* object, QDBusConnection::RegisterOptions options)
{
    Q_ASSERT(QThread::currentThread() == thread());
    // See if we have to export anything
    if ((options & QDBusConnection::ExportAllContents) == 0)
        return true;

    const QMetaObject* meta;
    for (meta = object->metaObject(); meta != NULL; meta = meta->superClass()) {
        // interface is deref'd in FreeDoBind
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

        d->bind.interface = iface;
        d->bind.cuser2 = d;

        d->bind.proxy = &ProxyMsgCallback;
        d->bind.puser = this;

        d->bind.release[0] = &ReleaseBind;
        d->bind.ruser[0] = d;

        dil_insert_after(QDBusBindData, &m_Binds, d, &d->hl);
        adbus_conn_proxy(m_Connection, &DoBind, &FreeDoBind, d);
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
    Q_ASSERT(QThread::currentThread() == thread());
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

    d->bind.proxy = &ProxyMsgCallback;
    d->bind.puser = this;

    d->bind.release[0] = &ReleaseBind;
    d->bind.ruser[0] = d;

    dil_insert_after(QDBusBindData, &m_Binds, d, &d->hl);
    adbus_conn_proxy(m_Connection, &DoBind, &FreeDoBind, d);

    return true;
}

/* ------------------------------------------------------------------------- */

static QEvent::Type sThreadChangeCompleteEvent = (QEvent::Type) QEvent::registerEventType();

bool QDBusObject::event(QEvent* e)
{
    if (e->type() == sThreadChangeCompleteEvent) {
        Q_ASSERT(thread() == QThread::currentThread() && thread() == m_Tracked->thread());
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


