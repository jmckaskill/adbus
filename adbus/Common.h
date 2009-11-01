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

/** \defgroup adbus adbus
 *
 * \brief Library brief
 *
 * Library description
 *
 */


#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>


#ifdef __cplusplus
#   define ADBUS_EXTERN extern "C"
#   define ADBUS_EXTERN_BLOCK_BEGIN extern "C" {
#   define ADBUS_EXTERN_BLOCK_END }
#else
#   define ADBUS_EXTERN extern
#   define ADBUS_EXTERN_BLOCK_BEGIN
#   define ADBUS_EXTERN_BLOCK_END
#endif


#ifdef ADBUS_BUILD_AS_DLL
#   ifdef ADBUS_LIBRARY
#       define ADBUS_API ADBUS_EXTERN __declspec(dllexport)
#   else
#       define ADBUS_API ADBUS_EXTERN __declspec(dllimport)
#   endif
#else
#   define ADBUS_API ADBUS_EXTERN
#endif


#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && defined(__ELF__)
#   define ADBUSI_FUNC __attribute__((visibility("hidden"))) ADBUS_EXTERN
#   define ADBUSI_DATA __attribute__((visibility("hidden"))) ADBUS_EXTERN
#   define ADBUS_CALLBACK __attribute((visibility("hidden"))) ADBUS_EXTERN
#else
#   define ADBUSI_FUNC
#   define ADBUSI_DATA ADBUS_EXTERN
#   define ADBUS_CALLBACK ADBUS_EXTERN
#endif


/* Pulled from http://dbus.freedesktop.org/doc/dbus-specification.html
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | Name        | Code             | Description                          | Alignment   | Encoding                                      |
 * +=============+==================+======================================+=============+===============================================+
 * | INVALID     | 0 (ASCII NUL)    | Not a valid type code, used to       | N/A         | Not applicable; cannot be marshaled.          |
 * |             |                  | terminate signatures                 |             |                                               |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | BYTE        | 121 (ASCII 'y')  | 8-bit unsigned integer               | 1           | A single 8-bit byte.                          |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | BOOLEAN     | 98 (ASCII 'b')   | Boolean value, 0 is FALSE and 1      | 4           | As for UINT32, but only 0 and 1 are valid     |
 * |             |                  | is TRUE. Everything else is invalid. |             | values.                                       |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | INT16       | 110 (ASCII 'n')  | 16-bit signed integer                | 2           | 16-bit signed integer in the message's byte   |
 * |             |                  |                                      |             | order.                                        |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | UINT16      | 113 (ASCII 'q')  | 16-bit unsigned integer              | 2           | 16-bit unsigned integer in the message's byte |
 * |             |                  |                                      |             | order.                                        |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | INT32       | 105 (ASCII 'i')  | 32-bit signed integer                | 4           | 32-bit signed integer in the message's byte   |
 * |             |                  |                                      |             | order.                                        |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | UINT32      | 117 (ASCII 'u')  | 32-bit unsigned integer              | 4           | 32-bit unsigned integer in the message's byte |
 * |             |                  |                                      |             | order.                                        |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | INT64       | 120 (ASCII 'x')  | 64-bit signed integer                | 8           | 64-bit signed integer in the message's byte   |
 * |             |                  |                                      |             | order.                                        |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | UINT64      | 116 (ASCII 't')  | 64-bit unsigned integer              | 8           | 64-bit unsigned integer in the message's byte |
 * |             |                  |                                      |             | order.                                        |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | DOUBLE      | 100 (ASCII 'd')  | IEEE 754 double                      | 8           | 64-bit IEEE 754 double in the message's byte  |
 * |             |                  |                                      |             | order.                                        |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | STRING      | 115 (ASCII 's')  | UTF-8 string (must be valid UTF-8).  | 4 (for      | A UINT32 indicating the string's length in    |
 * |             |                  | Must be nul terminated and contain   | the length) | bytes excluding its terminating nul, followed |
 * |             |                  | no other nul bytes.                  |             | by non-nul string data of the given length,   |
 * |             |                  |                                      |             | followed by a terminating nul byte.           |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | OBJECT_PATH | 111 (ASCII 'o')  | Name of an object instance           | 4 (for      | Exactly the same as STRING except the content |
 * |             |                  |                                      | the length) | must be a valid object path (see below).      |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | SIGNATURE   | 103 (ASCII 'g')  | A type signature                     | 1           | The same as STRING except the length is a     |
 * |             |                  |                                      |             | single byte (thus signatures have a maximum   |
 * |             |                  |                                      |             | length of 255) and the content must be a      |
 * |             |                  |                                      |             | valid signature (see below).                  |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | ARRAY       | 97 (ASCII 'a')   | Array                                | 4 (for      | A UINT32 giving the length of the array data  |
 * |             |                  |                                      | the length) | in bytes, followed by alignment padding to    |
 * |             |                  |                                      |             | the alignment boundary of the array element   |
 * |             |                  |                                      |             | type, followed by each array element. The     |
 * |             |                  |                                      |             | array length is from the end of the alignment |
 * |             |                  |                                      |             | padding to the end of the last element, i.e.  |
 * |             |                  |                                      |             | it does not include the padding after the     |
 * |             |                  |                                      |             | length, or any padding after the last         |
 * |             |                  |                                      |             | element. Arrays have a maximum length defined |
 * |             |                  |                                      |             | to be 2 to the 26th power or 67108864.        |
 * |             |                  |                                      |             | Implementations must not send or accept       |
 * |             |                  |                                      |             | arrays exceeding this length.                 |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | STRUCT      | 114 (ASCII 'r'), | Struct                               | 8           | A struct must start on an 8-byte boundary     |
 * |             | 40 (ASCII '('),  |                                      |             | regardless of the type of the struct fields.  |
 * |             | 41 (ASCII ')')   |                                      |             | The struct value consists of each field       |
 * |             |                  |                                      |             | marshaled in sequence starting from that      |
 * |             |                  |                                      |             | 8-byte alignment boundary.                    |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | VARIANT     | 118 (ASCII 'v')  | Variant type (the type of the        | 1           | A variant type has a marshaled SIGNATURE      |
 * |             |                  | value is part of the value           | (alignment  | followed by a marshaled value with the type   |
 * |             |                  | itself)                              | of          | given in the signature. Unlike a message      |
 * |             |                  |                                      | signature)  | signature, the variant signature can contain  |
 * |             |                  |                                      |             | only a single complete type.  So "i" is OK,   |
 * |             |                  |                                      |             | "ii" is not.                                  |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | DICT_ENTRY  | 101 (ASCII 'e'), | Entry in a dict or map (array        | 8           | Identical to STRUCT.                          |
 * |             | 123 (ASCII '{'), | of key-value pairs)                  |             |                                               |
 * |             | 125 (ASCII '}')  |                                      |             |                                               |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 */

/* ------------------------------------------------------------------------- */

typedef unsigned int uint;

/* ------------------------------------------------------------------------- */

enum
{
    ADBusMaximumArrayLength   = 67108864,
    ADBusMaximumMessageLength = 134217728,
};

// ----------------------------------------------------------------------------

enum ADBusMessageType
{
    ADBusInvalidMessage       = 0,
    ADBusMethodCallMessage    = 1,
    ADBusMethodReturnMessage  = 2,
    ADBusErrorMessage         = 3,
    ADBusSignalMessage        = 4,
    ADBusMessageTypeMax       = 4,
};

// ----------------------------------------------------------------------------

enum
{
    ADBusNoReplyExpectedFlag   = 1,
    ADBusNoAutoStartFlag       = 2,
};

// ----------------------------------------------------------------------------

enum ADBusHeaderFieldCode
{
    ADBusInvalidCode      = 0,
    ADBusPathCode         = 1,
    ADBusInterfaceCode    = 2,
    ADBusMemberCode       = 3,
    ADBusErrorNameCode    = 4,
    ADBusReplySerialCode  = 5,
    ADBusDestinationCode  = 6,
    ADBusSenderCode       = 7,
    ADBusSignatureCode    = 8,
};

// ----------------------------------------------------------------------------

enum ADBusFieldType
{
    ADBusInvalidField          = 0,
    ADBusEndField              = 0,
    ADBusUInt8Field            = 'y',
    ADBusBooleanField          = 'b',
    ADBusInt16Field            = 'n',
    ADBusUInt16Field           = 'q',
    ADBusInt32Field            = 'i',
    ADBusUInt32Field           = 'u',
    ADBusInt64Field            = 'x',
    ADBusUInt64Field           = 't',
    ADBusDoubleField           = 'd',
    ADBusStringField           = 's',
    ADBusObjectPathField       = 'o',
    ADBusSignatureField        = 'g',
    ADBusArrayBeginField       = 'a',
    ADBusArrayEndField         = 1,
    ADBusStructBeginField      = '(',
    ADBusStructEndField        = ')',
    ADBusDictEntryBeginField   = '{',
    ADBusDictEntryEndField     = '}',
    ADBusVariantBeginField     = 'v',
    ADBusVariantEndField       = 2,
};

// ----------------------------------------------------------------------------

enum ADBusParseError
{
    ADBusErrorJmp = -2,
    ADBusInternalError = -1,
    ADBusSuccess = 0,
    ADBusInvalidData,
    ADBusInvalidVersion,
    ADBusInvalidAlignment,
    ADBusInvalidArgument,
};

// ----------------------------------------------------------------------------

enum ADBusMemberType
{
    ADBusMethodMember,
    ADBusSignalMember,
    ADBusPropertyMember,
};

// ----------------------------------------------------------------------------

// Flags to be sent to the bus with ADBusRequestServiceName
enum
{
    // Normally when requesting a name, if there already exists an owner,
    // we get queued waiting for the previous owner to disconnect
    // The previous can indicate that it will allow replacement via the allow
    // flag and then we can take it over by using the replace flag.
    // Alternatively we can indicate that we don't want to be placed in a queue
    // (rather it should just fail to acquire).
    ADBusServiceRequestAllowReplacementFlag = 0x01,
    ADBusServiceRequestReplaceExistingFlag  = 0x02,
    ADBusServiceRequestDoNotQueueFlag       = 0x04,
};

// Value returned by the bus with ADBusRequestServiceName and
// ADBusReleaseServiceName
enum ADBusServiceCode
{
    // ADBusRequestServiceName
    // The return value can indicate whether we now have the name, are in the
    // queue to get the name, flat out failed (if we specified not to queue),
    // or we could already be the owner
    ADBusServiceRequestPrimaryOwner       = 1,
    ADBusServiceRequestInQueue            = 2,
    ADBusServiceRequestFailed             = 3,
    ADBusServiceRequestAlreadyOwner       = 4,

    // ADBusReleaseServiceName
    // The return value can indicate if the release succeeded or that it failed
    // due to the service name being invalid or since we are not the owner
    ADBusServiceReleaseSuccess            = 1,
    ADBusServiceReleaseInvalidName        = 2,
    ADBusServiceReleaseNotOwner           = 3,
};


// ----------------------------------------------------------------------------

// Forward declarations
struct ADBusCallDetails;
struct ADBusConnection;
struct ADBusFactory;
struct ADBusInterface;
struct ADBusIterator;
struct ADBusMarshaller;
struct ADBusMatch;
struct ADBusMember;
struct ADBusMessage;
struct ADBusObject;
struct ADBusObjectPath;
struct ADBusStreamBuffer;
struct ADBusUser;

ADBUS_EXTERN_BLOCK_BEGIN

typedef void (*ADBusSendCallback)(
        struct ADBusMessage*        message,
        const struct ADBusUser*     user);

typedef void (*ADBusConnectionCallback)(
        const char*                 unique,
        size_t                      uniqueSize,
        const struct ADBusUser*     user);

typedef void (*ADBusServiceCallback)(
        const struct ADBusUser*     user,
        enum ADBusServiceCode       success);

typedef int (*ADBusMessageCallback)(
        struct ADBusCallDetails*    details);

ADBUS_EXTERN_BLOCK_END

struct ADBusCallDetails
{
    struct ADBusConnection* connection;

    // Incoming message
    // Valid only if callback is originally in response to a method call
    struct ADBusMessage*    message;
    // Valid for method call callbacks
    struct ADBusIterator*   args;

    // Response
    // manualReply indicates to the callee whether there is a return message
    uint                    manualReply;
    // Messages to use for replying - may be NULL if the original caller
    // requested no reply
    // To send an error set the replyType to ADBusErrorMessageReply
    // and use ADBusSetupError
    struct ADBusMessage*    retmessage;
    struct ADBusMarshaller* retargs;

    // Properties
    // The iterator can be used to get the new property value and on a get
    // callback, the marshaller should be filled with the property value.
    // Based off of the message (if non NULL) or other conditions the
    // callback code may want to send back an error (if non NULL).
    struct ADBusIterator*   propertyIterator;
    struct ADBusMarshaller* propertyMarshaller;

    // User data
    // For Interface callbacks:
    // User1 is from ADBusSetMethodCallCallback etc
    // User2 is from ADBusBindInterface
    // For match callbacks both are from ADBusAddMatch
    const struct ADBusUser* user1;
    const struct ADBusUser* user2;

    // The jmpbuf will be setup for use by any callback, and is used by the
    // ADBusCheck* functions
    // You should setup returnMessage with an error message or set parseError
    // before longjmp'ing - the longjmp argument is ignored
    jmp_buf jmpbuf;
};
#define ADBusInitCallDetails(P) memset(P, 0, sizeof(struct ADBusCallDetails))



