/* sha1.h

Copyright (c) 2005 Michael D. Leonhard

http://tamale.net/

This file is licensed under the terms described in the
accompanying LICENSE file.
*/

#ifndef SHA1_HEADER

#include <adbus/adbus.h>
#include "misc.h"

#include <stdint.h>

typedef struct SHA1
{
		// fields
        uint32_t H0, H1, H2, H3, H4;
		unsigned char bytes[64];
		int unprocessedBytes;
        uint32_t size;
} SHA1;

ADBUSI_FUNC void SHA1Init(SHA1* sha);
ADBUSI_FUNC void SHA1AddBytes(SHA1* sha, const char* data, int num);
ADBUSI_FUNC unsigned char* SHA1GetDigest(SHA1* sha);

#define SHA1_HEADER
#endif
