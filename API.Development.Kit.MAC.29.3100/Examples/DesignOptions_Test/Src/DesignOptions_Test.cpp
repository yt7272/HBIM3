#include "Resources.hpp"

#include <queue>
#include <map>
#include <string>

// from GS
#include "Overloaded.hpp"

// from DG
#include "DG.h"

// from UDLib
#include "UDTextInputDialog.hpp"

// from API
#include "APIEnvir.h"
#include "ACAPinc.h"
#include "APICommon.h"

// from ACAPI
#include "ACAPI/Result.hpp"
#include "ACAPI_Navigator.h"
#include "ACAPI/View.hpp"
#include "ACAPI/DesignOptionManager.hpp"

namespace {

using namespace ACAPI::DesignOptions;


GS::UniString GetTextFromUser (const GS::UniString& title)
{
	UD::TextInputDialog textDialog (title, GS::EmptyUniString, GS::EmptyUniString);
	if (textDialog.Invoke ()) {
		return textDialog.GetText ();
	}
	return GS::EmptyUniString;
}


ACAPI::Result<std::vector<API_NavigatorItem>> GetLeafNavigatorItems (API_NavigatorMapID mapID)
{
	Int32 i = 0;
	API_NavigatorSet set {};
	set.mapId = mapID;

	if (ACAPI_Navigator_GetNavigatorSet (&set, &i) != NoError) {
		return { ACAPI::Error (APIERR_GENERAL, "Failed to retrieve the list of items in the navigator."), ACAPI_GetToken () };
	}

	API_NavigatorItem rootItem {};
	rootItem.guid = set.rootGuid;
	rootItem.mapId = mapID;

	std::queue<API_NavigatorItem> itemsToProcess;
	itemsToProcess.emplace (rootItem);

	std::vector<API_NavigatorItem> result;

	while (!itemsToProcess.empty ()) {
		GS::Array<API_NavigatorItem> childItems;
		API_NavigatorItem parentItem = itemsToProcess.front ();
		itemsToProcess.pop ();
		GSErrCode err = ACAPI_Navigator_GetNavigatorChildrenItems (&parentItem, &childItems);
		if (err != NoError) {
			continue;
		}

		for (const auto& childItem : childItems) {
			if (childItem.itemType == API_FolderNavItem || childItem.itemType == API_UndefinedNavItem) {
				itemsToProcess.emplace (childItem);
			} else {
				result.emplace_back (childItem);
			}
		}
	}

	return ACAPI::Ok (result);
}

void DumpViewMapDesignOptionCombinations ()
{
	const ACAPI::Result<std::vector<API_NavigatorItem>> mapItems = GetLeafNavigatorItems (API_PublicViewMap);
	if (mapItems.IsErr ()) {
		WriteReport ("Failed to get navigator map items.");
		return;
	}

	// ! [Get-View-Design-Option-Combination-Example]

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	for (const API_NavigatorItem& item : mapItems.Unwrap ()) {
		const ACAPI::Result<ACAPI::ModelViews::View> view = ACAPI::ModelViews::FindViewByGuid (item.guid);
		if (view.IsErr ()) {
			continue;
		}

		const ACAPI::Result<ACAPI::DesignOptions::DesignOptionCombinationViewSettings> settings =
			manager->GetDesignOptionCombinationSettingsOf (view.Unwrap ());
		if (settings.IsErr ()) {
			continue;
		}

		const ACAPI::Result<ACAPI::DesignOptions::DesignOptionCombinationViewSettings::Status> status = settings->GetStatus ();
		if (status.IsErr ()) {
			continue;
		}

		const GS::UniString itemId (item.uiId);
		const GS::UniString itemIdPrefix = !itemId.IsEmpty () ? itemId + " " : GS::EmptyUniString;
		const GS::UniString itemName (item.uName);
		const GS::UniString fullName = itemIdPrefix + itemName;
		std::visit (
			GS::Overloaded {
				[&fullName] (const ACAPI::DesignOptions::DesignOptionCombinationViewSettings::MainModelOnly&) {
					WriteReport ("%s: Main Model Only.", fullName.ToCStr ().Get ());
				},
				[&fullName] (const ACAPI::DesignOptions::DesignOptionCombinationViewSettings::Standard& standard) {
					WriteReport ("%s: %s.", fullName.ToCStr ().Get (), standard.combination.GetName ().ToCStr ().Get ());
				},
				[&fullName] (const ACAPI::DesignOptions::DesignOptionCombinationViewSettings::Custom& /*custom*/) {
					WriteReport ("%s: Custom.", fullName.ToCStr ().Get ());
				},
				[&fullName] (const ACAPI::DesignOptions::DesignOptionCombinationViewSettings::Missing&) {
					WriteReport ("%s: Missing.", fullName.ToCStr ().Get ());
				},
			},
			status.Unwrap ()
		);
	}

	// ! [Get-View-Design-Option-Combination-Example]
}

void SetAllDesignOptionCombinationsToMainModelOnly ()
{
	// ! [Set-View-Design-Option-Combination-Example]

	const ACAPI::Result<std::vector<API_NavigatorItem>> mapItems = GetLeafNavigatorItems (API_PublicViewMap);
	if (mapItems.IsErr ()) {
		WriteReport ("Failed to get navigator map items.");
		return;
	}

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	for (const API_NavigatorItem& item : mapItems.Unwrap ()) {
		const ACAPI::Result<ACAPI::ModelViews::View> view = ACAPI::ModelViews::FindViewByGuid (item.guid);
		if (view.IsErr ()) {
			continue;
		}

		ACAPI::Result<ACAPI::DesignOptions::DesignOptionCombinationViewSettings> settings =
			manager->GetDesignOptionCombinationSettingsOf (view.Unwrap ());
		if (settings.IsErr ()) {
			continue;
		}

		ACAPI_CallUndoableCommand ("Undo Set Design Option Combinations To Main Model", [&] () -> GSErrCode {
			settings->SetMainModelOnly ();
			return NoError;
		});
	}

	// ! [Set-View-Design-Option-Combination-Example]
}

void DumpDesignOptionCombinations ()
{
	// ! [Get-Design-Option-Combinations-Example]

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	const auto combinations = manager->GetDesignOptionCombinations ();
	if (combinations.IsErr ()) {
		WriteReport ("Failed to get design option combinations.");
		return;
	}

	for (const auto& combination : combinations.Unwrap ()) {
		WriteReport ("%s (%s)", combination.GetName ().ToCStr ().Get (), combination.GetGuid ().ToUniString ().ToCStr ().Get ());
	}

	// ! [Get-Design-Option-Combinations-Example]
}

std::vector<API_Guid>	GetSelectedElements ()
{
	GSErrCode            err;
	API_SelectionInfo    selectionInfo;
	GS::Array<API_Neig>  selNeigs;

	err = ACAPI_Selection_Get (&selectionInfo, &selNeigs, false);
	BMKillHandle ((GSHandle*)&selectionInfo.marquee.coords);
	if (err == APIERR_NOSEL || selectionInfo.typeID == API_SelEmpty) {
		DGAlert (DG_ERROR, "Error", "Please select an element!", "", "Ok");
	}

	if (err != NoError) {
		return std::vector<API_Guid> ();
	}

	std::vector<API_Guid> guidArray;
	for (const API_Neig& neig : selNeigs) {
		guidArray.push_back (neig.guid);
	}

	return guidArray;
}

void DumpDesignOptionRelatedInfoOfElement_PropertyBased (const API_Guid& elemGuid)
{
	static const std::map<std::string, API_Guid> designOptionProperties {
				{"Design_Option_Name",			APIGuidFromString ("114FC56B-160D-4CCF-91F1-EAD505051C9A")},
				{"Design_Option_ID",			APIGuidFromString ("84DD2D3F-50DD-4BF3-A810-6CAB81C37540")},
				{"Design_Option_Set_Name",		APIGuidFromString ("59634BB2-6D8C-4A4B-B349-172F4157974B")},
				{"Design_Option_Is_Missing",	APIGuidFromString ("5CC582DF-EC76-4A09-AFB9-B97FADA4F2EC")},
				{"Design_Option_Status",		APIGuidFromString ("5C3FB768-C349-49E7-B6E6-7675A26A2AEC")}};

	WriteReport ("Design Option Properties for element %s, based on properties", APIGuidToString (elemGuid).ToCStr ().Get ());

	for (const auto& [name, id] : designOptionProperties) {
		GS::Array<API_Property> properties;
		GSErrCode err = ACAPI_Element_GetPropertyValuesByGuid (elemGuid, {id}, properties);
		if (err != NoError) {
			WriteReport ("Failed to get value for %s", name.c_str ());
			return;
		}
		GS::UniString valueInString;
		ACAPI_Property_GetPropertyValueString (properties.GetFirst (), &valueInString);
		WriteReport ("%s: %s", name.c_str (), valueInString.ToCStr ().Get ());
	}
}

void DumpDesignOptionRelatedInfoForSelectedElements_PropertyBased ()
{
	for (const API_Guid& elemGuid : GetSelectedElements ()) {
		DumpDesignOptionRelatedInfoOfElement_PropertyBased (elemGuid);
	}
}

void DumpDesignOptionRelatedInfoOfElement (const ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager>& manager, const API_Guid& elemGuid)
{
	// ! [Get-Design-Option-Status-Of-Example]
	const auto guidString = APIGuidToString (elemGuid);

	const auto elemDesignOptionStatus = manager->GetDesignOptionStatusOf (elemGuid);

	if (elemDesignOptionStatus.IsErr ()) {
		WriteReport ("Failed to get Design Option Status for Element with guid '%s.'", guidString.ToCStr ().Get ());
		return;
	}

	std::visit (
		GS::Overloaded {
			[&guidString] (const ACAPI::DesignOptions::DesignOption& designOption) {
				WriteReport ("Element with guid '%s': Design option name: %s, id: %s, Set name: %s",
							 guidString.ToCStr ().Get (),
							 designOption.GetName ().ToCStr ().Get (),
							 designOption.GetId ().ToCStr ().Get (),
							 designOption.GetOwnerSetName ().ToCStr ().Get ());
			},
			[&guidString] (const ACAPI::DesignOptions::MissingDesignOption&) {
				WriteReport ("Element with guid '%s' has Missing Design Option", guidString.ToCStr ().Get ());
			},
			[&guidString] (const ACAPI::DesignOptions::MainModelDesignOption&) {
				WriteReport ("Element with guid '%s' is in Main Model", guidString.ToCStr ().Get ());
			}
		},
		elemDesignOptionStatus.Unwrap ()
	);
	// ! [Get-Design-Option-Status-Of-Example]
}

void DumpDesignOptionRelatedInfoForSelectedElements ()
{
	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();

	for (const API_Guid& elemGuid : GetSelectedElements ()) {
		DumpDesignOptionRelatedInfoOfElement (manager, elemGuid);
	}
}

void ModifyDesignOptionsName ()
{
	// ! [Modify-Design-Options-Name-Example]

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOptionSet>> sets = manager->GetDesignOptionSets ();

	if (sets.IsErr()) {
		WriteReport ("Failed to get Design Option Sets.");
		return;
	}

	for (auto& set : sets.Unwrap ()) {
		ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOption>> options = manager->GetDesignOptionsOfSet (set);
		if (options.IsErr ()) {
			WriteReport ("Failed to get Design Options.");
			return;
		}

		for (auto& option : options.Unwrap ()) {
			auto err = ACAPI_CallUndoableCommand ("Undo Modify Design Option Name with API", [&option] () -> GSErrCode {
				GS::UniString newName = option.GetName () + " new phrase";
				if (option.SetName (newName).IsErr ()) {
					return (GSErrCode) GS::ErrorType::Error;
				}
				return NoError;
			});
			if (err) {
				WriteReport ("Failed to Modify Design Option Name.");
			}
		}

	}

	// ! [Modify-Design-Options-Name-Example]
}


void ModifyDesignOptionsId ()
{
	// ! [Modify-Design-Options-Id-Example]

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOptionSet>> sets = manager->GetDesignOptionSets ();

	if (sets.IsErr ()) {
		WriteReport ("Failed to get Design Option Sets.");
		return;
	}

	for (auto& set : sets.Unwrap ()) {
		ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOption>> options = manager->GetDesignOptionsOfSet (set);
		if (options.IsErr ()) {
			WriteReport ("Failed to get Design Options.");
			return;
		}

		for (auto& option : options.Unwrap ()) {
			auto err = ACAPI_CallUndoableCommand ("Undo Modify Design Option Id with API", [&option] () -> GSErrCode {
				GS::UniString newId = option.GetId () + " new phrase";
				if (option.SetId (newId).IsErr ()) {
					return (GSErrCode) GS::ErrorType::Error;
				}
				return NoError;
			});
			if (err) {
				WriteReport ("Failed to Modify Design Option Id.");
			}
		}

	}

	// ! [Modify-Design-Options-Id-Example]
}

void ModifyDesignOptionSetsName ()
{
	// ! [Modify-Design-Option-Sets-Example]

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOptionSet>> sets = manager->GetDesignOptionSets ();
	if (sets.IsErr ()) {
		WriteReport ("Failed to get Design Option Sets.");
		return;
	}

	for (auto& set : sets.Unwrap ()) {
		GS::UniString newName = set.GetName () + " new phrase";
		auto renameResult = ACAPI_CallUndoableCommand ("Undo rename Design Option sets with API", [&]() -> GSErrCode {
			if (set.SetName (newName).IsErr ()) {
				return (GSErrCode) GS::ErrorType::Error;
			}
			return (GSErrCode) NoError;
		});
		if (renameResult != NoError) {
			WriteReport ("Failed to rename Design Option Set.");
		}
	}

	// ! [Modify-Design-Option-Sets-Example]
}


void CreateDesignOption ()
{
	// ! [Create-Design-Option-Example]
	/*
	This function creates a design option, and appends it to the design options in the first set.
	*/
	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOptionSet>> setsResult = manager->GetDesignOptionSets ();
	if (!setsResult.IsOk ()) {
		WriteReport ("Failed to get Design Option Sets.");
		return;
	}

	auto apiSets = setsResult.Unwrap ();
	if (apiSets.empty ()) {
		WriteReport ("You have no design option set. First, you have to create one!");
		return;
	}

	std::optional<ACAPI::DesignOptions::DesignOption> prevOption;

	ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOption>> optionsResult = manager->GetDesignOptionsOfSet (apiSets.front ());
	if (optionsResult.IsOk ()) {
		auto apiOptions = optionsResult.Unwrap ();
		if (!apiOptions.empty ()) {
			prevOption = apiOptions.back ();
		}
	}

	ACAPI_CallUndoableCommand ("Undo Create Design Option", [&] () -> GSErrCode {
		auto createResult = manager->CreateDesignOption (apiSets.front (), prevOption, "TestDOName", "TestDOId");
		if (createResult.IsErr ()) {
			WriteReport (createResult.UnwrapErr ().text.c_str ());
			return createResult.UnwrapErr ().kind;
		}
		return NoError;
	});
	// ! [Create-Design-Option-Example]
}


void DeleteTheLastDesignOptionFromTheFirstNonEmptySet ()
{
	// ! [Delete-Design-Option-Example]
	/*
	This function deletes the last design option from the first non empty set.
	*/
	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOptionSet>> setsResult = manager->GetDesignOptionSets ();
	if (!setsResult.IsOk ()) {
		WriteReport ("Failed to get Design Option Sets.");
		return;
	}

	auto apiSets = setsResult.Unwrap ();
	if (apiSets.empty ()) {
		WriteReport ("You have no design option set. First, you have to create one!");
		return;
	}

	auto firstNonEmptySet = std::find_if (
		apiSets.begin (),
		apiSets.end (),
		[&manager] (const auto& apiSet) {
			ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOption>> optionsResult = manager->GetDesignOptionsOfSet (apiSet);
			return optionsResult.IsOk () && !optionsResult->empty ();
		}
	);

	if (firstNonEmptySet == apiSets.end ()) {
		WriteReport ("You have no design option. First, you have to create one!");
		return;
	}

	auto apiOption = manager->GetDesignOptionsOfSet (*firstNonEmptySet).Unwrap ().back ();

	ACAPI_CallUndoableCommand ("Undo Delete Design Option", [&] () -> GSErrCode {
		auto createResult = manager->DeleteDesignOption (apiOption);
		if (createResult.IsErr ()) {
			WriteReport (createResult.UnwrapErr ().text.c_str ());
			return createResult.UnwrapErr ().kind;
		}
		return NoError;
	});
	// ! [Delete-Design-Option-Example]
}


void MoveElemsToDesignOption (bool toMainModel) 
{
	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	auto designOptionSets = manager->GetDesignOptionSets ();

	if (designOptionSets.IsErr () || designOptionSets.Unwrap ().empty ()) {
		return;
	}

	std::vector<API_Guid> selection = GetSelectedElements ();

	// ! [Move-Elem-To-Design-Option-Example]

	ACAPI_CallUndoableCommand ("Undo Relink Element To Design Option", [&] () -> GSErrCode {
		for (auto& selectedElement : selection) {
			// ! [Can-Be-Moved-To-Other-Design-Option]
			if (manager->CanBeMovedToOtherDesignOption (selectedElement).IsErr ()) {
				return Error;
			}
			// ! [Can-Be-Moved-To-Other-Design-Option]

			if (toMainModel) {
				auto result = manager->RelinkElementToDesignOption (selectedElement, std::nullopt);
				if (result.IsErr ()) {
					return Error;
				}
			} else {
				auto designOptionsOfFirstSet = manager->GetDesignOptionsOfSet (designOptionSets.Unwrap ()[0]);
				if (designOptionsOfFirstSet.Unwrap ().empty ()) {
					return Error;
				}

				auto result = manager->RelinkElementToDesignOption (selectedElement, designOptionsOfFirstSet.Unwrap ()[0]);
				if (result.IsErr ()) {
					return Error;
				}
			}

		}
		return NoError;
	});
	// ! [Move-Elem-To-Design-Option-Example]
}


static GS::Optional<API_Element>	GetPlacedHotlinkByClickOnElement ()
{
	API_Element elem = {};
	API_Guid& guid = elem.header.guid;
	guid = APINULLGuid;

	if (!ClickAnElem ("Click on an element to select a hotlink instance", API_ZombieElemID, nullptr, nullptr, &guid)) {
		ACAPI_WriteReport ("No element was selected", true);
		return GS::NoValue;
	}

	ACAPI_Element_Get (&elem);
	if (elem.header.type.typeID == API_SectElemID) {
		guid = elem.sectElem.parentGuid;
	}

	API_Element hotlinkElem {};
	GSErrCode err = ACAPI_Hotlink_GetContainingHotlinkGuid (&guid, &hotlinkElem.header.guid);

	if (err != NoError || hotlinkElem.header.guid == APINULLGuid) {
		ACAPI_WriteReport ("The selected element is not part of any hotlink instance", true);
		return GS::NoValue;
	}

	err = ACAPI_Element_Get (&hotlinkElem);
	if (err != NoError) {
		return GS::NoValue;
	}

	return hotlinkElem;
}


void GetHotlinkDesignOptionTable () {
	// ! [Get-Hotlink-Design-Option-Table]
	API_HotlinkNode hotlinkNode = {};
	
	GS::Optional<API_Element> clickedHotlink = GetPlacedHotlinkByClickOnElement ();
	if (clickedHotlink.HasValue ()) {
		hotlinkNode.guid = clickedHotlink.Get ().hotlink.hotlinkNodeGuid;
	} else {
		WriteReport ("Failed to get Hotlinknode.");
		return;
	}

	GSErrCode err = ACAPI_Hotlink_GetHotlinkNode (&hotlinkNode);
	if (err != NoError)
		return;

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<ACAPI::DesignOptions::HotlinkDesignOptionTable> result = manager->GetHotlinkDesignOptionTable (hotlinkNode);
	if (result.IsErr ()) {
		WriteReport ("Failed to get Design Option Table of a hotlinknode.");
		return;
	}

	ACAPI::DesignOptions::HotlinkDesignOptionTable& hotlinkDesignOptionTable = result.Unwrap ();
	WriteReport ("Design option sets and options:");
	for (const GS::UniString& setName : hotlinkDesignOptionTable.GetSetsNames ()) {
		WriteReport ("%s:", setName.ToCStr ().Get ());
		for (const GS::UniString& optionName : hotlinkDesignOptionTable.GetOptionsNamesOfSet (setName)) {
			WriteReport ("	%s", optionName.ToCStr ().Get ());
		}
	}
	WriteReport ("Design option combinations:");
	for (const GS::UniString& combinationName : hotlinkDesignOptionTable.GetCombinationsNames ()) {
		WriteReport ("%s", combinationName.ToCStr ().Get ());
	}
	// ! [Get-Hotlink-Design-Option-Table]
}


void GetHotlinkInstanceDesignOptionTable () {
	// ! [Get-Hotlink-Instance-Design-Option-Table]
	API_HotlinkType hotlinkElem = {};

	GS::Optional<API_Element> clickedHotlink = GetPlacedHotlinkByClickOnElement ();
	if (clickedHotlink.HasValue ()) {
		hotlinkElem = clickedHotlink.Get ().hotlink;
	} else {
		WriteReport ("Failed to get Hotlink elem.");
		return;
	}

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<ACAPI::DesignOptions::HotlinkDesignOptionTable> result = manager->GetHotlinkInstanceDesignOptionTable (hotlinkElem);
	if (result.IsErr ()) {
		WriteReport ("Failed to get Design Option Table of a hotlink.");
		return;
	}

	ACAPI::DesignOptions::HotlinkDesignOptionTable& hotlinkDesignOptionTable = result.Unwrap ();
	WriteReport ("Design option sets and options:");
	for (const GS::UniString& setName : hotlinkDesignOptionTable.GetSetsNames ()) {
		WriteReport ("%s:", setName.ToCStr ().Get ());
		for (const GS::UniString& optionName : hotlinkDesignOptionTable.GetOptionsNamesOfSet (setName)) {
			WriteReport ("	%s", optionName.ToCStr ().Get ());
		}
	}
	WriteReport ("Design option combinations:");
	for (const GS::UniString& combinationName : hotlinkDesignOptionTable.GetCombinationsNames ()) {
		WriteReport ("%s", combinationName.ToCStr ().Get ());
	}

	switch (hotlinkDesignOptionTable.GetFilterMode ()) {
		case HotlinkDesignOptionTable::FilterMode::MainModelOnly:
		{
			WriteReport ("Filter setting: %s", "MainModelOnly");
			break;
		}
		case HotlinkDesignOptionTable::FilterMode::FilterByOptions:
		{
			WriteReport ("Filter setting: %s", "FilterByOptions");
			for (const auto& set : hotlinkDesignOptionTable.GetSetsNames ()) {
				std::optional<GS::UniString> activeInSet = hotlinkDesignOptionTable.GetActiveOptionInSet (set);
				if (activeInSet) {
					WriteReport ("%s: %s", set.ToCStr ().Get (), activeInSet.value ().ToCStr ().Get ());
				}
			}
			break;
		}
		case HotlinkDesignOptionTable::FilterMode::FilterByCombination:
		{
			WriteReport ("Filter setting: %s", "FilterByCombination");
			std::optional<GS::UniString> activeCombination = hotlinkDesignOptionTable.GetActiveCombination ();
			if (activeCombination) {
				WriteReport ("%s", activeCombination.value ().ToCStr ().Get ());
			}
			break;
		}
		default:
			break;
	}


	// ! [Get-Hotlink-Instance-Design-Option-Table]
}


void SetHotlinkDesignOptionTable () 
{
	// ! [Set-Hotlink-Design-Option-Table]
	API_HotlinkNode hotlinkNode = {};

	GS::Optional<API_Element> clickedHotlink = GetPlacedHotlinkByClickOnElement ();
	if (clickedHotlink.HasValue ()) {
		hotlinkNode.guid = clickedHotlink.Get ().hotlink.hotlinkNodeGuid;
	} else {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	GSErrCode err = ACAPI_Hotlink_GetHotlinkNode (&hotlinkNode);
	if (err != NoError)
		return;

	GS::Array<API_Guid> elementRefList;
	err = ACAPI_Hotlink_GetHotlinkInstances (&hotlinkNode.guid, &elementRefList);
	if (err != NoError || elementRefList.IsEmpty ())
		return;

	API_Element element {};
	element.header.guid = elementRefList[0];
	err = ACAPI_Element_Get (&element);
	if (err != NoError)
		return;
	
	API_HotlinkType hotlink = element.hotlink;

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<ACAPI::DesignOptions::HotlinkDesignOptionTable> getterResult = manager->GetHotlinkDesignOptionTable (hotlinkNode);
	if (getterResult.IsErr ()) {
		WriteReport ("Failed to get Design Option Table of a hotlink element.");
		return;
	}

	ACAPI::DesignOptions::HotlinkDesignOptionTable& hotlinkDesignOptionTable = getterResult.Unwrap ();

	std::vector<GS::UniString> setNames = hotlinkDesignOptionTable.GetSetsNames ();
	for (auto& setName : setNames) {
		std::vector<GS::UniString> optionNames = hotlinkDesignOptionTable.GetOptionsNamesOfSet (setName);
		if (!optionNames.empty ()) {
			hotlinkDesignOptionTable.ActivateOptionInSet (setName, optionNames[0]);
		}
	}
	hotlinkDesignOptionTable.SetFilterMode (ACAPI::DesignOptions::HotlinkDesignOptionTable::FilterMode::FilterByOptions);

	err = ACAPI_CallUndoableCommand ("Set Hotlink Design Option Table", [&] () -> GSErrCode {
		ACAPI::Result<void> res = manager->SetHotlinkDesignOptionTable (hotlink, hotlinkDesignOptionTable);
		return res.IsErr () ? Error : NoError;
	});

	if (err != NoError) {
		WriteReport ("Failed to set Design Option Table of a hotlink element.");
	}
	// ! [Set-Hotlink-Design-Option-Table]
	
}


void MoveTheFirstDesignOptionToTheEndOfTheLastSet ()
{
	// ! [Move-To-Other-Set-Example]
	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOptionSet>> setsResult = manager->GetDesignOptionSets ();
	if (!setsResult.IsOk ()) {
		WriteReport ("Failed to get Design Option Sets.");
		return;
	}

	auto apiSets = setsResult.Unwrap ();
	if (apiSets.empty ()) {
		WriteReport ("You have no design option set. First, you have to create one!");
		return;
	}

	auto firstNonEmptySet = std::find_if (
		apiSets.begin (),
		apiSets.end (),
		[&manager] (const auto& apiSet) {
			ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOption>> optionsResult = manager->GetDesignOptionsOfSet (apiSet);
			return optionsResult.IsOk () && !optionsResult->empty ();
		}
	);

	if (firstNonEmptySet == apiSets.end ()) {
		WriteReport ("You have no design option. First, you have to create one!");
		return;
	}

	auto apiOptionToMove = manager->GetDesignOptionsOfSet (*firstNonEmptySet).Unwrap ().front ();

	ACAPI_CallUndoableCommand ("Undo Move Design Option to Other Set", [&] () -> GSErrCode {
		auto moveResult = manager->MoveDesignOptionToOtherSet (apiOptionToMove, apiSets.back ());
		if (moveResult.IsErr ()) {
			WriteReport (moveResult.UnwrapErr ().text.c_str ());
			return moveResult.UnwrapErr ().kind;
		}
		return NoError;
	});
	// ! [Move-To-Other-Set-Example]
}


void MoveTheLastDesignOptionOfTheFirstSetToTheFront ()
{
	// ! [Move-Within-Set-Example]
	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOptionSet>> setsResult = manager->GetDesignOptionSets ();
	if (!setsResult.IsOk ()) {
		WriteReport ("Failed to get Design Option Sets.");
		return;
	}

	auto apiSets = setsResult.Unwrap ();
	if (apiSets.empty ()) {
		WriteReport ("You have no design option set. First, you have to create one!");
		return;
	}

	auto firstNonEmptySet = std::find_if (
		apiSets.begin (),
		apiSets.end (),
		[&manager] (const auto& apiSet) {
		ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOption>> optionsResult = manager->GetDesignOptionsOfSet (apiSet);
		return optionsResult.IsOk () && !optionsResult->empty ();
	}
	);

	if (firstNonEmptySet == apiSets.end ()) {
		WriteReport ("You have no design option. First, you have to create one!");
		return;
	}

	auto apiOptionToMove = manager->GetDesignOptionsOfSet (*firstNonEmptySet).Unwrap ().back ();
	auto firstApiOption = manager->GetDesignOptionsOfSet (*firstNonEmptySet).Unwrap ().front ();

	ACAPI_CallUndoableCommand ("Undo Move Design Option within Set", [&] () -> GSErrCode {
		auto moveResult = manager->MoveDesignOptionWithinSet (apiOptionToMove, firstApiOption);
		if (moveResult.IsErr ()) {
			WriteReport (moveResult.UnwrapErr ().text.c_str ());
			return moveResult.UnwrapErr ().kind;
		}
		return NoError;
	});
	// ! [Move-Within-Set-Example]
}

void CreateDesignOptionSet ()
{
	// ! [Create-Design-Option-Set-Example]
	/*
	This function creates a design option set, and appends it after the first set (if there is one).
	*/
	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOptionSet>> setsResult = manager->GetDesignOptionSets ();
	if (!setsResult.IsOk ()) {
		WriteReport ("Failed to get Design Option Sets.");
		return;
	}

	auto apiSets = setsResult.Unwrap ();
	if (apiSets.empty ()) {
		ACAPI_CallUndoableCommand ("Undo Create Design Option Set", [&] () -> GSErrCode {
			auto createResult = manager->CreateDesignOptionSet (std::nullopt, "TestDOSetNameFirst");
			if (createResult.IsErr ()) {
				WriteReport (createResult.UnwrapErr ().text.c_str ());
				return createResult.UnwrapErr ().kind;
			}
			return NoError;
		});
	} else {
		ACAPI_CallUndoableCommand ("Undo Create Design Option Set", [&] () -> GSErrCode {
			auto createResult = manager->CreateDesignOptionSet (apiSets.front (), "TestDOSetName");
			if (createResult.IsErr ()) {
				WriteReport (createResult.UnwrapErr ().text.c_str ());
				return createResult.UnwrapErr ().kind;
			}
			return NoError;
		});
	}
	// ! [Create-Design-Option-Set-Example]
}


void DeleteDesignOptionSet ()
{
	// ! [Delete-Design-Option-Set-Example]
	/*
	This function deletes the first empty design option set.
	*/
	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOptionSet>> setsResult = manager->GetDesignOptionSets ();
	if (!setsResult.IsOk ()) {
		WriteReport ("Failed to get Design Option Sets.");
		return;
	}

	auto apiSets = setsResult.Unwrap ();
	if (apiSets.size () == 0) {
		WriteReport ("You have no design option set. First, you have to create one!");
		return;
	}

	auto firstEmptySet = std::find_if (
		apiSets.begin (),
		apiSets.end (),
		[&manager] (const auto& apiSet) {
			ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOption>> optionsResult = manager->GetDesignOptionsOfSet (apiSet);
			return (optionsResult.IsOk () && optionsResult->empty ());
		}
	);

	if (firstEmptySet == apiSets.end ()) {
		WriteReport ("You have no empty design option set. First, you have to create one!");
		return;
	}

	ACAPI_CallUndoableCommand ("Undo Delete Design Option Set", [&] () -> GSErrCode {
		auto deleteResult = manager->DeleteDesignOptionSet (*firstEmptySet);
		if (deleteResult.IsErr ()) {
			WriteReport (deleteResult.UnwrapErr ().text.c_str ());
			return deleteResult.UnwrapErr ().kind;
		}
		return NoError;
	});
	// ! [Delete-Design-Option-Set-Example]
}


void MoveDesignOptionSet ()
{
	// ! [Move-Design-Option-Set-Example]

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOptionSet>> setsResult = manager->GetDesignOptionSets ();
	if (!setsResult.IsOk ()) {
		WriteReport ("Failed to get Get Available Design Option Sets.");
		return;
	}

	auto apiSets = setsResult.Unwrap ();
	if (apiSets.empty ()) {
		WriteReport ("You have no design option set. First, you have to create one!");
		return;
	}

	ACAPI_CallUndoableCommand ("Undo Move Design Option Set", [&] () -> GSErrCode {
		auto moveResult = manager->MoveDesignOptionSet (apiSets.front (), apiSets.back ());
		if (moveResult.IsErr ()) {
			WriteReport (moveResult.UnwrapErr ().text.c_str ());
			return moveResult.UnwrapErr ().kind;
		}
		return NoError;
	});
	// ! [Move-Design-Option-Set-Example]
}


enum class SetDesignOptionAsDefaultSelector {
	LastDO,
	MainModel
};


void SetDesignOptionAsDefault (SetDesignOptionAsDefaultSelector selector)
{
	// ! [Set-Design-Option-As-Default-Example]
	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	std::optional<ACAPI::DesignOptions::DesignOption> designOption;
	if (selector == SetDesignOptionAsDefaultSelector::LastDO) {
		const auto options = manager->GetAllDesignOptions ();
		if (options.IsErr ()) {
			WriteReport ("Failed to get design options.");
			return;
		}

		if (options.Unwrap ().empty ()) {
			WriteReport ("There are no design options.");
			return;
		}

		designOption = options.Unwrap ().back ();
	}

	ACAPI_CallUndoableCommand ("Set Design Option As Default", [&] () -> GSErrCode {
		auto createResult = manager->SetDesignOptionAsDefault (designOption);
		if (createResult.IsErr ()) {
			WriteReport (createResult.UnwrapErr ().text.c_str ());
			return createResult.UnwrapErr ().kind;
		}
		return NoError;
	});
	// ! [Set-Design-Option-As-Default-Example]
}


void DumpDesignOptionsOfFirstSet ()
{
	// ! [Get-Design-Options-Of-Set-Example]

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	const auto sets = manager->GetDesignOptionSets ();
	if (sets.IsErr ()) {
		WriteReport ("Failed to get design option sets.");
		return;
	}

	if (sets.Unwrap ().empty ()) {
		WriteReport ("There are no design option sets.");
		return;
	}

	const auto set = sets.Unwrap ()[0];
	const auto options = manager->GetDesignOptionsOfSet (set);
	if (options.IsErr ()) {
		WriteReport ("Failed to get design options of first set.");
		return;
	}

	WriteReport ("Design options of set %s:", set.GetName ().ToCStr ().Get ());
	for (const auto& option : options.Unwrap ()) {
		WriteReport ("%s (%s)", option.GetName ().ToCStr ().Get (), option.GetGuid ().ToUniString ().ToCStr ().Get ());
	}

	// ! [Get-Design-Options-Of-Set-Example]
}


void DumpDesignOptionSets ()
{
	// ! [Get-Design-Option-Sets-Example]

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	const auto sets = manager->GetDesignOptionSets ();
	if (sets.IsErr ()) {
		WriteReport ("Failed to get design option sets.");
		return;
	}

	if (sets.Unwrap ().empty ()) {
		WriteReport ("There are no design option sets.");
		return;
	}

	for (const auto& set : sets.Unwrap ()) {
		WriteReport ("%s (%s)", set.GetName ().ToCStr ().Get (), set.GetGuid ().ToUniString ().ToCStr ().Get ());
	}

	// ! [Get-Design-Option-Sets-Example]
}


void DumpAllDesignOptions ()
{
	// ! [Get-All-Design-Options-Example]

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	const auto options = manager->GetAllDesignOptions ();
	if (options.IsErr ()) {
		WriteReport ("Failed to get design options.");
		return;
	}

	if (options.Unwrap ().empty ()) {
		WriteReport ("There are no design options.");
		return;
	}

	for (const auto& option : options.Unwrap ()) {
		WriteReport ("%s (%s)", option.GetName ().ToCStr ().Get (), option.GetGuid ().ToUniString ().ToCStr ().Get ());
	}

	// ! [Get-All-Design-Options-Example]
}


void DumpActiveDesignOptionsOfFirstCombination ()
{
	// ! [Get-Active-Design-Options-Of-Combination-Example]

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	const auto combinations = manager->GetDesignOptionCombinations ();
	if (combinations.IsErr ()) {
		WriteReport ("Failed to get design option combinations.");
		return;
	}

	if (combinations.Unwrap ().empty ()) {
		WriteReport ("There are no design option combinations.");
		return;
	}

	const auto combination = combinations.Unwrap ()[0];
	const auto options = manager->GetActiveDesignOptionsOfCombination (combination);
	if (options.IsErr ()) {
		WriteReport ("Failed to get active design options of first combination.");
		return;
	}

	WriteReport ("Active options of combination %s:", combination.GetName ().ToCStr ().Get ());
	for (const auto& option : options.Unwrap ()) {
		WriteReport ("%s (%s)", option.GetName ().ToCStr ().Get (), option.GetGuid ().ToUniString ().ToCStr ().Get ());
	}

	// ! [Get-Active-Design-Options-Of-Combination-Example]
}


void DumpElementsOfFirstDesignOption ()
{
	// ! [Get-Elements-Of-Design-Option-Example]

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	const auto options = manager->GetAllDesignOptions ();
	if (options.IsErr ()) {
		WriteReport ("Failed to get design options.");
		return;
	}

	if (options.Unwrap ().empty ()) {
		WriteReport ("There are no design options.");
		return;
	}

	const auto option = options.Unwrap ()[0];
	const auto elements = manager->GetElementsOfDesignOption (option);
	if (elements.IsErr ()) {
		WriteReport ("Failed to get elements linked to the first design option.");
		return;
	}

	WriteReport ("Elements of design option %s:", option.GetName ().ToCStr ().Get ());
	for (const auto& elemGuid : elements.Unwrap ()) {
		GS::Array<API_Property> properties;
		GS::Array<API_Guid> propertyGuids = {APIGuidFromString ("7E221F33-829B-4FBC-A670-E74DABCE6289")}; //guid of Property::GeneralElementIDPropertyId
		GSErrCode err = ACAPI_Element_GetPropertyValuesByGuid (elemGuid, propertyGuids, properties);
		if (err != NoError) {
			WriteReport ("Failed to get value for %s", APIGuidToString (elemGuid).ToCStr ().Get ());
			return;
		}
		GS::UniString valueInString;
		ACAPI_Property_GetPropertyValueString (properties.GetFirst (), &valueInString);
		WriteReport ("%s (%s)", valueInString.ToCStr ().Get (), APIGuidToString (elemGuid).ToCStr ().Get ());
	}

	// ! [Get-Elements-Of-Design-Option-Example]
}


void CreateDesignOptionCombination ()
{
	// ! [Create-Design-Option-Combination-Example]
	/*
	This function creates a design option combination.
	*/
	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI_CallUndoableCommand ("Undo Create Design Option Combination", [&] () -> GSErrCode {
		auto createResult = manager->CreateDesignOptionCombination ("TestDOCombination");
		if (createResult.IsErr ()) {
			WriteReport (createResult.UnwrapErr ().text.c_str ());
			return createResult.UnwrapErr ().kind;
		}
		return NoError;
	});
	// ! [Create-Design-Option-Combination-Example]
}


void DeleteDesignOptionCombination ()
{
	// ! [Delete-Design-Option-Combination-Example]
	/*
	This function deletes a design option combination.
	*/
	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOptionCombination>> combinationsResult = manager->GetDesignOptionCombinations ();
	if (!combinationsResult.IsOk ()) {
		WriteReport ("Failed to get Design Option Combinations.");
		return;
	}

	auto combinations = combinationsResult.Unwrap ();
	if (combinations.empty ()) {
		WriteReport ("You have no design option combinations.");
		return;
	}

	auto combinationName = GetTextFromUser ("Select a combination");

	auto combinationToDelete = std::find_if (
		combinations.begin (),
		combinations.end (),
		[&combinationName] (const auto& combination) {
			return combination.GetName () == combinationName;
		}
	);

	if (combinationToDelete == combinations.end ()) {
		WriteReport ("The given design option combination does not exist.");
		return;
	}

	auto err = ACAPI_CallUndoableCommand ("Undo Delete Design Option Combination", [&] () -> GSErrCode {
		auto deleteResult = manager->DeleteDesignOptionCombination (*combinationToDelete);
		if (deleteResult.IsErr ()) {
			WriteReport (deleteResult.UnwrapErr ().text.c_str ());
			return deleteResult.UnwrapErr ().kind;
		}
		return NoError;
	});
	if (err) {
		WriteReport ("Failed to delete design option combination.");
		return;
	}

	// ! [Delete-Design-Option-Combination-Example]
}


void ModifyDesignOptionsCombinationsName ()
{
	// ! [Modify-Design-Options-Combinations-Name-Example]
	/*
	This function adds a new phrase to all design option combinations' names.
	*/
	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	ACAPI::Result<std::vector<ACAPI::DesignOptions::DesignOptionCombination>> combinationsResult = manager->GetDesignOptionCombinations ();
	if (!combinationsResult.IsOk ()) {
		WriteReport ("Failed to get Design Option Combinations.");
		return;
	}

	for (auto& combination : combinationsResult.Unwrap ()) {
		auto err = ACAPI_CallUndoableCommand ("Undo Modify Design Option Combination Name", [&combination] () -> GSErrCode {
			GS::UniString newName = combination.GetName () + " new phrase";
			if (combination.SetName (newName).IsErr ()) {
				return (GSErrCode) GS::ErrorType::Error;
			}
			return NoError;
		});
		if (err) {
			WriteReport ("Failed to Modify Design Option Combination Name.");
		}
	}
	// ! [Modify-Design-Options-Combinations-Name-Example]
}


void ActivateFirstOptionOfEverySetInCombination ()
{
	// ! [Activate-First-Option-Of-Every-Set-In-Combination-Example]
	/*
	This function activates the first design option of every set in a combination.
	*/
	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	auto combinationsResult = manager->GetDesignOptionCombinations ();
	if (!combinationsResult.IsOk ()) {
		WriteReport ("Failed to get Design Option Combinations.");
		return;
	}

	auto combinations = combinationsResult.Unwrap ();
	if (combinations.empty ()) {
		WriteReport ("You have no Design Option Combinations.");
		return;
	}

	auto setsResult = manager->GetDesignOptionSets ();
	if (!setsResult.IsOk ()) {
		WriteReport ("Failed to get Design Option Sets.");
		return;
	}

	auto sets = setsResult.Unwrap ();
	if (sets.empty ()) {
		WriteReport ("You have no Design Option Sets.");
		return;
	}

	auto combinationName = GetTextFromUser ("Select a combination");

	auto combinationToModify = std::find_if (
		combinations.begin (),
		combinations.end (),
		[&combinationName] (const auto& combination) {
			return combination.GetName () == combinationName;
		}
	);
	
	if (combinationToModify == combinations.end ()) {
		WriteReport ("The given design option combination does not exist.");
		return;
	}

	for (auto& set : sets) {
		auto optionsResult = manager->GetDesignOptionsOfSet (set);
		if (!optionsResult.IsOk ()) {
			WriteReport ("Failed to get Design Options.");
			return;
		}
		auto options = optionsResult.Unwrap ();
		if (!options.empty ()) {
			auto err = ACAPI_CallUndoableCommand ("Activate design option in combination", [&] () -> GSErrCode {
				auto result = manager->ActivateDesignOptionInCombination (*combinationToModify, options[0]);
				if (result.IsErr ()) {
					WriteReport (result.UnwrapErr ().text.c_str ());
					return result.UnwrapErr ().kind;
				}
				return NoError;
			});
			if (err) {
				WriteReport ("Failed to activate design option.");
			}
		}
	}
	// ! [Activate-First-Option-Of-Every-Set-In-Combination-Example]
}


void DeactivateEveryOptionInCombination ()
{
	// ! [Deactivate-Every-Option-In-Combination-Example]
	/*
	This function deactivates every design option in a combination.
	*/
	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	auto combinationsResult = manager->GetDesignOptionCombinations ();
	if (!combinationsResult.IsOk ()) {
		WriteReport ("Failed to get Design Option Combinations.");
		return;
	}

	auto combinations = combinationsResult.Unwrap ();
	if (combinations.empty ()) {
		WriteReport ("You have no Design Option Combinations.");
		return;
	}

	auto combinationName = GetTextFromUser ("Select a combination");

	auto combinationToModify = std::find_if (
		combinations.begin (),
		combinations.end (),
		[&combinationName] (const auto& combination) {
			return combination.GetName () == combinationName;
		}
	);

	if (combinationToModify == combinations.end ()) {
		WriteReport ("The given design option combination does not exist.");
		return;
	}

	ACAPI_CallUndoableCommand ("Deactivate every design option in combination", [&] () -> GSErrCode {
		auto result = manager->SetActiveOptionsInCombination (*combinationToModify, {});
		if (result.IsErr ()) {
			WriteReport (result.UnwrapErr ().text.c_str ());
			return result.UnwrapErr ().kind;
		}
		return NoError;
	});

	// ! [Deactivate-Every-Option-In-Combination-Example]
}


void DuplicateDesignOptionCombination ()
{
	// ! [Duplicate-Design-Option-Combination-Example]
	/*
	This function duplicates a design option combination.
	*/

	ACAPI::Result<ACAPI::DesignOptions::DesignOptionManager> manager = CreateDesignOptionManager ();
	if (manager.IsErr ()) {
		WriteReport ("Failed to get Design Option Manager.");
		return;
	}

	auto combinationsResult = manager->GetDesignOptionCombinations ();
	if (!combinationsResult.IsOk ()) {
		WriteReport ("Failed to get Design Option Combinations.");
		return;
	}

	auto combinations = combinationsResult.Unwrap ();
	if (combinations.empty ()) {
		WriteReport ("You have no Design Option Combinations.");
		return;
	}

	auto combinationName = GetTextFromUser ("Select a combination");

	auto combinationToDuplicate = std::find_if (
		combinations.begin (),
		combinations.end (),
		[&combinationName] (const auto& combination) {
			return combination.GetName () == combinationName;
		}
	);

	if (combinationToDuplicate == combinations.end ()) {
		WriteReport ("The given design option combination does not exist.");
		return;
	}

	ACAPI_CallUndoableCommand ("Duplicate design option combination", [&] () -> GSErrCode {
		auto result = manager->DuplicateDesignOptionCombination (*combinationToDuplicate);
		if (result.IsErr ()) {
			WriteReport (result.UnwrapErr ().text.c_str ());
			return result.UnwrapErr ().kind;
		}
		return NoError;
	});

	// ! [Duplicate-Design-Option-Combination-Example]
}


void GetVisibleWallsByDesignOptions ()
{	
	GS::Array<API_Guid> elemGuidList;
	ACAPI_Element_GetElemList (API_WallID, &elemGuidList, APIFilt_IsVisibleByDesignOption);

	if (elemGuidList.IsEmpty ()) {
		WriteReport ("No walls are visible based on Design Option settings");
		return;
	}

	WriteReport ("Walls that are visible based on Design Option settings:");
	for (const API_Guid elemGuid : elemGuidList) {
		WriteReport ("%s", APIGuidToString (elemGuid).ToCStr ().Get ());
	}
}


}


// -----------------------------------------------------------------------------
// MenuCommandHandler
//		called to perform the user-asked command
// -----------------------------------------------------------------------------

GSErrCode MenuCommandHandler (const API_MenuParams* menuParams)
{
	PRECOND (menuParams->menuItemRef.menuResID == DESIGNOPTIONS_TEST_MENU_STRINGS);

	switch (menuParams->menuItemRef.itemIndex) {
		case DumpViewMapDesignOptionCombinationsID:
			DumpViewMapDesignOptionCombinations ();
			break;

		case SetAllDesignOptionCombinationsToMainModelOnlyID:
			SetAllDesignOptionCombinationsToMainModelOnly ();
			break;

		case DumpDesignOptionCombinationsID:
			DumpDesignOptionCombinations ();
			break;

		case DumpDesignOptionRelatedInfoForSelectedElementsPropertyBasedID:
			DumpDesignOptionRelatedInfoForSelectedElements_PropertyBased ();
			break;

		case DumpDesignOptionRelatedInfoForSelectedElementsID:
			DumpDesignOptionRelatedInfoForSelectedElements ();
			break;

		case ModifyDesignOptionsNameID:
			ModifyDesignOptionsName ();
			break;

		case ModifyDesignOptionSetsNameID:
			ModifyDesignOptionSetsName ();
			break;

		case CreateDesignOptionID:
			CreateDesignOption ();
			break;

		case DeleteTheLastDesignOptionFromTheFirstNonEmptySetID:
			DeleteTheLastDesignOptionFromTheFirstNonEmptySet ();
			break;

		case MoveElemsToDesignOptionID:
			MoveElemsToDesignOption (false);
			break;

		case MoveElemsToMainModelID:
			MoveElemsToDesignOption (true);
			break;

		case GetHotlinkDesignOptionTableID:
			GetHotlinkDesignOptionTable ();
			break;

		case GetHotlinkInstanceDesignOptionTableID:
			GetHotlinkInstanceDesignOptionTable ();
			break;

		case MoveTheFirstDesignOptionToTheEndOfTheLastSetID:
			MoveTheFirstDesignOptionToTheEndOfTheLastSet ();
			break;

		case MoveTheLastDesignOptionOfTheFirstSetToTheFrontID:
			MoveTheLastDesignOptionOfTheFirstSetToTheFront ();
			break;

		case CreateDesignOptionSetID:
			CreateDesignOptionSet ();
			break;

		case DumpDesignOptionsOfFirstSetID:
			DumpDesignOptionsOfFirstSet ();
			break;

		case DumpDesignOptionSetsID:
			DumpDesignOptionSets ();
			break;

		case DumpAllDesignOptionsID:
			DumpAllDesignOptions ();
			break;

		case DumpActiveDesignOptionsOfFirstCombinationID:
			DumpActiveDesignOptionsOfFirstCombination ();
			break;

		case DumpElementsOfFirstDesignOptionID:
			DumpElementsOfFirstDesignOption ();
			break;

		case SetHotlinkDesignOptionTableID:
			SetHotlinkDesignOptionTable ();
			break;

		case MoveDesignOptionSetID:
			MoveDesignOptionSet ();
			break;

		case DeleteDesignOptionSetID:
			DeleteDesignOptionSet ();
			break;

		case SetLastDesignOptionAsDefaultID:
			SetDesignOptionAsDefault (SetDesignOptionAsDefaultSelector::LastDO);
			break;

		case SetMainModelAsDefaultID:
			SetDesignOptionAsDefault (SetDesignOptionAsDefaultSelector::MainModel);
			break;

		case ModifyDesignOptionsIdID:
			ModifyDesignOptionsId ();
			break;

		case CreateDesignOptionCombinationID:
			CreateDesignOptionCombination ();
			break;

		case DeleteDesignOptionCombinationID:
			DeleteDesignOptionCombination ();
			break;

		case ModifyDesignOptionsCombinationsNameID:
			ModifyDesignOptionsCombinationsName ();
			break;

		case ActivateFirstOptionOfEverySetInCombinationID:
			ActivateFirstOptionOfEverySetInCombination ();
			break;

		case DeactivateEveryOptionInCombinationID:
			DeactivateEveryOptionInCombination ();
			break;

		case DuplicateDesignOptionCombinationID:
			DuplicateDesignOptionCombination ();
			break;

		case GetVisibleWallsByDesignOptionsID:
			GetVisibleWallsByDesignOptions ();
			break;

		default:
			GS::Unreachable ();

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
	RSGetIndString (&envir->addOnInfo.name,        DESIGNOPTIONS_TEST_ADDON_NAME, 1, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, DESIGNOPTIONS_TEST_ADDON_NAME, 2, ACAPI_GetOwnResModule ());

	return APIAddon_Normal;
}


// -----------------------------------------------------------------------------
// Interface definitions
// -----------------------------------------------------------------------------

GSErrCode RegisterInterface ()
{
	GSErrCode err = ACAPI_MenuItem_RegisterMenu (DESIGNOPTIONS_TEST_MENU_STRINGS, DESIGNOPTIONS_TEST_MENU_PROMPT_STRINGS, MenuCode_UserDef, MenuFlag_Default);
	DBASSERT (err == NoError);
	return err;
}


// -----------------------------------------------------------------------------
// Initialize
//		called after the Add-On has been loaded into memory
// -----------------------------------------------------------------------------

GSErrCode Initialize ()
{
	GSErrCode err = ACAPI_MenuItem_InstallMenuHandler (DESIGNOPTIONS_TEST_MENU_STRINGS, MenuCommandHandler);
	DBASSERT (err == NoError);
	ACAPI_KeepInMemory (true);
	return err;
}


// -----------------------------------------------------------------------------
// FreeData
//		called when the Add-On is going to be unloaded
// -----------------------------------------------------------------------------

GSErrCode FreeData ()
{
	return NoError;
}
