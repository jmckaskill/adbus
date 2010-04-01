
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

#include "Server.h"

#ifdef _MSC_VER
#   pragma warning(disable:4127) // conditional expression is constant
#endif

class Thread : public EventLoop
{
public:
    void RegisterHandle(Callback* cb, HANDLE event)
    {
        handles.push_back(event);
        callbacks.push_back(cb);
    }

    void UnregisterHandle(Callback* cb, HANDLE event)
    {
        for (size_t i = 0; i < callbacks.size(); i++) {
            if (callbacks[i] == cb && handles[i] == event) {
                callbacks.erase(callbacks.begin() + i);
                handles.erase(handles.begin() + i);
            }
        }
    }

    std::vector<HANDLE>     handles;
    std::vector<Callback*>  callbacks;
};

int main()
{
	adbus_set_loglevel(3);

    Thread t;
    Server s;
    s.Init(&t);

    while (true) {
        DWORD ret = WaitForMultipleObjects((DWORD) t.handles.size(), &t.handles[0], FALSE, INFINITE);
        if (ret >= WAIT_OBJECT_0 + t.handles.size())
            abort();

        t.callbacks[ret]->OnEvent(t.handles[ret]);
    }

    return 0;
}
