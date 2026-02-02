#include "Property_Registration_Test.hpp"
#include "ExampleRunner.hpp"
#include "APIdefs_Callback.h"
#include "ACAPinc.h"
#include "Definitions.hpp"
#include "RS.hpp"
#include "UniString.hpp"
#include "GSAssert.hpp"
#include "GSDebug.hpp"

/**********************************************************************************************************************/


GSErrCode MenuCommandHandler (const API_MenuParams* menuParams)
{
	PRECOND (menuParams != nullptr);

	switch (menuParams->menuItemRef.menuResID) {
	case APIDevKitExamples::PropertyRegistration::ExampleRunnerResId:
	{
		switch (menuParams->menuItemRef.itemIndex) {
		case APIDevKitExamples::PropertyRegistration::ExampleRunnerMenuId:
			APIDevKitExamples::PropertyRegistration::ExampleRunner::Toggle ();
			return NoError;
		default:
			GS::Unreachable ();
		}
	}
	default:
		GS::Unreachable ();
	}
}


API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	PRECOND (envir != nullptr);

	constexpr auto s_addOnStringsID = 32000;
	constexpr auto s_addOnStringsID_name = 1;
	constexpr auto s_addOnStringsID_description = 2;

	RSGetIndString (&envir->addOnInfo.name, s_addOnStringsID, s_addOnStringsID_name, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, s_addOnStringsID, s_addOnStringsID_description, ACAPI_GetOwnResModule ());

	return APIAddon_Normal;
}


GSErrCode RegisterInterface ()
{
	if (const auto err = ACAPI_MenuItem_RegisterMenu (APIDevKitExamples::PropertyRegistration::ExampleRunnerResId, 0, MenuCode_UserDef, MenuFlag_Default); err != NoError)
		return err;

	return NoError;
}


GSErrCode Initialize ()
{
	if (const auto err = ACAPI_MenuItem_InstallMenuHandler (APIDevKitExamples::PropertyRegistration::ExampleRunnerResId, MenuCommandHandler); err != NoError)
		return err;
	return APIDevKitExamples::PropertyRegistration::ExampleRunner::RegisterPaletteControlCallBack ();
}


GSErrCode FreeData ()
{
	return NoError;
}


/**********************************************************************************************************************/
