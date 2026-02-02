// *********************************************************************************************************************
// Description:		Utility functions to Quartz (CGContext)
//
// Namespaces:		GS
// Contact persons:	RA
//
// *********************************************************************************************************************


#if !defined (GSCGUTIL_HPP)
#define	GSCGUTIL_HPP

#pragma once

#include <ApplicationServices/ApplicationServices.h>
#include "GSRootExport.hpp"
#include "GSDebug.hpp"

namespace GS {

class GSROOT_DLL_EXPORT CGContextGState
{
public:
	CGContextGState ();
	CGContextGState (CGContextRef cgCntxt, bool save = false);
	~CGContextGState ();

	void SetCGContext (CGContextRef	cgCntxt);
	void ClearCGContext ();

	void Save ();
	void Restore ();
	void RestoreAll ();

private:
	CGContextRef	cgContext;
	int				saveCount;
	bool			saveAtConstructor;
};

GSROOT_DLL_EXPORT void* CreateNSImageFromCGImage (CGImageRef imageRef, double resolutionFactor = 1.);

}	// namespace GS

#endif	// GSCGUTIL_HPP
