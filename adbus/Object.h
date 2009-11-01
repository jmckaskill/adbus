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


ADBUS_API struct ADBusObject* ADBusCreateObject();
ADBUS_API void ADBusFreeObject(struct ADBusObject* object);
ADBUS_API void ADBusResetObject(struct ADBusObject* object);

ADBUS_API int ADBusBindObject(
        struct ADBusObject*         object,
        struct ADBusObjectPath*     path,
        struct ADBusInterface*      interface,
        struct ADBusUser*           user2);

ADBUS_API int ADBusUnbindObject(
        struct ADBusObject*         object,
        struct ADBusObjectPath*     path,
        struct ADBusInterface*      interface);

ADBUS_API uint32_t ADBusAddObjectMatch(
        struct ADBusObject*         object,
        struct ADBusConnection*     connection,
        const struct ADBusMatch*    match);

ADBUS_API void ADBusAddObjectMatchId(
        struct ADBusObject*         object,
        struct ADBusConnection*     connection,
        uint32_t                    id);

ADBUS_API void ADBusRemoveObjectMatch(
        struct ADBusObject*         object,
        struct ADBusConnection*     connection,
        uint32_t                    id);


