#include "PluginPalette.hpp"
#include "NewPluginDebug.hpp"
#include "APICommon.h"
#include "ComponentInfo.hpp"
#include "DGUtility.hpp"
#include "DGDialog.hpp"
#include "DGFileDialog.hpp"
#include "FileTypeManager.hpp"
#include <mutex>

namespace {
    struct ComponentInfoResult {
        GS::UniString componentId;
        GS::UniString photoDesc;
        GS::Array<GS::UniString> photoPaths;
        GS::UniString globalId;
        GS::UniString folderName;
    };
}





static const GS::Guid s_paletteGuid ("{A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}");
static GS::Ref<PluginPalette> s_instance;
static std::recursive_mutex s_instanceMutex;

static GSErrCode NotificationHandler (API_NotifyEventID notifID, Int32)
{
	if (notifID == APINotify_Quit)
		PluginPalette::DestroyInstance ();
	return NoError;
}

PluginPalette::PluginPalette ()
	: DG::Palette (ACAPI_GetOwnResModule (), PaletteResId, ACAPI_GetOwnResModule (), s_paletteGuid)
	, currentElemGuid (APINULLGuid)
	, isEditMode (false)
	, hasUnsavedChanges (false)
	, titleLabel (GetReference (), TitleLabelId)
	, guidLabel (GetReference (), GuidLabelId)
	, typeLabel (GetReference (), TypeLabelId)
	, infoTitleLabel (GetReference (), InfoTitleLabelId)
	, compIdLabel (GetReference (), CompIdLabelId)
	, componentIdEdit (GetReference (), ComponentIdEditId)
	, photoDescLabel (GetReference (), PhotoDescLabelId)
	, photoDescEdit (GetReference (), PhotoDescEditId)
	, photoPreviewTitleLabel (GetReference (), PhotoPreviewTitleLabelId)
	, photoPreviewLabel (GetReference (), PhotoPreviewLabelId)
	, editInfoButton (GetReference (), EditInfoButtonId)
	, selectPhotoButton (GetReference (), SelectPhotoButtonId)
	, viewPhotoButton (GetReference (), ViewPhotoButtonId)
	, saveButton (GetReference (), SaveButtonId)
	, cancelButton (GetReference (), CancelButtonId)
{
	NewPluginDebugLog ("[面板] PluginPalette 构造函数开始 PaletteResId=%d", (int) PaletteResId);
	ACAPI_ProjectOperation_CatchProjectEvent (APINotify_Quit, NotificationHandler);
	Attach (*this);
	editInfoButton.Attach (*this);
	selectPhotoButton.Attach (*this);
	viewPhotoButton.Attach (*this);
	saveButton.Attach (*this);
	cancelButton.Attach (*this);
	BeginEventProcessing ();

	NewPluginDebugLog ("[面板] BeginEventProcessing 完成");
	
	// 设置控件文本
	titleLabel.SetText ("HBIM构件信息输入工具 (0.2.14.125)");
	guidLabel.SetText ("GUID: 未选择构件");
	typeLabel.SetText ("TYPE: 未选择构件");
	infoTitleLabel.SetText ("构件信息填写");
	compIdLabel.SetText ("构件编号:");
	photoDescLabel.SetText ("照片描述:");
	photoPreviewTitleLabel.SetText ("照片预览");
	photoPreviewLabel.SetText ("暂无照片");
	editInfoButton.SetText ("编辑构件信息");
	selectPhotoButton.SetText ("选择照片");
	viewPhotoButton.SetText ("查看照片");
	saveButton.SetText ("保存");
	cancelButton.SetText ("取消");
	
	// 输入框默认为空
	componentIdEdit.SetText ("");
	photoDescEdit.SetText ("");
	
	// 禁用按钮（未选择构件时）
	editInfoButton.Disable ();
	selectPhotoButton.Disable ();
	viewPhotoButton.Disable ();
	saveButton.Hide ();
	cancelButton.Hide ();
	
	// 禁用输入框（未选择构件时）
	componentIdEdit.Disable ();
	photoDescEdit.Disable ();
	
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


	


// Static methods implementation
bool PluginPalette::HasInstance ()
{
	std::lock_guard<std::recursive_mutex> lock(s_instanceMutex);
	return s_instance != nullptr;
}

void PluginPalette::CreateInstance ()
{
	std::lock_guard<std::recursive_mutex> lock(s_instanceMutex);
	if (s_instance == nullptr) {
		s_instance = new PluginPalette ();
	}
}

PluginPalette& PluginPalette::GetInstance ()
{
	std::lock_guard<std::recursive_mutex> lock(s_instanceMutex);
	// Caller must ensure instance exists (via HasInstance())
	// This is internal code, so we assume it's safe
	return *s_instance;
}

void PluginPalette::DestroyInstance ()
{
	std::lock_guard<std::recursive_mutex> lock(s_instanceMutex);
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
	
	NewPluginDebugLog ("[选择] UpdateFromSelection called");
	
	// 刷新面板内容
	RefreshContent ();
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
		
		if (editMode) {
			// 进入编辑模式时重置未保存状态
			instance.hasUnsavedChanges = false;
			instance.pendingPhotoLocations.Clear ();
		}
		
		instance.UpdateEditModeUI ();
	}
}

void PluginPalette::RefreshContent ()
{
	if (!HasInstance ())
		return;
	
	NewPluginDebugLog ("[面板] RefreshContent");
	
	PluginPalette& instance = GetInstance ();
	
	// 获取当前选中元素
	GS::Array<ElementInfo> selectedElements = GetSelectedElements ();
	
	if (selectedElements.IsEmpty ()) {
		instance.currentElemGuid = APINULLGuid;
		instance.guidLabel.SetText ("GUID: 未选择构件");
		instance.typeLabel.SetText ("TYPE: 未选择构件");
		instance.componentIdEdit.SetText ("");
		instance.photoDescEdit.SetText ("");
		instance.photoPreviewLabel.SetText ("暂无照片");
		return;
	}
	
	// 只处理第一个选中元素
	const ElementInfo& elemInfo = selectedElements[0];
	instance.currentElemGuid = APIGuidFromString (elemInfo.guidStr.ToCStr ());
	
	GS::UniString guidText;
	guidText.Printf ("GUID: %s", elemInfo.guidStr.ToCStr ().Get ());
	instance.guidLabel.SetText (guidText);
	
	GS::UniString typeText;
	typeText.Printf ("TYPE: %s", elemInfo.typeName.ToCStr ().Get ());
	instance.typeLabel.SetText (typeText);
	
	NewPluginDebugLog ("[面板] 当前选中元素: GUID=%s, 类型=%s", 
		elemInfo.guidStr.ToCStr ().Get (),
		elemInfo.typeName.ToCStr ().Get ());
	
	// 读取构件属性并填充到编辑控件
	ComponentInfo::PropertyDefinitionGuids guids;
	GSErrCode err = ComponentInfo::FindPropertyDefinitions (guids);
	if (err == NoError) {
		GS::UniString compId;
		GS::UniString photoDesc;
		GS::Array<GS::UniString> photoPaths;
		
		err = ComponentInfo::ReadPropertiesFromElement (instance.currentElemGuid, guids, compId, photoDesc, photoPaths);
		if (err == NoError) {
			// 状态3：已有信息 - 显示原值
			if (compId.IsEmpty ()) {
				instance.componentIdEdit.SetText ("请填写构件编号...");
			} else {
				instance.componentIdEdit.SetText (compId);
			}
			
			if (photoDesc.IsEmpty ()) {
				instance.photoDescEdit.SetText ("请填写照片描述...");
			} else {
				instance.photoDescEdit.SetText (photoDesc);
			}
			
			NewPluginDebugLog ("[面板] 状态3-已有信息: 构件编号='%s', 照片说明='%s'", 
				compId.ToCStr ().Get (),
				photoDesc.ToCStr ().Get ());
		} else {
			// 状态2：已选择但无信息 - 显示"请填写..."
			instance.componentIdEdit.SetText ("请填写构件编号...");
			instance.photoDescEdit.SetText ("请填写照片描述...");
			NewPluginDebugLog ("[面板] 状态2-已选择但无信息: 读取构件属性失败: %d", (int) err);
		}
	} else {
		// 状态2：已选择但无信息 - 显示"请填写..."
		instance.componentIdEdit.SetText ("请填写构件编号...");
		instance.photoDescEdit.SetText ("请填写照片描述...");
		NewPluginDebugLog ("[面板] 状态2-已选择但无信息: 查找属性定义失败: %d", (int) err);
	}
	
	// 更新照片预览区域
	if (selectedElements.IsEmpty ()) {
		instance.photoPreviewLabel.SetText ("照片预览区域");
	} else {
		// 检查是否有照片
		ComponentInfo::ComponentInfoData info;
		GSErrCode infoErr = ComponentInfo::GetComponentInfo (instance.currentElemGuid, info);
		if (infoErr == NoError && !info.photoPaths.IsEmpty ()) {
			GS::UniString previewText;
			previewText.Printf ("有 %d 张照片", (int) info.photoPaths.GetSize ());
			instance.photoPreviewLabel.SetText (previewText);
		} else {
			instance.photoPreviewLabel.SetText ("暂无照片");
		}
	}
	
	instance.UpdateEditModeUI ();
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
	
	PluginPalette& instance = GetInstance ();
	
	if (instance.currentElemGuid == APINULLGuid) {
		NewPluginDebugLog ("[保存] 未选择构件");
		return;
	}
	
	NewPluginDebugLog ("[保存] DoSave called");
	
	GS::UniString compId = instance.componentIdEdit.GetText ();
	GS::UniString photoDesc = instance.photoDescEdit.GetText ();
	
	GS::Array<GS::UniString> savedRelPaths;
	
	if (!instance.pendingPhotoLocations.IsEmpty ()) {
		NewPluginDebugLog ("[保存] 正在复制 %d 张照片...", (int) instance.pendingPhotoLocations.GetSize ());
		
		GSErrCode copyErr = ComponentInfo::CopyPhotosForComponent (
			instance.currentElemGuid, 
			instance.pendingPhotoLocations, 
			savedRelPaths, 
			0);
		
		if (copyErr != NoError) {
			NewPluginDebugLog ("[保存] 复制照片失败: %d", (int) copyErr);
		} else {
			NewPluginDebugLog ("[保存] 已复制 %d 张照片", (int) savedRelPaths.GetSize ());
		}
		
		instance.pendingPhotoLocations.Clear ();
	}
	
	ComponentInfo::ComponentInfoData data;
	data.componentId = compId;
	data.photoDesc = photoDesc;
	data.photoPaths = savedRelPaths;
	data.globalId = ComponentInfo::GetGlobalIdPropertyValue (instance.currentElemGuid);
	data.folderName = ComponentInfo::GlobalIdToFolderName (instance.currentElemGuid);
	
	GSErrCode err = ComponentInfo::SaveComponentInfo (instance.currentElemGuid, data);
	if (err == NoError) {
		NewPluginDebugLog ("[保存] 保存成功: 构件编号='%s', 照片说明='%s', 照片数=%d", 
			compId.ToCStr ().Get (), photoDesc.ToCStr ().Get (), (int) savedRelPaths.GetSize ());
		instance.hasUnsavedChanges = false;
		instance.SetEditMode (false);
	} else {
		NewPluginDebugLog ("[保存] 保存失败: %d", (int) err);
	}
}

void PluginPalette::UpdateEditModeUI ()
{
	if (!HasInstance ())
		return;
	
	PluginPalette& instance = GetInstance ();
	bool hasSelection = (instance.currentElemGuid != APINULLGuid);
	
	NewPluginDebugLog ("[UI] UpdateEditModeUI called, isEditMode=%d, hasSelection=%d", 
		instance.isEditMode ? 1 : 0, hasSelection ? 1 : 0);
	
	if (instance.isEditMode) {
		instance.componentIdEdit.Enable ();
		instance.photoDescEdit.Enable ();
		instance.editInfoButton.Disable ();
		instance.selectPhotoButton.Enable ();
		instance.saveButton.Show ();
		instance.saveButton.Enable ();
		instance.cancelButton.Show ();
		instance.cancelButton.Enable ();
	} else {
		instance.componentIdEdit.Disable ();
		instance.photoDescEdit.Disable ();
		instance.saveButton.Hide ();
		instance.cancelButton.Hide ();
		
		if (hasSelection) {
			instance.editInfoButton.Enable ();
			instance.selectPhotoButton.Enable ();
			instance.viewPhotoButton.Enable ();
		} else {
			instance.editInfoButton.Disable ();
			instance.selectPhotoButton.Disable ();
			instance.viewPhotoButton.Disable ();
		}
	}
}

bool PluginPalette::HasUnsavedChanges () const
{
	return hasUnsavedChanges;
}

void PluginPalette::PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted)
{
	// Check for unsaved changes
	if (HasUnsavedChanges ()) {
		NewPluginDebugLog ("[面板] 有未保存的更改，阻止关闭");
		*accepted = false;
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

void PluginPalette::ButtonClicked (const DG::ButtonClickEvent& ev)
{
	if (!HasInstance ())
		return;
	
	PluginPalette& instance = GetInstance ();
	Int32 buttonId = ev.GetSource ()->GetId ();
	
		switch (buttonId) {
		case EditInfoButtonId:
			NewPluginDebugLog ("[按钮] 编辑构件信息按钮 clicked");
			instance.SetEditMode (true);
			break;
			
		case SelectPhotoButtonId:
			NewPluginDebugLog ("[按钮] 选择照片按钮 clicked");
			instance.DoSelectPhotos ();
			break;
			
		case ViewPhotoButtonId:
			NewPluginDebugLog ("[按钮] 查看照片按钮 clicked");
			instance.DoViewPhotos ();
			break;
			
		case SaveButtonId:
			NewPluginDebugLog ("[按钮] 保存按钮 clicked");
			instance.DoSave ();
			break;
			
		case CancelButtonId:
			NewPluginDebugLog ("[按钮] 取消按钮 clicked");
			instance.hasUnsavedChanges = false;
			instance.pendingPhotoLocations.Clear ();
			instance.SetEditMode (false);
			instance.RefreshContent (); // 恢复到原始状态
			break;
		}
}

void PluginPalette::DoSelectPhotos ()
{
	if (currentElemGuid == APINULLGuid) {
		NewPluginDebugLog ("[照片] 未选择构件");
		return;
	}
	
	DG::FileDialog dlg (DG::FileDialog::OpenMultiFile);
	FTM::FileTypeManager mgr ("ComponentInfoPhotos");
	FTM::FileType typeJpg ("JPEG", "jpg", 0, 0, 0);
	FTM::FileType typePng ("PNG", "png", 0, 0, 0);
	FTM::TypeID idJpg = mgr.AddType (typeJpg);
	FTM::TypeID idPng = mgr.AddType (typePng);
	dlg.AddFilter (idJpg);
	dlg.AddFilter (idPng);
	dlg.SetTitle ("选择照片");
	
	if (dlg.Invoke ()) {
		pendingPhotoLocations.Clear ();
		USize n = dlg.GetSelectionCount ();
		for (UIndex i = 0; i < n; ++i) {
			pendingPhotoLocations.Push (dlg.GetSelectedFile (i));
		}
		
		GS::UniString t;
		t += "已选 ";
		t += GS::ValueToUniString (pendingPhotoLocations.GetSize ());
		t += " 张新照片";
		
		photoPreviewLabel.SetText (t);
		NewPluginDebugLog ("[照片] 已选择 %d 张照片", (int) pendingPhotoLocations.GetSize ());
		
		// 标记有未保存的更改
		hasUnsavedChanges = true;
	}
}

void PluginPalette::DoViewPhotos ()
{
	if (currentElemGuid == APINULLGuid) {
		NewPluginDebugLog ("[照片] 未选择构件");
		return;
	}
	
	ComponentInfo::ComponentInfoData info;
	GSErrCode err = ComponentInfo::GetComponentInfo (currentElemGuid, info);
	if (err != NoError || info.photoPaths.IsEmpty ()) {
		photoPreviewLabel.SetText ("该构件没有照片");
		NewPluginDebugLog ("[照片] 该构件没有照片");
		return;
	}
	
	GS::UniString msg;
	msg.Printf ("该构件有 %d 张照片:\n", (int) info.photoPaths.GetSize ());
	for (UIndex i = 0; i < info.photoPaths.GetSize () && i < 5; ++i) {
		msg.Append (info.photoPaths[i]);
		msg.Append ("\n");
	}
	if (info.photoPaths.GetSize () > 5) {
		msg.Append ("... 还有更多照片");
	}
	
	photoPreviewLabel.SetText (msg);
	NewPluginDebugLog ("[照片] 查看照片: %d 张", (int) info.photoPaths.GetSize ());
}