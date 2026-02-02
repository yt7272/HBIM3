// *********************************************************************************************************************
// Description:		Utility functions to CFTypeRef
//
// Namespaces:		-
// Contact persons:	FM
//
// [SG compatible]
// *********************************************************************************************************************

#if !defined (AUTOCFTYPEREF_HPP)
#define	AUTOCFTYPEREF_HPP

#pragma once

// --- Includes	--------------------------------------------------------------------------------------------------------

// from GRoot
#include	"GSRootExport.hpp"
#include 	"GSException.hpp"

// from Carbon
#include    <CoreFoundation/CoreFoundation.h>

// --- Interface	----------------------------------------------------------------------------------------------------

namespace GS {

//*******************************************************************************
//
//  AutoCFTypeRef class
//
//  An auto_ptr like class for releasing CoreFoundation objects.
//  T can be any CFObject like: CFNumberRef, CFStringRef, etc.
//  To relase the object use: obj.CFRelase();
//  Example usage:
//	   AutoCFTypeRef<CFStringRef> data (CFStringCreateWithCString (...));
//
//     /* use data.Get() as parameter */
//
//	   CFRelease (data.Release ());
//
//*******************************************************************************
template <class T>
class AutoCFTypeRef {
private:
	T				data;	// The actual CoreFoundation object.
public:
					AutoCFTypeRef () : data (nullptr) {};
	explicit        AutoCFTypeRef (T iData) : data(iData) {};
					~AutoCFTypeRef () { CFRelease (); }
	void			Set (T iData) { CFRelease (); data = iData; }
	void            CFRelease () { if (data != nullptr) ::CFRelease (data); data = nullptr; }
	T				Detach () { T result = data; data = nullptr; return result; }
	T				Get () const { return data; }
					operator T () const { return data; }
	T*  			GetPtr () { return &data; }
	void			ThrowIfNull () const { if (data == nullptr) throw GS::OutOfMemoryException (); }
};

}

#endif	// AUTOCFTYPEREF_HPP
