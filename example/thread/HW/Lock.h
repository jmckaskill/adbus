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
  typedef CRITICAL_SECTION HW_Mutex;

# define HW_Mutex_Init(lock)     InitializeCriticalSection(lock)
# define HW_Mutex_Destroy(lock)  DeleteCriticalSection(lock)
# define HW_Mutex_Enter(lock)    EnterCriticalSection(lock)
# define HW_Mutex_Exit(lock)     LeaveCriticalSection(lock)

#else
  typedef pthread_mutex_t HW_Mutex;

# define HW_Mutex_Init(lock)     pthread_mutex_init(lock)
# define HW_Mutex_Destroy(lock)  pthread_mutex_destroy(lock)
# define HW_Mutex_Enter(lock)    pthread_mutex_lock(lock)
# define HW_Mutex_Exit(lock)     pthread_mutex_unlock(lock)

#endif

typedef void* volatile HW_AtomicPtr;
typedef long  volatile HW_AtomicInt;

HW_INLINE void HW_AtomicPtr_Set(HW_AtomicPtr* a, void* val)
{ InterlockedExchangePointer(a, val); }

HW_INLINE int HW_AtomicPtr_SetFrom(HW_AtomicPtr* a, void* from, void* to)
{ return InterlockedCompareExchangePointer(a, to, from) == from; }

HW_INLINE void HW_AtomicInt_Set(HW_AtomicInt* a, int val)
{ InterlockedExchange(a, val); }

HW_INLINE int HW_AtomicInt_SetFrom(HW_AtomicInt* a, int from, int to)
{ return InterlockedCompareExchange(a, to, from) == from; }

/* Returns the new value */
HW_INLINE int HW_AtomicInt_Increment(HW_AtomicInt* a)
{ return InterlockedIncrement(a); }

/* Returns the new value */
HW_INLINE int HW_AtomicInt_Decrement(HW_AtomicInt* a)
{ return InterlockedDecrement(a); }


typedef HW_AtomicInt HW_Spinlock;

#define HW_SPINLOCK_STATIC_INIT 0

HW_INLINE void HW_Spinlock_Init(HW_Spinlock* lock)
{ *lock = 0; }

HW_INLINE void HW_Spinlock_Destroy(HW_Spinlock* lock)
{ assert(*lock == 0); }

HW_INLINE void HW_Spinlock_Enter(HW_Spinlock* lock)
{ while (!HW_AtomicInt_SetFrom(lock, 0, 1)) {} }

HW_INLINE void HW_Spinlock_Exit(HW_Spinlock* lock)
{ HW_AtomicInt_Set(lock, 0); }


#ifdef __cplusplus
namespace HW
{
  class Mutex
  {
  public:
    Mutex()       {HW_Mutex_Init(&m_Lock);}
    ~Mutex()      {HW_Mutex_Destroy(&m_Lock);}

    void Enter()  {HW_Mutex_Enter(&m_Lock);}
    void Exit()   {HW_Mutex_Exit(&m_Lock);}

  private:
    HW_Mutex m_Lock;
  };

  class Spinlock
  {
  public:
    Spinlock()    {HW_Spinlock_Init(&m_Lock);}
    ~Spinlock()   {HW_Spinlock_Destroy(&m_Lock);}

    void Enter()  {HW_Spinlock_Enter(&m_Lock);}
    void Exit()   {HW_Spinlock_Exit(&m_Lock);}

  private:
    HW_Spinlock m_Lock;
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
