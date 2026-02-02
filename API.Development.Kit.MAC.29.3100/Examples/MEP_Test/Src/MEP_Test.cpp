#include "MEP_Test.hpp"
#include "Panels.hpp"
#include "SettingsHandler.hpp"
#include "Resources.hpp"

#include "QueryElementsAndDefaults.hpp"
#include "PlaceElements.hpp"
#include "PlacePolylineFlexibleSegments.hpp"
#include "ModifyElements.hpp"
#include "Preferences.hpp"
#include "Utility.hpp"
#include "LabelingElements.hpp"
#include "DistributionSystemsGraph.hpp"
#include "SystemBrowserCalculationCallbackInterface.hpp"
#include "GenerateLargeSystems.hpp"
#include "LabelingSystems.hpp"

// API
#include "ACAPinc.h"

// MEPPlugin
#include "ACAPI/MEPAdapter.hpp"


// -----------------------------------------------------------------------------
//
// Global variables and definitions
//
// -----------------------------------------------------------------------------

MEPExample::TestSettingsPanel*		settingsPanel = nullptr;
MEPExample::TestSettingsHandler*	settingsHandler = nullptr;

#ifndef NO_MEP_FEATURES

const Int32 settingsPanelRefCon = 1;


// -----------------------------------------------------------------------------
// Create tabpage callback function
// -----------------------------------------------------------------------------


static GSErrCode CreatePageCallback (Int32 /*refCon*/, const void* tabControl, void* data, void** tabPage)
{
	bool success = false;
	const DG::TabControl* control = reinterpret_cast<const DG::TabControl*>(tabControl);
	DG::TabPage** page = reinterpret_cast<DG::TabPage**>(tabPage);

	if (settingsPanel != nullptr)
		success = settingsPanel->CreatePage (*control, reinterpret_cast<TBUI::IAPIToolUIData*>(data), page);

	return (success ? NoError : (GSErrCode) APIERR_GENERAL);
}


// -----------------------------------------------------------------------------
// Destroy tabpage callback function
// -----------------------------------------------------------------------------

static GSErrCode DestroyPageCallback (Int32 /*refCon*/, void* /*tabPage*/)
{
	if (settingsPanel != nullptr)
		settingsPanel->DestroyPage ();

	return NoError;
}


// -----------------------------------------------------------------------------
// Create settings callback
// -----------------------------------------------------------------------------

static GSErrCode CreateSettingsCallback (void* data)
{
	GSErrCode err = NoError;

	TBUI::IAPIToolUIData* uiData = reinterpret_cast<TBUI::IAPIToolUIData*> (data);
	if (uiData == nullptr)
		return APIERR_GENERAL;

	settingsHandler = new MEPExample::TestSettingsHandler (uiData);

	return err;
}


// -----------------------------------------------------------------------------
// Destroy settings callback
// -----------------------------------------------------------------------------

static GSErrCode DestroySettingsCallback (void* /*data*/)
{
	GSErrCode err = NoError;

	delete settingsHandler;
	settingsHandler = nullptr;

	return err;
}
#endif


// -----------------------------------------------------------------------------
// MenuCommandHandler
//		called to perform the user-asked command
// -----------------------------------------------------------------------------

GSErrCode MenuCommandHandler (const API_MenuParams *menuParams)
{
	switch (menuParams->menuItemRef.menuResID) {
		case MEP_TEST_MENU_STRINGS:
		{
			switch (menuParams->menuItemRef.itemIndex) {
				case 1:
					return MEPExample::PlaceMEPElements ();
				case 2:
					return MEPExample::CopyPlaceSelectedTerminals ();
				case 3:
					break; // separator
				case 4:
					return MEPExample::PlaceTwoContinuousRoutes ();
				case 5:
					return MEPExample::PlaceTwoRoutesInTShape ();
				case 6:
					return MEPExample::PlaceThreeRoutesInTShape ();
				case 7:
					return MEPExample::PlaceConnectedElements ();
				case 8:
					return MEPExample::PlaceAndConnectRoutes ();
				case 9:
					return MEPExample::PlaceAnAccessoryBetweenTwoRoutes ();
				case 10:
					return MEPExample::PlacePolylineFlexibleSegments ();
				case 11:
					return MEPExample::PlaceSegmentedRouting ();
				case 12:
					return MEPExample::PlaceRoutesBasedOnFirstFavorite ();
				case 13:
					break; // separator
				case 14:
					return MEPExample::QuerySelectedElementDetails ();
				case 15:
					return MEPExample::QueryDefaultDetails ();
				case 16:
					break; // separator
				case 17:
					return MEPExample::ModifySelectedTerminals ();
				case 18:
					return MEPExample::ModifySelectedRoutingElements ();
				case 19:
					return MEPExample::ModifySelectedFlexibleSegments ();
				case 20:
					return MEPExample::ModifySelectedTakeOff ();
				case 21:
					return MEPExample::ResizeShiftAndReorientSelectedMEPElements ();
				case 22:
					return MEPExample::ModifySelectedElemGeneralParameters ();
				case 23:
					return MEPExample::ModifySelectedEquipment ();
				case 24:
					return MEPExample::ModifySelectedPipeFittingGDLParameters ();
				case 25:
					return MEPExample::ModifySelectedRoutingElemGDLParameters ();
				case 26:
					return MEPExample::ModifyPipeFittingDefaultParameters ();
				case 27:
					return MEPExample::ModifyRoutingElemDefaultParameters ();
				case 28:
					return MEPExample::ModifyLibraryPartOfSelectedPipeTerminal ();
				case 29:
					return MEPExample::ModifyLibraryPartOfDefaultPipeTerminalThenPlace ();
				case 30:
					return MEPExample::ModifyDefaultEquipmentBaseRotationAndFlipThenPlace ();
				case 31:
					return MEPExample::ModifyLibraryPartOfSelectedCableCarrierRoutesFirstRigidSegment ();
				case 32:
					return MEPExample::ModifyLibraryPartOfSelectedCableCarrierRoutesDefaultRigidSegment ();
				case 33:
					return MEPExample::ModifyLibraryPartOfDefaultCableCarrierRoutesRigidSegment ();
				case 34:
					break; // separator
				case 35:
					return MEPExample::DeleteSelectedElements ();
				case 36:
					break; // separator
				case 37:
					return MEPExample::QueryMEPPreferences ();
				case 38:
					return MEPExample::ModifyDuctSegmentPreferences ();
				case 39:
					return MEPExample::ModifyPipeElbowPreferences ();
				case 40:
					return MEPExample::ModifyPipeBranchPreferences ();
				case 41:
					return MEPExample::ModifyDuctTransitionPreferences ();
				case 42:
					return MEPExample::ModifyPipeReferenceSet ();
				case 43:
					return MEPExample::ModifyPipeAngleSet ();
				case 44:
					break; // separator
				case 45:
					return MEPExample::CreateElementSetFromSelectedElements ();
				case 46:
					return MEPExample::CreateElementLinksBetweenSelectedElements ();
				case 47:
					return MEPExample::LabelSubelementsOfSelectedRoute ();
				case 48:
					return MEPExample::LabelSelectedMEPElements ();
				case 49:
					return MEPExample::LabelConnectionsInGraph ();
				case 50:
					return MEPExample::UseCalculationInterface ();
				case 51:
					return MEPExample::LabelCurrentSystemInstancesAndGroups ();
				case 52:
					break; // separator
				case 53:
					return MEPExample::GenerateLargeSystems_Duct ();
				case 54:
					return MEPExample::GenerateLargeSystems_Pipe ();
				case 55:
					return MEPExample::ModifySelectedRoutesToByMVO ();
				case 56:
					return MEPExample::ModifySelectedRoutesToProjected ();
				case 57:
					return MEPExample::ModifySelectedRoutesToProjectedWithOverhead ();
				case 58:
					return MEPExample::ModifySelectedRoutesToSymbolic ();
				case 59:
					return MEPExample::ModifySelectedRoutesToAtUpperDisplayLimit ();
				case 60:
					return MEPExample::ModifySelectedRoutesToByViewSettings ();
				case 61:
					return MEPExample::ModifySelectedRoutesShowProjectionToFloorPlanRange ();
				case 62:
					return MEPExample::ModifySelectedRoutesToAllRelevantStories ();
				case 63:
					return MEPExample::ModifySelectedRoutesToHomeStoryOnly ();
			}
			break;
		}
	}

	return NoError;
}		// MenuCommandHandler


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
	RSGetIndString (&envir->addOnInfo.name,        MEP_TEST_ADDON_NAME, 1, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, MEP_TEST_ADDON_NAME, 2, ACAPI_GetOwnResModule ());

	return APIAddon_Preload;
}		// CheckEnvironment


// -----------------------------------------------------------------------------
// Interface definitions
// -----------------------------------------------------------------------------

GSErrCode RegisterInterface (void)
{
#ifndef NO_MEP_FEATURES
	// Register Menu
	ERRCHK (ACAPI_MenuItem_RegisterMenu (MEP_TEST_MENU_STRINGS, MEP_TEST_MENU_PROMPT_STRINGS, MenuCode_UserDef, MenuFlag_Default));

	// Register Settings Panel
	API_ElemType mepRoutingType (ACAPI::MEP::PipingRoutingSegmentID);

	ERRCHK (ACAPI_AddOnIntegration_RegisterSettingsPanel (settingsPanelRefCon, mepRoutingType, IDS_SETTINGSPAGE_ICON, IDS_SETTINGSPAGE_NAME, SettingsPageId));
	ERRCHK (ACAPI_AddOnIntegration_RegisterSettingsObserver (mepRoutingType));

#endif

	return NoError;
}		// RegisterInterface


// -----------------------------------------------------------------------------
// Initialize
//		called after the Add-On has been loaded into memory
// -----------------------------------------------------------------------------

GSErrCode Initialize (void)
{
#ifndef NO_MEP_FEATURES
	ERRCHK (ACAPI_MenuItem_InstallMenuHandler (MEP_TEST_MENU_STRINGS, MenuCommandHandler));

	try {
		settingsPanel = new MEPExample::TestSettingsPanel (settingsPanelRefCon);
	}
	catch (...) {
		DBPrintf ("Panel_Test add-on: settingsPanel construction failed\n");
		settingsPanel = nullptr;
	}

	if (settingsPanel != nullptr) {
		ERRCHK (ACAPI_AddOnIntegration_InstallPanelHandler (settingsPanel->GetRefCon (), CreatePageCallback, DestroyPageCallback));
	}

	API_ElemType mepRoutingType (ACAPI::MEP::PipingRoutingSegmentID);
	ERRCHK (ACAPI_AddOnIntegration_InstallSettingsHandler (mepRoutingType, CreateSettingsCallback, DestroySettingsCallback));

#endif

	return NoError;
}


// -----------------------------------------------------------------------------
// FreeData
//		called when the Add-On is going to be unloaded
// -----------------------------------------------------------------------------

GSErrCode FreeData (void)
{
	if (settingsPanel != nullptr)
		delete settingsPanel;

	return NoError;
}
