// vim: ts=4 sw=4 sts=4 et
//
// Copyright (c) 2009 James R. McKaskill
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// ----------------------------------------------------------------------------

#ifndef NDEBUG

#include "Tests.h"

#include "adbus/Interface.h"

#include <assert.h>

static struct ADBusCallDetails sDetails;
static int sData;
static struct ADBusUser sUser1[4];

static void Callback(struct ADBusCallDetails* details, int i)
{
    assert(sData == 0);
    assert(details == &sDetails);
    assert(details->user1 == &sUser1[i]);
    sData = i;
}

static void Callback1(struct ADBusCallDetails* details)
{ Callback(details, 1); }
static void Callback2(struct ADBusCallDetails* details)
{ Callback(details, 2); }
static void Callback3(struct ADBusCallDetails* details)
{ Callback(details, 3); }

void TestInterface()
{
    struct ADBusInterface* i = ADBusCreateInterface("foo.bar", -1);
    struct ADBusMember* m1 = ADBusAddMember(i, ADBusMethodMember, "foo", -1);
    struct ADBusMember* m2 = ADBusGetInterfaceMember(i, ADBusMethodMember, "foo", -1);
    assert(m1 && m2 && m1 == m2);

    struct ADBusMember* m3 = ADBusGetInterfaceMember(i, ADBusSignalMember, "foo", -1);
    assert(m3 == NULL);

    sData = 0;
    ADBusCallMethod(m1, &sDetails);
    assert(sData == 0);

    ADBusSetMethodCallback(m1, &Callback1, &sUser1[1]);
    sData = 0;
    ADBusCallMethod(m1, &sDetails);
    assert(sData == 1);

    struct ADBusMember* p1 = ADBusAddMember(i, ADBusPropertyMember, "bar", -1);

    assert(!ADBusIsPropertyReadable(p1));
    assert(!ADBusIsPropertyWritable(p1));

    ADBusSetPropertyGetCallback(p1, &Callback2, &sUser1[2]);
    assert(ADBusIsPropertyReadable(p1));
    assert(!ADBusIsPropertyWritable(p1));

    ADBusSetPropertySetCallback(p1, &Callback3, &sUser1[3]);
    assert(ADBusIsPropertyReadable(p1));
    assert(ADBusIsPropertyWritable(p1));

    sData = 0;
    ADBusCallGetProperty(p1, &sDetails);
    assert(sData == 2);

    sData = 0;
    ADBusCallSetProperty(p1, &sDetails);
    assert(sData == 3);

    size_t size = 3;
    assert(ADBusPropertyType(p1, &size) == NULL && size == 0);

    ADBusSetPropertyType(p1, "as", -1);
    size = 3;
    assert(strcmp(ADBusPropertyType(p1, &size), "as") == 0 && size == 2);

    ADBusAddAnnotation(p1, "foo", -1, "data1", -1);
    ADBusAddAnnotation(p1, "foo", -1, "data2", -1);

    ADBusFreeInterface(i);
}

#endif