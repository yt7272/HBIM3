// *****************************************************************************
// Source code for the IFCPropertyReaderPalette class
// *****************************************************************************

// ---------------------------------- Includes ---------------------------------

#include	"IFCPropertyReaderPalette.hpp"
#include	"IFCPropertyUtils.hpp"

static const GS::Guid	paletteGuid ("{533D8AED-455A-47FF-9A69-9DE9B43C9FBE}");

GS::Ref<IFCPropertyReaderPalette>	IFCPropertyReaderPalette::instance;

static GS::UniString LoadHtmlFromResource ()
{
	GS::UniString resourceData;
	GSHandle data = RSLoadResource ('DATA', ACAPI_GetOwnResModule (), 100);
	GSSize handleSize = BMhGetSize (data);
	if (data != nullptr) {
		resourceData.Append (*data, handleSize);
		BMhKill (&data);
	}
	return resourceData;
}

static GS::UniString GetStringFromJavaScriptVariable (GS::Ref<JS::Base> jsVariable)
{
	GS::Ref<JS::Value> jsValue = GS::DynamicCast<JS::Value> (jsVariable);
	if (DBVERIFY (jsValue != nullptr && jsValue->GetType () == JS::Value::STRING))
		return jsValue->GetString ();

	return GS::EmptyUniString;
}

template<class Type>
static GS::Ref<JS::Base> ConvertToJavaScriptVariable (const Type& cppVariable)
{
	return new JS::Value (cppVariable);
}

template<>
GS::Ref<JS::Base> ConvertToJavaScriptVariable (const IFCPropertyReaderPalette::ElementInfo& elemInfo)
{
	GS::Ref<JS::Array> js = new JS::Array ();
	js->AddItem (ConvertToJavaScriptVariable (elemInfo.guidStr));
	js->AddItem (ConvertToJavaScriptVariable (elemInfo.typeName));
	js->AddItem (ConvertToJavaScriptVariable (elemInfo.elemID));
	return js;
}

template<>
GS::Ref<JS::Base> ConvertToJavaScriptVariable (const IFCPropertyUtils::PropertyInfo& propInfo)
{
	GS::Ref<JS::Array> js = new JS::Array ();
	js->AddItem (ConvertToJavaScriptVariable (propInfo.propertySetName));
	js->AddItem (ConvertToJavaScriptVariable (propInfo.propertyName));
	js->AddItem (ConvertToJavaScriptVariable (propInfo.propertyValue));
	js->AddItem (ConvertToJavaScriptVariable (propInfo.propertyType));
	js->AddItem (ConvertToJavaScriptVariable (propInfo.unit));
	js->AddItem (ConvertToJavaScriptVariable (propInfo.isValid));
	return js;
}

template<class Type>
static GS::Ref<JS::Base> ConvertToJavaScriptVariable (const GS::Array<Type>& cppArray)
{
	GS::Ref<JS::Array> newArray = new JS::Array ();
	for (const Type& item : cppArray) {
		newArray->AddItem (ConvertToJavaScriptVariable (item));
	}
	return newArray;
}

// -----------------------------------------------------------------------------
// Project event handler function
// -----------------------------------------------------------------------------
static GSErrCode NotificationHandler (API_NotifyEventID notifID, Int32 /*param*/)
{
	switch (notifID) {
		case APINotify_Quit:
			IFCPropertyReaderPalette::DestroyInstance ();
			break;
	}

	return NoError;
}   // NotificationHandler

// --- Class definition: IFCPropertyReaderPalette ----------------------------------------

IFCPropertyReaderPalette::IFCPropertyReaderPalette () :
	DG::Palette (ACAPI_GetOwnResModule (), IFCPropertyReaderPaletteResId, ACAPI_GetOwnResModule (), paletteGuid),
	browser (GetReference (), BrowserId)
{
	ACAPI_ProjectOperation_CatchProjectEvent (APINotify_Quit, NotificationHandler);
	Attach (*this);
	BeginEventProcessing ();
	InitBrowserControl ();
}

IFCPropertyReaderPalette::~IFCPropertyReaderPalette ()
{
	EndEventProcessing ();
}

bool IFCPropertyReaderPalette::HasInstance ()
{
	return instance != nullptr;
}

void IFCPropertyReaderPalette::CreateInstance ()
{
	DBASSERT (!HasInstance ());
	instance = new IFCPropertyReaderPalette ();
	ACAPI_KeepInMemory (true);
}

IFCPropertyReaderPalette&	IFCPropertyReaderPalette::GetInstance ()
{
	DBASSERT (HasInstance ());
	return *instance;
}

void IFCPropertyReaderPalette::DestroyInstance ()
{
	instance = nullptr;
}

void IFCPropertyReaderPalette::Show ()
{
	DG::Palette::Show ();
	SetMenuItemCheckedState (true);
}

void IFCPropertyReaderPalette::Hide ()
{
	DG::Palette::Hide ();
	SetMenuItemCheckedState (false);
}

void IFCPropertyReaderPalette::InitBrowserControl ()
{
	browser.LoadHTML (LoadHtmlFromResource ());
	RegisterACAPIJavaScriptObject ();
	UpdateIFCPropertiesOnHTML ();
}

void  IFCPropertyReaderPalette::RegisterACAPIJavaScriptObject ()
{
	JS::Object* jsACAPI = new JS::Object ("ACAPI");

	jsACAPI->AddItem (new JS::Function ("GetSelectedElements", [] (GS::Ref<JS::Base>) {
		return ConvertToJavaScriptVariable (GetSelectedElements ());
	}));

	jsACAPI->AddItem (new JS::Function ("AddElementToSelection", [] (GS::Ref<JS::Base> param) {
		ModifySelection (GetStringFromJavaScriptVariable (param), AddToSelection);
		return ConvertToJavaScriptVariable (true);
	}));

	jsACAPI->AddItem (new JS::Function ("RemoveElementFromSelection", [] (GS::Ref<JS::Base> param) {
		ModifySelection (GetStringFromJavaScriptVariable (param), RemoveFromSelection);
		return ConvertToJavaScriptVariable (true);
	}));

	jsACAPI->AddItem (new JS::Function ("GetIFCProperties", [] (GS::Ref<JS::Base>) {
		GS::Array<IFCPropertyUtils::PropertyInfo> properties = IFCPropertyUtils::GetIFCPropertiesForSelectedElements ();
		return ConvertToJavaScriptVariable (properties);
	}));

	jsACAPI->AddItem (new JS::Function ("GetAllIFCProperties", [] (GS::Ref<JS::Base> param) {
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (param);
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		GS::Array<IFCPropertyUtils::PropertyInfo> properties = IFCPropertyUtils::GetAllIFCPropertiesForElement (elementGuid);
		return ConvertToJavaScriptVariable (properties);
	}));

	// 缓存相关函数
	jsACAPI->AddItem (new JS::Function ("GetCachedIFCProperties", [] (GS::Ref<JS::Base> param) {
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (param);
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		// 默认不强制刷新
		GS::Array<IFCPropertyUtils::PropertyInfo> properties = IFCPropertyUtils::GetCachedIFCPropertiesForElement (elementGuid, false);
		return ConvertToJavaScriptVariable (properties);
	}));

	jsACAPI->AddItem (new JS::Function ("GetCachedIFCPropertiesWithRefresh", [] (GS::Ref<JS::Base> param) {
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (param);
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		// 强制刷新缓存
		GS::Array<IFCPropertyUtils::PropertyInfo> properties = IFCPropertyUtils::GetCachedIFCPropertiesForElement (elementGuid, true);
		return ConvertToJavaScriptVariable (properties);
	}));

	jsACAPI->AddItem (new JS::Function ("ClearIFCPropertyCache", [] (GS::Ref<JS::Base>) {
		IFCPropertyUtils::ClearIFCPropertyCache ();
		return ConvertToJavaScriptVariable (true);
	}));

	jsACAPI->AddItem (new JS::Function ("GetIFCPropertyCacheSize", [] (GS::Ref<JS::Base>) {
		UInt32 cacheSize = IFCPropertyUtils::GetIFCPropertyCacheSize ();
		return ConvertToJavaScriptVariable (cacheSize);
	}));

	jsACAPI->AddItem (new JS::Function ("GetPluginVersion", [] (GS::Ref<JS::Base>) {
		GS::UniString version = IFCPropertyUtils::GetAddOnVersionString ();
		return ConvertToJavaScriptVariable (version);
	}));

	browser.RegisterAsynchJSObject (jsACAPI);
}

void IFCPropertyReaderPalette::UpdateSelectedElementsOnHTML ()
{
	browser.ExecuteJS ("UpdateSelectedElements ()");
}

void IFCPropertyReaderPalette::UpdateIFCPropertiesOnHTML ()
{
	browser.ExecuteJS ("UpdateIFCProperties ()");
}

void IFCPropertyReaderPalette::SetMenuItemCheckedState (bool isChecked)
{
	API_MenuItemRef	itemRef {};
	GSFlags			itemFlags {};

	itemRef.menuResID = IFCPropertyReaderPaletteMenuResId;
	itemRef.itemIndex = IFCPropertyReaderPaletteMenuItemIndex;

	ACAPI_MenuItem_GetMenuItemFlags (&itemRef, &itemFlags);
	if (isChecked)
		itemFlags |= API_MenuItemChecked;
	else
		itemFlags &= ~API_MenuItemChecked;
	ACAPI_MenuItem_SetMenuItemFlags (&itemRef, &itemFlags);
}

void IFCPropertyReaderPalette::PanelResized (const DG::PanelResizeEvent& ev)
{
	BeginMoveResizeItems ();
	browser.Resize (ev.GetHorizontalChange (), ev.GetVerticalChange ());
	EndMoveResizeItems ();
}

void IFCPropertyReaderPalette::PanelCloseRequested (const DG::PanelCloseRequestEvent&, bool* accepted)
{
	Hide ();
	*accepted = true;
}

GS::Array<IFCPropertyReaderPalette::ElementInfo> IFCPropertyReaderPalette::GetSelectedElements ()
{
	API_SelectionInfo	selectionInfo;
	GS::Array<API_Neig>	selNeigs;
	ACAPI_Selection_Get (&selectionInfo, &selNeigs, false, false);
	BMKillHandle ((GSHandle*)&selectionInfo.marquee.coords);

	GS::Array<IFCPropertyReaderPalette::ElementInfo> selectedElements;
	for (const API_Neig& neig : selNeigs) {
		API_Elem_Head elemHead {};
		elemHead.guid = neig.guid;
		ACAPI_Element_GetHeader (&elemHead);

		ElementInfo elemInfo;
		elemInfo.guidStr = APIGuidToString (elemHead.guid);
		ACAPI_Element_GetElemTypeName (elemHead.type, elemInfo.typeName);
		ACAPI_Element_GetElementInfoString (&elemHead.guid, &elemInfo.elemID);
		selectedElements.Push (elemInfo);
	}
	return selectedElements;
}

void IFCPropertyReaderPalette::ModifySelection (const GS::UniString& elemGuidStr, IFCPropertyReaderPalette::SelectionModification modification)
{
	ACAPI_Selection_Select ({ API_Neig (APIGuidFromString (elemGuidStr.ToCStr ().Get ())) }, modification == AddToSelection);
}

GSErrCode IFCPropertyReaderPalette::SelectionChangeHandler (const API_Neig*)
{
	if (IFCPropertyReaderPalette::HasInstance ())
		IFCPropertyReaderPalette::GetInstance ().UpdateIFCPropertiesOnHTML ();
	return NoError;
}

GSErrCode IFCPropertyReaderPalette::PaletteControlCallBack (Int32, API_PaletteMessageID messageID, GS::IntPtr param)
{
	switch (messageID) {
		case APIPalMsg_OpenPalette:
			if (!HasInstance ())
				CreateInstance ();
			GetInstance ().Show ();
			break;

		case APIPalMsg_ClosePalette:
			if (!HasInstance ())
				break;
			GetInstance ().Hide ();
			break;

		case APIPalMsg_HidePalette_Begin:
			if (HasInstance () && GetInstance ().IsVisible ())
				GetInstance ().Hide ();
			break;

		case APIPalMsg_HidePalette_End:
			if (HasInstance () && !GetInstance ().IsVisible ())
				GetInstance ().Show ();
			break;

		case APIPalMsg_DisableItems_Begin:
			if (HasInstance () && GetInstance ().IsVisible ())
				GetInstance ().DisableItems ();
			break;

		case APIPalMsg_DisableItems_End:
			if (HasInstance () && GetInstance ().IsVisible ())
				GetInstance ().EnableItems ();
			break;

		case APIPalMsg_IsPaletteVisible:
			*(reinterpret_cast<bool*> (param)) = HasInstance () && GetInstance ().IsVisible ();
			break;

		default:
			break;
	}

	return NoError;
}

GSErrCode IFCPropertyReaderPalette::RegisterPaletteControlCallBack ()
{
	return ACAPI_RegisterModelessWindow (
					GS::CalculateHashValue (paletteGuid),
					PaletteControlCallBack,
					API_PalEnabled_FloorPlan + API_PalEnabled_Section + API_PalEnabled_Elevation +
					API_PalEnabled_InteriorElevation + API_PalEnabled_3D + API_PalEnabled_Detail +
					API_PalEnabled_Worksheet + API_PalEnabled_Layout + API_PalEnabled_DocumentFrom3D,
					GSGuid2APIGuid (paletteGuid));
}
