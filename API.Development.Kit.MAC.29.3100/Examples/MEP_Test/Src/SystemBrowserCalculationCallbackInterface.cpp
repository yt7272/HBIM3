#include "SystemBrowserCalculationCallbackInterface.hpp"
#include "Reporter.hpp"

// API
#include "ACAPinc.h"

// ACAPI
#include "ACAPI/Result.hpp"

// MEPAPI
#include "ACAPI/MEPDistributionSystemsGraph.hpp"
#include "ACAPI/MEPDistributionSystem.hpp"
#include "ACAPI/MEPSystemBrowserCalculationCallbackInterface.hpp"

// STL
#include <vector>
#include <random>


namespace {

bool UseMetricElseImperial ()
{
	API_WorkingUnitPrefs workingUnitPrefs = {};
	ACAPI_ProjectSetting_GetPreferences (&workingUnitPrefs, APIPrefs_WorkingUnitsID);

	switch (workingUnitPrefs.lengthUnit) {
		case API_LengthTypeID::KiloMeter:
		case API_LengthTypeID::Meter:
		case API_LengthTypeID::Decimeter:
		case API_LengthTypeID::Centimeter:
		case API_LengthTypeID::Millimeter:
			return true;

		default:
			GSASSERT (workingUnitPrefs.lengthUnit == API_LengthTypeID::FootFracInch ||
					  workingUnitPrefs.lengthUnit == API_LengthTypeID::FootDecInch ||
					  workingUnitPrefs.lengthUnit == API_LengthTypeID::DecFoot ||
					  workingUnitPrefs.lengthUnit == API_LengthTypeID::FracInch ||
					  workingUnitPrefs.lengthUnit == API_LengthTypeID::Yard ||
					  workingUnitPrefs.lengthUnit == API_LengthTypeID::DecInch);
			return false;
	}
}



// ! [TypedCalculationResultColumn_Example]
class TestColumn1 : public ACAPI::MEP::TypedCalculationResultColumn<GS::UniString>
{
public:
	// Inherited via TypedCalculationResultColumn
	GS::UniString GetId () const override
	{
		return "TST1";
	}

	GS::UniString GetTitle () const override
	{
		return "Test1";
	}

	std::optional<GS::UniString> GetUnitToDisplay () const override
	{
		if (UseMetricElseImperial ())
			return "kg";
		else
			return "lb";
	}

	GS::Int32 GetDefaultWidth () const override
	{
		return 80;
	}

	bool IsLess (const std::any&, const std::any&) const override
	{
		return false;
	}

	bool IsEqual (const std::any&, const std::any&) const override
	{
		return false;
	}

	GS::UniString FormatValueToDisplayText (const GS::UniString& value) const override
	{
		return value;
	}
};
// ! [TypedCalculationResultColumn_Example]

// ! [SystemBrowserCalculationCallbackInterface_Example]
class TestCalculationCallback : public ACAPI::MEP::SystemBrowserCalculationCallbackInterface {
	public:
		TestColumn1 testColumn;

		TestCalculationCallback () = default;

		// ! [SystemBrowserCalculationCallbackInterface_RegisterCalculatedColumns_Example]
		virtual std::vector<const ACAPI::MEP::ICalculationResultColumn*> RegisterCalculatedColumns () override
		{
			return { &testColumn };
		}
		// ! [SystemBrowserCalculationCallbackInterface_RegisterCalculatedColumns_Example]

		// ! [SystemBrowserCalculationCallbackInterface_CalculationsRequested_Example]
		virtual void CalculationsRequested (const GS::Guid& root, const ACAPI::MEP::DistributionSystemsGraph& graph) override
		{
			Reporter reporter;
			reporter.Add (GS::UniString::Printf ("Calculation callback was called on root element %T!", root.ToUniString ().ToPrintf ()));
			reporter.AddNewLine ();
			reporter.Add (GS::UniString::Printf ("In a graph with %zu systems!", graph.GetSystems ().size ()));
			reporter.AddNewLine ();
			reporter.Write ();
		}
		// ! [SystemBrowserCalculationCallbackInterface_CalculationsRequested_Example]

		virtual ~TestCalculationCallback () = default;
	};
// ! [SystemBrowserCalculationCallbackInterface_Example]


}


namespace MEPExample {

GSErrCode UseCalculationInterface ()
{
	Reporter reporter;

	// ! [RegisterCalculationCallback_Example]
	TestCalculationCallback testCallback;
	ACAPI::Result<void> result = ACAPI::MEP::RegisterCalculationCallback (testCallback);
	if (result.IsErr ())
		return result.UnwrapErr ().kind;
	// ! [RegisterCalculationCallback_Example]
	
	reporter.Add ("Calculation callback registered");
	reporter.AddNewLine ();

	// Fetch all the elements shown in the system browser by fetching all elements from the current distribution systems graph
	std::vector<ACAPI::MEP::UniqueID> graphElements;
	const auto newGraph = ACAPI::MEP::CreateDistributionSystemsGraph ();
	if (newGraph.IsOk ())
		graphElements = newGraph->GetElements ();

	// ! [SystemBrowserCalculationCallbackInterface_SubmitCalculations_Example]
	ACAPI::MEP::SystemBrowserCalculationCallbackInterface::CalculationResultData changedElements;

	// Generate some random values to display in the system browser:
	auto genRandomNum = [gen = std::default_random_engine {}, dist = std::uniform_real_distribution<> { 0.1, 100.0 }] () mutable
		{
			return dist (gen);
		};

	for (const ACAPI::MEP::UniqueID& element : graphElements) {
		changedElements.emplace_back (ACAPI::MEP::SystemBrowserCalculationCallbackInterface::ColumnRowEntry { element.GetGuid (), testCallback.testColumn.GetId (), genRandomNum () });
	}

	testCallback.SubmitCalculations (changedElements);
	// ! [SystemBrowserCalculationCallbackInterface_SubmitCalculations_Example]

	reporter.Add ("SubmitCalculations called");
	reporter.AddNewLine ();

	reporter.Write ();
	return NoError;
}


} // namespace MEPExample
