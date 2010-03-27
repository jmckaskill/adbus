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

#include "messages.h"

/** \struct adbus_CbData
 *  \brief Data structure used for message callbacks.
 *
 *  For non-proxied callbacks this structure is setup within the connection
 *  parse functions. 
 *
 *  For proxied callbacks, the proxier has to copy a number of these fields
 *  see adbus_ConnectionCallbacks::get_proxy for more details.
 *
 *  The various adbus_check functions are very handy in C callbacks. Any
 *  errors (including incorrect argument types) encountered whilst iterating
 *  over the arguments causes the check function to longjmp out.
 *
 *  \note The adbus_check functions should not be used in combination with
 *  manual iteration.
 *
 *  For example:
 *  \code
 *  int Callback(adbus_CbData* d)
 *  {
 *      double d = adbus_check_double(d);
 *      adbus_check_beginstruct(d);
 *      double v1 = adbus_check_double(d);
 *      double v2 = adbus_check_double(d);
 *      double v3 = adbus_check_double(d);
 *      adbus_check_endstruct(d);
 *      adbus_check_end(d);
 *
 *      return 0;
 *  }
 *  \endcode
 *
 */

/** \var adbus_CbData::connection
 *  \brief Connection the message was received on.
 */

/** \var adbus_CbData::msg
 *  The received message.
 */

/** \var adbus_CbData::ret
 *  A message factory used for setting up returned messages.
 *
 *  By default this will be used to return a message immediately after the
 *  callback completes unless noreturn is set to non-zero.
 *
 *  \warning This is only set for method and property callbacks.
 */

/** \var adbus_CbData::noreturn
 *  Flag to indicate that no return message should be sent.
 */

/** \var adbus_CbData::setprop
 *  Iterator setup to iterate over the property value to set to.
 */

/** \var adbus_CbData::getprop
 *  Buffer to store the current property value.
 */

/** \var adbus_CbData::user1
 *  First user data variable.
 *
 *  For match and reply callbacks this is the user data supplied in the
 *  registration.
 *  For method and property callbacks this is the user data supplied in the
 *  interface.
 */

/** \var adbus_CbData::user2
 *  Second user data variable.
 *
 *  This is only set for method and property callbacks where it is set to the
 *  user data supplied in the bind.
 */

/* ------------------------------------------------------------------------- */

/** Dispatch a message callback
 *  \relates adbus_CbData
 */
int adbus_dispatch(adbus_MsgCallback callback, adbus_CbData* d)
{
    int ret = setjmp(d->jmpbuf);
    if (ret == ADBUSI_ERROR) {
        return 0;
    } else if (ret) {
        return ret;
    }

    adbus_iter_args(&d->checkiter, d->msg);
    return callback(d);
}

/* ------------------------------------------------------------------------- */

#ifdef _MSC_VER
#pragma warning(disable:4702) /* unreachable code due to longjmp */
#endif

/** Setup an error reply and longjmp out of the callback
 *  \relates adbus_CbData
 *
 *  \warning Since this uses longjmp, it should be used with caution.
 */
int adbus_errorf_jmp(
        adbus_CbData*    d,
        const char*                 errorName,
        const char*                 errorMsgFormat,
        ...)
{
    d_String msg;
    va_list ap;

    ZERO(msg);
    va_start(ap, errorMsgFormat);
    ds_cat_vf(&msg, errorMsgFormat, ap);
    va_end(ap);

    adbus_error(d, errorName, -1, ds_cstr(&msg), ds_size(&msg));

    ds_free(&msg);

    longjmp(d->jmpbuf, ADBUSI_ERROR);
    return 0;
}

/* ------------------------------------------------------------------------- */

/** Setup an error reply
 *  \relates adbus_CbData
 *
 *  The error message is formatted with a printf style format string in \a
 *  errorMsgFormat and the variable arguments.
 *
 *  \return 0 always - designed to be returned directly from a message
 *  callback.
 *
 *  For example:
 *  \code
 *  int Callback(adbus_CbData* d)
 *  {
 *      ...
 *      if (have_error) {
 *          return adbus_errorf(
 *              d, 
 *              "com.example.ExampleError", 
 *              "Something happened with %s", "foo");
 *      }
 *  }
 *  \endcode
 */
int adbus_errorf(
        adbus_CbData*    d,
        const char*                 errorName,
        const char*                 errorMsgFormat,
        ...)
{
    d_String msg;
    va_list ap;

    ZERO(msg);

    if (errorMsgFormat) {
        va_start(ap, errorMsgFormat);
        ds_cat_vf(&msg, errorMsgFormat, ap);
        va_end(ap);

        adbus_error(d, errorName, -1, ds_cstr(&msg), ds_size(&msg));
    } else {
        adbus_error(d, errorName, -1, NULL, 0);
    }

    ds_free(&msg);

    return 0;
}

/* ------------------------------------------------------------------------- */

/** Setup an error reply
 *  \relates adbus_CbData
 *
 *  \return 0 always - designed to be returned directly from a message
 *  callback
 *
 */
int adbus_error(
        adbus_CbData*    d,
        const char*      errorName,
        int              errorNameSize,
        const char*      errorMessage,
        int              errorMessageSize)
{
    if (errorNameSize < 0)
        errorNameSize = strlen(errorName);

    if (errorMessage && errorMessageSize < 0)
        errorMessageSize = strlen(errorMessage);

    if (errorMessage) {
        ADBUSI_LOG("Error '%*s' '%*s'", errorNameSize, errorName, errorMessageSize, errorMessage);
    } else {
        ADBUSI_LOG("Error '%*s'", errorNameSize, errorName);
    }

    assert(errorName);

    if (d->ret) {
        adbus_MsgFactory* m = d->ret;
        adbus_msg_reset(m);
        adbus_msg_settype(m, ADBUS_MSG_ERROR);
        adbus_msg_setflags(m, ADBUS_MSG_NO_REPLY);
        adbus_msg_setserial(m, adbus_conn_serial(d->connection));

        adbus_msg_setreply(m, d->msg->serial);
        adbus_msg_seterror(m, errorName, errorNameSize);

        if (d->msg->destination) {
            adbus_msg_setdestination(m, d->msg->destination, d->msg->destinationSize);
        }

        if (errorMessage) {
            adbus_msg_setsig(m, "s", 1);
            adbus_msg_string(m, errorMessage, errorMessageSize);
            adbus_msg_end(m);
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

int adbus_error_argument(adbus_CbData* d)
{
    if (d->msg->interface) {
        return adbus_errorf(
                d,
                "nz.co.foobar.adbus.InvalidArgument",
                "Invalid argument to the method '%s.%s' on %s",
                d->msg->interface,
                d->msg->member,
                d->msg->path);
    } else {
        return adbus_errorf(
                d,
                "nz.co.foobar.adbus.InvalidArgument",
                "Invalid argument to the method '%s' on %s",
                d->msg->member,
                d->msg->path);
    }
}

/* ------------------------------------------------------------------------- */

int adbusI_pathError(adbus_CbData* d)
{
    return adbus_errorf(
            d,
            "nz.co.foobar.adbus.InvalidPath",
            "The path '%s' does not exist.",
            d->msg->path);
}

/* ------------------------------------------------------------------------- */

int adbusI_interfaceError(adbus_CbData* d)
{
    return adbus_errorf(
            d,
            "nz.co.foobar.adbus.InvalidInterface",
            "The path '%s' does not export the interface '%s'.",
            d->msg->path,
            d->msg->interface,
            d->msg->member);
}

/* ------------------------------------------------------------------------- */

int adbusI_methodError(adbus_CbData* d)
{
    if (d->msg->interface) {
        return adbus_errorf(
                d,
                "nz.co.foobar.adbus.InvalidMethod",
                "The path '%s' does not export the method '%s.%s'.",
                d->msg->path,
                d->msg->interface,
                d->msg->member);
    } else {
        return adbus_errorf(
                d,
                "nz.co.foobar.adbus.InvalidMethod",
                "The path '%s' does not export the method '%s'.",
                d->msg->path,
                d->msg->member);
    }
}

/* ------------------------------------------------------------------------- */

int adbusI_propertyError(adbus_CbData* d)
{
    return adbus_errorf(
            d,
            "nz.co.foobar.adbus.InvalidProperty",
            "The path '%s' does not export the property '%s.%s'.",
            d->msg->path,
            d->msg->interface,
            d->msg->member);
}

/* ------------------------------------------------------------------------- */

int adbusI_propWriteError(adbus_CbData* d)
{
    return adbus_errorf(
            d,
            "nz.co.foobar.adbus.ReadOnlyProperty",
            "The property '%s.%s' on '%s' is read only.",
            d->msg->interface,
            d->msg->member,
            d->msg->path);
}

/* ------------------------------------------------------------------------- */

int adbusI_propReadError(adbus_CbData* d)
{
    return adbus_errorf(
            d,
            "nz.co.foobar.adbus.WriteOnlyProperty",
            "The property '%s.%s' on '%s' is write only.",
            d->msg->interface,
            d->msg->member,
            d->msg->path);
}

/* ------------------------------------------------------------------------- */

int adbusI_propTypeError(adbus_CbData* d)
{ 
    return adbus_errorf(
            d,
            "nz.co.foobar.adbus.InvalidPropertyType",
            "Incorrect property type for '%s.%s' on %s.",
            d->msg->interface,
            d->msg->member,
            d->msg->path);
}









