#include "PluginPalette.hpp"
#include "APICommon.h"
#include "APIdefs.h"
#include "DGModule.hpp"
#include "FileSystem.hpp"
#include <mutex>
#include <stdio.h>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <iomanip>

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
	
	// HBIM图片常量
	static const GS::UniString kHBIMImageGroupName = "HBIM构件图片";
	static const GS::UniString kHBIMImageLinksName = "HBIM图片链接";
	
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
	
	// 获取当前时间戳字符串
	static GS::UniString GetCurrentTimestamp()
	{
		auto now = std::chrono::system_clock::now();
		auto now_time_t = std::chrono::system_clock::to_time_t(now);
		auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch()) % 1000;
		
		std::tm tm_buf;
		localtime_r(&now_time_t, &tm_buf);
		
		std::ostringstream oss;
		oss << std::put_time(&tm_buf, "%Y%m%d_%H%M%S_")
			<< std::setfill('0') << std::setw(3) << now_ms.count();
		
		return GS::UniString(oss.str().c_str());
	}
	
	// 从文件路径中提取文件名
	static GS::UniString GetFileNameFromPath(const GS::UniString& filePath)
	{
		IO::Location loc(filePath);
		IO::Name fileName;
		loc.GetLastLocalName(&fileName);
		return GS::UniString(fileName);
	}
	

	
	// 复制文件到目标文件夹
	static bool CopyFileToDestination(const GS::UniString& sourcePath, const GS::UniString& destFolder, 
									  const GS::UniString& newFileName, GS::UniString& outDestPath)
	{
		try {
			// 创建目标文件夹（如果不存在）
			std::filesystem::path destFolderPath(destFolder.ToCStr().Get());
			if (!std::filesystem::exists(destFolderPath)) {
				std::filesystem::create_directories(destFolderPath);
			}
			
			// 构建目标文件路径
			std::filesystem::path destFilePath = destFolderPath / newFileName.ToCStr().Get();
			
			// 复制文件
			std::filesystem::path sourceFilePath(sourcePath.ToCStr().Get());
			if (!std::filesystem::exists(sourceFilePath)) {
				ACAPI_WriteReport("CopyFileToDestination: 源文件不存在: %s", true, sourcePath.ToCStr().Get());
				return false;
			}
			
			std::filesystem::copy_file(sourceFilePath, destFilePath, 
									   std::filesystem::copy_options::overwrite_existing);
			
			outDestPath = GS::UniString(destFilePath.string().c_str());
			return true;
		} catch (const std::filesystem::filesystem_error& e) {
			ACAPI_WriteReport("CopyFileToDestination: 文件复制失败: %s", true, e.what());
			return false;
		} catch (...) {
			ACAPI_WriteReport("CopyFileToDestination: 未知错误", true);
			return false;
		}
	}
	
	// 创建或获取HBIM图片属性组
	static GSErrCode FindOrCreateHBIMImageGroup(API_PropertyGroup& outGroup)
	{
		GS::Array<API_PropertyGroup> groups;
		GSErrCode err = ACAPI_Property_GetPropertyGroups(groups);
		if (err != NoError) {
			ACAPI_WriteReport("FindOrCreateHBIMImageGroup: GetPropertyGroups 失败: %s", true, GS::UniString::Printf("Error %d", err).ToCStr().Get());
			return err;
		}
		
		// 先查找已存在的属性组
		for (UInt32 i = 0; i < groups.GetSize(); ++i) {
			if (groups[i].name == kHBIMImageGroupName) {
				outGroup = groups[i];
				return NoError;
			}
		}
		
		// 创建新组
		outGroup = {};
		outGroup.guid = APINULLGuid;
		outGroup.name = kHBIMImageGroupName;
		outGroup.description = "HBIM构件图片链接";
		err = ACAPI_Property_CreatePropertyGroup(outGroup);
		if (err != NoError) {
			ACAPI_WriteReport("FindOrCreateHBIMImageGroup: CreatePropertyGroup 失败: %s (错误码: %d)", true, GS::UniString::Printf("Error %d", err).ToCStr().Get(), err);
		}
		
		// 无论如何都重新查找，确保获取到正确的group guid
		GS::Array<API_PropertyGroup> groups2;
		GSErrCode err2 = ACAPI_Property_GetPropertyGroups(groups2);
		ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 开始查找，共 %d 个属性组", false, (int)groups2.GetSize());
		if (err2 == NoError) {
			for (UInt32 i = 0; i < groups2.GetSize(); ++i) {
				ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 检查属性组[%d]: '%s'", false, (int)i, groups2[i].name.ToCStr().Get());
				GS::UniString existingName = groups2[i].name;
				GS::UniString targetName = kHBIMImageGroupName;
				if (existingName == targetName || 
					existingName.Contains(targetName) || 
					targetName.Contains(existingName)) {
					outGroup = groups2[i];
					ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 通过宽松比较找到匹配！guid=%s", false, APIGuidToString(outGroup.guid).ToCStr().Get());
					return NoError;
				}
			}
		}
		
		ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 未找到属性组 '%s'", true, kHBIMImageGroupName.ToCStr().Get());
		return err;
	}
	
	// 创建或获取HBIM图片链接属性定义
	static GSErrCode FindOrCreateHBIMImageDefinition(const API_PropertyGroup& group, 
													 API_PropertyDefinition& outDef, 
													 GS::Array<API_Guid>& allClassificationItems)
	{
		GS::Array<API_PropertyDefinition> defs;
		GSErrCode err = ACAPI_Property_GetPropertyDefinitions(group.guid, defs);
		if (err != NoError) {
			ACAPI_WriteReport("FindOrCreateHBIMImageDefinition: GetPropertyDefinitions 失败: %s", true, GS::UniString::Printf("Error %d", err).ToCStr().Get());
			return err;
		}
		
		for (UInt32 i = 0; i < defs.GetSize(); ++i) {
			if (defs[i].name == kHBIMImageLinksName) {
				outDef = defs[i];
				return NoError;
			}
		}
		
		// 属性不存在，创建新的
		outDef = {};
		outDef.guid = APINULLGuid;
		outDef.groupGuid = group.guid;
		outDef.name = kHBIMImageLinksName;
		outDef.collectionType = API_PropertySingleCollectionType;
		outDef.valueType = API_PropertyStringValueType;
		outDef.measureType = API_PropertyDefaultMeasureType;
		outDef.canValueBeEditable = true;
		outDef.definitionType = API_PropertyCustomDefinitionType;
		outDef.defaultValue.basicValue.variantStatus = API_VariantStatusNormal;
		outDef.defaultValue.basicValue.singleVariant.variant.type = API_PropertyStringValueType;
		outDef.availability = allClassificationItems;
		
		err = ACAPI_Property_CreatePropertyDefinition(outDef);
		if (err != NoError) {
			ACAPI_WriteReport("FindOrCreateHBIMImageDefinition: CreatePropertyDefinition 失败: %s (错误码: %d)", true, GS::UniString::Printf("Error %d", err).ToCStr().Get(), err);
			
			// 创建失败，可能是属性已存在，重新查找
			GS::Array<API_PropertyDefinition> defs2;
			GSErrCode err2 = ACAPI_Property_GetPropertyDefinitions(group.guid, defs2);
			if (err2 == NoError) {
				for (UInt32 i = 0; i < defs2.GetSize(); ++i) {
					if (defs2[i].name == kHBIMImageLinksName) {
						outDef = defs2[i];
						ACAPI_WriteReport("FindOrCreateHBIMImageDefinition: 找到已存在的属性定义", false);
						return NoError;
					}
				}
			}
		}
		return err;
	}
	
	// 确保HBIM图片属性组和定义存在
	static GSErrCode EnsureHBIMImagePropertyGroupAndDefinitions(API_Guid& outGroupGuid, API_Guid& outImageLinksGuid)
	{
		API_PropertyGroup group;
		GSErrCode err = FindOrCreateHBIMImageGroup(group);
		if (err != NoError) return err;
		
		// 收集所有分类项
		GS::Array<API_Guid> allClassificationItems;
		GetAllClassificationItems(allClassificationItems);
		
		API_PropertyDefinition defImageLinks;
		err = FindOrCreateHBIMImageDefinition(group, defImageLinks, allClassificationItems);
		if (err != NoError) return err;
		
		outGroupGuid = group.guid;
		outImageLinksGuid = defImageLinks.guid;
		return NoError;
	}
	
	// 从元素读取HBIM图片链接属性值
	static GSErrCode GetHBIMImageLinksPropertyValue(const API_Guid& elemGuid, const API_Guid& defGuid, GS::UniString& outVal)
	{
		API_Property p = {};
		GSErrCode err = ACAPI_Element_GetPropertyValue(elemGuid, defGuid, p);
		if (err != NoError || p.status != API_Property_HasValue || p.value.variantStatus != API_VariantStatusNormal) {
			outVal = "[]"; // 返回空JSON数组
			return err;
		}
		outVal = p.value.singleVariant.variant.uniStringValue;
		return NoError;
	}
	
	// 向元素写入HBIM图片链接属性值
	static GSErrCode SetHBIMImageLinksPropertyValue(const API_Guid& elemGuid, const API_Guid& defGuid, const GS::UniString& value)
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
	, imageSeparator (GetReference (), ImageSeparatorId)
	, imageTitle (GetReference (), ImageTitleId)
	, imageTitleUnderline (GetReference (), ImageTitleUnderlineId)
	, imageCountLabel (GetReference (), ImageCountLabelId)
	, imageCountValue (GetReference (), ImageCountValueId)
	, imageCurrentLabel (GetReference (), ImageCurrentLabelId)
	, imageCurrentValue (GetReference (), ImageCurrentValueId)
	, imageSelectButton (GetReference (), ImageSelectButtonId)
	, imageDeleteButton (GetReference (), ImageDeleteButtonId)
	, imagePrevButton (GetReference (), ImagePrevButtonId)
	, imageNextButton (GetReference (), ImageNextButtonId)
	, imageOKButton (GetReference (), ImageOKButtonId)
	, imageCancelButton (GetReference (), ImageCancelButtonId)
	, imagePreview (GetReference (), ImagePreviewId)
	, hasHBIMProperties (false)
	, isHBIMEditMode (false)
	, hbimGroupGuid (APINULLGuid)
	, hbimIdGuid (APINULLGuid)
	, hbimDescGuid (APINULLGuid)
	, currentElemGuid (APINULLGuid)
	, isReSelectingElement (false)
	, hasHBIMImages (false)
	, isImageEditMode (false)
	, currentImageIndex (0)
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
	
	// 初始化HBIM图片UI
	imageSeparator.Show();
	imageTitle.SetText("HBIM构件图片");
	imageTitle.Show();
	imageTitleUnderline.Show();
	imageTitleUnderline.Redraw();
	imageCountLabel.SetText("图片数量:");
	imageCountLabel.Show();
	imageCountValue.SetText("0");
	imageCountValue.Show();
	imageCurrentLabel.SetText("当前图片:");
	imageCurrentLabel.Show();
	imageCurrentValue.SetText("无");
	imageCurrentValue.Show();
	
	imageSelectButton.SetText("附着图片");
	imageSelectButton.Show();
	imageDeleteButton.SetText("删除当前");
	imageDeleteButton.Show();
	imagePrevButton.SetText("上一张");
	imagePrevButton.Show();
	imageNextButton.SetText("下一张");
	imageNextButton.Show();
	imageOKButton.SetText("确定");
	imageOKButton.Show();
	imageCancelButton.SetText("取消");
	imageCancelButton.Show();
	
	// 附加图片按钮观察者
	imageSelectButton.Attach(*this);
	imageDeleteButton.Attach(*this);
	imagePrevButton.Attach(*this);
	imageNextButton.Attach(*this);
	imageOKButton.Attach(*this);
	imageCancelButton.Attach(*this);
	
	imageSelectButton.Enable();
	imageDeleteButton.Disable();
	imagePrevButton.Disable();
	imageNextButton.Disable();
	imageOKButton.Hide();
	imageCancelButton.Hide();
	
	// 初始化图片预览控件（暂时使用LeftText代替DG::Picture）
	imagePreview.Show();
	imagePreview.Redraw();
	
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
		
		// 重置HBIM图片显示
		hasHBIMImages = false;
		isImageEditMode = false;
		imagePaths.Clear();
		originalImagePaths.Clear();
		currentImageIndex = 0;
		UpdateHBIMImageUI();
		return;
	}
	
	// 使用第一个选中的元素
	API_Guid elemGuid = selNeigs[0].guid;
	
	// 如果切换了元素，取消任何编辑模式
	if (elemGuid != currentElemGuid) {
		if (isHBIMEditMode) {
			ExitHBIMEditMode(false);
		}
		if (isImageEditMode) {
			ExitImageEditMode(false);
		}
	}
	
	currentElemGuid = elemGuid;
	
	// 更新IFC属性显示
	GS::UniString ifcType = GetIFCTypeForElement(elemGuid);
	GS::UniString globalId = GetGlobalIdForElement(elemGuid);
	typeValue.SetText(ifcType);
	idValue.SetText(globalId);
	typeValue.Redraw();
	idValue.Redraw();
	
	// 检查并更新HBIM属性
	CheckHBIMProperties(elemGuid);
	
	// 检查并更新HBIM图片
	CheckHBIMImages();
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
		
		// 检查HBIM图片
		instance.CheckHBIMImages();
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
		ExitHBIMEditMode(false); // 取消HBIM属性编辑
	}
	if (isImageEditMode) {
		ExitImageEditMode(false); // 取消图片编辑
	}
	*accepted = true;
}

void PluginPalette::UpdateHBIMImageUI ()
{
	// 更新图片计数和当前图片显示
	if (hasHBIMImages && imagePaths.GetSize() > 0) {
		GS::UniString countText;
		countText.Printf("%d", imagePaths.GetSize());
		imageCountValue.SetText(countText);
		
		if (currentImageIndex < imagePaths.GetSize()) {
			GS::UniString currentText;
			currentText.Printf("%d/%d", currentImageIndex + 1, imagePaths.GetSize());
			imageCurrentValue.SetText(currentText);
			
			// 显示当前图片信息（暂时使用文本显示）
			GS::UniString currentImagePath = imagePaths[currentImageIndex];
			GS::UniString previewText;
			previewText.Printf("图片: %s", currentImagePath.ToCStr().Get());
			imagePreview.SetText(previewText);
		}
	} else {
		imageCountValue.SetText("0");
		imageCurrentValue.SetText("无");
		
		// 没有图片时显示占位符文本
		imagePreview.SetText("无图片预览");
	}
	
	// 根据编辑模式更新按钮状态
	if (isImageEditMode) {
		// 编辑模式：显示确定和取消按钮，隐藏选择/编辑按钮
		imageSelectButton.Hide();
		imageOKButton.Show();
		imageCancelButton.Show();
		
		// 在编辑模式下启用删除和导航按钮
		if (hasHBIMImages && imagePaths.GetSize() > 0) {
			imageDeleteButton.Enable();
			imagePrevButton.Enable();
			imageNextButton.Enable();
			
			if (currentImageIndex == 0) {
				imagePrevButton.Disable();
			}
			if (currentImageIndex >= imagePaths.GetSize() - 1) {
				imageNextButton.Disable();
			}
		} else {
			imageDeleteButton.Disable();
			imagePrevButton.Disable();
			imageNextButton.Disable();
		}
	} else {
		// 非编辑模式：显示选择/编辑按钮，隐藏确定和取消按钮
		imageSelectButton.Show();
		imageOKButton.Hide();
		imageCancelButton.Hide();
		
		// 根据是否有图片更新选择按钮文本
		if (hasHBIMImages && imagePaths.GetSize() > 0) {
			imageSelectButton.SetText("编辑");
			
			// 非编辑模式下启用删除和导航按钮
			imageDeleteButton.Enable();
			imagePrevButton.Enable();
			imageNextButton.Enable();
			
			if (currentImageIndex == 0) {
				imagePrevButton.Disable();
			}
			if (currentImageIndex >= imagePaths.GetSize() - 1) {
				imageNextButton.Disable();
			}
		} else {
			imageSelectButton.SetText("附着图片");
			
			// 没有图片时禁用删除和导航按钮
			imageDeleteButton.Disable();
			imagePrevButton.Disable();
			imageNextButton.Disable();
		}
	}
	
	// 重绘所有控件
	imageCountValue.Redraw();
	imageCurrentValue.Redraw();
	imageSelectButton.Redraw();
	imageDeleteButton.Redraw();
	imagePrevButton.Redraw();
	imageNextButton.Redraw();
	imageOKButton.Redraw();
	imageCancelButton.Redraw();
	imagePreview.Redraw();
}

void PluginPalette::EnterImageEditMode ()
{
	if (currentElemGuid == APINULLGuid) {
		DG::InformationAlert("提示", "请先选择一个构件", "确定");
		return;
	}
	
	// 保存原始图片路径用于取消编辑时恢复
	originalImagePaths = imagePaths;
	
	// 进入编辑模式
	isImageEditMode = true;
	
	// 更新UI
	UpdateHBIMImageUI();
	
	ACAPI_WriteReport("EnterImageEditMode: 进入图片编辑模式", false);
}

void PluginPalette::ExitImageEditMode (bool save)
{
	if (!isImageEditMode) {
		return;
	}
	
	if (save) {
		// 保存更改：将当前图片路径保存到属性
		if (currentElemGuid != APINULLGuid) {
			// 确保HBIM图片属性组和定义存在
			API_Guid imageGroupGuid, imageLinksGuid;
			GSErrCode err = EnsureHBIMImagePropertyGroupAndDefinitions(imageGroupGuid, imageLinksGuid);
			if (err == NoError) {
				// 构建JSON数组保存到属性
				if (imagePaths.GetSize() > 0) {
					GS::UniString jsonArray = "[";
					for (UInt32 i = 0; i < imagePaths.GetSize(); i++) {
						if (i > 0) jsonArray.Append(",");
						jsonArray.Append("\"");
						jsonArray.Append(imagePaths[i]);
						jsonArray.Append("\"");
					}
					jsonArray.Append("]");
					
					// 保存到属性
					err = SetHBIMImageLinksPropertyValue(currentElemGuid, imageLinksGuid, jsonArray);
					if (err != NoError) {
						DG::InformationAlert("警告", "无法保存图片链接到属性", "确定");
					}
				} else {
					// 如果没有图片，保存空数组
					SetHBIMImageLinksPropertyValue(currentElemGuid, imageLinksGuid, "[]");
				}
				
				// 更新状态
				hasHBIMImages = (imagePaths.GetSize() > 0);
				currentImageIndex = 0;
			}
		}
	} else {
		// 取消编辑：恢复原始图片路径
		imagePaths = originalImagePaths;
		hasHBIMImages = (imagePaths.GetSize() > 0);
		currentImageIndex = 0;
		
		ACAPI_WriteReport("ExitImageEditMode: 取消编辑，恢复原始图片", false);
	}
	
	// 退出编辑模式
	isImageEditMode = false;
	
	// 清空原始路径
	originalImagePaths.Clear();
	
	// 更新UI
	UpdateHBIMImageUI();
	
	ACAPI_WriteReport("ExitImageEditMode: 退出图片编辑模式", false);
}

void PluginPalette::CheckHBIMImages ()
{
	hasHBIMImages = false;
	imagePaths.Clear();
	currentImageIndex = 0;
	
	if (currentElemGuid == APINULLGuid) {
		UpdateHBIMImageUI();
		return;
	}
	
	// 尝试查找现有的HBIM图片属性组和定义
	API_Guid imageGroupGuid, imageLinksGuid;
	GSErrCode err = EnsureHBIMImagePropertyGroupAndDefinitions(imageGroupGuid, imageLinksGuid);
	if (err != NoError) {
		// 属性定义不存在，没有图片
		UpdateHBIMImageUI();
		return;
	}
	
	// 从属性读取图片链接
	GS::UniString imageLinksJson;
	err = GetHBIMImageLinksPropertyValue(currentElemGuid, imageLinksGuid, imageLinksJson);
	if (err != NoError || imageLinksJson == "[]" || imageLinksJson.GetLength() <= 2) {
		// 没有图片链接或读取失败
		UpdateHBIMImageUI();
		return;
	}
	
	// 解析JSON数组 - 简单手动解析
	GS::UniString trimmed = imageLinksJson;
	trimmed.DeleteFirst();
	trimmed.DeleteLast();
	
	// 手动解析逗号分隔的带引号的字符串
	UInt32 startPos = 0;
	while (startPos < trimmed.GetLength()) {
		// 查找引号开始位置
		Int32 quoteStart = trimmed.FindFirst('"', startPos);
		if (quoteStart == MaxUIndex) break;
		
		// 查找引号结束位置
		Int32 quoteEnd = trimmed.FindFirst('"', quoteStart + 1);
		if (quoteEnd == MaxUIndex) break;
		
		// 提取引号内的内容
		GS::UniString path = trimmed.GetSubstring(quoteStart + 1, quoteEnd);
		if (!path.IsEmpty()) {
			imagePaths.Push(path);
		}
		
		startPos = quoteEnd + 1;
	}
	
	// 更新状态
	hasHBIMImages = (imagePaths.GetSize() > 0);
	UpdateHBIMImageUI();
}

void PluginPalette::SelectHBIMImages ()
{
	if (currentElemGuid == APINULLGuid) {
		DG::InformationAlert("提示", "请先选择一个构件", "确定");
		return;
	}
	
	// 使用DG::FileDialog选择多个图片文件
	DG::FileDialog dlg(DG::FileDialog::OpenMultiFile);
	FTM::FileTypeManager mgr("HBIMComponentEntryImages");
	FTM::FileType typeJpg("JPEG", "jpg", 0, 0, 0);
	FTM::FileType typePng("PNG", "png", 0, 0, 0);
	FTM::TypeID idJpg = mgr.AddType(typeJpg);
	FTM::TypeID idPng = mgr.AddType(typePng);
	dlg.AddFilter(idJpg);
	dlg.AddFilter(idPng);
	dlg.SetTitle("选择HBIM构件图片");
	
	if (dlg.Invoke()) {
		// 确保图片文件夹存在
		GSErrCode err = EnsureHBIMImageFolder();
		if (err != NoError) {
			DG::InformationAlert("错误", "无法创建图片文件夹，请确保项目已保存", "确定");
			return;
		}
		
		// 获取构件GlobalId
		GS::UniString globalId = GetGlobalIdForElement(currentElemGuid);
		if (globalId == "未找到") {
			DG::InformationAlert("错误", "无法获取构件GlobalId", "确定");
			return;
		}
		
		// 确保HBIM图片属性组和定义存在
		API_Guid imageGroupGuid, imageLinksGuid;
		err = EnsureHBIMImagePropertyGroupAndDefinitions(imageGroupGuid, imageLinksGuid);
		if (err != NoError) {
			DG::InformationAlert("错误", "无法创建图片属性定义", "确定");
			return;
		}
		
		// 读取现有的图片链接
		GS::UniString existingImageLinksJson;
		GetHBIMImageLinksPropertyValue(currentElemGuid, imageLinksGuid, existingImageLinksJson);
		
		// 解析现有的图片链接
		GS::Array<GS::UniString> existingImagePaths;
		if (existingImageLinksJson != "[]" && existingImageLinksJson.GetLength() > 2) {
			// 简单解析JSON数组，格式：["path1", "path2", ...]
			GS::UniString trimmed = existingImageLinksJson;
			trimmed.DeleteFirst();
			trimmed.DeleteLast();
			
			// 手动解析逗号分隔的带引号的字符串
			UInt32 startPos = 0;
			while (startPos < trimmed.GetLength()) {
				// 查找引号开始位置
				Int32 quoteStart = trimmed.FindFirst('"', startPos);
				if (quoteStart == MaxUIndex) break;
				
				// 查找引号结束位置
				Int32 quoteEnd = trimmed.FindFirst('"', quoteStart + 1);
				if (quoteEnd == MaxUIndex) break;
				
				// 提取引号内的内容
				GS::UniString path = trimmed.GetSubstring(quoteStart + 1, quoteEnd);
				if (!path.IsEmpty()) {
					existingImagePaths.Push(path);
				}
				
				startPos = quoteEnd + 1;
			}
		}
		
		// 清空现有图片路径，使用从属性读取的路径
		imagePaths = existingImagePaths;
		
		// 获取选中的文件并复制到项目文件夹
		USize n = dlg.GetSelectionCount();
		for (UIndex i = 0; i < n; ++i) {
			IO::Location fileLocation = dlg.GetSelectedFile(i);
			GS::UniString sourcePath;
			fileLocation.ToPath(&sourcePath);
			
			// 生成新的文件名：timestamp_originalName.ext
			GS::UniString timestamp = GetCurrentTimestamp();
			GS::UniString originalFileName = GetFileNameFromPath(sourcePath);
			GS::UniString newFileName;
			newFileName.Printf("%s_%s", timestamp.ToCStr().Get(), originalFileName.ToCStr().Get());
			
			// 构建目标文件夹路径：HBIM_Images_{projectHash}/{elementGlobalId}/
			GS::UniString destFolder;
			destFolder.Printf("HBIM_Images_%s/%s", projectHash.ToCStr().Get(), globalId.ToCStr().Get());
			
			// 构建项目文件夹路径
			API_ProjectInfo projectInfo;
			if (ACAPI_ProjectOperation_Project(&projectInfo) != NoError || projectInfo.untitled) {
				DG::InformationAlert("错误", "项目未保存，无法复制图片", "确定");
				return;
			}
			
			IO::Location projectLocation = *projectInfo.location;
			GS::UniString projectPath;
			projectLocation.ToPath(&projectPath);
			
			GS::UniString fullDestFolder;
			fullDestFolder.Printf("%s/%s", projectPath.ToCStr().Get(), destFolder.ToCStr().Get());
			
			// 复制文件到目标文件夹
			GS::UniString destPath;
			if (CopyFileToDestination(sourcePath, fullDestFolder, newFileName, destPath)) {
				// 添加相对路径到图片路径列表
				GS::UniString relativePath;
				relativePath.Printf("%s/%s", destFolder.ToCStr().Get(), newFileName.ToCStr().Get());
				imagePaths.Push(relativePath);
			} else {
				DG::InformationAlert("警告", GS::UniString::Printf("无法复制文件: %s", originalFileName.ToCStr().Get()).ToCStr().Get(), "确定");
			}
		}
		
		// 根据是否在编辑模式决定是否保存到属性
		if (!isImageEditMode) {
			// 不在编辑模式：直接保存到属性
			if (imagePaths.GetSize() > 0) {
				GS::UniString jsonArray = "[";
				for (UInt32 i = 0; i < imagePaths.GetSize(); i++) {
					if (i > 0) jsonArray.Append(",");
					jsonArray.Append("\"");
					jsonArray.Append(imagePaths[i]);
					jsonArray.Append("\"");
				}
				jsonArray.Append("]");
				
				// 保存到属性
				err = SetHBIMImageLinksPropertyValue(currentElemGuid, imageLinksGuid, jsonArray);
				if (err != NoError) {
					DG::InformationAlert("警告", "无法保存图片链接到属性", "确定");
				}
			} else {
				// 如果没有图片，保存空数组
				SetHBIMImageLinksPropertyValue(currentElemGuid, imageLinksGuid, "[]");
			}
		}
		// 在编辑模式下，不保存到属性，等待用户点击确定
		
		// 更新状态
		hasHBIMImages = (imagePaths.GetSize() > 0);
		currentImageIndex = 0;
		UpdateHBIMImageUI();
	}
}

void PluginPalette::DeleteCurrentHBIMImage ()
{
	if (!hasHBIMImages || imagePaths.GetSize() == 0 || currentImageIndex >= imagePaths.GetSize()) {
		return;
	}
	
	// 删除当前图片
	imagePaths.Delete(currentImageIndex);
	
	// 根据是否在编辑模式决定是否保存到属性
	if (!isImageEditMode) {
		// 不在编辑模式：直接保存到属性
		API_Guid imageGroupGuid, imageLinksGuid;
		GSErrCode err = EnsureHBIMImagePropertyGroupAndDefinitions(imageGroupGuid, imageLinksGuid);
		if (err == NoError) {
			// 构建更新后的JSON数组
			GS::UniString jsonArray = "[";
			for (UInt32 i = 0; i < imagePaths.GetSize(); i++) {
				if (i > 0) jsonArray.Append(",");
				jsonArray.Append("\"");
				jsonArray.Append(imagePaths[i]);
				jsonArray.Append("\"");
			}
			jsonArray.Append("]");
			
			// 保存到属性
			SetHBIMImageLinksPropertyValue(currentElemGuid, imageLinksGuid, jsonArray);
		}
	}
	// 在编辑模式下，不保存到属性，等待用户点击确定
	
	// 更新状态
	if (imagePaths.GetSize() == 0) {
		hasHBIMImages = false;
		currentImageIndex = 0;
	} else {
		// 调整当前索引
		if (currentImageIndex >= imagePaths.GetSize()) {
			currentImageIndex = imagePaths.GetSize() - 1;
		}
	}
	
	UpdateHBIMImageUI();
}

void PluginPalette::NavigateHBIMImage (bool forward)
{
	if (!hasHBIMImages || imagePaths.GetSize() == 0) {
		return;
	}
	
	if (forward) {
		if (currentImageIndex < imagePaths.GetSize() - 1) {
			currentImageIndex++;
		}
	} else {
		if (currentImageIndex > 0) {
			currentImageIndex--;
		}
	}
	
	UpdateHBIMImageUI();
}

GSErrCode PluginPalette::EnsureHBIMImageFolder ()
{
	// 获取项目信息
	API_ProjectInfo projectInfo;
	GSErrCode err = ACAPI_ProjectOperation_Project(&projectInfo);
	if (err != NoError) {
		ACAPI_WriteReport("EnsureHBIMImageFolder: 获取项目信息失败", true);
		return err;
	}
	
	// 检查项目是否已保存
	if (projectInfo.untitled) {
		ACAPI_WriteReport("EnsureHBIMImageFolder: 项目未保存，无法创建图片文件夹", false);
		return APIERR_GENERAL;
	}
	
	// 获取项目文件夹路径
	IO::Location projectLocation = *projectInfo.location;
	GS::UniString projectPath;
	projectLocation.ToPath(&projectPath);
	
	// 计算项目哈希（用于唯一标识项目）
	projectHash = CalculateProjectHash();
	
	// 创建HBIM_Images文件夹
	GS::UniString hbimImagesPath = projectPath;
	hbimImagesPath.Append("/HBIM_Images");
	
	// 检查文件夹是否存在，不存在则创建
	// 暂时简化实现，不检查文件夹是否存在
	// 后续需要实现完整的文件夹创建逻辑
	
	return NoError;
}

GS::UniString PluginPalette::CalculateProjectHash ()
{
	// 获取项目信息
	API_ProjectInfo projectInfo;
	GSErrCode err = ACAPI_ProjectOperation_Project(&projectInfo);
	if (err != NoError) {
		return "unknown";
	}
	
	// 使用项目路径计算哈希
	IO::Location projectLocation = *projectInfo.location;
	GS::UniString projectPath;
	projectLocation.ToPath(&projectPath);
	
	// 简单哈希计算
	UInt32 hashValue = GS::CalculateHashValue(projectPath);
	GS::UniString hashStr;
	hashStr.Printf("%08X", hashValue);
	return hashStr;
}

bool PluginPalette::IsProjectSaved ()
{
	API_ProjectInfo projectInfo;
	GSErrCode err = ACAPI_ProjectOperation_Project(&projectInfo);
	if (err != NoError) {
		return false;
	}
	
	return !projectInfo.untitled;
}

void PluginPalette::ButtonClicked (const DG::ButtonClickEvent& ev)
{
	if (ev.GetSource() == &hbimActionButton) {
		if (isHBIMEditMode) {
			ExitHBIMEditMode(true);
		} else {
			if (currentElemGuid == APINULLGuid) {
				DG::InformationAlert("提示", "请先选择一个构件", "确定");
				return;
			}
			EnterHBIMEditMode();
		}
	} else if (ev.GetSource() == &hbimCancelButton) {
		ExitHBIMEditMode(false);
	} else if (ev.GetSource() == &imageSelectButton) {
		// 根据当前状态决定行为
		if (hasHBIMImages && imagePaths.GetSize() > 0) {
			// 有图片：进入编辑模式
			EnterImageEditMode();
		} else {
			// 无图片：直接选择图片
			SelectHBIMImages();
		}
	} else if (ev.GetSource() == &imageDeleteButton) {
		DeleteCurrentHBIMImage();
	} else if (ev.GetSource() == &imagePrevButton) {
		NavigateHBIMImage(false);
	} else if (ev.GetSource() == &imageNextButton) {
		NavigateHBIMImage(true);
	} else if (ev.GetSource() == &imageOKButton) {
		ExitImageEditMode(true);
	} else if (ev.GetSource() == &imageCancelButton) {
		ExitImageEditMode(false);
	}
}