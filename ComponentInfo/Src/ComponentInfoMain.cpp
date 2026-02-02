// *****************************************************************************
// File:			ComponentInfoMain.cpp
// Description:		Component Info Add-on: CheckEnvironment, RegisterInterface,
//					Initialize, FreeData, APIMenuCommandProc_Main
// Project:			ComponentInfo
// *****************************************************************************

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "ComponentInfo.hpp"
#include "ComponentInfoPalette.hpp"

static GSErrCode APIMenuCommandProc_Main (const API_MenuParams* menuParams);

// -----------------------------------------------------------------------------
// Add-on entry: CheckEnvironment
// -----------------------------------------------------------------------------
API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, 32000, 1, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, 32000, 2, ACAPI_GetOwnResModule ());
	return APIAddon_Normal;
}

// -----------------------------------------------------------------------------
// RegisterInterface
// -----------------------------------------------------------------------------
GSErrCode RegisterInterface (void)
{
	return ACAPI_MenuItem_RegisterMenu (32500, 32600, MenuCode_UserDef, MenuFlag_Default);
}

// -----------------------------------------------------------------------------
// Initialize
// -----------------------------------------------------------------------------
GSErrCode Initialize (void)
{
	GSErrCode err = ACAPI_MenuItem_InstallMenuHandler (32500, APIMenuCommandProc_Main);
	if (err != NoError) return err;
	err = ACAPI_Notification_CatchSelectionChange (ComponentInfoPalette::SelectionChangeHandler);
	if (err != NoError) return err;
	err = ComponentInfoPalette::RegisterPaletteControlCallBack ();
	return err;
}

// -----------------------------------------------------------------------------
// FreeData
// -----------------------------------------------------------------------------
GSErrCode FreeData (void)
{
	ACAPI_Notification_CatchSelectionChange (nullptr);
	ACAPI_UnregisterModelessWindow (ComponentInfoPalette::GetPaletteReferenceId ());
	return NoError;
}

// -----------------------------------------------------------------------------
// APIMenuCommandProc_Main
// -----------------------------------------------------------------------------
// 导出梁IFC属性的辅助函数
static GSErrCode ExportBeamIFCPropertiesToJSON ()
{
	// 获取项目文件夹位置
	IO::Location projectFolderLoc;
	IO::Location photoFolderLoc;
	GS::UniString projectName;
	GSErrCode err = ComponentInfo::GetProjectPhotoFolder (photoFolderLoc, projectFolderLoc, projectName);
	if (err != NoError) {
		ACAPI_WriteReport ("导出梁IFC属性: 请先保存项目。", true);
		return err;
	}
	
	// 构建输出文件路径：项目文件夹/beam_ifc_properties.json
	IO::Location outputPath = projectFolderLoc;
	err = outputPath.AppendToLocal (IO::Name ("beam_ifc_properties.json"));
	if (err != NoError) {
		ACAPI_WriteReport ("导出梁IFC属性: 无法构建输出路径。", true);
		return err;
	}
	
	// 导出IFC属性
	ACAPI_WriteReport ("正在导出梁IFC属性到JSON文件...", false);
	err = ComponentInfo::ExportAllBeamIFCProperties (outputPath);
	if (err != NoError) {
		ACAPI_WriteReport ("导出梁IFC属性失败。", true);
		return err;
	}
	
	// 显示文件位置
	GS::UniString outputPathStr;
	if (outputPath.ToPath (&outputPathStr) == NoError) {
		ACAPI_WriteReport (GS::UniString::SPrintf ("导出完成: %s", outputPathStr.ToCStr ().Get ()), false);
	}
	
	return NoError;
}

static GSErrCode APIMenuCommandProc_Main (const API_MenuParams* menuParams)
{
	if (menuParams->menuItemRef.menuResID != 32500)
		return NoError;

	if (menuParams->menuItemRef.itemIndex == 1) {
		if (ComponentInfoPalette::HasInstance () && ComponentInfoPalette::GetInstance ().IsVisible ())
			ComponentInfoPalette::GetInstance ().Hide ();
		else {
			if (!ComponentInfoPalette::HasInstance ())
				ComponentInfoPalette::CreateInstance ();
			ComponentInfoPalette::GetInstance ().Show ();
		}
		return NoError;
	}
	
	if (menuParams->menuItemRef.itemIndex == 2) {
		GSErrCode err = ExportBeamIFCPropertiesToJSON ();
		return err;
	}
	
	return NoError;
}
