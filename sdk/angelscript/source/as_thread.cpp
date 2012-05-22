/*
   AngelCode Scripting Library
   Copyright (c) 2003-2012 Andreas Jonsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/



//
// as_thread.cpp
//
// Functions for multi threading support
//

#include "as_config.h"
#include "as_thread.h"
#include "as_atomic.h"

BEGIN_AS_NAMESPACE

//=======================================================================

// Singleton
static asCThreadManager *threadManager = 0;

//======================================================================

// Global API functions
extern "C"
{

AS_API int asThreadCleanup()
{
	return asCThreadManager::CleanupLocalData();
}

AS_API void asPrepareMultithread()
{
	asCThreadManager::Prepare();
}

AS_API void asUnprepareMultithread()
{
	asCThreadManager::Unprepare();
}

AS_API void asAcquireExclusiveLock()
{
	if( threadManager )
		ACQUIREEXCLUSIVE(threadManager->appRWLock);
}

AS_API void asReleaseExclusiveLock()
{
	if( threadManager )
		RELEASEEXCLUSIVE(threadManager->appRWLock);
}

}

//======================================================================

asCThreadManager::asCThreadManager()
{
	// We're already in the critical section when this function is called

#ifdef AS_NO_THREADS
	tld = 0;
#endif
	refCount = 1;
}

void asCThreadManager::Prepare()
{
	// The critical section cannot be declared globally, as there is no
	// guarantee for the order in which global variables are initialized
	// or uninitialized.

	// For this reason it's not possible to prevent two threads from calling 
	// AddRef at the same time, so there is a chance for a race condition here.

	// To avoid the race condition when the thread manager is first created, 
	// the application must make sure to call the global asPrepareForMultiThread()
	// in the main thread before any other thread creates a script engine. 
	if( threadManager == 0 )
		threadManager = asNEW(asCThreadManager);
	else
	{
		ENTERCRITICALSECTION(threadManager->criticalSection);
		threadManager->refCount++;
		LEAVECRITICALSECTION(threadManager->criticalSection);
	}
}

void asCThreadManager::Unprepare()
{
	asASSERT(threadManager);

	if( threadManager == 0 )
		return;

	// It's necessary to protect this section so no
	// other thread attempts to call AddRef or Release
	// while clean up is in progress.
	ENTERCRITICALSECTION(threadManager->criticalSection);
	if( --threadManager->refCount == 0 )
	{
		// As the critical section will be destroyed together 
		// with the thread manager we must first clear the global
		// variable in case a new thread manager needs to be created;
		asCThreadManager *mgr = threadManager;
		threadManager = 0;

		// Leave the critical section before it is destroyed
		LEAVECRITICALSECTION(mgr->criticalSection);

		asDELETE(mgr,asCThreadManager);
	}
	else
		LEAVECRITICALSECTION(threadManager->criticalSection);
}

asCThreadManager::~asCThreadManager()
{
#ifndef AS_NO_THREADS
	// Delete all thread local datas
	asSMapNode<asPWORD,asCThreadLocalData*> *cursor = 0;
	if( tldMap.MoveFirst(&cursor) )
	{
		do
		{
			if( tldMap.GetValue(cursor) ) 
			{
				asDELETE(tldMap.GetValue(cursor),asCThreadLocalData);
			}
		} while( tldMap.MoveNext(&cursor, cursor) );
	}
#else
	if( tld ) 
	{
		asDELETE(tld,asCThreadLocalData);
	}
	tld = 0;
#endif
}

int asCThreadManager::CleanupLocalData()
{
	if( threadManager == 0 )
		return 0;

#ifndef AS_NO_THREADS
	int r = 0;
#if defined AS_POSIX_THREADS
	asPWORD id = (asPWORD)pthread_self();
#elif defined AS_WINDOWS_THREADS
	asPWORD id = (asPWORD)GetCurrentThreadId();
#endif

	ENTERCRITICALSECTION(threadManager->criticalSection);

	asSMapNode<asPWORD,asCThreadLocalData*> *cursor = 0;
	if( threadManager->tldMap.MoveTo(&cursor, id) )
	{
		asCThreadLocalData *tld = threadManager->tldMap.GetValue(cursor);
		
		// Can we really remove it at this time?
		if( tld->activeContexts.GetLength() == 0 )
		{
			asDELETE(tld,asCThreadLocalData);
			threadManager->tldMap.Erase(cursor);
			r = 0;
		}
		else
			r = asCONTEXT_ACTIVE;
	}

	LEAVECRITICALSECTION(threadManager->criticalSection);

	return r;
#else
	if( threadManager->tld )
	{
		if( threadManager->tld->activeContexts.GetLength() == 0 )
		{
			asDELETE(threadManager->tld,asCThreadLocalData);
			threadManager->tld = 0;
		}
		else
			return asCONTEXT_ACTIVE;
	}
	return 0;
#endif
}

#ifndef AS_NO_THREADS
asCThreadLocalData *asCThreadManager::GetLocalData(asPWORD threadId)
{
	// We're already in the critical section when this function is called

	asCThreadLocalData *tld = 0;

	asSMapNode<asPWORD,asCThreadLocalData*> *cursor = 0;
	if( threadManager->tldMap.MoveTo(&cursor, threadId) )
		tld = threadManager->tldMap.GetValue(cursor);

	return tld;
}

void asCThreadManager::SetLocalData(asPWORD threadId, asCThreadLocalData *tld)
{
	// We're already in the critical section when this function is called

	tldMap.Insert(threadId, tld);
}
#endif

asCThreadLocalData *asCThreadManager::GetLocalData()
{
	if( threadManager == 0 )
		return 0;

#ifndef AS_NO_THREADS
#if defined AS_POSIX_THREADS
	asPWORD id = (asPWORD)pthread_self();
#elif defined AS_WINDOWS_THREADS
	asPWORD id = (asPWORD)GetCurrentThreadId();
#endif

	ENTERCRITICALSECTION(threadManager->criticalSection);

	asCThreadLocalData *tld = threadManager->GetLocalData(id);
	if( tld == 0 )
	{
		// Create a new tld
		tld = asNEW(asCThreadLocalData)();
		threadManager->SetLocalData(id, tld);
	}

	LEAVECRITICALSECTION(threadManager->criticalSection);

	return tld;
#else
	if( threadManager->tld == 0 )
		threadManager->tld = asNEW(asCThreadLocalData)();

	return threadManager->tld;
#endif
}

//=========================================================================

asCThreadLocalData::asCThreadLocalData()
{
}

asCThreadLocalData::~asCThreadLocalData()
{
}

//=========================================================================

#ifndef AS_NO_THREADS
asCThreadCriticalSection::asCThreadCriticalSection()
{
#if defined AS_POSIX_THREADS
	pthread_mutex_init(&cs, 0);
#elif defined AS_WINDOWS_THREADS
	InitializeCriticalSection(&cs);
#endif
}

asCThreadCriticalSection::~asCThreadCriticalSection()
{
#if defined AS_POSIX_THREADS
	pthread_mutex_destroy(&cs);
#elif defined AS_WINDOWS_THREADS
	DeleteCriticalSection(&cs);
#endif
}

void asCThreadCriticalSection::Enter()
{
#if defined AS_POSIX_THREADS
	pthread_mutex_lock(&cs);
#elif defined AS_WINDOWS_THREADS
	EnterCriticalSection(&cs);
#endif
}

void asCThreadCriticalSection::Leave()
{
#if defined AS_POSIX_THREADS
	pthread_mutex_unlock(&cs);
#elif defined AS_WINDOWS_THREADS
	LeaveCriticalSection(&cs);
#endif
}

bool asCThreadCriticalSection::TryEnter()
{
#if defined AS_POSIX_THREADS
	return !pthread_mutex_trylock(&cs);
#elif defined AS_WINDOWS_THREADS
	return TryEnterCriticalSection(&cs) ? true : false;
#else
	return true;
#endif
}

asCThreadReadWriteLock::asCThreadReadWriteLock()
{
#if defined AS_POSIX_THREADS
	int r = pthread_rwlock_init(&lock, 0);
	asASSERT( r == 0 );
#elif defined AS_WINDOWS_THREADS
	// With SRWLOCK - InitializeSRWLock(&lock);
	InitializeCriticalSection(&cs);
#endif
}

asCThreadReadWriteLock::~asCThreadReadWriteLock()
{
#if defined AS_POSIX_THREADS
	pthread_rwlock_destroy(&lock);
#elif defined AS_WINDOWS_THREADS
	// With SRWLOCK - no cleanup needed
	DeleteCriticalSection(&cs);
#endif
}

void asCThreadReadWriteLock::AcquireExclusive()
{
#if defined AS_POSIX_THREADS
	pthread_rwlock_wrlock(&lock);
#elif defined AS_WINDOWS_THREADS
	// With SRWLOCK - AcquireSRWLockExclusive(&lock);
	EnterCriticalSection(&cs);
#endif
}

void asCThreadReadWriteLock::ReleaseExclusive()
{
#if defined AS_POSIX_THREADS
	pthread_rwlock_unlock(&lock);
#elif defined AS_WINDOWS_THREADS
	// With SRWLOCK - ReleaseSRWLockExclusive(&lock);
	LeaveCriticalSection(&cs);
#endif
}

void asCThreadReadWriteLock::AcquireShared()
{
#if defined AS_POSIX_THREADS
	pthread_rwlock_rdlock(&lock);
#elif defined AS_WINDOWS_THREADS
	// With SRWLOCK - AcquireSRWLockShared(&lock);
	EnterCriticalSection(&cs);
#endif
}

void asCThreadReadWriteLock::ReleaseShared()
{
#if defined AS_POSIX_THREADS
	pthread_rwlock_unlock(&lock);
#elif defined AS_WINDOWS_THREADS
	// With SRWLOCK - ReleaseSRWLockShared(&lock);
	LeaveCriticalSection(&cs);
#endif
}

/* The Try options for SRWLOCK are only available from Win7 so we won't implement it
bool asCThreadReadWriteLock::TryAcquireExclusive()
{
#if defined AS_POSIX_THREADS
	return pthread_rwlock_trywrlock(&lock) ? false : true;
#elif defined AS_WINDOWS_THREADS
	return TryAcquireSRWLockExclusive(&lock) ? true : false;
#else
	return true;
#endif
}

bool asCThreadReadWriteLock::TryAcquireShared()
{
#if defined AS_POSIX_THREADS
	return pthread_rwlock_tryrdlock(&lock) ? false : true;
#elif defined AS_WINDOWS_THREADS
	return TryAcquireSRWLockShared(&lock) ? true : false;
#else
	return true;
#endif
}
*/

#endif

//========================================================================

END_AS_NAMESPACE

