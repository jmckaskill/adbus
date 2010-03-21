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

#include "qdbuserror_p.hxx"
#include "qdbusmessage.h"

/* ------------------------------------------------------------------------- */

QDBusError::QDBusError(const DBusError* error)
: code(NoError), unused(NULL)
{ 
    if (error) {
        adbus_Message* m = error->msg;
        this->nm = QString::fromUtf8(m->error, m->errorSize);
        this->code = Other;
        if (m->signature && strncmp(m->signature, "s", 1) == 0) {
            adbus_Iterator iter = {};
            adbus_iter_args(&iter, m);

            const char* msgstr;
            size_t msgsz;
            if (!adbus_iter_string(&iter, &msgstr, &msgsz)) {
                this->msg = QString::fromUtf8(msgstr, msgsz);
            }
        }
    }
}

QDBusError::QDBusError(const QDBusMessage& m)
: code(Other), msg(m.errorMessage()), nm(m.errorName()), unused(NULL)
{}

QDBusError::QDBusError(const QDBusError& other)
: code(other.code), msg(other.msg), nm(other.nm), unused(other.unused)
{}

QDBusError& QDBusError::operator=(const QDBusError& other)
{
    code = other.code;
    msg = other.msg;
    nm = other.nm;
    unused = other.unused;
    return *this;
}

QString QDBusError::name() const
{ return this->nm; }

QString QDBusError::message() const
{ return this->msg; }

QString QDBusError::errorString(ErrorType error)
{ Q_UNUSED(error); return "other"; }

