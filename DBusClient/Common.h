#pragma once

#include <stdlib.h>

#ifdef __MSC_VER
typedef unsigned __int8  uint8_t;
typedef          __int8  int8_t;
typedef unsigned __int16 uint16_t;
typedef          __int16 int16_t;
typedef unsigned __int32 uint32_t;
typedef          __int32 int32_t;
typedef unsigned __int64 uint64_t;
typedef          __int64 int64_t;
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
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


//-----------------------------------------------------------------------------

enum
{
  DBusMaximumArrayLength   = 67108864,
  DBusMaximumMessageLength = 134217728,
};

//-----------------------------------------------------------------------------

enum DBusMessageType
{
  DBusInvalidMessage       = 0,
  DBusMethodCallMessage    = 1,
  DBusMethodReturnMessage  = 2,
  DBusErrorMessage         = 3,
  DBusSignalMessage        = 4,
  DBusMessageTypeMax       = 4,
};

//-----------------------------------------------------------------------------

enum DBusFlagField
{
  DBusReplyExpectedFlag     = 1,
  DBusNoAutoStartFlag       = 2,
};

//-----------------------------------------------------------------------------

enum DBusHeaderFieldCode
{
  DBusInvalidFieldCode      = 0,
  DBusPathFieldCode         = 1,
  DBusInterfaceFieldCode    = 2,
  DBusMemberFieldCode       = 3,
  DBusErrorNameFieldCode    = 4,
  DBusReplySerialFieldCode  = 5,
  DBusDestinationFieldCode  = 6,
  DBusSenderFieldCode       = 7,
  DBusSignatureFieldCode    = 8,
};

//-----------------------------------------------------------------------------

enum DBusFieldType
{
  DBusMessageEndField       = 0,
  DBusUInt8Field            = 'y',
  DBusBooleanField          = 'b',
  DBusInt16Field            = 'n',
  DBusUInt16Field           = 'q',
  DBusInt32Field            = 'i',
  DBusUInt32Field           = 'u',
  DBusInt64Field            = 'x',
  DBusUInt64Field           = 't',
  DBusDoubleField           = 'd',
  DBusStringField           = 's',
  DBusObjectPathField       = 'o',
  DBusSignatureField        = 'g',
  DBusArrayBeginField       = 'a',
  DBusArrayEndField         = 1,
  DBusStructBeginField      = '(',
  DBusStructEndField        = ')',
  DBusDictEntryBeginField   = '{',
  DBusDictEntryEndField     = '}',
  DBusVariantBeginField     = 'v',
  DBusVariantEndField       = 2,
};

//-----------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif
