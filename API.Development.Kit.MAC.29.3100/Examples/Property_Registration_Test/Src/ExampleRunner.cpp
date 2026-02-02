#include "ExampleRunner.hpp"
#include "Property_Registration_Test.hpp"
#include "ACAPI/PropertyOperationManager.hpp"
#include "DGButton.hpp"
#include "DGUtility.hpp"
#include "APIdefs_Interface.h"
#include "APIdefs_Callback.h"
#include "APIEnvir.h"
#include "ACAPinc.h"
#include "Definitions.hpp"
#include "HashCommon.hpp"
#include "Result.hpp"
#include "GSGuid.hpp"
#include "GSAssert.hpp"
#include "GSDebug.hpp"
#include <memory>

/**********************************************************************************************************************/


const GS::Guid APIDevKitExamples::PropertyRegistration::ExampleRunner::s_paletteGuid ("{988556BA-0CC2-40DD-9CE4-DAA7A5873460}");

const GS::Guid APIDevKitExamples::PropertyRegistration::ExampleRunner::s_propertyGroup1Guid { "{BF770E7A-233C-4086-9283-0AE914BAF48D}" };
const GS::Guid APIDevKitExamples::PropertyRegistration::ExampleRunner::s_propertyGroup2Guid { "{182198B7-0204-41DF-9B2D-FCE0CC22138F}" };

const GS::Guid APIDevKitExamples::PropertyRegistration::ExampleRunner::s_property1_1Guid { "{24209B65-35E1-4DD6-9911-E45C870D45F5}" };
const GS::Guid APIDevKitExamples::PropertyRegistration::ExampleRunner::s_property1_2Guid { "{CBDB59B2-F07B-488C-A939-27237E86EF83}" };
const GS::Guid APIDevKitExamples::PropertyRegistration::ExampleRunner::s_property2_1Guid { "{4704D267-AE75-48DA-B72C-C2FD48C66B65}" };
const GS::Guid APIDevKitExamples::PropertyRegistration::ExampleRunner::s_property2_2Guid { "{53009C31-1A5C-4191-A805-81C9D1A86AE8}" };
const GS::Guid APIDevKitExamples::PropertyRegistration::ExampleRunner::s_property2_3Guid { "{3E82AE43-C036-4E8E-AFA7-4CB7FDDAE04E}" };

std::unique_ptr<APIDevKitExamples::PropertyRegistration::ExampleRunner> APIDevKitExamples::PropertyRegistration::ExampleRunner::s_instance;


/**********************************************************************************************************************/


GSResModule GetOwnResModule ()
{
	static GSResModule resModule = 0;

	if (resModule == 0)
		resModule = ACAPI_GetOwnResModule ();

	return resModule;
}


DG::Icon GetResourceIcon (const UInt32 index) 
{
	return DG::Icon (GetOwnResModule (), static_cast<short> (index));
}


static GSErrCode NotificationHandler (const API_NotifyEventID notifID, const Int32)
{
	switch (notifID) {
	case APINotify_Quit:
		APIDevKitExamples::PropertyRegistration::ExampleRunner::DestroyInstance ();
		return NoError;
	}

	return NoError;
}


void APIDevKitExamples::PropertyRegistration::ExampleRunner::Toggle ()
{
	if (HaveInstance () && GetInstance ().IsVisible ()) {
		GetInstance ().Hide ();
	}
	else {
		if (!HaveInstance ())
			CreateInstance ();
		GetInstance ().Show ();
	}
}


bool APIDevKitExamples::PropertyRegistration::ExampleRunner::HaveInstance ()
{
	return s_instance != nullptr;
}


APIDevKitExamples::PropertyRegistration::ExampleRunner& APIDevKitExamples::PropertyRegistration::ExampleRunner::GetInstance ()
{
	PRECOND (HaveInstance ());
	return *s_instance;
}


void APIDevKitExamples::PropertyRegistration::ExampleRunner::CreateInstance ()
{
	PRECOND (!HaveInstance ());
	s_instance = std::make_unique<ExampleRunner> ();
	ACAPI_KeepInMemory (true);
}


void APIDevKitExamples::PropertyRegistration::ExampleRunner::DestroyInstance ()
{
	s_instance.reset ();
}


APIDevKitExamples::PropertyRegistration::ExampleRunner::ExampleRunner ()
	: DG::Palette { ACAPI_GetOwnResModule (), ExampleRunnerResId, ACAPI_GetOwnResModule (), s_paletteGuid }
	, m_registerButton { GetReference (), RegisterButtonId }
	, m_unregisterButton { GetReference (), UnregisterButtonId }
	, m_registerStateText { GetReference (), RegisterStateTextId }
	, m_propertyOperationManager { *ACAPI::CreatePropertyOperationManager (ACAPI_GetToken ()) }
{
	ACAPI_ProjectOperation_CatchProjectEvent (APINotify_Quit, NotificationHandler);

	Attach (*this);
	m_registerButton.Attach (*this);
	m_unregisterButton.Attach (*this);
	SetupProperties ();
	BeginEventProcessing ();
}


void APIDevKitExamples::PropertyRegistration::ExampleRunner::SetupProperties ()
{
	auto propertyGroup1 = m_propertyOperationManager.CreateAPIPropertyGroup (s_propertyGroup1Guid, "Test Group 1", GetResourceIcon (1));
	GSASSERT (propertyGroup1.IsOk ());

	auto propertyGroup2 = m_propertyOperationManager.CreateAPIPropertyGroup (s_propertyGroup2Guid, "Test Group 2", GetResourceIcon (1));
	GSASSERT (propertyGroup2.IsOk ());

	const auto invalidGroupGuid = m_propertyOperationManager.CreateAPIPropertyGroup (GS::Guid {}, "test1", GetResourceIcon (1));
	GSASSERT (!invalidGroupGuid.IsOk ());
	const auto invalidGroupName = m_propertyOperationManager.CreateAPIPropertyGroup (GS::Guid { GS::Guid::GenerateGuid }, "", GetResourceIcon (1));
	GSASSERT (!invalidGroupName.IsOk ());
	const auto invalidGroupIcon = m_propertyOperationManager.CreateAPIPropertyGroup (GS::Guid { GS::Guid::GenerateGuid }, "test2", DG::Icon {});
	GSASSERT (!invalidGroupIcon.IsOk ());

	const auto property1_1 = m_propertyOperationManager.CreateAndAddAPIProperty (
		*propertyGroup1,
		s_property1_1Guid,
		"Test Wall Property with Integer return (Number of windows only if there are windows)",
		ACAPI::PropertyOperations::CriteriaEnvironmentType::OverrideAvailable,
		ACAPI::PropertyOperations::GroupAvailability::EveryTypesAvailable,
		ACAPI::PropertyOperations::PropertyToUseInContext::FieldAndCriteria,
		ACAPI::PropertyOperations::PropertyValueType::Integer,
		false,
		{ API_ElemType (API_WallID) },
		GetResourceIcon (1),
		[] (const API_Guid& guid) {
			auto apiElem = API_Element {};
			apiElem.header.type = API_ElemType (API_WallID);
			apiElem.header.guid = guid;
			GSErrCode err = ACAPI_Element_Get (&apiElem);
			if (err != NoError)
				return false;
			return apiElem.wall.hasWindow;
		},
		[] (const API_Guid& guid) {
			API_ElementMemo memo {};
			auto err = ACAPI_Element_GetMemo (guid, &memo, APIMemoMask_WallWindows);
			if (err || memo.wallWindows == nullptr)
				return 0;
			GSSize	nWinds = BMGetPtrSize (reinterpret_cast<GSPtr> (memo.wallWindows)) / sizeof (API_Guid);
			return nWinds;
		});
	GSASSERT (property1_1.IsOk ());

	const auto property1_2 = m_propertyOperationManager.CreateAndAddAPIProperty (
		*propertyGroup1,
		s_property1_2Guid,
		"Test Wall Property with double return (for walls thicker than 150)",
		ACAPI::PropertyOperations::CriteriaEnvironmentType::OverrideAvailable,
		ACAPI::PropertyOperations::GroupAvailability::EveryTypesAvailable,
		ACAPI::PropertyOperations::PropertyToUseInContext::FieldAndCriteria,
		ACAPI::PropertyOperations::PropertyValueType::Numeric,
		false,
		{ API_ElemType (API_WallID) },
		GetResourceIcon (1),
		[] (const API_Guid& guid) {
			auto apiElem = API_Element {};
			apiElem.header.type = API_ElemType (API_WallID);
			apiElem.header.guid = guid;
			GSErrCode err = ACAPI_Element_Get (&apiElem);
			if (err != NoError)
				return false;
			return apiElem.wall.thickness > 0.150;
		},
		[] (const API_Guid& guid) {
			auto apiElem = API_Element {};
			apiElem.header.type = API_ElemType (API_WallID);
			apiElem.header.guid = guid;
			GSErrCode err = ACAPI_Element_Get (&apiElem);
			if (err != NoError)
				return 0.0;
			return apiElem.wall.thickness;
		});
	GSASSERT (property1_2.IsOk ());

	const auto property2_1 = m_propertyOperationManager.CreateAndAddAPIProperty (
		*propertyGroup2,
		s_property2_1Guid,
		"Test Wall Property with String return (String with property name)",
		ACAPI::PropertyOperations::CriteriaEnvironmentType::OverrideAvailable,
		ACAPI::PropertyOperations::GroupAvailability::EveryTypesAvailable,
		ACAPI::PropertyOperations::PropertyToUseInContext::FieldAndCriteria,
		ACAPI::PropertyOperations::PropertyValueType::String,
		false,
		{ API_ElemType (API_WallID) },
		GetResourceIcon (1),
		[] (const API_Guid&) { return true; },
		[] (const API_Guid&) { return GS::UniString { "Test Wall Property with String return" }; });
	GSASSERT (property2_1.IsOk ());

	const auto property2_2 = m_propertyOperationManager.CreateAndAddAPIProperty (
		*propertyGroup2,
		s_property2_2Guid,
		"Test Wall Property with Bool return (Always True)",
		ACAPI::PropertyOperations::CriteriaEnvironmentType::OverrideAvailable,
		ACAPI::PropertyOperations::GroupAvailability::EveryTypesAvailable,
		ACAPI::PropertyOperations::PropertyToUseInContext::FieldAndCriteria,
		ACAPI::PropertyOperations::PropertyValueType::Boolean,
		false,
		{ API_ElemType (API_WallID) },
		GetResourceIcon (1),
		[] (const API_Guid&) { return true; },
		[] (const API_Guid&) { return true; });
	GSASSERT (property2_2.IsOk ());

	const auto property2_3 = m_propertyOperationManager.CreateAndAddAPIProperty (
		*propertyGroup2,
		s_property2_3Guid,
		"Test Wall, Door and Roof Property with Integer return (42)",
		ACAPI::PropertyOperations::CriteriaEnvironmentType::OverrideAvailable,
		ACAPI::PropertyOperations::GroupAvailability::EveryTypesAvailable,
		ACAPI::PropertyOperations::PropertyToUseInContext::FieldAndCriteria,
		ACAPI::PropertyOperations::PropertyValueType::Integer,
		false,
		{ API_ElemType (API_WallID), API_ElemType (API_DoorID), API_ElemType (API_RoofID) },
		GetResourceIcon (1),
		[] (const API_Guid&) { return true; },
		[] (const API_Guid&) { return 42; });
	GSASSERT (property2_3.IsOk ());

	const auto duplicatedGuid = m_propertyOperationManager.CreateAndAddAPIProperty (
		*propertyGroup2,
		s_property2_3Guid,
		"test3",
		ACAPI::PropertyOperations::CriteriaEnvironmentType::OverrideAvailable,
		ACAPI::PropertyOperations::GroupAvailability::EveryTypesAvailable,
		ACAPI::PropertyOperations::PropertyToUseInContext::FieldAndCriteria,
		ACAPI::PropertyOperations::PropertyValueType::Integer,
		false,
		{ API_ElemType (API_WallID), API_ElemType (API_DoorID), API_ElemType (API_RoofID) },
		GetResourceIcon (1),
		[] (const API_Guid&) { return true; },
		[] (const API_Guid&) { return 42; });
	GSASSERT (!duplicatedGuid.IsOk ());
	
	const auto invalidPropertyGuid = m_propertyOperationManager.CreateAndAddAPIProperty (
		*propertyGroup2,
		GS::Guid {},
		"test4",
		ACAPI::PropertyOperations::CriteriaEnvironmentType::OverrideAvailable,
		ACAPI::PropertyOperations::GroupAvailability::EveryTypesAvailable,
		ACAPI::PropertyOperations::PropertyToUseInContext::FieldAndCriteria,
		ACAPI::PropertyOperations::PropertyValueType::Integer,
		false,
		{ API_ElemType (API_WallID), API_ElemType (API_DoorID), API_ElemType (API_RoofID) },
		GetResourceIcon (1),
		[] (const API_Guid&) { return true; },
		[] (const API_Guid&) { return 42; });
	GSASSERT (!invalidPropertyGuid.IsOk ());

	const auto invalidElements = m_propertyOperationManager.CreateAndAddAPIProperty (
		*propertyGroup2,
		GS::Guid { GS::Guid::GenerateGuid },
		"test5",
		ACAPI::PropertyOperations::CriteriaEnvironmentType::OverrideAvailable,
		ACAPI::PropertyOperations::GroupAvailability::EveryTypesAvailable,
		ACAPI::PropertyOperations::PropertyToUseInContext::FieldAndCriteria,
		ACAPI::PropertyOperations::PropertyValueType::Integer,
		false,
		{},
		GetResourceIcon (1),
		[] (const API_Guid&) { return true; },
		[] (const API_Guid&) { return 42; });
	GSASSERT (!invalidElements.IsOk ());

	const auto invalidPropertyName = m_propertyOperationManager.CreateAndAddAPIProperty (
		*propertyGroup2,
		GS::Guid { GS::Guid::GenerateGuid },
		"",
		ACAPI::PropertyOperations::CriteriaEnvironmentType::OverrideAvailable,
		ACAPI::PropertyOperations::GroupAvailability::EveryTypesAvailable,
		ACAPI::PropertyOperations::PropertyToUseInContext::FieldAndCriteria,
		ACAPI::PropertyOperations::PropertyValueType::Integer,
		false,
		{ API_ElemType (API_WallID), API_ElemType (API_DoorID), API_ElemType (API_RoofID) },
		GetResourceIcon (1),
		[] (const API_Guid&) { return true; },
		[] (const API_Guid&) { return 42; });
	GSASSERT (!invalidPropertyName.IsOk ());

	const auto invalidPropertyIcon = m_propertyOperationManager.CreateAndAddAPIProperty (
		*propertyGroup2,
		GS::Guid { GS::Guid::GenerateGuid },
		"test6",
		ACAPI::PropertyOperations::CriteriaEnvironmentType::OverrideAvailable,
		ACAPI::PropertyOperations::GroupAvailability::EveryTypesAvailable,
		ACAPI::PropertyOperations::PropertyToUseInContext::FieldAndCriteria,
		ACAPI::PropertyOperations::PropertyValueType::Integer,
		false,
		{ API_ElemType (API_WallID), API_ElemType (API_DoorID), API_ElemType (API_RoofID) },
		{},
		[] (const API_Guid&) { return true; },
		[] (const API_Guid&) { return 42; });
	GSASSERT (!invalidPropertyIcon.IsOk ());

	const auto invalidEvaluableFor = m_propertyOperationManager.CreateAndAddAPIProperty (
		*propertyGroup2,
		GS::Guid { GS::Guid::GenerateGuid },
		"test7",
		ACAPI::PropertyOperations::CriteriaEnvironmentType::OverrideAvailable,
		ACAPI::PropertyOperations::GroupAvailability::EveryTypesAvailable,
		ACAPI::PropertyOperations::PropertyToUseInContext::FieldAndCriteria,
		ACAPI::PropertyOperations::PropertyValueType::Integer,
		false,
		{ API_ElemType (API_WallID), API_ElemType (API_DoorID), API_ElemType (API_RoofID) },
		GetResourceIcon (1),
		{},
		[] (const API_Guid&) { return 42; });
	GSASSERT (!invalidEvaluableFor.IsOk ());

	const auto invalidValueGetter = m_propertyOperationManager.CreateAndAddAPIProperty (
		*propertyGroup2,
		GS::Guid { GS::Guid::GenerateGuid },
		"test8",
		ACAPI::PropertyOperations::CriteriaEnvironmentType::OverrideAvailable,
		ACAPI::PropertyOperations::GroupAvailability::EveryTypesAvailable,
		ACAPI::PropertyOperations::PropertyToUseInContext::FieldAndCriteria,
		ACAPI::PropertyOperations::PropertyValueType::Integer,
		false,
		{ API_ElemType (API_WallID), API_ElemType (API_DoorID), API_ElemType (API_RoofID) },
		GetResourceIcon (1),
		[] (const API_Guid&) { return true; },
		{});
	GSASSERT (!invalidValueGetter.IsOk ());

	const auto ret1 = m_propertyOperationManager.AddPropertyGroup (std::move (*propertyGroup1));
	GSASSERT (ret1.IsOk ());
	const auto ret2 = m_propertyOperationManager.AddPropertyGroup (*propertyGroup2);
	GSASSERT (ret2.IsOk ());
	
	const auto retForDuplicateGroup = m_propertyOperationManager.AddPropertyGroup (std::move (*propertyGroup2));
	GSASSERT (!retForDuplicateGroup.IsOk ());
}


APIDevKitExamples::PropertyRegistration::ExampleRunner::~ExampleRunner ()
{
	EndEventProcessing ();
}


void APIDevKitExamples::PropertyRegistration::ExampleRunner::Show ()
{
	DG::Palette::Show ();
	SetMenuItemCheckedState (true);
}


void APIDevKitExamples::PropertyRegistration::ExampleRunner::Hide ()
{
	DG::Palette::Hide ();
	SetMenuItemCheckedState (false);
}


void APIDevKitExamples::PropertyRegistration::ExampleRunner::SetMenuItemCheckedState (bool isChecked)
{
	auto itemRef = API_MenuItemRef {};
	auto itemFlags = GSFlags {};

	itemRef.menuResID = ExampleRunnerResId;
	itemRef.itemIndex = ExampleRunnerMenuId;

	ACAPI_MenuItem_GetMenuItemFlags (&itemRef, &itemFlags);
	if (isChecked)
		itemFlags |= API_MenuItemChecked;
	else
		itemFlags &= ~API_MenuItemChecked;
	ACAPI_MenuItem_SetMenuItemFlags (&itemRef, &itemFlags);
}


void APIDevKitExamples::PropertyRegistration::ExampleRunner::PanelResized (const DG::PanelResizeEvent& ev)
{
	BeginMoveResizeItems ();
	m_registerButton.MoveAndResize (0, ev.GetVerticalChange (), ev.GetHorizontalChange (), 0);
	m_unregisterButton.MoveAndResize (0, ev.GetVerticalChange (), ev.GetHorizontalChange (), 0);
	EndMoveResizeItems ();
}


void APIDevKitExamples::PropertyRegistration::ExampleRunner::PanelCloseRequested (const DG::PanelCloseRequestEvent&, bool* accepted)
{
	PRECOND (accepted != nullptr);

	Hide ();
	*accepted = true;
}


void APIDevKitExamples::PropertyRegistration::ExampleRunner::ButtonClicked (const DG::ButtonClickEvent& ev)
{
	PRECOND (ev.GetSource () == &m_registerButton || ev.GetSource () == &m_unregisterButton);

	if (ev.GetSource () == &m_registerButton)
		Register ();
	else if (ev.GetSource () == &m_unregisterButton)
		Unregister ();
}


GSErrCode APIDevKitExamples::PropertyRegistration::ExampleRunner::PaletteControlCallBack (const Int32, const API_PaletteMessageID messageID, const GS::IntPtr param)
{
	switch (messageID) {
	case APIPalMsg_OpenPalette:
		if (!HaveInstance ())
			CreateInstance ();
		GetInstance ().Show ();
		return NoError;

	case APIPalMsg_ClosePalette:
		if (!HaveInstance ())
			return NoError;
		GetInstance ().Hide ();
		return NoError;

	case APIPalMsg_HidePalette_Begin:
		if (HaveInstance () && GetInstance ().IsVisible ())
			GetInstance ().Hide ();
		return NoError;

	case APIPalMsg_HidePalette_End:
		if (HaveInstance () && !GetInstance ().IsVisible ())
			GetInstance ().Show ();
		return NoError;

	case APIPalMsg_DisableItems_Begin:
		if (HaveInstance () && GetInstance ().IsVisible ())
			GetInstance ().DisableItems ();
		return NoError;

	case APIPalMsg_DisableItems_End:
		if (HaveInstance () && GetInstance ().IsVisible ())
			GetInstance ().EnableItems ();
		return NoError;

	case APIPalMsg_IsPaletteVisible:
		*(reinterpret_cast<bool*> (param)) = HaveInstance () && GetInstance ().IsVisible ();
		return NoError;

	default:
		return NoError;
	}
}


GSErrCode APIDevKitExamples::PropertyRegistration::ExampleRunner::RegisterPaletteControlCallBack ()
{
	return ACAPI_RegisterModelessWindow (
		GS::CalculateHashValue (s_paletteGuid),
		PaletteControlCallBack,
		API_PalEnabled_FloorPlan + API_PalEnabled_Section + API_PalEnabled_Elevation +
		API_PalEnabled_InteriorElevation + API_PalEnabled_3D + API_PalEnabled_Detail +
		API_PalEnabled_Worksheet + API_PalEnabled_Layout + API_PalEnabled_DocumentFrom3D,
		GSGuid2APIGuid (s_paletteGuid));
}

void APIDevKitExamples::PropertyRegistration::ExampleRunner::Register ()
{
	const ACAPI::Result<void> result = m_propertyOperationManager.RegisterAllProperties ();

	if (result.IsErr ())
	{
		m_registerStateText.SetTextColor (Gfx::Color::Red);
		m_registerStateText.SetText (result.UnwrapErr ().text.c_str ());
		ACAPI_WriteReport ("Error message: %s", false, result.UnwrapErr ().text.c_str ());
		return;
	}

	m_registerStateText.SetTextColor (Gfx::Color::Magenta);
	m_registerStateText.SetText ("Test property registered");
}


void APIDevKitExamples::PropertyRegistration::ExampleRunner::Unregister ()
{
	const ACAPI::Result<void> result = m_propertyOperationManager.UnregisterAllProperties ();

	if (result.IsErr ())
	{
		m_registerStateText.SetTextColor (Gfx::Color::Red);
		m_registerStateText.SetText (result.UnwrapErr ().text.c_str ());
		ACAPI_WriteReport ("Error message: %s", false, result.UnwrapErr ().text.c_str ());
		return;
	}

	m_registerStateText.ResetDefaultTextColor ();
	m_registerStateText.SetText ("Test property not registered");
}


/**********************************************************************************************************************/
