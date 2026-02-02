// *****************************************************************************
// File:			ComponentInfoMain.cpp
// Description:		Component Info Add-on: CheckEnvironment, RegisterInterface,
//					Initialize, FreeData, APIMenuCommandProc_Main
// Project:			ComponentInfo
// *****************************************************************************

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "MergedPlugin.hpp"
#include "MergedPluginPalette.hpp"
#include "MergedPluginDebug.hpp"

static GSErrCode APIMenuCommandProc_Main (const API_MenuParams* menuParams);

// -----------------------------------------------------------------------------
// Add-on entry: CheckEnvironment
// -----------------------------------------------------------------------------
API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	MergedPluginDebugLog ("[加载] CheckEnvironment 开始");
	RSGetIndString (&envir->addOnInfo.name, 32000, 1, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, 32000, 2, ACAPI_GetOwnResModule ());
	MergedPluginDebugLog ("[加载] CheckEnvironment 完成, 日志文件: %s", MergedPluginDebugLogPath ());
	return APIAddon_Normal;
}

// -----------------------------------------------------------------------------
// RegisterInterface
// -----------------------------------------------------------------------------
GSErrCode RegisterInterface (void)
{
	MergedPluginDebugLog ("[加载] RegisterInterface 开始");
	GSErrCode err = ACAPI_MenuItem_RegisterMenu (32500, 32600, MenuCode_UserDef, MenuFlag_Default);
	MergedPluginDebugLog ("[加载] RegisterInterface 完成, err=%d", (int) err);
	return err;
}

// -----------------------------------------------------------------------------
// Initialize
// -----------------------------------------------------------------------------
GSErrCode Initialize (void)
{
	MergedPluginDebugLog ("[加载] Initialize 开始");
	GSErrCode err = ACAPI_MenuItem_InstallMenuHandler (32500, APIMenuCommandProc_Main);
	if (err != NoError) {
		MergedPluginDebugLog ("[加载] Initialize 失败: InstallMenuHandler err=%d", (int) err);
		return err;
	}
	err = ACAPI_Notification_CatchSelectionChange (MergedPluginPalette::SelectionChangeHandler);
	if (err != NoError) {
		MergedPluginDebugLog ("[加载] Initialize 失败: CatchSelectionChange err=%d", (int) err);
		return err;
	}
	err = MergedPluginPalette::RegisterPaletteControlCallBack ();
	MergedPluginDebugLog ("[加载] Initialize 完成, RegisterPaletteControlCallBack err=%d", (int) err);
	return err;
}

// -----------------------------------------------------------------------------
// FreeData
// -----------------------------------------------------------------------------
GSErrCode FreeData (void)
{
	MergedPluginDebugLog ("[加载] FreeData 开始");
	ACAPI_Notification_CatchSelectionChange (nullptr);
	ACAPI_UnregisterModelessWindow (MergedPluginPalette::GetPaletteReferenceId ());
	MergedPluginDebugLog ("[加载] FreeData 完成");
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
		MergedPluginDebugLog ("[菜单] 显示/隐藏面板 itemIndex=1");
		if (MergedPluginPalette::HasInstance () && MergedPluginPalette::GetInstance ().IsVisible ())
			MergedPluginPalette::GetInstance ().Hide ();
		else {
			if (!MergedPluginPalette::HasInstance ()) {
				MergedPluginDebugLog ("[菜单] CreateInstance 开始");
				MergedPluginPalette::CreateInstance ();
				MergedPluginDebugLog ("[菜单] CreateInstance 完成");
			}
			MergedPluginPalette::GetInstance ().Show ();
			MergedPluginDebugLog ("[菜单] Show 已调用");
		}
		return NoError;
	}
	
	if (menuParams->menuItemRef.itemIndex == 2) {
		GSErrCode err = ExportBeamIFCPropertiesToJSON ();
		return err;
	}
	
	return NoError;
}
