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
#include "qdbusmessage_p.h"
#include "qdbusconnection_p.h"
#include <QtCore/qcoreapplication.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qthread.h>
#include <QtXml/qdom.h>

/* ------------------------------------------------------------------------- */

QDBusObject::QDBusObject(const QDBusConnection& connection, QObject* tracked)
:   QDBusProxy(QDBusConnectionPrivate::Connection(connection)),
    m_QConnection(connection),
    m_Tracked(tracked)
{
    memset(&m_Binds, 0, sizeof(m_Binds));
    memset(&m_Replies, 0, sizeof(m_Replies));
    memset(&m_Matches, 0, sizeof(m_Matches));

    if (m_Tracked) {
        // Filter events in order to get the ThreadChange event
        m_Tracked->installEventFilter(this);
        connect(m_Tracked, SIGNAL(destroyed()), this, SLOT(destroyOnConnectionThread()), Qt::DirectConnection);
    }
}

void QDBusObject::unregister()
{
    if (m_Tracked) {
        QDBusConnectionPrivate::RemoveObject(m_QConnection, m_Tracked);
    }

    QDBusMatchData* m;
    DIL_FOREACH(QDBusMatchData, m, &m_Matches, hl) {
        adbus_conn_removematch(m_Connection, m->connMatch);
    }

    QDBusReplyData* r;
    DIL_FOREACH(QDBusReplyData, r, &m_Replies, hl) {
        adbus_conn_removereply(m_Connection, r->connReply);
    }

    QDBusBindData* b;
    DIL_FOREACH(QDBusBindData, b, &m_Binds, hl) {
        adbus_conn_unbind(m_Connection, b->connBind);
    }
}

QDBusObject::~QDBusObject()
{
    QDBusMatchData* m;
    DIL_FOREACH(QDBusMatchData, m, &m_Matches, hl) {
        delete m;
    }

    QDBusReplyData* r;
    DIL_FOREACH(QDBusReplyData, r, &m_Replies, hl) {
        delete r;
    }

    QDBusBindData* b;
    DIL_FOREACH(QDBusBindData, b, &m_Binds, hl) {
        delete b;
    }
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

        data->arguments.setupMetacall(msg);

        data->object->qt_metacall(
                QMetaObject::InvokeMetaMethod,
                data->methodIndex,
                data->arguments.metacallData());

        data->arguments.finishMetacall();
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

        data->arguments.setupMetacall(msg);

        data->object->qt_metacall(
                QMetaObject::InvokeMetaMethod,
                data->methodIndex,
                data->arguments.metacallData());

        data->arguments.finishMetacall();
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
        QDBusConnectionPrivate::SetLastError(data->owner->m_QConnection, err);

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

    method->arguments.setupMetacall(msg);

    bind->object->qt_metacall(
            QMetaObject::InvokeMetaMethod,
            method->methodIndex,
            method->arguments.metacallData());

    method->arguments.getReply(&d->ret);
    method->arguments.finishMetacall();

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

    prop->type->marshall(d->getprop, prop->data, false, false);

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

    d->connection = m_Connection;

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

    d->connection = m_Connection;

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

                if (e->type) {
                    if (e->direction == QDBusInArgument) {
                        adbus_mbr_argsig(mbr, dbus.constData(), dbus.size());
                        adbus_mbr_argname(mbr, e->name.constData(), e->name.size());
                    } else {
                        adbus_mbr_retsig(mbr, dbus.constData(), dbus.size());
                        adbus_mbr_retname(mbr, e->name.constData(), e->name.size());
                    }
                }
            }

            adbus_mbr_setmethod(mbr, &QDBusObject::MethodCallback, d);
            adbus_mbr_addrelease(mbr, &QDBusUserData::Free, d);

            return mbr;
        }

    case QMetaMethod::Signal:
        {
            if (access != QMetaMethod::Protected)
                return NULL;

            QList<QByteArray> cpptypes = method.parameterTypes();
            QList<QDBusArgumentType*> types;

            for (int i = 0; i < cpptypes.size(); i++) {
                QDBusArgumentDirection dir;
                QDBusArgumentType* t = QDBusArgumentType::FromCppType(cpptypes[i], &dir);
                if (!t || dir == QDBusOutArgument)
                    return NULL;

                types += t;
            }

            if (types.size() != names.size())
                return NULL;

            adbus_Member* mbr = adbus_iface_addsignal(iface, name, int(nend - name));

            for (int i = 0; i < types.size(); i++) {
                const QByteArray& sig = types[i]->m_DBusSignature;
                adbus_mbr_argsig(mbr, sig.constData(), sig.size());
                adbus_mbr_argname(mbr, names[i].constData(), names[i].size());
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
    QDBusArgumentType* type = QDBusArgumentType::FromMetatype(prop.type());
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

void QDBusObject::createSignals(QObject* obj, const QMetaObject* meta, QDBusBindData* bind)
{
    int mbegin = meta->methodOffset();
    int mend = meta->methodCount();
    for (int mi = mbegin; mi < mend; mi++) {
        QMetaMethod method = meta->method(mi);
        if (method.methodType() != QMetaMethod::Signal)
            continue;

        QByteArray name = method.signature();
        int nend = name.indexOf('(');
        if (nend < 0)
            continue;

        name.remove(nend, name.size() - nend);

        const adbus_Interface* iface = bind->bind.interface;
        const adbus_Member* mbr = adbus_iface_signal(iface, name.constData(), name.size());
        if (mbr == NULL)
            continue;

        QDBusSignal* wrapper = new QDBusSignal(m_Connection, bind, name, method, this);

        // SIGNAL(x) == "2x"
        name.prepend("2");
        connect(obj, name.constData(), wrapper, SLOT(trigger()));

        bind->sigs += wrapper;
    }
}

/* ------------------------------------------------------------------------- */

void QDBusObject::DoBind(void* u)
{
    QDBusBindData* d = (QDBusBindData*) u;
    d->connBind = adbus_conn_bind(d->connection, &d->bind);
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

    int ifaces = 0;
    const QMetaObject* meta;
    for (meta = object->metaObject(); meta != NULL; meta = meta->superClass()) {
        QByteArray name = "local.";
        name += meta->className();
        name.replace("::", ".");

        adbus_Interface* iface = adbus_iface_new(name.constData(), name.size());
        adbus_iface_ref(iface);

        int mbrs = 0;
        int mbegin = meta->methodOffset();
        int mend = meta->methodCount();
        for (int mi = mbegin; mi < mend; mi++) {
            if (AddMethod(iface, meta->method(mi), mi) != NULL) {
                mbrs++;
            }
        }

        int pbegin = meta->propertyOffset();
        int pend = meta->propertyCount();
        for (int pi = pbegin; pi < pend; pi++) {
            if (AddProperty(iface, meta->property(pi), pi) != NULL) {
                mbrs++;
            }
        }

        if (mbrs == 0) {
            adbus_iface_deref(iface);
            continue;
        }

        ifaces++;

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

        d->connection = m_Connection;

        createSignals(object, meta, d);

        dil_insert_after(QDBusBindData, &m_Binds, d, &d->hl);
        adbus_conn_proxy(m_Connection, &DoBind, NULL, d);
    }

    return ifaces > 0;
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
        QByteArray name, value;
        if (GetAttribute(anno, "name", &name) || GetAttribute(anno, "value", &value))
            continue;

        adbus_mbr_annotate(
                mbr,
                name.constData(),
                name.size(),
                value.constData(),
                value.size());

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
    adbus_iface_ref(iface);

    int mbrs = 0;
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
        // to pull the annotations from the xml.
        QString tag = xmlMember.tagName();
        if (tag == "method") {

            int methodIndex = meta->indexOfSlot(name.constData());
            if (methodIndex < 0)
                methodIndex = meta->indexOfMethod(name.constData());
            if (methodIndex < 0)
                continue;

            QMetaMethod metaMethod = meta->method(methodIndex);

            adbus_Member* mbr = AddMethod(iface, metaMethod, methodIndex);
            if (!mbr)
                continue;

            GetAnnotations(mbr, xmlMember);
            mbrs++;

        } else if (tag == "signal") {

            int sigIndex = meta->indexOfSignal(name.constData());
            if (sigIndex < 0)
                continue;

            QMetaMethod metaMethod = meta->method(sigIndex);

            adbus_Member* mbr = AddMethod(iface, metaMethod, sigIndex);
            if (!mbr)
                continue;

            GetAnnotations(mbr, xmlMember);
            mbrs++;

        } else if (tag == "property") {

            int propIndex = meta->indexOfProperty(name.constData());
            if (propIndex < 0)
                continue;

            QMetaProperty metaProperty = meta->property(propIndex);

            adbus_Member* mbr = AddProperty(iface, metaProperty, propIndex);
            if (!mbr)
                continue;

            GetAnnotations(mbr, xmlMember);
            mbrs++;
        }
    }

    if (mbrs == 0) {
        adbus_iface_deref(iface);
        return false;
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

    d->connection = m_Connection;

    createSignals(object, meta, d);

    dil_insert_after(QDBusBindData, &m_Binds, d, &d->hl);
    adbus_conn_proxy(m_Connection, &DoBind, NULL, d);

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

/* ------------------------------------------------------------------------- */

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










/* ------------------------------------------------------------------------- */

QDBusSignalBase::QDBusSignalBase(QObject* parent)
:   QObject(parent)
{}

void QDBusSignalBase::trigger()
{
    // This should've been intercepted in QDBusSignal::qt_metacall
    Q_ASSERT(false);
}

/* ------------------------------------------------------------------------- */

QDBusSignal::QDBusSignal(
        adbus_Connection*   connection,
        QDBusBindData*      bind,
        const QByteArray&   name,
        QMetaMethod         method,
        QObject*            parent)
:   QDBusSignalBase(parent),
    m_Connection(connection),
    m_Name(name),
    m_Message(adbus_msg_new()),
    m_Bind(bind)
{
    adbus_conn_ref(m_Connection);
    if (!m_Arguments.init(method)) {
        Q_ASSERT(false);
    }
}

/* ------------------------------------------------------------------------- */

QDBusSignal::~QDBusSignal()
{
    adbus_conn_deref(m_Connection);
    adbus_msg_free(m_Message);
}

/* ------------------------------------------------------------------------- */

void QDBusSignal::trigger(void** a)
{
    adbus_MsgFactory* m = m_Message;

    adbus_msg_reset(m);
    adbus_msg_settype(m, ADBUS_MSG_SIGNAL);
    adbus_msg_setflags(m, ADBUS_MSG_NO_REPLY);
    adbus_msg_setpath(m, m_Bind->path.constData(), m_Bind->path.size());
    adbus_msg_setinterface(m, m_Bind->interface.constData(), m_Bind->interface.size());
    adbus_msg_setmember(m, m_Name.constData(), m_Name.size());

    m_Arguments.appendArguments(m, a);

    adbus_msg_send(m, m_Connection);
}

/* ------------------------------------------------------------------------- */

int QDBusSignal::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: trigger(_a); break;
        default: ;
        }
        _id -= 1;
    }
    return _id;
}





