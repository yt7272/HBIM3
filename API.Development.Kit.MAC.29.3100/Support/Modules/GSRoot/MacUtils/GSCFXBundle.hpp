// *********************************************************************************************************************
// Description:		Utility functions to CFBundle
//
// Namespaces:		-
// Contact persons:	FM
//
// [SG compatible]
// *********************************************************************************************************************

#if !defined (GSCFXBUNDLE_HPP)
#define	GSCFXBUNDLE_HPP

#pragma once

// --- Includes	--------------------------------------------------------------------------------------------------------

// from GRoot
#include	"GSRootExport.hpp"

// from CoreFoundation
#include <ApplicationServices/ApplicationServices.h>

// --- Predeclarations	------------------------------------------------------------------------------------------------

struct mach_header;
struct mach_header_64;

// --- Interface	----------------------------------------------------------------------------------------------------
extern "C" {

//#if (__LP64__)
//extern CFBundleRef GSROOT_DLL_EXPORT GSCFXBundleCreateFromMachHeader (CFAllocatorRef allocator, const mach_header_64* header);
//#else
extern CFBundleRef	GSROOT_DLL_EXPORT	GSCFXBundleCreateFromMachHeader (CFAllocatorRef allocator, const mach_header* header);
extern CFURLRef		GSROOT_DLL_EXPORT	GSCFXBundleURLCreateFromMachHeader (CFAllocatorRef allocator, const mach_header* header);
extern CFURLRef		GSROOT_DLL_EXPORT	GSCFXBundleURLCreateFromImageName (CFAllocatorRef allocator, const char* image_name);
//#endif

extern CFStringRef GSROOT_DLL_EXPORT GSCFXBundleGetBundleName (CFAllocatorRef allocator, CFBundleRef theBundle) __attribute__((cf_returns_retained));
extern CFBundleRef GSROOT_DLL_EXPORT GSCFXCreateSystemBundle (const char* id, const char* bundleName);
}

#endif	// GSCFXBUNDLE_HPP
