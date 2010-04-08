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

#include "Common.h"
#include <assert.h>

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

// This is a header only API

#ifdef _WIN32
    typedef CRITICAL_SECTION MT_Mutex;

#   define MT_Mutex_Init(lock)     InitializeCriticalSection(lock)
#   define MT_Mutex_Destroy(lock)  DeleteCriticalSection(lock)
#   define MT_Mutex_Enter(lock)    EnterCriticalSection(lock)
#   define MT_Mutex_Exit(lock)     LeaveCriticalSection(lock)

#else
    typedef pthread_mutex_t MT_Mutex;

#   define MT_Mutex_Init(lock)     pthread_mutex_init(lock)
#   define MT_Mutex_Destroy(lock)  pthread_mutex_destroy(lock)
#   define MT_Mutex_Enter(lock)    pthread_mutex_lock(lock)
#   define MT_Mutex_Exit(lock)     pthread_mutex_unlock(lock)

#endif

typedef void* volatile MT_AtomicPtr;
typedef long  volatile MT_AtomicInt;

#if defined _WIN32
    MT_INLINE void MT_AtomicPtr_Set(MT_AtomicPtr* a, void* val)
    { InterlockedExchangePointer(a, val); }

    MT_INLINE int MT_AtomicPtr_SetFrom(MT_AtomicPtr* a, void* from, void* to)
    { return InterlockedCompareExchangePointer(a, to, from) == from; }

    MT_INLINE void MT_AtomicInt_Set(MT_AtomicInt* a, int val)
    { InterlockedExchange(a, val); }

    MT_INLINE int MT_AtomicInt_SetFrom(MT_AtomicInt* a, int from, int to)
    { return InterlockedCompareExchange(a, to, from) == from; }

    /* Returns the new value */
    MT_INLINE int MT_AtomicInt_Increment(MT_AtomicInt* a)
    { return InterlockedIncrement(a); }

    /* Returns the new value */
    MT_INLINE int MT_AtomicInt_Decrement(MT_AtomicInt* a)
    { return InterlockedDecrement(a); }

#elif defined __GNUC__
    MT_INLINE void MT_AtomicPtr_Set(MT_AtomicPtr* a, void* val)
    { (void) __sync_lock_test_and_set(a, val); }

    MT_INLINE int MT_AtomicPtr_SetFrom(MT_AtomicPtr* a, void* from, void* to)
    { return __sync_bool_compare_and_swap(a, from, to); }

    MT_INLINE void MT_AtomicInt_Set(MT_AtomicInt* a, int val)
    { (void) __sync_lock_test_and_set(a, val); }

    MT_INLINE int MT_AtomicInt_SetFrom(MT_AtomicInt* a, int from, int to)
    { return __sync_bool_compare_and_swap(a, from, to); }

    /* Returns the new value */
    MT_INLINE int MT_AtomicInt_Increment(MT_AtomicInt* a)
    { return __sync_add_and_fetch(a, 1); }

    /* Returns the new value */
    MT_INLINE int MT_AtomicInt_Decrement(MT_AtomicInt* a)
    { return __sync_sub_and_fetch(a, 1); }

#else
#error
#endif


typedef MT_AtomicInt MT_Spinlock;

#define MT_SPINLOCK_STATIC_INIT 0

MT_INLINE void MT_Spinlock_Init(MT_Spinlock* lock)
{ *lock = 0; }

MT_INLINE void MT_Spinlock_Destroy(MT_Spinlock* lock)
{ assert(*lock == 0); }

MT_INLINE void MT_Spinlock_Enter(MT_Spinlock* lock)
{ while (!MT_AtomicInt_SetFrom(lock, 0, 1)) {} }

MT_INLINE void MT_Spinlock_Exit(MT_Spinlock* lock)
{ MT_AtomicInt_Set(lock, 0); }


#ifdef __cplusplus
namespace MT
{
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
    Spinlock()    {MT_Spinlock_Init(&m_Lock);}
    ~Spinlock()   {MT_Spinlock_Destroy(&m_Lock);}

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
}
#endif

#ifdef _MSC_VER
# pragma warning(pop)
#endif
