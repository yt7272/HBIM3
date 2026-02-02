#include "IFCPropertyUtils.hpp"

#include "ACAPI/IFCAssignments.hpp"
#include "ACAPI/IFCAttribute.hpp"
#include "ACAPI/IFCClassificationReference.hpp"

#include "File.hpp"
#include "Folder.hpp"

#include "APICommon.h"
#include "MDIDs_Public.h"

// C++标准库头文件
#include <unordered_map>
#include <list>
#include <chrono>
#include <mutex>
#include <algorithm>

namespace IFCPropertyUtils {

namespace {
	static const GS::UniString TrueString ("True");
	static const GS::UniString FalseString ("False");
	static const GS::UniString UnknownString ("Unknown");
	static const GS::UniString SeparatorString ("; ");

	static GS::UniString Concatenate (const GS::Array<GS::UniString>& strings, const GS::UniString& separator)
	{
		if (strings.IsEmpty ())
			return GS::EmptyUniString;

		GS::UniString result = strings[0];
		for (UIndex i = 1; i < strings.GetSize (); ++i) {
			result.Append (separator);
			result.Append (strings[i]);
		}
		return result;
	}

	static GS::UniString PropertyValueToString (const IFCAPI::Value& value)
	{
		GS::UniString result;

		auto anyValue = value.GetAnyValue ();
		if (anyValue.has_value ()) {
			if (std::holds_alternative<Int64> (*anyValue)) {
				result.Append (GS::ValueToUniString (std::get<Int64> (*anyValue)));
			} else if (std::holds_alternative<double> (*anyValue)) {
				result.Append (GS::ValueToUniString (std::get<double> (*anyValue)));
			} else if (std::holds_alternative<bool> (*anyValue)) {
				result.Append (std::get<bool> (*anyValue) ? TrueString : FalseString);
			} else if (std::holds_alternative<IFCAPI::IfcLogical> (*anyValue)) {
				switch (std::get<IFCAPI::IfcLogical> (*anyValue)) {
				case IFCAPI::IfcLogical::Unknown: result.Append (UnknownString); break;
				case IFCAPI::IfcLogical::False: result.Append (FalseString); break;
				case IFCAPI::IfcLogical::True: result.Append (TrueString); break;
				}
			} else if (std::holds_alternative<GS::UniString> (*anyValue)) {
				result.Append (std::get<GS::UniString> (*anyValue));
			}
		}

		return result;
	}

	static GS::UniString ConcatenatePropertyValues (const std::vector<IFCAPI::Value>& values, const GS::UniString& separator)
	{
		GS::Array<GS::UniString> valueStrings;

		for (const auto& value: values)
			valueStrings.Push (PropertyValueToString (value));

		return Concatenate (valueStrings, separator);
	}

	static GS::UniString PropertyBoundedValueToString (const IFCAPI::PropertyBoundedValue& boundedValue)
	{
		GS::UniString result;
		GS::UniString lowerBoundValue = PropertyValueToString (boundedValue.GetLowerBoundValue ());
		GS::UniString upperBoundValue = PropertyValueToString (boundedValue.GetUpperBoundValue ());
		GS::UniString setPointValue = PropertyValueToString (boundedValue.GetSetPointValue ());

		if (!lowerBoundValue.IsEmpty () || !upperBoundValue.IsEmpty ()) {
			result.Append (lowerBoundValue);
			result.Append (" ... ");
			result.Append (upperBoundValue);
		}

		if (!setPointValue.IsEmpty ()) {
			if (!result.IsEmpty ())
				result.Append (SeparatorString);
			result.Append (setPointValue);
		}

		return result;
	}

	static GS::UniString PropertyTableValueToString (const IFCAPI::PropertyTableValue& tableValue)
	{
		GS::Array<GS::UniString> names;
		const auto& definingValues = tableValue.GetDefiningValues ();
		const auto& definedValues = tableValue.GetDefinedValues ();
		const USize size = std::min (definingValues.size (), definedValues.size ());

		for (UIndex i = 0; i < size; ++i)
			names.Push (PropertyValueToString (definingValues[i]) + ": " + PropertyValueToString (definedValues[i]));

		return Concatenate (names, SeparatorString);
	}

	static GS::UniString GetPropertyValueString (const IFCAPI::Property& property)
	{
		auto propertyVariant = property.GetTyped ();
		
		if (std::holds_alternative<IFCAPI::PropertySingleValue> (propertyVariant)) {
			const auto& propertyCast = std::get<IFCAPI::PropertySingleValue> (propertyVariant);
			return PropertyValueToString (propertyCast.GetNominalValue ());
		} else if (std::holds_alternative<IFCAPI::PropertyListValue> (propertyVariant)) {
			const auto& propertyCast = std::get<IFCAPI::PropertyListValue> (propertyVariant);
			return ConcatenatePropertyValues (propertyCast.GetListValues (), SeparatorString);
		} else if (std::holds_alternative<IFCAPI::PropertyBoundedValue> (propertyVariant)) {
			const auto& propertyCast = std::get<IFCAPI::PropertyBoundedValue> (propertyVariant);
			return PropertyBoundedValueToString (propertyCast);
		} else if (std::holds_alternative<IFCAPI::PropertyEnumeratedValue> (propertyVariant)) {
			const auto& propertyCast = std::get<IFCAPI::PropertyEnumeratedValue> (propertyVariant);
			GS::Array<GS::UniString> names;
			for (const auto& value : propertyCast.GetEnumerationValues ())
				names.Push (PropertyValueToString (value));
			return Concatenate (names, SeparatorString);
		} else {
			const auto& propertyCast = std::get<IFCAPI::PropertyTableValue> (propertyVariant);
			return PropertyTableValueToString (propertyCast);
		}
	}

	static GS::UniString GetPropertyTypeString (const IFCAPI::Property& property)
	{
		auto propertyVariant = property.GetTyped ();
		
		if (std::holds_alternative<IFCAPI::PropertySingleValue> (propertyVariant)) {
			return "Single Value";
		} else if (std::holds_alternative<IFCAPI::PropertyListValue> (propertyVariant)) {
			return "List Value";
		} else if (std::holds_alternative<IFCAPI::PropertyBoundedValue> (propertyVariant)) {
			return "Bounded Value";
		} else if (std::holds_alternative<IFCAPI::PropertyEnumeratedValue> (propertyVariant)) {
			return "Enumerated Value";
		} else {
			return "Table Value";
		}
	}

	static GS::UniString GetPropertyUnit (const IFCAPI::Property& property)
	{
		auto propertyVariant = property.GetTyped ();
		
		if (std::holds_alternative<IFCAPI::PropertySingleValue> (propertyVariant)) {
			const auto& propertyCast = std::get<IFCAPI::PropertySingleValue> (propertyVariant);
			return propertyCast.GetNominalValue ().GetType ();
		} else if (std::holds_alternative<IFCAPI::PropertyListValue> (propertyVariant)) {
			const auto& propertyCast = std::get<IFCAPI::PropertyListValue> (propertyVariant);
			const auto& listValues = propertyCast.GetListValues ();
			if (!listValues.empty ()) {
				return listValues[0].GetType ();
			}
			return GS::EmptyUniString;
		} else if (std::holds_alternative<IFCAPI::PropertyBoundedValue> (propertyVariant)) {
			const auto& propertyCast = std::get<IFCAPI::PropertyBoundedValue> (propertyVariant);
			return propertyCast.GetLowerBoundValue ().GetType ();
		} else if (std::holds_alternative<IFCAPI::PropertyEnumeratedValue> (propertyVariant)) {
			const auto& propertyCast = std::get<IFCAPI::PropertyEnumeratedValue> (propertyVariant);
			const auto& enumValues = propertyCast.GetEnumerationValues ();
			if (!enumValues.empty ()) {
				return enumValues[0].GetType ();
			}
			return GS::EmptyUniString;
		} else {
			const auto& propertyCast = std::get<IFCAPI::PropertyTableValue> (propertyVariant);
			const auto& definingValues = propertyCast.GetDefiningValues ();
			if (!definingValues.empty ()) {
				return definingValues[0].GetType ();
			}
			return GS::EmptyUniString;
		}
	}

} // anonymous namespace


GS::Array<PropertyInfo> GetIFCPropertiesForElement (const API_Guid& elementGuid)
{
	GS::Array<PropertyInfo> propertyInfos;

	try {
		IFCAPI::ObjectAccessor objectAccessor = IFCAPI::GetObjectAccessor ();

		API_Elem_Head elemHead{};
		elemHead.guid = elementGuid;
		
		GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
		if (err != NoError)
			return propertyInfos;

		auto objectIDResult = objectAccessor.CreateElementObjectID (elemHead);
		if (objectIDResult.IsErr ())
			return propertyInfos;

		IFCAPI::ObjectID objectID = objectIDResult.Unwrap ();
		
		// 获取IFC类型（如IfcWall, IfcDoor等）
		GS::UniString ifcTypeValue;
		auto ifcTypeResult = objectAccessor.GetIFCType (objectID);
		if (ifcTypeResult.IsOk ()) {
			ifcTypeValue = ifcTypeResult.Unwrap ();
		}
		
		IFCAPI::PropertyAccessor propertyAccessor (objectID);
		auto propertiesResult = propertyAccessor.GetLocalProperties ();

		if (propertiesResult.IsErr ())
			return propertyInfos;

		std::vector<IFCAPI::Property> properties = propertiesResult.Unwrap ();

		// 调试版本：返回所有属性，同时查找我们需要的信息
		GS::UniString globalIdValue;
		GS::UniString globalIdPropertySetName;
		GS::UniString tagValue;
		GS::UniString tagPropertySetName;
		GS::UniString nameValue;
		GS::UniString namePropertySetName;
		GS::UniString ifcTypePropertySetName;
		
		for (const IFCAPI::Property& property : properties) {
			GS::UniString propertySetName = property.GetPropertySetName ();
			GS::UniString propertyName = property.GetName ();
			GS::UniString propertyValue = GetPropertyValueString (property);
			GS::UniString propertyType = GetPropertyTypeString (property);
			GS::UniString propertyUnit = GetPropertyUnit (property);
			
			// 查找Tag属性（Archicad IFC ID），类型通常是IfcIdentifier
			// 支持英文"Tag"和中文"名称"（中文系统可能显示的名称）
			if (tagValue.IsEmpty () && 
				(propertyName.IsEqual ("Tag", GS::CaseInsensitive) ||
				 propertyName.IsEqual ("tag", GS::CaseInsensitive) ||
				 propertyName.IsEqual ("TAG", GS::CaseInsensitive) ||
				 propertyName.IsEqual (GS::UniString ("名称"), GS::CaseInsensitive) ||
				 propertyName.IsEqual (GS::UniString ("名稱"), GS::CaseInsensitive) ||
				 propertyName.IsEqual (GS::UniString ("标签"), GS::CaseInsensitive))) {
				tagValue = propertyValue;
				tagPropertySetName = propertySetName;
			}
			
			// 查找GlobalId属性（可能是Archicad IFC ID的备用）
			if (globalIdValue.IsEmpty () && 
				(propertyName.IsEqual ("GlobalId", GS::CaseInsensitive) ||
				 propertyName.IsEqual ("globalId", GS::CaseInsensitive) ||
				 propertyName.IsEqual ("GLOBALID", GS::CaseInsensitive) ||
				 propertyName.IsEqual ("GlobalID", GS::CaseInsensitive) ||
				 propertyName.IsEqual ("ID", GS::CaseInsensitive) ||
				 propertyName.IsEqual ("Id", GS::CaseInsensitive) ||
				 propertyName.IsEqual ("Identifier", GS::CaseInsensitive))) {
				globalIdValue = propertyValue;
				globalIdPropertySetName = propertySetName;
			}
			
			// 查找Name属性
			if (nameValue.IsEmpty () && 
				(propertyName.IsEqual ("Name", GS::CaseInsensitive) ||
				 propertyName.IsEqual ("name", GS::CaseInsensitive) ||
				 propertyName.IsEqual ("NAME", GS::CaseInsensitive))) {
				nameValue = propertyValue;
				namePropertySetName = propertySetName;
			}
			

			// 返回所有属性用于调试
			PropertyInfo info;
			info.propertySetName = propertySetName;
			info.propertyName = propertyName;
			info.propertyValue = propertyValue;
			info.propertyType = propertyType;
			info.unit = propertyUnit;
			info.isValid = true;
			propertyInfos.Push (info);
		}
		
		// 添加Archicad IFC ID作为特殊属性（优先使用Tag，其次使用GlobalId）
		GS::UniString archicadIFCIDValue;
		GS::UniString archicadIFCIDSource;
		
		if (!tagValue.IsEmpty ()) {
			archicadIFCIDValue = tagValue;
			archicadIFCIDSource = "Tag";
		} else if (!globalIdValue.IsEmpty ()) {
			archicadIFCIDValue = globalIdValue;
			archicadIFCIDSource = "GlobalId";
		}
		
		if (!archicadIFCIDValue.IsEmpty ()) {
			PropertyInfo info;
			info.propertySetName = "IFC_IDENTIFIER";
			info.propertyName = "Archicad_IFC_ID";
			info.propertyValue = archicadIFCIDValue;
			info.propertyType = "IfcIdentifier";
			info.unit = GS::EmptyUniString;
			info.isValid = true;
			propertyInfos.Push (info);
		}
		
		// 添加Name作为特殊属性（如果找到）
		if (!nameValue.IsEmpty ()) {
			PropertyInfo info;
			info.propertySetName = "IFC_BASIC";
			info.propertyName = "Name";
			info.propertyValue = nameValue;
			info.propertyType = "IfcLabel";
			info.unit = GS::EmptyUniString;
			info.isValid = true;
			propertyInfos.Push (info);
		}
		
		// 添加IFC类型作为特殊属性（如果找到）
		if (!ifcTypeValue.IsEmpty ()) {
			PropertyInfo info;
			info.propertySetName = "IFC_ENTITY";
			info.propertyName = "IFC_Type";
			info.propertyValue = ifcTypeValue;
			info.propertyType = "IfcEntity";
			info.unit = GS::EmptyUniString;
			info.isValid = true;
			propertyInfos.Push (info);
		}
		
		// 添加插件版本信息
		{
			PropertyInfo versionInfo;
			versionInfo.propertySetName = "ADDON_INFO";
			versionInfo.propertyName = "Version";
			versionInfo.propertyValue = IFCPropertyUtils::GetAddOnVersionString ();
			versionInfo.propertyType = "String";
			versionInfo.unit = GS::EmptyUniString;
			versionInfo.isValid = true;
			propertyInfos.Push (versionInfo);
		}
		

	}
	catch (...) {
		// Handle any unexpected exceptions
	}

	return propertyInfos;
}

GS::Array<PropertyInfo> GetIFCPropertiesForSelectedElements ()
{
	GS::Array<PropertyInfo> allPropertyInfos;

	API_SelectionInfo selectionInfo;
	GS::Array<API_Neig> selNeigs;
	ACAPI_Selection_Get (&selectionInfo, &selNeigs, false, false);
	BMKillHandle ((GSHandle*)&selectionInfo.marquee.coords);

	for (const API_Neig& neig : selNeigs) {
		GS::Array<PropertyInfo> elementProperties = GetIFCPropertiesForElement (neig.guid);
		for (const PropertyInfo& propInfo : elementProperties) {
			allPropertyInfos.Push (propInfo);
		}
	}

	return allPropertyInfos;
}



// 版本信息函数实现
GS::UniString GetAddOnVersionString ()
{
#ifdef AC_ADDON_VERSION_STRING
	return GS::UniString (AC_ADDON_VERSION_STRING);
#else
	return GS::UniString ("1.0.0");
#endif
}

int GetAddOnVersionMajor ()
{
#ifdef AC_ADDON_VERSION_MAJOR
	return AC_ADDON_VERSION_MAJOR;
#else
	return 1;
#endif
}

int GetAddOnVersionMinor ()
{
#ifdef AC_ADDON_VERSION_MINOR
	return AC_ADDON_VERSION_MINOR;
#else
	return 0;
#endif
}

int GetAddOnVersionPatch ()
{
#ifdef AC_ADDON_VERSION_PATCH
	return AC_ADDON_VERSION_PATCH;
#else
	return 0;
#endif
}

// 完整IFC属性读取函数 - 读取所有类型的IFC属性
GS::Array<PropertyInfo> GetAllIFCPropertiesForElement (const API_Guid& elementGuid)
{
	GS::Array<PropertyInfo> propertyInfos;

	try {
		IFCAPI::ObjectAccessor objectAccessor = IFCAPI::GetObjectAccessor ();

		API_Elem_Head elemHead{};
		elemHead.guid = elementGuid;
		
		GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
		if (err != NoError)
			return propertyInfos;

		auto objectIDResult = objectAccessor.CreateElementObjectID (elemHead);
		if (objectIDResult.IsErr ())
			return propertyInfos;

		IFCAPI::ObjectID objectID = objectIDResult.Unwrap ();
		
		// 获取IFC类型
		GS::UniString ifcTypeValue;
		auto ifcTypeResult = objectAccessor.GetIFCType (objectID);
		if (ifcTypeResult.IsOk ()) {
			ifcTypeValue = ifcTypeResult.Unwrap ();
		}
		
		IFCAPI::PropertyAccessor propertyAccessor (objectID);
		
		// 1. 读取本地属性
		auto localPropertiesResult = propertyAccessor.GetLocalProperties ();
		if (localPropertiesResult.IsOk ()) {
			std::vector<IFCAPI::Property> localProperties = localPropertiesResult.Unwrap ();
			for (const IFCAPI::Property& property : localProperties) {
				GS::UniString propertySetName = property.GetPropertySetName ();
				// 过滤掉Pset_DesignOptions属性集
				if (propertySetName.IsEqual ("Pset_DesignOptions", GS::CaseInsensitive)) {
					continue;
				}
				PropertyInfo info;
				info.propertySetName = propertySetName;
				info.propertyName = property.GetName ();
				info.propertyValue = GetPropertyValueString (property);
				info.propertyType = GetPropertyTypeString (property);
				info.unit = GetPropertyUnit (property);
				info.isValid = true;
				// 标记为本地属性
				info.propertySetName.Append (" [Local]");
				propertyInfos.Push (info);
			}
		}
		
		// 2. 读取预览属性（导出时的属性）
		auto previewPropertiesResult = propertyAccessor.GetPreviewProperties ();
		if (previewPropertiesResult.IsOk ()) {
			std::vector<IFCAPI::Property> previewProperties = previewPropertiesResult.Unwrap ();
			for (const IFCAPI::Property& property : previewProperties) {
				GS::UniString propSetName = property.GetPropertySetName ();
				// 过滤掉Pset_DesignOptions属性集
				if (propSetName.IsEqual ("Pset_DesignOptions", GS::CaseInsensitive)) {
					continue;
				}
				GS::UniString propName = property.GetName ();
				// 检查是否已存在相同的属性（避免重复）
				bool alreadyExists = false;
				for (const PropertyInfo& existing : propertyInfos) {
					if (existing.propertySetName == propSetName && existing.propertyName == propName) {
						alreadyExists = true;
						break;
					}
				}
				
				if (!alreadyExists) {
					PropertyInfo info;
					info.propertySetName = propSetName;
					info.propertyName = propName;
					info.propertyValue = GetPropertyValueString (property);
					info.propertyType = GetPropertyTypeString (property);
					info.unit = GetPropertyUnit (property);
					info.isValid = true;
					// 标记为预览属性
					info.propertySetName.Append (" [Preview]");
					propertyInfos.Push (info);
				}
			}
		}
		
		// 3. 读取属性（IFC Attributes）
		auto attributesResult = propertyAccessor.GetAttributes ();
		if (attributesResult.IsOk ()) {
			std::vector<IFCAPI::Attribute> attributes = attributesResult.Unwrap ();
			for (const IFCAPI::Attribute& attribute : attributes) {
				PropertyInfo info;
				info.propertySetName = "IFC_Attributes";
				info.propertyName = attribute.GetName ();
				auto value = attribute.GetValue ();
				if (value.has_value ()) {
					info.propertyValue = value.value ();
				} else {
					info.propertyValue = "(Empty)";
				}
				info.propertyType = GS::UniString ("Ifc") + attribute.GetValueType ();
				info.unit = GS::EmptyUniString;
				info.isValid = true;
				propertyInfos.Push (info);
			}
		}
		
		// 4. 读取分类引用
		auto classificationRefsResult = propertyAccessor.GetPreviewClassificationReferences ();
		if (classificationRefsResult.IsOk ()) {
			std::vector<IFCAPI::ClassificationReference> classificationRefs = classificationRefsResult.Unwrap ();
			for (const IFCAPI::ClassificationReference& classificationRef : classificationRefs) {
				PropertyInfo info;
				info.propertySetName = "Classification";
				info.propertyName = classificationRef.GetName ().value_or ("Unnamed");
				info.propertyValue = classificationRef.GetIdentification ().value_or ("No ID");
				info.propertyType = "IfcClassificationReference";
				info.unit = GS::EmptyUniString;
				info.isValid = true;
				propertyInfos.Push (info);
			}
		}
		
		// 添加特殊属性：IFC类型
		if (!ifcTypeValue.IsEmpty ()) {
			PropertyInfo info;
			info.propertySetName = "IFC_ENTITY";
			info.propertyName = "IFC_Type";
			info.propertyValue = ifcTypeValue;
			info.propertyType = "IfcEntity";
			info.unit = GS::EmptyUniString;
			info.isValid = true;
			propertyInfos.Push (info);
		}
		

	}
	catch (...) {
		// Handle any unexpected exceptions
	}

	return propertyInfos;
}

 
// ============================================================================
// 缓存实现
// ============================================================================

namespace {

// API_Guid哈希函数
struct API_GuidHash {
	size_t operator()(const API_Guid& guid) const {
		// 使用内存布局创建哈希值
		// API_Guid通常包含多个32位整数
		size_t hash = 0;
		const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&guid);
		for (size_t i = 0; i < sizeof(API_Guid); ++i) {
			hash = hash * 31 + bytes[i];
		}
		return hash;
	}
};

// API_Guid相等比较
struct API_GuidEqual {
	bool operator()(const API_Guid& a, const API_Guid& b) const {
		return a == b;
	}
};

// LRU缓存实现
class IFCPropertyCache {
private:
	struct CacheEntry {
		GS::Array<PropertyInfo> properties;
		std::chrono::steady_clock::time_point timestamp;
		std::list<API_Guid>::iterator lruIterator;
	};

	// 缓存大小限制
	static constexpr size_t MAX_CACHE_SIZE = 100;
	
	// 缓存数据结构
	std::unordered_map<API_Guid, CacheEntry, API_GuidHash, API_GuidEqual> cacheMap;
	std::list<API_Guid> lruList; // 最近最少使用顺序
	
	// 缓存统计
	size_t hitCount = 0;
	size_t missCount = 0;
	size_t evictionCount = 0;
	
	// 线程安全
	mutable std::mutex cacheMutex;
	
	// 私有构造函数（单例）
	IFCPropertyCache() = default;
	
public:
	// 获取单例实例
	static IFCPropertyCache& GetInstance() {
		static IFCPropertyCache instance;
		return instance;
	}
	
	// 获取缓存属性
	GS::Array<PropertyInfo> Get(const API_Guid& elementGuid, bool& foundInCache) {
		std::lock_guard<std::mutex> lock(cacheMutex);
		
		auto it = cacheMap.find(elementGuid);
		if (it != cacheMap.end()) {
			// 命中缓存，更新LRU顺序
			CacheEntry& entry = it->second;
			lruList.erase(entry.lruIterator);
			lruList.push_front(elementGuid);
			entry.lruIterator = lruList.begin();
			
			hitCount++;
			foundInCache = true;
			return entry.properties;
		}
		
		missCount++;
		foundInCache = false;
		return GS::Array<PropertyInfo>();
	}
	
	// 设置缓存属性
	void Set(const API_Guid& elementGuid, const GS::Array<PropertyInfo>& properties) {
		std::lock_guard<std::mutex> lock(cacheMutex);
		
		// 检查是否已存在
		auto it = cacheMap.find(elementGuid);
		if (it != cacheMap.end()) {
			// 更新现有条目
			CacheEntry& entry = it->second;
			lruList.erase(entry.lruIterator);
			lruList.push_front(elementGuid);
			entry.lruIterator = lruList.begin();
			entry.properties = properties;
			entry.timestamp = std::chrono::steady_clock::now();
		} else {
			// 添加新条目，检查是否超过大小限制
			if (cacheMap.size() >= MAX_CACHE_SIZE) {
				// 移除LRU条目
				API_Guid lruGuid = lruList.back();
				lruList.pop_back();
				cacheMap.erase(lruGuid);
				evictionCount++;
			}
			
			// 添加新条目
			lruList.push_front(elementGuid);
			CacheEntry entry;
			entry.properties = properties;
			entry.timestamp = std::chrono::steady_clock::now();
			entry.lruIterator = lruList.begin();
			cacheMap[elementGuid] = entry;
		}
	}
	
	// 清除指定元素的缓存
	void Clear(const API_Guid& elementGuid) {
		std::lock_guard<std::mutex> lock(cacheMutex);
		
		auto it = cacheMap.find(elementGuid);
		if (it != cacheMap.end()) {
			lruList.erase(it->second.lruIterator);
			cacheMap.erase(it);
		}
	}
	
	// 清除所有缓存
	void ClearAll() {
		std::lock_guard<std::mutex> lock(cacheMutex);
		cacheMap.clear();
		lruList.clear();
	}
	
	// 获取缓存统计信息
	struct CacheStats {
		size_t size;
		size_t maxSize;
		size_t hitCount;
		size_t missCount;
		size_t evictionCount;
		double hitRate;
	};
	
	CacheStats GetStats() const {
		std::lock_guard<std::mutex> lock(cacheMutex);
		
		CacheStats stats;
		stats.size = cacheMap.size();
		stats.maxSize = MAX_CACHE_SIZE;
		stats.hitCount = hitCount;
		stats.missCount = missCount;
		stats.evictionCount = evictionCount;
		
		size_t totalAccess = hitCount + missCount;
		if (totalAccess > 0) {
			stats.hitRate = static_cast<double>(hitCount) / totalAccess * 100.0;
		} else {
			stats.hitRate = 0.0;
		}
		
		return stats;
	}
	
	// 获取缓存大小
	size_t GetSize() const {
		std::lock_guard<std::mutex> lock(cacheMutex);
		return cacheMap.size();
	}
};

} // anonymous namespace

// ============================================================================
// 缓存函数实现
// ============================================================================

GS::Array<PropertyInfo> GetCachedIFCPropertiesForElement (const API_Guid& elementGuid, bool forceRefresh)
{
	// 如果强制刷新，先清除缓存
	if (forceRefresh) {
		IFCPropertyCache::GetInstance().Clear(elementGuid);
	}
	
	// 尝试从缓存获取
	bool foundInCache = false;
	GS::Array<PropertyInfo> properties = IFCPropertyCache::GetInstance().Get(elementGuid, foundInCache);
	
	if (foundInCache) {
		return properties;
	}
	
	// 缓存未命中，计算属性
	properties = GetAllIFCPropertiesForElement(elementGuid);
	
	// 存入缓存（如果属性不为空）
	if (!properties.IsEmpty()) {
		IFCPropertyCache::GetInstance().Set(elementGuid, properties);
	}
	
	return properties;
}

void ClearIFCPropertyCache ()
{
	IFCPropertyCache::GetInstance().ClearAll();
}

UInt32 GetIFCPropertyCacheSize ()
{
	return static_cast<UInt32>(IFCPropertyCache::GetInstance().GetSize());
}

} // namespace IFCPropertyUtils