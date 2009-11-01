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

#pragma once

#include "Interface.h"
#include "User.h"
#include "memory/khash.h"
#include "memory/kvector.h"

// ----------------------------------------------------------------------------

KHASH_MAP_INIT_STR(StringPair, char*)

struct Argument
{
    char*           name;
    char*           type;
};

KVECTOR_INIT(Argument, struct Argument)

// ----------------------------------------------------------------------------

struct ADBusMember
{
    char*                       name;

    struct ADBusInterface*      interface;
    enum ADBusMemberType        type;

    char*                       propertyType;

    kvector_t(Argument)*        inArguments;
    kvector_t(Argument)*        outArguments;

    khash_t(StringPair)*        annotations;

    ADBusMessageCallback        methodCallback;
    ADBusMessageCallback        getPropertyCallback;
    ADBusMessageCallback        setPropertyCallback;

    struct ADBusUser*           methodData;
    struct ADBusUser*           getPropertyData;
    struct ADBusUser*           setPropertyData;
};

KHASH_MAP_INIT_STR(MemberPtr, struct ADBusMember*)

// ----------------------------------------------------------------------------

struct ADBusInterface
{
    char*                   name;
    khash_t(MemberPtr)*     members;
};

// ----------------------------------------------------------------------------

int IntrospectCallback(struct ADBusCallDetails* details);
int GetPropertyCallback(struct ADBusCallDetails* details);
int GetAllPropertiesCallback(struct ADBusCallDetails* details);
int SetPropertyCallback(struct ADBusCallDetails* details);




