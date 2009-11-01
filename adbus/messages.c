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

#include <adbus/adbus.h>

#include "connection.h"
#include "interface.h"
#include "misc.h"

#include "memory/kstring.h"

// ----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable:4702) /* unreachable code due to longjmp */
#endif

int adbus_error_longjmp(
        adbus_CbData*    d,
        const char*                 errorName,
        const char*                 errorMsgFormat,
        ...)
{
    kstring_t* msg = ks_new();
    va_list ap;
    va_start(ap, errorMsgFormat);
    ks_vprintf(msg, errorMsgFormat, ap);
    va_end(ap);

    adbus_setup_error(d, errorName, -1, ks_cstr(msg), ks_size(msg));

    ks_free(msg);

    longjmp(d->jmpbuf, ADBUSI_ERROR);
    return 0;
}

// ----------------------------------------------------------------------------

int adbus_error(
        adbus_CbData*    d,
        const char*                 errorName,
        const char*                 errorMsgFormat,
        ...)
{
    kstring_t* msg = ks_new();
    va_list ap;
    va_start(ap, errorMsgFormat);
    ks_vprintf(msg, errorMsgFormat, ap);
    va_end(ap);

    adbus_setup_error(d, errorName, -1, ks_cstr(msg), ks_size(msg));

    ks_free(msg);

    return 0;
}

// ----------------------------------------------------------------------------

void adbus_setup_error(
        adbus_CbData*    d,
        const char*                 errorName,
        int                         errorNameSize,
        const char*                 errorMessage,
        int                         errorMessageSize)
{
    if (!d->retmessage)
        return;

    d->manualReply = 0;

    size_t destsize;
    const char* dest = adbus_msg_sender(d->message, &destsize);
    int serial = adbus_msg_serial(d->message);

    adbus_Message* m = d->retmessage;
    adbus_msg_reset(m);
    adbus_msg_settype(m, ADBUS_MSG_ERROR);
    adbus_msg_setflags(m, ADBUS_MSG_NO_REPLY);
    adbus_msg_setserial(m, adbus_conn_serial(d->connection));

    adbus_msg_setreply(m, serial);
    adbus_msg_seterror(m, errorName, errorNameSize);

    if (dest) {
        adbus_msg_setdestination(m, dest, destsize);
    }

    if (errorMessage) {
        adbus_msg_append(m, "s", -1);
        adbus_msg_string(m, errorMessage, errorMessageSize);
    }
}

// ----------------------------------------------------------------------------

void adbus_setup_signal(
        adbus_Message*        message,
        adbus_Path*     path,
        adbus_Member*         signal)
{
    adbus_msg_reset(message);
    adbus_msg_settype(message, ADBUS_MSG_SIGNAL);
    adbus_msg_setflags(message, ADBUS_MSG_NO_REPLY);
    adbus_msg_setserial(message, adbus_conn_serial(path->connection));

    adbus_msg_setpath(message, path->string, path->size);
    adbus_msg_setinterface(message, signal->interface->name, -1);
    adbus_msg_setmember(message, signal->name, -1);
}









