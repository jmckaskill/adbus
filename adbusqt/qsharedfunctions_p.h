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

// We want QDBusMessage to use shared data but can't use QSharedDataPointer in
// the header so we use wrappers instead.
// These are basically a copy of QSharedDataPointer

// Specialise this to change the delete mechanism - eg if you actually want to
// delete the data on another thread.
template <class T>
inline void qDeleteSharedData(T*& d)
{ delete d; }

// Call at the beginning of non-const member functions
template <class T>
inline void qDetachSharedData(T*& d)
{
    if (d && d->ref != 1) {
        T* x = new T(*d);
        x->ref.ref();
        if (!d->ref.deref())
            qDeleteSharedData(d);
        d = x;
    }
}

// Call in the assignment constructor
template <class T>
inline void qAssignSharedData(T*& d, const T* o)
{
    if (d != o) {
        if (o)
            o->ref.ref();
        if (d && !d->ref.deref())
            qDeleteSharedData(d);
        d = (T*) o;
    }    
}

// Call in the private and copy constructor
template <class T>
inline void qCopySharedData(T*& d, const T* o)
{
    d = (T*) o;
    if (d)
        d->ref.ref();
}

// Call in the destructor
template <class T>
inline void qDestructSharedData(T*& d)
{
    if (d && !d->ref.deref())
        qDeleteSharedData(d);
    d = NULL;
}


