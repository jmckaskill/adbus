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
#include "Common.h"
#include "User.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ADBusUser;
struct ADBusIterator;
struct ADBusMessage;
struct ADBusConnection;
struct ADBusMarshaller;


// ----------------------------------------------------------------------------
// Interface management
// ----------------------------------------------------------------------------
struct ADBusInterface;

ADBUS_API struct ADBusInterface* ADBusCreateInterface(const char* name, int size); 
ADBUS_API void ADBusFreeInterface(struct ADBusInterface* interface);

enum ADBusMemberType
{
    ADBusMethodMember,
    ADBusSignalMember,
    ADBusPropertyMember,
};

ADBUS_API struct ADBusMember* ADBusAddMember(
        struct ADBusInterface*      interface,
        enum ADBusMemberType        type,
        const char*                 name,
        int                         size);

ADBUS_API struct ADBusMember* ADBusGetInterfaceMember(
        struct ADBusInterface*      interface,
        enum ADBusMemberType        type,
        const char*                 name,
        int                         size);

// ----------------------------------------------------------------------------
// Member management
// ----------------------------------------------------------------------------

ADBUS_API void ADBusAddAnnotation(
        struct ADBusMember*         member,
        const char*                 name,
        int                         nameSize,
        const char*                 value,
        int                         valueSize);


// Signals and Methods argumentsiff
enum ADBusArgumentDirection
{
    ADBusInArgument,
    ADBusOutArgument,
    ADBusSignalArgument = ADBusOutArgument,
};

ADBUS_API void ADBusAddArgument(
        struct ADBusMember*         member,
        enum ADBusArgumentDirection direction,
        const char*                 name,
        int                         nameSize,
        const char*                 type,
        int                         typeSize);

// Methods
ADBUS_API void ADBusSetMethodCallback(
        struct ADBusMember*         member,
        ADBusMessageCallback        callback,
        struct ADBusUser*           user1);


// Properties

ADBUS_API void ADBusSetPropertyType(
        struct ADBusMember*         member,
        const char*                 type,
        int                         typeSize);

ADBUS_API const char* ADBusPropertyType(
        struct ADBusMember*         member,
        size_t*                     size);

ADBUS_API uint ADBusIsPropertyReadable(
        struct ADBusMember*         member);

ADBUS_API uint ADBusIsPropertyWritable(
        struct ADBusMember*         member);

ADBUS_API void ADBusSetPropertyGetCallback(
        struct ADBusMember*         member,
        ADBusMessageCallback        callback,
        struct ADBusUser*           user1);

ADBUS_API void ADBusSetPropertySetCallback(
        struct ADBusMember*         member,
        ADBusMessageCallback        callback,
        struct ADBusUser*           user1);

// Call callbacks

ADBUS_API void ADBusCallMethod(
        struct ADBusMember*         member,
        struct ADBusCallDetails*    details);

ADBUS_API void ADBusCallSetProperty(
        struct ADBusMember*         member,
        struct ADBusCallDetails*    details);

ADBUS_API void ADBusCallGetProperty(
        struct ADBusMember*         member,
        struct ADBusCallDetails*    details);
#ifdef __cplusplus
}
#endif



