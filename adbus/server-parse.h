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

#include "internal.h"

/* For parsing buffers, the routine is:
 * 1. BEGIN: Copy over basic header - enough to know how big the message is.
 *      a) This sets native, headerSize and msgSize.
 *
 * 2. HEADER
 *      a) Copy over header fields
 *      b) Endian swap header fields
 *      c) Reset endian byte to native
 *      d) Iterate over the header fields removing any sender fields
 *      e) Append correct sender field
 *      f) Update header field length
 *      g) Update headerSize and msgSize 
 *
 * 3. DATA: Copy over the data
 *      a) Copy over arguments
 *      b) Dispatch message
 */

enum adbusI_ServerParseState
{
    PARSE_DISPATCH,
    PARSE_BEGIN,
    PARSE_HEADER,
    PARSE_DATA
};

typedef enum adbusI_ServerParseState adbusI_ServerParseState;

struct adbusI_ServerParser
{
    adbusI_ServerParseState state;
    adbus_Buffer*           buffer;
    adbus_Bool              native;
    size_t                  headerSize;
    size_t                  msgSize;
};

ADBUSI_FUNC void adbusI_remote_initParser(adbusI_ServerParser* p);
ADBUSI_FUNC void adbusI_remote_freeParser(adbusI_ServerParser* p);

ADBUS_API int adbus_remote_dispatch(adbus_Remote* r, adbus_Message* m);
ADBUS_API int adbus_remote_parse(adbus_Remote* r, adbus_Buffer* buf);
