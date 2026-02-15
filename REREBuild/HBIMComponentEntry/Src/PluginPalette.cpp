#include "PluginPalette.hpp"
#include "APICommon.h"
#include "APIdefs.h"
#include "DGModule.hpp"
#include <mutex>
#include <stdio.h>

// Property API头文件
#include "APIdefs_Properties.h"

// IFC API头文件
#include "ACAPI/IFCObjectAccessor.hpp"
#include "ACAPI/IFCObjectID.hpp"
#include "ACAPI/IFCPropertyAccessor.hpp"
#include "ACAPI/IFCProperty.hpp"

namespace {
	// HBIM属性常量
	static const GS::UniString kHBIMGroupName = "HBIM属性信息";
	static const GS::UniString kHBIMIdName = "HBIM构件编号";
	static const GS::UniString kHBIMDescName = "HBIM构件说明";
	
	// Forward declaration
	static void CollectClassificationItemsRecursive(const API_Guid& itemGuid, GS::Array<API_Guid>& outGuids);
	
	// Helper function to get IFC type from element
	static GS::UniString GetIFCTypeForElement(const API_Guid& elementGuid)
	{
		GS::UniString ifcType = "未知";
		
		try {
			IFCAPI::ObjectAccessor objectAccessor = IFCAPI::GetObjectAccessor();
			
			API_Elem_Head elemHead{};
			elemHead.guid = elementGuid;
			
			GSErrCode err = ACAPI_Element_GetHeader(&elemHead);
			if (err != NoError)
				return ifcType;
			
			auto objectIDResult = objectAccessor.CreateElementObjectID(elemHead);
			if (objectIDResult.IsErr())
				return ifcType;
			
			IFCAPI::ObjectID objectID = objectIDResult.Unwrap();
			auto ifcTypeResult = objectAccessor.GetIFCType(objectID);
			
			if (ifcTypeResult.IsOk()) {
				ifcType = ifcTypeResult.Unwrap();
			}
		} catch (...) {
			// Handle exception
		}
		
		return ifcType;
	}
	
	// Helper function to get GlobalId from element
	static GS::UniString GetGlobalIdForElement(const API_Guid& elementGuid)
	{
		GS::UniString globalId = "未找到";
		
		try {
			IFCAPI::ObjectAccessor objectAccessor = IFCAPI::GetObjectAccessor();
			
			API_Elem_Head elemHead{};
			elemHead.guid = elementGuid;
			
			GSErrCode err = ACAPI_Element_GetHeader(&elemHead);
			if (err != NoError)
				return globalId;
			
			auto objectIDResult = objectAccessor.CreateElementObjectID(elemHead);
			if (objectIDResult.IsErr())
				return globalId;
			
			IFCAPI::ObjectID objectID = objectIDResult.Unwrap();
			IFCAPI::PropertyAccessor propertyAccessor(objectID);
			
			auto attributesResult = propertyAccessor.GetAttributes();
			if (attributesResult.IsOk()) {
				std::vector<IFCAPI::Attribute> attributes = attributesResult.Unwrap();
				for (const IFCAPI::Attribute& attribute : attributes) {
					if (attribute.GetName().IsEqual("GlobalId", GS::CaseInsensitive)) {
						auto value = attribute.GetValue();
						if (value.has_value()) {
							globalId = value.value();
						}
						break;
					}
				}
			}
		} catch (...) {
			// Handle exception
		}
		
		return globalId;
	}
	
	// 收集所有分类项，使属性对所有元素可用
	static GSErrCode GetAllClassificationItems(GS::Array<API_Guid>& outAllItems)
	{
		outAllItems.Clear();
		GS::Array<API_ClassificationSystem> systems;
		GSErrCode err = ACAPI_Classification_GetClassificationSystems(systems);
		if (err != NoError) return err;
		
		for (UInt32 i = 0; i < systems.GetSize(); ++i) {
			GS::Array<API_ClassificationItem> rootItems;
			if (ACAPI_Classification_GetClassificationSystemRootItems(systems[i].guid, rootItems) == NoError) {
				for (UInt32 j = 0; j < rootItems.GetSize(); ++j) {
					CollectClassificationItemsRecursive(rootItems[j].guid, outAllItems);
				}
			}
		}
		return NoError;
	}
	
	// 递归收集分类项
	static void CollectClassificationItemsRecursive(const API_Guid& itemGuid, GS::Array<API_Guid>& outGuids)
	{
		outGuids.Push(itemGuid);
		GS::Array<API_ClassificationItem> children;
		if (ACAPI_Classification_GetClassificationItemChildren(itemGuid, children) == NoError) {
			for (UInt32 i = 0; i < children.GetSize(); ++i) {
				CollectClassificationItemsRecursive(children[i].guid, outGuids);
			}
		}
	}
	
	// 创建或获取HBIM属性组
	static GSErrCode FindOrCreateHBIMGroup(API_PropertyGroup& outGroup)
	{
		GS::Array<API_PropertyGroup> groups;
		GSErrCode err = ACAPI_Property_GetPropertyGroups(groups);
		if (err != NoError) {
			ACAPI_WriteReport("FindOrCreateHBIMGroup: GetPropertyGroups 失败: %s", true, GS::UniString::Printf("Error %d", err).ToCStr().Get());
			return err;
		}
		
		ACAPI_WriteReport("FindOrCreateHBIMGroup: 找到 %d 个属性组", false, (int)groups.GetSize());
		
		// 记录所有属性组名称用于调试
		for (UInt32 i = 0; i < groups.GetSize(); ++i) {
			ACAPI_WriteReport("  属性组[%d]: %s", false, (int)i, groups[i].name.ToCStr().Get());
		}
		
		for (UInt32 i = 0; i < groups.GetSize(); ++i) {
			if (groups[i].name == kHBIMGroupName) {
				ACAPI_WriteReport("FindOrCreateHBIMGroup: 找到现有属性组: %s", false, kHBIMGroupName.ToCStr().Get());
				outGroup = groups[i];
				return NoError;
			}
		}
		
	ACAPI_WriteReport("FindOrCreateHBIMGroup: 未找到属性组 '%s'，创建新组", false, kHBIMGroupName.ToCStr().Get());
	outGroup = {};
	outGroup.guid = APINULLGuid;
	outGroup.name = kHBIMGroupName;
	outGroup.description = "HBIM构件编号和说明";
	err = ACAPI_Property_CreatePropertyGroup(outGroup);
	if (err != NoError) {
		ACAPI_WriteReport("FindOrCreateHBIMGroup: CreatePropertyGroup 失败: %s (错误码: %d)", true, GS::UniString::Printf("Error %d", err).ToCStr().Get(), err);
		ACAPI_WriteReport("  尝试创建属性组: 名称='%s', 描述='%s'", false, kHBIMGroupName.ToCStr().Get(), "HBIM构件编号和说明");
		
		// 如果创建失败，可能是组已存在但名称比较失败（例如空格、编码问题）
		// 重新获取属性组列表，尝试再次查找
		ACAPI_WriteReport("FindOrCreateHBIMGroup: 创建失败，尝试重新查找现有属性组", false);
		GS::Array<API_PropertyGroup> groups2;
		GSErrCode err2 = ACAPI_Property_GetPropertyGroups(groups2);
		if (err2 == NoError) {
			ACAPI_WriteReport("FindOrCreateHBIMGroup: 重新找到 %d 个属性组", false, (int)groups2.GetSize());
			for (UInt32 i = 0; i < groups2.GetSize(); ++i) {
				ACAPI_WriteReport("  重新检查属性组[%d]: '%s'", false, (int)i, groups2[i].name.ToCStr().Get());
				// 尝试更宽松的比较：去除空格后比较
				GS::UniString existingName = groups2[i].name;
				GS::UniString targetName = kHBIMGroupName;
				// 简单比较：如果名称包含目标字符串或目标字符串包含现有名称
				if (existingName == targetName || 
					existingName.Contains(targetName) || 
					targetName.Contains(existingName)) {
					ACAPI_WriteReport("FindOrCreateHBIMGroup: 通过宽松比较找到属性组: '%s' (目标: '%s')", false, existingName.ToCStr().Get(), targetName.ToCStr().Get());
					outGroup = groups2[i];
					return NoError;
				}
			}
		} else {
			ACAPI_WriteReport("FindOrCreateHBIMGroup: 重新获取属性组失败: %s", true, GS::UniString::Printf("Error %d", err2).ToCStr().Get());
		}
		
		// 检查是否是名称已使用的错误（APIERR_NAMEALREADYUSED ≈ -2130241524?）
		// 错误码 -2130312307 可能表示名称已使用
		ACAPI_WriteReport("FindOrCreateHBIMGroup: 注意: 错误码 %d (0x%X) 可能表示属性组名称已存在", true, err, err);
	} else {
		ACAPI_WriteReport("FindOrCreateHBIMGroup: 成功创建属性组: %s", false, kHBIMGroupName.ToCStr().Get());
	}
	return err;
	}
	
	// 创建或获取HBIM属性定义
	static GSErrCode FindOrCreateHBIMDefinition(const API_PropertyGroup& group, const GS::UniString& name, 
												API_PropertyDefinition& outDef, GS::Array<API_Guid>& allClassificationItems)
	{
		GS::Array<API_PropertyDefinition> defs;
		GSErrCode err = ACAPI_Property_GetPropertyDefinitions(group.guid, defs);
		if (err != NoError) {
			ACAPI_WriteReport("FindOrCreateHBIMDefinition [%s]: GetPropertyDefinitions 失败: %s", true, name.ToCStr().Get(), GS::UniString::Printf("Error %d", err).ToCStr().Get());
			return err;
		}
		for (UInt32 i = 0; i < defs.GetSize(); ++i) {
			if (defs[i].name == name) {
				outDef = defs[i];
				return NoError;
			}
		}
		// 属性不存在，创建新的
		outDef = {};
		outDef.guid = APINULLGuid;
		outDef.groupGuid = group.guid;
		outDef.name = name;
		outDef.collectionType = API_PropertySingleCollectionType;
		outDef.valueType = API_PropertyStringValueType;
		outDef.measureType = API_PropertyDefaultMeasureType;
		outDef.canValueBeEditable = true;
		outDef.definitionType = API_PropertyCustomDefinitionType;
		outDef.defaultValue.basicValue.variantStatus = API_VariantStatusNormal;
		outDef.defaultValue.basicValue.singleVariant.variant.type = API_PropertyStringValueType;
		// 设置 availability 为所有分类项，使属性对所有元素可用
		outDef.availability = allClassificationItems;
		err = ACAPI_Property_CreatePropertyDefinition(outDef);
		if (err != NoError) {
			ACAPI_WriteReport("FindOrCreateHBIMDefinition [%s]: CreatePropertyDefinition 失败: %s", true, name.ToCStr().Get(), GS::UniString::Printf("Error %d", err).ToCStr().Get());
		}
		return err;
	}
	
	// 确保HBIM属性组和定义存在
	static GSErrCode EnsureHBIMPropertyGroupAndDefinitions(API_Guid& outGroupGuid, API_Guid& outIdGuid, API_Guid& outDescGuid)
	{
		API_PropertyGroup group;
		GSErrCode err = FindOrCreateHBIMGroup(group);
		if (err != NoError) return err;
		
		// 收集所有分类项
		GS::Array<API_Guid> allClassificationItems;
		GetAllClassificationItems(allClassificationItems);
		
		API_PropertyDefinition defId, defDesc;
		err = FindOrCreateHBIMDefinition(group, kHBIMIdName, defId, allClassificationItems);
		if (err != NoError) return err;
		
		err = FindOrCreateHBIMDefinition(group, kHBIMDescName, defDesc, allClassificationItems);
		if (err != NoError) return err;
		
		// 所有操作成功，设置输出参数
		outGroupGuid = group.guid;
		outIdGuid = defId.guid;
		outDescGuid = defDesc.guid;
		return NoError;
	}
	
 	// 查找现有的HBIM属性组和定义（不创建）
 	static GSErrCode FindExistingHBIMPropertyGroupAndDefinitions(API_Guid& outGroupGuid, API_Guid& outIdGuid, API_Guid& outDescGuid)
 	{
 		GS::Array<API_PropertyGroup> groups;
 		GSErrCode err = ACAPI_Property_GetPropertyGroups(groups);
 		if (err != NoError) {
 			ACAPI_WriteReport("FindExistingHBIMPropertyGroupAndDefinitions: GetPropertyGroups 失败: %s", true, GS::UniString::Printf("Error %d", err).ToCStr().Get());
 			return err;
 		}
 		
 		// 查找属性组
 		API_PropertyGroup foundGroup;
 		bool groupFound = false;
 		for (UInt32 i = 0; i < groups.GetSize(); ++i) {
 			if (groups[i].name == kHBIMGroupName) {
 				foundGroup = groups[i];
 				groupFound = true;
 				break;
 			}
 		}
 		
 		if (!groupFound) {
 			outGroupGuid = APINULLGuid;
 			outIdGuid = APINULLGuid;
 			outDescGuid = APINULLGuid;
 			return APIERR_BADNAME;
 		}
 		
 		outGroupGuid = foundGroup.guid;
 		
 		// 查找属性定义
 		GS::Array<API_PropertyDefinition> defs;
 		err = ACAPI_Property_GetPropertyDefinitions(foundGroup.guid, defs);
 		if (err != NoError) {
 			ACAPI_WriteReport("FindExistingHBIMPropertyGroupAndDefinitions: GetPropertyDefinitions 失败: %s", true, GS::UniString::Printf("Error %d", err).ToCStr().Get());
 			outGroupGuid = APINULLGuid;
 			outIdGuid = APINULLGuid;
 			outDescGuid = APINULLGuid;
 			return err;
 		}
 		
 		bool idFound = false, descFound = false;
 		API_PropertyDefinition defId, defDesc;
 		for (UInt32 i = 0; i < defs.GetSize(); ++i) {
 			if (defs[i].name == kHBIMIdName) {
 				defId = defs[i];
 				idFound = true;
 			} else if (defs[i].name == kHBIMDescName) {
 				defDesc = defs[i];
 				descFound = true;
 			}
 		}
 		
 		if (!idFound || !descFound) {
 			outGroupGuid = APINULLGuid;
 			outIdGuid = APINULLGuid;
 			outDescGuid = APINULLGuid;
 			return APIERR_BADNAME;
 		}
 		
 		outIdGuid = defId.guid;
 		outDescGuid = defDesc.guid;
 		return NoError;
 	}
 	
 	// 从元素读取HBIM属性值
 	static GSErrCode GetHBIMPropertyValue(const API_Guid& elemGuid, const API_Guid& defGuid, GS::UniString& outVal)
	{
		API_Property p = {};
		GSErrCode err = ACAPI_Element_GetPropertyValue(elemGuid, defGuid, p);
		if (err != NoError || p.status != API_Property_HasValue || p.value.variantStatus != API_VariantStatusNormal) return err;
		outVal = p.value.singleVariant.variant.uniStringValue;
		return NoError;
	}
	
	// 向元素写入HBIM属性值
	static GSErrCode SetHBIMPropertyValue(const API_Guid& elemGuid, const API_Guid& defGuid, const GS::UniString& value)
	{
		API_PropertyDefinition def = {};
		def.guid = defGuid;
		GSErrCode err = ACAPI_Property_GetPropertyDefinition(def);
		if (err != NoError) return err;
		
		API_Property p = {};
		p.definition = def;
		p.status = API_Property_HasValue;
		p.isDefault = false;
		p.value.variantStatus = API_VariantStatusNormal;
		p.value.singleVariant.variant.type = API_PropertyStringValueType;
		p.value.singleVariant.variant.uniStringValue = value;
		err = ACAPI_Element_SetProperty(elemGuid, p);
		return err;
	}
	
	// 检查元素是否有HBIM属性
	static bool HasHBIMProperties(const API_Guid& elemGuid, API_Guid idGuid, API_Guid descGuid)
	{
		// 如果GUID无效，说明属性定义不存在，元素不可能有HBIM属性
		if (idGuid == APINULLGuid || descGuid == APINULLGuid) {
			return false;
		}
		GS::UniString idVal, descVal;
		bool hasId = (GetHBIMPropertyValue(elemGuid, idGuid, idVal) == NoError);
		bool hasDesc = (GetHBIMPropertyValue(elemGuid, descGuid, descVal) == NoError);
		return hasId || hasDesc; // 只要有一个属性有值就认为有HBIM属性
	}
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
	, titleLabel (GetReference (), TitleLabelId)
	, underlineLabel (GetReference (), UnderlineId)
	, typeLabel (GetReference (), TypeLabelId)
	, typeValue (GetReference (), TypeValueId)
	, idLabel (GetReference (), IdLabelId)
	, idValue (GetReference (), IdValueId)
	, hbimSeparator (GetReference (), HBIMSeparatorId)
	, hbimTitle (GetReference (), HBIMTitleId)
	, hbimTitleUnderline (GetReference (), HBIMTitleUnderlineId)
	, hbimIdLabel (GetReference (), HBIMIdLabelId)
	, hbimIdValue (GetReference (), HBIMIdValueId)
	, hbimDescLabel (GetReference (), HBIMDescLabelId)
	, hbimDescValue (GetReference (), HBIMDescValueId)
	, hbimActionButton (GetReference (), HBIMActionButtonId)
	, hbimCancelButton (GetReference (), HBIMCancelButtonId)
	, hasHBIMProperties (false)
	, isHBIMEditMode (false)
	, hbimGroupGuid (APINULLGuid)
	, hbimIdGuid (APINULLGuid)
	, hbimDescGuid (APINULLGuid)
	, currentElemGuid (APINULLGuid)
	, isReSelectingElement (false)
{

	
	ACAPI_ProjectOperation_CatchProjectEvent (APINotify_Quit, NotificationHandler);
	Attach (*this);
	
	GS::UniString titleText;
	titleText = "HBIM构件信息录入 v";
	titleText += AC_ADDON_VERSION_STRING;
	
	titleLabel.SetText(titleText);
	titleLabel.Show();
	titleLabel.Redraw();
	
	// 设置下划线：使用一行连字符
	GS::UniString underlineText;
	for (int i = 0; i < 60; i++) {
		underlineText += "-";
	}
	underlineLabel.SetText(underlineText);
	underlineLabel.Show();
	underlineLabel.Redraw();
	
	// 初始化HBIM属性UI
	hbimSeparator.Show();
	hbimTitle.SetText("HBIM属性信息");
	hbimTitle.Show();
	hbimTitleUnderline.Show();
	hbimTitleUnderline.Redraw();
	hbimIdLabel.SetText("HBIM构件编号:");
	hbimIdLabel.Show();
	hbimDescLabel.SetText("HBIM构件说明:");
	hbimDescLabel.Show();
	
	// 初始状态：隐藏编辑控件，显示默认文本
	hbimIdValue.Hide();
	hbimDescValue.Hide();
	hbimActionButton.Hide();
	hbimCancelButton.Hide();
	
	// 注意：HBIM属性组和定义的创建延迟到第一次需要时
	// 这避免了在插件初始化时API可能未完全准备好的问题
	hbimGroupGuid = APINULLGuid;
	hbimIdGuid = APINULLGuid;
	hbimDescGuid = APINULLGuid;
	
	// 设置标签文本
	typeLabel.SetText("构件类型:");
	typeLabel.Show();
	typeLabel.Redraw();
	
	typeValue.SetText("未选择构件");
	typeValue.Show();
	typeValue.Redraw();
	
	idLabel.SetText("构件编号:");
	idLabel.Show();
	idLabel.Redraw();
	
	idValue.SetText("未选择构件");
	idValue.Show();
	idValue.Redraw();
	
	// 附加按钮观察者
	hbimActionButton.Attach(*this);
	hbimCancelButton.Attach(*this);
	
	hbimActionButton.Enable();
	hbimCancelButton.Enable();
	
	// 调试日志
	ACAPI_WriteReport("HBIMComponentEntry: 插件面板已创建，按钮观察者已附加", false);
	
	BeginEventProcessing ();
	
	UpdateFromSelection();
}

PluginPalette::~PluginPalette ()
{
	EndEventProcessing ();
}

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
	// 获取当前选中的元素
	API_SelectionInfo selInfo = {};
	GS::Array<API_Neig> selNeigs;
	GSErrCode err = ACAPI_Selection_Get (&selInfo, &selNeigs, false);
	if (err != NoError || selNeigs.IsEmpty()) {
		// 没有选中元素，重置显示
		typeValue.SetText("未选择构件");
		idValue.SetText("未选择构件");
		typeValue.Redraw();
		idValue.Redraw();
		
		// 重置HBIM属性显示
		hasHBIMProperties = false;
		isHBIMEditMode = false;
		UpdateHBIMUI();
		return;
	}
	
	// 使用第一个选中的元素
	API_Guid elemGuid = selNeigs[0].guid;
	
	// 更新IFC属性显示
	GS::UniString ifcType = GetIFCTypeForElement(elemGuid);
	GS::UniString globalId = GetGlobalIdForElement(elemGuid);
	typeValue.SetText(ifcType);
	idValue.SetText(globalId);
	typeValue.Redraw();
	idValue.Redraw();
	
	// 检查并更新HBIM属性
	CheckHBIMProperties(elemGuid);
}

void PluginPalette::UpdateHBIMUI ()
{
	ACAPI_WriteReport("HBIMComponentEntry: UpdateHBIMUI 开始，isHBIMEditMode=%d, hasHBIMProperties=%d", false, isHBIMEditMode ? 1 : 0, hasHBIMProperties ? 1 : 0);
	
	if (isHBIMEditMode) {
		ACAPI_WriteReport("HBIMComponentEntry: 编辑模式 - 显示编辑控件", false);
		// 编辑模式：显示编辑控件和按钮
		hbimIdValue.Show();
		hbimDescValue.Show();
		hbimActionButton.SetText("保存");
		hbimActionButton.Show();
		hbimCancelButton.Show();
		
		// 启用编辑控件
		hbimIdValue.Enable();
		hbimDescValue.Enable();
	} else {
		ACAPI_WriteReport("HBIMComponentEntry: 查看模式", false);
		// 查看模式
		if (hasHBIMProperties) {
			ACAPI_WriteReport("HBIMComponentEntry: 有属性 - 显示属性值（只读）和编辑按钮", false);
			// 有属性：显示属性值（只读），按钮为"属性编辑"
			hbimIdValue.Show();
			hbimDescValue.Show();
			hbimActionButton.SetText("属性编辑");
			hbimActionButton.Show();
			hbimCancelButton.Hide();
			
			// 禁用编辑控件（只读模式）
			hbimIdValue.Disable();
			hbimDescValue.Disable();
		} else {
			ACAPI_WriteReport("HBIMComponentEntry: 无属性 - 显示添加按钮", false);
			// 无属性：显示"添加"按钮
			hbimIdValue.Hide();
			hbimDescValue.Hide();
			hbimActionButton.SetText("添加");
			hbimActionButton.Show();
			hbimCancelButton.Hide();
		}
	}
	
	// 重绘控件
	hbimIdValue.Redraw();
	hbimDescValue.Redraw();
	hbimActionButton.Redraw();
	hbimCancelButton.Redraw();
	
	ACAPI_WriteReport("HBIMComponentEntry: UpdateHBIMUI 完成", false);
}

void PluginPalette::EnterHBIMEditMode ()
{
	ACAPI_WriteReport("HBIMComponentEntry: EnterHBIMEditMode 开始", false);
	
	if (isHBIMEditMode) {
		ACAPI_WriteReport("HBIMComponentEntry: 已经在编辑模式，跳过", false);
		return;
	}
	
	// 保存原始值
	if (hasHBIMProperties) {
		originalHBIMId = hbimIdValue.GetText();
		originalHBIMDesc = hbimDescValue.GetText();
		ACAPI_WriteReport("HBIMComponentEntry: 保存原始值，ID长度=%d，描述长度=%d", false, originalHBIMId.GetLength(), originalHBIMDesc.GetLength());
	} else {
		originalHBIMId.Clear();
		originalHBIMDesc.Clear();
		hbimIdValue.SetText("");
		hbimDescValue.SetText("");
		ACAPI_WriteReport("HBIMComponentEntry: 没有现有属性，清空字段", false);
	}
	
	isHBIMEditMode = true;
	ACAPI_WriteReport("HBIMComponentEntry: 设置 isHBIMEditMode=true，调用 UpdateHBIMUI", false);
	UpdateHBIMUI();
	
	ACAPI_WriteReport("HBIMComponentEntry: EnterHBIMEditMode 完成", false);
}

void PluginPalette::ExitHBIMEditMode (bool save)
{
	if (!isHBIMEditMode) return;
	
	if (save) {
		if (currentElemGuid != APINULLGuid) {
			WriteHBIMProperties(currentElemGuid);
			hasHBIMProperties = true;
		}
	} else {
		// 恢复原始值
		if (hasHBIMProperties) {
			hbimIdValue.SetText(originalHBIMId);
			hbimDescValue.SetText(originalHBIMDesc);
		} else {
			hbimIdValue.SetText("");
			hbimDescValue.SetText("");
		}
	}
	
	isHBIMEditMode = false;
	UpdateHBIMUI();
}

bool PluginPalette::TryFindExistingHBIMPropertyGroupAndDefinitions ()
{
	API_Guid groupGuid, idGuid, descGuid;
	GSErrCode err = FindExistingHBIMPropertyGroupAndDefinitions(groupGuid, idGuid, descGuid);
	if (err == NoError) {
		hbimGroupGuid = groupGuid;
		hbimIdGuid = idGuid;
		hbimDescGuid = descGuid;
		return true;
	}
	// 如果未找到，保持GUID为APINULLGuid
	return false;
}

GSErrCode PluginPalette::EnsureHBIMPropertiesInitialized ()
{
	// 如果GUID已经有效，说明已初始化
	if (hbimGroupGuid != APINULLGuid && hbimIdGuid != APINULLGuid && hbimDescGuid != APINULLGuid) {
		return NoError;
	}
	
	// 首先尝试查找现有的属性定义（可能已被其他实例创建）
	if (TryFindExistingHBIMPropertyGroupAndDefinitions()) {
		return NoError;
	}
	
	// 未找到现有定义，创建新的属性组和定义
	API_Guid groupGuid, idGuid, descGuid;
	GSErrCode err = ACAPI_CallUndoableCommand("创建HBIM属性定义",
		[&]() -> GSErrCode {
			return EnsureHBIMPropertyGroupAndDefinitions(groupGuid, idGuid, descGuid);
		}
	);
	if (err == NoError) {
		// 所有操作成功，更新成员变量
		hbimGroupGuid = groupGuid;
		hbimIdGuid = idGuid;
		hbimDescGuid = descGuid;
		ACAPI_WriteReport("EnsureHBIMPropertiesInitialized: 成功创建HBIM属性组和定义", false);
	} else {
		ACAPI_WriteReport("EnsureHBIMPropertiesInitialized: 初始化失败: %s", true, GS::UniString::Printf("Error %d", err).ToCStr().Get());
	}
	return err;
}

void PluginPalette::CheckHBIMProperties (const API_Guid& elementGuid)
{
	// 尝试查找现有的HBIM属性定义（不创建）
	if (hbimGroupGuid == APINULLGuid || hbimIdGuid == APINULLGuid || hbimDescGuid == APINULLGuid) {
		TryFindExistingHBIMPropertyGroupAndDefinitions();
	}
	
	// 如果GUID仍然无效，说明属性定义不存在，元素不可能有HBIM属性
	if (hbimGroupGuid == APINULLGuid || hbimIdGuid == APINULLGuid || hbimDescGuid == APINULLGuid) {
		hasHBIMProperties = false;
		UpdateHBIMUI();
		return;
	}
	
	hasHBIMProperties = HasHBIMProperties(elementGuid, hbimIdGuid, hbimDescGuid);
	if (hasHBIMProperties) {
		ReadHBIMProperties(elementGuid);
	}
	UpdateHBIMUI();
}

void PluginPalette::ReadHBIMProperties (const API_Guid& elementGuid)
{
	GS::UniString idVal, descVal;
	if (GetHBIMPropertyValue(elementGuid, hbimIdGuid, idVal) == NoError) {
		hbimIdValue.SetText(idVal);
	} else {
		hbimIdValue.SetText("");
	}
	
	if (GetHBIMPropertyValue(elementGuid, hbimDescGuid, descVal) == NoError) {
		hbimDescValue.SetText(descVal);
	} else {
		hbimDescValue.SetText("");
	}
}

void PluginPalette::WriteHBIMProperties (const API_Guid& elementGuid)
{
	// 确保属性系统已初始化
	GSErrCode initErr = EnsureHBIMPropertiesInitialized();
	if (initErr != NoError) {
		ACAPI_WriteReport("写入HBIM属性失败: 属性系统未初始化", true);
		// 显示用户友好的错误信息
		DG::InformationAlert("保存失败", "无法创建HBIM属性定义。错误代码: " + GS::UniString::Printf("%d", initErr), "确定");
		return;
	}
	
	GS::UniString idVal = hbimIdValue.GetText();
	GS::UniString descVal = hbimDescValue.GetText();
	
	// 在可撤销命令中写入属性值（遵循ComponentInfo模式）
	GSErrCode saveErr = ACAPI_CallUndoableCommand("保存HBIM属性信息",
		[&]() -> GSErrCode {
			GSErrCode err1 = SetHBIMPropertyValue(elementGuid, hbimIdGuid, idVal);
			if (err1 != NoError) {
				ACAPI_WriteReport("写入HBIM构件编号失败: %s", true, GS::UniString::Printf("Error %d", err1).ToCStr().Get());
				return err1;
			}
			
			GSErrCode err2 = SetHBIMPropertyValue(elementGuid, hbimDescGuid, descVal);
			if (err2 != NoError) {
				ACAPI_WriteReport("写入HBIM构件说明失败: %s", true, GS::UniString::Printf("Error %d", err2).ToCStr().Get());
				return err2;
			}
			
			return NoError;
		}
	);
	
	if (saveErr != NoError) {
		ACAPI_WriteReport("写入HBIM属性失败: %s", true, GS::UniString::Printf("Error %d", saveErr).ToCStr().Get());
		DG::InformationAlert("保存失败", "无法保存HBIM属性值。错误代码: " + GS::UniString::Printf("%d", saveErr), "确定");
	} else {
		ACAPI_WriteReport("写入HBIM属性成功", false);
	}
}

bool PluginPalette::IsInEditMode ()
{
	if (!HasInstance()) return false;
	return GetInstance().isHBIMEditMode;
}

void PluginPalette::SetEditMode (bool editMode)
{
	if (!HasInstance()) return;
	PluginPalette& instance = GetInstance();
	if (!editMode && instance.isHBIMEditMode) {
		instance.ExitHBIMEditMode(false); // 取消编辑
	}
}

void PluginPalette::SetMenuItemCheckedState (bool checked)
{
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
	}
}

GSErrCode PluginPalette::PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param)
{
	if (!HasInstance ())
		return NoError;
	
	PluginPalette& instance = GetInstance ();
	(void)instance; // Mark as intentionally unused for now
	
	switch (messageID) {
		case APIPalMsg_OpenPalette:
			break;
			
		case APIPalMsg_ClosePalette:
			break;
			
		case APIPalMsg_HidePalette_Begin:
			break;
			
		case APIPalMsg_HidePalette_End:
			break;
			
		case APIPalMsg_DisableItems_Begin:
			break;
			
		case APIPalMsg_DisableItems_End:
			break;
			
		default:
			break;
	}
	
	return NoError;
}

GSErrCode PluginPalette::SelectionChangeHandler (const API_Neig* selElemNeig)
{
	if (HasInstance()) {
		PluginPalette& instance = GetInstance();
		
		if (instance.isHBIMEditMode && selElemNeig != nullptr) {
			if (selElemNeig->guid != instance.currentElemGuid) {
				instance.ExitHBIMEditMode(true);
				DG::InformationAlert("提示", "你已经切换构件，对当前构件的编辑已保存并退出。", "确定");
			}
		}
	}
	
	if (selElemNeig == nullptr) {
		// 没有选中元素，显示默认文本
		if (HasInstance()) {
			PluginPalette& instance = GetInstance();
			instance.typeValue.SetText("未选择构件");
			instance.idValue.SetText("未选择构件");
			instance.typeValue.Redraw();
			instance.idValue.Redraw();
			
			instance.currentElemGuid = APINULLGuid;
			
			// 重置HBIM属性
			instance.hasHBIMProperties = false;
			instance.isHBIMEditMode = false;
			instance.UpdateHBIMUI();
		}
		return NoError;
	}
	
	// 更新选中元素的显示
	if (HasInstance()) {
		PluginPalette& instance = GetInstance();
		
		// 更新当前选中的构件GUID
		instance.currentElemGuid = selElemNeig->guid;
		
		GS::UniString ifcType = GetIFCTypeForElement(selElemNeig->guid);
		GS::UniString globalId = GetGlobalIdForElement(selElemNeig->guid);
		
		instance.typeValue.SetText(ifcType);
		instance.idValue.SetText(globalId);
		instance.typeValue.Redraw();
		instance.idValue.Redraw();
		
		// 检查HBIM属性
		instance.CheckHBIMProperties(selElemNeig->guid);
	}
	
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

void PluginPalette::PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted)
{
	if (isHBIMEditMode) {
		ExitHBIMEditMode(false); // 取消编辑
	}
	*accepted = true;
}

void PluginPalette::ButtonClicked (const DG::ButtonClickEvent& ev)
{
	ACAPI_WriteReport("HBIMComponentEntry: ButtonClicked 被调用", false);
	
	if (ev.GetSource() == &hbimActionButton) {
		ACAPI_WriteReport("HBIMComponentEntry: hbimActionButton 被点击", false);
		if (isHBIMEditMode) {
			ACAPI_WriteReport("HBIMComponentEntry: 保存按钮点击", false);
			ExitHBIMEditMode(true);
		} else {
			if (currentElemGuid == APINULLGuid) {
				DG::InformationAlert("提示", "请先选择一个构件", "确定");
				return;
			}
			ACAPI_WriteReport("HBIMComponentEntry: 添加/编辑按钮点击，当前 isHBIMEditMode=%d, hasHBIMProperties=%d", false, isHBIMEditMode ? 1 : 0, hasHBIMProperties ? 1 : 0);
			EnterHBIMEditMode();
		}
	} else if (ev.GetSource() == &hbimCancelButton) {
		ACAPI_WriteReport("HBIMComponentEntry: hbimCancelButton 被点击", false);
		ExitHBIMEditMode(false);
	}
}