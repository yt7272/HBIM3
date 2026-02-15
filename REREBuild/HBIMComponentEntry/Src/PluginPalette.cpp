#include "PluginPalette.hpp"
#include "APICommon.h"
#include <mutex>
#include <stdio.h>

namespace {
}



static const GS::Guid s_paletteGuid ("{A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}");
static GS::Ref<PluginPalette> s_instance;
static std::recursive_mutex s_instanceMutex;

static GSErrCode NotificationHandler (API_NotifyEventID notifID, Int32)
{
	if (notifID == APINotify_Quit)
		PluginPalette::DestroyInstance ();
	return NoError;
}

PluginPalette::PluginPalette ()
	: DG::Palette (ACAPI_GetOwnResModule (), PaletteResId, ACAPI_GetOwnResModule (), s_paletteGuid)
	, titleLabel (GetReference (), TitleLabelId)
{

	
	ACAPI_ProjectOperation_CatchProjectEvent (APINotify_Quit, NotificationHandler);
	Attach (*this);
	
	GS::UniString titleText;
	titleText = "HBIM构件信息录入 v";
	titleText += AC_ADDON_VERSION_STRING;
	
	titleLabel.SetText(titleText);
	titleLabel.Show();
	titleLabel.Redraw();
	
	BeginEventProcessing ();
}

PluginPalette::~PluginPalette ()
{
	EndEventProcessing ();
}

bool PluginPalette::HasInstance ()
{
	std::lock_guard<std::recursive_mutex> lock(s_instanceMutex);
	return s_instance != nullptr;
}

void PluginPalette::CreateInstance ()
{
	std::lock_guard<std::recursive_mutex> lock(s_instanceMutex);
	if (s_instance == nullptr) {
		s_instance = new PluginPalette ();
	}
}

PluginPalette& PluginPalette::GetInstance ()
{
	std::lock_guard<std::recursive_mutex> lock(s_instanceMutex);
	return *s_instance;
}

void PluginPalette::DestroyInstance ()
{
	std::lock_guard<std::recursive_mutex> lock(s_instanceMutex);
	s_instance = nullptr;
}

void PluginPalette::Show ()
{
	SetMenuItemCheckedState (true);
	DG::Palette::Show ();
}

void PluginPalette::Hide ()
{
	SetMenuItemCheckedState (false);
	DG::Palette::Hide ();
}

void PluginPalette::UpdateFromSelection ()
{
}

bool PluginPalette::IsInEditMode ()
{
	return false;
}

void PluginPalette::SetEditMode (bool editMode)
{
}

void PluginPalette::SetMenuItemCheckedState (bool checked)
{
	API_MenuItemRef itemRef;
	itemRef.menuResID = 32500;
	itemRef.itemIndex = 1;
	
	GSFlags itemFlags = 0;
	GSErrCode err = ACAPI_MenuItem_GetMenuItemFlags (&itemRef, &itemFlags);
	if (err == NoError) {
		if (checked) {
			itemFlags |= API_MenuItemChecked;
		} else {
			itemFlags &= (GSFlags) ~API_MenuItemChecked;
		}
		err = ACAPI_MenuItem_SetMenuItemFlags (&itemRef, &itemFlags);
	}
}

GSErrCode PluginPalette::PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param)
{
	if (!HasInstance ())
		return NoError;
	
	PluginPalette& instance = GetInstance ();
	(void)instance; // Mark as intentionally unused for now
	
	switch (messageID) {
		case APIPalMsg_OpenPalette:
			break;
			
		case APIPalMsg_ClosePalette:
			break;
			
		case APIPalMsg_HidePalette_Begin:
			break;
			
		case APIPalMsg_HidePalette_End:
			break;
			
		case APIPalMsg_DisableItems_Begin:
			break;
			
		case APIPalMsg_DisableItems_End:
			break;
			
		default:
			break;
	}
	
	return NoError;
}

GSErrCode PluginPalette::SelectionChangeHandler (const API_Neig* selElemNeig)
{
	return NoError;
}

GSErrCode PluginPalette::RegisterPaletteControlCallBack ()
{
	return ACAPI_RegisterModelessWindow (
		GetPaletteReferenceId (),
		PaletteControlCallBack,
		API_PalEnabled_FloorPlan + API_PalEnabled_Section + API_PalEnabled_Elevation +
		API_PalEnabled_InteriorElevation + API_PalEnabled_3D + API_PalEnabled_Detail +
		API_PalEnabled_Worksheet + API_PalEnabled_Layout + API_PalEnabled_DocumentFrom3D,
		GSGuid2APIGuid (s_paletteGuid));
}

Int32 PluginPalette::GetPaletteReferenceId ()
{
	return GS::CalculateHashValue (s_paletteGuid);
}

void PluginPalette::PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted)
{
	*accepted = true;
}

void PluginPalette::ButtonClicked (const DG::ButtonClickEvent& ev)
{
}