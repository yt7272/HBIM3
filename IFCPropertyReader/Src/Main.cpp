// *****************************************************************************
// Source code for the IFC Property Reader Add-On
// *****************************************************************************

// =============================================================================
//
// API Includes
//
// =============================================================================

#include	"APIEnvir.h"
#include	"ACAPinc.h"		// also includes APIdefs.h
#include	"IFCPropertyReaderPalette.hpp"

// -----------------------------------------------------------------------------
// Show or Hide Browser Palette
// -----------------------------------------------------------------------------

static void	ShowOrHideIFCPropertyReaderPalette ()
{
	if (IFCPropertyReaderPalette::HasInstance () && IFCPropertyReaderPalette::GetInstance ().IsVisible ()) {
		IFCPropertyReaderPalette::GetInstance ().Hide ();
	} else {
		if (!IFCPropertyReaderPalette::HasInstance ())
			IFCPropertyReaderPalette::CreateInstance ();
		IFCPropertyReaderPalette::GetInstance ().Show ();
	}
}

// -----------------------------------------------------------------------------
// MenuCommandHandler
//		called to perform the user-asked command
// -----------------------------------------------------------------------------

GSErrCode MenuCommandHandler (const API_MenuParams *menuParams)
{
	switch (menuParams->menuItemRef.menuResID) {
		case IFCPropertyReaderPaletteMenuResId:
			switch (menuParams->menuItemRef.itemIndex) {
				case IFCPropertyReaderPaletteMenuItemIndex:
					ShowOrHideIFCPropertyReaderPalette ();
					break;
			}
			break;
	}

	return NoError;
}


// =============================================================================
//
// Required functions
//
// =============================================================================

// -----------------------------------------------------------------------------
// Dependency definitions
// -----------------------------------------------------------------------------

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, 32000, 1, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, 32000, 2, ACAPI_GetOwnResModule ());

	return APIAddon_Preload;
}		// CheckEnvironment


// -----------------------------------------------------------------------------
// Interface definitions
// -----------------------------------------------------------------------------

GSErrCode RegisterInterface (void)
{
	GSErrCode err = ACAPI_MenuItem_RegisterMenu (IFCPropertyReaderPaletteMenuResId, 0, MenuCode_UserDef, MenuFlag_Default);
	if (DBERROR (err != NoError))
		return err;

	return err;
}		// RegisterInterface


// -----------------------------------------------------------------------------
// Initialize
//		called after the Add-On has been loaded into memory
// -----------------------------------------------------------------------------

GSErrCode Initialize (void)
{
	GSErrCode err = ACAPI_MenuItem_InstallMenuHandler (IFCPropertyReaderPaletteMenuResId, MenuCommandHandler);
	if (DBERROR (err != NoError))
		return err;

	err = ACAPI_Notification_CatchSelectionChange (IFCPropertyReaderPalette::SelectionChangeHandler);
	if (DBERROR (err != NoError))
		return err;

	err = IFCPropertyReaderPalette::RegisterPaletteControlCallBack ();
	if (DBERROR (err != NoError))
		return err;

	return err;
}		// Initialize


// -----------------------------------------------------------------------------
// FreeData
//		called when the Add-On is going to be unloaded
// -----------------------------------------------------------------------------

GSErrCode FreeData (void)
{
	return NoError;
}		// FreeData
