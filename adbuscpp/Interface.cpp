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

#include "Interface.h"

#include "adbus/CommonMessages.h"
#include <assert.h>

using namespace adbus;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void adbus::CallMethod(struct ADBusCallDetails* details)
{
    UserDataBase* data = (UserDataBase*) details->user1;
    assert(data->chainedFunction);

    try
    {
        data->chainedFunction(details);
    }
    catch (ParseError& e)
    {
        details->parseError = e.parseError;
    }
    catch (Error& e)
    {
        if (!details->returnMessage)
            return;

        const char* errorName = e.errorName();
        const char* errorMessage = e.errorMessage();

        ADBusSetupError(
                details,
                errorName,
                -1,
                errorMessage,
                -1);
    }
}

// ----------------------------------------------------------------------------

Interface::Interface(const std::string& name)
{
  m_I = ADBusCreateInterface(name.c_str(), name.size());
}

// ----------------------------------------------------------------------------

Interface::~Interface()
{
  ADBusFreeInterface(m_I);
}

// ----------------------------------------------------------------------------

Member Interface::addMethod(const std::string& name)
{
  struct ADBusMember* member 
    = ADBusAddMember(m_I, ADBusMethodMember, name.c_str(), name.size());

  return Member(member, ADBusMethodMember);
}

// ----------------------------------------------------------------------------

Member Interface::addSignal(const std::string& name)
{
  struct ADBusMember* member 
    = ADBusAddMember(m_I, ADBusSignalMember, name.c_str(), name.size());

  return Member(member, ADBusSignalMember);
}

// ----------------------------------------------------------------------------

Member Interface::addProperty(const std::string& name, const std::string& type)
{
  struct ADBusMember* member 
    = ADBusAddMember(m_I, ADBusPropertyMember, name.c_str(), name.size());

  ADBusSetPropertyType(member, type.c_str(), type.size());

  return Member(member, ADBusPropertyMember);
}

// ----------------------------------------------------------------------------

Member Interface::method(const std::string& name)
{
  struct ADBusMember* member =
      ADBusGetInterfaceMember(m_I, ADBusMethodMember, name.c_str(), name.size());
  return Member(member, ADBusMethodMember);
}

Member Interface::signal(const std::string& name)
{
  struct ADBusMember* member =
      ADBusGetInterfaceMember(m_I, ADBusSignalMember, name.c_str(), name.size());
  return Member(member, ADBusSignalMember);
}

Member Interface::property(const std::string& name)
{
  struct ADBusMember* member =
      ADBusGetInterfaceMember(m_I, ADBusMethodMember, name.c_str(), name.size());
  return Member(member, ADBusPropertyMember);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

Member::Member(ADBusMember* member, ADBusMemberType type)
: m(member),
  m_Type(type)
{
}

// ----------------------------------------------------------------------------

Member& Member::addAnnotation(const std::string& name, const std::string& value)
{
  ADBusAddAnnotation(m, name.c_str(), name.size(), value.c_str(), value.size());
  return *this;
}

// ----------------------------------------------------------------------------

Member& Member::addArgument(const std::string& name, const std::string& type)
{
  ADBusArgumentDirection dir = (m_Type == ADBusSignalMember)
                             ? ADBusSignalArgument
                             : ADBusInArgument;

  ADBusAddArgument(m, dir, name.c_str(), name.size(), type.c_str(), type.size());
  return *this;
}

// ----------------------------------------------------------------------------

Member& Member::addReturn(const std::string& name, const std::string& type)
{
  ADBusArgumentDirection dir = ADBusOutArgument;
  ADBusAddArgument(m, dir, name.c_str(), name.size(), type.c_str(), type.size());
  return *this;
}

// ----------------------------------------------------------------------------

Member& Member::setMethod(ADBusMessageCallback callback, ADBusUser* user1)
{
  ADBusSetMethodCallback(m, callback, user1);
  return *this;
}

// ----------------------------------------------------------------------------

Member& Member::setSetter(ADBusMessageCallback callback, ADBusUser* user1)
{
  ADBusSetPropertySetCallback(m, callback, user1);
  return *this;
}

// ----------------------------------------------------------------------------

Member& Member::setGetter(ADBusMessageCallback callback, ADBusUser* user1)
{
  ADBusSetPropertyGetCallback(m, callback, user1);
  return *this;
}



