/**
 * @file Contains the functions required by Archicad.
 */

#include	"APIEnvir.h"

/**
 * Dependency definitions.
 * @param envir [in] Archicad environment values.
 * @return The Add-On loading type.
 */
API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name       , 32000, 1, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, 32000, 2, ACAPI_GetOwnResModule ());

	return APIAddon_Normal;
}		// CheckEnvironment ()

/**
 * Interface definitions.
 * @return Archicad error code.
 */
GSErrCode RegisterInterface (void)
{
	GSErrCode err = NoError;

	return err;
}		// RegisterInterface

/**
 * Called after the Add-On has been loaded into memory.
 * @return Archicad error code.
 */
GSErrCode Initialize (void)
{
	GSErrCode err = NoError;

	return err;
}		// Initialize

/**
 * Called when the Add-On is going to be unloaded.
 * @return Archicad error code.
 */
GSErrCode FreeData (void)

{
	GSErrCode err = NoError;

	return err;
}		// FreeData
