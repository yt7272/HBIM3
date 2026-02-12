// *****************************************************************************
// File:			PluginMain.cpp
// Description:		HBIM Plugin: CheckEnvironment, RegisterInterface,
//					Initialize, FreeData, APIMenuCommandProc_Main
// Project:			HBIM Plugin
// *****************************************************************************

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "PluginPalette.hpp"

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
	GSErrCode err = ACAPI_MenuItem_RegisterMenu (32500, 32600, MenuCode_UserDef, MenuFlag_Default);
	return err;
}

// -----------------------------------------------------------------------------
// Initialize
// -----------------------------------------------------------------------------
GSErrCode Initialize (void)
{
	GSErrCode err = ACAPI_MenuItem_InstallMenuHandler (32500, APIMenuCommandProc_Main);
	if (err != NoError) {
		return err;
	}
	err = ACAPI_Notification_CatchSelectionChange (PluginPalette::SelectionChangeHandler);
	if (err != NoError) {
		return err;
	}
	err = PluginPalette::RegisterPaletteControlCallBack ();
	return err;
}

// -----------------------------------------------------------------------------
// FreeData
// -----------------------------------------------------------------------------
GSErrCode FreeData (void)
{
	ACAPI_Notification_CatchSelectionChange (nullptr);
	ACAPI_UnregisterModelessWindow (PluginPalette::GetPaletteReferenceId ());
	return NoError;
}

// -----------------------------------------------------------------------------
// APIMenuCommandProc_Main
// -----------------------------------------------------------------------------
static GSErrCode APIMenuCommandProc_Main (const API_MenuParams* menuParams)
{
	if (menuParams->menuItemRef.menuResID != 32500)
		return NoError;

	if (menuParams->menuItemRef.itemIndex == 1) {
		// Show/hide palette
		if (PluginPalette::HasInstance () && PluginPalette::GetInstance ().IsVisible ())
			PluginPalette::GetInstance ().Hide ();
		else {
			if (!PluginPalette::HasInstance ()) {
				PluginPalette::CreateInstance ();
			}
			PluginPalette::GetInstance ().Show ();
		}
		return NoError;
	}
	
	return NoError;
}