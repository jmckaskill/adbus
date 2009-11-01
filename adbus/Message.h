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

/** \defgroup ADBusMessage Message
 *  \ingroup adbus
 *
 * \brief Functions to get/set message attributes/data.
 *
 * Encapsulates a complete message including a set of header fields and a
 * block of marshalled argument data.
 *
 * It owns its internal data:
 * - Will copy any data given to it
 * - Any memory referenced is still owned by the message and should be copied
 *   or used immediately
 *
 * After setting the full message data or the argument data, headers can only
 * be read.
 *
 * On setting the data, the data is endian flipped to the native endianness if
 * needed.
 *
 * @{
 */

ADBUS_API size_t ADBusNextMessageSize(const uint8_t* data, size_t size);

ADBUS_API struct ADBusMessage* ADBusCreateMessage();
ADBUS_API void ADBusFreeMessage(struct ADBusMessage* m);
ADBUS_API void ADBusResetMessage(struct ADBusMessage* m);

ADBUS_API int  ADBusSetMessageData(struct ADBusMessage* m, const uint8_t* data, size_t size);
ADBUS_API void ADBusGetMessageData(struct ADBusMessage* m, const uint8_t** data, size_t* size);

ADBUS_API const char* ADBusGetPath(struct ADBusMessage* m, size_t* len);
ADBUS_API const char* ADBusGetInterface(struct ADBusMessage* m, size_t* len);
ADBUS_API const char* ADBusGetSender(struct ADBusMessage* m, size_t* len);
ADBUS_API const char* ADBusGetDestination(struct ADBusMessage* m, size_t* len);
ADBUS_API const char* ADBusGetMember(struct ADBusMessage* m, size_t* len);
ADBUS_API const char* ADBusGetErrorName(struct ADBusMessage* m, size_t* len);
ADBUS_API const char* ADBusGetSignature(struct ADBusMessage* m, size_t* len);

ADBUS_API enum ADBusMessageType ADBusGetMessageType(struct ADBusMessage* m);
ADBUS_API uint8_t  ADBusGetFlags(struct ADBusMessage* m);
ADBUS_API uint32_t ADBusGetSerial(struct ADBusMessage* m);
ADBUS_API uint     ADBusHasReplySerial(struct ADBusMessage* m);
ADBUS_API uint32_t ADBusGetReplySerial(struct ADBusMessage* m);

ADBUS_API void ADBusGetArgumentData(struct ADBusMessage* m, const uint8_t** data, size_t* size);

ADBUS_API void ADBusSetMessageType(struct ADBusMessage* m, enum ADBusMessageType type);
ADBUS_API void ADBusSetSerial(struct ADBusMessage* m, uint32_t serial);
ADBUS_API void ADBusSetFlags(struct ADBusMessage* m, uint8_t flags);
ADBUS_API void ADBusSetReplySerial(struct ADBusMessage* m, uint32_t reply);

ADBUS_API void ADBusSetPath(struct ADBusMessage* m, const char* path, int size);
ADBUS_API void ADBusSetInterface(struct ADBusMessage* m, const char* path, int size);
ADBUS_API void ADBusSetMember(struct ADBusMessage* m, const char* path, int size);
ADBUS_API void ADBusSetErrorName(struct ADBusMessage* m, const char* path, int size);
ADBUS_API void ADBusSetDestination(struct ADBusMessage* m, const char* path, int size);
ADBUS_API void ADBusSetSender(struct ADBusMessage* m, const char* path, int size);

ADBUS_API struct ADBusMarshaller* ADBusArgumentMarshaller(struct ADBusMessage* m);
ADBUS_API void  ADBusArgumentIterator(struct ADBusMessage* m, struct ADBusIterator* iterator);
ADBUS_API char* ADBusNewMessageSummary(struct ADBusMessage* m, size_t* size);

/** @} */



