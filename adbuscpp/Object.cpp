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

#include "Object.h"

#include "adbus/CommonMessages.h"

#include <assert.h>


using namespace adbus;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

Object::Object()
:   m_Object(NULL),
    m_Connection(NULL)
{
}

// ----------------------------------------------------------------------------

Object::~Object()
{
    deregisterObject();
}

// ----------------------------------------------------------------------------

void Object::registerObject(Connection* connection, const std::string& path)
{
    deregisterObject();

    m_Connection = connection;
    m_Path = path;

    ADBusConnection* c = connection->m_C;

    m_Object = ADBusAddObject(c, m_Path.c_str(), m_Path.size());
}

// ----------------------------------------------------------------------------

void Object::deregisterObject()
{
    if (!m_Connection || !m_Object)
        return;

    struct ADBusConnection* connection = m_Connection->m_C;
    for (size_t i = 0; i < m_Matches.size(); ++i)
        ADBusRemoveMatch(connection, m_Matches[i]);
    m_Matches.clear();

    for (size_t i = 0; i < m_Interfaces.size(); ++i)
        ADBusUnbindInterface(m_Object, m_Interfaces[i]->m_I);
    m_Interfaces.clear();

    m_Connection = NULL;
    m_Object = NULL;
    m_Path.clear();
}

// ----------------------------------------------------------------------------

std::string Object::childPath(const std::string& child)
{
    assert(!child.empty() && child[0] != '/');
    if (!m_Path.empty() && m_Path[m_Path.size() - 1] == '/')
        return m_Path + child;
    else
        return m_Path + '/' + child;
}

// ----------------------------------------------------------------------------

static void CopyString(
        const std::string&  source,
        const char**        target,
        int*                targetSize)
{
    if (source.empty())
        return;

    *target = source.c_str();
    *targetSize = source.size();
}

void Object::addMatch(
        Match*                  m,
        ADBusMessageCallback    function,
        struct ADBusUser*       user1,
        struct ADBusUser*       user2)
{
    ADBusMatch adbusmatch;
    ADBusMatchInit(&adbusmatch);

    adbusmatch.type = m->type;
    adbusmatch.addMatchToBusDaemon = m->addMatchToBusDaemon;
    adbusmatch.removeOnFirstMatch = m->removeOnFirstMatch;
    CopyString(m->sender, &adbusmatch.sender, &adbusmatch.senderSize);
    CopyString(m->destination, &adbusmatch.destination, &adbusmatch.destinationSize);
    CopyString(m->interface, &adbusmatch.interface, &adbusmatch.interfaceSize);
    CopyString(m->path, &adbusmatch.path, &adbusmatch.pathSize);
    CopyString(m->member, &adbusmatch.member, &adbusmatch.memberSize);
    CopyString(m->errorName, &adbusmatch.errorName, &adbusmatch.errorNameSize);

    adbusmatch.callback = function;
    adbusmatch.user1 = user1;
    adbusmatch.user2 = user2;

    ADBusConnection* connection = m_Connection->m_C;
    uint32_t matchId = ADBusAddMatch(connection, &adbusmatch);
    m_Matches.push_back(matchId);
}

// ----------------------------------------------------------------------------

void Object::doBind(Interface* interface, struct ADBusUser* user2)
{
    ADBusInterface* adbusinterface = interface->m_I;
    int err = ADBusBindInterface(m_Object, adbusinterface, user2);
    if (!err)
        m_Interfaces.push_back(interface);
}

