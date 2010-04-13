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

#ifndef __STDC_LIMIT_MACROS
#   define __STDC_LIMIT_MACROS
#endif

#include <stdlib.h>
#include <stdint.h>

#ifdef _WIN32
# include <windows.h>
#else
# include <pthread.h>
#endif

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4311)
# pragma warning(disable:4312)
#endif

#if defined __cplusplus || __STDC_VERSION__ + 0 >= 199901L
#   define MT_INLINE static inline
#else
#   define MT_INLINE static
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined MT_SHARED_LIBRARY
#   ifdef _MSC_VER
#       if defined MT_LIBRARY
#           define MT_API extern __declspec(dllexport)
#       else
#           define MT_API extern __declspec(dllimport)
#       endif
#   elif defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && defined(__ELF__)
#       define MT_API extern __attribute__((visibility("default")))
#   else
#       define MT_API extern
#   endif
#else
#   define MT_API extern
#endif

typedef struct MT_MainLoop          MT_MainLoop;
typedef struct MT_LoopRegistration  MT_LoopRegistration;
typedef struct MT_Lock              MT_Lock;
typedef struct MT_Message           MT_Message;
typedef struct MT_Freelist          MT_Freelist;
typedef struct MT_Header            MT_Header;
typedef struct MT_ThreadStorage     MT_ThreadStorage;
typedef struct MT_Target            MT_Target;
typedef struct MT_Queue             MT_Queue;
typedef struct MT_QueueItem         MT_QueueItem;
typedef struct MT_Signal            MT_Signal;
typedef struct MT_Subscription      MT_Subscription;

/* MT_Time gives the time in microseconds centered on the unix epoch (midnight
 * Jan 1 1970) */
typedef int64_t MT_Time;

typedef long volatile MT_AtomicInt;
typedef MT_AtomicInt MT_Spinlock;

typedef void        (*MT_Callback)(void*);
typedef void        (*MT_MessageCallback)(MT_Message*);
typedef MT_Header*  (*MT_CreateCallback)(void);
typedef void        (*MT_FreeCallback)(MT_Header*);

#ifdef _WIN32
    typedef void* MT_Handle; /* HANDLE */
    typedef uintptr_t MT_Socket; /* SOCKET */
    typedef CRITICAL_SECTION MT_Mutex;
    typedef MT_Handle MT_Thread;
    typedef uint32_t MT_ThreadStorageKey;
#else
    /* Handles on unix are file descriptors where we only care about read
     * being signalled. File descriptors that aren't actually files, pipes,
     * sockets etc normally fall in this category. This includes epoll
     * handles, thread wake up pipes (pipes that are just used to kick us out
     * out of poll), bound sockets, etc.
     */
    typedef int MT_Handle;   /* fd_t */
    typedef int MT_Socket;   /* fd_t */
    typedef pthread_mutex_t MT_Mutex;
    typedef pthread_t MT_Thread;
    typedef pthread_key_t MT_ThreadStorageKey;
#endif

/* Pad out the pointers to prevent false cache sharing */

struct MT_QueueItem
{
    MT_QueueItem* volatile  next;
    char                    nextPad[16 - sizeof(MT_QueueItem*)];
};

struct MT_Queue
{
    MT_QueueItem* volatile  first;
    char                    firstPad[16 - sizeof(MT_QueueItem*)];

    MT_QueueItem* volatile  last;
    char                    lastPad[16 - sizeof(MT_QueueItem*)];
};

/* The message given in the callbacks may be a copy of the original message.
 */
struct MT_Message
{
    MT_MessageCallback      call;
    MT_MessageCallback      free;
    MT_Target*              target;
    void*                   user;

    /* internal */
    MT_QueueItem            titem;
    MT_QueueItem            qitem;
};

struct MT_Target
{
    MT_MainLoop*            loop;

    /* internal */
    MT_Queue                queue;
    MT_AtomicInt            lock;
    MT_Subscription*        subscriptions;
};

struct MT_Signal
{
    MT_AtomicInt            lock;
    int                     count;
    MT_Subscription*        subscriptions;
};


/* Needs to be memset to 0 if not a global */
struct MT_ThreadStorage
{
    MT_Spinlock             lock;
    int                     ref;
    MT_ThreadStorageKey     tls;
};

struct MT_Header
{
    MT_Header* volatile     next;
};


void MT_Queue_Init(MT_Queue* s);

/* This should only be called from a single consume thread */
MT_QueueItem* MT_Queue_Consume(MT_Queue* s);

/* This can be called from any producer threads as long as you syncronise
 * destroying the queue.
 */
void MT_Queue_Produce(MT_Queue* s, MT_QueueItem* newval);


MT_API void MT_Freelist_Ref(MT_Freelist** s, MT_CreateCallback create, MT_FreeCallback free);
MT_API void MT_Freelist_Deref(MT_Freelist** s);

MT_API MT_Header* MT_Freelist_Pop(MT_Freelist* s);
MT_API void MT_Freelist_Push(MT_Freelist* s, MT_Header* h);


MT_API void MT_Signal_Init(MT_Signal* s);
MT_API void MT_Signal_Destroy(MT_Signal* s);
MT_API void MT_Signal_Emit(MT_Signal* s, MT_Message* m);

MT_API void MT_Connect(MT_Signal* s, MT_Target* t);

MT_API MT_MainLoop* MT_Loop_New(void);
MT_API void MT_Loop_Free(MT_MainLoop* s);

MT_API MT_LoopRegistration* MT_Loop_AddClientSocket(MT_MainLoop* s, MT_Socket fd, MT_Callback read, MT_Callback write, MT_Callback close, void* user);
MT_API MT_LoopRegistration* MT_Loop_AddServerSocket(MT_MainLoop* s, MT_Socket fd, MT_Callback accept, void* user);
MT_API MT_LoopRegistration* MT_Loop_AddHandle(MT_MainLoop* s, MT_Handle h, MT_Callback cb, void* user);
MT_API MT_LoopRegistration* MT_Loop_AddIdle(MT_MainLoop* s, MT_Callback cb, void* user);
MT_API MT_LoopRegistration* MT_Loop_AddTick(MT_MainLoop* s, MT_Time period, MT_Callback cb, void* user);

#define MT_LOOP_HANDLE  0x01
#define MT_LOOP_READ    0x02
#define MT_LOOP_WRITE   0x04
#define MT_LOOP_CLOSE   0x08
#define MT_LOOP_ACCEPT  0x10
#define MT_LOOP_IDLE    0x20
#define MT_LOOP_TICK    0x40

MT_API void MT_Loop_Enable(MT_MainLoop* s, MT_LoopRegistration* r, int flags);
MT_API void MT_Loop_Disable(MT_MainLoop* s, MT_LoopRegistration* r, int flags);

MT_API void MT_Loop_Remove(MT_MainLoop* s, MT_LoopRegistration* r);
MT_API void MT_Loop_Post(MT_MainLoop* s, MT_Message* m);

MT_API MT_MainLoop* MT_Current(void);
MT_API void MT_SetCurrent(MT_MainLoop* s);
MT_API int  MT_Current_Step(void);
MT_API int  MT_Current_Run(void);
MT_API void MT_Current_Exit(int code);

#define MT_Current_AddClientSocket(fd, read, write, close, user)    \
    MT_Loop_AddClientSocket(MT_Current(), fd, read, write, close, user)

#define MT_Current_AddServerSocket(fd, accept, user)    \
    MT_Loop_AddServerSocket(MT_Current(), fd, accept, user)

#define MT_Current_AddHandle(h, cb, user)    \
    MT_Loop_AddHandle(MT_Current(), h, cb, user)

#define MT_Current_AddIdle(cb, user) \
    MT_Loop_AddIdle(MT_Current(), cb, user)

#define MT_Current_AddTick(period, cb, user) \
    MT_Loop_AddTick(MT_Current(), period, cb, user)

#define MT_Current_Enable(reg, flags) \
    MT_Loop_Enable(MT_Current(), reg, flags)

#define MT_Current_Disable(reg, flags) \
    MT_Loop_Disable(MT_Current(), reg, flags)

#define MT_Current_Remove(reg) \
    MT_Loop_Remove(MT_Current(), reg)



#if defined _WIN32
    MT_INLINE void MT_Mutex_Init(MT_Mutex* lock)
    { InitializeCriticalSection(lock); }

    MT_INLINE void MT_Mutex_Destroy(MT_Mutex* lock)
    { DeleteCriticalSection(lock); }

    MT_INLINE void MT_Mutex_Enter(MT_Mutex* lock)
    { EnterCriticalSection(lock); }

    MT_INLINE void MT_Mutex_Exit(MT_Mutex* lock)
    { LeaveCriticalSection(lock); }


    /* Returns previous value */
#   define MT_AtomicPtr_Set(pval, new_val) \
        InterlockedExchangePointer(pval, new_val)

    /* Returns previous value */
#   define MT_AtomicPtr_SetFrom(pval, from, to) \
        InterlockedCompareExchangePointer(pval, to, from)


    /* Returns previous value */
    MT_INLINE int MT_AtomicInt_Set(MT_AtomicInt* a, int val)
    { return InterlockedExchange(a, val); }

    /* Returns previous value */
    MT_INLINE int MT_AtomicInt_SetFrom(MT_AtomicInt* a, int from, int to)
    { return InterlockedCompareExchange(a, to, from); }

    /* Returns the new value */
    MT_INLINE int MT_AtomicInt_Increment(MT_AtomicInt* a)
    { return InterlockedIncrement(a); }

    /* Returns the new value */
    MT_INLINE int MT_AtomicInt_Decrement(MT_AtomicInt* a)
    { return InterlockedDecrement(a); }

#elif defined __GNUC__
    MT_INLINE void MT_Mutex_Init(MT_Mutex* lock)
    { pthread_mutex_init(lock, NULL); }

    MT_INLINE void MT_Mutex_Destroy(MT_Mutex* lock)
    { pthread_mutex_destroy(lock); }

    MT_INLINE void MT_Mutex_Enter(MT_Mutex* lock)
    { pthread_mutex_lock(lock); }

    MT_INLINE void MT_Mutex_Exit(MT_Mutex* lock)
    { pthread_mutex_unlock(lock); }



    /* Returns previous value */
#ifdef __llvm__
#   define MT_AtomicPtr_Set(pval, new_val) ((void*) __sync_lock_test_and_set(pval, new_val))
#else
#   define MT_AtomicPtr_Set(pval, new_val) __sync_lock_test_and_set(pval, new_val)
#endif

    /* Returns previous value */
#ifdef __llvm__
#   define MT_AtomicPtr_SetFrom(pval, from, to) ((void*) __sync_val_compare_and_swap(pval, from, to))
#else
#   define MT_AtomicPtr_SetFrom(pval, from, to)  __sync_val_compare_and_swap(pval, from, to)
#endif



    /* Returns previous value */
    MT_INLINE int MT_AtomicInt_Set(MT_AtomicInt* a, int val)
    { return __sync_lock_test_and_set(a, val); }

    /* Returns previous value */
    MT_INLINE int MT_AtomicInt_SetFrom(MT_AtomicInt* a, int from, int to)
    { return __sync_val_compare_and_swap(a, from, to); }

    /* Returns the new value */
    MT_INLINE int MT_AtomicInt_Increment(MT_AtomicInt* a)
    { return __sync_add_and_fetch(a, 1); }

    /* Returns the new value */
    MT_INLINE int MT_AtomicInt_Decrement(MT_AtomicInt* a)
    { return __sync_sub_and_fetch(a, 1); }

#else
#error
#endif

MT_INLINE void MT_Spinlock_Enter(MT_Spinlock* lock)
{ while (MT_AtomicInt_SetFrom(lock, 0, 1) != 0) {} }

MT_INLINE void MT_Spinlock_Exit(MT_Spinlock* lock)
{ MT_AtomicInt_Set(lock, 0); }






MT_API void MT_Target_Init(MT_Target* t);
MT_API void MT_Target_InitToLoop(MT_Target* t, MT_MainLoop* loop);
MT_API void MT_Target_Destroy(MT_Target* t);
MT_API void MT_Target_Post(MT_Target* t, MT_Message* m);

MT_API void MT_Thread_Start(MT_Callback func, void* arg);
MT_API MT_Thread MT_Thread_StartJoinable(MT_Callback func, void* arg);
MT_API void MT_Thread_Join(MT_Thread thread);


MT_API void MT_ThreadStorage_Ref(MT_ThreadStorage* s);
MT_API void MT_ThreadStorage_Deref(MT_ThreadStorage* s);

#ifdef _WIN32
    MT_INLINE void* MT_ThreadStorage_Get(MT_ThreadStorage* s)
    { return TlsGetValue(s->tls); }

    MT_INLINE void MT_ThreadStorage_Set(MT_ThreadStorage* s, void* val)
    { TlsSetValue(s->tls, val); }
#else
    MT_INLINE void* MT_ThreadStorage_Get(MT_ThreadStorage* s)
    { return pthread_getspecific(s->tls); }

    MT_INLINE void MT_ThreadStorage_Set(MT_ThreadStorage* s, void* val)
    { pthread_setspecific(s->tls, val); }
#endif




#define MT_TIME_INVALID           INT64_MAX
#define MT_TIME_ISVALID(x)        ((x) != MT_TIME_INVALID)

#define MT_TIME_FROM_US(x)        ((MT_Time) (x))
#define MT_TIME_FROM_MS(x)        ((MT_Time) ((double) (x) * 1000))
#define MT_TIME_FROM_SEC(x)       ((MT_Time) ((double) (x) * 1000000))
#define MT_TIME_FROM_HOURS(x)     ((MT_Time) ((double) (x) * 1000000 * 3600))
#define MT_TIME_FROM_DAYS(x)      ((MT_Time) ((double) (x) * 1000000 * 3600 * 24))
#define MT_TIME_FROM_WEEKS(x)     ((MT_Time) ((double) (x) * 1000000 * 3600 * 24 * 7))
#define MT_TIME_FROM_HZ(x)        ((MT_Time) ((1.0 / (double) (x)) * 1000000))

#define MT_TIME_TO_US(x)          (x)
#define MT_TIME_TO_MS(x)          ((double) (x) / 1000)
#define MT_TIME_TO_SEC(x)         ((double) (x) / 1000000)
#define MT_TIME_TO_HOURS(x)       ((double) (x) / 1000000 / 3600)
#define MT_TIME_TO_DAYS(x)        ((double) (x) / 1000000 / 3600 / 24 / 7)
#define MT_TIME_TO_WEEKS(x)       ((double) (x) / 1000000 / 3600 / 24 / 7)

#define MT_TIME_GPS_EPOCH         MT_TIME_FROM_SEC(315964800)


/* Functions to convert to/from broken down time. We use struct tm here
 * instead of SYSTEMTIME for portability. Broken-down time is stored in the
 * structure tm which is defined in <time.h> as follows:
 */

#if 0
struct tm 
{
    /* The number of seconds after the minute, normally in the range 0 to 59,
     * but can be up to 60 to allow for leap seconds. 
     */
    int tm_sec;

    /* The number of minutes after the hour, in the range 0 to 59. */
    int tm_min;

    /* The number of hours past midnight, in the range 0 to 23. */
    int tm_hour;

    /* The day of the month, in the range 1 to 31. */
    int tm_mday;

    /* The number of months since January, in the range 0 to 11. */
    int tm_mon;

    /* The number of years since 1900. */
    int tm_year;

    /* The number of days since Sunday, in the range 0 to 6. */
    int tm_wday;

    /* The number of days since January 1, in the range 0 to 365. */
    int tm_yday;

    /* A flag that indicates whether daylight saving time is in effect at the
     * time described. The value is positive if daylight saving time is in
     * effect, zero if it is not, and negative if the information is not
     * available.
     */
    int tm_isdst;
};
#endif

MT_API MT_Time MT_FromBrokenDownTime(struct tm* tm); /* Returns MT_TIME_INVALID on error */
MT_API int MT_ToBrokenDownTime(MT_Time t, struct tm* tm); /* Returns non zero on error */

#ifdef _WIN32
    MT_API MT_Time MT_FromFileTime(FILETIME* ft);
    MT_API void    MT_ToFileTime(MT_Time, FILETIME* ft);
#endif

MT_API MT_Time MT_CurrentTime();

MT_API char* MT_NewDateString(MT_Time t);
MT_API char* MT_NewDateTimeString(MT_Time t);
MT_API void  MT_FreeDateString(char* str);

#ifdef __cplusplus
} /* extern "C" */
#endif




#ifdef __cplusplus
namespace MT
{

#   define MT_NOT_COPYABLE(c) private: c(c& __DummyArg); c& operator=(c& __DummyArg)

    class Message
    {
    public:
        virtual void Call() = 0;
        virtual ~Message() {}

        void Post(MT_MainLoop* loop)
        {
            Setup();
            MT_Loop_Post(loop, &m_Header);
        }

        void Post(MT_Target* target)
        {
            Setup();
            MT_Target_Post(target, &m_Header);
        }

    private:
        static void Callback(MT_Message* m)
        { ((Message*) m->user)->Call(); }

        static void Free(MT_Message* m)
        { delete (Message*) m->user; }

        void Setup()
        {
            m_Header.call = &Callback;
            m_Header.free = &Free;
            m_Header.user = this;
        }

        MT_Message m_Header;
    };


    class EventLoop
    {
        MT_NOT_COPYABLE(EventLoop);
    public:
        EventLoop() : m(MT_Loop_New()) {}
        ~EventLoop() {MT_Loop_Free(m);}

        void SetTick(MT_Time period, MT_Callback cb, void* user)
        { MT_Loop_SetTick(m, period, cb, user); }

        void Register(MT_Handle h, MT_Callback cb, void* user)
        { MT_Loop_Register(m, h, cb, user); }

        void Unregister(MT_Handle h)
        { MT_Loop_Unregister(m, h); }

        void SetCurrent()
        { MT_SetCurrent(m); }

        static int Run()
        { return MT_Current_Run(); }

        static void Exit(int code)
        { MT_Current_Exit(code); }

        operator MT_MainLoop*() {return m;}

    private:
        MT_MainLoop* m;
    };


    class Mutex
    {
    public:
        Mutex()       {MT_Mutex_Init(&m_Lock);}
        ~Mutex()      {MT_Mutex_Destroy(&m_Lock);}

        void Enter()  {MT_Mutex_Enter(&m_Lock);}
        void Exit()   {MT_Mutex_Exit(&m_Lock);}

    private:
        MT_Mutex m_Lock;
    };


    class Spinlock
    {
    public:
        Spinlock()    {m_Lock = 0;}
        void Enter()  {MT_Spinlock_Enter(&m_Lock);}
        void Exit()   {MT_Spinlock_Exit(&m_Lock);}

    private:
        MT_Spinlock m_Lock;
    };


    template <class Lock>
    class ScopedLock
    {
    public:
        ScopedLock(Lock* lock) 
        : m_Lock(lock)
        { m_Lock->Enter(); }

        ~ScopedLock()
        { m_Lock->Exit(); }

    private:
        Lock* m_Lock;
    };

    struct MT_DateString
    {
      MT_DateString(char* str) : m_Str(str) {}
      ~MT_DateString(){MT_FreeDateString(m_Str);}
      char* m_Str;
    };

    /* These return ISO 8601 strings eg "2010-02-16" and "2010-02-16 22:00:08.067890Z" */
#   define MT_LogDateString(t) MT_DateString(MT_NewDateString(t)).m_Str
#   define MT_LogDateTimeString(t) MT_DateString(MT_NewDateTimeString(t)).m_Str

}
#endif
