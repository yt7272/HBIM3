#include "GenerateLargeSystems.hpp"

// API
#include "ACAPinc.h"
#include "Definitions.hpp"
#include "API_Guid.hpp"
#include "MDIDs_Public.h"

// ACAPI
#include "ACAPI/Result.hpp"

// MEPAPI
#include "ACAPI/MEPRoutingElement.hpp"
#include "ACAPI/MEPRoutingElementDefault.hpp"
#include "ACAPI/MEPRoutingSegment.hpp"
#include "ACAPI/MEPTerminalDefault.hpp"
#include "ACAPI/MEPEquipmentDefault.hpp"
#include "ACAPI/MEPAdapter.hpp"
#include "ACAPI/MEPPhysicalSystem.hpp"
#include "ACAPI/MEPSystemGroup.hpp"

// UD
#include "UDTextInputDialog.hpp"

// GSUtils
#include "GSUnID.hpp"


namespace {

struct SetHydronicsElementDataParams {
	ACAPI::MEP::UniqueID routingElementID;
	double capacityHeatingInWatts;
	double capacityCoolingInWatts;
	double returnTemperatureHeatingInCelcius;
	double returnTemperatureCoolingInCelcius;
};


constexpr API_ModulID HydronicsAddOnMDID = { MDID_GS2, MDID_GS2_Hydronics };
constexpr GSType ADDONCMD_SETMEPCALCULATIONDATA = 'SMCD';

struct MEPHydronicsCalculationData {
	ACAPI::UniqueID		elementID;
	double				capacityHeating_InWatts { 0 };
	double				capacityCooling_InWatts { 0 };
	double				returnTemperatureHeating_InCelsius { 0 };
	double				returnTemperatureCooling_InCelsius { 0 };

	MEPHydronicsCalculationData (const ACAPI::UniqueID& elementID) :
		elementID (elementID),
		capacityHeating_InWatts (0),
		capacityCooling_InWatts (0),
		returnTemperatureHeating_InCelsius (0),
		returnTemperatureCooling_InCelsius (0)
	{
	}
};

GSErrCode SetHeatingCapacity (ACAPI::MEP::UniqueID elementId, double heatingCapacityInWatts)
{
	MEPHydronicsCalculationData data (elementId);
	data.capacityHeating_InWatts = heatingCapacityInWatts;

	return ACAPI_AddOnAddOnCommunication_Call (&HydronicsAddOnMDID, ADDONCMD_SETMEPCALCULATIONDATA, 1, reinterpret_cast<GSHandle> (&data), nullptr, true);
}

} // namespace

namespace MEPExample {

namespace {

// Macros for error checking.
#define RETURN_IF_RESULT_ERROR(res)                   \
    do {                                              \
        auto _result = (res);                         \
        if (_result.IsErr())                          \
            return _result.UnwrapErr().kind;          \
    } while (0)

#define RETURN_IF_ERROR_CODE(code)                    \
    do {                                              \
        GS::GSErrCode _err = (code);                  \
        if (_err != NoError)                          \
            return _err;                              \
    } while (0)


// ! [CreateSystemGroup-Example]
constexpr std::string_view DefaultSystemGroupName { "Generated System Group" };

std::optional<ACAPI::MEP::SystemGroup> FindOrCreateDefaultGroup ()
{
	// First, locate the default system group:
	auto systemGroups { ACAPI::MEP::GetSystemGroupIDs () };
	if (systemGroups.IsErr ())
		return {};

	for (const auto& id : systemGroups.Unwrap ()) {
		auto systemGroup { ACAPI::MEP::SystemGroup::Get (id) };
		if (systemGroup.IsErr ())
			return {};

		if (systemGroup->GetName () == DefaultSystemGroupName)
			return systemGroup.Unwrap ();
	}

	auto newlyCreatedSystemGroup { ACAPI::MEP::CreateSystemGroup (GS::UniString { DefaultSystemGroupName }) };
	if (newlyCreatedSystemGroup.IsErr ())
		return {};

	return newlyCreatedSystemGroup.Unwrap ();
}
// ! [CreateSystemGroup-Example]

static std::optional<ACAPI::MEP::UniqueID> CreateSystem (ACAPI::MEP::SystemGroup& group, const GS::UniString& suggestedName, API_AttributeIndex systemCategoryIndex, const ACAPI::MEP::UniqueID& rootElement)
{
// ! [SystemGroup-Modify-Example]
// ! [SystemGroup-Modifier-Add-Example]
	std::optional<ACAPI::MEP::UniqueID> createdId;
	if (group.Modify ([&](ACAPI::MEP::SystemGroup::Modifier& modifier) -> GS::GSErrCode {
		const auto result { modifier.Add (suggestedName) };
		if (result.IsErr ())
			return result.UnwrapErr ().kind;

		createdId = result.Unwrap ();
		return NoError;
	}, "Create new system.").IsErr ())
		return {};
// ! [SystemGroup-Modifier-Add-Example]
// ! [SystemGroup-Modify-Example]

	if (!createdId.has_value ())
		return {};

// ! [PhysicalSystem-Modify-Example]
	auto system { ACAPI::MEP::PhysicalSystem::Get (*createdId) };
	if (system.IsErr ())
		return {};

	if (system->Modify ([&](ACAPI::MEP::PhysicalSystem::Modifier& modifier) -> GS::GSErrCode {
		RETURN_IF_RESULT_ERROR (modifier.SetSystemCategory (systemCategoryIndex));
		RETURN_IF_RESULT_ERROR (modifier.SetRootElement (rootElement));
		return NoError;
	}, "Set data of newly created system").IsErr ())
		return {};
// ! [PhysicalSystem-Modify-Example]
	
	return *createdId;
}

static GSErrCode SetEquipmentDefaultToLoadObject ()
{
	ACAPI::Result<ACAPI::MEP::EquipmentDefault> eqDefault = ACAPI::MEP::CreateEquipmentDefault ();
	RETURN_IF_RESULT_ERROR (eqDefault);

	ACAPI::Result<void> modifyResult = eqDefault->Modify ([&] (ACAPI::MEP::EquipmentDefault::Modifier& modifier) {
		modifier.SetObjectId (GSGuid2APIGuid (GS::Guid ("51E5AC44-68FB-4B59-AD14-CFE3C8B9CEC6")));
	});
	RETURN_IF_RESULT_ERROR (modifyResult);

	// It is important to set it back as AC default, because the old API can only work on the global defaults
	ACAPI::Result<void> setAsDefaultResult = eqDefault->SetAsArchicadDefault ();
	RETURN_IF_RESULT_ERROR (setAsDefaultResult);

	return NoError;
}

static GSErrCode SetEquipmentDefault ()
{
	SetEquipmentDefaultToLoadObject ();

	// Set the GDL parameters
	API_Element		element {};
	API_ElementMemo	memo {};

	API_ElemType mepElemType;
	mepElemType.typeID = API_ExternalElemID;
	mepElemType.classID = ACAPI::MEP::EquipmentID;
	mepElemType.variationID = APIVarId_Generic;

	element.header.type = mepElemType;

	RETURN_IF_ERROR_CODE (ACAPI_Element_GetDefaults (&element, &memo));

	API_Element mask {};

	Int32 n = BMGetHandleSize ((GSHandle) memo.params) / sizeof (API_AddParType);
	for (Int32 i = 0; i < n; i++) {
		if (CHCompareASCII ((*memo.params)[i].name, "iConPlacement_1", GS::CaseInsensitive) == 0) {
			(*memo.params)[i].value.real = 10.0; // Lower left side
			break;
		}
	}
	for (Int32 i = 0; i < n; i++) {
		if (CHCompareASCII ((*memo.params)[i].name, "iConPlacement_2", GS::CaseInsensitive) == 0) {
			(*memo.params)[i].value.real = 4.0; // Upper right side
			break;
		}
	}

	RETURN_IF_ERROR_CODE (ACAPI_Element_ChangeDefaults (&element, &memo, &mask));
	RETURN_IF_ERROR_CODE (ACAPI_DisposeElemMemoHdls (&memo));

	return SetEquipmentDefaultToLoadObject ();
}

// Connects one routing element to a list of other routing elements.
// If idsOfEndsAfterMerging is not null, it stores the end IDs after merging.
GSErrCode ConnectToRoute (const ACAPI::MEP::UniqueID& routeId,
						  const std::vector<ACAPI::MEP::UniqueID>& idsOfObjectsToConnect,
						  std::vector<ACAPI::MEP::UniqueID>* idsOfEndsAfterMerging)
{
	if (idsOfObjectsToConnect.empty ())
		return Error;

	ACAPI::MEP::UniqueID elemToModifyId = routeId;

	for (const auto& objectId : idsOfObjectsToConnect) {
		ACAPI::Result<ACAPI::MEP::RoutingElement> elemToModify = ACAPI::MEP::RoutingElement::Get (elemToModifyId);
		RETURN_IF_RESULT_ERROR (elemToModify);

		ACAPI::Result<void> connectionResult = elemToModify->Modify (
			[&] (ACAPI::MEP::RoutingElement::Modifier& modifier) -> GSErrCode {
			ACAPI::Result<ACAPI::MEP::RouteConnectionResult> connRes = modifier.ConnectLogically (objectId);
			if (connRes.IsErr ())
				return connRes.UnwrapErr ().kind;

			// Update the element if it was split.
			if (connRes.Unwrap ().splittedRoutingElementId.has_value ())
				elemToModifyId = connRes.Unwrap ().splittedRoutingElementId.value ();

			if (idsOfEndsAfterMerging != nullptr) {
				if (connRes.Unwrap ().deletedRoutingElementId.has_value ())
					idsOfEndsAfterMerging->push_back (elemToModifyId);
				else
					idsOfEndsAfterMerging->push_back (objectId);
			}
			return NoError;
		}, "Connect routes." );
		RETURN_IF_RESULT_ERROR (connectionResult);
	}

	return NoError;
}

// Generates duct (ventilation) systems.
GSErrCode GenerateDuct (GS::Int32 numberOfSystems, GS::Int32 numberOfTerminalsInEachSystem)
{
	// Compute subsystems and terminals per subsystem.
	GS::Int32 numberOfSubSystems = static_cast<GS::Int32>(std::sqrt (numberOfTerminalsInEachSystem));
	if (numberOfSubSystems * numberOfSubSystems < numberOfTerminalsInEachSystem)
		++numberOfSubSystems;

	GS::Int32 numberOfTerminalsInEachSubSystem = static_cast<GS::Int32> (numberOfTerminalsInEachSystem / numberOfSubSystems);
	if ((numberOfTerminalsInEachSubSystem * numberOfSubSystems * numberOfSystems) < (numberOfTerminalsInEachSystem * numberOfSystems))
		++numberOfTerminalsInEachSubSystem;

	const double distanceBetweenBranches = 4.0;
	const GS::Int32 placementDeltaOfSystems = static_cast<GS::Int32>(numberOfSubSystems * distanceBetweenBranches + 5);

	// Set up the process window.
	GS::UniString title ("Generating systems");
	GS::UniString subTitle ("Number of terminals: ");
	GS::UniString finishingText ("Notifying system about changes");
	GS::Int32 nProgressElements = numberOfSystems * numberOfTerminalsInEachSystem;
	Int32 phaseNum = 1;

	subTitle += GS::UniString::Printf ("%d", nProgressElements);
	API_ProcessControlTypeID pcType = API_MenuCommandEnabled;
	ACAPI_ProcessWindow_InitProcessWindow (&title, &nProgressElements, &pcType);

	ACAPI::Result<void> commandResult = ACAPI::CallUndoableCommand ([&] () -> GSErrCode {
		ACAPI::Result<ACAPI::MEP::RoutingElementDefault> routingElementDefault = ACAPI::MEP::CreateRoutingElementDefault (ACAPI::MEP::Domain::Ventilation);
		RETURN_IF_RESULT_ERROR (routingElementDefault);

		ACAPI::Result<ACAPI::MEP::TerminalDefault> terminalDefault = ACAPI::MEP::CreateTerminalDefault (ACAPI::MEP::Domain::Ventilation);
		RETURN_IF_RESULT_ERROR (terminalDefault);

		for (GS::Int32 i = 0; i < numberOfSystems; ++i) {
			const double xStartPlacement = i * placementDeltaOfSystems;
			GS::Int32 currentlyPlacedTerminals = 0;
			std::vector<API_Coord3D> mainRouteCoords;

			// Build main route coordinates.
			for (GS::Int32 j = 0; j <= numberOfSubSystems; ++j)
				mainRouteCoords.push_back ({xStartPlacement + (distanceBetweenBranches * j), 0.0, 0.0});

			ACAPI::Result<ACAPI::MEP::UniqueID> mainRoutingElemId = routingElementDefault->Place (mainRouteCoords, {}, {});
			RETURN_IF_RESULT_ERROR (mainRoutingElemId);

			std::vector<ACAPI::MEP::UniqueID> subRouteIds;

			// Build sub–routes.
			for (GS::Int32 j = 1; j <= numberOfSubSystems; ++j) {
				std::vector<API_Coord3D> subRouteCoords;
				for (GS::Int32 k = 0; k <= numberOfTerminalsInEachSubSystem; ++k)
					subRouteCoords.push_back ({xStartPlacement + (distanceBetweenBranches * j),
												(static_cast<double>(k) * distanceBetweenBranches),
												0.0});
				ACAPI::Result<ACAPI::MEP::UniqueID> subRoutingElemId = routingElementDefault->Place (subRouteCoords, {}, {});
				RETURN_IF_RESULT_ERROR (subRoutingElemId);
				subRouteIds.push_back (*subRoutingElemId);

				std::vector<ACAPI::MEP::UniqueID> endRouteIds;
				// Build end–routes for each terminal.
				for (GS::Int32 k = 1; k <= numberOfTerminalsInEachSubSystem; ++k) {
					std::vector<API_Coord3D> endRouteCoords;
					endRouteCoords.push_back ({xStartPlacement + (distanceBetweenBranches * j),
												(static_cast<double>(k) * distanceBetweenBranches),
												0.0});
					endRouteCoords.push_back ({xStartPlacement + (distanceBetweenBranches * j) - (distanceBetweenBranches / 2),
												(static_cast<double>(k) * distanceBetweenBranches),
												0.0});

					ACAPI::Result<ACAPI::MEP::UniqueID> endRoutingElemId = routingElementDefault->Place (endRouteCoords, {}, {});
					RETURN_IF_RESULT_ERROR (endRoutingElemId);
					endRouteIds.push_back (*endRoutingElemId);

					// Get the end segment and place a terminal.
					auto endElement = ACAPI::MEP::RoutingElement::Get (*endRoutingElemId);
					RETURN_IF_RESULT_ERROR (endElement);
					ACAPI::Result<ACAPI::MEP::RoutingSegment> endSegment = ACAPI::MEP::RoutingSegment::Get (endElement.Unwrap ().GetRoutingSegmentIds ().front ());
					RETURN_IF_RESULT_ERROR (endSegment);

					ACAPI_ProcessWindow_SetNextProcessPhase (&subTitle, &phaseNum);
					ACAPI::Result<ACAPI::MEP::UniqueID> terminalId = terminalDefault->Place (endSegment.Unwrap ().GetEndNodeId ());
					RETURN_IF_RESULT_ERROR (terminalId);

					if (++currentlyPlacedTerminals >= numberOfTerminalsInEachSystem)
						break;
				}

				RETURN_IF_ERROR_CODE (ConnectToRoute (*subRoutingElemId, endRouteIds, nullptr));
			}

			RETURN_IF_ERROR_CODE (ConnectToRoute (*mainRoutingElemId, subRouteIds, nullptr));
		}

		ACAPI_ProcessWindow_SetNextProcessPhase (&finishingText, &phaseNum);
		return NoError;
	}, ACAPI_GetToken (), "GenerateSystems.");

	ACAPI_ProcessWindow_CloseProcessWindow ();
	return NoError;
}

// Generates pipe (piping) systems.
GSErrCode GeneratePipe (GS::Int32 numberOfSystems, GS::Int32 numberOfEquipmentsInEachSystem)
{
	// Compute subsystems and equipments per subsystem.
	GS::Int32 numberOfSubSystems = static_cast<GS::Int32>(std::sqrt (numberOfEquipmentsInEachSystem));
	if (numberOfSubSystems * numberOfSubSystems < numberOfEquipmentsInEachSystem)
		++numberOfSubSystems;

	GS::Int32 numberOfEquipmentsInEachSubSystem = static_cast<GS::Int32> (numberOfEquipmentsInEachSystem / numberOfSubSystems);
	if ((numberOfEquipmentsInEachSubSystem * numberOfSubSystems * numberOfSystems) < (numberOfEquipmentsInEachSystem * numberOfSystems))
		++numberOfEquipmentsInEachSubSystem;

	const double distanceBetweenBranches = 6.0;
	const GS::Int32 placementDeltaOfSystems = static_cast<GS::Int32>(numberOfSubSystems * distanceBetweenBranches + 5);

	// Process window setup.
	GS::UniString title ("Generating systems");
	GS::UniString subTitle ("Number of equipments: ");
	GS::UniString finishingText ("Notifying system about changes");
	GS::Int32 nProgressElements = numberOfSystems * numberOfEquipmentsInEachSystem;
	Int32 phaseNum = 1;
	constexpr double zOfReturn = 0.7;
	constexpr double zOfSupply = 0.1;
	constexpr double zOfEquipment = 0.25;

	subTitle += GS::UniString::Printf ("%d", nProgressElements);
	API_ProcessControlTypeID pcType = API_MenuCommandEnabled;
	ACAPI_ProcessWindow_InitProcessWindow (&title, &nProgressElements, &pcType);

	ACAPI::Result<void> commandResult = ACAPI::CallUndoableCommand ([&] () -> GSErrCode {
		ACAPI::Result<ACAPI::MEP::RoutingElementDefault> routingElementSupplyDefault = ACAPI::MEP::CreateRoutingElementDefault (ACAPI::MEP::Domain::Piping);
		RETURN_IF_RESULT_ERROR (routingElementSupplyDefault);

		ACAPI::Result<ACAPI::MEP::RoutingElementDefault> routingElementReturnDefault = ACAPI::MEP::CreateRoutingElementDefault (ACAPI::MEP::Domain::Piping);
		RETURN_IF_RESULT_ERROR (routingElementReturnDefault);

		// Define attribute indices
		API_AttributeIndex supplyIndex = ACAPI_CreateAttributeIndex (0);
		API_AttributeIndex returnIndex = ACAPI_CreateAttributeIndex (0);

		ACAPI_Attribute_EnumerateAttributesByType (API_MEPSystemID, [&] (const API_Attribute& attribute) {
			if (attribute.mepSystem.domain == APIMEPDomain_Piping && attribute.mepSystem.head.uniStringNamePtr->IsEqual ("Hydronic Supply"))
				supplyIndex = attribute.mepSystem.head.index;
			else if (attribute.mepSystem.domain == APIMEPDomain_Piping && attribute.mepSystem.head.uniStringNamePtr->IsEqual ("Hydronic Return"))
				returnIndex = attribute.mepSystem.head.index;
		});

		// Set MEP system attribute for supply and return defaults.
		RETURN_IF_RESULT_ERROR (routingElementSupplyDefault->Modify ([&] (ACAPI::MEP::RoutingElementDefault::Modifier& modifier) -> ACAPI::Result<void> {
			modifier.SetMEPSystem (supplyIndex);
			return ACAPI::Ok ();
		}));
		RETURN_IF_RESULT_ERROR (routingElementReturnDefault->Modify ([&] (ACAPI::MEP::RoutingElementDefault::Modifier& modifier) -> ACAPI::Result<void> {
			modifier.SetMEPSystem (returnIndex);
			return ACAPI::Ok ();
		}));
		// Set equipment default from the given library part GUID.
		RETURN_IF_ERROR_CODE (SetEquipmentDefault ());

		ACAPI::Result<ACAPI::MEP::EquipmentDefault> equipmentDefault = ACAPI::MEP::CreateEquipmentDefault ();
		RETURN_IF_RESULT_ERROR (equipmentDefault);

		std::optional<ACAPI::MEP::SystemGroup> systemGroup { FindOrCreateDefaultGroup () };
		if (!systemGroup.has_value ())
			return Error;

		for (GS::Int32 i = 0; i < numberOfSystems; ++i) {
			const double xStartPlacement = i * placementDeltaOfSystems;
			GS::Int32 currentlyPlacedEquipments = 0;
			std::vector<API_Coord3D> supplyRouteCoords;
			std::vector<API_Coord3D> returnRouteCoords;
			const double distanceToSubtractFromXOnSupply = distanceBetweenBranches * 3 / 4;

			// Build main supply and return routes.
			for (GS::Int32 j = 0; j <= numberOfSubSystems; ++j) {
				supplyRouteCoords.push_back ({xStartPlacement + (distanceBetweenBranches * j) -	(j == 0 ? 0.0 : distanceToSubtractFromXOnSupply),
											   distanceBetweenBranches / 4,
											   zOfSupply});
				returnRouteCoords.push_back ({xStartPlacement + (distanceBetweenBranches * j),
											   0.0,
											   zOfReturn});
			}

			ACAPI::Result<ACAPI::MEP::UniqueID> supplyRoutingElemId = routingElementSupplyDefault->Place (supplyRouteCoords, {}, {});
			RETURN_IF_RESULT_ERROR (supplyRoutingElemId);

			ACAPI::Result<ACAPI::MEP::UniqueID> returnRoutingElemId = routingElementReturnDefault->Place (returnRouteCoords, {}, {});
			RETURN_IF_RESULT_ERROR (returnRoutingElemId);

			std::vector<ACAPI::MEP::UniqueID> subSupplyRouteIds;
			std::vector<ACAPI::MEP::UniqueID> subReturnRouteIds;

			// Process subsystems.
			for (GS::Int32 j = 1; j <= numberOfSubSystems; ++j) {
				std::vector<API_Coord3D> subSupplyRouteCoords;
				std::vector<API_Coord3D> subReturnRouteCoords;

				for (GS::Int32 k = 0; k <= numberOfEquipmentsInEachSubSystem; ++k) {
					// Don't create routes without equipments
					if ((j - 1) * numberOfEquipmentsInEachSubSystem + k > numberOfEquipmentsInEachSystem)
						break;

					subSupplyRouteCoords.push_back ({xStartPlacement + (distanceBetweenBranches * j) - distanceToSubtractFromXOnSupply,
													  static_cast<double>(k) * distanceBetweenBranches + (k == 0 ? distanceBetweenBranches / 4 : 0.0),
													  zOfSupply});
					subReturnRouteCoords.push_back ({xStartPlacement + (distanceBetweenBranches * j),
													  static_cast<double>(k) * distanceBetweenBranches,
													  zOfReturn});
				}
				ACAPI::Result<ACAPI::MEP::UniqueID> subSupplyRoutingElemId = routingElementSupplyDefault->Place (subSupplyRouteCoords, {}, {});
				RETURN_IF_RESULT_ERROR (subSupplyRoutingElemId);
				ACAPI::Result<ACAPI::MEP::UniqueID> subReturnRoutingElemId = routingElementReturnDefault->Place (subReturnRouteCoords, {}, {});
				RETURN_IF_RESULT_ERROR (subReturnRoutingElemId);

				subSupplyRouteIds.push_back (*subSupplyRoutingElemId);
				subReturnRouteIds.push_back (*subReturnRoutingElemId);

				std::vector<ACAPI::MEP::UniqueID> endSupplyRouteIds;
				std::vector<ACAPI::MEP::UniqueID> endReturnRouteIds;
				// Process end–routes for each equipment.
				for (GS::Int32 k = 1; k <= numberOfEquipmentsInEachSubSystem; ++k) {
					std::vector<API_Coord3D> endSupplyRouteCoords;
					std::vector<API_Coord3D> endReturnRouteCoords;

					endSupplyRouteCoords.push_back ({xStartPlacement + (distanceBetweenBranches * j) - (distanceBetweenBranches * 3 / 4),
													  static_cast<double>(k) * distanceBetweenBranches,
													  zOfSupply});
					endSupplyRouteCoords.push_back ({xStartPlacement + (distanceBetweenBranches * j) - (distanceBetweenBranches / 2) + 0.13,
													  static_cast<double>(k) * distanceBetweenBranches,
													  zOfSupply});

					endReturnRouteCoords.push_back ({xStartPlacement + (distanceBetweenBranches * j),
													  static_cast<double>(k) * distanceBetweenBranches,
													  zOfReturn});
					endReturnRouteCoords.push_back ({xStartPlacement + (distanceBetweenBranches * j) - (distanceBetweenBranches / 4) - 0.13,
													  static_cast<double>(k) * distanceBetweenBranches,
													  zOfReturn});

					ACAPI::Result<ACAPI::MEP::UniqueID> endSupplyRoutingElemId = routingElementSupplyDefault->Place (endSupplyRouteCoords, {}, {});
					RETURN_IF_RESULT_ERROR (endSupplyRoutingElemId);
					ACAPI::Result<ACAPI::MEP::UniqueID> endReturnRoutingElemId = routingElementReturnDefault->Place (endReturnRouteCoords, {}, {});
					RETURN_IF_RESULT_ERROR (endReturnRoutingElemId);

					endSupplyRouteIds.push_back (*endSupplyRoutingElemId);
					endReturnRouteIds.push_back (*endReturnRoutingElemId);

					ACAPI::Result<ACAPI::MEP::RoutingElement> endSupplyElement = ACAPI::MEP::RoutingElement::Get (*endSupplyRoutingElemId);
					RETURN_IF_RESULT_ERROR (endSupplyElement);
					ACAPI::Result<ACAPI::MEP::RoutingElement> endReturnElement = ACAPI::MEP::RoutingElement::Get (*endReturnRoutingElemId);
					RETURN_IF_RESULT_ERROR (endReturnElement);

					ACAPI_ProcessWindow_SetNextProcessPhase (&subTitle, &phaseNum);

					// Adjust equipment placement relative to the return route.
					API_Coord3D equipmentPlacement = endReturnRouteCoords.back ();
					equipmentPlacement.z = zOfEquipment;
					equipmentPlacement.x -= 1.22; // ToDo: Should be relative to the equipment length.
					equipmentPlacement.y -= 0.15; // ToDo: Should be half of equipment depth.

					ACAPI::Result<ACAPI::MEP::UniqueID> equipmentId = equipmentDefault->Place (equipmentPlacement, {{ 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0 }});
					RETURN_IF_RESULT_ERROR (equipmentId);

					// Connect the equipment to both the supply and return end routes.
					ACAPI::Result<void> connectionSupplyResult =
						endSupplyElement->Modify ([&] (ACAPI::MEP::RoutingElement::Modifier& modifier) -> GSErrCode {
						ACAPI::Result<ACAPI::MEP::RouteConnectionResult> connRes = modifier.ConnectLogically (equipmentId.Unwrap ());
						if (connRes.IsErr ())
							return connRes.UnwrapErr ().kind;
						return NoError;
					}, "Connect equipment to endSupplyRoute.");
					RETURN_IF_RESULT_ERROR (connectionSupplyResult);

					ACAPI::Result<void> connectionReturnResult =
						endReturnElement->Modify ([&] (ACAPI::MEP::RoutingElement::Modifier& modifier) -> GSErrCode {
						ACAPI::Result<ACAPI::MEP::RouteConnectionResult> connRes = modifier.ConnectLogically (equipmentId.Unwrap ());
						if (connRes.IsErr ())
							return connRes.UnwrapErr ().kind;
						return NoError;
					}, "Connect equipment to endReturnRoute.");
					RETURN_IF_RESULT_ERROR (connectionReturnResult);

					// Set heating capacity on equipment
					RETURN_IF_ERROR_CODE (SetHeatingCapacity (equipmentId.Unwrap (), 4000.0));

					if (++currentlyPlacedEquipments >= numberOfEquipmentsInEachSystem)
						break;
				}

				std::vector<ACAPI::MEP::UniqueID> endSupplyRouteIdsAfterMerge;
				std::vector<ACAPI::MEP::UniqueID> endReturnRouteIdsAfterMerge;
				RETURN_IF_ERROR_CODE (ConnectToRoute (*subSupplyRoutingElemId, endSupplyRouteIds, &endSupplyRouteIdsAfterMerge));
				RETURN_IF_ERROR_CODE (ConnectToRoute (*subReturnRoutingElemId, endReturnRouteIds, &endReturnRouteIdsAfterMerge));
			}

			RETURN_IF_ERROR_CODE (ConnectToRoute (*supplyRoutingElemId, subSupplyRouteIds, nullptr));
			RETURN_IF_ERROR_CODE (ConnectToRoute (*returnRoutingElemId, subReturnRouteIds, nullptr));


			// Finally, create PhysicalSystems for supply and route "systems"
			bool systemCreationSuccess = CreateSystem (*systemGroup, "Hydronics Supply", supplyIndex, *supplyRoutingElemId).has_value ();
			if (!systemCreationSuccess)
				return Error;

			systemCreationSuccess = CreateSystem (*systemGroup, "Hydronics Return", returnIndex, *returnRoutingElemId).has_value ();
			if (!systemCreationSuccess)
				return Error;
		}

		ACAPI_ProcessWindow_SetNextProcessPhase (&finishingText, &phaseNum);
		return NoError;
	}, ACAPI_GetToken (), "GenerateSystems.");

	ACAPI_ProcessWindow_CloseProcessWindow ();
	return NoError;
}

// General function to generate large systems (duct or pipe).
GSErrCode GenerateLargeSystems (bool isDuct)
{
	UD::TextInputDialog numberOfSystemsDialog ("How many systems",
											   "Number of systems to generate:",
											   "1");
	if (numberOfSystemsDialog.Invoke ()) {
		GS::UniString numberOfSystems = numberOfSystemsDialog.GetText ();
		UD::TextInputDialog numberOfTerminalsDialog ("How many terminals per system",
													 "Number of terminals/equipment to generate per system:",
													 "64");
		if (numberOfTerminalsDialog.Invoke ()) {
			GS::UniString numberOfTerminals = numberOfTerminalsDialog.GetText ();
			if (isDuct)
				return GenerateDuct (std::atoi (numberOfSystems.ToCStr ()),
									 std::atoi (numberOfTerminals.ToCStr ()));
			else
				return GeneratePipe (std::atoi (numberOfSystems.ToCStr ()),
									 std::atoi (numberOfTerminals.ToCStr ()));
		}
	}
	return Error;
}

} // anonymous namespace

GSErrCode GenerateLargeSystems_Duct ()
{
	return GenerateLargeSystems (true);
}

GSErrCode GenerateLargeSystems_Pipe ()
{
	return GenerateLargeSystems (false);
}

} // namespace MEPExample
