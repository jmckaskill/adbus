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

#include "Parse.h"

#include "Message.h"
#include "Misc_p.h"
#include "memory/kvector.h"

KVECTOR_INIT(uint8_t, uint8_t)

// ----------------------------------------------------------------------------

struct ADBusStreamBuffer* ADBusCreateStreamBuffer()
{
    return (struct ADBusStreamBuffer*) kv_new(uint8_t);
}

// ----------------------------------------------------------------------------

void ADBusFreeStreamBuffer(struct ADBusStreamBuffer* b)
{
    if (b) {
        kv_free(uint8_t, (kvector_t(uint8_t)*) b);
    }
}

// ----------------------------------------------------------------------------

static uint HaveDataInBuffer(
        kvector_t(uint8_t)*         b,
        size_t                      needed,
        const uint8_t**             data,
        size_t*                     size)
{
    int toAdd = needed - kv_size(b);
    if (toAdd > (int) *size) {
        // Don't have enough data
        uint8_t* dest = kv_push(uint8_t, b, *size);
        memcpy(dest, *data, *size);
        *data += *size;
        *size = 0;
        return 0;

    } else if (toAdd > 0) {
        // Need to push data into the buffer, but we have enough
        uint8_t* dest = kv_push(uint8_t, b, toAdd);
        memcpy(dest, *data, toAdd);
        *data += toAdd;
        *size -= toAdd;
        return 1;

    } else {
        // Buffer already has enough data
        return 1;
    }
}

// ----------------------------------------------------------------------------

int ADBusParse(
        struct ADBusStreamBuffer*   buffer,
        struct ADBusMessage*        message,
        const uint8_t**             data,
        size_t*                     size)
{
    kvector_t(uint8_t)* b = (kvector_t(uint8_t)*) buffer;

    ADBusResetMessage(message);
    if (kv_size(b) > 0)
    {
        // Use up message from the stream buffer
        
        // We need to add enough so we can figure out how big the next message is
        if (!HaveDataInBuffer(b, sizeof(struct ExtendedHeader), data, size))
            return 0;

        // Now we add enough to the buffer to be able to parse the message
        size_t messageSize = ADBusNextMessageSize(&kv_a(b, 0), kv_size(b));
        assert(messageSize > 0);
        if (!HaveDataInBuffer(b, messageSize, data, size))
            return 0;

        int err = ADBusSetMessageData(message, &kv_a(b, 0), messageSize);
        kv_remove(uint8_t, b, 0, messageSize);
        return err;
    }
    else
    {
        size_t messageSize = ADBusNextMessageSize(*data, *size);
        if (messageSize == 0 || messageSize > *size)
        {
            // We don't have enough to parse the message
            uint8_t* dest = kv_push(uint8_t, b, *size);
            memcpy(dest, *data, *size);
            return 0;
        }

        int err = ADBusSetMessageData(message, *data, messageSize);

        *data += messageSize;
        *size -= messageSize;
        return err;
    }
}




