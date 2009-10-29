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
#include "adbus/Connection.h"

#include <assert.h>


using namespace adbus;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

Object::Object()
{
}

// ----------------------------------------------------------------------------

Object::~Object()
{
    removeAllMatches();
    unbindAll();
}

// ----------------------------------------------------------------------------

static void SetupString(
        const std::string&  source,
        const char**        target,
        int*                targetSize)
{
    if (source.empty())
        return;

    *target = source.c_str();
    *targetSize = (int) source.size();
}

uint32_t Object::addMatch(
        ADBusConnection*             c,
        Match*                  m,
        ADBusMessageCallback    function,
        struct ADBusUser*       user1,
        struct ADBusUser*       user2)
{
    ADBusMatch adbusmatch;
    ADBusInitMatch(&adbusmatch);

    adbusmatch.type = m->type;
    adbusmatch.addMatchToBusDaemon = m->addMatchToBusDaemon;
    adbusmatch.removeOnFirstMatch = m->removeOnFirstMatch;
    SetupString(m->sender, &adbusmatch.sender, &adbusmatch.senderSize);
    SetupString(m->destination, &adbusmatch.destination, &adbusmatch.destinationSize);
    SetupString(m->interface, &adbusmatch.interface, &adbusmatch.interfaceSize);
    SetupString(m->path, &adbusmatch.path, &adbusmatch.pathSize);
    SetupString(m->member, &adbusmatch.member, &adbusmatch.memberSize);
    SetupString(m->errorName, &adbusmatch.errorName, &adbusmatch.errorNameSize);

    adbusmatch.callback = function;
    adbusmatch.user1 = user1;
    adbusmatch.user2 = user2;

    uint32_t matchId = ADBusAddMatch(c, &adbusmatch);

    MatchData match;
    match.connection = c;
    match.matchId    = matchId;
    m_Matches.push_back(match);

    return matchId;
}

// ----------------------------------------------------------------------------

void Object::removeMatch(
        ADBusConnection*             c,
        uint32_t                id)
{
    for (size_t i = 0; i < m_Matches.size(); ++i) {
        MatchData& m = m_Matches[i];
        if (m.connection == c && m.matchId == id) {
            ADBusRemoveMatch(c, id);
            m_Matches.erase(m_Matches.begin() + i);
        }
    }
}

// ----------------------------------------------------------------------------

void Object::removeAllMatches()
{
    for (size_t i = 0; i < m_Matches.size(); ++i) {
        ADBusRemoveMatch(m_Matches[i].connection, m_Matches[i].matchId);
    }
    m_Matches.clear();
}

// ----------------------------------------------------------------------------

void Object::doBind(
        ADBusConnection*        c,
        const std::string&      path,
        ADBusInterface*         interface,
        struct ADBusUser*       user2)
{
    BindData bound;
    bound.connection = c;
    bound.object     = ADBusAddObject(c, path.c_str(), (int) path.size());
    bound.interface  = interface;

    int err = ADBusBindInterface(bound.object, bound.interface, user2);
    if (!err)
        m_Interfaces.push_back(bound);
}

// ----------------------------------------------------------------------------

void Object::unbind(
        ADBusConnection*        c,
        const std::string&      path,
        ADBusInterface*         interface)
{
    ADBusObject* object = ADBusGetObject(c, path.c_str(), (int) path.size());
    if (!object)
        return;

    for (size_t i = 0; i < m_Interfaces.size(); ++i) {
        BindData& b = m_Interfaces[i];
        if (b.connection == c 
         && b.interface == interface 
         && b.object == object)
        {
            ADBusUnbindInterface(object, b.interface);
            m_Interfaces.erase(m_Interfaces.begin() + i);
            break;
        }
    }
}

// ----------------------------------------------------------------------------

void Object::unbindAll()
{
    for (size_t i = 0; i < m_Interfaces.size(); ++i) {
        BindData& b = m_Interfaces[i];
        ADBusUnbindInterface(b.object, b.interface);
    }
    m_Interfaces.clear();
}
