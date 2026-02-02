#include "LabelingSystems.hpp"

// ACAPI
#include "ACAPinc.h"

// MEPAPI
#include "ACAPI/MEPSystemGroup.hpp"
#include "ACAPI/MEPPhysicalSystem.hpp"
#include "ACAPI/MEPElement.hpp"
#include "ACAPI/MEPRigidSegment.hpp"


namespace {

	ACAPI::MEP::UniqueID APIElemIDToMEPElemID (const ACAPI::UniqueID& APIElemID) {
		return ACAPI::MEP::UniqueID { APIElemID.GetGuid (), APIElemID.GetToken () };
	}


	double GetLabelSize (const ACAPI::MEP::Element& element) {
		constexpr double DefaultSize = 0.2;

		// Fetch size from minimum dimension of rigid segments
		ACAPI::Result<ACAPI::MEP::RigidSegment> rigidSegment { ACAPI::MEP::RigidSegment::Get (APIElemIDToMEPElemID (element.GetElemId ())) };
		if (rigidSegment.IsOk ()) {
			return std::max (DefaultSize, std::min ({ rigidSegment->GetWidth (), rigidSegment->GetHeight (), rigidSegment->GetLength () }));
		}

		// Otherwise, if the element is not a rigid segment. Check if it's direct neighbours are:
		for (const ACAPI::MEP::UniqueID& directlyConnectedElementID : element.GetDirectConnectedMEPElementIds ()) {
			ACAPI::Result<ACAPI::MEP::RigidSegment> rigidSegment { ACAPI::MEP::RigidSegment::Get (directlyConnectedElementID) };
			if (rigidSegment.IsOk ()) {
				return std::max (DefaultSize, std::min ({ rigidSegment->GetWidth (), rigidSegment->GetHeight (), rigidSegment->GetLength () }));
			}
		}

		// In all other cases (routes, equipment, bends, transitions, etc.), just default to a small size:
		return DefaultSize;
	}


	GSErrCode CreateLabelOnElement (const ACAPI::MEP::Element& element, GS::UniString labelText) {
		API_Element textElement {};
		textElement.header.type = API_TextID;
		API_ElementMemo	textMemo;
		GS::GSErrCode err { ACAPI_Element_GetDefaults (&textElement, &textMemo) };
		if (err != NoError)
			return err;

		textElement.text.size = GetLabelSize (element);
		textElement.text.loc.x = element.GetAnchorPoint ().x;
		textElement.text.loc.y = element.GetAnchorPoint ().y;
		textMemo.textContent = new GS::UniString { std::move (labelText) };
		err = ACAPI_Element_Create (&textElement, &textMemo);
		if (err != NoError)
			return err;

		err = ACAPI_DisposeElemMemoHdls (&textMemo);
		if (err != NoError)
			return err;

		return NoError;
	}

} // namespace

namespace MEPExample {

GSErrCode LabelCurrentSystemInstancesAndGroups ()
{
	return ACAPI_CallUndoableCommand ("Label All Current Systems", []() -> GS::GSErrCode {
// ! [GetSystemGroupIDs-Example]
		const ACAPI::Result<std::vector<ACAPI::MEP::UniqueID>> systemGroupIDs { ACAPI::MEP::GetSystemGroupIDs () };
		if (systemGroupIDs.IsErr ())
			return systemGroupIDs.UnwrapErr ().kind;

		for (const ACAPI::MEP::UniqueID& systemGroupID : systemGroupIDs.Unwrap ()) {
// ! [GetSystemGroupIDs-Example]
// ! [SystemGroup-Get-Example]
			const ACAPI::Result<ACAPI::MEP::SystemGroup> systemGroup { ACAPI::MEP::SystemGroup::Get (systemGroupID) };
			if (systemGroup.IsErr ())
				return systemGroup.UnwrapErr ().kind;
// ! [SystemGroup-Get-Example]

// ! [SystemGroup-GetSystemIDs-Example]
			const std::vector<ACAPI::MEP::UniqueID> systemIDs = systemGroup.Unwrap ().GetSystemIDs ();

			for (const ACAPI::MEP::UniqueID& systemID : systemIDs) {
// ! [SystemGroup-GetSystemIDs-Example]
// ! [PhysicalSystem-Get-Example]
				const ACAPI::Result<ACAPI::MEP::PhysicalSystem> system { ACAPI::MEP::PhysicalSystem::Get (systemID) };
				if (system.IsErr ())
					return system.UnwrapErr ().kind;
// ! [PhysicalSystem-Get-Example]

// ! [PhysicalSystem-GetRootElementId-Example]
				const std::optional<ACAPI::MEP::UniqueID> rootElementId = system.Unwrap ().GetRootElementId ();
				if (!rootElementId.has_value ())
					continue;

				const ACAPI::Result<ACAPI::MEP::Element> element { ACAPI::MEP::Element::Get (rootElementId.value ()) };
				if (element.IsErr ())
					return element.UnwrapErr ().kind;
// ! [PhysicalSystem-GetRootElementId-Example]
				
// ! [SystemGroup-GetName-Example]
// ! [PhysicalSystem-GetName-Example]
				GS::UniString labelContent { GS::UniString::SPrintf ("Group: %T, System: %T", systemGroup->GetName (), system->GetName ()) };
				const GS::GSErrCode err { CreateLabelOnElement (element.Unwrap (), std::move (labelContent)) };
				if (err != NoError)
					return err;
// ! [SystemGroup-GetName-Example]
// ! [PhysicalSystem-GetName-Example]
			}
		}

		return NoError;
	});
}

} // namespace MEPExample
