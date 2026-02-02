// *****************************************************************************
//
//					AutoCFStringRef - Specialization of AutoTypeRef for CFString
//
// Module:			GSRoot
// Namespace:		-
// Contact person:	FM
//
// SG compatible
//
// *****************************************************************************
#ifndef MACUTILS_AUTOCFSTRINGREF_HPP
#define MACUTILS_AUTOCFSTRINGREF_HPP

#pragma once

#include "GSRootExport.hpp"
#include "UniString.hpp"
#include "AutoCFTypeRef.hpp"

#include <CoreFoundation/CoreFoundation.h>

// --- Interface	----------------------------------------------------------------------------------------------------

namespace GS {

//*******************************************************************************
//
//  Special AutoCFTypeRef for CFString. Supports conversion of from/to CFString.
//
//*******************************************************************************

class GSROOT_DLL_EXPORT AutoCFStringRef {
private:
	CFStringRef		data;	// The actual CoreFoundation object.
public:
					AutoCFStringRef () : data (0) {}
					AutoCFStringRef (const UniString& uniStr);
	explicit        AutoCFStringRef (CFStringRef iData) : data (iData) {}
					~AutoCFStringRef () { CFRelease (); }
	void			Set (CFStringRef iData) { CFRelease (); data = iData; }
	void            CFRelease () { if (data != nullptr) ::CFRelease (data); data = nullptr; }
	CFStringRef		Detach () { CFStringRef result = data;  data = nullptr; return result; }
	CFStringRef		Get () const { return data; }
					operator CFStringRef () const { return Get (); }
	operator UniString () const { return GetUniString (); }
	UniString       GetUniString () const;
	UniString       GetComposedUniString () const;
	void			ThrowIfNull () const;
};

//*******************************************************************************
//
//  Special AutoCFStringRef: CFStringRef is not valid after object destruction.
//  You should use this only for temporary cases.
//
//*******************************************************************************

class GSROOT_DLL_EXPORT TemporaryAutoCFStringRef {
private:
	CFStringRef             data;       // The actual CoreFoundation object.
	const UniChar::Layout*  buffer;     // The buffer

	#if defined(DEBUVERS)
	GS::UniString           backup;
	Int32                    backupLength;
	#endif
	void                    Init (const UniChar::Layout* chars, Int32 length);
public:
					TemporaryAutoCFStringRef (const UniChar::Layout* chars, Int32 length);
					~TemporaryAutoCFStringRef () { CFRelease (); }
	void            CFRelease ();
	CFStringRef		Get () const { return data; }
					operator CFStringRef () const { return Get (); }
	void			ThrowIfNull () const;
};


}

#endif
