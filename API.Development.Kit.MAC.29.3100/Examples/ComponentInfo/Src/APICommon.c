// *****************************************************************************
// Helper functions for Add-On development (minimal for ComponentInfo)
// *****************************************************************************

#include	"GSSystem.h"
#include	"APIEnvir.h"
#include	"ACAPinc.h"
#include	"APICommon.h"

void WriteReport_Err (const char* info, GSErrCode err)
{
	ACAPI_WriteReport ("%s: %s", true, info, ErrID_To_Name (err));
}

void WriteReport_End (GSErrCode err)
{
	if (err == NoError)
		ACAPI_WriteReport ("OK", false);
	else
		ACAPI_WriteReport ("Error: %s", false, ErrID_To_Name (err));
}

void ErrorBeep (const char* info, GSErrCode err)
{
	WriteReport_Err (info, err);
	GSSysBeep ();
}

const char* ErrID_To_Name (GSErrCode err)
{
	switch (err) {
		case APIERR_GENERAL:			return "APIERR_GENERAL";
		case APIERR_MEMFULL:			return "APIERR_MEMFULL";
		case APIERR_CANCEL:				return "APIERR_CANCEL";
		case APIERR_BADID:				return "APIERR_BADID";
		case APIERR_BADINDEX:			return "APIERR_BADINDEX";
		case APIERR_BADPARS:			return "APIERR_BADPARS";
		case APIERR_NOPLAN:				return "APIERR_NOPLAN";
		case APIERR_NOSEL:				return "APIERR_NOSEL";
		case APIERR_BADPROPERTY:		return "APIERR_BADPROPERTY";
		case APIERR_NAMEALREADYUSED:	return "APIERR_NAMEALREADYUSED";
		default:						return "Unknown Error";
	}
}
