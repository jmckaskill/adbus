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

#include "internal.h"
#include "auth.h"

/* ------------------------------------------------------------------------- */

int adbus_aconn_connect(adbus_AuthConnection* c)
{
    c->authenticated = 0;
    adbus_conn_setbuffer(c->connection, c->buffer);
    if (c->auth->send(c->auth->data, "\0", 1) != 1)
        return -1;

    return adbus_cauth_start(c->auth);
}

/* ------------------------------------------------------------------------- */

int adbus_aconn_parse(adbus_AuthConnection* c)
{
    if (!c->authenticated) {
        int ret = adbus_auth_parse(c->auth, c->buffer);

        if (ret <= 0) {
            return -1;
        } else if (ret == 0) {
            return 0;
        } else {
            if (c->authCallback) {
                c->authCallback(c->authUser);
            }

            if (c->connectToBus) {
                adbus_conn_connect(c->connection, c->connectCallback, c->connectUser);
            }

            return adbus_conn_parse(c->connection);
        }
    } else {
        return adbus_conn_parse(c->connection);
    }

}

