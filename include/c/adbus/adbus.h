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

#ifndef ADBUS_H
#define ADBUS_H

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include <stdlib.h>
#include <setjmp.h>


#ifdef __cplusplus
    extern "C" {
#endif

#if defined _WIN32 && defined ADBUS_SHARED
#   if defined ADBUS_LIBRARY
#       define ADBUS_API __declspec(dllexport)
#   else
#       define ADBUS_API __declspec(dllimport)
#   endif
#else
#   define ADBUS_API extern
#endif


enum
{
    ADBUS_SERVICE_ALLOW_REPLACEMENT         = 0x01,
    ADBUS_SERVICE_REPLACE_EXISTING          = 0x02,
    ADBUS_SERVICE_DO_NOT_QUEUE              = 0x04,

    ADBUS_SERVICE_SUCCESS                   = 1,

    ADBUS_SERVICE_REQUEST_IN_QUEUE          = 1,
    ADBUS_SERVICE_REQUEST_FAILED            = 2,
    ADBUS_SERVICE_REQUEST_ALREADY_OWNER     = 3,

    ADBUS_SERVICE_RELEASE_INVALID_NAME      = 2,
    ADBUS_SERVICE_RELEASE_NOT_OWNER         = 3,
};

enum adbus_MessageType
{
    ADBUS_MSG_INVALID               = 0,
    ADBUS_MSG_METHOD                = 1,
    ADBUS_MSG_RETURN                = 2,
    ADBUS_MSG_ERROR                 = 3,
    ADBUS_MSG_SIGNAL                = 4,
};


enum adbus_FieldType
{
    ADBUS_END_FIELD         = 0,
    ADBUS_UINT8             = 'y',
    ADBUS_BOOLEAN           = 'b',
    ADBUS_INT16             = 'n',
    ADBUS_UINT16            = 'q',
    ADBUS_INT32             = 'i',
    ADBUS_UINT32            = 'u',
    ADBUS_INT64             = 'x',
    ADBUS_UINT64            = 't',
    ADBUS_DOUBLE            = 'd',
    ADBUS_STRING            = 's',
    ADBUS_OBJECT_PATH       = 'o',
    ADBUS_SIGNATURE         = 'g',
    ADBUS_ARRAY_BEGIN       = 'a',
    ADBUS_ARRAY_END         = 1,
    ADBUS_STRUCT_BEGIN      = '(',
    ADBUS_STRUCT_END        = ')',
    ADBUS_DICT_ENTRY_BEGIN         = '{',
    ADBUS_DICT_ENTRY_END           = '}',
    ADBUS_VARIANT_BEGIN     = 'v',
    ADBUS_VARIANT_END       = 2,
};

enum
{
    ADBUS_MSG_NO_REPLY      = 1,
    ADBUS_MSG_NO_AUTOSTART  = 2,
};

enum adbus_BusType
{
    ADBUS_SYSTEM_BUS,
    ADBUS_SESSION_BUS,
    ADBUS_OTHER_BUS
};

typedef struct adbus_Buffer         adbus_Buffer;
typedef struct adbus_Caller         adbus_Caller;
typedef struct adbus_CbData         adbus_CbData;
typedef struct adbus_Connection     adbus_Connection;
typedef struct adbus_Field          adbus_Field;
typedef struct adbus_Interface      adbus_Interface;
typedef struct adbus_Iterator       adbus_Iterator;
typedef struct adbus_Match          adbus_Match;
typedef struct adbus_MatchArgument  adbus_MatchArgument;
typedef struct adbus_Member         adbus_Member;
typedef struct adbus_Message        adbus_Message;
typedef struct adbus_Object         adbus_Object;
typedef struct adbus_Path           adbus_Path;
typedef struct adbus_Proxy          adbus_Proxy;
typedef struct adbus_Signal         adbus_Signal;
typedef struct adbus_Stream         adbus_Stream;
typedef struct adbus_User           adbus_User;
typedef enum adbus_BusType          adbus_BusType;
typedef enum adbus_FieldType        adbus_FieldType;
typedef enum adbus_MessageType      adbus_MessageType;
typedef unsigned int                adbus_Bool;


#ifdef _WIN32
    typedef uintptr_t adbus_Socket;
#   define ADBUS_SOCK_INVALID ~((adbus_Socket) 0)
#else
    typedef int adbus_Socket;
#   define ADBUS_SOCK_INVALID -1
#endif


typedef int (*adbus_Callback)(
        adbus_CbData*       data);

typedef void (*adbus_SendCallback)(
        adbus_Message*      m,
        const adbus_User*   user);

typedef void (*adbus_ConnectCallback)(
        const char*         unique,
        size_t              size,
        const adbus_User*   user);

typedef void (*adbus_NameCallback)(
        const adbus_User*   user,
        int                 ret);

typedef void (*adbus_FreeFunction)(
        adbus_User*         data);

typedef void    (*adbus_AuthSendCallback)(void*, const char*, size_t);
typedef int     (*adbus_AuthRecvCallback)(void*, char*, size_t);
typedef uint8_t (*adbus_AuthRandCallback)(void*);


struct adbus_CbData
{
    adbus_Connection* connection;
 
    // Incoming message
    // Valid only if callback is originally in response to a method call
    adbus_Message* message;
    // Valid for method call callbacks
    adbus_Iterator* args;
 
    // Response
    // manualReply indicates to the callee whether there is a return message
    adbus_Bool manualReply;
    // Messages to use for replying - may be NULL if the original caller
    // requested no reply
    // To send an error set the replyType to ADBusErrorMessageReply
    // and use ADBusSetupError
    adbus_Message* retmessage;
    adbus_Buffer* retargs;
 
    // Properties
    // The iterator can be used to get the new property value and on a get
    // callback, the marshaller should be filled with the property value.
    // Based off of the message (if non NULL) or other conditions the
    // callback code may want to send back an error (if non NULL).
    adbus_Iterator* propertyIterator;
    adbus_Buffer* propertyMarshaller;
 
    // User data
    // For Interface callbacks:
    // User1 is from ADBusSetMethodCallCallback etc
    // User2 is from ADBusBindInterface
    // For match callbacks both are from ADBusAddMatch
    const adbus_User* user1;
    const adbus_User* user2;
 
    // The jmpbuf will be setup for use by any callback, and is used by the
    // ADBusCheck* functions
    // You should setup returnMessage with an error message or set parseError
    // before longjmp'ing - the longjmp argument is ignored
    jmp_buf jmpbuf;
};



ADBUS_API int adbus_auth_dbuscookiesha1(
        adbus_AuthSendCallback  send,
        adbus_AuthRecvCallback  recv,
        adbus_AuthRandCallback  rand,
        void*                   data);

ADBUS_API int adbus_auth_external(
        adbus_AuthSendCallback  send,
        adbus_AuthRecvCallback  recv,
        void*                   data);







ADBUS_API int adbus_error(
        adbus_CbData*           details,
        const char*             error,
        const char*             format,
        ...);

ADBUS_API int adbus_error_longjmp(
        adbus_CbData*           details,
        const char*             error,
        const char*             format,
        ...);

ADBUS_API void adbus_setup_error(
        adbus_CbData*           details,
        const char*             error,
        int                     errorSize,
        const char*             message,
        int                     messageSize);

ADBUS_API void adbus_setup_signal(
        adbus_Message*          message,
        adbus_Path*             path,
        adbus_Member*           signal);







ADBUS_API adbus_Connection* adbus_conn_new(void);
ADBUS_API void adbus_conn_free(adbus_Connection* connection);

ADBUS_API void adbus_conn_setsender(
        adbus_Connection*       connection,
        adbus_SendCallback      callback,
        adbus_User*             data);

ADBUS_API void adbus_conn_send(
        adbus_Connection*       connection,
        adbus_Message*          message);

ADBUS_API uint32_t adbus_conn_serial(
        adbus_Connection*       connection);

ADBUS_API int adbus_conn_dispatch(
        adbus_Connection*       connection,
        adbus_Message*          message);

ADBUS_API int adbus_conn_rawdispatch(
        adbus_CbData*           data);

ADBUS_API int adbus_conn_parse(
        adbus_Connection*       connection,
        const uint8_t*          data,
        size_t                  size);

ADBUS_API void adbus_conn_connect(
        adbus_Connection*       connection,
        adbus_ConnectCallback   callback,
        adbus_User*             user);

ADBUS_API adbus_Bool adbus_conn_isconnected(
        const adbus_Connection* connection);

ADBUS_API const char* adbus_conn_uniquename(
        const adbus_Connection* connection,
        size_t*                 size);




ADBUS_API uint32_t adbus_conn_requestname(
        adbus_Connection*       connection,
        const char*             service,
        int                     size,
        uint32_t                flags,
        adbus_NameCallback      callback,
        adbus_User*             user);

ADBUS_API uint32_t adbus_conn_releasename(
        adbus_Connection*       connection,
        const char*             service,
        int                     size,
        adbus_NameCallback      callback,
        adbus_User*             user);








struct adbus_Caller 
{
    adbus_Connection*       connection;
    adbus_Message*          msg;

    adbus_MessageType       type;
    uint32_t                serial;
    uint8_t                 flags;

    const char*             destination;
    int                     destinationSize;
    const char*             path;
    int                     pathSize;
    const char*             interface;
    int                     interfaceSize;
    const char*             member;
    int                     memberSize;

    adbus_Callback          callback;
    adbus_User*             user1;
    adbus_User*             user2;

    adbus_Callback          errorCallback;
    adbus_User*             errorUser1;
    adbus_User*             errorUser2;

    uint32_t                matchId;
    uint32_t                errorMatchId;

};

ADBUS_API void adbus_call_init(
        adbus_Caller*       caller,
        adbus_Connection*   connection,
        adbus_Message*      message);

ADBUS_API uint32_t adbus_call_send(
        adbus_Caller*       caller);


ADBUS_API void adbus_call_signal(
        adbus_Caller*       caller,
        adbus_Signal*       signal);

ADBUS_API void adbus_call_proxy(
        adbus_Caller*       caller,
        adbus_Proxy*        proxy,
        const char*         method,
        int                 size);












ADBUS_API adbus_Interface* adbus_iface_new(const char* name, int size); 
ADBUS_API void adbus_iface_free(adbus_Interface* interface);

ADBUS_API adbus_Member* adbus_iface_addmethod(
        adbus_Interface*    interface,
        const char*         name,
        int                 size);

ADBUS_API adbus_Member* adbus_iface_addsignal(
        adbus_Interface*    interface,
        const char*         name,
        int                 size);

ADBUS_API adbus_Member* adbus_iface_addproperty(
        adbus_Interface*    interface,
        const char*         name,
        int                 namesize,
        const char*         sig,
        int                 sigsize);

ADBUS_API adbus_Member* adbus_iface_method(
        adbus_Interface*    interface,
        const char*         name,
        int                 size);

ADBUS_API adbus_Member* adbus_iface_signal(
        adbus_Interface*    interface,
        const char*         name,
        int                 size);

ADBUS_API adbus_Member* adbus_iface_property(
        adbus_Interface*    interface,
        const char*         name,
        int                 size);


ADBUS_API void adbus_mbr_addannotation(
        adbus_Member*       member,
        const char*         name,
        int                 nameSize,
        const char*         value,
        int                 valueSize);


ADBUS_API void adbus_mbr_addargument(
        adbus_Member*       member,
        const char*         name,
        int                 nameSize,
        const char*         type,
        int                 typeSize);


ADBUS_API void adbus_mbr_addreturn(
        adbus_Member*       member,
        const char*         name,
        int                 nameSize,
        const char*         type,
        int                 typeSize);

// Methods
ADBUS_API void adbus_mbr_setmethod(
        adbus_Member*       member,
        adbus_Callback      callback,
        adbus_User*         user1);


// Properties

ADBUS_API void adbus_mbr_setgetter(
        adbus_Member*       member,
        adbus_Callback      callback,
        adbus_User*         user1);

ADBUS_API void adbus_mbr_setsetter(
        adbus_Member*       member,
        adbus_Callback      callback,
        adbus_User*         user1);





ADBUS_API adbus_Iterator* adbus_iter_new(void);
ADBUS_API void adbus_iter_free(adbus_Iterator* iterator);

ADBUS_API void adbus_iter_reset(
        adbus_Iterator*     iterator,
        const char*         sig,
        int                 sigsize,
        const uint8_t*      data,
        size_t              size);

ADBUS_API void adbus_iter_setnonnative(
        adbus_Iterator*     iterator);

// ----------------------------------------------------------------------------

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

struct adbus_Field 
{
    /** The type of field returned.
     *
     * Based on this only the related fields will be initialised.
     */
    adbus_FieldType type;

    adbus_Bool  b;     /**< boolean data */
    uint8_t     u8;    /**< uint8 data */
    int16_t     i16;   /**< int16 data */
    uint16_t    u16;   /**< uint16 data */
    int32_t     i32;   /**< int32 data */
    uint32_t    u32;   /**< uint32 data */
    int64_t     i64;   /**< int64 data */
    uint64_t    u64;   /**< uint64 data */
    double      d;     /**< double data */

    /** Data pointer for the array begin field.
     *
     * The size is specified in the size field.
     *
     * This can be used in combination with \ref adbus_iter_jumpToArrayEnd with a
     * fixed size data member to reference the array data directly in the
     * buffer.
     *
     * For example (this should really also be checking error codes or using
     * the check functions - \ref adbus_check_arrayBegin):
     *
     * \code
     * adbus_Field field;
     * adbus_iter_next(iter, &field); // field.type == ADBUS_ARRAY_BEGIN
     * struct my_struct_t* p = (struct my_struct_t*) field->data;
     * size_t count = field->size / sizeof(struct my_struct_t);
     * adbus_iter_jumpToArrayEnd(iter, field.scope);
     * adbus_iter_next(iter, &field); // field.type == ADBUS_ARRAY_END
     * \endcode
     */
    const uint8_t* data;

    /** String pointer for string fields (string, object path, signature) and the
     * type of variants.
     *
     * The string is guarenteed by the dbus protocol and the iterator to be
     * valid UTF8, contain no embedded nulls and have a terminating null.
     */
    const char* string;

    /** Indicates the size of the data.
     *  This is used for both the string and data fields.
     */
    size_t size;

    /** The scope of the current field.
     *
     * The scoped begin/end functions all increment this when returning their
     * begin field (eg \ref ADBusBeginArrayField) and decrement this when
     * returning their end field (eg \ref ADBusEndArrayField).
     *
     * The scoped types are:
     * - variant
     * - struct
     * - array
     * - dict entry
     *
     */
    int scope;
};

ADBUS_API int adbus_iter_isfinished(adbus_Iterator* i, int scope);
ADBUS_API int adbus_iter_next(adbus_Iterator* i, adbus_Field* field);
ADBUS_API int adbus_iter_arrayjump(adbus_Iterator* i, int scope);

// ----------------------------------------------------------------------------

/* Check functions in the style of the lua check functions (eg luaL_checkint).
 *
 * They will longjmp back to throw an invalid argument
 * error. Thus they should only be used to pull out the arguments at the very
 * beginning of a message callback.
 */

ADBUS_API void        adbus_check_end(adbus_CbData* d);
ADBUS_API adbus_Bool  adbus_check_bool(adbus_CbData* d);
ADBUS_API uint8_t     adbus_check_uint8(adbus_CbData* d);
ADBUS_API int16_t     adbus_check_int16(adbus_CbData* d);
ADBUS_API uint16_t    adbus_check_uint16(adbus_CbData* d);
ADBUS_API int32_t     adbus_check_int32(adbus_CbData* d);
ADBUS_API uint32_t    adbus_check_uint32(adbus_CbData* d);
ADBUS_API int64_t     adbus_check_int64(adbus_CbData* d);
ADBUS_API uint64_t    adbus_check_uint64(adbus_CbData* d);
ADBUS_API double      adbus_check_double(adbus_CbData* d);
ADBUS_API const char* adbus_check_string(adbus_CbData* d, size_t* size);
ADBUS_API const char* adbus_check_objectpath(adbus_CbData* d, size_t* size);
ADBUS_API const char* adbus_check_signature(adbus_CbData* d, size_t* size);
ADBUS_API int         adbus_check_arraybegin(adbus_CbData* d, const uint8_t** data, size_t* size);
ADBUS_API void        adbus_check_arrayend(adbus_CbData* d);
ADBUS_API int         adbus_check_structbegin(adbus_CbData* d);
ADBUS_API void        adbus_check_structend(adbus_CbData* d);
ADBUS_API int         adbus_check_mapbegin(adbus_CbData* d);
ADBUS_API void        adbus_check_mapend(adbus_CbData* d);
ADBUS_API const char* adbus_check_variantbegin(adbus_CbData* d, size_t* size);
ADBUS_API void        adbus_check_variantend(adbus_CbData* d);























ADBUS_API adbus_Buffer* adbus_buf_new(void);
ADBUS_API void adbus_buf_free(adbus_Buffer* buffer);
ADBUS_API void adbus_buf_reset(adbus_Buffer* buffer);

// Data and string is valid until the buffer is reset, freed, or updated
ADBUS_API void adbus_buf_get(
        const adbus_Buffer*   buffer,
        const char**          sig,
        size_t*               sigsize,
        const uint8_t**       data,
        size_t*               datasize);

ADBUS_API void adbus_buf_set(
        adbus_Buffer*         buffer,
        const char*           sig,
        int                   sigsize,
        const uint8_t*        data,
        size_t                datasize);

ADBUS_API int adbus_buf_append(adbus_Buffer* m, const char* sig, int size);

ADBUS_API adbus_FieldType adbus_buf_expected(adbus_Buffer* b);

ADBUS_API int adbus_buf_appenddata(adbus_Buffer* b, const uint8_t* data, size_t size);

ADBUS_API int adbus_buf_bool(adbus_Buffer* b, uint32_t data);
ADBUS_API int adbus_buf_uint8(adbus_Buffer* b, uint8_t data);
ADBUS_API int adbus_buf_int16(adbus_Buffer* b, int16_t data);
ADBUS_API int adbus_buf_uint16(adbus_Buffer* b, uint16_t data);
ADBUS_API int adbus_buf_int32(adbus_Buffer* b, int32_t data);
ADBUS_API int adbus_buf_uint32(adbus_Buffer* b, uint32_t data);
ADBUS_API int adbus_buf_int64(adbus_Buffer* b, int64_t data);
ADBUS_API int adbus_buf_uint64(adbus_Buffer* b, uint64_t data);

ADBUS_API int adbus_buf_double(adbus_Buffer* b, double data);

ADBUS_API int adbus_buf_string(adbus_Buffer* b, const char* str, int size);
ADBUS_API int adbus_buf_objectpath(adbus_Buffer* b, const char* str, int size);
ADBUS_API int adbus_buf_signature(adbus_Buffer* b, const char* str, int size);

ADBUS_API int adbus_buf_beginarray(adbus_Buffer* b);
ADBUS_API int adbus_buf_endarray(adbus_Buffer* b);

ADBUS_API int adbus_buf_beginstruct(adbus_Buffer* b);
ADBUS_API int adbus_buf_endstruct(adbus_Buffer* b);

ADBUS_API int adbus_buf_begindictentry(adbus_Buffer* b);
ADBUS_API int adbus_buf_enddictentry(adbus_Buffer* b);

ADBUS_API int adbus_buf_beginvariant(adbus_Buffer* b, const char* sig, int size);
ADBUS_API int adbus_buf_endvariant(adbus_Buffer* b);

ADBUS_API int adbus_buf_copy(adbus_Buffer* b, adbus_Iterator* i, int scope);














struct adbus_MatchArgument
{
    int             number;
    const char*     value;
    int             valueSize;
};

ADBUS_API void adbus_matchargument_init(adbus_MatchArgument* args, size_t num);



struct adbus_Match
{
    // The type is checked if it is not ADBUS_MSG_INVALID (0)
    adbus_MessageType       type;

    // Matches for signals should be added to the bus, but method returns
    // are automatically routed to us by the daemon
    adbus_Bool              addMatchToBusDaemon;
    adbus_Bool              removeOnFirstMatch;

    int64_t                 replySerial;

    // The strings will all be copied out
    const char*             sender;
    int                     senderSize;
    const char*             destination;
    int                     destinationSize;
    const char*             interface;
    int                     interfaceSize;
    const char*             path;
    int                     pathSize;
    const char*             member;
    int                     memberSize;
    const char*             errorName;
    int                     errorNameSize;

    adbus_MatchArgument*    arguments;
    size_t                  argumentsSize;

    adbus_Callback          callback;

    // The user pointers will be freed using their free function
    adbus_User*             user1;
    adbus_User*             user2;

    // The id is ignored if its not set (0), but can be set with a value
    // returned from adbus_conn_matchId
    uint32_t                id;
};

ADBUS_API void adbus_match_init(adbus_Match* match);



ADBUS_API uint32_t adbus_conn_matchid(
        adbus_Connection*     connection);

ADBUS_API uint32_t adbus_conn_addmatch(
        adbus_Connection*     connection,
        const adbus_Match*    match);

ADBUS_API void adbus_conn_removematch(
        adbus_Connection*     connection,
        uint32_t                    id);



ADBUS_API size_t adbus_parse_size(const uint8_t* data, size_t size);
ADBUS_API int    adbus_parse(adbus_Message* m, const uint8_t* data, size_t size);

ADBUS_API adbus_Message* adbus_msg_new(void);
ADBUS_API void adbus_msg_free(adbus_Message* m);
ADBUS_API void adbus_msg_reset(adbus_Message* m);


ADBUS_API const uint8_t* adbus_msg_data(const adbus_Message* m, size_t* size);
ADBUS_API void adbus_msg_build(adbus_Message* m);
ADBUS_API void adbus_msg_iterator(const adbus_Message* m, adbus_Iterator* iterator);
ADBUS_API const char* adbus_msg_summary(adbus_Message* m, size_t* size);

ADBUS_API const char* adbus_msg_path(const adbus_Message* m, size_t* len);
ADBUS_API const char* adbus_msg_interface(const adbus_Message* m, size_t* len);
ADBUS_API const char* adbus_msg_sender(const adbus_Message* m, size_t* len);
ADBUS_API const char* adbus_msg_destination(const adbus_Message* m, size_t* len);
ADBUS_API const char* adbus_msg_member(const adbus_Message* m, size_t* len);
ADBUS_API const char* adbus_msg_error(const adbus_Message* m, size_t* len);
ADBUS_API const char* adbus_msg_signature(const adbus_Message* m, size_t* len);

ADBUS_API adbus_MessageType adbus_msg_type(const adbus_Message* m);
ADBUS_API uint8_t  adbus_msg_flags(const adbus_Message* m);
ADBUS_API uint32_t adbus_msg_serial(const adbus_Message* m);
ADBUS_API adbus_Bool adbus_msg_reply(const adbus_Message* m, uint32_t* serial);


ADBUS_API void adbus_msg_settype(adbus_Message* m, adbus_MessageType type);
ADBUS_API void adbus_msg_setserial(adbus_Message* m, uint32_t serial);
ADBUS_API void adbus_msg_setflags(adbus_Message* m, uint8_t flags);
ADBUS_API void adbus_msg_setreply(adbus_Message* m, uint32_t reply);

ADBUS_API void adbus_msg_setpath(adbus_Message* m, const char* path, int size);
ADBUS_API void adbus_msg_setinterface(adbus_Message* m, const char* path, int size);
ADBUS_API void adbus_msg_setmember(adbus_Message* m, const char* path, int size);
ADBUS_API void adbus_msg_seterror(adbus_Message* m, const char* path, int size);
ADBUS_API void adbus_msg_setdestination(adbus_Message* m, const char* path, int size);
ADBUS_API void adbus_msg_setsender(adbus_Message* m, const char* path, int size);

ADBUS_API adbus_Buffer* adbus_msg_buffer(adbus_Message* m);

#define adbus_msg_append(m,t,s)     adbus_buf_append(adbus_msg_buffer(m), t, s)
#define adbus_msg_uint8(m, v)       adbus_buf_uint8(adbus_msg_buffer(m), v)
#define adbus_msg_int16(m, v)       adbus_buf_int16(adbus_msg_buffer(m), v)
#define adbus_msg_uint16(m, v)      adbus_buf_uint16(adbus_msg_buffer(m), v)
#define adbus_msg_int32(m, v)       adbus_buf_int32(adbus_msg_buffer(m), v)
#define adbus_msg_uint32(m, v)      adbus_buf_uint32(adbus_msg_buffer(m), v)
#define adbus_msg_int64(m, v)       adbus_buf_int64(adbus_msg_buffer(m), v)
#define adbus_msg_uint64(m, v)      adbus_buf_uint64(adbus_msg_buffer(m), v)
#define adbus_msg_double(m, v)      adbus_buf_double(adbus_msg_buffer(m), v)
#define adbus_msg_string(m, v, s)   adbus_buf_string(adbus_msg_buffer(m), v, s)
#define adbus_msg_beginarray(m)     adbus_buf_beginarray(adbus_msg_buffer(m))
#define adbus_msg_endarray(m)       adbus_buf_endarray(adbus_msg_buffer(m))
#define adbus_msg_beginstruct(m)    adbus_buf_beginstruct(adbus_msg_buffer(m))
#define adbus_msg_endstruct(m)      adbus_buf_endstruct(adbus_msg_buffer(m))
#define adbus_msg_beginmap(m)       adbus_buf_beginmap(adbus_msg_buffer(m))
#define adbus_msg_endmap(m)         adbus_buf_endmap(adbus_msg_buffer(m))
#define adbus_msg_beginvariant(m,t,s)   adbus_buf_beginvariant(adbus_msg_buffer(m), t, s)
#define adbus_msg_endvariant(m)         adbus_buf_endvariant(adbus_msg_buffer(m))
















ADBUS_API adbus_Object* adbus_obj_new(void);
ADBUS_API void adbus_obj_free(adbus_Object* object);
ADBUS_API void adbus_obj_reset(adbus_Object* object);

ADBUS_API int adbus_obj_bind(
        adbus_Object*       object,
        adbus_Path*         path,
        adbus_Interface*    interface,
        adbus_User*         user2);

ADBUS_API int adbus_obj_unbind(
        adbus_Object*       object,
        adbus_Path*         path,
        adbus_Interface*    interface);

ADBUS_API uint32_t adbus_obj_addmatch(
        adbus_Object*       object,
        adbus_Connection*   connection,
        const adbus_Match*  match);

ADBUS_API void adbus_obj_addmatchid(
        adbus_Object*       object,
        adbus_Connection*   connection,
        uint32_t            id);

ADBUS_API void adbus_obj_removematch(
        adbus_Object*       object,
        adbus_Connection*   connection,
        uint32_t            id);


















struct adbus_Path
{
    adbus_Connection* connection;
    const char*       string;
    size_t            size;
};



ADBUS_API adbus_Path* adbus_conn_path(
        adbus_Connection*   connection,
        const char*         path,
        int                 size);

ADBUS_API adbus_Path* adbus_path_relative(
        adbus_Path*         path,
        const char*         relpath,
        int                 size);



ADBUS_API int adbus_path_bind(
        adbus_Path*         path,
        adbus_Interface*    interface,
        adbus_User*         user2);

ADBUS_API int adbus_path_unbind(
        adbus_Path*         path,
        adbus_Interface*    interface);



ADBUS_API adbus_Interface* adbus_path_interface(
        adbus_Path*         path,
        const char*         interface,
        int                 interfaceSize,
        const adbus_User**  user2);

ADBUS_API adbus_Member* adbus_path_method(
        adbus_Path*         path,
        const char*         name,
        int                 size,
        const adbus_User**  user2);













ADBUS_API adbus_Stream* adbus_stream_new(void);
ADBUS_API void adbus_stream_free(adbus_Stream* buffer);

ADBUS_API int adbus_stream_parse(
        adbus_Stream*       stream,
        adbus_Message*      message,
        const uint8_t**     pdata,
        size_t*             size);









ADBUS_API adbus_Proxy* adbus_proxy_new(
        adbus_Connection*   connection,
        const char*         service,
        int                 ssize,
        const char*         path,
        int                 psize,
        const char*         interface,
        int                 isize);

ADBUS_API void adbus_proxy_free(
        adbus_Proxy*        proxy);



ADBUS_API adbus_Signal* adbus_sig_new(
        adbus_Path*         path, 
        adbus_Member*       member);

ADBUS_API void adbus_sig_free(
        adbus_Signal*       signal);





ADBUS_API adbus_Socket adbus_sock_connect(
        adbus_BusType   type);

ADBUS_API adbus_Socket adbus_sock_envconnect(
        const char*     envstr,
        int             size);




struct adbus_User
{
    adbus_FreeFunction free;
};

ADBUS_API void adbus_user_free(adbus_User* data);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ADBUS_H */

