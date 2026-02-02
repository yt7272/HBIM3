
// *****************************************************************************
//
// Declaration and implementation of FastLock class (Macintosh)
//
// Module:          GSRoot
// Namespace:       GS
// Contact person:  SN
//
// *****************************************************************************

#ifndef GS_FASTLOCKIMPL_HPP
#define GS_FASTLOCKIMPL_HPP

#pragma once


// --- Includes ----------------------------------------------------------------

#include "SynchronizationException.hpp"
#include "UniString.hpp"

#include <errno.h>
#include <pthread.h>



// --- FastLock class ----------------------------------------------------------

namespace GS {

class FastLock {

// Data members:
private:
	pthread_mutex_t		m_lock;

// Construction / destruction:
public:
	 FastLock ();
private:
	 FastLock (const FastLock&); // Disabled
public:
	~FastLock ();

// Operator overloading:
private:
	const FastLock&		operator = (const FastLock&); // Disabled

// Operations:
public:
	void				Acquire ();
	bool				TryAcquire ();
	void				Release ();

};

////////////////////////////////////////////////////////////////////////////////
// FastLock implementation

////////////////////////////////////////////////////////////////////////////////
// Construction / destruction
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// Default constructor
// -----------------------------------------------------------------------------

inline FastLock::FastLock () :
	m_lock ()
{
	pthread_mutexattr_t attr;
	int error;
	
	error = ::pthread_mutexattr_init (&attr);
	if (error != 0) {
		throw SynchronizationException (
			GS::UniString::Printf ("Failed to create fast lock: failed to init mutex attribute. (Error code: %d)", error),
			__FILE__,
			__LINE__);
	}

	error = ::pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
	if (error != 0) {
		throw SynchronizationException (
			GS::UniString::Printf ("Failed to create fast lock: failed to set mutex attribute. (Error code: %d)", error),
			__FILE__,
			__LINE__);
	}

	error = ::pthread_mutex_init (&m_lock, &attr);
	if (error != 0) {
		throw SynchronizationException (
			GS::UniString::Printf ("Failed to create fast lock: failed to init mutex. (Error code: %d)", error),
			__FILE__,
			__LINE__);
	}
}

// -----------------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------------

inline FastLock::~FastLock ()
{
	::pthread_mutex_destroy (&m_lock);
}

////////////////////////////////////////////////////////////////////////////////
// Operations
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// Acquire
// -----------------------------------------------------------------------------

inline void		FastLock::Acquire ()
{
	int error = ::pthread_mutex_lock (&m_lock);
	if (error != 0) {
		throw SynchronizationException (
			GS::UniString::Printf ("Failed to acquire fast lock: failed to lock mutex. (Error code: %d)", error),
			__FILE__,
			__LINE__);
	}
}

// -----------------------------------------------------------------------------
// TryAcquire
// -----------------------------------------------------------------------------

inline bool		FastLock::TryAcquire ()
{
	int error = ::pthread_mutex_trylock (&m_lock);
	if (error != 0 && error != EBUSY) {
		throw SynchronizationException (
			GS::UniString::Printf ("Failed to acquire fast lock: failed to lock mutex. (Error code: %d)", error),
			__FILE__,
			__LINE__);
	}

	return (error == 0);
}

// -----------------------------------------------------------------------------
// Release
// -----------------------------------------------------------------------------

inline void		FastLock::Release ()
{
	int error = ::pthread_mutex_unlock (&m_lock);
	if (error != 0) {
		throw SynchronizationException (
			GS::UniString::Printf ("Failed to release fast lock: failed to unlock mutex. (Error code: %d)", error),
			__FILE__,
			__LINE__);
	}
}

}

#endif // GS_FASTLOCKIMPL_HPP
