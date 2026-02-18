#include "PluginPalette.hpp"
#include "APICommon.h"
#include "APIdefs.h"
#include "DGModule.hpp"
#include "DGImage.hpp"
#include "ApplicationLauncher.hpp"
#include "GXImage.hpp"
#include "Location.hpp"
#include "FileSystem.hpp"
#include <mutex>
#include <stdio.h>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <random>
#include <cstring>
#include <thread>

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
	
	static GS::UniString NormalizeUniString(const GS::UniString& str) {
		GS::UniString result;
		for (UInt32 i = 0; i < str.GetLength(); ++i) {
			GS::uchar_t ch = str.GetChar(i);
			if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r' && ch != 0xA0) {
				result.Append(ch);
			}
		}
		return result;
	}
	
	// 清理字符串用于文件路径（移除非法字符）
	static GS::UniString SanitizeForFilePath(const GS::UniString& str) {
		GS::UniString result;
		for (UInt32 i = 0; i < str.GetLength(); ++i) {
			GS::uchar_t ch = str.GetChar(i);
			// 允许：字母、数字、下划线、连字符、点
			if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || 
				(ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '.') {
				result.Append(ch);
			} else {
				// 替换其他字符为下划线
				result.Append('_');
			}
		}
		return result;
	}
	
	// 验证和修复UUID格式
	static bool IsValidUUIDFormat(const GS::UniString& uuid) {
		// 标准格式：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (36个字符)
		if (uuid.GetLength() != 36) return false;
		
		for (Int32 i = 0; i < 36; i++) {
			GS::uchar_t ch = uuid.GetChar(i);
			// 检查连字符位置
			if (i == 8 || i == 13 || i == 18 || i == 23) {
				if (ch != '-') return false;
			} else {
				// 必须是十六进制字符
				if (!((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))) {
					return false;
				}
			}
		}
		return true;
	}
	
	// 构建图片的完整路径字符串（仅构造路径，不检查文件存在；用于诊断或解析）
	static bool BuildImageFullPath(const GS::UniString& relativePath, std::string& outFullPath, GS::UniString* outProjectDir = nullptr) {
		if (relativePath.IsEmpty()) return false;
		API_ProjectInfo projectInfo;
		if (ACAPI_ProjectOperation_Project(&projectInfo) != NoError || projectInfo.untitled || !projectInfo.location)
			return false;
		GS::UniString projectPath;
		projectInfo.location->ToPath(&projectPath);
		std::filesystem::path projectFsPath(projectPath.ToCStr().Get());
		std::filesystem::path projectDirPath = projectFsPath.parent_path();
		std::filesystem::path fullPath = projectDirPath / relativePath.ToCStr().Get();
		outFullPath = fullPath.string();
		if (outProjectDir) *outProjectDir = GS::UniString(projectDirPath.string().c_str());
		return true;
	}
	
	// 解析图片路径（相对路径转绝对路径）
	static IO::Location ResolveImagePath(const GS::UniString& relativePath) {
		if (relativePath.IsEmpty()) {
			ACAPI_WriteReport("ResolveImagePath调试: relativePath为空", false);
			return IO::Location();
		}
		
			// 获取项目信息
		API_ProjectInfo projectInfo;
		GSErrCode projectErr = ACAPI_ProjectOperation_Project(&projectInfo);
		if (projectErr != NoError) {
			ACAPI_WriteReport("ResolveImagePath调试: 获取项目信息失败，错误码=%d", false, projectErr);
			return IO::Location();
		}
		
		if (projectInfo.untitled) {
			ACAPI_WriteReport("ResolveImagePath调试: 项目未保存(untitled)", false);
			return IO::Location();
		}
		
		// 获取项目文件夹路径
		IO::Location projectLocation = *projectInfo.location;
		GS::UniString projectPath;
		projectLocation.ToPath(&projectPath);
		
		// 构建完整路径：项目文件夹/相对路径
		std::filesystem::path projectFsPath(projectPath.ToCStr().Get());
		std::filesystem::path projectDirPath = projectFsPath.parent_path();
		
		std::filesystem::path fullPath = projectDirPath / relativePath.ToCStr().Get();
		
	// 检查文件是否存在 - 添加重试机制
	bool fileExists = false;
	int retryCount = 3;
	
	for (int retry = 0; retry < retryCount; retry++) {
		fileExists = std::filesystem::exists(fullPath);
		if (fileExists) {
			break;
		}
		
		if (retry < retryCount - 1) {
			// 等待50ms后重试（文件系统可能延迟）
			// ACAPI_WriteReport("ResolveImagePath调试: 文件不存在，重试 %d/%d (等待50ms)", 
			// 	false, retry + 1, retryCount);
			std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 等待50毫秒
		}
	}
	
	if (!fileExists) {
		ACAPI_WriteReport("ResolveImagePath调试: 文件不存在，完整路径='%s'", 
			false, fullPath.string().c_str());
		
		// 尝试列出父目录以调试
		try {
			std::filesystem::path parentPath = fullPath.parent_path();
			if (std::filesystem::exists(parentPath)) {
				ACAPI_WriteReport("ResolveImagePath调试: 父目录存在，列出内容:", false);
				for (const auto& entry : std::filesystem::directory_iterator(parentPath)) {
					ACAPI_WriteReport("  - %s", false, entry.path().filename().string().c_str());
				}
			} else {
				ACAPI_WriteReport("ResolveImagePath调试: 父目录也不存在: %s", 
					false, parentPath.string().c_str());
			}
		} catch (...) {
			ACAPI_WriteReport("ResolveImagePath调试: 列出目录内容时出错", false);
		}
		
		return IO::Location();
	}
		
		IO::Location location;
		location.Set(fullPath.string().c_str());
		
		// 验证位置对象
		GS::UniString resolvedPath;
		location.ToPath(&resolvedPath);
		// ACAPI_WriteReport("ResolveImagePath调试: 解析后的位置='%s'", false, resolvedPath.ToCStr().Get());
		
		return location;
	}
	
	// 根据相对路径删除磁盘上的图片文件，保持文件夹与属性同步
	static bool DeleteImageFileFromDisk(const GS::UniString& relativePath) {
		if (relativePath.IsEmpty()) return false;
		IO::Location loc = ResolveImagePath(relativePath);
		GS::UniString fullPath;
		loc.ToPath(&fullPath);
		if (fullPath.IsEmpty()) return false;
		try {
			std::filesystem::path p(fullPath.ToCStr().Get());
			if (std::filesystem::exists(p) && std::filesystem::is_regular_file(p)) {
				return std::filesystem::remove(p);
			}
		} catch (...) {}
		return false;
	}
 	
	// 加载并显示图片到PictureItem控件

	
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
		
		// ACAPI_WriteReport("FindOrCreateHBIMGroup: 找到 %d 个属性组", false, (int)groups.GetSize());
		
		// 记录所有属性组名称用于调试
		for (UInt32 i = 0; i < groups.GetSize(); ++i) {
			// ACAPI_WriteReport("  属性组[%d]: %s", false, (int)i, groups[i].name.ToCStr().Get());
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
		
		// 检查错误类型
		// 错误码 -2130312307 = APIERR_NEEDSUNDOSCOPE (需要撤销作用域)
		// 错误码 -2130312988 = APIERR_NAMEALREADYUSED (名称已存在)
		ACAPI_WriteReport("FindOrCreateHBIMGroup: 注意: 错误码 %d (0x%X)", true, err, err);
		if (err == -2130312307) {
			ACAPI_WriteReport("  -> APIERR_NEEDSUNDOSCOPE: 属性组创建需要在ACAPI_CallUndoableCommand作用域内", false);
		} else if (err == -2130312988) {
			ACAPI_WriteReport("  -> APIERR_NAMEALREADYUSED: 属性组名称已存在", false);
		}
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
	

	
	// 复制文件到目标文件夹 (使用IO::Location)
	static bool CopyFileToDestination(const GS::UniString& sourcePath, const IO::Location& destFolderLoc, 
									  const GS::UniString& newFileName, GS::UniString& outDestPath,
									  GS::UniString* outError = nullptr)
	{
		try {
			// 调试：检查输入参数
			GS::UniString destFolderPathStr;
			destFolderLoc.ToPath(&destFolderPathStr);
			
			ACAPI_WriteReport("CopyFileToDestination: 源路径='%s', 目标文件夹='%s', 新文件名='%s'", 
							false, sourcePath.ToCStr().Get(), destFolderPathStr.ToCStr().Get(), newFileName.ToCStr().Get());
			
			// 检查目标文件夹路径字符串是否为空
			if (destFolderPathStr.IsEmpty()) {
				GS::UniString errorMsg = "目标文件夹路径为空";
				ACAPI_WriteReport("CopyFileToDestination: 错误: %s", true, errorMsg.ToCStr().Get());
				if (outError != nullptr) *outError = errorMsg;
				return false;
			}
			
			// 创建目标文件夹（如果不存在）
			std::filesystem::path destFolderPath(destFolderPathStr.ToCStr().Get());
			ACAPI_WriteReport("CopyFileToDestination: 目标文件夹路径='%s'", false, destFolderPath.string().c_str());
			
			if (!std::filesystem::exists(destFolderPath)) {
				ACAPI_WriteReport("CopyFileToDestination: 创建文件夹...", false);
				std::filesystem::create_directories(destFolderPath);
				ACAPI_WriteReport("CopyFileToDestination: 文件夹创建成功", false);
			}
			
			// 检查新文件名是否为空
			if (newFileName.IsEmpty()) {
				GS::UniString errorMsg = "新文件名为空，无法复制文件";
				ACAPI_WriteReport("CopyFileToDestination: 错误: %s", true, errorMsg.ToCStr().Get());
				if (outError != nullptr) *outError = errorMsg;
				return false;
			}
			
			// 构建目标文件路径
			std::filesystem::path destFilePath = destFolderPath / newFileName.ToCStr().Get();
			
			// DG::InformationAlert("调试", 
			// 	GS::UniString::Printf("目标文件路径: '%s'", destFilePath.string().c_str()).ToCStr().Get(), "确定");
			
			// 复制文件
			std::filesystem::path sourceFilePath(sourcePath.ToCStr().Get());
			if (!std::filesystem::exists(sourceFilePath)) {
				GS::UniString errorMsg = GS::UniString::Printf("源文件不存在: %s", sourcePath.ToCStr().Get());
				ACAPI_WriteReport("CopyFileToDestination: %s", true, errorMsg.ToCStr().Get());
				if (outError != nullptr) *outError = errorMsg;
				return false;
			}
			
			std::filesystem::copy_file(sourceFilePath, destFilePath, 
									   std::filesystem::copy_options::overwrite_existing);
			
			outDestPath = GS::UniString(destFilePath.string().c_str());
			return true;
		} catch (const std::filesystem::filesystem_error& e) {
			GS::UniString errorMsg = GS::UniString::Printf("文件复制失败: %s", e.what());
			ACAPI_WriteReport("CopyFileToDestination: %s", true, errorMsg.ToCStr().Get());
			if (outError != nullptr) *outError = errorMsg;
			return false;
		} catch (...) {
			GS::UniString errorMsg = "未知错误";
			ACAPI_WriteReport("CopyFileToDestination: %s", true, errorMsg.ToCStr().Get());
			if (outError != nullptr) *outError = errorMsg;
			return false;
		}
	}
	
	// 创建或获取HBIM图片属性组
	static GSErrCode FindOrCreateHBIMImageGroup(API_PropertyGroup& outGroup)
	{
		// 第一步：获取所有属性组并详细记录
		GS::Array<API_PropertyGroup> groups;
		GSErrCode err = ACAPI_Property_GetPropertyGroups(groups);
		if (err != NoError) {
			ACAPI_WriteReport("FindOrCreateHBIMImageGroup: GetPropertyGroups 失败: %s", true, GS::UniString::Printf("Error %d", err).ToCStr().Get());
			return err;
		}
		
		ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 系统中共有 %d 个属性组", false, (int)groups.GetSize());
		
		// 第二步：详细检查每个属性组，使用多种比较方法
		GS::UniString targetName = kHBIMImageGroupName;
		GS::UniString targetNameNormalized = NormalizeUniString(targetName);
		
		for (UInt32 i = 0; i < groups.GetSize(); ++i) {
			GS::UniString existingName = groups[i].name;
			GS::UniString existingNameNormalized = NormalizeUniString(existingName);
			
			// 记录每个属性组的详细信息
			ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 属性组[%d]: 原始名称='%s', 标准化='%s', guid=%s", 
				false, (int)i, 
				existingName.ToCStr().Get(),
				existingNameNormalized.ToCStr().Get(),
				APIGuidToString(groups[i].guid).ToCStr().Get());
			
			// 方法1：完全匹配原始名称
			if (existingName == targetName) {
				ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 通过完全匹配找到属性组！", false);
				outGroup = groups[i];
				return NoError;
			}
			
			// 方法2：完全匹配标准化名称
			if (existingNameNormalized == targetNameNormalized) {
				ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 通过标准化匹配找到属性组！", false);
				outGroup = groups[i];
				return NoError;
			}
			
			// 方法3：互相包含（宽松匹配）
			if (existingNameNormalized.Contains(targetNameNormalized) || 
				targetNameNormalized.Contains(existingNameNormalized)) {
				ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 通过宽松匹配找到属性组！", false);
				outGroup = groups[i];
				return NoError;
			}
		}
		
		// 第三步：属性组不存在，尝试创建
		ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 属性组不存在，尝试创建...", false);
		outGroup = {};
		outGroup.guid = APINULLGuid;
		outGroup.name = kHBIMImageGroupName;
		outGroup.description = "HBIM构件图片链接";
		err = ACAPI_Property_CreatePropertyGroup(outGroup);
		
		if (err != NoError) {
			// 创建失败，分析错误原因
			ACAPI_WriteReport("FindOrCreateHBIMImageGroup: CreatePropertyGroup 失败: %s (错误码: %d)", true, 
				GS::UniString::Printf("Error %d", err).ToCStr().Get(), err);
			
			// 检查错误类型
			// 错误码 -2130312307 = APIERR_NEEDSUNDOSCOPE (需要撤销作用域)
			// 错误码 -2130312988 = APIERR_NAMEALREADYUSED (名称已存在)
			ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 注意: 错误码 %d (0x%X)", true, err, err);
			if (err == -2130312307) {
				ACAPI_WriteReport("  -> APIERR_NEEDSUNDOSCOPE: 属性组创建需要在ACAPI_CallUndoableCommand作用域内", false);
			} else if (err == -2130312988) {
				ACAPI_WriteReport("  -> APIERR_NAMEALREADYUSED: 属性组名称已存在", false);
			}
			
			// 重新获取属性组列表，尝试再次查找
			ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 创建失败，尝试重新查找现有属性组", false);
			GS::Array<API_PropertyGroup> groups2;
			GSErrCode err2 = ACAPI_Property_GetPropertyGroups(groups2);
			if (err2 == NoError) {
				ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 重新找到 %d 个属性组", false, (int)groups2.GetSize());
				for (UInt32 i = 0; i < groups2.GetSize(); ++i) {
					ACAPI_WriteReport("  重新检查属性组[%d]: '%s'", false, (int)i, groups2[i].name.ToCStr().Get());
					// 尝试更宽松的比较：使用原始名称和标准化名称
					GS::UniString existingName = groups2[i].name;
					GS::UniString existingNameNormalized = NormalizeUniString(existingName);
					
					// 多种匹配方式：完全匹配、标准化匹配、互相包含
					if (existingName == targetName || 
						existingNameNormalized == targetNameNormalized ||
						existingName.Contains(targetName) || 
						targetName.Contains(existingName) ||
						existingNameNormalized.Contains(targetNameNormalized) ||
						targetNameNormalized.Contains(existingNameNormalized)) {
						ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 通过宽松比较找到属性组: '%s' (目标: '%s')", false, existingName.ToCStr().Get(), targetName.ToCStr().Get());
						outGroup = groups2[i];
						return NoError;
					}
				}
			} else {
				ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 重新获取属性组失败: %s", true, GS::UniString::Printf("Error %d", err2).ToCStr().Get());
			}
			
			ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 最终未找到属性组 '%s'", true, kHBIMImageGroupName.ToCStr().Get());
			return err;
		}
		
		// 创建成功
		ACAPI_WriteReport("FindOrCreateHBIMImageGroup: 属性组创建成功！", false);
		return NoError;
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
 

// 加载并显示图片到PictureItem控件
void PluginPalette::LoadAndDisplayImage(const IO::Location& imageLocation, DG::PictureItem& pictureItem) {
	// 防止重入
	if (isLoadingImage) {
		ACAPI_WriteReport("LoadAndDisplayImage: 已在加载中，跳过", false);
		return;
	}
	isLoadingImage = true;
	
	try {
		// 清空现有图片，释放可能锁定的资源
		pictureItem.SetPicture(DG::Picture());
		pictureItem.Redraw();  // 强制重绘以释放资源
		
		// 检查文件是否可访问
		IO::File file(imageLocation, IO::File::Fail);
		GSErrCode openErr = file.Open(IO::File::ReadMode);
		if (openErr == NoError && file.IsOpen()) {
			UInt64 dataLength = 0;
			GSErrCode lengthErr = file.GetDataLength(&dataLength);
			if (lengthErr == NoError) {
				ACAPI_WriteReport("LoadAndDisplayImage: 文件可打开，大小: %llu", false, dataLength);
			} else {
				ACAPI_WriteReport("LoadAndDisplayImage: 无法获取文件大小，错误码: %d", false, lengthErr);
			}
			file.Close();
		} else {
			ACAPI_WriteReport("LoadAndDisplayImage: 文件无法打开，错误码: %d", false, openErr);
			pictureItem.SetPicture(DG::Picture());
			isLoadingImage = false;
			return;
		}
		
		// 使用GX::Image加载图片
		GX::Image img{imageLocation};
		if (img.IsEmpty()) {
			ACAPI_WriteReport("LoadAndDisplayImage: 图片加载失败: img.IsEmpty()", false);
			pictureItem.SetPicture(DG::Picture());
			isLoadingImage = false;
			return;
		}
		
		UInt32 imgW = img.GetWidth();
		UInt32 imgH = img.GetHeight();
		// 使用固定目标尺寸（与.grc中Picture控件 20 380 360 180 一致）
		// 不依赖 GetWidth/GetHeight，因控件在部分时机可能返回0导致显示异常
		const short ctrlW = 360;
		const short ctrlH = 180;
		
		if (imgW == 0 || imgH == 0) {
			ACAPI_WriteReport("LoadAndDisplayImage: 图片尺寸无效", false);
			pictureItem.SetPicture(DG::Picture());
			isLoadingImage = false;
			return;
		}
		
		// 按控件尺寸缩放，保持宽高比
		double scaleW = (double) ctrlW / (double) imgW;
		double scaleH = (double) ctrlH / (double) imgH;
		double scale = (scaleW < scaleH) ? scaleW : scaleH;
		
		UInt32 newW = (UInt32) (imgW * scale);
		UInt32 newH = (UInt32) (imgH * scale);
		if (newW < 1) newW = 1;
		if (newH < 1) newH = 1;
		
		// 转换为NativeImage并调整大小
		NewDisplay::NativeImage nativeImg = img.ToNativeImage(1.0, false);
		NewDisplay::NativeImage resized = nativeImg.Resize(newW, newH);
		
		// 创建GX::Image并立即转换为DG图片数据
		// 关键：确保resized在scaledImg使用期间保持活动
		GX::Image scaledImg(resized);
		
		// 立即转换为DG图片数据并创建DG::Picture
		void* dgData = scaledImg.ToDGPicture();
		if (dgData != nullptr) {
			DG::Picture picture(dgData);
			pictureItem.SetPicture(picture);
			pictureItem.Redraw();
		} else {
			pictureItem.SetPicture(DG::Picture());
		}
		
		isLoadingImage = false;
	} catch (...) {
		ACAPI_WriteReport("LoadAndDisplayImage: 捕获到异常", false);
		pictureItem.SetPicture(DG::Picture());
		isLoadingImage = false;  // 异常情况下也重置标志
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
	, diagnosisButton (GetReference (), DiagnosisButtonId)
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
 	, isUpdatingImages (false)
	, isLoadingImage (false)
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
	
	// 设置下划线：使用 Unicode 横线字符 ─（与区块分隔符一致）
	static const GS::uchar_t kHorizontalLine = 0x2500;  // BOX DRAWINGS LIGHT HORIZONTAL
	GS::UniString underlineText;
	for (int i = 0; i < 48; i++) {
		underlineText.Append(kHorizontalLine);
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
	imageCountLabel.SetText("图片数量: 0");
		imageCountLabel.Show();
		imageCountValue.Hide();   // 占位控件，实际内容在 Label 中
		imageCurrentLabel.SetText("当前图片: 无");
		imageCurrentLabel.Show();
		imageCurrentValue.Hide();  // 占位控件，实际内容在 Label 中
	
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
	diagnosisButton.SetText("诊断");
	diagnosisButton.Show();
	
	// 附加图片按钮观察者
	imageSelectButton.Attach(*this);
	imageDeleteButton.Attach(*this);
	imagePrevButton.Attach(*this);
	imageNextButton.Attach(*this);
	imageOKButton.Attach(*this);
	imageCancelButton.Attach(*this);
	diagnosisButton.Attach(*this);
	
	// 附加图片预览控件观察者，支持点击预览全尺寸图片
	imagePreview.Attach(*this);
	
	imageSelectButton.Enable();
	imageDeleteButton.Disable();
	imagePrevButton.Disable();
	imageNextButton.Disable();
	imageOKButton.Hide();
	imageCancelButton.Hide();
	
	// 初始化图片预览控件 (Picture)
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
		
		// 切换构件时：若在编辑 HBIM 属性或图片，自动保存并退出编辑，与 HBIM 属性逻辑一致
		if (selElemNeig != nullptr && selElemNeig->guid != instance.currentElemGuid) {
			bool wasEditing = instance.isHBIMEditMode || instance.isImageEditMode;
			if (instance.isHBIMEditMode) {
				instance.ExitHBIMEditMode(true);
			}
			if (instance.isImageEditMode) {
				instance.ExitImageEditMode(true);
			}
			if (wasEditing) {
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
			
			// 重置HBIM图片（与 UpdateFromSelection 一致）
			instance.hasHBIMImages = false;
			instance.isImageEditMode = false;
			instance.imagePaths.Clear();
			instance.originalImagePaths.Clear();
			instance.currentImageIndex = 0;
			instance.UpdateHBIMImageUI();
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
	// 防止在CheckHBIMImages中重复调用
	if (isUpdatingImages) {
		// ACAPI_WriteReport("UpdateHBIMImageUI: 已在CheckHBIMImages中更新，跳过", false);
		return;
	}
	
	// 更新图片计数和当前图片显示（使用 Append 避免 Printf 中文编码问题）
	if (hasHBIMImages && imagePaths.GetSize() > 0) {
		// 先清空预览，避免残留上一张图片导致“删除后偶尔显示”的错觉
		imagePreview.SetPicture(DG::Picture());
		imagePreview.Redraw();
		
		GS::UniString countLabelText;
		countLabelText.Append("图片数量: ");
		countLabelText.Append(GS::ValueToUniString(static_cast<Int32>(imagePaths.GetSize())));
		imageCountLabel.SetText(countLabelText);
		imageCountLabel.Redraw();
		
		if (currentImageIndex < imagePaths.GetSize()) {
			GS::UniString currentLabelText;
			currentLabelText.Append("当前图片: ");
			currentLabelText.Append(GS::ValueToUniString(static_cast<Int32>(currentImageIndex + 1)));
			currentLabelText.Append("/");
			currentLabelText.Append(GS::ValueToUniString(static_cast<Int32>(imagePaths.GetSize())));
			imageCurrentLabel.SetText(currentLabelText);
			imageCurrentLabel.Redraw();
			
			// 显示当前图片
			GS::UniString currentImagePath = imagePaths[currentImageIndex];
			// ACAPI_WriteReport("UpdateHBIMImageUI: 当前图片路径: %s", false, currentImagePath.ToCStr().Get());
			IO::Location imageLocation = ResolveImagePath(currentImagePath);
			
			// 调试：检查解析后的路径
			GS::UniString resolvedPath;
			imageLocation.ToPath(&resolvedPath);
			// ACAPI_WriteReport("UpdateHBIMImageUI: 解析后的完整路径: %s (长度=%d)", false, resolvedPath.ToCStr().Get(), resolvedPath.GetLength());
			
			// 检查路径是否有效
			if (resolvedPath.IsEmpty()) {
				ACAPI_WriteReport("UpdateHBIMImageUI: 错误: ResolveImagePath返回空路径，无法加载图片", false);
				imagePreview.SetPicture(DG::Picture());
				imagePreview.Redraw();
			} else {
				// 检查文件是否存在
				bool fileExists = std::filesystem::exists(std::filesystem::path(resolvedPath.ToCStr().Get()));
				// ACAPI_WriteReport("UpdateHBIMImageUI: 文件存在检查: %s", false, fileExists ? "存在" : "不存在");
				
				if (!fileExists) {
					ACAPI_WriteReport("UpdateHBIMImageUI: 警告: 图片文件不存在: %s", false, resolvedPath.ToCStr().Get());
					imagePreview.SetPicture(DG::Picture());
					imagePreview.Redraw();
				} else {
					LoadAndDisplayImage(imageLocation, imagePreview);
					// ACAPI_WriteReport("UpdateHBIMImageUI: LoadAndDisplayImage调用完成", false);
				}
			}
		} else {
			ACAPI_WriteReport("UpdateHBIMImageUI: currentImageIndex超出范围", false);
			imagePreview.SetPicture(DG::Picture());
			imagePreview.Redraw();
		}
	} else {
		imageCountLabel.SetText("图片数量: 0");
		imageCurrentLabel.SetText("当前图片: 无");
		imageCountLabel.Redraw();
		imageCurrentLabel.Redraw();
		
		// 没有图片时显示空图片
		imagePreview.SetPicture(DG::Picture());
		imagePreview.Redraw();
	}
	
	// 根据编辑模式更新按钮状态
	if (isImageEditMode) {
		// 编辑模式：显示确定、取消、添加图片按钮
		imageSelectButton.SetText("添加图片");
		imageSelectButton.Show();
		imageOKButton.Show();
		imageCancelButton.Show();
		
		// 在编辑模式下启用删除、导航、添加按钮
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
			
			// 非编辑模式：允许上一张/下一张浏览，但禁止删除
			imageDeleteButton.Disable();
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
			
			imageDeleteButton.Disable();
			imagePrevButton.Disable();
			imageNextButton.Disable();
		}
	}
	
	// 重绘所有控件
	imageCountLabel.Redraw();
	imageCurrentLabel.Redraw();
	imageSelectButton.Redraw();
	imageDeleteButton.Redraw();
	imagePrevButton.Redraw();
	imageNextButton.Redraw();
	imageOKButton.Redraw();
	imageCancelButton.Redraw();
	imagePreview.Redraw();
	
	// 强制重绘整个面板以确保UI更新
	Redraw();
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
	ACAPI_WriteReport("ExitImageEditMode: 开始 save=%d, hasHBIMImages=%d, imagePaths=%d", false, save ? 1 : 0, hasHBIMImages ? 1 : 0, (int)imagePaths.GetSize());
	if (!isImageEditMode) {
		ACAPI_WriteReport("ExitImageEditMode: 不在图片编辑模式，直接返回", false);
		return;
	}
	
	if (save) {
		// 保存更改：将当前图片路径保存到属性
		if (currentElemGuid != APINULLGuid) {
			// 确保HBIM图片属性组和定义存在
			API_Guid imageGroupGuid, imageLinksGuid;
			GSErrCode err = ACAPI_CallUndoableCommand("创建HBIM图片属性定义",
				[&]() -> GSErrCode {
					return EnsureHBIMImagePropertyGroupAndDefinitions(imageGroupGuid, imageLinksGuid);
				}
			);
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
					
					// 保存到属性（在撤销命令中）
					err = ACAPI_CallUndoableCommand("保存HBIM图片链接属性",
						[&]() -> GSErrCode {
							return SetHBIMImageLinksPropertyValue(currentElemGuid, imageLinksGuid, jsonArray);
						}
					);
					if (err != NoError) {
						DG::InformationAlert("警告", "无法保存图片链接到属性", "确定");
					}
				} else {
					// 如果没有图片，保存空数组（在撤销命令中）
					ACAPI_CallUndoableCommand("保存HBIM图片链接属性",
						[&]() -> GSErrCode {
							return SetHBIMImageLinksPropertyValue(currentElemGuid, imageLinksGuid, "[]");
						}
					);
				}
				
				// 更新状态
				hasHBIMImages = (imagePaths.GetSize() > 0);
				currentImageIndex = 0;
				ACAPI_WriteReport("ExitImageEditMode: 保存完成，hasHBIMImages=%d, imagePaths=%d", false, hasHBIMImages ? 1 : 0, (int)imagePaths.GetSize());
			}
		}
	} else {
		// 取消编辑：删除本次新增的图片文件（imagePaths 中不在 originalImagePaths 里的），再恢复
		for (UInt32 i = 0; i < imagePaths.GetSize(); ++i) {
			bool wasOriginal = false;
			for (UInt32 j = 0; j < originalImagePaths.GetSize(); ++j) {
				if (imagePaths[i] == originalImagePaths[j]) { wasOriginal = true; break; }
			}
			if (!wasOriginal) {
				DeleteImageFileFromDisk(imagePaths[i]);
			}
		}
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
	if (isUpdatingImages) {
		return;
	}
	
	isUpdatingImages = true;
	
	hasHBIMImages = false;
	imagePaths.Clear();
	currentImageIndex = 0;
	
	if (projectHash.IsEmpty()) {
		projectHash = CalculateProjectHash();
	}
	
	if (currentElemGuid == APINULLGuid) {
		isUpdatingImages = false;
		UpdateHBIMImageUI();
		return;
	}
	
	API_Guid imageGroupGuid, imageLinksGuid;
	// 新建项目时属性组可能不存在，需在撤销作用域内创建（APIERR_NEEDSUNDOSCOPE）
	GSErrCode err = ACAPI_CallUndoableCommand("初始化HBIM构件图片属性",
		[&imageGroupGuid, &imageLinksGuid]() -> GSErrCode {
			return EnsureHBIMImagePropertyGroupAndDefinitions(imageGroupGuid, imageLinksGuid);
		}
	);
	
	if (err != NoError) {
		ACAPI_WriteReport("CheckHBIMImages: 获取/创建属性组失败，错误码=%d", false, err);
		isUpdatingImages = false;
		UpdateHBIMImageUI();
		return;
	}
	
	GS::UniString imageLinksJson;
	err = GetHBIMImageLinksPropertyValue(currentElemGuid, imageLinksGuid, imageLinksJson);
	
	if (err != NoError || imageLinksJson == "[]" || imageLinksJson.GetLength() <= 2) {
		ACAPI_WriteReport("CheckHBIMImages: 读取属性失败或为空，错误码=%d，内容='%s'", 
			false, err, imageLinksJson.ToCStr().Get());
		isUpdatingImages = false;
		UpdateHBIMImageUI();
		return;
	}
	
	// 解析 JSON 数组 - 简单手动解析
	GS::UniString trimmed = imageLinksJson;
	trimmed.DeleteFirst();
	trimmed.DeleteLast();
	
	// 手动解析逗号分隔的带引号的字符串
	UInt32 startPos = 0;
	int parsedCount = 0;
	while (startPos < trimmed.GetLength()) {
		// 查找引号开始位置
		UIndex quoteStart = trimmed.FindFirst('"', startPos);
		if (quoteStart == MaxUIndex) break;
		
		// 查找引号结束位置
		UIndex quoteEnd = trimmed.FindFirst('"', quoteStart + 1);
		if (quoteEnd == MaxUIndex) break;
		
		// 提取引号内的内容（GetSubstring第二参数是长度length，不是end索引）
		USize pathLen = (quoteEnd > quoteStart + 1) ? (quoteEnd - quoteStart - 1) : 0;
		GS::UniString path = trimmed.GetSubstring(quoteStart + 1, pathLen);
		if (!path.IsEmpty()) {
			imagePaths.Push(path);
			parsedCount++;
		}
		
		startPos = quoteEnd + 1;
	}
	
	(void)parsedCount;
	
	// 更新状态
	hasHBIMImages = (imagePaths.GetSize() > 0);
	ACAPI_WriteReport("CheckHBIMImages: 解析完成，imagePaths=%d, hasHBIMImages=%d", false, (int)imagePaths.GetSize(), hasHBIMImages ? 1 : 0);
	isUpdatingImages = false;
	UpdateHBIMImageUI();
}

void PluginPalette::SelectHBIMImages ()
{
	if (currentElemGuid == APINULLGuid) {
		DG::InformationAlert("提示", "请先选择一个构件", "确定");
		return;
	}
	
	// 使用DG::FileDialog选择多个图片文件
	ACAPI_WriteReport("SelectHBIMImages: 创建文件对话框", false);
	DG::FileDialog dlg(DG::FileDialog::OpenMultiFile);
	FTM::FileTypeManager mgr("HBIMComponentEntryImages");
	FTM::FileType typeJpg("JPEG", "jpg", 0, 0, 0);
	FTM::FileType typePng("PNG", "png", 0, 0, 0);
	FTM::TypeID idJpg = mgr.AddType(typeJpg);
	FTM::TypeID idPng = mgr.AddType(typePng);
	dlg.AddFilter(idJpg);
	dlg.AddFilter(idPng);
	dlg.SetTitle("选择HBIM构件图片");
	
	ACAPI_WriteReport("SelectHBIMImages: 调用文件对话框", false);
	ACAPI_WriteReport("SelectHBIMImages: 即将弹出文件选择对话框", false);
	bool dialogResult = dlg.Invoke();
	ACAPI_WriteReport(GS::UniString::Printf("SelectHBIMImages: 文件对话框返回: %s", dialogResult ? "true (选择了文件)" : "false (取消或无文件)").ToCStr().Get(), false);
	if (dialogResult) {
		ACAPI_WriteReport("SelectHBIMImages: 文件对话框成功返回，选择了文件", false);
		// 确保图片文件夹存在
		ACAPI_WriteReport("SelectHBIMImages: 调用EnsureHBIMImageFolder", false);
		GSErrCode err = EnsureHBIMImageFolder();
		ACAPI_WriteReport("SelectHBIMImages: EnsureHBIMImageFolder返回: %d", false, err);
		if (err != NoError) {
			DG::InformationAlert("错误", GS::UniString::Printf("无法创建图片文件夹 (错误码: %d)，请确保项目已保存", err).ToCStr().Get(), "确定");
			ACAPI_WriteReport("SelectHBIMImages: EnsureHBIMImageFolder失败，退出", true);
			return;
		}
		ACAPI_WriteReport("SelectHBIMImages: 图片文件夹检查通过", false);
		ACAPI_WriteReport("SelectHBIMImages: 进度: 图片文件夹检查通过，正在获取构件信息...", false);
		
		// 获取构件GlobalId
		ACAPI_WriteReport("SelectHBIMImages: 调用GetGlobalIdForElement", false);
		GS::UniString globalId = GetGlobalIdForElement(currentElemGuid);
		ACAPI_WriteReport("SelectHBIMImages: globalId='%s'(长度=%d), projectHash='%s'(长度=%d)", false, 
						globalId.ToCStr().Get(), globalId.GetLength(), 
						projectHash.ToCStr().Get(), projectHash.GetLength());
		if (globalId.IsEmpty() || globalId == "未找到") {
			DG::InformationAlert("错误", "无法获取构件GlobalId (IFC属性可能未启用)", "确定");
			ACAPI_WriteReport("SelectHBIMImages: GlobalId无效，退出", true);
			return;
		}
		ACAPI_WriteReport("SelectHBIMImages: GlobalId获取成功", false);
		ACAPI_WriteReport("SelectHBIMImages: 进度: 构件GlobalId获取成功: %s", false, globalId.ToCStr().Get());
		
		// 确保HBIM图片属性组和定义存在
		API_Guid imageGroupGuid, imageLinksGuid;
		err = ACAPI_CallUndoableCommand("创建HBIM图片属性定义",
			[&]() -> GSErrCode {
				return EnsureHBIMImagePropertyGroupAndDefinitions(imageGroupGuid, imageLinksGuid);
			}
		);
		if (err != NoError) {
			DG::InformationAlert("错误", GS::UniString::Printf("无法创建图片属性定义 (错误码: %d)", err).ToCStr().Get(), "确定");
			return;
		}
		ACAPI_WriteReport("SelectHBIMImages: 进度: 图片属性定义创建成功", false);
		
		// 确定现有图片路径的基准：编辑模式下用当前imagePaths，否则从属性读取
		GS::Array<GS::UniString> existingImagePaths;
		if (isImageEditMode) {
			existingImagePaths = imagePaths;  // 编辑模式：保留当前状态（含本回合删除等）
		} else {
			GS::UniString existingImageLinksJson;
			GetHBIMImageLinksPropertyValue(currentElemGuid, imageLinksGuid, existingImageLinksJson);
			if (existingImageLinksJson != "[]" && existingImageLinksJson.GetLength() > 2) {
				GS::UniString trimmed = existingImageLinksJson;
				trimmed.DeleteFirst();
				trimmed.DeleteLast();
				UInt32 startPos = 0;
				while (startPos < trimmed.GetLength()) {
					UIndex quoteStart = trimmed.FindFirst('"', startPos);
					if (quoteStart == MaxUIndex) break;
					UIndex quoteEnd = trimmed.FindFirst('"', quoteStart + 1);
					if (quoteEnd == MaxUIndex) break;
					USize pathLen = (quoteEnd > quoteStart + 1) ? (quoteEnd - quoteStart - 1) : 0;
					GS::UniString path = trimmed.GetSubstring(quoteStart + 1, pathLen);
					if (!path.IsEmpty()) {
						existingImagePaths.Push(path);
					}
					startPos = quoteEnd + 1;
				}
			}
		}
		
		imagePaths = existingImagePaths;
		
		// 获取选中的文件并复制到项目文件夹
		ACAPI_WriteReport("SelectHBIMImages: 获取文件选择数量", false);
		USize n = dlg.GetSelectionCount();
		ACAPI_WriteReport("SelectHBIMImages: 选择了 %d 个文件", false, n);
		for (UIndex i = 0; i < n; ++i) {
			ACAPI_WriteReport("SelectHBIMImages: 处理第 %d 个文件", false, i + 1);
			IO::Location fileLocation = dlg.GetSelectedFile(i);
			GS::UniString sourcePath;
			fileLocation.ToPath(&sourcePath);
			
			// 生成新的文件名：timestamp_originalName.ext
			GS::UniString timestamp = GetCurrentTimestamp();
			GS::UniString originalFileName = GetFileNameFromPath(sourcePath);
			
			// 调试：显示文件名生成信息
			ACAPI_WriteReport("SelectHBIMImages: 文件名调试 - timestamp='%s' (长度=%d), originalFileName='%s' (长度=%d), sourcePath='%s'", 
				false, timestamp.ToCStr().Get(), timestamp.GetLength(),
				originalFileName.ToCStr().Get(), originalFileName.GetLength(),
				sourcePath.ToCStr().Get());
			
			// 检查时间戳和文件名是否有效
			if (timestamp.IsEmpty()) {
				timestamp = "unknown_time";
				ACAPI_WriteReport("警告: 时间戳为空，使用默认值", true);
			}
			if (originalFileName.IsEmpty()) {
				originalFileName = "unknown_file";
				ACAPI_WriteReport("警告: 文件名为空，使用默认值", true);
			}
			
			GS::UniString newFileName = timestamp + "_" + originalFileName;
			
			// 调试：显示生成的文件名
			ACAPI_WriteReport("SelectHBIMImages: 文件名调试 - newFileName='%s' (长度=%d)", 
				false, newFileName.ToCStr().Get(), newFileName.GetLength());
			
			// 验证newFileName不为空
			if (newFileName.IsEmpty()) {
				DG::InformationAlert("错误", "生成的文件名为空，无法复制文件", "确定");
				ACAPI_WriteReport("错误: 生成的文件名为空", true);
				continue; // 跳过此文件，继续处理下一个
			}
			
			// 构建目标文件夹路径：HBIM_Images_{projectHash}/{elementGlobalId}/
			// 清理globalId用于文件路径
			GS::UniString cleanGlobalId = SanitizeForFilePath(globalId);
			
			// 调试：显示清理后的globalId
			ACAPI_WriteReport("SelectHBIMImages: 调试: globalId清理后: cleanGlobalId='%s' (长度=%d)", false, 
					cleanGlobalId.ToCStr().Get(), cleanGlobalId.GetLength());
			
			if (cleanGlobalId.IsEmpty() || cleanGlobalId == "未找到" || cleanGlobalId == "___") {
				DG::InformationAlert("错误", "构件GlobalId无效，无法创建图片文件夹", "确定");
				return;
			}
			
			// 验证projectHash格式
			if (!IsValidUUIDFormat(projectHash) && projectHash != "unsaved_project" && projectHash != "unknown") {
				ACAPI_WriteReport("SelectHBIMImages: 警告: projectHash格式异常: %s", true, projectHash.ToCStr().Get());
			}
			
			// 构建目标文件夹名称：HBIM_Images_{projectHash}/{elementGlobalId}/
			// 调试：显示projectHash的原始值和清理前的状态
			ACAPI_WriteReport("SelectHBIMImages: 调试: projectHash清理前: 原始='%s' (长度=%d)", false, 
					projectHash.ToCStr().Get(), projectHash.GetLength());
			
			GS::UniString cleanProjectHash = SanitizeForFilePath(projectHash);
			
			// 调试：显示清理后的projectHash
			ACAPI_WriteReport("SelectHBIMImages: 调试: projectHash清理后: cleanProjectHash='%s' (长度=%d)", false, 
					cleanProjectHash.ToCStr().Get(), cleanProjectHash.GetLength());
			
			if (cleanProjectHash != projectHash) {
				ACAPI_WriteReport("SelectHBIMImages: projectHash已清理: %s -> %s", false, 
								projectHash.ToCStr().Get(), cleanProjectHash.ToCStr().Get());
			}
			
			// 检查cleanProjectHash是否为空（在构建destFolderName之前）
			if (cleanProjectHash.IsEmpty()) {
				DG::InformationAlert("错误", 
					GS::UniString::Printf("项目Hash无效，无法创建图片文件夹\n原始projectHash='%s' (长度=%d)", 
						projectHash.ToCStr().Get(), projectHash.GetLength()).ToCStr().Get(), "确定");
				ACAPI_WriteReport("SelectHBIMImages: 错误: cleanProjectHash为空，projectHash='%s'", true, projectHash.ToCStr().Get());
				return;
			}
			
			// 检查cleanGlobalId是否为空（已经检查过，但再次确认）
			if (cleanGlobalId.IsEmpty()) {
				DG::InformationAlert("错误", 
					GS::UniString::Printf("构件GlobalId无效，无法创建图片文件夹\n原始globalId='%s' (长度=%d)", 
						globalId.ToCStr().Get(), globalId.GetLength()).ToCStr().Get(), "确定");
				ACAPI_WriteReport("SelectHBIMImages: 错误: cleanGlobalId为空，globalId='%s'", true, globalId.ToCStr().Get());
				return;
			}
			
			// 调试：检查清理后的字符串
			ACAPI_WriteReport("SelectHBIMImages: 调试: cleanProjectHash长度=%d, cleanGlobalId长度=%d", false, 
							cleanProjectHash.GetLength(), cleanGlobalId.GetLength());
			ACAPI_WriteReport("SelectHBIMImages: 调试: cleanProjectHash内容='%s'", false, cleanProjectHash.ToCStr().Get());
			ACAPI_WriteReport("SelectHBIMImages: 调试: cleanGlobalId内容='%s'", false, cleanGlobalId.ToCStr().Get());
			
			// 使用Append方法构建destFolderName，避免Printf问题
			GS::UniString destFolderName;
			destFolderName.Append("HBIM_Images_");
			destFolderName.Append(cleanProjectHash);
			destFolderName.Append("/");
			destFolderName.Append(cleanGlobalId);
			
			// 调试：显示构建的destFolderName
			// DG::InformationAlert("调试", 
			// 	GS::UniString::Printf("destFolderName构建后: '%s' (长度=%d)", 
			// 		destFolderName.ToCStr().Get(), destFolderName.GetLength()).ToCStr().Get(), "确定");
			
			if (destFolderName.IsEmpty()) {
				DG::InformationAlert("错误", 
					GS::UniString::Printf("目标文件夹路径为空\ncleanProjectHash='%s'(长度=%d)\ncleanGlobalId='%s'(长度=%d)", 
						cleanProjectHash.ToCStr().Get(), cleanProjectHash.GetLength(), 
						cleanGlobalId.ToCStr().Get(), cleanGlobalId.GetLength()).ToCStr().Get(), "确定");
				ACAPI_WriteReport("SelectHBIMImages: 错误: destFolderName为空，cleanProjectHash='%s'(长度=%d), cleanGlobalId='%s'(长度=%d)", true, 
								cleanProjectHash.ToCStr().Get(), cleanProjectHash.GetLength(), 
								cleanGlobalId.ToCStr().Get(), cleanGlobalId.GetLength());
				return;
			}
			ACAPI_WriteReport("SelectHBIMImages: destFolderName='%s' (长度=%d)", false, 
							destFolderName.ToCStr().Get(), destFolderName.GetLength());
			
			// 构建项目文件夹路径
			// 调试：开始检查项目保存状态
			// DG::InformationAlert("调试", "正在检查项目保存状态...", "确定");
			
			API_ProjectInfo projectInfo;
			GSErrCode projectErr = ACAPI_ProjectOperation_Project(&projectInfo);
			
			// 调试：显示项目检查结果
			// DG::InformationAlert("调试", 
			// 	GS::UniString::Printf("项目检查结果: 错误码=%d, untitled=%d", 
			// 		projectErr, projectInfo.untitled ? 1 : 0).ToCStr().Get(), "确定");
			
			if (projectErr != NoError || projectInfo.untitled) {
				DG::InformationAlert("错误", "项目未保存，无法复制图片", "确定");
				return;
			}

			// 调试：在获取项目路径前检查location是否为空
			// DG::InformationAlert("调试", 
			// 	GS::UniString::Printf("projectInfo.location指针检查: %s", 
			// 		projectInfo.location ? "非空" : "为空NULL!").ToCStr().Get(), "确定");
			
			IO::Location projectLocation = *projectInfo.location;
			GS::UniString projectFilePath;
			projectLocation.ToPath(&projectFilePath);
			
			// DG::InformationAlert("调试", 
			// 	GS::UniString::Printf("项目文件路径获取成功: '%s' (长度=%d)", 
			// 		projectFilePath.ToCStr().Get(), projectFilePath.GetLength()).ToCStr().Get(), "确定");
			
			ACAPI_WriteReport("SelectHBIMImages: 项目文件路径='%s'", false, projectFilePath.ToCStr().Get());
			ACAPI_WriteReport("SelectHBIMImages: destFolderName='%s', projectHash='%s', globalId='%s'", false, 
							destFolderName.ToCStr().Get(), projectHash.ToCStr().Get(), globalId.ToCStr().Get());
			
			// 使用与EnsureHBIMImageFolder相同的逻辑构建目标文件夹路径
			// 获取项目文件所在目录（父目录）
			std::filesystem::path projectFsPath(projectFilePath.ToCStr().Get());
			std::filesystem::path projectDirPath = projectFsPath.parent_path();
			ACAPI_WriteReport("SelectHBIMImages: 项目目录='%s'", false, projectDirPath.string().c_str());
			// 改用 std::string 解析路径，避免 GS::UniString API 问题
			std::string destFolderNameStr(destFolderName.ToCStr().Get());
			std::filesystem::path fullDestFolderPath = projectDirPath;
			
			size_t slashPos = destFolderNameStr.find('/');
			if (slashPos == std::string::npos) {
				fullDestFolderPath /= destFolderNameStr;
			} else {
				std::string part1 = destFolderNameStr.substr(0, slashPos);
				std::string part2 = destFolderNameStr.substr(slashPos + 1);
				fullDestFolderPath /= part1;
				fullDestFolderPath /= part2;
			}
			
			// 将std::filesystem路径转换为IO::Location
			IO::Location destFolderLocation;
			destFolderLocation.Set(fullDestFolderPath.string().c_str());
			
			GS::UniString fullDestFolder;
			destFolderLocation.ToPath(&fullDestFolder);
			ACAPI_WriteReport("SelectHBIMImages: 完整目标文件夹路径='%s'", false, fullDestFolder.ToCStr().Get());
			
			// 复制文件到目标文件夹
			GS::UniString destPath;
			GS::UniString copyError;
			bool copyResult = CopyFileToDestination(sourcePath, destFolderLocation, newFileName, destPath, &copyError);
			ACAPI_WriteReport("SelectHBIMImages: CopyFileToDestination返回: %s", false, copyResult ? "成功" : "失败");
			if (copyResult) {
				// 添加相对路径到图片路径列表
				GS::UniString relativePath;
				relativePath.Append(destFolderName);
				relativePath.Append("/");
				relativePath.Append(newFileName);
				ACAPI_WriteReport("SelectHBIMImages: relativePath='%s'", false, relativePath.ToCStr().Get());
				imagePaths.Push(relativePath);
				ACAPI_WriteReport("SelectHBIMImages: 已添加到imagePaths，当前数量: %d", false, imagePaths.GetSize());
			} else {
				ACAPI_WriteReport("SelectHBIMImages: 文件复制失败", true);
				DG::InformationAlert("错误", GS::UniString::Printf("无法复制文件 %s: %s", originalFileName.ToCStr().Get(), copyError.ToCStr().Get()).ToCStr().Get(), "确定");
			}
		}
		
		// 选择完成后：若未在编辑模式则进入编辑模式（流程1：选择→复制→进入编辑→确定才保存）
		if (!isImageEditMode && imagePaths.GetSize() > 0) {
			EnterImageEditMode();
		}
		// 不在此处保存到属性，统一在用户点击「确定」时由 ExitImageEditMode 保存
		
		// 更新状态
		ACAPI_WriteReport("SelectHBIMImages: 更新状态，imagePaths数量: %d", false, imagePaths.GetSize());
		hasHBIMImages = (imagePaths.GetSize() > 0);
		currentImageIndex = 0;
		isUpdatingImages = false;
		
		UpdateHBIMImageUI();
		ACAPI_WriteReport("SelectHBIMImages: 函数结束，UI已更新", false);
	} else {
		ACAPI_WriteReport("SelectHBIMImages: 文件对话框取消或无文件选择", false);
		ACAPI_WriteReport("SelectHBIMImages: 提示: 文件选择已取消或未选择文件", false);
	}
	ACAPI_WriteReport("SelectHBIMImages: 函数完全结束", false);
	ACAPI_WriteReport("SelectHBIMImages: 函数执行完成", false);
}

void PluginPalette::DeleteCurrentHBIMImage ()
{
	if (!hasHBIMImages || imagePaths.GetSize() == 0 || currentImageIndex >= imagePaths.GetSize()) {
		return;
	}
	
	// 先删除磁盘上的文件，再移除链接
	GS::UniString pathToDelete = imagePaths[currentImageIndex];
	DeleteImageFileFromDisk(pathToDelete);
	
	imagePaths.Delete(currentImageIndex);
	
	// 根据是否在编辑模式决定是否保存到属性
	if (!isImageEditMode) {
		// 不在编辑模式：直接保存到属性
		API_Guid imageGroupGuid, imageLinksGuid;
		GSErrCode err = ACAPI_CallUndoableCommand("创建HBIM图片属性定义",
			[&]() -> GSErrCode {
				return EnsureHBIMImagePropertyGroupAndDefinitions(imageGroupGuid, imageLinksGuid);
			}
		);
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
			
			// 保存到属性（在撤销命令中）
			ACAPI_CallUndoableCommand("保存HBIM图片链接属性",
				[&]() -> GSErrCode {
					return SetHBIMImageLinksPropertyValue(currentElemGuid, imageLinksGuid, jsonArray);
				}
			);
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
	
	// 计算项目哈希（用于唯一标识项目）
	projectHash = CalculateProjectHash();
	ACAPI_WriteReport("EnsureHBIMImageFolder: projectHash='%s'", false, projectHash.ToCStr().Get());
	
	// 检查项目是否已保存
	if (projectInfo.untitled) {
		ACAPI_WriteReport("EnsureHBIMImageFolder: 项目未保存，无法创建图片文件夹", false);
		return APIERR_GENERAL;
	}
	
	// 获取项目文件夹路径
	IO::Location projectLocation = *projectInfo.location;
	GS::UniString projectPath;
	projectLocation.ToPath(&projectPath);
	
	ACAPI_WriteReport("EnsureHBIMImageFolder: 项目文件路径='%s'", false, projectPath.ToCStr().Get());
	
	// 使用std::filesystem获取项目文件所在目录
	std::filesystem::path projectFilePath(projectPath.ToCStr().Get());
	std::filesystem::path projectDirPath = projectFilePath.parent_path();
	
	// 创建HBIM_Images_{projectHash}文件夹
	GS::UniString hbimImagesPath = GS::UniString(projectDirPath.string().c_str());
	hbimImagesPath.Append("/HBIM_Images_");
	hbimImagesPath.Append(projectHash);
	
	ACAPI_WriteReport("EnsureHBIMImageFolder: 目标文件夹路径='%s'", false, hbimImagesPath.ToCStr().Get());
	
	// 检查文件夹是否存在，不存在则创建
	try {
		std::filesystem::path folderPath(hbimImagesPath.ToCStr().Get());
		if (!std::filesystem::exists(folderPath)) {
			std::filesystem::create_directories(folderPath);
			ACAPI_WriteReport("EnsureHBIMImageFolder: 创建文件夹: %s", false, hbimImagesPath.ToCStr().Get());
		} else {
			ACAPI_WriteReport("EnsureHBIMImageFolder: 文件夹已存在: %s", false, hbimImagesPath.ToCStr().Get());
		}
	} catch (const std::filesystem::filesystem_error& e) {
		ACAPI_WriteReport("EnsureHBIMImageFolder: 创建文件夹失败: %s", true, e.what());
		return APIERR_GENERAL;
	} catch (...) {
		ACAPI_WriteReport("EnsureHBIMImageFolder: 未知错误", true);
		return APIERR_GENERAL;
	}
	
	return NoError;
}

namespace {
	// 生成随机UUID字符串 (格式: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx)
	static GS::UniString GenerateUUID()
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, 15);
		
		const char* hexChars = "0123456789ABCDEF";
		char uuid[37];
		
		// 生成32个十六进制字符
		for (int i = 0; i < 32; i++) {
			uuid[i] = hexChars[dis(gen)];
		}
		
		// 直接构建格式化字符串：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
		char formatted[37];
		// 第一部分：8个字符
		std::memcpy(formatted, uuid, 8);
		formatted[8] = '-';
		// 第二部分：4个字符
		std::memcpy(formatted + 9, uuid + 8, 4);
		formatted[13] = '-';
		// 第三部分：4个字符
		std::memcpy(formatted + 14, uuid + 12, 4);
		formatted[18] = '-';
		// 第四部分：4个字符
		std::memcpy(formatted + 19, uuid + 16, 4);
		formatted[23] = '-';
		// 第五部分：12个字符
		std::memcpy(formatted + 24, uuid + 20, 12);
		formatted[36] = '\0';
		
		return GS::UniString(formatted);
	}
	
	// 获取当前项目文件完整路径（用于区分不同项目）
	static GS::UniString GetCurrentProjectPath()
	{
		API_ProjectInfo projectInfo;
		if (ACAPI_ProjectOperation_Project(&projectInfo) != NoError || projectInfo.untitled || !projectInfo.location)
			return GS::UniString();
		GS::UniString path;
		projectInfo.location->ToPath(&path);
		return path;
	}
	
	// Preferences 版本2：存储 uuid + path，用于检测"另存为"副本
	// 数据格式: [version=2][uuidLen][uuid][pathLen][path]
	const Int32 kPreferencesVersionV2 = 2;
	static const size_t kMaxPrefDataSize = 600;
	
	static bool ReadProjectMappingFromPreferences(GS::UniString& outUuid, GS::UniString& outPath)
	{
		Int32 version = 0;
		GSSize bytes = 0;
		GSErrCode err = ACAPI_GetPreferences(&version, &bytes, nullptr);
		if (err != NoError || version != kPreferencesVersionV2 || bytes < 8 || bytes > kMaxPrefDataSize)
			return false;
		char* data = new char[bytes];
		err = ACAPI_GetPreferences(&version, &bytes, data);
		if (err != NoError) {
			delete[] data;
			return false;
		}
		Int32 uuidLen = *reinterpret_cast<Int32*>(data);
		if (uuidLen <= 0 || uuidLen > 40 || bytes < 8 + uuidLen) {
			delete[] data;
			return false;
		}
		outUuid = GS::UniString(data + 4, uuidLen);
		Int32 pathLen = *reinterpret_cast<Int32*>(data + 4 + uuidLen);
		if (pathLen < 0 || pathLen > 400 || bytes < 8 + uuidLen + pathLen) {
			delete[] data;
			return false;
		}
		if (pathLen > 0)
			outPath = GS::UniString(data + 8 + uuidLen, pathLen);
		delete[] data;
		return true;
	}
	
	static GSErrCode SaveProjectMappingToPreferences(const GS::UniString& uuid, const GS::UniString& path)
	{
		Int32 uuidLen = static_cast<Int32>(uuid.GetLength());
		Int32 pathLen = static_cast<Int32>(path.GetLength());
		if (8 + uuidLen + pathLen > static_cast<GSSize>(kMaxPrefDataSize))
			return APIERR_GENERAL;
		char data[kMaxPrefDataSize];
		*reinterpret_cast<Int32*>(data) = uuidLen;
		std::memcpy(data + 4, uuid.ToCStr().Get(), uuidLen);
		*reinterpret_cast<Int32*>(data + 4 + uuidLen) = pathLen;
		if (pathLen > 0)
			std::memcpy(data + 8 + uuidLen, path.ToCStr().Get(), pathLen);
		return ACAPI_SetPreferences(kPreferencesVersionV2, 8 + uuidLen + pathLen, data);
	}
	
	// 从项目信息读取项目UUID (使用keywords字段)
	static GS::UniString ReadProjectUUIDFromProjectNotes()
	{
		API_ProjectNoteInfo projectNotes;
		std::memset(&projectNotes, 0, sizeof(API_ProjectNoteInfo));
		
		GSErrCode err = ACAPI_ProjectSetting_GetProjectNotes(&projectNotes);
		if (err != NoError) {
			return GS::UniString();
		}
		
		// 在keywords字段中查找UUID (格式: "HBIM_UUID=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx")
		const char* keywords = projectNotes.keywords;
		const char* uuidPrefix = "HBIM_UUID=";
		size_t prefixLen = std::strlen(uuidPrefix);
		
		if (std::strncmp(keywords, uuidPrefix, prefixLen) == 0) {
			return GS::UniString(keywords + prefixLen);
		}
		
		return GS::UniString();
	}
	
	// 保存项目UUID到项目信息 (使用keywords字段)
	static GSErrCode SaveProjectUUIDToProjectNotes(const GS::UniString& uuid)
	{
		API_ProjectNoteInfo projectNotes;
		std::memset(&projectNotes, 0, sizeof(API_ProjectNoteInfo));
		
		// 先读取现有项目信息
		GSErrCode err = ACAPI_ProjectSetting_GetProjectNotes(&projectNotes);
		if (err != NoError) {
			return err;
		}
		
		// 准备keywords字符串: 保留原有内容，添加HBIM_UUID
		char newKeywords[256];
		std::string uuidStr = std::string(uuid.ToCStr().Get());
		std::snprintf(newKeywords, sizeof(newKeywords), "HBIM_UUID=%s", uuidStr.c_str());
		
		// 复制回结构体
		std::strncpy(projectNotes.keywords, newKeywords, sizeof(projectNotes.keywords) - 1);
		projectNotes.keywords[sizeof(projectNotes.keywords) - 1] = '\0';
		
		// 保存修改
		return ACAPI_ProjectSetting_ChangeProjectNotes(&projectNotes);
	}
	
	// 修复UUID格式（如果格式不正确）
	static GS::UniString FixUUIDFormat(const GS::UniString& uuid)
	{
		GS::UniString result = uuid;
		
		// 如果已经是标准格式，直接返回
		if (IsValidUUIDFormat(result)) {
			return result;
		}
		
		// 移除所有非十六进制字符（保留连字符）
		GS::UniString clean;
		for (UInt32 i = 0; i < result.GetLength(); ++i) {
			GS::uchar_t ch = result.GetChar(i);
			if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f') || ch == '-') {
				clean.Append(ch);
			}
		}
		
		// 如果长度正确且有正确的连字符位置，尝试修复
		if (clean.GetLength() == 32) {
			// 32个十六进制字符，添加连字符
			char fixed[37];
			std::memcpy(fixed, clean.ToCStr().Get(), 8);
			fixed[8] = '-';
			std::memcpy(fixed + 9, clean.ToCStr().Get() + 8, 4);
			fixed[13] = '-';
			std::memcpy(fixed + 14, clean.ToCStr().Get() + 12, 4);
			fixed[18] = '-';
			std::memcpy(fixed + 19, clean.ToCStr().Get() + 16, 4);
			fixed[23] = '-';
			std::memcpy(fixed + 24, clean.ToCStr().Get() + 20, 12);
			fixed[36] = '\0';
			
			GS::UniString fixedUuid(fixed);
			if (IsValidUUIDFormat(fixedUuid)) {
				ACAPI_WriteReport("FixUUIDFormat: 已修复UUID格式: %s → %s", false, 
								uuid.ToCStr().Get(), fixedUuid.ToCStr().Get());
				return fixedUuid;
			}
		}
		
		// 无法修复，生成新的UUID
		ACAPI_WriteReport("FixUUIDFormat: 无法修复UUID格式，生成新的: %s", true, uuid.ToCStr().Get());
		return GenerateUUID();
	}
	
	// 获取或创建项目UUID (主要函数)
	// 策略：项目文件(project notes)为唯一权威来源；preferences仅用于检测"另存为"副本
	// - 同一项目重命名/移动：UUID 不变，照片仍可读（相对路径解析到新目录）
	// - 多项目同目录：每个项目有独立 UUID，照片文件夹分开
	// - 另存为副本：检测到同一 UUID 对应不同路径且原路径仍存在，则为副本，生成新 UUID
	static GS::UniString GetOrCreateProjectUUID()
	{
		GS::UniString currentPath = GetCurrentProjectPath();
		GS::UniString uuid = ReadProjectUUIDFromProjectNotes();
		if (!uuid.IsEmpty())
			uuid = FixUUIDFormat(uuid);
		
		if (!uuid.IsEmpty()) {
			// 项目已有 UUID，检查是否为"另存为"副本
			GS::UniString lastUuid, lastPath;
			if (ReadProjectMappingFromPreferences(lastUuid, lastPath)) {
				if (currentPath == lastPath) {
					// 同一项目
					ACAPI_WriteReport("GetOrCreateProjectUUID: 同一项目，使用UUID: %s", false, uuid.ToCStr().Get());
					return uuid;
				}
				if (uuid == lastUuid && !lastPath.IsEmpty()) {
					// 同一 UUID、不同路径：判断是副本还是重命名
					bool oldPathExists = false;
					try {
						oldPathExists = std::filesystem::exists(std::filesystem::path(lastPath.ToCStr().Get()));
					} catch (...) {}
					if (oldPathExists) {
						// 原文件仍存在 → 当前为副本（另存为）
						uuid = GenerateUUID();
						GSErrCode err = SaveProjectUUIDToProjectNotes(uuid);
						if (err == NoError) {
							ACAPI_WriteReport("GetOrCreateProjectUUID: 检测到另存为副本，生成新UUID: %s", false, uuid.ToCStr().Get());
						}
						SaveProjectMappingToPreferences(uuid, currentPath);
						return uuid;
					}
					// 原路径不存在 → 视为重命名/移动，保持 UUID
					ACAPI_WriteReport("GetOrCreateProjectUUID: 视为重命名/移动，保持UUID: %s", false, uuid.ToCStr().Get());
				}
			}
			SaveProjectMappingToPreferences(uuid, currentPath);
			ACAPI_WriteReport("GetOrCreateProjectUUID: 从项目信息读取UUID: %s", false, uuid.ToCStr().Get());
			return uuid;
		}
		
		// 项目无 UUID，生成并保存
		uuid = GenerateUUID();
		ACAPI_WriteReport("GetOrCreateProjectUUID: 生成新UUID: %s", false, uuid.ToCStr().Get());
		GSErrCode notesErr = SaveProjectUUIDToProjectNotes(uuid);
		if (notesErr == NoError) {
			ACAPI_WriteReport("GetOrCreateProjectUUID: UUID已保存到项目信息", false);
		} else {
			ACAPI_WriteReport("GetOrCreateProjectUUID: 保存到项目信息失败: %d (可接受)", true, notesErr);
		}
		SaveProjectMappingToPreferences(uuid, currentPath);
		return uuid;
	}
}

GS::UniString PluginPalette::CalculateProjectHash ()
{
	// 使用新的UUID方案替代路径哈希
	// 该方法生成的项目标识符在文件重命名/移动时保持稳定
	
	try {
		GS::UniString projectUuid = GetOrCreateProjectUUID();
		if (!projectUuid.IsEmpty()) {
			// 使用UUID作为项目标识符
			ACAPI_WriteReport("CalculateProjectHash: 使用项目UUID: %s", false, projectUuid.ToCStr().Get());
			return projectUuid;
		}
	} catch (...) {
		ACAPI_WriteReport("CalculateProjectHash: UUID方案失败，回退到路径哈希", true);
	}
	
	// 回退到原始的路径哈希方案
	API_ProjectInfo projectInfo;
	GSErrCode err = ACAPI_ProjectOperation_Project(&projectInfo);
	if (err != NoError) {
		ACAPI_WriteReport("CalculateProjectHash: 获取项目信息失败: %d", true, err);
		return "unknown";
	}
	
	// 检查项目是否已保存
	if (projectInfo.untitled) {
		ACAPI_WriteReport("CalculateProjectHash: 项目未保存，使用临时标识符", false);
		return "unsaved_project";
	}
	
	// 使用项目路径计算哈希
	IO::Location projectLocation = *projectInfo.location;
	GS::UniString projectPath;
	projectLocation.ToPath(&projectPath);
	
	ACAPI_WriteReport("CalculateProjectHash: 回退到路径哈希，项目路径='%s'", false, projectPath.ToCStr().Get());
	
	UInt32 hashValue = GS::CalculateHashValue(projectPath);
	GS::UniString hashStr;
	hashStr.Printf("%08X", hashValue);
	ACAPI_WriteReport("CalculateProjectHash: 哈希值=0x%08X", false, hashValue);
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

void PluginPalette::ShowDiagnostics ()
{
	GS::UniString msg;
	msg.Append("【HBIM构件图片 诊断信息】\n\n");
	msg.Append("当前构件: ");
	msg.Append(APIGuidToString(currentElemGuid));
	msg.Append("\n\n");
	msg.Append("projectHash: ");
	msg.Append(projectHash.IsEmpty() ? "(空)" : projectHash);
	msg.Append("\n\n");
	msg.Append("hasHBIMImages: ");
	msg.Append(hasHBIMImages ? "是" : "否");
	msg.Append("\n");
	msg.Append("imagePaths数量: ");
	msg.Append(GS::ValueToUniString((Int32)imagePaths.GetSize()));
	msg.Append("\n");
	msg.Append("currentImageIndex: ");
	msg.Append(GS::ValueToUniString((Int32)currentImageIndex));
	msg.Append("\n\n");
	if (imagePaths.GetSize() > 0) {
		msg.Append("第一条路径: ");
		msg.Append(imagePaths[0]);
		msg.Append("\n\n");
		std::string attemptedFullPath;
		GS::UniString projectDir;
		if (BuildImageFullPath(imagePaths[0], attemptedFullPath, &projectDir)) {
			msg.Append("项目目录: ");
			msg.Append(projectDir);
			msg.Append("\n\n");
			msg.Append("尝试的完整路径: ");
			msg.Append(GS::UniString(attemptedFullPath.c_str()));
			msg.Append("\n\n");
		} else {
			msg.Append("项目目录: (获取失败，可能未保存项目)\n\n");
		}
		IO::Location loc = ResolveImagePath(imagePaths[0]);
		GS::UniString resolved;
		loc.ToPath(&resolved);
		msg.Append("解析后路径: ");
		msg.Append(resolved.IsEmpty() ? "(解析失败，通常因文件不存在)" : resolved);
		msg.Append("\n");
		bool exists = !resolved.IsEmpty() && std::filesystem::exists(std::filesystem::path(resolved.ToCStr().Get()));
		msg.Append("文件存在: ");
		msg.Append(exists ? "是" : "否");
	}
	DG::InformationAlert("诊断", msg.ToCStr().Get(), "确定");
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
		if (isImageEditMode) {
			// 编辑模式下的"添加图片"：直接打开文件选择
			SelectHBIMImages();
		} else if (hasHBIMImages && imagePaths.GetSize() > 0) {
			// 有图片且非编辑模式：进入编辑模式
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
	} else if (ev.GetSource() == &diagnosisButton) {
		ShowDiagnostics();
	}
}

void PluginPalette::ImageClicked (const DG::ImageClickEvent& ev)
{
	if (ev.GetSource() != &imagePreview)
		return;
	if (imagePaths.IsEmpty() || currentImageIndex >= imagePaths.GetSize())
		return;
	
	IO::Location imageLoc = ResolveImagePath(imagePaths[currentImageIndex]);
	GS::UniString fullPath;
	if (imageLoc.ToPath(&fullPath) != NoError || fullPath.IsEmpty())
		return;
	
#if defined (GS_MAC)
	IO::Location openCmd("/usr/bin/open");
	GS::Array<GS::UniString> argv;
	argv.Push(fullPath);
	GSErrCode err = IO::Process::ApplicationLauncher::Instance().Launch(openCmd, argv);
	if (err != NoError) {
		ACAPI_WriteReport("预览图片失败 (错误码: %d)", true, static_cast<int>(err));
	}
#elif defined (GS_WIN)
	// Windows: 使用 explorer.exe 打开文件（使用默认关联程序）
	IO::Location openCmd("C:\\Windows\\explorer.exe");
	GS::Array<GS::UniString> argv;
	argv.Push(fullPath);
	GSErrCode err = IO::Process::ApplicationLauncher::Instance().Launch(openCmd, argv);
	if (err != NoError) {
		ACAPI_WriteReport("预览图片失败 (错误码: %d)", true, static_cast<int>(err));
	}
#endif
}