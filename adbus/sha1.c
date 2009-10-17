/* sha1.cpp

Copyright (c) 2005 Michael D. Leonhard

http://tamale.net/

This file is licensed under the terms described in the
accompanying LICENSE file.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "sha1.h"

// circular left bit rotation.  MSB wraps around to LSB
static uint32_t lrot( uint32_t x, int bits )
{
	return (x<<bits) | (x>>(32 - bits));
};

// Save a 32-bit unsigned integer to memory, in big-endian order
static void storeBigEndianUint32(unsigned char* byte, uint32_t num )
{
	assert( byte );
	byte[0] = (unsigned char)(num>>24);
	byte[1] = (unsigned char)(num>>16);
	byte[2] = (unsigned char)(num>>8);
	byte[3] = (unsigned char)num;
}


// Constructor *******************************************************

void SHA1Init(SHA1* s)
{
    // make sure that the data type is the right s->size
    assert( sizeof( uint32_t ) * 5 == 20 );
	
	// initialize
    s->H0 = 0x67452301;
    s->H1 = 0xefcdab89;
    s->H2 = 0x98badcfe;
    s->H3 = 0x10325476;
    s->H4 = 0xc3d2e1f0;
    s->unprocessedBytes = 0;
    s->size = 0;
}

// process ***********************************************************
static void process(SHA1* s)
{
    assert( s->unprocessedBytes == 64 );
    //printf( "process: " ); hexPrinter( s->bytes, 64 ); printf( "\n" );
	int t;
    uint32_t a, b, c, d, e, K, f, W[80];
	// starting values
    a = s->H0;
    b = s->H1;
    c = s->H2;
    d = s->H3;
    e = s->H4;
	// copy and expand the message block
    for( t = 0; t < 16; t++ ) W[t] = (s->bytes[t*4] << 24)
                                    +(s->bytes[t*4 + 1] << 16)
                                    +(s->bytes[t*4 + 2] << 8)
                                    + s->bytes[t*4 + 3];
	for(; t< 80; t++ ) W[t] = lrot( W[t-3]^W[t-8]^W[t-14]^W[t-16], 1 );
	
	/* main loop */
    uint32_t temp;
	for( t = 0; t < 80; t++ )
	{
		if( t < 20 ) {
			K = 0x5a827999;
			f = (b & c) | ((b ^ 0xFFFFFFFF) & d);//TODO: try using ~
		} else if( t < 40 ) {
			K = 0x6ed9eba1;
			f = b ^ c ^ d;
		} else if( t < 60 ) {
			K = 0x8f1bbcdc;
			f = (b & c) | (b & d) | (c & d);
		} else {
			K = 0xca62c1d6;
			f = b ^ c ^ d;
		}
		temp = lrot(a,5) + f + e + W[t] + K;
		e = d;
		d = c;
		c = lrot(b,30);
		b = a;
		a = temp;
		//printf( "t=%d %08x %08x %08x %08x %08x\n",t,a,b,c,d,e );
	}
	/* add variables */
    s->H0 += a;
    s->H1 += b;
    s->H2 += c;
    s->H3 += d;
    s->H4 += e;
    //printf( "Current: %08x %08x %08x %08x %08x\n",s->H0,s->H1,s->H2,s->H3,s->H4 );
    /* all s->bytes have been processed */
    s->unprocessedBytes = 0;
}

// addBytes **********************************************************
void SHA1AddBytes(SHA1* s, const char* data, int num )
{
	assert( data );
	assert( num > 0 );
    // add these s->bytes to the running total
    s->size += num;
	// repeat until all data is processed
	while( num > 0 )
	{
        // number of s->bytes required to complete block
        int needed = 64 - s->unprocessedBytes;
		assert( needed > 0 );
        // number of s->bytes to copy (use smaller of two)
		int toCopy = (num < needed) ? num : needed;
        // Copy the s->bytes
        memcpy( s->bytes + s->unprocessedBytes, data, toCopy );
		// Bytes have been copied
		num -= toCopy;
		data += toCopy;
        s->unprocessedBytes += toCopy;
		
		// there is a full block
        if( s->unprocessedBytes == 64 ) process(s);
	}
}

// digest ************************************************************
unsigned char* SHA1GetDigest(SHA1* s)
{
    // save the message s->size
    uint32_t totalBitsL = s->size << 3;
    uint32_t totalBitsH = s->size >> 29;
	// add 0x80 to the message
    SHA1AddBytes(s, "\x80", 1 );
	
	unsigned char footer[64] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	// block has no room for 8-byte filesize, so finish it
    if( s->unprocessedBytes > 56 )
        SHA1AddBytes(s, (char*)footer, 64 - s->unprocessedBytes);
    assert( s->unprocessedBytes <= 56 );
	// how many zeros do we need
    int neededZeros = 56 - s->unprocessedBytes;
    // store file s->size (in bits) in big-endian format
    storeBigEndianUint32(footer + neededZeros    , totalBitsH );
    storeBigEndianUint32(footer + neededZeros + 4, totalBitsL );
	// finish the final block
    SHA1AddBytes(s, (char*)footer, neededZeros + 8 );
    // allocate memory for the digest s->bytes
	unsigned char* digest = (unsigned char*)malloc( 20 );
    // copy the digest s->bytes
    storeBigEndianUint32(digest, s->H0 );
    storeBigEndianUint32(digest + 4, s->H1 );
    storeBigEndianUint32(digest + 8, s->H2 );
    storeBigEndianUint32(digest + 12, s->H3 );
    storeBigEndianUint32(digest + 16, s->H4 );
	// return the digest
	return digest;
}
