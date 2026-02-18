// *****************************************************************************
// Helper functions for Add-On development (minimal for ComponentInfo)
// *****************************************************************************

#ifndef	_APICOMMON_H_
#define	_APICOMMON_H_

#include "UniString.hpp"

/* -- Messages ------------------------------- */

template<typename... Args>
void		WriteReport (const char* format, Args&&... args)
{
	ACAPI_WriteReport (format, false, std::forward<Args> (args)...);
}

void		WriteReport_Err (const char* info, GSErrCode err);
void		WriteReport_End (GSErrCode err);
void		ErrorBeep (const char* info, GSErrCode err);

const char*	ErrID_To_Name (GSErrCode err);

#endif
