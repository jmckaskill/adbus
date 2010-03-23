/* sha1.h

Copyright (c) 2005 Michael D. Leonhard

http://tamale.net/

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef ADBUS_SHA1_H
#define ADBUS_SHA1_H

#include "internal.h"
#include <stdint.h>

typedef struct adbusI_SHA1 adbusI_SHA1;

struct adbusI_SHA1
{
		// fields
    uint32_t H0, H1, H2, H3, H4;
		unsigned char bytes[64];
		int unprocessedBytes;
    uint32_t size;
};

ADBUSI_FUNC void adbusI_SHA1Init(adbusI_SHA1* sha);
ADBUSI_FUNC void adbusI_SHA1AddBytes(adbusI_SHA1* sha, const char* data, int num);
ADBUSI_FUNC unsigned char* adbusI_SHA1GetDigest(adbusI_SHA1* sha);

#endif
