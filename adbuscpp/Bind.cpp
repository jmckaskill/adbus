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

#include "Bind.h"

#include "adbus/CommonMessages.h"

#include <assert.h>

// ----------------------------------------------------------------------------

int adbus::detail::CallMethod(struct ADBusCallDetails* details)
{
    int err = 0;
    UserDataBase* data = (UserDataBase*) details->user1;
    assert(data->chainedFunction);

    try
    {
        err = data->chainedFunction(details);
    }
    catch (ParseError& e)
    {
        err = e.parseError;
    }
    catch (Error& e)
    {
        if (details->retmessage) {
            const char* errorName = e.errorName();
            const char* errorMessage = e.errorMessage();

            ADBusSetupError(
                    details,
                    errorName,
                    -1,
                    errorMessage,
                    -1);
        }
    }

    return err;
}

