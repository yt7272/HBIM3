// *****************************************************************************
// File:			ComponentInfoMain.cpp
// Description:		Component Info Add-on: CheckEnvironment, RegisterInterface,
//					Initialize, FreeData, APIMenuCommandProc_Main
// Project:			ComponentInfo
// *****************************************************************************

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "ComponentInfo.hpp"

static GSErrCode APIMenuCommandProc_Main (const API_MenuParams* menuParams);

// -----------------------------------------------------------------------------
// Add-on entry: CheckEnvironment
// -----------------------------------------------------------------------------
API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, 32000, 1, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, 32000, 2, ACAPI_GetOwnResModule ());
	return APIAddon_Normal;
}

// -----------------------------------------------------------------------------
// RegisterInterface
// -----------------------------------------------------------------------------
GSErrCode RegisterInterface (void)
{
	return ACAPI_MenuItem_RegisterMenu (32500, 32600, MenuCode_UserDef, MenuFlag_Default);
}

// -----------------------------------------------------------------------------
// Initialize
// -----------------------------------------------------------------------------
GSErrCode Initialize (void)
{
	return ACAPI_MenuItem_InstallMenuHandler (32500, APIMenuCommandProc_Main);
}

// -----------------------------------------------------------------------------
// FreeData
// -----------------------------------------------------------------------------
GSErrCode FreeData (void)
{
	return NoError;
}

// -----------------------------------------------------------------------------
// APIMenuCommandProc_Main
// -----------------------------------------------------------------------------
static GSErrCode APIMenuCommandProc_Main (const API_MenuParams* menuParams)
{
	if (menuParams->menuItemRef.menuResID != 32500)
		return NoError;
	if (menuParams->menuItemRef.itemIndex != 1)
		return NoError;

	return ACAPI_CallUndoableCommand ("构件扩展信息",
		[&] () -> GSErrCode {
			ComponentInfo::RunComponentInfoCommand ();
			return NoError;
		}
	);
}
