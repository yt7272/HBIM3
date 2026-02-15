#include "ComponentInfo.hpp"
// #include "ComponentInfoDialog.hpp"  // Not needed for NewPlugin
#include "ACAPinc.h"
#include "APICommon.h"
#include "APIdefs_Properties.h"
#include "ApplicationLauncher.hpp"
#include "DGModule.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "Folder.hpp"
#include "BM.hpp"
#include "APIEnvir.h"  // For GSErrCode, NoError
#include "GSSystem.h"  // For GSSysBeep
#include "NewPluginDebug.hpp"

// Helper functions from APICommon.h that need implementations
void WriteReport_Err (const char* info, GSErrCode err)
{
	ACAPI_WriteReport ("%s: %s", true, info, ErrID_To_Name (err));
}

void WriteReport_End (GSErrCode err)
{
	if (err == NoError)
		ACAPI_WriteReport ("OK", false);
	else
		ACAPI_WriteReport ("Error: %s", false, ErrID_To_Name (err));
}

void ErrorBeep (const char* info, GSErrCode err)
{
	WriteReport_Err (info, err);
	GSSysBeep ();
}

const char* ErrID_To_Name (GSErrCode err)
{
	switch (err) {
		case APIERR_GENERAL:			return "APIERR_GENERAL";
		case APIERR_MEMFULL:			return "APIERR_MEMFULL";
		case APIERR_CANCEL:				return "APIERR_CANCEL";
		case APIERR_BADID:				return "APIERR_BADID";
		case APIERR_BADINDEX:			return "APIERR_BADINDEX";
		case APIERR_BADPARS:			return "APIERR_BADPARS";
		case APIERR_NOPLAN:				return "APIERR_NOPLAN";
		case APIERR_NOSEL:				return "APIERR_NOSEL";
		case APIERR_BADPROPERTY:		return "APIERR_BADPROPERTY";
		case APIERR_NAMEALREADYUSED:	return "APIERR_NAMEALREADYUSED";
		default:						return "Unknown Error";
	}
}
// IFC API 头文件
#include "ACAPI/IFCObjectAccessor.hpp"
#include "ACAPI/IFCPropertyAccessor.hpp"
#include "ACAPI/IFCProperty.hpp"
#include "ACAPI/IFCValue.hpp"
#include <variant>
#include <optional>
#include <ctime>

static const GS::UniString kGroupName        = "构件扩展信息";
static const GS::UniString kCompIdName       = "构件编号";
static const GS::UniString kPhotoDescName    = "照片说明";
static const GS::UniString kPhotoPathsName   = "照片路径";
static const GS::UniString kPhotoFolderSuffix = "_构件照片";

// IFC 属性值转换为字符串的辅助函数
static GS::UniString GetIFCPropertyValueString (const IFCAPI::Property& property)
{
	auto propertyVariant = property.GetTyped ();
	
	if (std::holds_alternative<IFCAPI::PropertySingleValue> (propertyVariant)) {
		const auto& propertyCast = std::get<IFCAPI::PropertySingleValue> (propertyVariant);
		auto value = propertyCast.GetNominalValue ();
		auto anyValue = value.GetAnyValue ();
		if (anyValue.has_value ()) {
			if (std::holds_alternative<std::int64_t> (*anyValue)) {
				return GS::ValueToUniString (std::get<std::int64_t> (*anyValue));
			} else if (std::holds_alternative<double> (*anyValue)) {
				return GS::ValueToUniString (std::get<double> (*anyValue));
			} else if (std::holds_alternative<bool> (*anyValue)) {
				return std::get<bool> (*anyValue) ? "是" : "否";
			} else if (std::holds_alternative<GS::UniString> (*anyValue)) {
				return std::get<GS::UniString> (*anyValue);
			}
		}
		return "[值]";
	} else if (std::holds_alternative<IFCAPI::PropertyListValue> (propertyVariant)) {
		return "[列表值]";
	} else if (std::holds_alternative<IFCAPI::PropertyBoundedValue> (propertyVariant)) {
		return "[范围值]";
	} else if (std::holds_alternative<IFCAPI::PropertyEnumeratedValue> (propertyVariant)) {
		return "[枚举值]";
	} else {
		return "[表值]";
	}
}

// JSON字符串转义辅助函数
static GS::UniString EscapeJsonString (const GS::UniString& input)
{
	GS::UniString output;
	for (UInt32 i = 0; i < input.GetLength (); ++i) {
		GS::uchar_t ch = input.GetChar (i);
		switch (ch) {
			case '\\': output += "\\\\"; break;
			case '\"': output += "\\\""; break;
			case '\b': output += "\\b"; break;
			case '\f': output += "\\f"; break;
			case '\n': output += "\\n"; break;
			case '\r': output += "\\r"; break;
			case '\t': output += "\\t"; break;
			default:
				if (ch < 0x20) {
					// 控制字符使用Unicode转义
					output += GS::UniString::SPrintf ("\\u%04x", ch);
				} else {
					output.Append (ch);
				}
				break;
		}
	}
	return output;
}

// 递归收集一个分类项及其所有子项的 GUID
static void CollectClassificationItemsRecursive (const API_Guid& itemGuid, GS::Array<API_Guid>& outGuids)
{
	outGuids.Push (itemGuid);
	GS::Array<API_ClassificationItem> children;
	if (ACAPI_Classification_GetClassificationItemChildren (itemGuid, children) == NoError) {
		for (UInt32 i = 0; i < children.GetSize (); ++i) {
			CollectClassificationItemsRecursive (children[i].guid, outGuids);
		}
	}
}

// 收集所有分类系统中的所有分类项
static GSErrCode GetAllClassificationItems (GS::Array<API_Guid>& outAllItems)
{
	outAllItems.Clear ();
	GS::Array<API_ClassificationSystem> systems;
	GSErrCode err = ACAPI_Classification_GetClassificationSystems (systems);
	if (err != NoError) return err;
	
	for (UInt32 i = 0; i < systems.GetSize (); ++i) {
		GS::Array<API_ClassificationItem> rootItems;
		if (ACAPI_Classification_GetClassificationSystemRootItems (systems[i].guid, rootItems) == NoError) {
			for (UInt32 j = 0; j < rootItems.GetSize (); ++j) {
				CollectClassificationItemsRecursive (rootItems[j].guid, outAllItems);
			}
		}
	}
	return NoError;
}

static GSErrCode FindOrCreateGroup (API_PropertyGroup& outGroup)
{
	GS::Array<API_PropertyGroup> groups;
	GSErrCode err = ACAPI_Property_GetPropertyGroups (groups);
	if (err != NoError) {
		ACAPI_WriteReport ("FindOrCreateGroup: GetPropertyGroups 失败: %s", true, ErrID_To_Name (err));
		return err;
	}
	for (UInt32 i = 0; i < groups.GetSize (); ++i) {
		if (groups[i].name == kGroupName) {
			outGroup = groups[i];
			return NoError;
		}
	}
	outGroup = {};
	outGroup.guid = APINULLGuid;
	outGroup.name = kGroupName;
	outGroup.description = "构件编号、照片说明、照片路径";
	err = ACAPI_Property_CreatePropertyGroup (outGroup);
	if (err != NoError) {
		ACAPI_WriteReport ("FindOrCreateGroup: CreatePropertyGroup 失败: %s", true, ErrID_To_Name (err));
	}
	return err;
}

static GSErrCode FindOrCreateDefinition (const API_PropertyGroup& group, const GS::UniString& name, API_PropertyDefinition& outDef, API_VariantType valueType, API_PropertyCollectionType collType, const GS::Array<API_Guid>& allClassificationItems)
{
	GS::Array<API_PropertyDefinition> defs;
	GSErrCode err = ACAPI_Property_GetPropertyDefinitions (group.guid, defs);
	if (err != NoError) {
		ACAPI_WriteReport ("FindOrCreateDefinition [%s]: GetPropertyDefinitions 失败: %s", true, name.ToCStr ().Get (), ErrID_To_Name (err));
		return err;
	}
	for (UInt32 i = 0; i < defs.GetSize (); ++i) {
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
	outDef.collectionType = collType;
	outDef.valueType = valueType;
	outDef.measureType = API_PropertyDefaultMeasureType;
	outDef.canValueBeEditable = true;
	outDef.definitionType = API_PropertyCustomDefinitionType;
	outDef.defaultValue.basicValue.variantStatus = API_VariantStatusNormal;
	if (collType == API_PropertySingleCollectionType)
		outDef.defaultValue.basicValue.singleVariant.variant.type = valueType;
	// 设置 availability 为所有分类项，使属性对所有元素可用
	outDef.availability = allClassificationItems;
	err = ACAPI_Property_CreatePropertyDefinition (outDef);
	if (err != NoError) {
		ACAPI_WriteReport ("FindOrCreateDefinition [%s]: CreatePropertyDefinition 失败: %s", true, name.ToCStr ().Get (), ErrID_To_Name (err));
	}
	return err;
}

GSErrCode ComponentInfo::EnsurePropertyGroupAndDefinitions (PropertyDefinitionGuids& outGuids)
{
	API_PropertyGroup group;
	GSErrCode err = FindOrCreateGroup (group);
	if (err != NoError) return err;
	outGuids.groupGuid = group.guid;

	// 收集所有分类项，使属性对所有元素可用
	GS::Array<API_Guid> allClassificationItems;
	GetAllClassificationItems (allClassificationItems);
	ACAPI_WriteReport ("EnsurePropertyGroupAndDefinitions: 找到 %d 个分类项", false, (int)allClassificationItems.GetSize ());

	API_PropertyDefinition defCompId, defDesc, defPaths;
	err = FindOrCreateDefinition (group, kCompIdName, defCompId, API_PropertyStringValueType, API_PropertySingleCollectionType, allClassificationItems);
	if (err != NoError) return err;
	outGuids.compIdGuid = defCompId.guid;

	err = FindOrCreateDefinition (group, kPhotoDescName, defDesc, API_PropertyStringValueType, API_PropertySingleCollectionType, allClassificationItems);
	if (err != NoError) return err;
	outGuids.photoDescGuid = defDesc.guid;

	err = FindOrCreateDefinition (group, kPhotoPathsName, defPaths, API_PropertyStringValueType, API_PropertyListCollectionType, allClassificationItems);
	if (err != NoError) return err;
	outGuids.photoPathsGuid = defPaths.guid;

	return NoError;
}

GSErrCode ComponentInfo::DeleteAndRecreatePropertyGroup (PropertyDefinitionGuids& outGuids)
{
	ACAPI_WriteReport ("DeleteAndRecreatePropertyGroup: 开始删除属性组...", false);
	
	// 查找现有属性组
	GS::Array<API_PropertyGroup> groups;
	GSErrCode err = ACAPI_Property_GetPropertyGroups (groups);
	if (err != NoError) return err;

	for (UInt32 i = 0; i < groups.GetSize (); ++i) {
		if (groups[i].name == kGroupName) {
			ACAPI_WriteReport ("DeleteAndRecreatePropertyGroup: 找到属性组，正在删除...", false);
			err = ACAPI_Property_DeletePropertyGroup (groups[i].guid);
			if (err != NoError) {
				ACAPI_WriteReport ("DeleteAndRecreatePropertyGroup: 删除失败: %s", true, ErrID_To_Name (err));
				return err;
			}
			ACAPI_WriteReport ("DeleteAndRecreatePropertyGroup: 删除成功", false);
			break;
		}
	}

	// 重新创建属性组和定义
	ACAPI_WriteReport ("DeleteAndRecreatePropertyGroup: 重新创建...", false);
	return EnsurePropertyGroupAndDefinitions (outGuids);
}

GSErrCode ComponentInfo::FindPropertyDefinitions (PropertyDefinitionGuids& outGuids)
{
	outGuids = {};

	// 查找属性组
	GS::Array<API_PropertyGroup> groups;
	GSErrCode err = ACAPI_Property_GetPropertyGroups (groups);
	if (err != NoError) return err;

	API_PropertyGroup* foundGroup = nullptr;
	for (UInt32 i = 0; i < groups.GetSize (); ++i) {
		if (groups[i].name == kGroupName) {
			foundGroup = &groups[i];
			break;
		}
	}
	if (foundGroup == nullptr) return APIERR_BADID;
	outGuids.groupGuid = foundGroup->guid;

	// 查找属性定义
	GS::Array<API_PropertyDefinition> defs;
	err = ACAPI_Property_GetPropertyDefinitions (foundGroup->guid, defs);
	if (err != NoError) return err;

	for (UInt32 i = 0; i < defs.GetSize (); ++i) {
		if (defs[i].name == kCompIdName)
			outGuids.compIdGuid = defs[i].guid;
		else if (defs[i].name == kPhotoDescName)
			outGuids.photoDescGuid = defs[i].guid;
		else if (defs[i].name == kPhotoPathsName)
			outGuids.photoPathsGuid = defs[i].guid;
	}

	// 检查是否所有定义都找到了
	if (outGuids.compIdGuid == APINULLGuid || outGuids.photoDescGuid == APINULLGuid || outGuids.photoPathsGuid == APINULLGuid)
		return APIERR_BADID;

	return NoError;
}

GSErrCode ComponentInfo::GetProjectPhotoFolder (IO::Location& outPhotoFolderLoc, IO::Location& outProjectFolderLoc, GS::UniString& outProjectName)
{
	API_ProjectInfo pi;
	GSErrCode err = ACAPI_ProjectOperation_Project (&pi);
	if (err != NoError) {
		ACAPI_WriteReport ("GetProjectPhotoFolder: ACAPI_ProjectOperation_Project失败: %s", true, ErrID_To_Name(err));
		return err;
	}
	
	if (pi.untitled) {
		ACAPI_WriteReport ("项目未保存，无法添加构件照片。请先保存项目。", true);
		return APIERR_NOPLAN;
	}
	
	IO::Location* fileLoc = pi.teamwork ? pi.location_team : pi.location;
	if (!fileLoc || fileLoc->IsEmpty ()) {
		ACAPI_WriteReport ("GetProjectPhotoFolder: 项目文件位置无效", true);
		return APIERR_NOPLAN;
	}

	outProjectFolderLoc = *fileLoc;
	if (err != NoError) { ACAPI_WriteReport ("GetProjectPhotoFolder: DeleteLastLocalName失败: %s", true, ErrID_To_Name(err)); return err; }
	err = outProjectFolderLoc.DeleteLastLocalName ();
	if (err != NoError) return err;

	outProjectName = (pi.projectName && !pi.projectName->IsEmpty ()) ? *pi.projectName : "Project";
	GS::UniString folderName = outProjectName + kPhotoFolderSuffix;
	outPhotoFolderLoc = outProjectFolderLoc;
	if (err != NoError) { ACAPI_WriteReport ("GetProjectPhotoFolder: AppendToLocal失败: %s", true, ErrID_To_Name(err)); return err; }
	err = outPhotoFolderLoc.AppendToLocal (IO::Name (folderName));
	if (err != NoError) return err;

	err = IO::fileSystem.CreateFolder (outPhotoFolderLoc);
	if (err != NoError && err != (GSErrCode) IO::FileSystem::TargetExists) { ACAPI_WriteReport ("GetProjectPhotoFolder: CreateFolder失败: %s", true, ErrID_To_Name(err)); return err; }
	if (err != NoError && err != (GSErrCode) IO::FileSystem::TargetExists) return err;
	ACAPI_WriteReport ("GetProjectPhotoFolder: 成功，项目文件夹: %s，照片文件夹: %s", false, outProjectName.ToCStr().Get(), folderName.ToCStr().Get());
	return NoError;
}

GS::UniString ComponentInfo::GuidToFolderName (const API_Guid& guid)
{
	GS::Guid gsGuid = APIGuid2GSGuid (guid);
	return gsGuid.ToUniString ();
}

GSErrCode ComponentInfo::GetComponentPhotoFolder (const API_Guid& elemGuid, IO::Location& outComponentPhotoFolder, IO::Location& outProjectFolderLoc, GS::UniString& outProjectName)
{
	IO::Location photoRootLoc;
	GSErrCode err = GetProjectPhotoFolder (photoRootLoc, outProjectFolderLoc, outProjectName);
	if (err != NoError) return err;

	GS::UniString folderName = GlobalIdToFolderName (elemGuid);
	if (folderName.IsEmpty ()) return APIERR_BADPROPERTY;
	outComponentPhotoFolder = photoRootLoc;
	err = outComponentPhotoFolder.AppendToLocal (IO::Name (folderName));
	if (err != NoError) return err;

	err = IO::fileSystem.CreateFolder (outComponentPhotoFolder);
	if (err != NoError && err != (GSErrCode) IO::FileSystem::TargetExists) return err;
	return NoError;
}

GSErrCode ComponentInfo::CopyPhotosForComponent (const API_Guid& elemGuid, const GS::Array<IO::Location>& sourceLocs, GS::Array<GS::UniString>& outRelPaths, UInt32 startIndex)
{
	IO::Location componentPhotoFolder, projectFolderLoc;
	GS::UniString projectName;
	GSErrCode err = GetComponentPhotoFolder (elemGuid, componentPhotoFolder, projectFolderLoc, projectName);
	if (err != NoError) return err;

	GS::UniString folderName = GlobalIdToFolderName (elemGuid);
	GS::UniString prefix = projectName + kPhotoFolderSuffix + "/" + folderName + "/";
	outRelPaths.Clear ();

	for (UInt32 i = 0; i < sourceLocs.GetSize (); ++i) {
		IO::Name originalFileName;
		if (sourceLocs[i].GetLastLocalName (&originalFileName) != NoError) continue;

		// 按序号重命名照片: 001_originalname.jpg，从 startIndex 开始编号
		GS::UniString seqStr;
		seqStr.Printf ("%03u_", (unsigned)(startIndex + i + 1));
		GS::UniString newFileName = seqStr + originalFileName.ToString ();

		IO::Location dst = componentPhotoFolder;
		if (dst.AppendToLocal (IO::Name (newFileName)) != NoError) continue;

		err = IO::fileSystem.Copy (sourceLocs[i], dst);
		if (err != NoError && err != (GSErrCode) IO::FileSystem::TargetExists) {
			ACAPI_WriteReport ("Failed to copy photo: %s", true, sourceLocs[i].ToDisplayText ().ToCStr ().Get ());
			continue;
		}
		outRelPaths.Push (prefix + newFileName);
	}
	return NoError;
}

GSErrCode ComponentInfo::WritePropertiesToElement (API_Guid elemGuid, const PropertyDefinitionGuids& guids, const GS::UniString& compId, const GS::UniString& photoDesc, const GS::Array<GS::UniString>& relPaths)
{
	GSErrCode err;

	// ---- 构件编号 ----
	API_PropertyDefinition def = {};
	def.guid = guids.compIdGuid;
	err = ACAPI_Property_GetPropertyDefinition (def);
	if (err != NoError) return err;
	{
		API_Property p = {};
		p.definition = def;
		p.status = API_Property_HasValue;
		p.isDefault = false;
		p.value.variantStatus = API_VariantStatusNormal;
		p.value.singleVariant.variant.type = API_PropertyStringValueType;
		p.value.singleVariant.variant.uniStringValue = compId;
		err = ACAPI_Element_SetProperty (elemGuid, p);
		if (err != NoError) return err;
	}

	// ---- 照片说明 ----
	def = {};
	def.guid = guids.photoDescGuid;
	err = ACAPI_Property_GetPropertyDefinition (def);
	if (err != NoError) return err;
	{
		API_Property p = {};
		p.definition = def;
		p.status = API_Property_HasValue;
		p.isDefault = false;
		p.value.variantStatus = API_VariantStatusNormal;
		p.value.singleVariant.variant.type = API_PropertyStringValueType;
		p.value.singleVariant.variant.uniStringValue = photoDesc;
		err = ACAPI_Element_SetProperty (elemGuid, p);
		if (err != NoError) return err;
	}

	// ---- 照片路径 (List) ----
	def = {};
	def.guid = guids.photoPathsGuid;
	err = ACAPI_Property_GetPropertyDefinition (def);
	if (err != NoError) return err;
	{
		API_Property p = {};
		p.definition = def;
		p.status = API_Property_HasValue;
		p.isDefault = false;
		p.value.variantStatus = API_VariantStatusNormal;
		p.value.listVariant.variants.Clear ();
		for (UInt32 i = 0; i < relPaths.GetSize (); ++i) {
			API_Variant v = {};
			v.type = API_PropertyStringValueType;
			v.uniStringValue = relPaths[i];
			p.value.listVariant.variants.Push (v);
		}
		err = ACAPI_Element_SetProperty (elemGuid, p);
		if (err != NoError) return err;
	}

	return NoError;
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

GSErrCode ComponentInfo::GetStringPropertyFromElement (API_Guid elemGuid, API_Guid defGuid, GS::UniString& outVal)
{
	API_Property p = {};
	GSErrCode err = ACAPI_Element_GetPropertyValue (elemGuid, defGuid, p);
	if (err != NoError || p.status != API_Property_HasValue || p.value.variantStatus != API_VariantStatusNormal) return err;
	outVal = p.value.singleVariant.variant.uniStringValue;
	return NoError;
}

GSErrCode ComponentInfo::GetElementClassificationInfo (API_Guid elemGuid, GS::UniString& outClassification, GS::UniString& outId)
{
	outClassification.Clear ();
	outId.Clear ();
	
	// 获取元素的所有分类项
	GS::Array<GS::Pair<API_Guid, API_Guid>> systemItemPairs;
	GSErrCode err = ACAPI_Element_GetClassificationItems (elemGuid, systemItemPairs);
	if (err != NoError || systemItemPairs.IsEmpty ()) {
		return err != NoError ? err : APIERR_GENERAL;
	}
	
	// 使用第一个分类项
	API_Guid itemGuid = systemItemPairs[0].second;
	
	// 获取分类项详细信息
	API_ClassificationItem item = {};
	item.guid = itemGuid;
	err = ACAPI_Classification_GetClassificationItem (item);
	if (err != NoError) return err;
	
	// 获取分类系统信息
	API_ClassificationSystem system = {};
	err = ACAPI_Classification_GetClassificationItemSystem (itemGuid, system);
	if (err != NoError) return err;
	
	// 构造分类显示文本：系统名 版本 分类项名
	outClassification = system.name;
	if (!system.editionVersion.IsEmpty ()) {
		outClassification += " ";
		outClassification += system.editionVersion;
	}
	outClassification += " ";
	outClassification += item.name;
	
	// ID
	outId = item.id;
	
	return NoError;
}

GSErrCode ComponentInfo::GetElementAllProperties (API_Guid elemGuid, GS::UniString& outFormattedProperties)
{
	outFormattedProperties.Clear ();
	
	if (elemGuid == APINULLGuid) {
		return APIERR_BADID;
	}
	
	GS::UniString propertiesText;
	
	// 1. 获取元素头信息（类型、图层、楼层）
	API_Elem_Head elemHead = {};
	elemHead.guid = elemGuid;
	GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
	if (err != NoError) {
		ACAPI_WriteReport ("GetElementAllProperties: ACAPI_Element_GetHeader 失败: %s", true, ErrID_To_Name (err));
		return err;
	}
	
	// 元素类型 - 使用简单映射
	GS::HashTable<API_ElemTypeID, GS::UniString> typeNames = {
		{ API_WallID, "墙" },
		{ API_ColumnID, "柱" },
		{ API_BeamID, "梁" },
		{ API_WindowID, "窗" },
		{ API_DoorID, "门" },
		{ API_ObjectID, "物体" },
		{ API_SlabID, "楼板" },
		{ API_RoofID, "屋顶" },
		{ API_MeshID, "网格" },
		{ API_ZoneID, "区域" },
		{ API_TextID, "文字" },
		{ API_DimensionID, "尺寸" }
	};
	
	GS::UniString typeName = "未知类型";
	if (typeNames.ContainsKey (elemHead.type.typeID)) {
		typeName = typeNames.Get (elemHead.type.typeID);
	}
	propertiesText += "元素类型: " + typeName + "\n";
	
	// 图层信息
	API_Attribute layerAttr = {};
	layerAttr.header.typeID = API_LayerID;
	layerAttr.header.index = elemHead.layer;
	if (ACAPI_Attribute_Get (&layerAttr) == NoError) {
		propertiesText += "图层: " + GS::UniString(layerAttr.header.name) + "\n";
	} else {
		propertiesText += "图层: 未知\n";
	}
	
	// 楼层信息 - 简化，只显示楼层索引
	propertiesText += "楼层索引: " + GS::ValueToUniString (elemHead.floorInd) + "\n";
	
	// 2. 获取分类信息
	GS::UniString classificationInfo, classificationId;
	if (GetElementClassificationInfo (elemGuid, classificationInfo, classificationId) == NoError) {
		if (!classificationInfo.IsEmpty ()) {
			propertiesText += "分类: " + classificationInfo + "\n";
		}
		if (!classificationId.IsEmpty ()) {
			propertiesText += "分类ID: " + classificationId + "\n";
		}
	}
	
	// 3. 获取自定义属性（构件编号、照片说明）
	PropertyDefinitionGuids guids;
	if (FindPropertyDefinitions (guids) == NoError) {
		GS::UniString compId;
		if (GetStringPropertyFromElement (elemGuid, guids.compIdGuid, compId) == NoError && !compId.IsEmpty ()) {
			propertiesText += "构件编号: " + compId + "\n";
		}
		
		GS::UniString photoDesc;
		if (GetStringPropertyFromElement (elemGuid, guids.photoDescGuid, photoDesc) == NoError && !photoDesc.IsEmpty ()) {
			propertiesText += "照片说明: " + photoDesc + "\n";
		}
		
		// 照片数量
		GS::Array<GS::UniString> photoPaths;
		if (GetPhotoPathsFromElement (elemGuid, guids.photoPathsGuid, photoPaths) == NoError && !photoPaths.IsEmpty ()) {
			propertiesText += "照片数量: " + GS::ValueToUniString (photoPaths.GetSize ()) + "\n";
		}
	}
	
	// 4. 元素GUID和基本信息
	propertiesText += "元素GUID: " + APIGuidToString (elemGuid) + "\n";
	
	// 移除最后一个换行符
	if (!propertiesText.IsEmpty () && propertiesText.GetLast () == '\n') {
		propertiesText.Delete (propertiesText.GetLength () - 1, 1);
	}
	
	outFormattedProperties = propertiesText;
	return NoError;
}

GSErrCode ComponentInfo::GetElementIFCProperties (API_Guid elemGuid, GS::UniString& outFormattedProperties)
{
	outFormattedProperties.Clear ();
	
	if (elemGuid == APINULLGuid) {
		return APIERR_BADID;
	}
	
	// 1. 获取元素头信息
	API_Elem_Head elemHead = {};
	elemHead.guid = elemGuid;
	GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
	if (err != NoError) {
		ACAPI_WriteReport ("GetElementIFCProperties: ACAPI_Element_GetHeader 失败: %s", true, ErrID_To_Name (err));
		return err;
	}
	
	// 2. 创建IFC ObjectID
	auto objectIDResult = IFCAPI::GetObjectAccessor ().CreateElementObjectID (elemHead);
	if (objectIDResult.IsErr ()) {
		ACAPI::Error ifcErr = objectIDResult.UnwrapErr ();
		ACAPI_WriteReport ("GetElementIFCProperties: CreateElementObjectID 失败: %s", true, ErrID_To_Name (ifcErr.kind));
		return ifcErr.kind;
	}
	IFCAPI::ObjectID objectID = objectIDResult.Unwrap ();
	
	// 3. 创建PropertyAccessor并获取本地IFC属性
	IFCAPI::PropertyAccessor propertyAccessor (objectID);
	auto propertiesResult = propertyAccessor.GetLocalProperties ();
	if (propertiesResult.IsErr ()) {
		ACAPI::Error propErr = propertiesResult.UnwrapErr ();
		ACAPI_WriteReport ("GetElementIFCProperties: GetLocalProperties 失败: %s", true, ErrID_To_Name (propErr.kind));
		return propErr.kind;
	}
	
	std::vector<IFCAPI::Property> properties = propertiesResult.Unwrap ();
	
	// 4. 格式化属性文本
	GS::UniString propertiesText;
	propertiesText += "IFC 属性 (本地存储):\n";
	
	if (properties.empty ()) {
		propertiesText += "  无本地存储的IFC属性\n";
	} else {
		for (const auto& prop : properties) {
			GS::UniString psetName = prop.GetPropertySetName ();
			GS::UniString propName = prop.GetName ();
			
			propertiesText += "  [" + psetName + "] " + propName + ": ";
			propertiesText += GetIFCPropertyValueString (prop);
			
			propertiesText += "\n";
		}
	}
	
	// 5. 获取预览IFC属性（导出时计算的属性）
	auto previewResult = propertyAccessor.GetPreviewProperties ();
	if (previewResult.IsOk ()) {
		std::vector<IFCAPI::Property> previewProps = previewResult.Unwrap ();
		if (!previewProps.empty ()) {
			propertiesText += "\nIFC 属性 (预览导出，含计算值):\n";
			for (const auto& prop : previewProps) {
				GS::UniString psetName = prop.GetPropertySetName ();
				GS::UniString propName = prop.GetName ();
				propertiesText += "  [" + psetName + "] " + propName + "\n";
			}
		}
	}
	
	outFormattedProperties = propertiesText;
	return NoError;
}

#if 0
void ComponentInfo::RunComponentInfoCommand ()
{
	// 先获取选择
	API_SelectionInfo selectionInfo = {};
	GS::Array<API_Neig> selNeigs;
	GSErrCode err = ACAPI_Selection_Get (&selectionInfo, &selNeigs, true);
	BMKillHandle ((GSHandle*) &selectionInfo.marquee.coords);
	if (err == APIERR_NOSEL || selectionInfo.typeID == API_SelEmpty || selNeigs.IsEmpty ()) {
		ACAPI_WriteReport ("Please select at least one element.", true);
		return;
	}

	// 确保属性定义存在（不设置分类限制，对所有元素类型可用）
	PropertyDefinitionGuids guids;
	err = EnsurePropertyGroupAndDefinitions (guids);
	if (err != NoError) { ErrorBeep ("EnsurePropertyGroupAndDefinitions", err); return; }

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

	for (UInt32 i = 0; i < selNeigs.GetSize (); ++i) {
		API_Guid elemGuid = selNeigs[i].guid;

		// 1. 先获取该元素的现有照片路径
		GS::Array<GS::UniString> existingPaths;
		GetPhotoPathsFromElement (elemGuid, guids.photoPathsGuid, existingPaths);

		// 2. 如果有新照片，复制到该元素的文件夹（从现有数量开始编号）
		GS::Array<GS::UniString> newRelPaths;
		if (!photoLocs.IsEmpty ()) {
			UInt32 startIdx = (UInt32) existingPaths.GetSize ();
			err = CopyPhotosForComponent (elemGuid, photoLocs, newRelPaths, startIdx);
			if (err != NoError) {
				ACAPI_WriteReport ("复制照片失败: %s", true, ErrID_To_Name (err));
				continue;
			}
		}

		// 3. 合并路径：现有 + 新增
		GS::Array<GS::UniString> allPaths = existingPaths;
		for (UInt32 j = 0; j < newRelPaths.GetSize (); ++j) {
			allPaths.Push (newRelPaths[j]);
		}

		// 4. 写入属性
		err = WritePropertiesToElement (elemGuid, guids, compId, photoDesc, allPaths);
		if (err != NoError)
			ACAPI_WriteReport ("SetProperty failed for element: %s", true, ErrID_To_Name (err));
	}
	ACAPI_WriteReport ("Component Info: OK, %d elements updated.", false, (int) selNeigs.GetSize ());
}

#endif

void ComponentInfo::RunViewComponentPhotosCommand ()
{
	API_SelectionInfo selectionInfo = {};
	GS::Array<API_Neig> selNeigs;
	GSErrCode err = ACAPI_Selection_Get (&selectionInfo, &selNeigs, true);
	BMKillHandle ((GSHandle*) &selectionInfo.marquee.coords);
	if (err == APIERR_NOSEL || selectionInfo.typeID == API_SelEmpty || selNeigs.IsEmpty ()) {
		ACAPI_WriteReport ("请先选中要查看照片的构件。", true);
		return;
	}

	PropertyDefinitionGuids guids;
	err = EnsurePropertyGroupAndDefinitions (guids);
	if (err != NoError) { ErrorBeep ("EnsurePropertyGroupAndDefinitions", err); return; }

	IO::Location photoFolderLoc, projectFolderLoc;
	GS::UniString projectName;
	err = GetProjectPhotoFolder (photoFolderLoc, projectFolderLoc, projectName);
	if (err != NoError) {
		ACAPI_WriteReport ("请先保存项目后再查看构件照片。", true);
		return;
	}

	GS::UniString projectFolderPath;
	if (projectFolderLoc.ToPath (&projectFolderPath) != NoError) {
		ACAPI_WriteReport ("无法解析项目路径。", true);
		return;
	}

	GS::Array<GS::UniString> allPaths;
	for (UInt32 i = 0; i < selNeigs.GetSize (); ++i) {
		GS::Array<GS::UniString> relPaths;
		if (GetPhotoPathsFromElement (selNeigs[i].guid, guids.photoPathsGuid, relPaths) != NoError)
			continue;
		for (UInt32 j = 0; j < relPaths.GetSize (); ++j) {
			GS::UniString full = projectFolderPath;
			if (!full.IsEmpty () && full.GetLast () != '/' && full.GetLast () != '\\')
				full += "/";
			full += relPaths[j];
			allPaths.Push (full);
		}
	}

	if (allPaths.IsEmpty ()) {
		ACAPI_WriteReport ("所选构件没有照片。", true);
		return;
	}

	IO::Location openCmd ("/usr/bin/open");
	GS::Array<GS::UniString> argv;
	argv.Push ("--");
	for (UInt32 k = 0; k < allPaths.GetSize (); ++k)
		argv.Push (allPaths[k]);

	err = IO::Process::ApplicationLauncher::Instance ().Launch (openCmd, argv);
 	if (err != NoError)
 		ACAPI_WriteReport ("打开照片失败: %s", true, ErrID_To_Name (err));
 	else
 		ACAPI_WriteReport ("已打开 %d 张照片。", false, (int) allPaths.GetSize ());
 }
 
 GSErrCode ComponentInfo::GetComponentIdentifier (API_Guid elemGuid, GS::UniString& outIdentifier)
 {
 	outIdentifier.Clear ();
 	
 	if (elemGuid == APINULLGuid) {
 		return APIERR_BADID;
 	}
 	
 	GS::Array<GS::UniString> identifierParts;
 	
 	// 1. 获取元素类型
 	API_Elem_Head elemHead = {};
 	elemHead.guid = elemGuid;
 	GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
 	if (err == NoError) {
 		GS::HashTable<API_ElemTypeID, GS::UniString> typeNames = {
 			{ API_WallID, "墙" },
 			{ API_ColumnID, "柱" },
 			{ API_BeamID, "梁" },
 			{ API_WindowID, "窗" },
 			{ API_DoorID, "门" },
 			{ API_ObjectID, "物体" },
 			{ API_SlabID, "楼板" },
 			{ API_RoofID, "屋顶" },
 			{ API_MeshID, "网格" },
 			{ API_ZoneID, "区域" },
 			{ API_TextID, "文字" },
 			{ API_DimensionID, "尺寸" }
 		};
 		
		if (typeNames.ContainsKey (elemHead.type.typeID)) {
			GS::UniString typeName = typeNames.Get (elemHead.type.typeID);
			identifierParts.Push (typeName);
		} else {
			identifierParts.Push ("未知类型");
		}
 	} else {
 		identifierParts.Push ("未知元素");
 	}
 	
	// 2. 尝试获取IFC类型
	GS::UniString ifcType;
	try {
		IFCAPI::ObjectAccessor objectAccessor = IFCAPI::GetObjectAccessor ();
		
		API_Elem_Head elemHeadForIFC = {};
		elemHeadForIFC.guid = elemGuid;
		GSErrCode errIFC = ACAPI_Element_GetHeader (&elemHeadForIFC);
		if (errIFC == NoError) {
			auto objectIDResult = objectAccessor.CreateElementObjectID (elemHeadForIFC);
			if (objectIDResult.IsOk ()) {
				IFCAPI::ObjectID objectID = objectIDResult.Unwrap ();
				auto ifcTypeResult = objectAccessor.GetIFCType (objectID);
				if (ifcTypeResult.IsOk ()) {
					ifcType = ifcTypeResult.Unwrap ();
				}
			}
		}
	} catch (...) {
		// IFC API异常，忽略但记录日志
		NewPluginDebugLog("[ComponentInfo] IFC API异常，忽略IFC类型获取");
	}
	
	if (!ifcType.IsEmpty ()) {
		identifierParts.Push ("IFC类型: " + ifcType);
	}
 	
 	// 3. 获取分类信息
 	GS::UniString classificationInfo, classificationId;
 	if (GetElementClassificationInfo (elemGuid, classificationInfo, classificationId) == NoError) {
 		if (!classificationInfo.IsEmpty ()) {
 			identifierParts.Push ("分类: " + classificationInfo);
 		}
 		if (!classificationId.IsEmpty ()) {
 			identifierParts.Push ("ID: " + classificationId);
 		}
 	}
 	
 	// 4. 获取构件编号（如果已设置）
 	PropertyDefinitionGuids guids;
 	if (FindPropertyDefinitions (guids) == NoError) {
 		GS::UniString compId;
 		if (GetStringPropertyFromElement (elemGuid, guids.compIdGuid, compId) == NoError && !compId.IsEmpty ()) {
 			identifierParts.Push ("编号: " + compId);
 		}
 	}
 	
 	// 构建最终标识字符串
 	if (identifierParts.IsEmpty ()) {
 		outIdentifier = "未设置标识";
 	} else {
 		// 使用 " | " 分隔各个部分
 		for (UInt32 i = 0; i < identifierParts.GetSize (); i++) {
 			if (i > 0) {
 				outIdentifier += " | ";
 			}
 			outIdentifier += identifierParts[i];
 		}
 	}
 	
  	return NoError;
  }

  // 导出所有梁元素的IFC属性到JSON文件
  GSErrCode ComponentInfo::ExportAllBeamIFCProperties (const IO::Location& outputPath)
  {
  	// 1. 获取所有梁元素
  	GS::Array<API_Guid> beamGuids;
  	GSErrCode err = ACAPI_Element_GetElemList (API_BeamID, &beamGuids);
  	if (err != NoError) {
  		ACAPI_WriteReport ("ExportAllBeamIFCProperties: ACAPI_Element_GetElemList 失败: %s", true, ErrID_To_Name (err));
  		return err;
  	}
  	
  	if (beamGuids.IsEmpty ()) {
  		ACAPI_WriteReport ("模型中未找到梁元素。", false);
  		return NoError;
  	}
  	
  	ACAPI_WriteReport (GS::UniString::SPrintf ("找到 %d 个梁元素，开始提取IFC属性...", beamGuids.GetSize ()), false);
  	
  	// 2. 打开输出文件
  	IO::File outputFile (outputPath, IO::File::Create);
  	err = outputFile.Open (IO::File::WriteEmptyMode);
  	if (err != NoError) {
  		ACAPI_WriteReport ("ExportAllBeamIFCProperties: 无法创建输出文件: %s", true, ErrID_To_Name (err));
  		return err;
  	}
  	
  	// 3. 获取时间戳和项目信息
  	// 时间戳
  	std::time_t now = std::time (nullptr);
  	std::tm* localTime = std::localtime (&now);
  	char timeStr[64];
  	std::strftime (timeStr, sizeof (timeStr), "%Y-%m-%d %H:%M:%S", localTime);
  	
  	// 项目名称
  	GS::UniString projectName = "";
  	API_ProjectInfo pi;
  	if (ACAPI_ProjectOperation_Project (&pi) == NoError && pi.projectName && !pi.projectName->IsEmpty ()) {
  		projectName = *pi.projectName;
  	}
  	
  	// 4. 写入JSON开始
  	GS::UniString jsonStr = "{\n";
  	jsonStr += "  \"metadata\": {\n";
  	jsonStr += "    \"total_beams\": " + GS::ValueToUniString (beamGuids.GetSize ()) + ",\n";
  	jsonStr += "    \"export_time\": \"" + GS::UniString (timeStr) + "\",\n";
  	jsonStr += "    \"model\": \"" + EscapeJsonString (projectName) + "\"\n";
  	jsonStr += "  },\n";
  	jsonStr += "  \"beams\": [\n";
  	
  	// 5. 遍历每个梁元素
  	for (UInt32 i = 0; i < beamGuids.GetSize (); ++i) {
  		const API_Guid& beamGuid = beamGuids[i];
  		
  		// 获取元素头信息（用于IFC类型）
  		API_Elem_Head elemHead = {};
  		elemHead.guid = beamGuid;
  		GSErrCode headerErr = ACAPI_Element_GetHeader (&elemHead);
  		if (headerErr != NoError) {
  			ACAPI_WriteReport (GS::UniString::SPrintf ("跳过元素 %d: 无法获取元素头信息", i), true);
  			continue;
  		}
  		
  		// 创建IFC ObjectID
  		auto objectIDResult = IFCAPI::GetObjectAccessor ().CreateElementObjectID (elemHead);
  		if (objectIDResult.IsErr ()) {
  			ACAPI_WriteReport (GS::UniString::SPrintf ("跳过元素 %d: 无法创建IFC ObjectID", i), true);
  			continue;
  		}
  		IFCAPI::ObjectID objectID = objectIDResult.Unwrap ();
  		
  		// 获取本地IFC属性
  		IFCAPI::PropertyAccessor propertyAccessor (objectID);
  		auto propertiesResult = propertyAccessor.GetLocalProperties ();
  		if (propertiesResult.IsErr ()) {
  			ACAPI_WriteReport (GS::UniString::SPrintf ("跳过元素 %d: 无法获取本地IFC属性", i), true);
  			continue;
  		}
  		std::vector<IFCAPI::Property> properties = propertiesResult.Unwrap ();
  		
  		// 获取预览IFC属性
  		std::vector<IFCAPI::Property> previewProperties;
  		auto previewResult = propertyAccessor.GetPreviewProperties ();
  		if (previewResult.IsOk ()) {
  			previewProperties = previewResult.Unwrap ();
  		}
  		
  		// 如果没有任何属性，跳过此元素
  		if (properties.empty () && previewProperties.empty ()) {
  			continue;
  		}
  		
  		// 构建此梁的JSON对象
  		if (i > 0) {
  			jsonStr += ",\n";
  		}
  		
  		jsonStr += "    {\n";
  		jsonStr += "      \"guid\": \"" + APIGuidToString (beamGuid) + "\",\n";
  		jsonStr += "      \"ifc_type\": \"IFCBeam\",\n";
  		jsonStr += "      \"properties\": [\n";
  		
  		// 处理本地属性
  		bool hasWrittenAnyProperty = false;
  		for (const auto& prop : properties) {
  			GS::UniString psetName = prop.GetPropertySetName ();
  			GS::UniString propName = prop.GetName ();
  			GS::UniString propValue = GetIFCPropertyValueString (prop);
  			
  			if (propValue.IsEmpty () || propValue == "[值]" || propValue == "[列表值]" || 
  			    propValue == "[范围值]" || propValue == "[枚举值]" || propValue == "[表值]") {
  				continue; // 跳过无意义的值
  			}
  			
  			if (hasWrittenAnyProperty) {
  				jsonStr += ",\n";
  			}
  			
  			jsonStr += "        {\n";
  			jsonStr += "          \"pset\": \"" + EscapeJsonString (psetName) + "\",\n";
  			jsonStr += "          \"name\": \"" + EscapeJsonString (propName) + "\",\n";
  			jsonStr += "          \"value\": \"" + EscapeJsonString (propValue) + "\",\n";
  			jsonStr += "          \"type\": \"single\"\n";
  			jsonStr += "        }";
  			
  			hasWrittenAnyProperty = true;
  		}
  		
  		// 处理预览属性（只记录名称）
  		for (const auto& prop : previewProperties) {
  			GS::UniString psetName = prop.GetPropertySetName ();
  			GS::UniString propName = prop.GetName ();
  			
  			if (hasWrittenAnyProperty) {
  				jsonStr += ",\n";
  			}
  			
  			jsonStr += "        {\n";
  			jsonStr += "          \"pset\": \"" + EscapeJsonString (psetName) + "\",\n";
  			jsonStr += "          \"name\": \"" + EscapeJsonString (propName) + "\",\n";
  			jsonStr += "          \"value\": \"\",\n";
  			jsonStr += "          \"type\": \"preview\"\n";
  			jsonStr += "        }";
  			
  			hasWrittenAnyProperty = true;
  		}
  		
  		jsonStr += "\n      ]\n";
  		jsonStr += "    }";
  		
  		// 进度报告
  		if ((i + 1) % 100 == 0) {
  			ACAPI_WriteReport (GS::UniString::SPrintf ("已处理 %d/%d 个梁元素", i + 1, beamGuids.GetSize ()), false);
  		}
  	}
  	
  	// 5. 写入JSON结束
  	jsonStr += "\n  ]\n";
  	jsonStr += "}\n";
  	
  	// 6. 写入文件
  	USize bytesWritten = 0;
  	err = outputFile.WriteBin (jsonStr.ToCStr (), jsonStr.GetLength () * sizeof (char), &bytesWritten);
  	if (err != NoError) {
  		ACAPI_WriteReport ("ExportAllBeamIFCProperties: 写入文件失败: %s", true, ErrID_To_Name (err));
  		outputFile.Close ();
  		return err;
  	}
  	
  	outputFile.Close ();
  	
  	// 7. 成功报告
  	ACAPI_WriteReport (GS::UniString::SPrintf ("成功导出 %d 个梁元素的IFC属性到文件。", beamGuids.GetSize ()), false);
  	return NoError;
  }

// Component info management (added for NewPlugin)
GSErrCode ComponentInfo::GetComponentInfo (API_Guid elemGuid, ComponentInfoData& outInfo)
{
    GSErrCode err = NoError;
    PropertyDefinitionGuids guids;
    err = FindPropertyDefinitions (guids);
    if (err != NoError) return err;
    
    err = GetStringPropertyFromElement (elemGuid, guids.compIdGuid, outInfo.componentId);
    if (err != NoError) outInfo.componentId = "";
    
    err = GetStringPropertyFromElement (elemGuid, guids.photoDescGuid, outInfo.photoDesc);
    if (err != NoError) outInfo.photoDesc = "";
    
    err = GetPhotoPathsFromElement (elemGuid, guids.photoPathsGuid, outInfo.photoPaths);
    if (err != NoError) outInfo.photoPaths.Clear ();
    
    // Get IFC GlobalId property
    outInfo.globalId = GetGlobalIdPropertyValue (elemGuid);
    outInfo.folderName = GlobalIdToFolderName (elemGuid);
    
    return NoError;
}

GSErrCode ComponentInfo::SaveComponentInfo (API_Guid elemGuid, const ComponentInfoData& info)
{
    GSErrCode err = NoError;
    PropertyDefinitionGuids guids;
    err = FindPropertyDefinitions (guids);
    if (err != NoError) return err;
    
    err = WritePropertiesToElement (elemGuid, guids, info.componentId, info.photoDesc, info.photoPaths);
    return err;
}


// GlobalId property retrieval (IFC) - implementation
GS::UniString ComponentInfo::GetGlobalIdPropertyValue (const API_Guid& elementGuid)
{
    GS::UniString globalId;
    
    try {
        IFCAPI::ObjectAccessor objectAccessor = IFCAPI::GetObjectAccessor ();
        
        API_Elem_Head elemHead{};
        elemHead.guid = elementGuid;
        
        GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError)
            return globalId;
        
        auto objectIDResult = objectAccessor.CreateElementObjectID (elemHead);
        if (objectIDResult.IsErr ())
            return globalId;
        
        IFCAPI::ObjectID objectID = objectIDResult.Unwrap ();
        IFCAPI::PropertyAccessor propertyAccessor (objectID);
        
        // Get IFC attributes
        auto attributesResult = propertyAccessor.GetAttributes ();
        if (attributesResult.IsOk ()) {
            std::vector<IFCAPI::Attribute> attributes = attributesResult.Unwrap ();
            for (const IFCAPI::Attribute& attribute : attributes) {
                if (attribute.GetName ().IsEqual ("GlobalId", GS::CaseInsensitive)) {
                    auto value = attribute.GetValue ();
                    if (value.has_value ()) {
                        globalId = value.value ();
                    }
                    break;
                }
            }
        }
    }
    catch (...) {
        // IFC API异常，忽略但记录日志
        NewPluginDebugLog("[ComponentInfo] IFC API异常，忽略GlobalId获取");
    }
    
    return globalId; // Returns empty string if GlobalId not found
}

GS::UniString ComponentInfo::GlobalIdToFolderName (const API_Guid& elementGuid)
{
    GS::UniString globalId = GetGlobalIdPropertyValue (elementGuid);
    // Empty string indicates no GlobalId (photo functionality disabled per guardrail)
    return globalId;
}


GSErrCode ComponentInfo::GetPhotoFullPath (const GS::UniString& relPath, IO::Location& outFullPath)
{
    IO::Location photoRootLoc, projectFolderLoc;
    GS::UniString projectName;
    GSErrCode err = GetProjectPhotoFolder (photoRootLoc, projectFolderLoc, projectName);
    if (err != NoError) return err;
    
    outFullPath = photoRootLoc;
    return outFullPath.AppendToLocal (IO::Name (relPath));
}

GSErrCode ComponentInfo::SetStringPropertyToElement (API_Guid elemGuid, API_Guid defGuid, const GS::UniString& val)
{
    API_PropertyDefinition def = {};
    def.guid = defGuid;
    GSErrCode err = ACAPI_Property_GetPropertyDefinition (def);
    if (err != NoError) return err;
    
    API_Property p = {};
    p.definition = def;
    p.status = API_Property_HasValue;
    p.isDefault = false;
    p.value.variantStatus = API_VariantStatusNormal;
    p.value.singleVariant.variant.type = API_PropertyStringValueType;
    p.value.singleVariant.variant.uniStringValue = val;
    
    return ACAPI_Element_SetProperty (elemGuid, p);
}


GSErrCode ComponentInfo::ReadPropertiesFromElement (API_Guid elemGuid, const PropertyDefinitionGuids& guids, GS::UniString& outCompId, GS::UniString& outPhotoDesc, GS::Array<GS::UniString>& outRelPaths)
{
    GSErrCode err = NoError;
    
    err = GetStringPropertyFromElement (elemGuid, guids.compIdGuid, outCompId);
    if (err != NoError) outCompId = "";
    
    err = GetStringPropertyFromElement (elemGuid, guids.photoDescGuid, outPhotoDesc);
    if (err != NoError) outPhotoDesc = "";
    
    err = GetPhotoPathsFromElement (elemGuid, guids.photoPathsGuid, outRelPaths);
    if (err != NoError) outRelPaths.Clear ();
    
    return NoError;
}

