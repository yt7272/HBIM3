#include "MergedPluginPalette.hpp"
#include "MergedPlugin.hpp"
#include "APICommon.h"
#include "ApplicationLauncher.hpp"
#include "BM.hpp"
#include "DGUtility.hpp"
#include "DGDialog.hpp"
#include "DGBrowserEventArgs.hpp"
#include "MergedPluginDebug.hpp"

#include "DGFileDialog.hpp"
#include "FileTypeManager.hpp"

#include "PropertyUtils.hpp"

#include "FileSystem.hpp"

static void DebugLogHtmlPreview (const char* prefix, const char* bytes, GSSize size)
{
	char preview[96];
	GSSize n = size < (GSSize) (sizeof (preview) - 1) ? size : (GSSize) (sizeof (preview) - 1);
	for (GSSize i = 0; i < n; i++) {
		unsigned char c = (unsigned char) bytes[i];
		preview[i] = (c >= 32 && c < 127) ? (char) c : '.';
	}
	preview[n] = '\0';
	MergedPluginDebugLog ("[HTML] %s size=%d 前%d字节: %s", prefix, (int) size, (int) n, preview);
}

static GS::UniString LoadHtmlFromResource ()
{
	MergedPluginDebugLog ("[HTML] LoadHtmlFromResource 开始, 请求 DATA 100");
	GS::UniString resourceData;
	GSHandle data = RSLoadResource ('DATA', ACAPI_GetOwnResModule (), 100);
	if (data == nullptr) {
		MergedPluginDebugLog ("[HTML] DATA 100 资源未找到 (RSLoadResource 返回 nullptr)");
		ACAPI_WriteReport ("LoadHtmlFromResource: DATA 100 资源未找到", true);
		return GS::UniString ("<html><head><meta charset=\"UTF-8\"></head><body style=\"font-family:sans-serif;padding:20px;background:#1a1d23;color:#e2e8f0;\"><h2>HTML 资源加载失败</h2><p>DATA 100 资源未找到。请确认构建时 RFIX/Selection_Test.html 存在且资源已正确编译。</p><p>查看日志: ~/Library/Logs/MergedPlugin.log</p></body></html>");
	}
	GSSize handleSize = BMhGetSize (data);
	MergedPluginDebugLog ("[HTML] DATA 100 handleSize=%d", (int) handleSize);
	if (handleSize == 0) {
		BMhKill (&data);
		MergedPluginDebugLog ("[HTML] DATA 100 资源为空 (handleSize=0)");
		ACAPI_WriteReport ("LoadHtmlFromResource: DATA 100 资源为空", true);
		return GS::UniString ("<html><head><meta charset=\"UTF-8\"></head><body style=\"font-family:sans-serif;padding:20px;background:#1a1d23;color:#e2e8f0;\"><h2>HTML 资源为空</h2><p>请确认 RFIX/Selection_Test.html 已被 ResConv 正确嵌入。</p></body></html>");
	}
	DebugLogHtmlPreview ("DATA 100 内容", (const char*) *data, handleSize);
	resourceData.Append (*data, handleSize);
	BMhKill (&data);
	MergedPluginDebugLog ("[HTML] LoadHtmlFromResource 完成, resourceData.GetLength()=%d", (int) resourceData.GetLength ());
	return resourceData;
}

static GS::UniString GetStringFromJavaScriptVariable (GS::Ref<JS::Base> jsVariable)
{
	GS::Ref<JS::Value> jsValue = GS::DynamicCast<JS::Value> (jsVariable);
	if (DBVERIFY (jsValue != nullptr && jsValue->GetType () == JS::Value::STRING))
		return jsValue->GetString ();

	return GS::EmptyUniString;
}

template<class Type>
static GS::Ref<JS::Base> ConvertToJavaScriptVariable (const Type& cppVariable)
{
	return new JS::Value (cppVariable);
}

template<>
GS::Ref<JS::Base> ConvertToJavaScriptVariable (const MergedPluginPalette::ElementInfo& elemInfo)
{
	GS::Ref<JS::Array> js = new JS::Array ();
	js->AddItem (ConvertToJavaScriptVariable (elemInfo.guidStr));
	js->AddItem (ConvertToJavaScriptVariable (elemInfo.typeName));
	js->AddItem (ConvertToJavaScriptVariable (elemInfo.elemID));
	return js;
}

template<>
GS::Ref<JS::Base> ConvertToJavaScriptVariable (const PropertyUtils::PropertyInfo& propInfo)
{
	GS::Ref<JS::Array> js = new JS::Array ();
	js->AddItem (ConvertToJavaScriptVariable (propInfo.propertySetName));
	js->AddItem (ConvertToJavaScriptVariable (propInfo.propertyName));
	js->AddItem (ConvertToJavaScriptVariable (propInfo.propertyValue));
	js->AddItem (ConvertToJavaScriptVariable (propInfo.propertyType));
	js->AddItem (ConvertToJavaScriptVariable (propInfo.unit));
	js->AddItem (ConvertToJavaScriptVariable (propInfo.isValid));
	return js;
}

template<class Type>
static GS::Ref<JS::Base> ConvertToJavaScriptVariable (const GS::Array<Type>& cppArray)
{
	GS::Ref<JS::Array> newArray = new JS::Array ();
	for (const Type& item : cppArray) {
		newArray->AddItem (ConvertToJavaScriptVariable (item));
	}
	return newArray;
}

static const GS::Guid s_paletteGuid ("{A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}");
static GS::Ref<MergedPluginPalette> s_instance;

static GSErrCode NotificationHandler (API_NotifyEventID notifID, Int32)
{
	if (notifID == APINotify_Quit)
		MergedPluginPalette::DestroyInstance ();
	return NoError;
}

MergedPluginPalette::MergedPluginPalette ()
	: DG::Palette (ACAPI_GetOwnResModule (), PaletteResId, ACAPI_GetOwnResModule (), s_paletteGuid)
	, browser (GetReference (), BrowserId)
	, currentElemGuid (APINULLGuid)
	, isEditMode (false)
{
	MergedPluginDebugLog ("[面板] MergedPluginPalette 构造函数开始 PaletteResId=%d BrowserId=%d", (int) PaletteResId, (int) BrowserId);
	ACAPI_WriteReport ("MergedPluginPalette构造函数: 开始初始化，PaletteResId=%d", false, PaletteResId);
	ACAPI_ProjectOperation_CatchProjectEvent (APINotify_Quit, NotificationHandler);
	Attach (*this);
	BeginEventProcessing ();

	// 订阅浏览器加载错误和加载状态，便于排查“面板空白”（Add 要求 target 为第一参数）
	browser.onLoadError.Add ([] (DG::EventTarget*, const DG::BrowserBase&, const DG::BrowserLoadErrorArg& arg) {
		MergedPluginDebugLog ("[浏览器] LoadError code=%d url=%s", (int) static_cast<int> (arg.errorCode), arg.url.ToCStr ().Get ());
	}, this->GetDefaultEventTarget ());
	browser.onLoadingStateChange.Add ([] (DG::EventTarget*, const DG::BrowserBase&, const DG::BrowserLoadingStateChangeArg& arg) {
		MergedPluginDebugLog ("[浏览器] LoadingState isLoading=%d", arg.isLoading ? 1 : 0);
	}, this->GetDefaultEventTarget ());

	// 与 IFCPropertyReader 一致：使用 LoadHTML(LoadHtmlFromResource())，避免 LoadURL(data URL) 在部分环境下不渲染
	MergedPluginDebugLog ("[面板] BeginEventProcessing 完成，开始加载 HTML（LoadHTML 与 IFCPropertyReader 一致）");
	GS::UniString htmlContent = LoadHtmlFromResource ();
	if (!htmlContent.IsEmpty ()) {
		browser.LoadHTML (htmlContent);
		MergedPluginDebugLog ("[面板] browser.LoadHTML 已调用");
	}
	MergedPluginDebugLog ("[面板] InitializeJavaScriptBridge 开始");
	InitializeJavaScriptBridge ();
	MergedPluginDebugLog ("[面板] MergedPluginPalette 构造函数完成");
}

MergedPluginPalette::~MergedPluginPalette ()
{
	EndEventProcessing ();
}

GS::Array<MergedPluginPalette::ElementInfo> MergedPluginPalette::GetSelectedElements ()
{
	API_SelectionInfo	selectionInfo;
	GS::Array<API_Neig>	selNeigs;
	ACAPI_Selection_Get (&selectionInfo, &selNeigs, false, false);
	BMKillHandle ((GSHandle*)&selectionInfo.marquee.coords);

	GS::Array<MergedPluginPalette::ElementInfo> selectedElements;
	for (const API_Neig& neig : selNeigs) {
		API_Elem_Head elemHead {};
		elemHead.guid = neig.guid;
		ACAPI_Element_GetHeader (&elemHead);

		ElementInfo elemInfo;
		elemInfo.guidStr = APIGuidToString (elemHead.guid);
		ACAPI_Element_GetElemTypeName (elemHead.type, elemInfo.typeName);
		ACAPI_Element_GetElementInfoString (&elemHead.guid, &elemInfo.elemID);
		selectedElements.Push (elemInfo);
	}
	return selectedElements;
}

void MergedPluginPalette::InitializeJavaScriptBridge ()
{
	MergedPluginDebugLog ("[JS桥] InitializeJavaScriptBridge 开始");
	ACAPI_WriteReport ("InitializeJavaScriptBridge: 开始初始化JavaScript桥接", false);
	GS::Ref<JS::Object> jsACAPI = new JS::Object ("ACAPI");

	jsACAPI->AddItem (new JS::Function ("GetSelectedElements", [] (GS::Ref<JS::Base>) {
		ACAPI_WriteReport ("JavaScript桥接: GetSelectedElements被调用", false);
		return ConvertToJavaScriptVariable (GetSelectedElements ());
	}));

	jsACAPI->AddItem (new JS::Function ("GetAllIFCProperties", [] (GS::Ref<JS::Base> param) {
		ACAPI_WriteReport ("JavaScript桥接: GetAllIFCProperties被调用", false);
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (param);
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		GS::Array<PropertyUtils::PropertyInfo> properties = PropertyUtils::GetAllIFCPropertiesForElement (elementGuid);
		ACAPI_WriteReport ("JavaScript桥接: GetAllIFCProperties返回 %d 个属性", false, properties.GetSize ());
		return ConvertToJavaScriptVariable (properties);
	}));

	jsACAPI->AddItem (new JS::Function ("GetCachedIFCProperties", [] (GS::Ref<JS::Base> param) {
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (param);
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		GS::Array<PropertyUtils::PropertyInfo> properties = PropertyUtils::GetCachedIFCPropertiesForElement (elementGuid, false);
		return ConvertToJavaScriptVariable (properties);
	}));

	jsACAPI->AddItem (new JS::Function ("GetCachedIFCPropertiesWithRefresh", [] (GS::Ref<JS::Base> param) {
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (param);
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		GS::Array<PropertyUtils::PropertyInfo> properties = PropertyUtils::GetCachedIFCPropertiesForElement (elementGuid, true);
		return ConvertToJavaScriptVariable (properties);
	}));

	jsACAPI->AddItem (new JS::Function ("ClearIFCPropertyCache", [] (GS::Ref<JS::Base>) {
		PropertyUtils::ClearIFCPropertyCache ();
		return ConvertToJavaScriptVariable (true);
	}));

	jsACAPI->AddItem (new JS::Function ("GetIFCPropertyCacheSize", [] (GS::Ref<JS::Base>) {
		UInt32 cacheSize = PropertyUtils::GetIFCPropertyCacheSize ();
		return ConvertToJavaScriptVariable (cacheSize);
	}));

	jsACAPI->AddItem (new JS::Function ("GetPluginVersion", [] (GS::Ref<JS::Base>) {
		GS::UniString version = PropertyUtils::GetAddOnVersionString ();
		return new JS::Value (version);
	}));

	// ComponentInfo JavaScript API - functions added directly to ACAPI object
	jsACAPI->AddItem (new JS::Function ("ComponentInfo_GetComponentInfo", [] (GS::Ref<JS::Base> param) {
		ACAPI_WriteReport ("JavaScript桥接: ComponentInfo_GetComponentInfo被调用", false);
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (param);
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		
		// Find property definitions
		ComponentInfo::PropertyDefinitionGuids guids;
		GSErrCode err = ComponentInfo::FindPropertyDefinitions (guids);
		if (err != NoError) {
			ACAPI_WriteReport ("JavaScript桥接: ComponentInfo_GetComponentInfo - 属性定义未找到", false);
			// Property definitions don't exist yet, return empty object
			GS::Ref<JS::Object> result = new JS::Object ("ComponentInfoResult");
			result->AddItem ("componentId", new JS::Value (GS::EmptyUniString));
			result->AddItem ("photoDescription", new JS::Value (GS::EmptyUniString));
			result->AddItem ("photoPaths", new JS::Array ());
			return result;
		}
		
		GS::UniString compId, photoDesc;
		GS::Array<GS::UniString> photoPaths;
		
		ComponentInfo::GetStringPropertyFromElement (elementGuid, guids.compIdGuid, compId);
		ComponentInfo::GetStringPropertyFromElement (elementGuid, guids.photoDescGuid, photoDesc);
		ComponentInfo::GetPhotoPathsFromElement (elementGuid, guids.photoPathsGuid, photoPaths);
		
		GS::Ref<JS::Object> result = new JS::Object ("ComponentInfoResult");
		result->AddItem ("componentId", new JS::Value (compId));
		result->AddItem ("photoDescription", new JS::Value (photoDesc));
		result->AddItem ("photoPaths", ConvertToJavaScriptVariable (photoPaths));
		ACAPI_WriteReport ("JavaScript桥接: ComponentInfo_GetComponentInfo返回 componentId='%s', photoDesc='%s', photoPaths=%d", false, compId.ToCStr().Get(), photoDesc.ToCStr().Get(), photoPaths.GetSize());
		return result;
	}));
	
	jsACAPI->AddItem (new JS::Function ("ComponentInfo_SaveComponentInfo", [] (GS::Ref<JS::Base> param) {
		// param should be an array: [elementGuid, componentId, photoDescription, photoPaths[]]
		GS::Ref<JS::Array> params = GS::DynamicCast<JS::Array> (param);
		if (params == nullptr || params->GetItemArray ().GetSize () < 3) {
			return new JS::Value (false);
		}
		
		const GS::Array<GS::Ref<JS::Base>>& paramItems = params->GetItemArray ();
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (paramItems[0]);
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		
		GS::UniString componentId = GetStringFromJavaScriptVariable (paramItems[1]);
		GS::UniString photoDescription = GetStringFromJavaScriptVariable (paramItems[2]);
		
		GS::Array<GS::UniString> photoPaths;
		if (paramItems.GetSize () > 3) {
			GS::Ref<JS::Array> pathsArray = GS::DynamicCast<JS::Array> (paramItems[3]);
			if (pathsArray != nullptr) {
				const GS::Array<GS::Ref<JS::Base>>& pathItems = pathsArray->GetItemArray ();
				for (UInt32 i = 0; i < pathItems.GetSize (); ++i) {
					photoPaths.Push (GetStringFromJavaScriptVariable (pathItems[i]));
				}
			}
		}
		
		// Ensure property definitions exist
		ComponentInfo::PropertyDefinitionGuids guids;
		GSErrCode err = ComponentInfo::EnsurePropertyGroupAndDefinitions (guids);
		if (err != NoError) {
			return new JS::Value (false);
		}
		
		// Write properties to element
		err = ComponentInfo::WritePropertiesToElement (elementGuid, guids, componentId, photoDescription, photoPaths);
		return new JS::Value (err == NoError);
	}));
	
	jsACAPI->AddItem (new JS::Function ("ComponentInfo_SelectPhotos", [] (GS::Ref<JS::Base> param) {
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (param);
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		(void)elementGuid; // Not used currently, but part of API signature
		
		// Open file picker dialog (similar to DoSelectPhotos in palette)
		DG::FileDialog dlg (DG::FileDialog::OpenMultiFile);
		FTM::FileTypeManager mgr ("ComponentInfoPhotos");
		FTM::FileType typeJpg ("JPEG", "jpg", 0, 0, 0);
		FTM::FileType typePng ("PNG", "png", 0, 0, 0);
		FTM::TypeID idJpg = mgr.AddType (typeJpg);
		FTM::TypeID idPng = mgr.AddType (typePng);
		dlg.AddFilter (idJpg);
		dlg.AddFilter (idPng);
		dlg.SetTitle ("选择照片");
		
		GS::Array<GS::UniString> selectedPaths;
		if (dlg.Invoke ()) {
			USize n = dlg.GetSelectionCount ();
			for (UIndex i = 0; i < n; ++i) {
				GS::UniString path;
				dlg.GetSelectedFile (i).ToPath (&path);
				selectedPaths.Push (path);
			}
		}
		
		return ConvertToJavaScriptVariable (selectedPaths);
	}));
	
	jsACAPI->AddItem (new JS::Function ("ComponentInfo_ViewPhotos", [] (GS::Ref<JS::Base> param) {
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (param);
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		
		// Get component photo folder
		IO::Location componentPhotoFolder, projectFolderLoc;
		GS::UniString projectName;
		GSErrCode err = ComponentInfo::GetComponentPhotoFolder (elementGuid, componentPhotoFolder, projectFolderLoc, projectName);
		if (err != NoError) {
			return new JS::Value (false);
		}
		
		// Convert location to path string
		GS::UniString folderPath;
		if (componentPhotoFolder.ToPath (&folderPath) != NoError) {
			return new JS::Value (false);
		}
		
		// Open folder in system explorer
		IO::Location openCmd;
#if defined (GS_MAC)
		openCmd = IO::Location ("/usr/bin/open");
#elif defined (GS_WIN)
		openCmd = IO::Location ("C:\\Windows\\explorer.exe");
#endif
		
		GS::Array<GS::UniString> argv;
		argv.Push (folderPath);
		
 		err = IO::Process::ApplicationLauncher::Instance ().Launch (openCmd, argv);
		return new JS::Value (err == NoError);
	}));
	
	jsACAPI->AddItem (new JS::Function ("ComponentInfo_SetEditMode", [] (GS::Ref<JS::Base> param) {
		GS::UniString modeStr = GetStringFromJavaScriptVariable (param);
		bool editMode = (modeStr == "true" || modeStr == "1");
		if (MergedPluginPalette::HasInstance ()) {
			MergedPluginPalette::GetInstance ().isEditMode = editMode;
			ACAPI_WriteReport ("JavaScript桥接: ComponentInfo_SetEditMode设置为 %s", false, editMode ? "true" : "false");
		}
		return new JS::Value (true);
	}));

	browser.RegisterAsynchJSObject (jsACAPI);
	MergedPluginDebugLog ("[JS桥] RegisterAsynchJSObject 完成");
	ACAPI_WriteReport ("InitializeJavaScriptBridge: JavaScript桥接注册完成", false);
}

bool MergedPluginPalette::HasInstance ()
{
	return s_instance != nullptr;
}

void MergedPluginPalette::CreateInstance ()
{
	if (s_instance == nullptr) {
		s_instance = new MergedPluginPalette ();
		ACAPI_KeepInMemory (true);
	}
}

MergedPluginPalette& MergedPluginPalette::GetInstance ()
{
	return *s_instance;
}

void MergedPluginPalette::DestroyInstance ()
{
	s_instance = nullptr;
}

void MergedPluginPalette::Show ()
{
	DG::Palette::Show ();
	SetMenuItemCheckedState (true);
	// 与 IFCPropertyReader 一致：Show 时不再重复加载 HTML，仅刷新内容
	RefreshContent ();
}

void MergedPluginPalette::Hide ()
{
	DG::Palette::Hide ();
	SetMenuItemCheckedState (false);
}

void MergedPluginPalette::UpdateFromSelection ()
{
	RefreshContent ();
}

bool MergedPluginPalette::IsInEditMode ()
{
	return HasInstance () && GetInstance ().isEditMode;
}

void MergedPluginPalette::RefreshContent ()
{
	ACAPI_WriteReport ("RefreshContent: 开始刷新内容，isEditMode=%s", false, isEditMode ? "true" : "false");
	if (isEditMode) return;

	// Clear internal state
	currentPhotoRelPaths.Clear ();
	deletedPhotoRelPaths.Clear ();
	currentElemGuid = APINULLGuid;


	// Get selected elements
	API_SelectionInfo selInfo = {};
	GS::Array<API_Neig> selNeigs;
	GSErrCode err = ACAPI_Selection_Get (&selInfo, &selNeigs, false);
	if (err != NoError && err != APIERR_NOSEL) { 
		ACAPI_WriteReport ("RefreshContent: ACAPI_Selection_Get失败: %s", true, ErrID_To_Name(err)); 
		return; 
	}
	BMKillHandle ((GSHandle*) &selInfo.marquee.coords);
	
	ACAPI_WriteReport ("RefreshContent: 获取到%d个选中构件", false, selNeigs.GetSize());
	
	// Call JavaScript to update UI
	GS::UniString jsCode;
	if (selNeigs.IsEmpty ()) {
		// No selection - clear UI
		jsCode = "if (typeof ClearComponentInfoUI === 'function') ClearComponentInfoUI();";
		// Also hide multi-selection warning
		jsCode += "document.getElementById('multi-selection-warning').classList.add('hidden');";
	} else {
		// Store first element GUID for later operations
		currentElemGuid = selNeigs[0].guid;
		
		if (selNeigs.GetSize () > 1) {
			// Multiple selection - show warning
			jsCode = "document.getElementById('multi-selection-warning').classList.remove('hidden');";
		} else {
			// Single selection - hide warning
			jsCode = "document.getElementById('multi-selection-warning').classList.add('hidden');";
		}
		// Call UpdateSelectedElements() which will fetch selected elements via ACAPI
		jsCode += "if (typeof UpdateSelectedElements === 'function') UpdateSelectedElements();";
	}
	
	// Execute JavaScript
	if (!jsCode.IsEmpty ()) {
		browser.ExecuteJS (jsCode);
		ACAPI_WriteReport ("RefreshContent: 执行JavaScript代码: %s", false, jsCode.ToCStr ().Get ());
	}
}













void MergedPluginPalette::SetMenuItemCheckedState (bool checked)
{
	API_MenuItemRef ref = {};
	GSFlags flags = {};
	ref.menuResID = 32500;
	ref.itemIndex = 1;
	ACAPI_MenuItem_GetMenuItemFlags (&ref, &flags);
	if (checked) flags |= API_MenuItemChecked;
	else flags &= (GSFlags) ~API_MenuItemChecked;
	ACAPI_MenuItem_SetMenuItemFlags (&ref, &flags);
}



void MergedPluginPalette::PanelResized (const DG::PanelResizeEvent& ev)
{
	BeginMoveResizeItems ();
	browser.Resize (ev.GetHorizontalChange (), ev.GetVerticalChange ());
	EndMoveResizeItems ();
}

void MergedPluginPalette::PanelCloseRequested (const DG::PanelCloseRequestEvent&, bool* accepted)
{
	Hide ();
	*accepted = true;
}



void MergedPluginPalette::UpdateEditModeUI ()
{
	// Call JavaScript to update UI based on edit mode
	GS::UniString jsCode;
	if (isEditMode) {
		jsCode = "if (typeof enterEditMode === 'function') enterEditMode();";
	} else {
		jsCode = "if (typeof exitEditMode === 'function') exitEditMode(false);";
	}
	if (!jsCode.IsEmpty ()) {
		browser.ExecuteJS (jsCode);
	}
}









bool MergedPluginPalette::HasUnsavedChanges () const
{
	// Always return false - JavaScript handles all save operations
	return false;
}

void MergedPluginPalette::DoSave ()
{
	if (currentElemGuid == APINULLGuid) {
		ACAPI_WriteReport ("请先选择一个构件。", true);
		return;
	}

	// DG controls removed - values will come from JavaScript via ComponentInfo_SaveComponentInfo
	GS::UniString compId = GS::EmptyUniString; // Was: editCompId.GetText()
	GS::UniString photoDesc = GS::EmptyUniString; // Was: editPhotoDesc.GetText()
	API_Guid elemGuid = currentElemGuid;
	GS::Array<IO::Location> photoLocs;

	// 第一步：检查属性是否对当前元素可用，如果不可用则删除并重建属性组
	ComponentInfo::PropertyDefinitionGuids guids;
	bool needRecreate = false;

	// 先尝试查找现有属性
	GSErrCode findErr = ComponentInfo::FindPropertyDefinitions (guids);
	if (findErr == NoError) {
		// 检查属性是否对当前元素可用
		bool available = ACAPI_Element_IsPropertyDefinitionAvailable (elemGuid, guids.compIdGuid);
		if (!available) {
			needRecreate = true;
		}
	}

	if (needRecreate) {
		// 删除并重建属性组（使其对所有元素类型可用）
		GSErrCode recreateErr = ACAPI_CallUndoableCommand ("重建属性组",
			[&] () -> GSErrCode {
				return ComponentInfo::DeleteAndRecreatePropertyGroup (guids);
			}
		);
		if (recreateErr != NoError) {
			DG::InformationAlert ("需要手动操作", 
				"属性组重建失败。\n\n请手动操作：\n1. 打开 选项 → 属性管理器\n2. 删除 \"构件扩展信息\" 属性组\n3. 重新保存", 
				"确定");
			return;
		}
	} else {
		// 确保属性存在
		GSErrCode fixErr = ACAPI_CallUndoableCommand ("确保属性定义",
			[&] () -> GSErrCode {
				return ComponentInfo::EnsurePropertyGroupAndDefinitions (guids);
			}
		);
		if (fixErr != NoError) {
			ACAPI_WriteReport ("确保属性定义失败: %s", true, ErrID_To_Name (fixErr));
			return;
		}
	}

	// 保存当前的照片路径列表（可能已被编辑模式下的删除操作修改）
	GS::Array<GS::UniString> remainingPhotoPaths = currentPhotoRelPaths;

	// 第二步：在另一个命令中写入属性值
	GSErrCode cmdErr = ACAPI_CallUndoableCommand ("保存构件信息",
		[&] () -> GSErrCode {
			GSErrCode err;

			// 1. 使用编辑后的照片路径列表（已排除删除的照片）
			GS::Array<GS::UniString> allRelPaths = remainingPhotoPaths;

			// 2. 如果有新照片，复制到构件文件夹
			GS::Array<GS::UniString> newRelPaths;
			if (!photoLocs.IsEmpty ()) {
				UInt32 startIndex = (UInt32) allRelPaths.GetSize ();
				err = ComponentInfo::CopyPhotosForComponent (elemGuid, photoLocs, newRelPaths, startIndex);
				if (err != NoError) {
					ACAPI_WriteReport ("复制照片失败: %s", true, ErrID_To_Name (err));
					return err;
				}
				// 添加新照片路径
				for (UInt32 i = 0; i < newRelPaths.GetSize (); ++i) {
					allRelPaths.Push (newRelPaths[i]);
				}
			}

			// 3. 写入属性
			err = ComponentInfo::WritePropertiesToElement (elemGuid, guids, compId, photoDesc, allRelPaths);
			if (err != NoError) {
				ACAPI_WriteReport ("保存属性失败: %s", true, ErrID_To_Name (err));
				return err;
			}
			return NoError;
		}
	);

	if (cmdErr == NoError) {
		ACAPI_WriteReport ("构件信息已保存。", false);

		// 刷新显示
		RefreshContent ();
	}
}

GSErrCode MergedPluginPalette::SelectionChangeHandler (const API_Neig*)
{
	if (HasInstance () && GetInstance ().IsVisible ()) {
		MergedPluginPalette& instance = GetInstance ();
		
		// 检查是否有未保存的更改
		if (instance.HasUnsavedChanges ()) {
			// 获取当前构件标识
			GS::UniString currentIdentifier;
			if (instance.currentElemGuid != APINULLGuid) {
				ComponentInfo::GetComponentIdentifier (instance.currentElemGuid, currentIdentifier);
			} else {
				currentIdentifier = "当前构件";
			}
			
			// 构建警告消息
			GS::UniString warningMessage;
			warningMessage += "你选择了另一个构件，但你编辑的内容将仍然存储到构件";
			warningMessage += currentIdentifier;
			warningMessage += "当中去。";
			
			// 显示模态警告对话框
			DG::InformationAlert ("警告", warningMessage, "确定");
			
			// 自动保存当前编辑
			instance.DoSave ();
			
			// 退出编辑模式
			instance.isEditMode = false;
			instance.UpdateEditModeUI ();
		}
		
		// 刷新显示新选择的构件
		instance.RefreshContent ();
	}
	return NoError;
}

GSErrCode MergedPluginPalette::PaletteControlCallBack (Int32, API_PaletteMessageID messageID, GS::IntPtr param)
{
	MergedPluginDebugLog ("[回调] PaletteControlCallBack messageID=%d", (int) messageID);
	switch (messageID) {
		case APIPalMsg_OpenPalette:
			MergedPluginDebugLog ("[回调] APIPalMsg_OpenPalette: CreateInstance/Show");
			if (!HasInstance ()) CreateInstance ();
			GetInstance ().Show ();
			break;
		case APIPalMsg_ClosePalette:
			if (HasInstance ()) GetInstance ().Hide ();
			break;
		case APIPalMsg_HidePalette_Begin:
			if (HasInstance () && GetInstance ().IsVisible ()) GetInstance ().Hide ();
			break;
		case APIPalMsg_HidePalette_End:
			if (HasInstance () && !GetInstance ().IsVisible ()) GetInstance ().Show ();
			break;
		case APIPalMsg_IsPaletteVisible:
			*(reinterpret_cast<bool*> (param)) = HasInstance () && GetInstance ().IsVisible ();
			break;
		default:
			break;
	}
	return NoError;
}

Int32 MergedPluginPalette::GetPaletteReferenceId ()
{
	return GS::CalculateHashValue (s_paletteGuid);
}

GSErrCode MergedPluginPalette::RegisterPaletteControlCallBack ()
{
	return ACAPI_RegisterModelessWindow (
		GetPaletteReferenceId (),
		PaletteControlCallBack,
		API_PalEnabled_FloorPlan + API_PalEnabled_Section + API_PalEnabled_Elevation +
		API_PalEnabled_InteriorElevation + API_PalEnabled_3D + API_PalEnabled_Detail +
		API_PalEnabled_Worksheet + API_PalEnabled_Layout + API_PalEnabled_DocumentFrom3D,
		GSGuid2APIGuid (s_paletteGuid));
}


