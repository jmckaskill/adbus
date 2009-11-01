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
#include "adbus/Object.h"

#include <assert.h>


using namespace adbus;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

Object::Object()
{
    m_Object = ADBusCreateObject();
}

// ----------------------------------------------------------------------------

Object::~Object()
{
    ADBusFreeObject(m_Object);
}

// ----------------------------------------------------------------------------

void Object::reset()
{
    ADBusResetObject(m_Object);
}

// ----------------------------------------------------------------------------

uint32_t Object::addMatch(
        ADBusConnection*    connection,
        ADBusMatch*         match)
{
    return ADBusAddObjectMatch(m_Object, connection, match);
}

// ----------------------------------------------------------------------------

void Object::addMatch(
        ADBusConnection*    connection,
        uint32_t            matchId)
{
    ADBusAddObjectMatchId(m_Object, connection, matchId);
}

// ----------------------------------------------------------------------------

void Object::removeMatch(
        ADBusConnection*    connection,
        uint32_t            matchId)
{
    ADBusRemoveObjectMatch(m_Object, connection, matchId);
}

// ----------------------------------------------------------------------------

void Object::doBind(
        ADBusObjectPath*    path,
        ADBusInterface*     interface,
        struct ADBusUser*   user2)
{
    ADBusBindObject(m_Object, path, interface, user2);
}

// ----------------------------------------------------------------------------

void Object::unbind(
        ADBusObjectPath*    path,
        ADBusInterface*     interface)
{
    ADBusUnbindObject(m_Object, path, interface);
}

// ----------------------------------------------------------------------------

void Object::unbind(
        ADBusConnection*    c,
        const std::string&  path,
        ADBusInterface*     interface)
{
    ADBusObjectPath* opath = ADBusGetObjectPath(c, path.c_str(), (int) path.size());
    if (opath) {
        unbind(opath, interface);
    }
}


