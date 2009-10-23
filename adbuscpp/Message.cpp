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

#include "Message.h"

using namespace adbus;

// ----------------------------------------------------------------------------

void adbus::CheckForMarshallError(
    int                   err)
{
    if (err)
        throw MarshallError();
}

// ----------------------------------------------------------------------------

void adbus::CheckForDemarshallError(
    struct ADBusField*    field,
    enum ADBusFieldType   expectedType)
{
    if (field->type != expectedType)
        throw DemarshallError();
}

// ----------------------------------------------------------------------------

static void UserDataBaseFreeFunction(struct ADBusUser* user)
{
    delete (UserDataBase*) user;
}

UserDataBase::UserDataBase()
: chainedFunction(NULL)
{
    free = &UserDataBaseFreeFunction;
}


// ----------------------------------------------------------------------------
// Demarshallers
// ----------------------------------------------------------------------------

void operator<<(bool& data, ADBusIterator& i)
{ 
    struct ADBusField field;
    ADBusIterate(&i, &field);
    adbus::CheckForDemarshallError(&field, ADBusBooleanField);
    data = (field.b != 0);
}

void operator<<(uint8_t& data, ADBusIterator& i)
{ 
    struct ADBusField field;
    ADBusIterate(&i, &field);
    adbus::CheckForDemarshallError(&field, ADBusUInt8Field);
    data = field.u8;
}

void operator<<(int16_t& data, ADBusIterator& i)
{ 
    struct ADBusField field;
    ADBusIterate(&i, &field);
    adbus::CheckForDemarshallError(&field, ADBusInt16Field);
    data = field.i16;
}

void operator<<(uint16_t& data, ADBusIterator& i)
{ 
    struct ADBusField field;
    ADBusIterate(&i, &field);
    adbus::CheckForDemarshallError(&field, ADBusUInt16Field);
    data = field.u16;
}

void operator<<(int32_t& data, ADBusIterator& i)
{ 
    struct ADBusField field;
    ADBusIterate(&i, &field);
    adbus::CheckForDemarshallError(&field, ADBusInt32Field);
    data = field.i32;
}

void operator<<(uint32_t& data, ADBusIterator& i)
{ 
    struct ADBusField field;
    ADBusIterate(&i, &field);
    adbus::CheckForDemarshallError(&field, ADBusUInt32Field);
    data = field.u32;
}

void operator<<(int64_t& data, ADBusIterator& i)
{ 
    struct ADBusField field;
    ADBusIterate(&i, &field);
    adbus::CheckForDemarshallError(&field, ADBusInt64Field);
    data = field.i64;
}

void operator<<(uint64_t& data, ADBusIterator& i)
{ 
    struct ADBusField field;
    ADBusIterate(&i, &field);
    adbus::CheckForDemarshallError(&field, ADBusUInt64Field);
    data = field.u64;
}

void operator<<(double& data, ADBusIterator& i)
{ 
    struct ADBusField field;
    ADBusIterate(&i, &field);
    adbus::CheckForDemarshallError(&field, ADBusDoubleField);
    data = field.d;
}

void operator<<(const char*& str, ADBusIterator& i)
{ 
    struct ADBusField field;
    ADBusIterate(&i, &field);
    adbus::CheckForDemarshallError(&field, ADBusStringField);
    str = field.string;
}

void operator<<(std::string& str, ADBusIterator& i)
{
    struct ADBusField field;
    ADBusIterate(&i, &field);
    adbus::CheckForDemarshallError(&field, ADBusStringField);
    str.clear();
    str.insert(str.end(), field.string, field.string + field.size);
}

void operator<<(adbus::MessageEnd& end, ADBusIterator& i)
{
    (void) end;
    struct ADBusField field;
    ADBusIterate(&i, &field);
    adbus::CheckForDemarshallError(&field, ADBusEndField);
}

