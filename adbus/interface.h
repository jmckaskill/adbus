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

#include "misc.h"
#include "dmem/hash.h"
#include "dmem/vector.h"
#include <adbus.h>

// ----------------------------------------------------------------------------

DHASH_MAP_INIT_STR(StringPair, char*);
DVECTOR_INIT(String, char*);

typedef enum adbusI_MemberType
{
    ADBUSI_METHOD,
    ADBUSI_SIGNAL,
    ADBUSI_PROPERTY,
} adbusI_MemberType;

struct adbus_Member
{
    /** \privatesection */
    dh_strsz_t              name;

    adbus_Interface*        interface;
    adbusI_MemberType       type;

    char*                   propertyType;

    d_Vector(String)        arguments;
    d_Vector(String)        returns;
    d_String                argsig;
    d_String                retsig;

    d_Hash(StringPair)      annotations;

    adbus_MsgCallback       methodCallback;
    adbus_MsgCallback       getPropertyCallback;
    adbus_MsgCallback       setPropertyCallback;

    adbus_Callback          release[2];
    void*                   ruser[2];

    void*                   methodData;
    void*                   getPropertyData;
    void*                   setPropertyData;
};

DHASH_MAP_INIT_STRSZ(MemberPtr, adbus_Member*)

// ----------------------------------------------------------------------------

struct adbus_Interface
{
    /** \privatesection */
    volatile long           ref;
    dh_strsz_t              name;
    d_Hash(MemberPtr)       members;
};

// ----------------------------------------------------------------------------

ADBUSI_FUNC int adbusI_introspect(adbus_CbData* details);
ADBUSI_FUNC int adbusI_getProperty(adbus_CbData* details);
ADBUSI_FUNC int adbusI_getAllProperties(adbus_CbData* details);
ADBUSI_FUNC int adbusI_setProperty(adbus_CbData* details);





