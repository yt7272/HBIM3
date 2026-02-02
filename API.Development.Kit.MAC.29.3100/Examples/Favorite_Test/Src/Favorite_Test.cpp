// *****************************************************************************
// Source code for the Favorite Test Add-On
// *****************************************************************************

#define	_Favorite_Test_TRANSL_
#define	_Geometry_Test_TRANSL_


// ---------------------------------- Includes ---------------------------------

#include	"APIEnvir.h"
#include	"ACAPinc.h"					// also includes APIdefs.h
#include	"APICommon.h"

#include	"ACAPI/Favorite.hpp"
#include	"ACAPI/Library/LibraryManager.hpp"
#include	"ACAPI/Library/LibPart.hpp"

#include	"FileSystem.hpp"
#include	"ProfileVectorImage.hpp"

#include	"UserItemDialog.hpp"

#include "DGFileDialog.hpp"
#include "UDTextInputDialog.hpp"

namespace {


GS::UniString GetTextFromUser (const GS::UniString& title)
{
	UD::TextInputDialog textDialog (title, GS::EmptyUniString, GS::EmptyUniString);
	if (textDialog.Invoke ()) {
		return textDialog.GetText ();
	}
	return GS::EmptyUniString;
}


} // namespace



// -----------------------------------------------------------------------------
// PrintFourCharCode
//	helper function
// -----------------------------------------------------------------------------
static GS::String	PrintFourCharCode (API_ElemVariationID variationID)
{
	GS::String	str;

	str.SetLength (5);
	const char*	fc = (const char*) &variationID;
	for (UInt32 ii = 0; ii < 4; ++ii)
		str[ii] = fc[3-ii];

	return str;
}


// =============================================================================
//
// Main functions
//
// =============================================================================


// -----------------------------------------------------------------------------
// Lists the favorites (name) to the Report Window
//
// -----------------------------------------------------------------------------

static void		Do_ListFavorites (void)
{
	API_ToolBoxInfo	toolboxInfo {};
	GSErrCode		err = NoError;

	err = ACAPI_Toolbox_GetToolBoxInfo (&toolboxInfo, true);
	if (err == NoError) {
		ACAPI_WriteReport ("### Favorite_Test Do_ListFavorites ###", false);

		auto favoriteManager = ACAPI::Favorite::GetFavoriteManager ();
		for (Int32 i = 0; i < toolboxInfo.nTools; ++i) {
			const GS::UniString elemTypeStr = ElemID_To_Name ((*toolboxInfo.data)[i].type);
			ACAPI_WriteReport ("------------------------------------------\nElem type: %s,  VariationID: '%s'\n",
							   false,
							   elemTypeStr.ToCStr ().Get (),
							   PrintFourCharCode ((*toolboxInfo.data)[i].type.variationID).ToCStr ());

			std::vector<ACAPI::Favorite::Favorite> favoritesByElemType = favoriteManager.GetFavoritesByElementType ((*toolboxInfo.data)[i].type);
			for (const auto& favorite : favoritesByElemType) {
				auto name = favorite.GetName ();
				if (name.IsOk ())
					ACAPI_WriteReport ("Name: %s", false, (*name).ToCStr ().Get ());
			}
		}
	} else {
		ACAPI_WriteReport ("APIEnv_GetToolBoxInfoID failed", false);
	}

	BMKillHandle ((GSHandle *) &toolboxInfo.data);

	return;
}		/* Do_ListFavorites */


// -----------------------------------------------------------------------------
// Place instances of the Favorites with column type
//
// -----------------------------------------------------------------------------
static void Do_PlaceFavorite (API_ElemType type, const std::function<void (API_Element&)>& modifier)
{
	GS::UniString			 elemTypeStr = ElemID_To_Name (type);

	ACAPI_WriteReport ("### Favorite_Test Do_PlaceFavorite" + elemTypeStr + "###", false);

	auto favoriteManager = ACAPI::Favorite::GetFavoriteManager ();
	std::vector<ACAPI::Favorite::Favorite> favorites = favoriteManager.GetFavoritesByElementType (type);

	if (favorites.empty ()) {
		ACAPI_WriteReport ("There isn't any favorite " + elemTypeStr + " to place!", false);
		return;
	}

	ACAPI_CallUndoableCommand ("Favorite_Test Do_PlaceFavorite" + elemTypeStr, [&] () -> GSErrCode {
		for (const auto& favorite : favorites) {
			API_Element element = *favorite.GetOldElementDefault ();
			API_ElementMemo memo = *favorite.GetMemo ();
			modifier (element);

			GSErrCode err1 = ACAPI_Element_Create (&element, &memo);

			if (err1 != NoError)
				ACAPI_WriteReport ("ACAPI_Element_Create failed", false);
			else
				ACAPI_WriteReport ("ACAPI_Element_Create successfully placed new element by favorite:\n  Name: %s, Elem type: %s,  VariationID: '%s'",
								   false,
								   favorite.GetName ()->ToCStr ().Get (),
								   elemTypeStr.ToCStr ().Get (),
								   PrintFourCharCode (element.header.type.variationID).ToCStr ());

			// Must set classifications before properties
			for (const GS::Pair<API_Guid, API_Guid>& classification : *favorite.GetClassifications ()) {
				ACAPI_Element_AddClassificationItem (element.header.guid, classification.second);
			}

			for (const API_ElemCategoryValue& categoryValue : *favorite.GetElemCategoryValues ()) {
				ACAPI_Category_SetCategoryValue (element.header.guid, categoryValue.category, categoryValue);
			}

			GS::Array<API_Property> properties;
			for (const API_Property& property : *favorite.GetProperties ()) {
				properties.Push (property);
			}

			ACAPI_Element_SetProperties (element.header.guid, properties);
			ACAPI_DisposeElemMemoHdls (&memo);

			if (err1 != NoError)
				return err1;
		}
		return NoError;
	});

}


static void		Do_PlaceFavoriteColumn (void)
{
	Int32 index = 0;
	return Do_PlaceFavorite (API_ColumnID, [&index] (API_Element& element) {
		element.column.origoPos.x = index * 2.0;
		index++;
	});
}		/* Do_PlaceFavoriteColumn */


// -----------------------------------------------------------------------------
// Place instances of the Favorites with column segment type as
// single segment columns
// -----------------------------------------------------------------------------

static void		Do_PlaceFavoriteColumnSegment (void)
{
	GSErrCode					err = NoError;
	GS::UniString				elemTypeStr = ElemID_To_Name (API_ColumnSegmentID);

	auto								   favoriteManager = ACAPI::Favorite::GetFavoriteManager ();
	std::vector<ACAPI::Favorite::Favorite> favorites	   = favoriteManager.GetFavoritesByElementType (API_ColumnSegmentID);

	if (favorites.empty ()) {
		ACAPI_WriteReport ("There isn't any favorite column segment to place!", false);
		return;
	}

	ACAPI_CallUndoableCommand ("Favorite_Test Do_PlaceFavoriteColumnSegment", [&] () -> GSErrCode {
		Int32 ii = 0;
		for (const auto& favorite : favorites) {
			API_Element element {};

			element.header.type = API_ColumnID;
			err					= ACAPI_Element_GetDefaults (&element, nullptr);
			if (err != NoError) {
				ACAPI_WriteReport ("ACAPI_Element_GetDefaults failed", false);
				return err;
			}

			API_ElementMemo memo {};
			API_ElementMemo favoriteMemo = favorite.GetMemo ().Unwrap ();

			element.column.origoPos.x = ii * 2.0;

			element.column.nSegments = 1;
			memo.columnSegments		 = reinterpret_cast<API_ColumnSegmentType*> (BMAllocatePtr (sizeof (API_ColumnSegmentType), ALLOCATE_CLEAR, 0));
			memo.columnSegments[0]	 = favorite.GetOldElementDefault ()->columnSegment;

			element.column.nCuts = 2;
			memo.assemblySegmentCuts =
				reinterpret_cast<API_AssemblySegmentCutData*> (BMAllocatePtr (2 * sizeof (API_AssemblySegmentCutData), ALLOCATE_CLEAR, 0));
			memo.assemblySegmentCuts[0].cutType = APIAssemblySegmentCut_Horizontal;
			memo.assemblySegmentCuts[1].cutType = APIAssemblySegmentCut_Horizontal;

			element.column.nSchemes = 1;
			memo.assemblySegmentSchemes =
				reinterpret_cast<API_AssemblySegmentSchemeData*> (BMAllocatePtr (sizeof (API_AssemblySegmentSchemeData), ALLOCATE_CLEAR, 0));
			memo.assemblySegmentSchemes[0].lengthType		= APIAssemblySegment_Proportional;
			memo.assemblySegmentSchemes[0].lengthProportion = 1.0;

			element.column.nProfiles = 0;
			if (favoriteMemo.customOrigProfile != nullptr) {
				element.column.nProfiles = 1;
				memo.assemblySegmentProfiles =
					reinterpret_cast<API_AssemblySegmentProfileData*> (BMAllocatePtr (sizeof (API_AssemblySegmentProfileData), ALLOCATE_CLEAR, 0));
				memo.assemblySegmentProfiles[0].segmentIndex	   = 0;
				memo.assemblySegmentProfiles[0].customOrigProfile  = new ProfileVectorImage ();
				*memo.assemblySegmentProfiles[0].customOrigProfile = *favoriteMemo.customOrigProfile;

				if (favoriteMemo.stretchedProfile != nullptr) {
					memo.assemblySegmentProfiles[0].stretchedProfile  = new ProfileVectorImage ();
					*memo.assemblySegmentProfiles[0].stretchedProfile = *favoriteMemo.stretchedProfile;
				}
			}

			GSErrCode err = ACAPI_Element_Create (&element, &memo);

			if (err != NoError)
				ACAPI_WriteReport ("ACAPI_Element_Create failed", false);
			else
				ACAPI_WriteReport ("ACAPI_Element_Create successfully placed new element by favorite:\n  Name: %s, Elem type: %s,  VariationID: '%s'",
								   false,
								   favorite.GetName ()->ToCStr ().Get (),
								   elemTypeStr.ToCStr ().Get (),
								   PrintFourCharCode (APIVarId_Generic).ToCStr ());

			ACAPI_DisposeElemMemoHdls (&memo);
			ACAPI_DisposeElemMemoHdls (&favoriteMemo);
			if (err != NoError)
				return err;
			ii++;
		}
		return NoError;
	});
}		/* Do_PlaceFavoriteColumnSegment */


// -----------------------------------------------------------------------------
// Create favorite from the clicked element
//
// -----------------------------------------------------------------------------

static void		Do_CreateFavorite (void)
{
	API_ElemType		type;
	API_Guid			guid;
	GSErrCode			err = NoError;

	ACAPI_WriteReport ("### Favorite_Test Do_CreateFavorite ###", false);

	if (!ClickAnElem ("Click an elem to create new favorite", API_ZombieElemID, nullptr, &type, &guid)) {
		WriteReport_Alert ("No element was clicked");
		return;
	}

	API_Element element;
	element.header.type = type;
	element.header.guid = guid;
	err = ACAPI_Element_Get (&element);
	if (err != NoError) {
		ACAPI_WriteReport ("ACAPI_Element_Get failed", false);
		return;
	}

	GS::Array<GS::Pair<API_Guid, API_Guid>> classificationsArray;
	err = ACAPI_Element_GetClassificationItems (guid, classificationsArray);
	if (err != NoError) {
		ACAPI_WriteReport ("ACAPI_Element_GetClassificationItems failed", false);
		return;
	}
	std::vector<GS::Pair<API_Guid, API_Guid>> classifications;
	for (const auto& classification : classificationsArray) {
		classifications.push_back (classification);
	}

	GS::Array<API_PropertyDefinition> definitions;
	err = ACAPI_Element_GetPropertyDefinitions (guid, API_PropertyDefinitionFilter_UserDefined, definitions);
	if (err != NoError) {
		ACAPI_WriteReport ("ACAPI_Element_GetPropertyDefinitions failed", false);
		return;
	}

	GS::Array<API_Property> properties;
	err = ACAPI_Element_GetPropertyValues (guid, definitions, properties);
	if (err != NoError) {
		ACAPI_WriteReport ("ACAPI_Element_GetProperties failed", false);
		return;
	}
    for (UIndex ii = properties.GetSize (); ii >= 1; --ii) {
        const API_Property& p = properties.Get (ii - 1);
        if (p.isDefault || p.definition.canValueBeEditable == false || p.status != API_Property_HasValue)
            properties.Delete (ii - 1);
    }

	std::vector<API_Property> propertiesVector;
	propertiesVector.reserve (properties.GetSize ());
	for (const auto& property : properties) {
		propertiesVector.push_back (property);
	}

	API_ElementMemo memo {};

	err = ACAPI_Element_GetMemo	(guid, &memo);
	if (err != NoError) {
		ACAPI_WriteReport ("ACAPI_Element_GetMemo failed", false);
		return;
	}
	if (memo.elemInfoString != nullptr)
		delete memo.elemInfoString;

    memo.elemInfoString = new GS::UniString ("My favorite ID");
	GS::UniString favoriteName (u"Favorite_Test"sv);

	auto favoriteManager = ACAPI::Favorite::GetFavoriteManager ();
	while (favoriteManager.GetFavoriteByName (favoriteName).IsOk ()) {
		favoriteName.Append (u"_1"sv);
	}
	ACAPI_CallCommand ("Favorite_Test Do_CreateFavorite", [&] () -> GSErrCode {
		return favoriteManager.CreateFavorite (
			favoriteName,
			*favoriteManager.GetRootFolder (),
			element,
			std::nullopt,
			memo,
			std::nullopt,
			std::nullopt,
			propertiesVector,
			classifications,
			std::nullopt
		).UnwrapErr ().kind;
	});
	if (err != NoError) {
		if (err == APIERR_BADNAME)
			ACAPI_WriteReport ("ACAPI_Favorite_Create failed because bad name was given (maybe already in used)", false);
		else
			ACAPI_WriteReport ("ACAPI_Favorite_Create failed", false);
	} else {
		ACAPI_WriteReport ("ACAPI_Favorite_Create successfully created new favorite:", false);
		ACAPI_WriteReport ("Name: %s, Elem type: %s", false,
						   favoriteName.ToCStr ().Get (),
						   ElemID_To_Name (type).ToCStr ().Get ());
	}

	ACAPI_DisposeElemMemoHdls (&memo);

	return;
}		/* Do_CreateFavorite */


// -----------------------------------------------------------------------------
// Change favorite named "Favorite_Test"
//	- set it's ID string to "[Changed]"
// -----------------------------------------------------------------------------

static void		Do_ChangeFavorite (void)
{
	// Get a favorite name from the user
	GS::UniString favoriteName = GetTextFromUser ("Enter the name of the favorite to change");

	ACAPI_WriteReport ("### Favorite_Test Do_ChangeFavorite ###", false);
	// Create the favoriteManager, and get the favorite by name
	auto favoriteManager = ACAPI::Favorite::GetFavoriteManager ();
	auto favoriteResult = favoriteManager.GetFavoriteByName (favoriteName);

	// Check if the favorite is valid
	if (favoriteResult.IsErr ()) {
		ACAPI_WriteReport ("%s favorite not found", false, favoriteName.ToCStr ().Get ());
		return;
	}

	ACAPI_CallCommand ("Favorite_Test Do_ChangeFavorite", [&] () -> GSErrCode {
		const auto favorite	  = favoriteResult.Unwrap ();
		auto elementMemoResult = favorite.GetMemo ();
		if (elementMemoResult.IsErr ()) {
			ACAPI_WriteReport ("GetOldElementDefault failed", false);
			return elementMemoResult.UnwrapErr ().kind;
		}

		API_Element elemDefault = *favorite.GetOldElementDefault ();
		API_ElementMemo memo = *favorite.GetMemo ();
		auto subElems = *favorite.GetSubElements ();
		auto elementMarker = *favorite.GetElementMarker ();
		auto memoMarker = *favorite.GetMemoMarker ();
		auto properties = *favorite.GetProperties ();
		auto classifications = *favorite.GetClassifications ();
		auto elemCategoryValues = *favorite.GetElemCategoryValues ();

		memo.elemInfoString = new GS::UniString ("[Changed]");
		auto changeResult = favoriteManager.ChangeFavorite (
			favorite,
			elemDefault,
			subElems,
			memo,
			elementMarker,
			memoMarker,
			properties,
			classifications,
			elemCategoryValues
		);


		if (changeResult.IsOk ()) {
			ACAPI_WriteReport ("ACAPI_Favorite_Rename successfully", false);
			return NoError;
		} else {
			ACAPI_WriteReport ("ACAPI_Favorite_Rename failed:", false);
			ACAPI_WriteReport ("Error message: %s", false, changeResult.UnwrapErr ().text.c_str ());
			return changeResult.UnwrapErr ().kind;
		}
	});
}		/* Do_ChangeFavorite */


// -----------------------------------------------------------------------------
// Rename favorite named "Favorite_Test"
//	- appends "[Renamed]" to it's name
// -----------------------------------------------------------------------------

static void		Do_RenameFavorite (void)
{
	// Get a favorite name from the user
	GS::UniString favoriteName = GetTextFromUser ("Enter the name of the favorite to change");
	ACAPI_WriteReport ("### Favorite_Test Do_RenameFavorite ###", false);

	// Create the favoriteManager, and get the favorite by name
	auto favoriteManager = ACAPI::Favorite::GetFavoriteManager ();
	auto favoriteResult	 = favoriteManager.GetFavoriteByName (favoriteName);

	// Check if the favorite is valid
	if (favoriteResult.IsErr ()) {
		ACAPI_WriteReport ("%s favorite not found", false, favoriteName.ToCStr ().Get ());
		return;
	}

	ACAPI_CallCommand ("Favorite_Test Do_RenameFavorite", [&] () -> GSErrCode {
		auto favorite	  = favoriteResult.Unwrap ();
		auto renameResult = favoriteManager.RenameFavorite (favorite, GetTextFromUser ("Enter the new name of the favorite"));

		if (renameResult.IsOk ()) {
			ACAPI_WriteReport ("ACAPI_Favorite_Rename successfully", false);
			return NoError;
		} else {
			ACAPI_WriteReport ("ACAPI_Favorite_Rename failed:", false);
			ACAPI_WriteReport ("Error message: %s", false, renameResult.UnwrapErr ().text.c_str ());
			return renameResult.UnwrapErr ().kind;
		}
	});
}		/* Do_RenameFavorite */


// -----------------------------------------------------------------------------
// Delete favorite named "Favorite_Test"
//
// -----------------------------------------------------------------------------

static void		Do_DeleteFavorite (void)
{
	GS::UniString		name = GetTextFromUser ("Enter the name of the favorite to delete");

	ACAPI_WriteReport ("### Favorite_Test Do_DeleteFavorite ###", false);

	auto favoriteManager = ACAPI::Favorite::GetFavoriteManager ();
	auto favoriteResult = favoriteManager.GetFavoriteByName (name);
	if (favoriteResult.IsErr ()) {
		ACAPI_WriteReport ("%s favorite not found", false, name.ToCStr ().Get ());
		return;
	}

	ACAPI_CallCommand ("Favorite_Test Do_DeleteFavorite",[&] () -> GSErrCode {
		auto result = favoriteManager.DeleteFavorite (favoriteResult.Unwrap ());
		if (result.IsOk ()) {
			ACAPI_WriteReport ("DeleteFavorite successfully deleted favorite named \"%s\"", false, name.ToCStr ().Get ());
		} else {
			ACAPI_WriteReport ("DeleteFavorite failed:", false);
			if (result.UnwrapErr ().kind == APIERR_BADNAME) {
				ACAPI_WriteReport ("Bad name was given", false);
			} else if (result.UnwrapErr ().kind == APIERR_NOTMINE) {
				ACAPI_WriteReport ("The given favorite does not belong to the client", false);
			} else {
				ACAPI_WriteReport ("DeleteFavorite failed", false);
			}
		}
		return result.UnwrapErr ().kind;
	});
	return;
}		/* Do_DeleteFavorite */


// -----------------------------------------------------------------------------
// List the favorite sub elements library part names. 
// Currently only the Curtain Wall is supported.
//
// -----------------------------------------------------------------------------

static void		Do_ListCurtainWallFavoriteSubElements (void)
{
	ACAPI_WriteReport ("### Favorite_Test Do_ListCurtainWallFavoriteSubElements ###", false);

	auto favoriteManager = ACAPI::Favorite::GetFavoriteManager ();
	auto libraryManager = ACAPI::Library::GetLibraryManager ();
	std::vector<ACAPI::Favorite::Favorite> favorites = favoriteManager.GetFavoritesByElementType (API_CurtainWallID);

	for (const auto& favorite : favorites) {
		auto subElems = favorite.GetSubElements ();
		if (subElems.IsErr ())
			continue;
		
		for (auto& subElement : subElems.Unwrap ()) {
			ACAPI::Result<ACAPI::Library::LibPart> libPart = libraryManager->GetLibPartByLibInd (subElement.subElem.object.libInd);

			if (subElement.subType == APISubElement_CWFrameClass) {
				ACAPI_WriteReport ("Frame with library part name: %s", false, libPart.IsOk () ? libPart->GetName ()->ToCStr ().Get () : "Unknown");
			}

			if (subElement.subType == APISubElement_CWPanelClass) {
				ACAPI_WriteReport ("Panel with library part name: %s", false, libPart.IsOk () ? libPart->GetName ()->ToCStr ().Get () : "Unknown");
			}

			ACAPI_DisposeElemMemoHdls (&subElement.memo);
		}
	}
}		/* Do_ListCurtainWallFavoriteSubElements */


// -----------------------------------------------------------------------------
// Import and export favorites"
//
// -----------------------------------------------------------------------------

static void Do_ImportExportFavorites (void)
{
	IO::Location fileLocation;
	IO::fileSystem.GetSpecialLocation (IO::FileSystem::TemporaryFolder, &fileLocation);
	fileLocation.AppendToLocal (IO::Name ("favorite_test_file.prf"));

	auto favoriteManager = ACAPI::Favorite::GetFavoriteManager ();
	auto result = favoriteManager.ExportFavoritesToLegacyPRF (favoriteManager.GetAllFavorites (),fileLocation);
	
	if (result.IsErr ()) {
		ACAPI_WriteReport ("ACAPI_Favorite_Export failed", false);
		return;
	}

	ACAPI_CallCommand ("Favorite_Test Do_ImportExportFavorites", [&] () -> GSErrCode {
		auto newFolder = *favoriteManager.CreateFolder (*favoriteManager.GetRootFolder(), "FavoritTest Add-On Folder");
		auto importResult = favoriteManager.ImportFavoritesFromLegacyPRF (fileLocation, ACAPI::Favorite::FavoriteManager::ConflictPolicy::RenameNew, newFolder);
		if (importResult.IsOk ()) {
			ACAPI_WriteReport ("ACAPI_Favorite_Import successfully imported favorites", false);
		} else {
			ACAPI_WriteReport ("ACAPI_Favorite_Import failed", false);
		}
		return importResult.UnwrapErr ().kind;
	});
	IO::fileSystem.Delete (fileLocation);
}

static void Do_ExportReadCreateFavorites (void)
{
	GSErrCode err = NoError;

	IO::Location fileLocation;
	IO::fileSystem.GetSpecialLocation (IO::FileSystem::TemporaryFolder, &fileLocation);
	fileLocation.AppendToLocal (IO::Name ("favorite_test_file.prf"));

	err = ACAPI_Favorite_Export (fileLocation);
	if (err != NoError) {
		ACAPI_WriteReport ("ACAPI_Favorite_Export failed", false);
		return;
	}

	GS::Array<API_Favorite> favorites;
	err = ACAPI_Favorite_Read (fileLocation, &favorites);
	if (err != NoError) {
		ACAPI_WriteReport ("ACAPI_Favorite_Read failed", false);
		return;
	}

	API_FavoriteFolderHierarchy newFolder;
    newFolder.Push ("FavoritTest Add-On Folder 2");

    for (API_Favorite& favorite : favorites) {
        favorite.name += " [Reloaded]";
        favorite.folder = newFolder;
		err = ACAPI_Favorite_Create (favorite);
		if (err != NoError) {
			ACAPI_WriteReport ("ACAPI_Favorite_Create failed", false);
		}
		if (favorite.memo.HasValue ())
			ACAPI_DisposeElemMemoHdls (&favorite.memo.Get ());
		if (favorite.memoMarker.HasValue ())
			ACAPI_DisposeElemMemoHdls (&favorite.memoMarker.Get ());
	}
}


// -----------------------------------------------------------------------------
// Get preview images for Favorite named "Favorite_Test"
//	- draw them into useritems on a new modal dialog
// -----------------------------------------------------------------------------

static void		Do_GetPreviewImagesOfFavorite (void)
{
	ACAPI_WriteReport ("### Favorite_Test Do_GetPreviewImagesOfFavorite ###", false);

	auto favoriteManager = ACAPI::Favorite::GetFavoriteManager ();
	GS::UniString favoriteName = GetTextFromUser ("Enter the name of the favorite to get preview images");

	UserItemDialog dialog (3, 250, 250, [&] (const DG::UserItem& userItem, const UIndex index) {
		NewDisplay::NativeImage nativeImage (userItem.GetClientWidth (), userItem.GetClientHeight (), 32, nullptr, false, 0, true, userItem.GetResolutionFactor ());

		auto testFavorite = favoriteManager.GetFavoriteByName (favoriteName);
		
		if (testFavorite.IsErr ()) {
			ACAPI_WriteReport ("GetFavoriteByName failed because bad name was given", false);
			return nativeImage;
		}

		auto result = testFavorite->GetPreviewImage ((API_ImageViewID)index, nativeImage);
		if (result.IsErr ()) {
			ACAPI_WriteReport ("GetPreviewImage failed", false);
		}

		return nativeImage;
	});
	dialog.Invoke ();
}		/* Do_GetPreviewImagesOfFavorite */


// -----------------------------------------------------------------------------
// Entry points to handle ARCHICAD events
//
// -----------------------------------------------------------------------------

GSErrCode MenuCommandHandler (const API_MenuParams *params)
{
	switch (params->menuItemRef.itemIndex) {
		case 1:
			Do_ListFavorites ();
			break;
		case 2:
			Do_PlaceFavoriteColumn ();
			break;
		case 3:
			Do_PlaceFavoriteColumnSegment ();
			break;
		case 4:
			Do_CreateFavorite ();
			break;
		case 5:
			Do_ChangeFavorite ();
			break;
		case 6:
			Do_RenameFavorite ();
			break;
		case 7:
			Do_DeleteFavorite ();
			break;
		case 8:
			Do_ListCurtainWallFavoriteSubElements ();
			break;
		case 9:
			Do_ImportExportFavorites ();
			break;
		case 10:
			Do_ExportReadCreateFavorites ();
			break;
		case 11:
			Do_GetPreviewImagesOfFavorite ();
			break;
	}

	return NoError;
}		// DoCommand


// =============================================================================
//
// Required functions
//
// =============================================================================


//------------------------------------------------------
// Dependency definitions
//------------------------------------------------------
API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, 32000, 1, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, 32000, 2, ACAPI_GetOwnResModule ());

	return APIAddon_Normal;
}		/* CheckEnvironment */


//------------------------------------------------------
// Interface definitions
//------------------------------------------------------
GSErrCode RegisterInterface (void)
{
	ACAPI_MenuItem_RegisterMenu (32500, 0, MenuCode_UserDef, MenuFlag_Default);

	return NoError;
}		/* RegisterInterface */


//------------------------------------------------------
// Called when the Add-On has been loaded into memory
// to perform an operation
//------------------------------------------------------
GSErrCode Initialize	(void)
{
	GSErrCode err = ACAPI_MenuItem_InstallMenuHandler (32500, MenuCommandHandler);
	if (err != NoError)
		DBPrintf ("Favorite_Test:: Initialize() ACAPI_MenuItem_InstallMenuHandler failed\n");

	return err;
}		/* Initialize */


// -----------------------------------------------------------------------------
// FreeData
//		called when the Add-On is going to be unloaded
// -----------------------------------------------------------------------------

GSErrCode FreeData (void)
{
	return NoError;
}		// FreeData
