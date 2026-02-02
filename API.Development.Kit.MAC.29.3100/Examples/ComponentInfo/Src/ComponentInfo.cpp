#include "ComponentInfo.hpp"
#include "ComponentInfoDialog.hpp"
#include "APICommon.h"
#include "APIdefs_Properties.h"
#include "DGModule.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "Folder.hpp"
#include "BM.hpp"

static const GS::UniString kGroupName        = "构件扩展信息";
static const GS::UniString kCompIdName       = "构件编号";
static const GS::UniString kPhotoDescName    = "照片说明";
static const GS::UniString kPhotoPathsName   = "照片路径";
static const GS::UniString kPhotoFolderSuffix = "_构件照片";

static GSErrCode FindOrCreateGroup (API_PropertyGroup& outGroup)
{
	GS::Array<API_PropertyGroup> groups;
	GSErrCode err = ACAPI_Property_GetPropertyGroups (groups);
	if (err != NoError) return err;
	for (UInt32 i = 0; i < groups.GetSize (); ++i) {
		if (groups[i].name == kGroupName) {
			outGroup = groups[i];
			return NoError;
		}
	}
	outGroup.guid = APINULLGuid;
	outGroup.name = kGroupName;
	outGroup.description = "构件编号、照片说明、照片路径";
	err = ACAPI_Property_CreatePropertyGroup (outGroup);
	return err;
}

static GSErrCode FindOrCreateDefinition (const API_PropertyGroup& group, const GS::UniString& name, API_PropertyDefinition& outDef, API_VariantType valueType, API_PropertyCollectionType collType)
{
	GS::Array<API_PropertyDefinition> defs;
	GSErrCode err = ACAPI_Property_GetPropertyDefinitions (group.guid, defs);
	if (err != NoError) return err;
	for (UInt32 i = 0; i < defs.GetSize (); ++i) {
		if (defs[i].name == name) {
			outDef = defs[i];
			return NoError;
		}
	}
	outDef = {};
	outDef.guid = APINULLGuid;
	outDef.groupGuid = group.guid;
	outDef.name = name;
	outDef.collectionType = collType;
	outDef.valueType = valueType;
	outDef.measureType = API_PropertyDefaultMeasureType;
	outDef.canValueBeEditable = true;
	outDef.defaultValue.basicValue.variantStatus = API_VariantStatusNormal;
	if (collType == API_PropertySingleCollectionType)
		outDef.defaultValue.basicValue.singleVariant.variant.type = valueType;
	return ACAPI_Property_CreatePropertyDefinition (outDef);
}

GSErrCode ComponentInfo::EnsurePropertyGroupAndDefinitions (PropertyDefinitionGuids& outGuids)
{
	API_PropertyGroup group;
	GSErrCode err = FindOrCreateGroup (group);
	if (err != NoError) return err;
	outGuids.groupGuid = group.guid;

	API_PropertyDefinition defCompId, defDesc, defPaths;
	err = FindOrCreateDefinition (group, kCompIdName, defCompId, API_PropertyStringValueType, API_PropertySingleCollectionType);
	if (err != NoError) return err;
	outGuids.compIdGuid = defCompId.guid;

	err = FindOrCreateDefinition (group, kPhotoDescName, defDesc, API_PropertyStringValueType, API_PropertySingleCollectionType);
	if (err != NoError) return err;
	outGuids.photoDescGuid = defDesc.guid;

	err = FindOrCreateDefinition (group, kPhotoPathsName, defPaths, API_PropertyStringValueType, API_PropertyListCollectionType);
	if (err != NoError) return err;
	outGuids.photoPathsGuid = defPaths.guid;

	return NoError;
}

GSErrCode ComponentInfo::GetProjectPhotoFolder (IO::Location& outPhotoFolderLoc, IO::Location& outProjectFolderLoc, GS::UniString& outProjectName)
{
	API_ProjectInfo pi;
	GSErrCode err = ACAPI_ProjectOperation_Project (&pi);
	if (err != NoError) return err;
	if (pi.untitled) {
		ACAPI_WriteReport ("Project must be saved before adding component photos.", true);
		return APIERR_NOPLAN;
	}
	IO::Location* fileLoc = pi.teamwork ? pi.location_team : pi.location;
	if (!fileLoc || fileLoc->IsEmpty ()) return APIERR_NOPLAN;

	outProjectFolderLoc = *fileLoc;
	err = outProjectFolderLoc.DeleteLastLocalName ();
	if (err != NoError) return err;

	outProjectName = (pi.projectName && !pi.projectName->IsEmpty ()) ? *pi.projectName : "Project";
	GS::UniString folderName = outProjectName + kPhotoFolderSuffix;
	outPhotoFolderLoc = outProjectFolderLoc;
	err = outPhotoFolderLoc.AppendToLocal (IO::Name (folderName));
	if (err != NoError) return err;

	err = IO::fileSystem.CreateFolder (outPhotoFolderLoc);
	if (err != NoError && err != (GSErrCode) IO::FileSystem::TargetExists) return err;
	return NoError;
}

GSErrCode ComponentInfo::CopyPhotosToProjectFolder (const IO::Location& photoFolderLoc, const GS::UniString& projectName, const GS::Array<IO::Location>& sourceLocs, GS::Array<GS::UniString>& outRelPaths)
{
	GS::UniString prefix = projectName + kPhotoFolderSuffix + "/";
	outRelPaths.Clear ();

	for (UInt32 i = 0; i < sourceLocs.GetSize (); ++i) {
		IO::Name fileName;
		if (sourceLocs[i].GetLastLocalName (&fileName) != NoError) continue;
		IO::Location dst = photoFolderLoc;
		if (dst.AppendToLocal (fileName) != NoError) continue;
		GSErrCode err = IO::fileSystem.Copy (sourceLocs[i], dst);
		if (err != NoError) {
			ACAPI_WriteReport ("Failed to copy photo: %s", true, sourceLocs[i].ToDisplayText ().ToCStr ().Get ());
			continue;
		}
		outRelPaths.Push (prefix + fileName.ToString ());
	}
	return NoError;
}

GSErrCode ComponentInfo::WritePropertiesToElement (API_Guid elemGuid, const PropertyDefinitionGuids& guids, const GS::UniString& compId, const GS::UniString& photoDesc, const GS::Array<GS::UniString>& relPaths)
{
	API_Property p;
	p.status = API_Property_HasValue;

	p.definition.guid = guids.compIdGuid;
	p.value.variantStatus = API_VariantStatusNormal;
	p.value.singleVariant.variant.type = API_PropertyStringValueType;
	p.value.singleVariant.variant.uniStringValue = compId;
	GSErrCode err = ACAPI_Element_SetProperty (elemGuid, p);
	if (err != NoError) return err;

	p.definition.guid = guids.photoDescGuid;
	p.value.singleVariant.variant.uniStringValue = photoDesc;
	err = ACAPI_Element_SetProperty (elemGuid, p);
	if (err != NoError) return err;

	p.definition.guid = guids.photoPathsGuid;
	p.value.listVariant.variants.Clear ();
	for (UInt32 i = 0; i < relPaths.GetSize (); ++i) {
		API_Variant v;
		v.type = API_PropertyStringValueType;
		v.uniStringValue = relPaths[i];
		p.value.listVariant.variants.Push (v);
	}
	return ACAPI_Element_SetProperty (elemGuid, p);
}

GSErrCode ComponentInfo::GetPhotoPathsFromElement (API_Guid elemGuid, API_Guid photoPathsDefGuid, GS::Array<GS::UniString>& outRelPaths)
{
	API_Property p = {};
	GSErrCode err = ACAPI_Element_GetPropertyValue (elemGuid, photoPathsDefGuid, p);
	if (err != NoError || p.status != API_Property_HasValue || p.value.variantStatus != API_VariantStatusNormal) return err;
	outRelPaths.Clear ();
	for (UInt32 i = 0; i < p.value.listVariant.variants.GetSize (); ++i) {
		if (p.value.listVariant.variants[i].type == API_PropertyStringValueType)
			outRelPaths.Push (p.value.listVariant.variants[i].uniStringValue);
	}
	return NoError;
}

void ComponentInfo::RunComponentInfoCommand ()
{
	PropertyDefinitionGuids guids;
	GSErrCode err = EnsurePropertyGroupAndDefinitions (guids);
	if (err != NoError) { ErrorBeep ("EnsurePropertyGroupAndDefinitions", err); return; }

	API_SelectionInfo selectionInfo = {};
	GS::Array<API_Neig> selNeigs;
	err = ACAPI_Selection_Get (&selectionInfo, &selNeigs, true);
	BMKillHandle ((GSHandle*) &selectionInfo.marquee.coords);
	if (err == APIERR_NOSEL || selectionInfo.typeID == API_SelEmpty || selNeigs.IsEmpty ()) {
		ACAPI_WriteReport ("Please select at least one element.", true);
		return;
	}

	IO::Location photoFolderLoc, projectFolderLoc;
	GS::UniString projectName;
	err = GetProjectPhotoFolder (photoFolderLoc, projectFolderLoc, projectName);
	if (err != NoError) return;

	GSResModule resMod = ACAPI_GetOwnResModule ();
	ComponentInfoDialog dlg (resMod, 32510, resMod);

	if (selNeigs.GetSize () > 0) {
		API_Guid first = selNeigs[0].guid;
		API_Property pComp = {}, pDesc = {};
		if (ACAPI_Element_GetPropertyValue (first, guids.compIdGuid, pComp) == NoError && pComp.status == API_Property_HasValue)
			dlg.SetComponentId (pComp.value.singleVariant.variant.uniStringValue);
		if (ACAPI_Element_GetPropertyValue (first, guids.photoDescGuid, pDesc) == NoError && pDesc.status == API_Property_HasValue)
			dlg.SetPhotoDescription (pDesc.value.singleVariant.variant.uniStringValue);
	}

	if (dlg.Invoke () != DG::ModalDialog::Accept) return;

	GS::UniString compId = dlg.GetComponentId ();
	GS::UniString photoDesc = dlg.GetPhotoDescription ();
	GS::Array<IO::Location> photoLocs = dlg.GetSelectedPhotoLocations ();

	GS::Array<GS::UniString> relPaths;
	if (!photoLocs.IsEmpty ()) {
		err = CopyPhotosToProjectFolder (photoFolderLoc, projectName, photoLocs, relPaths);
		if (err != NoError) { ErrorBeep ("CopyPhotosToProjectFolder", err); return; }
	}

	for (UInt32 i = 0; i < selNeigs.GetSize (); ++i) {
		API_Guid elemGuid = selNeigs[i].guid;
		GS::Array<GS::UniString> paths = relPaths;
		if (paths.IsEmpty ())
			GetPhotoPathsFromElement (elemGuid, guids.photoPathsGuid, paths);
		err = WritePropertiesToElement (elemGuid, guids, compId, photoDesc, paths);
		if (err != NoError)
			ACAPI_WriteReport ("SetProperty failed for element: %s", true, ErrID_To_Name (err));
	}
	ACAPI_WriteReport ("Component Info: OK, %d elements updated.", false, (int) selNeigs.GetSize ());
}
