// *****************************************************************************
// Source code for the Opening Test Add-On
// *****************************************************************************

// ---------------------------------- Includes ---------------------------------

#include <vector>

#include	"Opening_Functions.hpp"

#include	"APIEnvir.h"
#include	"ACAPinc.h"					// also includes APIdefs.h
#include	"APICommon.h"
#include	"ACAPI/Favorite.hpp"	
#include	"ACAPI/Element/Opening/OpeningDefault.hpp"
#include	"ACAPI/Element/Opening/Opening.hpp"
#include	"ACAPI/Element/Opening/OpeningGeometry.hpp"
#include	"ACAPI/Element/Opening/OpeningFloorPlanParameters.hpp"
#include	"ACAPI/Element/Opening/OpeningExtrusionParameters.hpp"

#include "Polygon2D.hpp"

#include "DGFileDialog.hpp"
#include "UDTextInputDialog.hpp"


GS::UniString GetTextFromUser (const GS::UniString& title)
{
	UD::TextInputDialog textDialog (title, GS::EmptyUniString, GS::EmptyUniString);
	if (textDialog.Invoke ()) {
		return textDialog.GetText ();
	}
	return GS::EmptyUniString;
}


// =============================================================================
//
// Opening functions
//
// =============================================================================

// -----------------------------------------------------------------------------
// Places an Opening into a clicked element
// -----------------------------------------------------------------------------

GSErrCode	PlaceOpeningIntoAnElement ()
{
	API_Guid	elemGuid {};
	API_Coord3D	clickPointFirst {};
	API_Coord3D	clickPointSecond {};

	if (!ClickAnElem ("Click an element to place an opening in it!", API_ZombieElemID, nullptr, nullptr, &elemGuid, &clickPointFirst)) {
		WriteReport_Alert ("Please click any element!");
		return Error;
	}

	if (!ClickAnElem ("Click an element to specify the position of the opening!", API_ZombieElemID, nullptr, nullptr, nullptr, &clickPointSecond)) {
		WriteReport_Alert ("Please click any element!");
		return Error;
	}

	GS::UniString openingName = GetTextFromUser ("Enter the name of the opening favorite");
	auto favoriteManager = ACAPI::Favorite::GetFavoriteManager ();
	ACAPI::Result<ACAPI::Favorite::Favorite> favorite = favoriteManager.GetFavoriteByName (openingName);
	if (favorite.IsErr ()) {
		WriteReport_Alert (favorite.UnwrapErr ().text.c_str ());
		return favorite.UnwrapErr ().kind;
	}

	ACAPI::Result<ACAPI::Element::OpeningDefault> openingDefault = ACAPI::Element::CreateOpeningDefault (*favorite->GetElementDefault ());
	if (openingDefault.IsErr ()) {
		WriteReport_Alert (openingDefault.UnwrapErr ().text.c_str ());
		return openingDefault.UnwrapErr ().kind;
	}

	ACAPI::UniqueID parentElemId (elemGuid, ACAPI_GetToken ());
	return ACAPI_CallUndoableCommand ("Place Opening Test API Function", [&] () -> GSErrCode {
		ACAPI::Result<ACAPI::UniqueID> resultId = openingDefault->Place (parentElemId, clickPointSecond);

		if (resultId.IsErr ()) {
			ErrorBeep ("Place opening", resultId.UnwrapErr ().kind);
			return resultId.UnwrapErr ().kind;
		}

		return NoError;
	});
}


// -----------------------------------------------------------------------------
// Places a triangular Opening into a clicked element
// -----------------------------------------------------------------------------

GSErrCode PlacePolygonalOpeningIntoAnElement ()
{
	API_Guid	elemGuid {};
	API_Coord3D	clickPointFirst {};
	API_Coord3D	clickPointSecond {};

	if (!ClickAnElem ("Click an element to place an opening in it!", API_ZombieElemID, nullptr, nullptr, &elemGuid, &clickPointFirst)) {
		WriteReport_Alert ("Please click any element!");
		return Error;
	}

	if (!ClickAnElem ("Click an element to specify the position of the opening!", API_ZombieElemID, nullptr, nullptr, nullptr, &clickPointSecond)) {
		WriteReport_Alert ("Please click any element!");
		return Error;
	}

	ACAPI::Result<ACAPI::Element::OpeningDefault> openingDefault = ACAPI::Element::CreateOpeningDefault ();
	if (openingDefault.IsErr ()) {
		return openingDefault.UnwrapErr ().kind;
	}

	GS::Array<Point2D> polygonCorners { {-1, -1}, {1, -1}, {0, 1} };
	Geometry::Polygon2D triangle = Geometry::Polygon2D::Create (polygonCorners, 0 /*Geometry::PolyCreateFlags*/).PopLargest ();

	ACAPI::UniqueID parentElemId (APIGuid2GSGuid (elemGuid), ACAPI_GetToken ());
	return ACAPI_CallUndoableCommand ("Place Opening Test API Function", [&] () -> GSErrCode {
		ACAPI::Result<ACAPI::UniqueID> resultId = openingDefault->PlacePolygonal (parentElemId, clickPointSecond, triangle);

		if (resultId.IsErr ()) {
			ErrorBeep ("Place opening", resultId.UnwrapErr ().kind);
			return resultId.UnwrapErr ().kind;
		}

		return NoError;
	});
}


// -----------------------------------------------------------------------------
// Change the parent element of an opening
// -----------------------------------------------------------------------------

GSErrCode	ChangeOpeningsParent ()
{
	API_Guid	openingGuid {};
	API_Guid	newParentGuid {};
	API_Coord3D	clickPointFirst {};
	API_Coord3D	clickPointSecond {};

	if (!ClickAnElem ("Click an opening to change its parent element!", API_OpeningID, nullptr, nullptr, &openingGuid, &clickPointFirst)) {
		WriteReport_Alert ("No opening was clicked!");
		return Error;
	}

	if (!ClickAnElem ("Click the element you wish to be the opening's parent!", API_ZombieElemID, nullptr, nullptr, &newParentGuid, &clickPointSecond)) {
		WriteReport_Alert ("Please click an element the opening can connect to!");
		return Error;
	}

	ACAPI::Result<ACAPI::Element::Opening> opening = ACAPI::Element::Opening::Get ({openingGuid, ACAPI_GetToken ()});
	if (opening.IsErr ()) {
		return opening.UnwrapErr ().kind;
	}

	GSErrCode changeParentResultCode = NoError;
	ACAPI::Result<void> modifyResult = opening->Modify ([&] (ACAPI::Element::Opening::Modifier& modifier) {
		ACAPI::Result<void> changeParentResult = modifier.ChangeParentElement ({newParentGuid, ACAPI_GetToken ()});
		if (changeParentResult.IsErr ()) {
			changeParentResultCode = changeParentResult.UnwrapErr ().kind;
		}
	}, "Change Parent Element Of Opening");

	if (modifyResult.IsErr ()) {
		return modifyResult.UnwrapErr ().kind;
	}

	return changeParentResultCode;
}


// -----------------------------------------------------------------------------
// Connect opening to another element
// -----------------------------------------------------------------------------


GSErrCode	ConnectOpeningToAnotherElement ()
{
	API_Guid	openingGuid {};
	API_Guid	elementToConnectGuid {};
	API_Coord3D	clickPointFirst {};
	API_Coord3D	clickPointSecond {};

	if (!ClickAnElem ("Click an opening you wish to connect to another element!", API_OpeningID, nullptr, nullptr, &openingGuid, &clickPointFirst)) {
		WriteReport_Alert ("No opening was clicked!");
		return Error;
	}

	if (!ClickAnElem ("Click an element you wish to connect the opening to!", API_ZombieElemID, nullptr, nullptr, &elementToConnectGuid, &clickPointSecond)) {
		WriteReport_Alert ("Please click an element the opening can connect to!");
		return Error;
	}

	ACAPI::Result<ACAPI::Element::Opening> opening = ACAPI::Element::Opening::Get ({openingGuid, ACAPI_GetToken ()});
	if (opening.IsErr ()) {
		return opening.UnwrapErr ().kind;
	}

	GSErrCode connectOpeningResultCode = NoError;
	ACAPI::Result<void> modifyResult = opening->Modify ([&] (ACAPI::Element::Opening::Modifier& modifier) {
		ACAPI::Result<void> connectOpeningResult = modifier.AddCutElement ({elementToConnectGuid, ACAPI_GetToken ()});
		if (connectOpeningResult.IsErr ()) {
			connectOpeningResultCode = connectOpeningResult.UnwrapErr ().kind;
		}
	}, "Connect Opening to another element");

	if (modifyResult.IsErr ()) {
		return modifyResult.UnwrapErr ().kind;
	}

	return connectOpeningResultCode;
}


// -----------------------------------------------------------------------------
// Disconnect opening from another element
// -----------------------------------------------------------------------------


GSErrCode	DisconnectOpeningFromAnotherElement ()
{
	API_Guid	openingGuid {};
	API_Guid	elementToDisconnectGuid {};
	API_Coord3D	clickPointFirst {};
	API_Coord3D	clickPointSecond {};

	if (!ClickAnElem ("Click an opening you wish to disconnect from another element!", API_OpeningID, nullptr, nullptr, &openingGuid, &clickPointFirst)) {
		WriteReport_Alert ("No opening was clicked!");
		return Error;
	}

	if (!ClickAnElem ("Click the element you wish to disconnect from the opening!", API_ZombieElemID, nullptr, nullptr, &elementToDisconnectGuid, &clickPointSecond)) {
		WriteReport_Alert ("Please click an element the opening can connect to!");
		return Error;
	}

	ACAPI::Result<ACAPI::Element::Opening> opening = ACAPI::Element::Opening::Get ({openingGuid, ACAPI_GetToken ()});
	if (opening.IsErr ()) {
		return opening.UnwrapErr ().kind;
	}

	GSErrCode disconnectOpeningResultCode = NoError;
	ACAPI::Result<void> modifyResult = opening->Modify ([&] (ACAPI::Element::Opening::Modifier& modifier) {
		ACAPI::Result<void> disconnectOpeningResult = modifier.RemoveCutElement ({elementToDisconnectGuid, ACAPI_GetToken ()});
		if (disconnectOpeningResult.IsErr ()) {
			disconnectOpeningResultCode = disconnectOpeningResult.UnwrapErr ().kind;
		}
	}, "Connect Opening to another element");

	if (modifyResult.IsErr ()) {
		return modifyResult.UnwrapErr ().kind;
	}

	return disconnectOpeningResultCode;
}


// -----------------------------------------------------------------------------
// Select Opening(s) connected to the clicked element
// -----------------------------------------------------------------------------


GSErrCode	SelectOpeningsConnectedToElement ()
{
	API_Guid	elemGuid {};
	API_Coord3D	clickPointFirst {};

	if (!ClickAnElem ("Click an element!", API_ZombieElemID, nullptr, nullptr, &elemGuid, &clickPointFirst)) {
		WriteReport_Alert ("Please click any element!");
		return Error;
	}

	ACAPI::UniqueID clickedElemId (elemGuid, ACAPI_GetToken ());

	ACAPI::Result<std::vector<ACAPI::UniqueID>> connectedOpenings = ACAPI::Element::Opening::GetConnectedOpenings (clickedElemId, ACAPI_GetToken ());

	if (connectedOpenings.IsErr ()) {
		return connectedOpenings.UnwrapErr ().kind;
	}

	GS::Array<API_Neig> connectedOpeningsToSelect;
	for (const ACAPI::UniqueID& id : connectedOpenings.Unwrap ()) {
		API_Neig neig (GSGuid2APIGuid (id.GetGuid ()));
		connectedOpeningsToSelect.Push (neig);
	}

	ACAPI_Selection_Select (connectedOpeningsToSelect, true);

	return NoError;
}


// -----------------------------------------------------------------------------
// Select element(s) connected to the clicked opening
// -----------------------------------------------------------------------------


GSErrCode	SelectElementsConnectedToOpening ()
{
	API_Guid	openingGuid {};
	API_Coord3D	clickPointFirst {};

	if (!ClickAnElem ("Click an opening!", API_OpeningID, nullptr, nullptr, &openingGuid, &clickPointFirst)) {
		WriteReport_Alert ("No opening was clicked!");
		return Error;
	}

	ACAPI::Result<ACAPI::Element::Opening> opening = ACAPI::Element::Opening::Get ({openingGuid, ACAPI_GetToken ()});
	if (opening.IsErr ()) {
		return opening.UnwrapErr ().kind;
	}

	std::vector<ACAPI::UniqueID> connectedElementIds = opening->GetCutElements ();
	GS::Array<API_Neig> connectedElements;

	for (const ACAPI::UniqueID& id : connectedElementIds) {
		API_Neig neig (GSGuid2APIGuid (id.GetGuid ()));
		connectedElements.Push (neig);
	}

	ACAPI_Selection_Select (connectedElements, true);

	return NoError;
}


// -----------------------------------------------------------------------------
// Selects the clicked opening's parent element
// -----------------------------------------------------------------------------


GSErrCode	SelectOpeningsParentElement ()
{
	API_Guid	openingGuid {};
	API_Coord3D	clickPointFirst {};

	if (!ClickAnElem ("Click an opening!", API_OpeningID, nullptr, nullptr, &openingGuid, &clickPointFirst)) {
		WriteReport_Alert ("No opening was clicked!");
		return Error;
	}

	ACAPI::Result<ACAPI::Element::Opening> opening = ACAPI::Element::Opening::Get ({openingGuid, ACAPI_GetToken ()});
	if (opening.IsErr ()) {
		return opening.UnwrapErr ().kind;
	}

	ACAPI::Result<ACAPI::UniqueID> parentElement = opening->GetParentElement ();

	if (parentElement.IsErr ()) {
		return parentElement.UnwrapErr ().kind;
	}

	GS::Array<API_Neig> parentElementToSelect;
	API_Neig neig (GSGuid2APIGuid (parentElement->GetGuid ()));
	parentElementToSelect.Push (neig);

	ACAPI_Selection_Select (parentElementToSelect, true);

	return NoError;
}


// -----------------------------------------------------------------------------
// Changes the clicked opening's dimensions to the dimensions defined in the current opening default
// -----------------------------------------------------------------------------


GSErrCode	ChangeOpeningsDimensionsToCurrentDefault () {
	API_Guid	openingGuid {};
	API_Coord3D	clickPointFirst {};

	if (!ClickAnElem ("Click an opening to change its dimensions!", API_OpeningID, nullptr, nullptr, &openingGuid, &clickPointFirst)) {
		WriteReport_Alert ("Please click an opening!");
		return Error;
	}

	ACAPI::Result<ACAPI::Element::OpeningDefault> openingDefault = ACAPI::Element::CreateOpeningDefault ();
	if (openingDefault.IsErr ()) {
		return openingDefault.UnwrapErr ().kind;
	}

	if (openingDefault->GetExtrusionParameters ().GetShapeType () == ACAPI::Element::ShapeType::Polygonal) {
		WriteReport_Alert ("Opening Default should not be Polygonal!");
		return Error;
	}

	std::optional<double> width = openingDefault->GetExtrusionParameters ().GetWidth ();
	std::optional<double> height = openingDefault->GetExtrusionParameters ().GetHeight ();
	std::optional<ACAPI::Element::LinkedStatus> linkedStatus = openingDefault->GetExtrusionParameters ().GetLinkedStatus ();
	if (!width.has_value () || !height.has_value () || !linkedStatus.has_value ()) {
		WriteReport_Alert ("Opening Default has no dimensions!");
		return Error;
	}

	ACAPI::Result<ACAPI::Element::Opening> opening = ACAPI::Element::Opening::Get ({openingGuid, ACAPI_GetToken ()});
	if (opening.IsErr ()) {
		return opening.UnwrapErr ().kind;
	}

	if (opening->GetExtrusionParameters ().GetShapeType () == ACAPI::Element::ShapeType::Polygonal) {
		WriteReport_Alert ("Clicked opening should not be Polygonal!");
		return Error;
	}

	GSErrCode changeDimensionsResultCode = NoError;
	ACAPI::Result<void> modifyResult = opening->Modify ([&] (ACAPI::Element::Opening::Modifier& modifier) {
		if (ACAPI::Result<void> changeWidthResult = modifier.GetExtrusionParameters ().SetWidth (width.value ());
			changeWidthResult.IsErr ())
		{
			changeDimensionsResultCode = changeWidthResult.UnwrapErr ().kind;
			return;
		}

		if (ACAPI::Result<void> changeHeightResult = modifier.GetExtrusionParameters ().SetHeight (height.value ());
			changeHeightResult.IsErr ())
		{
			changeDimensionsResultCode = changeHeightResult.UnwrapErr ().kind;
			return;
		}

		if (ACAPI::Result<void> changeLinkedStatusResult = modifier.GetExtrusionParameters ().SetLinkedStatus (linkedStatus.value ());
			changeLinkedStatusResult.IsErr ())
		{
			changeDimensionsResultCode = changeLinkedStatusResult.UnwrapErr ().kind;
		}
	}, "Changes the Opening's dimensions");

	if (modifyResult.IsErr ()) {
		return modifyResult.UnwrapErr ().kind;
	}

	return changeDimensionsResultCode;
}


// -----------------------------------------------------------------------------
// Changes the clicked opening's symbol to the symbol defined in the current opening default
// -----------------------------------------------------------------------------


GSErrCode	ChangeOpeningsCutSymbolToCurrentDefault ()
{
	API_Guid	openingGuid {};
	API_Coord3D	clickPointFirst {};

	if (!ClickAnElem ("Click an opening to change its symbol!", API_OpeningID, nullptr, nullptr, &openingGuid, &clickPointFirst)) {
		WriteReport_Alert ("Please click an opening!");
		return Error;
	}

	ACAPI::Result<ACAPI::Element::OpeningDefault> openingDefault = ACAPI::Element::CreateOpeningDefault ();
	if (openingDefault.IsErr ()) {
		return openingDefault.UnwrapErr ().kind;
	}

	std::optional<API_Guid> cutSymbolGuid = openingDefault->GetFloorPlanParameters ().GetCutSymbolLibpartId ();
	if (!cutSymbolGuid.has_value ()) {
		WriteReport_Alert ("Opening Default has no cut symbol!");
		return Error;
	}

	ACAPI::Result<ACAPI::Element::Opening> opening = ACAPI::Element::Opening::Get ({openingGuid, ACAPI_GetToken ()});
	if (opening.IsErr ()) {
		return opening.UnwrapErr ().kind;
	}

	GSErrCode changeSymbolResultCode = NoError;
	ACAPI::Result<void> modifyResult = opening->Modify ([&] (ACAPI::Element::Opening::Modifier& modifier) {
		ACAPI::Result<void> changeSymbolResult = modifier.GetFloorPlanParameters ().SetCutSymbol (*cutSymbolGuid);
		if (changeSymbolResult.IsErr ()) {
			changeSymbolResultCode = changeSymbolResult.UnwrapErr ().kind;
		}
	}, "Changes the Opening's cut symbol");

	if (modifyResult.IsErr ()) {
		return modifyResult.UnwrapErr ().kind;
	}

	return changeSymbolResultCode;
}


// -----------------------------------------------------------------------------
// Changes the clicked opening's override surfaces to the override surfaces defined in the current opening default
// -----------------------------------------------------------------------------


GSErrCode	ChangeOpeningsOverrideSurfacesToCurrentDefault ()
{
	API_Guid	openingGuid {};
	API_Coord3D	clickPointFirst {};

	if (!ClickAnElem ("Click an opening to change its override surfaces!", API_OpeningID, nullptr, nullptr, &openingGuid, &clickPointFirst)) {
		WriteReport_Alert ("Please click an opening!");
		return Error;
	}

	ACAPI::Result<ACAPI::Element::OpeningDefault> openingDefault = ACAPI::Element::CreateOpeningDefault ();
	if (openingDefault.IsErr ()) {
		return openingDefault.UnwrapErr ().kind;
	}

	ACAPI::Result<ACAPI::Element::Opening> opening = ACAPI::Element::Opening::Get ({openingGuid, ACAPI_GetToken ()});
	if (opening.IsErr ()) {
		return opening.UnwrapErr ().kind;
	}

	bool endSurfaceIsOverriden = openingDefault->GetExtrusionParameters ().IsEndSurfaceAttributeOverridden ();
	API_AttributeIndex endSurfaceIndex = openingDefault->GetExtrusionParameters ().GetOverriddenEndSurfaceAttributeIndex ();
	bool extrusionSurfaceIsOverriden = openingDefault->GetExtrusionParameters ().IsExtrusionSurfaceAttributeOverridden ();
	API_AttributeIndex extrusionSurfaceIndex = openingDefault->GetExtrusionParameters ().GetOverriddenExtrusionSurfaceAttributeIndex ();
	bool surfacesAreLinked = openingDefault->GetExtrusionParameters ().AreOverriddenSurfaceAttributesLinked ();

	ACAPI::Result<void> modifyResult = opening->Modify ([&] (ACAPI::Element::Opening::Modifier& modifier) {
		modifier.GetExtrusionParameters ().SetEndSurfaceAttributeOverridden (endSurfaceIsOverriden);
		modifier.GetExtrusionParameters ().SetOverriddenEndSurfaceAttributeIndex (endSurfaceIndex);
		modifier.GetExtrusionParameters ().SetExtrusionSurfaceAttributeOverridden (extrusionSurfaceIsOverriden);
		modifier.GetExtrusionParameters ().SetOverriddenExtrusionSurfaceAttributeIndex (extrusionSurfaceIndex);
		modifier.GetExtrusionParameters ().SetOverriddenSurfaceAttributesLinked (surfacesAreLinked);
	}, "Changes the Opening's override surfaces");

	if (modifyResult.IsErr ()) {
		return modifyResult.UnwrapErr ().kind;
	}

	return NoError;
}


// -----------------------------------------------------------------------------
// Changes the current Opening default's cover fill type and foreground pen parameters with the parameters of the clicked Opening's.
// -----------------------------------------------------------------------------


GSErrCode	ChangeOpeningDefaultsCoverFillTypeAndPen ()
{
	API_Guid	openingGuid {};
	API_Coord3D	clickPointFirst {};

	if (!ClickAnElem ("Click an opening to change its default's parameters!", API_OpeningID, nullptr, nullptr, &openingGuid, &clickPointFirst)) {
		WriteReport_Alert ("Please click an opening!");
		return Error;
	}

	ACAPI::Result<ACAPI::Element::Opening> opening = ACAPI::Element::Opening::Get ({openingGuid, ACAPI_GetToken ()});
	if (opening.IsErr ()) {
		return opening.UnwrapErr ().kind;
	}

	API_AttributeIndex coverFillIndex = opening->GetFloorPlanParameters ().GetCoverFillsIndex ();
	short coverFillPenIndex = opening->GetFloorPlanParameters ().GetCoverFillsPenIndex ();

	ACAPI::Result<ACAPI::Element::OpeningDefault> openingDefault = ACAPI::Element::CreateOpeningDefault ();
	if (openingDefault.IsErr ()) {
		return openingDefault.UnwrapErr ().kind;
	}

	ACAPI::Result<void> modifyResult = openingDefault->Modify ([&] (ACAPI::Element::OpeningDefault::Modifier& modifier) {
		modifier.GetFloorPlanParameters ().SetCoverFillsIndex (coverFillIndex);
		modifier.GetFloorPlanParameters ().SetCoverFillsPenIndex (coverFillPenIndex);
	});

	if (modifyResult.IsErr ()) {
		return modifyResult.UnwrapErr ().kind;
	}

	ACAPI::Result<void> setAsDefaultResult = openingDefault->SetAsArchicadDefault ();

	if (setAsDefaultResult.IsErr ()) {
		return setAsDefaultResult.UnwrapErr ().kind;
	}

	return NoError;
}