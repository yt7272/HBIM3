// *****************************************************************************
// File:			PluginMain.cpp
// Description:		HBIM构件信息录入插件: CheckEnvironment, RegisterInterface,
//					Initialize, FreeData, APIMenuCommandProc_Main
// Project:			HBIM构件信息录入插件
// *****************************************************************************

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "PluginPalette.hpp"
#include <stdio.h>



static GSErrCode APIMenuCommandProc_Main (const API_MenuParams* menuParams);

// -----------------------------------------------------------------------------
// Add-on entry: CheckEnvironment
// -----------------------------------------------------------------------------
API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	// Set plugin name to "HBIM构件信息录入"
	envir->addOnInfo.name = "HBIM构件信息录入";
	envir->addOnInfo.description = "HBIM构件信息录入插件";
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
	PluginPalette::DestroyInstance ();
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
		if (PluginPalette::HasInstance () && PluginPalette::GetInstance ().IsVisible ()) {
			PluginPalette::GetInstance ().Hide ();
		} else {
			if (!PluginPalette::HasInstance ()) {
				PluginPalette::CreateInstance ();
			}
			PluginPalette::GetInstance ().Show ();
		}
		return NoError;
	}
	
	return NoError;
}