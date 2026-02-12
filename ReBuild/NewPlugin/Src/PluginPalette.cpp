#include "PluginPalette.hpp"
#include "PropertyUtils.hpp"
#include "ComponentInfo.hpp"
#include "NewPluginDebug.hpp"
#include "APICommon.h"
#include "ApplicationLauncher.hpp"
#include "BM.hpp"
#include "DGUtility.hpp"
#include "DGDialog.hpp"
#include "DGBrowserEventArgs.hpp"

#include "DGFileDialog.hpp"
#include "FileTypeManager.hpp"
#include "DG.h"

#include "FileSystem.hpp"

namespace {
    struct ComponentInfoResult {
        GS::UniString componentId;
        GS::UniString photoDesc;
        GS::Array<GS::UniString> photoPaths;
        GS::UniString globalId;
        GS::UniString folderName;
    };
}

static GS::UniString LoadHtmlFromResource ()
{
	NewPluginDebugLog ("[HTML] LoadHtmlFromResource 开始, 请求 DATA 100");
	GS::UniString resourceData;
	GSHandle data = RSLoadResource ('DATA', ACAPI_GetOwnResModule (), 100);
	if (data == nullptr) {
		NewPluginDebugLog ("[HTML] DATA 100 资源未找到 (RSLoadResource 返回 nullptr)");
		ACAPI_WriteReport ("LoadHtmlFromResource: DATA 100 资源未找到", true);
		return GS::UniString ("<html><head><meta charset=\"UTF-8\"></head><body style=\"font-family:sans-serif;padding:20px;background:#1a1d23;color:#e2e8f0;\"><h2>HTML 资源加载失败</h2><p>DATA 100 资源未找到。请确认构建时 RFIX/index.html 存在且资源已正确编译。</p><p>查看日志: ~/Library/Logs/NewPlugin.log</p></body></html>");
	}
	GSSize handleSize = BMhGetSize (data);
	NewPluginDebugLog ("[HTML] DATA 100 handleSize=%d", (int) handleSize);
	if (handleSize == 0) {
		BMhKill (&data);
		NewPluginDebugLog ("[HTML] DATA 100 资源为空 (handleSize=0)");
		ACAPI_WriteReport ("LoadHtmlFromResource: DATA 100 资源为空", true);
		return GS::UniString ("<html><head><meta charset=\"UTF-8\"></head><body style=\"font-family:sans-serif;padding:20px;background:#1a1d23;color:#e2e8f0;\"><h2>HTML 资源为空</h2><p>请确认 RFIX/index.html 已被 ResConv 正确嵌入。</p></body></html>");
	}
	// 预览前100个字符
	const char* dataPtr = (const char*) *data;
	GSSize previewSize = handleSize < 100 ? handleSize : 100;
	GS::UniString preview;
	preview.Append (dataPtr, previewSize);
	NewPluginDebugLog ("[HTML] DATA 100 预览 (前 %d 字符): %s", (int) previewSize, preview.ToCStr ().Get ());
	
	resourceData.Append (*data, handleSize);
	BMhKill (&data);
	NewPluginDebugLog ("[HTML] LoadHtmlFromResource 完成, resourceData.GetLength()=%d", (int) resourceData.GetLength ());
	return resourceData;
}

static GS::UniString GetStringFromJavaScriptVariable (GS::Ref<JS::Base> jsVariable)
{
	GS::Ref<JS::Value> jsValue = GS::DynamicCast<JS::Value> (jsVariable);
	if (DBVERIFY (jsValue != nullptr && jsValue->GetType () == JS::Value::STRING))
		return jsValue->GetString ();

	return GS::EmptyUniString;
}



static bool GetBoolFromJavaScriptVariable (GS::Ref<JS::Base> jsVariable)
{
	GS::Ref<JS::Value> jsValue = GS::DynamicCast<JS::Value> (jsVariable);
	if (DBVERIFY (jsValue != nullptr && jsValue->GetType () == JS::Value::BOOL))
		return jsValue->GetBool ();

	return false;
}

static GS::Ref<JS::Array> GetArrayFromJavaScriptVariable (GS::Ref<JS::Base> jsVariable)
{
	GS::Ref<JS::Array> jsArray = GS::DynamicCast<JS::Array> (jsVariable);
	return jsArray;
}



template<class Type>
static GS::Ref<JS::Base> ConvertToJavaScriptVariable (const Type& cppVariable)
{
	return new JS::Value (cppVariable);
}

template<>
GS::Ref<JS::Base> ConvertToJavaScriptVariable (const GS::Array<GS::UniString>& array)
{
	GS::Ref<JS::Array> js = new JS::Array ();
	for (const GS::UniString& str : array) {
		js->AddItem (ConvertToJavaScriptVariable (str));
	}
	return js;
}

template<>
GS::Ref<JS::Base> ConvertToJavaScriptVariable (const PluginPalette::ElementInfo& elemInfo)
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

template<>
GS::Ref<JS::Base> ConvertToJavaScriptVariable (const ComponentInfoResult& compInfo)
{
	GS::Ref<JS::Array> js = new JS::Array ();
	js->AddItem (ConvertToJavaScriptVariable (compInfo.componentId));
	js->AddItem (ConvertToJavaScriptVariable (compInfo.photoDesc));
	js->AddItem (ConvertToJavaScriptVariable (compInfo.photoPaths));
	js->AddItem (ConvertToJavaScriptVariable (compInfo.globalId));
	js->AddItem (ConvertToJavaScriptVariable (compInfo.folderName));
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
static GS::Ref<PluginPalette> s_instance;

static GSErrCode NotificationHandler (API_NotifyEventID notifID, Int32)
{
	if (notifID == APINotify_Quit)
		PluginPalette::DestroyInstance ();
	return NoError;
}

PluginPalette::PluginPalette ()
	: DG::Palette (ACAPI_GetOwnResModule (), PaletteResId, ACAPI_GetOwnResModule (), s_paletteGuid)
	, browser (GetReference (), BrowserId)
	, currentElemGuid (APINULLGuid)
	, isEditMode (false)
	, hasUnsavedChanges (false)
	, selectionDisabled (false)
{
	NewPluginDebugLog ("[面板] PluginPalette 构造函数开始 PaletteResId=%d BrowserId=%d", (int) PaletteResId, (int) BrowserId);
	ACAPI_ProjectOperation_CatchProjectEvent (APINotify_Quit, NotificationHandler);
	Attach (*this);
	BeginEventProcessing ();

	// 订阅浏览器加载错误和加载状态，便于排查"面板空白"
	browser.onLoadError.Add ([] (DG::EventTarget*, const DG::BrowserBase&, const DG::BrowserLoadErrorArg& arg) {
		NewPluginDebugLog ("[浏览器] LoadError code=%d url=%s", (int) static_cast<int> (arg.errorCode), arg.url.ToCStr ().Get ());
	}, this->GetDefaultEventTarget ());
	browser.onLoadingStateChange.Add ([] (DG::EventTarget*, const DG::BrowserBase&, const DG::BrowserLoadingStateChangeArg& arg) {
		NewPluginDebugLog ("[浏览器] LoadingState isLoading=%d", arg.isLoading ? 1 : 0);
	}, this->GetDefaultEventTarget ());

	NewPluginDebugLog ("[面板] BeginEventProcessing 完成，开始加载 HTML");
	GS::UniString htmlContent = LoadHtmlFromResource ();
	if (!htmlContent.IsEmpty ()) {
		browser.LoadHTML (htmlContent);
		NewPluginDebugLog ("[面板] browser.LoadHTML 已调用");
	}
	NewPluginDebugLog ("[面板] InitializeJavaScriptBridge 开始");
	InitializeJavaScriptBridge ();
	NewPluginDebugLog ("[面板] PluginPalette 构造函数完成");
}

PluginPalette::~PluginPalette ()
{
	// Selection control is handled by SelectionChangeHandler
	// No need for ACAPI_DisableSelection (function doesn't exist in API)
	EndEventProcessing ();
}

GS::Array<PluginPalette::ElementInfo> PluginPalette::GetSelectedElements ()
{
	API_SelectionInfo	selectionInfo{};
	GS::Array<API_Neig>	selNeigs;
	
	GSErrCode err = ACAPI_Selection_Get (&selectionInfo, &selNeigs, false, false);
	
	// APIERR_NOSEL (206) means "no selection" - this is NOT a real error, just empty selection
	if (err != NoError && err != APIERR_NOSEL) {
		// Real API error - log it but return empty array
		NewPluginDebugLog ("[选择API] ACAPI_Selection_Get 失败，错误码: %d (%s)", 
			static_cast<int> (err), ErrID_To_Name (err));
		ACAPI_WriteReport ("GetSelectedElements: ACAPI_Selection_Get failed: %s", true, ErrID_To_Name (err));
		return GS::Array<ElementInfo> ();
	}
	
	// Only kill marquee handle for actual marquee selections
	if (selectionInfo.typeID == API_MarqueePoly || 
		selectionInfo.typeID == API_MarqueeHorBox || 
		selectionInfo.typeID == API_MarqueeRotBox) {
		BMKillHandle ((GSHandle*)&selectionInfo.marquee.coords);
	}
	
	// Debug log selection info
	if (err == APIERR_NOSEL) {
		NewPluginDebugLog ("[选择API] 没有选中元素 (APIERR_NOSEL)");
	} else {
		NewPluginDebugLog ("[选择API] 选中 %d 个元素，选择类型: %d", 
			static_cast<int> (selNeigs.GetSize ()), static_cast<int> (selectionInfo.typeID));
	}

	GS::Array<PluginPalette::ElementInfo> selectedElements;
	for (const API_Neig& neig : selNeigs) {
		API_Elem_Head elemHead {};
		elemHead.guid = neig.guid;
		GSErrCode elemErr = ACAPI_Element_GetHeader (&elemHead);
		if (elemErr != NoError) {
			NewPluginDebugLog ("[选择API] 无法获取元素头信息，GUID: %s, 错误: %d", 
				APIGuidToString (neig.guid).ToCStr ().Get (), static_cast<int> (elemErr));
			continue;
		}

		ElementInfo elemInfo;
		elemInfo.guidStr = APIGuidToString (elemHead.guid);
		ACAPI_Element_GetElemTypeName (elemHead.type, elemInfo.typeName);
		ACAPI_Element_GetElementInfoString (&elemHead.guid, &elemInfo.elemID);
		selectedElements.Push (elemInfo);
		
		NewPluginDebugLog ("[选择API] 元素: GUID=%s, 类型=%s, ID=%s", 
			elemInfo.guidStr.ToCStr ().Get (), 
			elemInfo.typeName.ToCStr ().Get (),
			elemInfo.elemID.ToCStr ().Get ());
	}
	
	NewPluginDebugLog ("[选择API] 返回 %d 个元素信息", static_cast<int> (selectedElements.GetSize ()));
	return selectedElements;
}

void PluginPalette::InitializeJavaScriptBridge ()
{
	GS::Ref<JS::Object> jsACAPI = new JS::Object ("ACAPI");

	jsACAPI->AddItem (new JS::Function ("GetSelectedElements", [] (GS::Ref<JS::Base>) {
		return ConvertToJavaScriptVariable (GetSelectedElements ());
	}));

	jsACAPI->AddItem (new JS::Function ("GetPluginVersion", [] (GS::Ref<JS::Base>) {
		GS::UniString version = PropertyUtils::GetAddOnVersionString ();
		return ConvertToJavaScriptVariable (version);
	}));

	jsACAPI->AddItem (new JS::Function ("GetAllIFCProperties", [] (GS::Ref<JS::Base> param) {
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (param);
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		GS::Array<PropertyUtils::PropertyInfo> properties = PropertyUtils::GetAllIFCPropertiesForElement (elementGuid);
		return ConvertToJavaScriptVariable (properties);
	}));

	jsACAPI->AddItem (new JS::Function ("GetCachedIFCProperties", [] (GS::Ref<JS::Base> param) {
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (param);
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		GS::Array<PropertyUtils::PropertyInfo> properties = PropertyUtils::GetCachedIFCPropertiesForElement (elementGuid, false);
		return ConvertToJavaScriptVariable (properties);
	}));

	// TODO: Add more JavaScript bridge functions for component info and photo management
	
	// Component Info JavaScript bridge functions
	jsACAPI->AddItem (new JS::Function ("ComponentInfo_GetComponentInfo", [] (GS::Ref<JS::Base> param) {
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (param);
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		
		NewPluginDebugLog ("[读取] ComponentInfo_GetComponentInfo: 开始读取，元素 %s", elementGuidStr.ToCStr ().Get ());
		
		ComponentInfoResult result;
		
		// Find property definitions (don't create if they don't exist)
		ComponentInfo::PropertyDefinitionGuids guids;
		GSErrCode err = ComponentInfo::FindPropertyDefinitions (guids);
		if (err != NoError) {
			NewPluginDebugLog ("[读取] ComponentInfo_GetComponentInfo: 属性定义未找到 err=%d", (int) err);
			// Property definitions don't exist yet, return empty result
			// Still get GlobalId and folder name (they don't require property definitions)
			result.globalId = ComponentInfo::GetGlobalIdPropertyValue (elementGuid);
			result.folderName = ComponentInfo::GlobalIdToFolderName (elementGuid);
			NewPluginDebugLog ("[读取] ComponentInfo_GetComponentInfo: 返回空结果，GlobalId=%s, folderName=%s", 
				result.globalId.ToCStr ().Get (), result.folderName.ToCStr ().Get ());
			return ConvertToJavaScriptVariable (result);
		}
		
		NewPluginDebugLog ("[读取] ComponentInfo_GetComponentInfo: 属性定义找到，组GUID：%s，构件编号GUID：%s，照片说明GUID：%s，照片路径GUID：%s",
			APIGuidToString (guids.groupGuid).ToCStr ().Get (),
			APIGuidToString (guids.compIdGuid).ToCStr ().Get (),
			APIGuidToString (guids.photoDescGuid).ToCStr ().Get (),
			APIGuidToString (guids.photoPathsGuid).ToCStr ().Get ());
		
		// Read component properties using individual property accessors
		GS::UniString compId, photoDesc;
		GS::Array<GS::UniString> photoPaths;
		
		ComponentInfo::GetStringPropertyFromElement (elementGuid, guids.compIdGuid, compId);
		ComponentInfo::GetStringPropertyFromElement (elementGuid, guids.photoDescGuid, photoDesc);
		ComponentInfo::GetPhotoPathsFromElement (elementGuid, guids.photoPathsGuid, photoPaths);
		
		result.componentId = compId;
		result.photoDesc = photoDesc;
		result.photoPaths = photoPaths;
		
		// Get GlobalId and folder name
		result.globalId = ComponentInfo::GetGlobalIdPropertyValue (elementGuid);
		result.folderName = ComponentInfo::GlobalIdToFolderName (elementGuid);
		
		NewPluginDebugLog ("[读取] ComponentInfo_GetComponentInfo: 返回结果 componentId='%s', photoDesc='%s', photoPaths=%d, globalId=%s, folderName=%s",
			compId.ToCStr ().Get (), photoDesc.ToCStr ().Get (), photoPaths.GetSize (),
			result.globalId.ToCStr ().Get (), result.folderName.ToCStr ().Get ());
		
		return ConvertToJavaScriptVariable (result);
	}));
	
	jsACAPI->AddItem (new JS::Function ("ComponentInfo_SaveComponentInfo", [] (GS::Ref<JS::Base> param) {
		// param should be an array: [elementGuid, componentId, photoDescription, photoPaths[]]
		// photoPaths is optional (can be empty array)
		GS::Ref<JS::Array> paramArray = GetArrayFromJavaScriptVariable (param);
		if (paramArray == nullptr || paramArray->GetItemArray ().GetSize () < 3) {
			NewPluginDebugLog ("[保存] ComponentInfo_SaveComponentInfo: 参数无效，至少需要3个参数");
			return ConvertToJavaScriptVariable (false);
		}
		
		const GS::Array<GS::Ref<JS::Base>>& paramItems = paramArray->GetItemArray ();
		// Extract parameters
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (paramItems[0]);
		GS::UniString componentId = GetStringFromJavaScriptVariable (paramItems[1]);
		GS::UniString photoDesc = GetStringFromJavaScriptVariable (paramItems[2]);
		
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
		
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		
		NewPluginDebugLog ("[保存] ComponentInfo_SaveComponentInfo: 开始保存，元素 %s, 构件编号: %s, 照片描述: %s, %d 个照片路径", 
			elementGuidStr.ToCStr ().Get (), componentId.ToCStr ().Get (), photoDesc.ToCStr ().Get (), photoPaths.GetSize ());
		
		// Get property definition GUIDs
		ComponentInfo::PropertyDefinitionGuids guids;
		GSErrCode err = ComponentInfo::EnsurePropertyGroupAndDefinitions (guids);
		if (err != NoError) {
			NewPluginDebugLog ("[保存] ComponentInfo_SaveComponentInfo: 创建属性定义失败 err=%d", (int) err);
			return ConvertToJavaScriptVariable (false);
		}
		
		GS::Array<IO::Location> photoLocsForCopying;
		GS::Array<GS::UniString> existingRelPaths;
		GS::Array<GS::UniString> invalidPaths;
		
		for (const GS::UniString& path : photoPaths) {
			if (!path.IsEmpty ()) {
				// 检查路径是否是已经保存的相对路径（包含 "_构件照片/" 模式）
				if (path.Contains ("_构件照片/")) {
					// 这是已经保存的相对路径，不需要再次复制
					existingRelPaths.Push (path);
					NewPluginDebugLog ("[保存] ComponentInfo_SaveComponentInfo: 已存在的相对图片路径: %s", path.ToCStr ().Get ());
				} else {
					// 尝试作为绝对路径处理
					IO::Location loc (path);
					photoLocsForCopying.Push (loc);
					NewPluginDebugLog ("[保存] ComponentInfo_SaveComponentInfo: 添加绝对图片路径: %s", path.ToCStr ().Get ());
				}
			} else {
				NewPluginDebugLog ("[保存] ComponentInfo_SaveComponentInfo: 空路径，跳过");
				invalidPaths.Push (path);
			}
		}
		
		if (!invalidPaths.IsEmpty ()) {
			GS::UniString warningMsg = "以下图片文件不存在或无效，已跳过：";
			for (UInt32 i = 0; i < invalidPaths.GetSize () && i < 3; ++i) {
				if (i > 0) warningMsg += ", ";
				warningMsg += invalidPaths[i];
			}
			if (invalidPaths.GetSize () > 3) {
				warningMsg += GS::UniString::SPrintf (" ... 共 %d 个文件", invalidPaths.GetSize ());
			}
			ACAPI_WriteReport (warningMsg.ToCStr ().Get (), true);
		}
		
		GS::Array<GS::UniString> newRelPaths;
		if (!photoLocsForCopying.IsEmpty ()) {
			NewPluginDebugLog ("[保存] ComponentInfo_SaveComponentInfo: 开始复制 %d 个新图片文件", photoLocsForCopying.GetSize ());
			
			err = ComponentInfo::CopyPhotosForComponent (elementGuid, photoLocsForCopying, newRelPaths);
			if (err != NoError) {
				NewPluginDebugLog ("[保存] ComponentInfo_SaveComponentInfo: 复制图片失败 err=%d", (int) err);
				GS::UniString errorMsg = GS::UniString::SPrintf ("图片复制失败: %s", ErrID_To_Name (err));
				ACAPI_WriteReport (errorMsg.ToCStr ().Get (), true);
				return ConvertToJavaScriptVariable (false);
			}
			
			NewPluginDebugLog ("[保存] ComponentInfo_SaveComponentInfo: 新图片复制成功，%d 个相对路径", newRelPaths.GetSize ());
		} else if (photoPaths.IsEmpty ()) {
			// 没有图片路径是正常的（用户可能删除了所有图片）
			NewPluginDebugLog ("[保存] ComponentInfo_SaveComponentInfo: 没有图片路径");
		}
		
		// 合并已有的相对路径和新复制的相对路径
		photoPaths.Clear ();
		for (const GS::UniString& relPath : existingRelPaths) {
			photoPaths.Push (relPath);
		}
		for (const GS::UniString& relPath : newRelPaths) {
			photoPaths.Push (relPath);
		}
		
		NewPluginDebugLog ("[保存] ComponentInfo_SaveComponentInfo: 总共 %d 个图片路径 (%d 已有 + %d 新增)", 
			photoPaths.GetSize (), existingRelPaths.GetSize (), newRelPaths.GetSize ());
		
		err = ComponentInfo::WritePropertiesToElement (elementGuid, guids, componentId, photoDesc, photoPaths);
		if (err != NoError) {
			NewPluginDebugLog ("[保存] ComponentInfo_SaveComponentInfo: 写入属性失败 err=%d", (int) err);
			return ConvertToJavaScriptVariable (false);
		}
		
		NewPluginDebugLog ("[保存] ComponentInfo_SaveComponentInfo: 保存成功");
		return ConvertToJavaScriptVariable (true);
	}));
	
	// Test function to verify JavaScript bridge is working
	jsACAPI->AddItem (new JS::Function ("TestBridge", [] (GS::Ref<JS::Base> param) {
		NewPluginDebugLog ("[JS桥] TestBridge 函数被调用，JavaScript桥接工作正常");
		ACAPI_WriteReport ("TestBridge: JavaScript bridge is working", false);
		return ConvertToJavaScriptVariable (true);
	}));
	
	// Debug logging function for HTML to send messages to C++ log
	jsACAPI->AddItem (new JS::Function ("LogDebug", [] (GS::Ref<JS::Base> param) {
		GS::UniString message = GetStringFromJavaScriptVariable (param);
		NewPluginDebugLog ("[HTML调试] %s", message.ToCStr ().Get ());
		return ConvertToJavaScriptVariable (true);
	}));
	
	jsACAPI->AddItem (new JS::Function ("ComponentInfo_SelectPhotos", [] (GS::Ref<JS::Base> param) {
		NewPluginDebugLog ("[JS桥] ComponentInfo_SelectPhotos 函数被调用");
		
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (param);
		NewPluginDebugLog ("[JS桥] ComponentInfo_SelectPhotos 提取的elementGuidStr: '%s' (长度: %d)", 
			elementGuidStr.ToCStr ().Get (), elementGuidStr.GetLength ());
		
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		(void)elementGuid; // Not used currently, but part of API signature
		
		// Open file picker dialog (similar to DoSelectPhotos in palette)
		NewPluginDebugLog ("ComponentInfo_SelectPhotos called for element: %s", elementGuidStr.ToCStr ().Get ());
		ACAPI_WriteReport ("ComponentInfo_SelectPhotos called for element: %s", false, elementGuidStr.ToCStr ().Get ());
		
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
		NewPluginDebugLog ("Opening file dialog...");
		if (dlg.Invoke ()) {
			USize n = dlg.GetSelectionCount ();
			NewPluginDebugLog ("User selected %d photos", n);
			ACAPI_WriteReport ("User selected %d photos", false, n);
			for (UIndex i = 0; i < n; ++i) {
				GS::UniString path;
				dlg.GetSelectedFile (i).ToPath (&path);
				selectedPaths.Push (path);
				ACAPI_WriteReport ("  Photo %d: %s", false, i + 1, path.ToCStr ().Get ());
			}
		} else {
			NewPluginDebugLog ("File dialog canceled or failed");
			ACAPI_WriteReport ("File dialog canceled or failed", false);
		}
		
		NewPluginDebugLog ("ComponentInfo_SelectPhotos returning %d paths", selectedPaths.GetSize ());
		ACAPI_WriteReport ("ComponentInfo_SelectPhotos returning %d paths", false, selectedPaths.GetSize ());
		return ConvertToJavaScriptVariable (selectedPaths);
	}));
	
	jsACAPI->AddItem (new JS::Function ("ComponentInfo_GetPhotoFullPath", [] (GS::Ref<JS::Base> param) {
		GS::UniString relPath = GetStringFromJavaScriptVariable (param);
		IO::Location fullPath;
		GSErrCode err = ComponentInfo::GetPhotoFullPath (relPath, fullPath);
		if (err == NoError) {
			return ConvertToJavaScriptVariable (fullPath.ToDisplayText ());
		}
		return ConvertToJavaScriptVariable (GS::EmptyUniString);
	}));
	
	jsACAPI->AddItem (new JS::Function ("ComponentInfo_ViewPhotos", [] (GS::Ref<JS::Base> param) {
		// param should be an array: [elementGuid, photoRelativePath] or just [elementGuid] (open folder)
		GS::Ref<JS::Array> params = GetArrayFromJavaScriptVariable (param);
		if (params == nullptr || params->GetItemArray ().GetSize () < 1) {
			return ConvertToJavaScriptVariable (false);
		}
		
		const GS::Array<GS::Ref<JS::Base>>& paramItems = params->GetItemArray ();
		GS::UniString elementGuidStr = GetStringFromJavaScriptVariable (paramItems[0]);
		API_Guid elementGuid = APIGuidFromString (elementGuidStr.ToCStr ().Get ());
		
		IO::Location openCmd;
#if defined (GS_MAC)
		openCmd = IO::Location ("/usr/bin/open");
#elif defined (GS_WIN)
		openCmd = IO::Location ("C:\\Windows\\explorer.exe");
#endif
		
		GS::Array<GS::UniString> argv;
		
		if (paramItems.GetSize () >= 2) {
			// Open specific photo file
			GS::UniString relPath = GetStringFromJavaScriptVariable (paramItems[1]);
			
			// Get project folder
			IO::Location photoFolderLoc, projectFolderLoc;
			GS::UniString projectName;
			GSErrCode err = ComponentInfo::GetProjectPhotoFolder (photoFolderLoc, projectFolderLoc, projectName);
			if (err != NoError) {
				return ConvertToJavaScriptVariable (false);
			}
			
			// Build full path from project folder + relative path
			IO::Location fullPathLoc = projectFolderLoc;
			GS::Array<GS::UniString> pathParts;
			UIndex start = 0;
			for (UIndex i = 0; i <= relPath.GetLength (); ++i) {
				if (i == relPath.GetLength () || relPath[i] == '/' || relPath[i] == '\\') {
					if (i > start) {
						pathParts.Push (relPath.GetSubstring (start, i - start));
					}
					start = i + 1;
				}
			}
			
			for (UInt32 p = 0; p < pathParts.GetSize (); ++p) {
				if (fullPathLoc.AppendToLocal (IO::Name (pathParts[p])) != NoError) {
					return ConvertToJavaScriptVariable (false);
				}
			}
			
			GS::UniString fullPath;
			if (fullPathLoc.ToPath (&fullPath) != NoError) {
				return ConvertToJavaScriptVariable (false);
			}
			
			argv.Push (fullPath);  // Open the photo file
		} else {
			// Open folder (fallback behavior)
			IO::Location componentPhotoFolder, projectFolderLoc;
			GS::UniString projectName;
			GSErrCode err = ComponentInfo::GetComponentPhotoFolder (elementGuid, componentPhotoFolder, projectFolderLoc, projectName);
			if (err != NoError) {
				return ConvertToJavaScriptVariable (false);
			}
			
			GS::UniString folderPath;
			if (componentPhotoFolder.ToPath (&folderPath) != NoError) {
				return ConvertToJavaScriptVariable (false);
			}
			
			argv.Push (folderPath);  // Open the folder
		}
		
		GSErrCode err = IO::Process::ApplicationLauncher::Instance ().Launch (openCmd, argv);
		return ConvertToJavaScriptVariable (err == NoError);
	}));
	
	jsACAPI->AddItem (new JS::Function ("ComponentInfo_GetPhotoBase64", [] (GS::Ref<JS::Base> param) {
		GS::Ref<JS::Array> params = GetArrayFromJavaScriptVariable (param);
		if (params == nullptr || params->GetItemArray ().GetSize () < 2) {
			return ConvertToJavaScriptVariable (GS::EmptyUniString);
		}
		
		const GS::Array<GS::Ref<JS::Base>>& paramItems = params->GetItemArray ();
		GS::UniString relPath = GetStringFromJavaScriptVariable (paramItems[1]);
		
		IO::Location photoFolderLoc, projectFolderLoc;
		GS::UniString projectName;
		GSErrCode err = ComponentInfo::GetProjectPhotoFolder (photoFolderLoc, projectFolderLoc, projectName);
		if (err != NoError) {
			return ConvertToJavaScriptVariable (GS::EmptyUniString);
		}
		
		IO::Location fullPathLoc = projectFolderLoc;
		GS::Array<GS::UniString> pathParts;
		UIndex start = 0;
		for (UIndex i = 0; i <= relPath.GetLength (); ++i) {
			if (i == relPath.GetLength () || relPath[i] == '/' || relPath[i] == '\\') {
				if (i > start) {
					pathParts.Push (relPath.GetSubstring (start, i - start));
				}
				start = i + 1;
			}
		}
		
		for (UInt32 p = 0; p < pathParts.GetSize (); ++p) {
			if (fullPathLoc.AppendToLocal (IO::Name (pathParts[p])) != NoError) {
				return ConvertToJavaScriptVariable (GS::EmptyUniString);
			}
		}
		
		GS::UniString fullPath;
		if (fullPathLoc.ToPath (&fullPath) != NoError) {
			return ConvertToJavaScriptVariable (GS::EmptyUniString);
		}
		
		IO::Location fileLoc (fullPath);
		IO::File file (fileLoc);
		if (file.Open (IO::File::ReadMode) != NoError) {
			return ConvertToJavaScriptVariable (GS::EmptyUniString);
		}
		
		UInt64 fileSize = 0;
		if (file.GetDataLength (&fileSize) != NoError) {
			file.Close ();
			return ConvertToJavaScriptVariable (GS::EmptyUniString);
		}
		
		if (fileSize <= 0 || fileSize > 10 * 1024 * 1024) {
			file.Close ();
			return ConvertToJavaScriptVariable (GS::EmptyUniString);
		}
		
		GS::Array<char> buffer (fileSize);
		char* bufferPtr = buffer.GetContent ();
		err = file.Read (bufferPtr, fileSize);
		file.Close ();
		
		if (err != NoError) {
			return ConvertToJavaScriptVariable (GS::EmptyUniString);
		}
		
		const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		GS::UniString base64Result;
		
		for (GSSize i = 0; i < fileSize; i += 3) {
			unsigned char byte1 = i < fileSize ? (unsigned char) buffer[i] : 0;
			unsigned char byte2 = i + 1 < fileSize ? (unsigned char) buffer[i + 1] : 0;
			unsigned char byte3 = i + 2 < fileSize ? (unsigned char) buffer[i + 2] : 0;
			
			unsigned int triple = (byte1 << 16) | (byte2 << 8) | byte3;
			
			char encoded[4];
			encoded[0] = base64Chars[(triple >> 18) & 0x3F];
			encoded[1] = base64Chars[(triple >> 12) & 0x3F];
			encoded[2] = base64Chars[(triple >> 6) & 0x3F];
			encoded[3] = base64Chars[triple & 0x3F];
			
			base64Result.Append (encoded, 4);
		}
		
		GSSize padding = fileSize % 3;
		if (padding == 1) {
			base64Result.Delete (base64Result.GetLength () - 2, 2);
			base64Result.Append ("==");
		} else if (padding == 2) {
			base64Result.Delete (base64Result.GetLength () - 1, 1);
			base64Result.Append ("=");
		}
		
		GS::UniString mimeType = "image/jpeg";
		if (fullPath.ToLowerCase ().EndsWith (".png")) {
			mimeType = "image/png";
		} else if (fullPath.ToLowerCase ().EndsWith (".gif")) {
			mimeType = "image/gif";
		}
		
		GS::UniString dataUrl = "data:" + mimeType + ";base64," + base64Result;
		return ConvertToJavaScriptVariable (dataUrl);
	}));
	
	jsACAPI->AddItem (new JS::Function ("ComponentInfo_SetEditMode", [] (GS::Ref<JS::Base> param) {
		bool editMode = GetBoolFromJavaScriptVariable (param);
		PluginPalette::SetEditMode (editMode);
		return ConvertToJavaScriptVariable (true);
	}));
	
	// Function to notify C++ about unsaved changes
	jsACAPI->AddItem (new JS::Function ("ComponentInfo_SetUnsavedChanges", [] (GS::Ref<JS::Base> param) {
		bool hasChanges = GetBoolFromJavaScriptVariable (param);
		if (PluginPalette::HasInstance ()) {
			PluginPalette::GetInstance ().hasUnsavedChanges = hasChanges;
		}
		return ConvertToJavaScriptVariable (true);
	}));

	// Register the ACAPI object with the browser
	NewPluginDebugLog ("[JS桥] 正在注册JavaScript异步对象...");
	browser.RegisterAsynchJSObject (jsACAPI);
	NewPluginDebugLog ("[JS桥] RegisterAsynchJSObject 完成");
	ACAPI_WriteReport ("JavaScript桥接注册完成", false);
}

// Static methods implementation
bool PluginPalette::HasInstance ()
{
	return s_instance != nullptr;
}

void PluginPalette::CreateInstance ()
{
	if (!HasInstance ()) {
		s_instance = new PluginPalette ();
	}
}

PluginPalette& PluginPalette::GetInstance ()
{
	return *s_instance;
}

void PluginPalette::DestroyInstance ()
{
	s_instance = nullptr;
}

void PluginPalette::Show ()
{
	SetMenuItemCheckedState (true);
	DG::Palette::Show ();
}

void PluginPalette::Hide ()
{
	SetMenuItemCheckedState (false);
	DG::Palette::Hide ();
}

void PluginPalette::UpdateFromSelection ()
{
	if (!HasInstance ())
		return;
	
	// Note: JavaScript bridge is one-way (JS -> C++), so we can't call JS from C++
	// The HTML will poll for selection changes via UpdateSelectedElements() function
	// This function is kept for future use or C++-side selection processing
	NewPluginDebugLog ("[选择] UpdateFromSelection called");
}

bool PluginPalette::IsInEditMode ()
{
	if (HasInstance ()) {
		return GetInstance ().isEditMode;
	}
	return false;
}

void PluginPalette::SetEditMode (bool editMode)
{
	if (HasInstance ()) {
		PluginPalette& instance = GetInstance ();
		instance.isEditMode = editMode;
		
		// Selection control is handled by SelectionChangeHandler
		// ACAPI_DisableSelection does not exist in ArchiCAD API
		// Use SelectionChangeHandler to reject selection changes while in edit mode
		
		instance.UpdateEditModeUI ();
	}
}

void PluginPalette::RefreshContent ()
{
	if (!HasInstance ())
		return;
	
	// Reload HTML content
	GS::UniString htmlContent = LoadHtmlFromResource ();
	if (!htmlContent.IsEmpty ()) {
		GetInstance ().browser.LoadHTML (htmlContent);
		// Re-initialize JavaScript bridge after reload
		GetInstance ().InitializeJavaScriptBridge ();
	}
}

void PluginPalette::SetMenuItemCheckedState (bool checked)
{
	// Update menu item check state via API
	API_MenuItemRef itemRef;
	itemRef.menuResID = 32500;
	itemRef.itemIndex = 1;
	
	GSFlags itemFlags = 0;
	GSErrCode err = ACAPI_MenuItem_GetMenuItemFlags (&itemRef, &itemFlags);
	if (err == NoError) {
		if (checked) {
			itemFlags |= API_MenuItemChecked;
		} else {
			itemFlags &= (GSFlags) ~API_MenuItemChecked;
		}
		err = ACAPI_MenuItem_SetMenuItemFlags (&itemRef, &itemFlags);
		if (err != NoError) {
			NewPluginDebugLog ("[菜单] SetMenuItemCheckedState 失败: %d", (int) err);
		}
	} else {
		NewPluginDebugLog ("[菜单] GetMenuItemFlags 失败: %d", (int) err);
	}
}

void PluginPalette::DoSave ()
{
	if (!HasInstance ())
		return;
	
	// Note: JavaScript bridge is one-way (JS -> C++), so we can't call JS from C++
	// The save operation should be triggered from JavaScript side
	// This function is kept for future use or C++-side save processing
	NewPluginDebugLog ("[保存] DoSave called - save should be triggered from JavaScript");
	// Clear unsaved changes flag (will be set again if save fails)
	GetInstance ().hasUnsavedChanges = false;
}

void PluginPalette::UpdateEditModeUI ()
{
	if (!HasInstance ())
		return;
	
	// Note: JavaScript bridge is one-way (JS -> C++), so we can't call JS from C++
	// The UI update should be handled by JavaScript polling or event handling
	// This function is kept for future use or C++-side UI processing
	NewPluginDebugLog ("[UI] UpdateEditModeUI called, isEditMode=%d", isEditMode ? 1 : 0);
}

bool PluginPalette::HasUnsavedChanges () const
{
	return hasUnsavedChanges;
}

void PluginPalette::PanelResized (const DG::PanelResizeEvent& ev)
{
	BeginMoveResizeItems ();
	browser.Resize (ev.GetHorizontalChange (), ev.GetVerticalChange ());
	EndMoveResizeItems ();
}

void PluginPalette::PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted)
{
	// Check for unsaved changes
	if (HasUnsavedChanges ()) {
		// Show confirmation dialog using simple DGAlert
		// Note: DGAlert only supports simple OK/Cancel, so we'll use a warning
		short result = DGAlert (DG_WARNING, 
			GS::UniString ("未保存的更改"),
			GS::UniString ("您有未保存的更改。关闭面板将丢失这些更改。"),
			GS::UniString ("是否继续关闭？"),
			GS::UniString ("取消"));
		
		if (result == DG_OK) {
			// User clicked OK (Cancel button) - keep panel open
			*accepted = false;
		} else {
			// User dismissed or clicked Cancel - discard changes and close
			hasUnsavedChanges = false;
			*accepted = true;
		}
	} else {
		*accepted = true;
	}
}

// Static callback methods
GSErrCode PluginPalette::PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param)
{
	// Handle palette control messages
	if (!HasInstance ())
		return NoError;
	
	PluginPalette& instance = GetInstance ();
	
	switch (messageID) {
		case APIPalMsg_OpenPalette:
			// Palette opened
			NewPluginDebugLog ("[面板] 面板已打开");
			instance.UpdateFromSelection ();
			break;
			
		case APIPalMsg_ClosePalette:
			// Palette closed - check for unsaved changes
			if (instance.HasUnsavedChanges ()) {
				// Show confirmation dialog
				short result = DGAlert (DG_WARNING, 
					GS::UniString ("未保存的更改"),
					GS::UniString ("您有未保存的更改。关闭面板将丢失这些更改。"),
					GS::UniString ("是否继续关闭？"),
					GS::UniString ("取消"));
				
				if (result == DG_OK) {
					// User clicked Cancel - prevent closing
					return APIERR_CANCEL;
				}
				// User dismissed - discard changes
			}
			instance.hasUnsavedChanges = false;
			break;
			
		case APIPalMsg_HidePalette_Begin:
			// Palette should be hidden
			if (instance.isEditMode) {
				instance.SetEditMode (false);
			}
			break;
			
		case APIPalMsg_HidePalette_End:
			// Palette can be shown again
			instance.UpdateFromSelection ();
			break;
			
		case APIPalMsg_DisableItems_Begin:
			// Disable palette items during user input
			if (instance.isEditMode) {
				instance.SetEditMode (false);
			}
			break;
			
		case APIPalMsg_DisableItems_End:
			// Enable palette items again
			instance.UpdateFromSelection ();
			break;
			
		default:
			// Unknown message - ignore
			break;
	}
	
	return NoError;
}

GSErrCode PluginPalette::SelectionChangeHandler (const API_Neig* selElemNeig)
{
	if (!HasInstance ())
		return NoError;
	
	PluginPalette& instance = GetInstance ();
	
	// If in edit mode, reject any selection changes
	if (instance.isEditMode) {
		// Check if selection is actually changing
		if (selElemNeig != nullptr) {
			API_Guid newGuid = selElemNeig->guid;
			if (newGuid != APINULLGuid && newGuid != instance.currentElemGuid) {
				// Attempting to select different element while in edit mode - reject
				NewPluginDebugLog ("[选择] 编辑模式下阻止选择其他元素");
				return APIERR_CANCEL; // Reject selection change
			}
		} else {
			// Attempting to deselect while in edit mode - reject
			NewPluginDebugLog ("[选择] 编辑模式下阻止取消选择");
			return APIERR_CANCEL;
		}
		// Selection not changing (same element or null), allow
	}
	
	// Update current element GUID
	if (selElemNeig != nullptr) {
		instance.currentElemGuid = selElemNeig->guid;
	} else {
		instance.currentElemGuid = APINULLGuid;
	}
	
	// Trigger UI update
	instance.UpdateFromSelection ();
	
	return NoError;
}

GSErrCode PluginPalette::RegisterPaletteControlCallBack ()
{
	return ACAPI_RegisterModelessWindow (
		GetPaletteReferenceId (),
		PaletteControlCallBack,
		API_PalEnabled_FloorPlan + API_PalEnabled_Section + API_PalEnabled_Elevation +
		API_PalEnabled_InteriorElevation + API_PalEnabled_3D + API_PalEnabled_Detail +
		API_PalEnabled_Worksheet + API_PalEnabled_Layout + API_PalEnabled_DocumentFrom3D,
		GSGuid2APIGuid (s_paletteGuid));
}

Int32 PluginPalette::GetPaletteReferenceId ()
{
	return GS::CalculateHashValue (s_paletteGuid);
}