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

#include <stdint.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdarg.h>


#ifdef __cplusplus
    extern "C" {
#endif

#if defined ADBUS_SHARED_LIBRARY
#   ifdef _MSC_VER
#       if defined ADBUS_LIBRARY
#           define ADBUS_API __declspec(dllexport)
#       else
#           define ADBUS_API __declspec(dllimport)
#       endif
#   elif defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && defined(__ELF__)
#       define ADBUS_API __attribute__((visibility("default"))) extern
#   else
#       define ADBUS_API extern
#   endif
#else
#   define ADBUS_API extern
#endif

#if !defined ADBUS_INLINE
#   if defined __cplusplus || __STDC_VERSION__ + 0 >= 199901L
#       define ADBUS_INLINE static inline
#   else
#       define ADBUS_INLINE static
#   endif
#endif

#undef interface

/** \file adbus.h
 *  \brief The include file for the adbus c library.
 */

enum
{
    ADBUS_SERVICE_ALLOW_REPLACEMENT         = 0x01,
    ADBUS_SERVICE_REPLACE_EXISTING          = 0x02,
    ADBUS_SERVICE_DO_NOT_QUEUE              = 0x04,

    ADBUS_SERVICE_SUCCESS                   = 1,

    ADBUS_SERVICE_REQUEST_IN_QUEUE          = 2,
    ADBUS_SERVICE_REQUEST_FAILED            = 3,
    ADBUS_SERVICE_REQUEST_ALREADY_OWNER     = 4,

    ADBUS_SERVICE_RELEASE_INVALID_NAME      = 2,
    ADBUS_SERVICE_RELEASE_NOT_OWNER         = 3,

    ADBUS_SERVICE_START_SUCCESS             = 1,
    ADBUS_SERVICE_START_ALREADY_RUNNING     = 2
};

enum adbus_MessageType
{
    ADBUS_MSG_INVALID               = 0,
    ADBUS_MSG_METHOD                = 1,
    ADBUS_MSG_RETURN                = 2,
    ADBUS_MSG_ERROR                 = 3,
    ADBUS_MSG_SIGNAL                = 4
};


enum adbus_FieldType
{
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
    ADBUS_STRUCT_BEGIN      = '(',
    ADBUS_STRUCT_END        = ')',
    ADBUS_VARIANT_BEGIN     = 'v',
    ADBUS_DICTENTRY_BEGIN   = '{',
    ADBUS_DICTENTRY_END     = '}'
};

enum
{
    ADBUS_MSG_NO_REPLY      = 1,
    ADBUS_MSG_NO_AUTOSTART  = 2
};

enum adbus_BusType
{
    ADBUS_DEFAULT_BUS,
    ADBUS_SYSTEM_BUS,
    ADBUS_SESSION_BUS,
    ADBUS_BUS_NUM
};

enum adbus_BlockType
{
    ADBUS_WAIT_FOR_CONNECTED,
    ADBUS_BLOCK,
    ADBUS_UNBLOCK
};

typedef struct adbus_Auth               adbus_Auth;
typedef struct adbus_Argument           adbus_Argument;
typedef struct adbus_Bind               adbus_Bind;
typedef struct adbus_Buffer             adbus_Buffer;
typedef struct adbus_BufArray           adbus_BufArray;
typedef struct adbus_BufVariant         adbus_BufVariant;
typedef struct adbus_Call               adbus_Call;
typedef struct adbus_CbData             adbus_CbData;
typedef struct adbus_Connection         adbus_Connection;
typedef struct adbus_ConnectionCallbacks  adbus_ConnectionCallbacks;
typedef struct adbus_ConnBind           adbus_ConnBind;
typedef struct adbus_ConnMatch          adbus_ConnMatch;
typedef struct adbus_ConnReply          adbus_ConnReply;
typedef struct adbus_Field              adbus_Field;
typedef struct adbus_Interface          adbus_Interface;
typedef struct adbus_Iterator           adbus_Iterator;
typedef struct adbus_IterArray          adbus_IterArray;
typedef struct adbus_IterVariant        adbus_IterVariant;
typedef struct adbus_Match              adbus_Match;
typedef struct adbus_Member             adbus_Member;
typedef struct adbus_Message            adbus_Message;
typedef struct adbus_MsgFactory         adbus_MsgFactory;
typedef struct adbus_Proxy              adbus_Proxy;
typedef struct adbus_Reply              adbus_Reply;
typedef struct adbus_Remote             adbus_Remote;
typedef struct adbus_Server             adbus_Server;
typedef struct adbus_Signal             adbus_Signal;
typedef struct adbus_State              adbus_State;
typedef enum adbus_BlockType            adbus_BlockType;
typedef enum adbus_BusType              adbus_BusType;
typedef enum adbus_FieldType            adbus_FieldType;
typedef enum adbus_MessageType          adbus_MessageType;
typedef uint32_t                        adbus_Bool;

#ifdef _WIN32
    typedef uintptr_t   adbus_Socket;
#   define ADBUS_SOCK_INVALID ~((adbus_Socket) 0)
#else
    typedef int         adbus_Socket;
#   define ADBUS_SOCK_INVALID -1
#endif

typedef int             (*adbus_RecvCallback)(void*, char*, size_t);
typedef int             (*adbus_SendCallback)(void*, const char*, size_t);
typedef uint8_t         (*adbus_RandCallback)(void*);
typedef int             (*adbus_MsgCallback)(adbus_CbData* d);
typedef void            (*adbus_Callback)(void*);
typedef void            (*adbus_ProxyCallback)(void*,adbus_Callback cb,adbus_Callback release,void*);
typedef int             (*adbus_ProxyMsgCallback)(void*,adbus_MsgCallback,adbus_CbData*);

#include "adbus-iterator.h"


typedef void (*adbus_LogCallback)(const char*, size_t);
ADBUS_API void adbus_set_logger(adbus_LogCallback cb);





struct adbus_CbData
{
    adbus_Connection*       connection;

    adbus_Message*          msg;
    adbus_MsgFactory*       ret;
    adbus_Bool              noreturn;

    adbus_Iterator          setprop;
    adbus_Buffer*           getprop;

    void*                   user1;
    void*                   user2;

    /** \privatesection 
     *  Used by the adbus_check* functions. Setup by adbus_dispatch.
     */
    adbus_Iterator          checkiter;
    jmp_buf                 jmpbuf;
};

ADBUS_API int adbus_dispatch(adbus_MsgCallback callback, adbus_CbData* d);


struct adbus_Argument
{
    const char* value;
    int         size;
};

ADBUS_API void adbus_arg_init(adbus_Argument* args, size_t num);


struct adbus_Message
{
    const char*             data;
    size_t                  size;

    const char*             argdata;
    size_t                  argsize;

    adbus_MessageType       type;
    int                     flags;
    uint32_t                serial;

    const char*             signature;
    size_t                  signatureSize;

    const uint32_t*         replySerial;
    const char*             path;
    size_t                  pathSize;
    const char*             interface;
    size_t                  interfaceSize;
    const char*             member;
    size_t                  memberSize;
    const char*             error;
    size_t                  errorSize;
    const char*             destination;
    size_t                  destinationSize;
    const char*             sender;
    size_t                  senderSize;

    adbus_Argument*         arguments;
    size_t                  argumentsSize;
};

ADBUS_API int adbus_parse(adbus_Message* m, char* data, size_t size);
ADBUS_API int adbus_parseargs(adbus_Message* m);
ADBUS_API void adbus_freeargs(adbus_Message* m);
ADBUS_API size_t adbus_parse_size(const char* data, size_t size);
ADBUS_API void adbus_clonedata(adbus_Message* from, adbus_Message* to);
ADBUS_API void adbus_freedata(adbus_Message* m);


ADBUS_API int adbus_connect_address(
        adbus_BusType   type,
        char*           buf,
        size_t          sz);

ADBUS_API int adbus_bind_address(
        adbus_BusType   type,
        char*           buf,
        size_t          sz);

ADBUS_API adbus_Socket adbus_sock_connect(
        adbus_BusType   type);

ADBUS_API adbus_Socket adbus_sock_connect_s(
        const char*     envstr,
        int             size);

ADBUS_API adbus_Socket adbus_sock_bind(
        adbus_BusType   type);

ADBUS_API adbus_Socket adbus_sock_bind_s(
        const char*     envstr,
        int             size);

ADBUS_API adbus_Connection* adbus_sock_busconnect(
        adbus_BusType   type,
        adbus_Socket*   psock);

ADBUS_API adbus_Connection* adbus_sock_busconnect_s(
        const char*     envstr,
        int             size,
        adbus_Socket*   psock);





ADBUS_API adbus_Auth* adbus_sauth_new(
        adbus_SendCallback  send,
        adbus_RandCallback  rand,
        void*               data);

ADBUS_API adbus_Auth* adbus_cauth_new(
        adbus_SendCallback  send,
        adbus_RandCallback  rand,
        void*               data);

typedef adbus_Bool (*adbus_ExternalCallback)(void* data, const char* id);

ADBUS_API void adbus_sauth_external(
        adbus_Auth*             a,
        adbus_ExternalCallback  cb);

ADBUS_API void adbus_sauth_setuuid(adbus_Auth* a, const char* uuid);

ADBUS_API void adbus_cauth_external(adbus_Auth* a);
ADBUS_API int adbus_cauth_start(adbus_Auth* a);

ADBUS_API void adbus_auth_free(adbus_Auth* a);

/* Returns data consumed or -1 on error */
ADBUS_API int adbus_auth_parse(adbus_Auth* a, const char* data, size_t sz, adbus_Bool* finished);






ADBUS_API int adbus_error(
        adbus_CbData*           details,
        const char*             error,
        int                     errorSize,
        const char*             message,
        int                     messageSize);

ADBUS_API int adbus_errorf(
        adbus_CbData*           details,
        const char*             error,
        const char*             format,
        ...);

ADBUS_API int adbus_errorf_jmp(
        adbus_CbData*           details,
        const char*             error,
        const char*             format,
        ...);

ADBUS_API int adbus_error_argument(
        adbus_CbData*           details);






typedef int         (*adbus_SendMsgCallback)(void*, adbus_Message*);
typedef void        (*adbus_GetProxyCallback)(void*, adbus_ProxyCallback*, adbus_ProxyMsgCallback*, void**);
typedef adbus_Bool  (*adbus_ShouldProxyCallback)(void*);
typedef int         (*adbus_BlockCallback)(void*, adbus_BlockType type, void** handle, int timeoutms);

struct adbus_ConnectionCallbacks
{
    adbus_Callback                release;
    adbus_SendMsgCallback         send_message;
    adbus_RecvCallback            recv_data;
    adbus_ProxyCallback           proxy;
    adbus_ShouldProxyCallback     should_proxy;
    adbus_GetProxyCallback        get_proxy;
    adbus_BlockCallback           block;
};

ADBUS_API adbus_Connection* adbus_conn_new(adbus_ConnectionCallbacks* cb, void* user);
ADBUS_API adbus_Connection* adbus_conn_get(adbus_BusType type);
ADBUS_API void adbus_conn_set(adbus_BusType type, adbus_Connection* c);

ADBUS_API void adbus_conn_ref(adbus_Connection* connection);
ADBUS_API void adbus_conn_deref(adbus_Connection* connection);

#define adbus_conn_free(c) adbus_conn_deref(c)


ADBUS_API void adbus_conn_setsender(
        adbus_Connection*       connection,
        adbus_SendMsgCallback   callback,
        void*                   data);

ADBUS_API int adbus_conn_send(
        adbus_Connection*       connection,
        adbus_Message*          message);

ADBUS_API adbus_Bool adbus_conn_shouldproxy(
        adbus_Connection*   connection);

ADBUS_API void adbus_conn_proxy(
        adbus_Connection*   connection,
        adbus_Callback      callback,
        adbus_Callback      release,
        void*               user);

ADBUS_API void adbus_conn_getproxy(
        adbus_Connection*       connection,
        adbus_ProxyCallback*    cb,
        adbus_ProxyMsgCallback* msgcb,
        void**                  user);

ADBUS_API int adbus_conn_block(
        adbus_Connection*       connection,
        adbus_BlockType         type,
        void**                  handle,
        int                     timeoutms);

ADBUS_API uint32_t adbus_conn_serial(
        adbus_Connection*       connection);

ADBUS_API int adbus_conn_dispatch(
        adbus_Connection*       connection,
        adbus_Message*          message);

/* returns 0 on continue, 1 on finish, -1 on error */
ADBUS_API int adbus_conn_continue(
        adbus_Connection*       connection);

ADBUS_API int adbus_conn_parsecb(
        adbus_Connection*       connection);

ADBUS_API int adbus_conn_parse(
        adbus_Connection*       connection,
        const char*             data,
        size_t                  size);

ADBUS_API void adbus_conn_connect(
        adbus_Connection*       connection,
        adbus_Callback          callback,
        void*                   data);

ADBUS_API adbus_Bool adbus_conn_isconnected(
        const adbus_Connection* connection);

ADBUS_API const char* adbus_conn_uniquename(
        const adbus_Connection* connection,
        size_t*                 size);


struct adbus_Match
{
    /* The type is checked if it is not ADBUS_MSG_INVALID (0) */
    adbus_MessageType       type;

    /* Matches for signals should be added to the bus, but method returns
     * are automatically routed to us by the daemon
     */
    adbus_Bool              addMatchToBusDaemon;

    int64_t                 replySerial;

    /* The strings will all be copied out */
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
    const char*             error;
    int                     errorSize;

    adbus_Argument*         arguments;
    size_t                  argumentsSize;

    adbus_MsgCallback       callback;
    void*                   cuser;

    adbus_ProxyMsgCallback  proxy;
    void*                   puser;

    adbus_Callback          release[2];
    void*                   ruser[2];

    adbus_ProxyCallback     relproxy;
    void*                   relpuser;
};

ADBUS_API void adbus_match_init(adbus_Match* match);

struct adbus_Reply
{
    int64_t                 serial;

    const char*             remote;
    int                     remoteSize;

    adbus_MsgCallback       callback;
    void*                   cuser;

    adbus_MsgCallback       error;
    void*                   euser;

    adbus_ProxyMsgCallback  proxy;
    void*                   puser;

    adbus_Callback          release[2];
    void*                   ruser[2];

    adbus_ProxyCallback     relproxy;
    void*                   relpuser;
};

ADBUS_API void adbus_reply_init(adbus_Reply* reply);

struct adbus_Bind
{
    const char*             path;
    int                     pathSize;

    adbus_Interface*        interface;
    void*                   cuser2;

    adbus_ProxyMsgCallback  proxy;
    void*                   puser;

    adbus_Callback          release[2];
    void*                   ruser[2];

    adbus_ProxyCallback     relproxy;
    void*                   relpuser;
};

ADBUS_API void adbus_bind_init(adbus_Bind* bind);


ADBUS_API adbus_ConnMatch* adbus_conn_addmatch(
        adbus_Connection*       connection,
        const adbus_Match*      match);

ADBUS_API void adbus_conn_removematch(
        adbus_Connection*       connection,
        adbus_ConnMatch*        match);

ADBUS_API adbus_ConnReply* adbus_conn_addreply(
        adbus_Connection*       connection,
        const adbus_Reply*      reply);

ADBUS_API void adbus_conn_removereply(
        adbus_Connection*       connection,
        adbus_ConnReply*        reply);

ADBUS_API adbus_ConnBind* adbus_conn_bind(
        adbus_Connection*   connection,
        const adbus_Bind*   bind);

ADBUS_API void adbus_conn_unbind(
        adbus_Connection*   connection,
        adbus_ConnBind*     bind);



ADBUS_API adbus_Interface* adbus_conn_interface(
        adbus_Connection*       connection,
        const char*             path,
        int                     pathSize,
        const char*             interface,
        int                     interfaceSize,
        adbus_ConnBind**        bind);

ADBUS_API adbus_Member* adbus_conn_method(
        adbus_Connection*       connection,
        const char*             path,
        int                     pathSize,
        const char*             method,
        int                     methodSize,
        adbus_ConnBind**        bind);










ADBUS_API adbus_Interface* adbus_iface_new(const char* name, int size);
ADBUS_API void adbus_iface_ref(adbus_Interface* interface);
ADBUS_API void adbus_iface_deref(adbus_Interface* interface);
#define adbus_iface_free(iface) adbus_iface_deref(iface)

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
        int                 sigsz);

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


ADBUS_API void adbus_mbr_annotate(
        adbus_Member*       member,
        const char*         name,
        int                 nameSize,
        const char*         value,
        int                 valueSize);

ADBUS_API void adbus_mbr_argsig(
        adbus_Member*       member,
        const char*         sig,
        int                 size);

ADBUS_API void adbus_mbr_retsig(
        adbus_Member*       member,
        const char*         sig,
        int                 size);

ADBUS_API void adbus_mbr_argname(
        adbus_Member*       member,
        const char*         name,
        int                 nameSize);


ADBUS_API void adbus_mbr_retname(
        adbus_Member*       member,
        const char*         name,
        int                 nameSize);

ADBUS_API void adbus_mbr_addrelease(
        adbus_Member*       member,
        adbus_Callback      release,
        void*               ruser);

/* Methods */
ADBUS_API void adbus_mbr_setmethod(
        adbus_Member*       member,
        adbus_MsgCallback   callback,
        void*               user1);

ADBUS_API int adbus_mbr_call(
        adbus_Member*       method,
        adbus_ConnBind*     bind,
        adbus_CbData*       data);


/* Properties */

ADBUS_API void adbus_mbr_setgetter(
        adbus_Member*       member,
        adbus_MsgCallback   callback,
        void*               user1);

ADBUS_API void adbus_mbr_setsetter(
        adbus_Member*       member,
        adbus_MsgCallback   callback,
        void*               user1);





#ifdef DOC
ADBUS_INLINE int adbus_iter_align(adbus_Iterator* i, int alignment);
ADBUS_INLINE int adbus_iter_alignfield(adbus_Iterator* i, char field);
ADBUS_INLINE int adbus_iter_bool(adbus_Iterator* i, const adbus_Bool** v);
ADBUS_INLINE int adbus_iter_u8(adbus_Iterator* i, const uint8_t** v);
ADBUS_INLINE int adbus_iter_i16(adbus_Iterator* i, const int16_t** v);
ADBUS_INLINE int adbus_iter_u16(adbus_Iterator* i, const uint16_t** v);
ADBUS_INLINE int adbus_iter_i32(adbus_Iterator* i, const int32_t** v);
ADBUS_INLINE int adbus_iter_u32(adbus_Iterator* i, const uint32_t** v);
ADBUS_INLINE int adbus_iter_i64(adbus_Iterator* i, const int64_t** v);
ADBUS_INLINE int adbus_iter_u64(adbus_Iterator* i, const uint64_t** v);
ADBUS_INLINE int adbus_iter_double(adbus_Iterator* i, const double** v);
ADBUS_INLINE int adbus_iter_string(adbus_Iterator* i, const char** pstr, size_t* pstrsz);
ADBUS_INLINE int adbus_iter_objectpath(adbus_Iterator* i, const char** str, size_t* strsz);
ADBUS_INLINE int adbus_iter_signature(adbus_Iterator* i, const char** pstr, size_t* pstrsz);
ADBUS_INLINE int adbus_iter_beginarray(adbus_Iterator* i, adbus_IterArray* a);
ADBUS_INLINE adbus_Bool adbus_iter_inarray(adbus_Iterator* i, adbus_IterArray* a);
ADBUS_INLINE int adbus_iter_endarray(adbus_Iterator* i, adbus_IterArray* a);
ADBUS_INLINE int adbus_iter_begindictentry(adbus_Iterator* i);
ADBUS_INLINE int adbus_iter_enddictentry(adbus_Iterator* i);
ADBUS_INLINE int adbus_iter_beginstruct(adbus_Iterator* i);
ADBUS_INLINE int adbus_iter_endstruct(adbus_Iterator* i);
ADBUS_INLINE int adbus_iter_beginvariant(adbus_Iterator* i, adbus_IterVariant* v);;
ADBUS_INLINE int adbus_iter_endvariant(adbus_Iterator* i, adbus_IterVariant* v);;
#endif

ADBUS_API int adbus_iter_value(adbus_Iterator* i);
ADBUS_API void adbus_iter_buffer(adbus_Iterator* i, const adbus_Buffer* buf);
ADBUS_API void adbus_iter_args(adbus_Iterator* i, const adbus_Message* msg);

/* Check functions in the style of the lua check functions (eg luaL_checkint).
 *
 * They will longjmp back to throw an invalid argument
 * error. Thus they should only be used to pull out the arguments at the very
 * beginning of a message callback.
 */

ADBUS_API void        adbus_check_end(adbus_CbData* d);
ADBUS_API adbus_Bool  adbus_check_bool(adbus_CbData* d);
ADBUS_API uint8_t     adbus_check_u8(adbus_CbData* d);
ADBUS_API int16_t     adbus_check_i16(adbus_CbData* d);
ADBUS_API uint16_t    adbus_check_u16(adbus_CbData* d);
ADBUS_API int32_t     adbus_check_i32(adbus_CbData* d);
ADBUS_API uint32_t    adbus_check_u32(adbus_CbData* d);
ADBUS_API int64_t     adbus_check_i64(adbus_CbData* d);
ADBUS_API uint64_t    adbus_check_u64(adbus_CbData* d);
ADBUS_API double      adbus_check_double(adbus_CbData* d);
ADBUS_API const char* adbus_check_string(adbus_CbData* d, size_t* size);
ADBUS_API const char* adbus_check_objectpath(adbus_CbData* d, size_t* size);
ADBUS_API const char* adbus_check_signature(adbus_CbData* d, size_t* size);
ADBUS_API void        adbus_check_beginarray(adbus_CbData* d, adbus_IterArray* a);
ADBUS_API adbus_Bool  adbus_check_inarray(adbus_CbData* d, adbus_IterArray* a);
ADBUS_API void        adbus_check_endarray(adbus_CbData* d, adbus_IterArray* a);
ADBUS_API void        adbus_check_beginstruct(adbus_CbData* d);
ADBUS_API void        adbus_check_endstruct(adbus_CbData* d);
ADBUS_API void        adbus_check_begindictentry(adbus_CbData* d);
ADBUS_API void        adbus_check_enddictentry(adbus_CbData* d);
ADBUS_API const char* adbus_check_beginvariant(adbus_CbData* d, adbus_IterVariant* v);
ADBUS_API void        adbus_check_endvariant(adbus_CbData* d, adbus_IterVariant* v);
ADBUS_API void        adbus_check_value(adbus_CbData* d);


struct adbus_BufArray
{
    size_t      szindex;
    size_t      dataindex;
    const char* sigbegin;
    const char* sigend;
};

struct adbus_BufVariant
{
    const char* oldsig;
};


ADBUS_API adbus_Buffer* adbus_buf_new(void);
ADBUS_API void adbus_buf_free(adbus_Buffer* b);
ADBUS_API size_t adbus_buf_size(const adbus_Buffer* b);
ADBUS_API char* adbus_buf_data(const adbus_Buffer* b);
ADBUS_API size_t adbus_buf_reserved(adbus_Buffer* b);
ADBUS_API char* adbus_buf_release(adbus_Buffer* b);
ADBUS_API void adbus_buf_reset(adbus_Buffer* b);
ADBUS_API void adbus_buf_remove(adbus_Buffer* b, size_t off, size_t num);
ADBUS_API const char* adbus_buf_line(adbus_Buffer* b, size_t* sz);
ADBUS_API char* adbus_buf_recvbuf(adbus_Buffer* b, size_t len);
ADBUS_API void adbus_buf_recvd(adbus_Buffer* b, size_t len, int recvd);

ADBUS_API void adbus_buf_append(adbus_Buffer* b, const char* data, size_t sz);
ADBUS_API void adbus_buf_align(adbus_Buffer* b, int alignment);
ADBUS_API void adbus_buf_alignfield(adbus_Buffer* b, char field);
ADBUS_API const char* adbus_buf_sig(const adbus_Buffer* b, size_t* sz);
ADBUS_API const char* adbus_buf_signext(const adbus_Buffer* b, size_t* sz);
ADBUS_API void adbus_buf_setsig(adbus_Buffer* b, const char* sig, int size);
ADBUS_API void adbus_buf_appendsig(adbus_Buffer* b, const char* sig, int size);
ADBUS_API int adbus_buf_appendvalue(adbus_Buffer* b, adbus_Iterator* iter);


ADBUS_API void adbus_buf_end(adbus_Buffer* b);
ADBUS_API void adbus_buf_bool(adbus_Buffer* b, adbus_Bool v);
ADBUS_API void adbus_buf_u8(adbus_Buffer* b, uint8_t v);
ADBUS_API void adbus_buf_i16(adbus_Buffer* b, int16_t v);
ADBUS_API void adbus_buf_u16(adbus_Buffer* b, uint16_t v);
ADBUS_API void adbus_buf_i32(adbus_Buffer* b, int32_t v);
ADBUS_API void adbus_buf_u32(adbus_Buffer* b, uint32_t v);
ADBUS_API void adbus_buf_i64(adbus_Buffer* b, int64_t v);
ADBUS_API void adbus_buf_u64(adbus_Buffer* b, uint64_t v);
ADBUS_API void adbus_buf_double(adbus_Buffer* b, double v);
ADBUS_API void adbus_buf_string_f(adbus_Buffer* b, const char* format, ...);
ADBUS_API void adbus_buf_string_vf(adbus_Buffer* b, const char* format, va_list ap);
ADBUS_API void adbus_buf_string(adbus_Buffer* b, const char* str, int size);
ADBUS_API void adbus_buf_objectpath(adbus_Buffer* b, const char* str, int size);
ADBUS_API void adbus_buf_signature(adbus_Buffer* b, const char* str, int size);
ADBUS_API void adbus_buf_beginarray(adbus_Buffer* b, adbus_BufArray* a);
ADBUS_API void adbus_buf_arrayentry(adbus_Buffer* b, adbus_BufArray* a);
ADBUS_API void adbus_buf_checkarrayentry(adbus_Buffer* b, adbus_BufArray* a);
ADBUS_API void adbus_buf_endarray(adbus_Buffer* b, adbus_BufArray* a);
ADBUS_API void adbus_buf_begindictentry(adbus_Buffer* b);
ADBUS_API void adbus_buf_enddictentry(adbus_Buffer* b);
ADBUS_API void adbus_buf_beginstruct(adbus_Buffer* b);
ADBUS_API void adbus_buf_endstruct(adbus_Buffer* b);
ADBUS_API void adbus_buf_beginvariant(adbus_Buffer* b, adbus_BufVariant* v, const char* sig, int sigsize);
ADBUS_API void adbus_buf_endvariant(adbus_Buffer* b, adbus_BufVariant* v);

ADBUS_API int adbus_flip_value(char** data, size_t* size, const char** sig);
ADBUS_API int adbus_flip_data(char* data, size_t size, const char* sig);













ADBUS_API adbus_MsgFactory* adbus_msg_new(void);
ADBUS_API void adbus_msg_free(adbus_MsgFactory* m);
ADBUS_API void adbus_msg_reset(adbus_MsgFactory* m);


ADBUS_API int adbus_msg_build(adbus_MsgFactory* m, adbus_Message* msg);
ADBUS_API void adbus_msg_iterator(const adbus_Message* m, adbus_Iterator* iterator);

ADBUS_API const char* adbus_msg_path(const adbus_MsgFactory* m, size_t* len);
ADBUS_API const char* adbus_msg_interface(const adbus_MsgFactory* m, size_t* len);
ADBUS_API const char* adbus_msg_sender(const adbus_MsgFactory* m, size_t* len);
ADBUS_API const char* adbus_msg_destination(const adbus_MsgFactory* m, size_t* len);
ADBUS_API const char* adbus_msg_member(const adbus_MsgFactory* m, size_t* len);
ADBUS_API const char* adbus_msg_error(const adbus_MsgFactory* m, size_t* len);

ADBUS_API adbus_MessageType adbus_msg_type(const adbus_MsgFactory* m);
ADBUS_API int     adbus_msg_flags(const adbus_MsgFactory* m);
ADBUS_API int64_t adbus_msg_serial(const adbus_MsgFactory* m);
ADBUS_API adbus_Bool adbus_msg_reply(const adbus_MsgFactory* m, uint32_t* serial);


ADBUS_API void adbus_msg_settype(adbus_MsgFactory* m, adbus_MessageType type);
ADBUS_API void adbus_msg_setserial(adbus_MsgFactory* m, uint32_t serial);
ADBUS_API void adbus_msg_setflags(adbus_MsgFactory* m, int flags);
ADBUS_API void adbus_msg_setreply(adbus_MsgFactory* m, uint32_t reply);

ADBUS_API void adbus_msg_setpath(adbus_MsgFactory* m, const char* str, int size);
ADBUS_API void adbus_msg_setinterface(adbus_MsgFactory* m, const char* str, int size);
ADBUS_API void adbus_msg_setmember(adbus_MsgFactory* m, const char* str, int size);
ADBUS_API void adbus_msg_seterror(adbus_MsgFactory* m, const char* str, int size);
ADBUS_API void adbus_msg_setdestination(adbus_MsgFactory* m, const char* str, int size);
ADBUS_API void adbus_msg_setsender(adbus_MsgFactory* m, const char* str, int size);

ADBUS_API int adbus_msg_send(adbus_MsgFactory* m, adbus_Connection* c);
ADBUS_API adbus_Buffer* adbus_msg_argbuffer(adbus_MsgFactory* m);

ADBUS_INLINE const adbus_Buffer* adbus_msg_argbuffer_c(const adbus_MsgFactory* m)
{ return adbus_msg_argbuffer((adbus_MsgFactory*) m); }

/** \relates adbus_MsgFactory
 * @{ */

/** Appends to the argument signature (see adbus_buf_appendsig()) */
#define adbus_msg_appendsig(m,t,s)  adbus_buf_appendsig(adbus_msg_argbuffer(m), t, s)

/** Sets the argument signature (see adbus_buf_setsig()) */
#define adbus_msg_setsig(m,t,s)     adbus_buf_setsig(adbus_msg_argbuffer(m), t, s)

/** Appends argument data (see adbus_buf_append()) */
#define adbus_msg_append(m,d,s)     adbus_buf_append(adbus_msg_argbuffer(m), d, s)

/** Finalises argument data (see adbus_buf_end()) */
#define adbus_msg_end(m)            adbus_buf_end(adbus_msg_argbuffer(m))

/** Appends a boolean to the argument data (see adbus_buf_bool()) */
#define adbus_msg_bool(m,v)         adbus_buf_bool(adbus_msg_argbuffer(m), v)

/** Appends a uint8_t to the argument data (see adbus_buf_u8()) */
#define adbus_msg_u8(m,v)           adbus_buf_u8(adbus_msg_argbuffer(m), v)

/** Appends a int16_t to the argument data (see adbus_buf_i16()) */
#define adbus_msg_i16(m,v)          adbus_buf_i16(adbus_msg_argbuffer(m), v)

/** Appends a uint16_t to the argument data (see adbus_buf_u16()) */
#define adbus_msg_u16(m,v)          adbus_buf_u16(adbus_msg_argbuffer(m), v)

/** Appends a int32_t to the argument data (see adbus_buf_i32()) */
#define adbus_msg_i32(m,v)          adbus_buf_i32(adbus_msg_argbuffer(m), v)

/** Appends a uint32_t to the argument data (see adbus_buf_u32()) */
#define adbus_msg_u32(m,v)          adbus_buf_u32(adbus_msg_argbuffer(m), v)

/** Appends a int64_t to the argument data (see adbus_buf_i64()) */
#define adbus_msg_i64(m,v)          adbus_buf_i64(adbus_msg_argbuffer(m), v)

/** Appends a uint64_t to the argument data (see adbus_buf_u64()) */
#define adbus_msg_u64(m,v)          adbus_buf_u64(adbus_msg_argbuffer(m), v)

/** Appends a double to the argument data (see adbus_buf_double()) */
#define adbus_msg_double(m,v)       adbus_buf_double(adbus_msg_argbuffer(m), v)

/** Appends a string to the argument data (see adbus_buf_string()) */
#define adbus_msg_string(m,v,s)     adbus_buf_string(adbus_msg_argbuffer(m), v, s)
#define adbus_msg_string_vf(m,v,s)  adbus_buf_string(adbus_msg_argbuffer(m), v, s)

ADBUS_API void adbus_msg_string_f(adbus_MsgFactory* m, const char* format, ...);

/** Appends an object path to the argument data (see adbus_buf_objectpath()) */
#define adbus_msg_objectpath(m,v,s) adbus_buf_objectpath(adbus_msg_argbuffer(m), v, s)

/** Begins an array scope in the argument data (see adbus_buf_beginarray()) */
#define adbus_msg_beginarray(m,a)   adbus_buf_beginarray(adbus_msg_argbuffer(m), a)

/** Begins an array entry in the argument data (see adbus_buf_arrayentry()) */
#define adbus_msg_arrayentry(m,a)   adbus_buf_arrayentry(adbus_msg_argbuffer(m), a)

/** Ends an array scope in the argument data (see adbus_buf_endarray()) */
#define adbus_msg_endarray(m,a)     adbus_buf_endarray(adbus_msg_argbuffer(m), a)

/** Begins a struct array scope in the argument data (see adbus_buf_beginstruct()) */
#define adbus_msg_beginstruct(m)    adbus_buf_beginstruct(adbus_msg_argbuffer(m))

/** Ends a struct array scope in the argument data (see adbus_buf_endstruct()) */
#define adbus_msg_endstruct(m)      adbus_buf_endstruct(adbus_msg_argbuffer(m))

/** Begins a dict entry scope in the argument data (see adbus_buf_begindictentry()) */
#define adbus_msg_begindictentry(m) adbus_buf_begindictentry(adbus_msg_argbuffer(m))

/** Ends a dict entry scope in the argument data (see adbus_buf_enddictentry()) */
#define adbus_msg_enddictentry(m)   adbus_buf_enddictentry(adbus_msg_argbuffer(m))

/** Begins a variant scope in the argument data (see adbus_buf_beginvariant()) */
#define adbus_msg_beginvariant(m,v,t,s) adbus_buf_beginvariant(adbus_msg_argbuffer(m), v, t, s)

/** Ends a variant scope in the argument data (see adbus_buf_endvariant()) */
#define adbus_msg_endvariant(m,v)       adbus_buf_endvariant(adbus_msg_argbuffer(m), v)

/** @} */











ADBUS_API adbus_State* adbus_state_new(void);
ADBUS_API void adbus_state_free(adbus_State* state);
ADBUS_API void adbus_state_reset(adbus_State* state);

ADBUS_API void adbus_state_bind(
        adbus_State*        state,
        adbus_Connection*   connection,
        const adbus_Bind*   bind);

ADBUS_API void adbus_state_addmatch(
        adbus_State*        state,
        adbus_Connection*   connection,
        const adbus_Match*  match);

ADBUS_API void adbus_state_addreply(
        adbus_State*        state,
        adbus_Connection*   connection,
        const adbus_Reply*  reply);




struct adbus_Call
{
    adbus_Proxy*            proxy;
    adbus_MsgFactory*       msg;
    int                     timeoutms;

    adbus_MsgCallback       callback;
    void*                   cuser;

    adbus_MsgCallback       error;
    void*                   euser;

    adbus_Callback          release[2];
    void*                   ruser[2];
};

ADBUS_API adbus_Proxy* adbus_proxy_new(
        adbus_State*        state);

ADBUS_API void adbus_proxy_init(
        adbus_Proxy*        proxy,
        adbus_Connection*   connection,
        const char*         service,
        int                 ssize,
        const char*         path,
        int                 psize);

ADBUS_API void adbus_proxy_free(
        adbus_Proxy*        proxy);

ADBUS_API void adbus_proxy_setinterface(
        adbus_Proxy*        proxy,
        const char*         interface,
        int                 isize);

ADBUS_API void adbus_proxy_signal(
        adbus_Proxy*        proxy,
        adbus_Match*        match,
        const char*         signal,
        int                 size);

ADBUS_API void adbus_proxy_method(
        adbus_Proxy*        proxy,
        adbus_Call*         call,
        const char*         method,
        int                 size);

ADBUS_API void adbus_proxy_setproperty(
        adbus_Proxy*        proxy,
        adbus_Call*         call,
        const char*         property,
        int                 propsize,
        const char*         type,
        int                 typesize);

ADBUS_API void adbus_proxy_getproperty(
        adbus_Proxy*        proxy,
        adbus_Call*         call,
        const char*         property,
        int                 propsize);

ADBUS_API int adbus_call_send(
        adbus_Call*         call);

ADBUS_API int adbus_call_block(
        adbus_Call*         call);






ADBUS_API adbus_Proxy* adbus_busproxy_new(
        adbus_State*        s,
        adbus_Connection*   c);

ADBUS_API void adbus_busproxy_requestname(
        adbus_Proxy*        p,
        adbus_Call*         c,
        const char*         name,
        int                 size,
        int                 flags);

ADBUS_API void adbus_busproxy_releasename(
        adbus_Proxy*        p,
        adbus_Call*         c,
        const char*         name,
        int                 size);










ADBUS_API adbus_Signal* adbus_sig_new(
        adbus_Member*       mbr);

ADBUS_API void adbus_sig_free(
        adbus_Signal*       signal);

ADBUS_API void adbus_sig_reset(
        adbus_Signal*       signal);

ADBUS_API void adbus_sig_bind(
        adbus_Signal*       signal,
        adbus_Connection*   connection,
        const char*         path,
        int                 pathSize);

ADBUS_API adbus_MsgFactory* adbus_sig_msg(
        adbus_Signal*       signal);

ADBUS_API void adbus_sig_emit(
        adbus_Signal*       signal);




ADBUS_API adbus_Server* adbus_serv_new(adbus_Interface* bus);
ADBUS_API void adbus_serv_free(adbus_Server* s);

ADBUS_API adbus_Remote* adbus_serv_connect(
        adbus_Server*           s,
        adbus_SendMsgCallback   send,
        void*                   data);

ADBUS_API void adbus_remote_disconnect(adbus_Remote* r);
ADBUS_API int adbus_remote_dispatch(adbus_Remote* r, adbus_Message* m);
ADBUS_API int adbus_remote_parse(adbus_Remote* r, adbus_Buffer* buf);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ADBUS_H */

