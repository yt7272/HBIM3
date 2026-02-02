// *****************************************************************************
// Source code for the Opening Test Add-On
// *****************************************************************************
// 
// ---------------------------------- Includes ---------------------------------

#include	"APIEnvir.h"
#include	"ACAPinc.h"					// also includes APIdefs.h
#include	"APICommon.h"

#include	"Opening_Functions.hpp"
#include	"Resources.hpp"

// -----------------------------------------------------------------------------
// OpeningFunctions
// -----------------------------------------------------------------------------
GSErrCode OpeningFunctions (const API_MenuParams* menuParams)
{
	ACAPI_KeepInMemory (false);

	switch (menuParams->menuItemRef.itemIndex) {
		case 1:		return PlaceOpeningIntoAnElement ();						break;
		case 2:		return PlacePolygonalOpeningIntoAnElement ();				break;
		case 3:		return ChangeOpeningsParent ();								break;
		case 4:		return ConnectOpeningToAnotherElement ();					break;
		case 5:		return DisconnectOpeningFromAnotherElement ();				break;
		case 6:		return SelectOpeningsConnectedToElement ();					break;
		case 7:		return SelectElementsConnectedToOpening ();					break;
		case 8:		return SelectOpeningsParentElement ();						break;
		case 9:		return ChangeOpeningsDimensionsToCurrentDefault ();			break;
		case 10:	return ChangeOpeningsCutSymbolToCurrentDefault ();			break;
		case 11:	return ChangeOpeningsOverrideSurfacesToCurrentDefault ();	break;
		case 12:	return ChangeOpeningDefaultsCoverFillTypeAndPen ();			break;
		default:
			break;
	}
	return NoError;
}		/* OpeningFunctions */

// -----------------------------------------------------------------------------
// Dependency definitions
// -----------------------------------------------------------------------------

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, OPENING_TEST_ADDON_NAME, 1, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, OPENING_TEST_ADDON_NAME, 2, ACAPI_GetOwnResModule ());

	return APIAddon_Preload;
}		/* CheckEnvironment */


// -----------------------------------------------------------------------------
// Interface definitions
// -----------------------------------------------------------------------------
GSErrCode RegisterInterface (void)
{
	return ACAPI_MenuItem_RegisterMenu (OPENING_TEST_MENU_STRINGS, OPENING_TEST_MENU_PROMPT_STRINGS, MenuCode_UserDef, MenuFlag_Default);
}		/* RegisterInterface */


// -----------------------------------------------------------------------------
// Called when the Add-On has been loaded into memory
// to perform an operation
// -----------------------------------------------------------------------------
GSErrCode Initialize (void)
{
	return ACAPI_MenuItem_InstallMenuHandler (OPENING_TEST_MENU_STRINGS, OpeningFunctions);
}		/* Initialize */


// -----------------------------------------------------------------------------
// Called when the Add-On is going to be unloaded
// -----------------------------------------------------------------------------

GSErrCode FreeData (void)
{
	return NoError;
}		/* FreeData */
