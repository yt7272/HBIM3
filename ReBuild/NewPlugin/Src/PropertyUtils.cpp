#include "PropertyUtils.hpp"
#include "UniString.hpp"
#include "APIdefs_Properties.h"
#include "APIEnvir.h"
#include "ACAPinc.h"

// IFC API headers
#include "ACAPI/IFCAssignments.hpp"
#include "ACAPI/IFCAttribute.hpp"
#include "ACAPI/IFCClassificationReference.hpp"
#include "ACAPI/IFCObjectAccessor.hpp"
#include "ACAPI/IFCObjectID.hpp"
#include "ACAPI/IFCPropertyAccessor.hpp"
#include "ACAPI/IFCProperty.hpp"

#include "File.hpp"
#include "Folder.hpp"
#include "APICommon.h"
#include "MDIDs_Public.h"
#include "NewPluginDebug.hpp"

// C++ standard library headers (仅限必要)
#include <variant>
#include <optional>

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
	
	// 缓存已移除 - 符合防护栏要求 NO caching system
	
} // anonymous namespace

GSErrCode PropertyUtils::PropertyToString (const API_Property& property, GS::UniString& propertyValue)
{
	if (property.status == API_Property_NotAvailable || property.status == API_Property_NotEvaluated)
		return APIERR_BADPROPERTY;
	return ACAPI_Property_GetPropertyValueString (property, &propertyValue);
}

GS::UniString PropertyUtils::GetAddOnVersionString ()
{
	NewPluginDebugLog("[版本] GetAddOnVersionString 被调用");
	
#ifdef AC_ADDON_VERSION_STRING
	NewPluginDebugLog("[版本] AC_ADDON_VERSION_STRING 已定义: %s", AC_ADDON_VERSION_STRING);
	return GS::UniString (AC_ADDON_VERSION_STRING);
#else
	NewPluginDebugLog("[版本] AC_ADDON_VERSION_STRING 未定义，返回 0.0.0.0");
	return GS::UniString ("0.0.0.0");
#endif
}

int PropertyUtils::GetAddOnVersionMajor ()
{
#ifdef AC_ADDON_VERSION_MAJOR
	return AC_ADDON_VERSION_MAJOR;
#else
	return 0;
#endif
}

int PropertyUtils::GetAddOnVersionMinor ()
{
#ifdef AC_ADDON_VERSION_MINOR
	return AC_ADDON_VERSION_MINOR;
#else
	return 0;
#endif
}

int PropertyUtils::GetAddOnVersionPatch ()
{
#ifdef AC_ADDON_VERSION_PATCH
	return AC_ADDON_VERSION_PATCH;
#else
	return 0;
#endif
}

int PropertyUtils::GetAddOnVersionBuild ()
{
#ifdef AC_ADDON_VERSION_BUILD
	return AC_ADDON_VERSION_BUILD;
#else
	return 0;
#endif
}

GS::Array<PropertyUtils::PropertyInfo> PropertyUtils::GetAllIFCPropertiesForElement (const API_Guid& elementGuid)
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
		// IFC API异常，忽略但记录日志
		NewPluginDebugLog("[PropertyUtils] IFC API异常，忽略属性获取");
	}

	return propertyInfos;
}

GS::Array<PropertyUtils::PropertyInfo> PropertyUtils::GetIFCPropertiesForElement (const API_Guid& elementGuid, bool forceRefresh)
{
	// 基本实现：直接调用 GetAllIFCPropertiesForElement，无缓存
	// 符合防护栏要求：NO caching system
	(void)forceRefresh; // 未使用参数
	return GetAllIFCPropertiesForElement(elementGuid);
}




