#ifndef GS_IFCPROPERTYUTILS_HPP
#define GS_IFCPROPERTYUTILS_HPP


#include "APIEnvir.h"
#include "ACAPinc.h"
#include "ACAPI/IFCObjectAccessor.hpp"
#include "ACAPI/IFCObjectID.hpp"
#include "ACAPI/IFCPropertyAccessor.hpp"
#include "ACAPI/IFCProperty.hpp"


namespace IFCPropertyUtils {

struct PropertyInfo {
	GS::UniString propertySetName;
	GS::UniString propertyName;
	GS::UniString propertyValue;
	GS::UniString propertyType;
	GS::UniString unit;
	bool isValid;
};

GS::Array<PropertyInfo> GetIFCPropertiesForElement (const API_Guid& elementGuid);

GS::Array<PropertyInfo> GetIFCPropertiesForSelectedElements ();

// 完整IFC属性读取函数
GS::Array<PropertyInfo> GetAllIFCPropertiesForElement (const API_Guid& elementGuid);

// 性能优化：带缓存的属性读取
GS::Array<PropertyInfo> GetCachedIFCPropertiesForElement (const API_Guid& elementGuid, bool forceRefresh = false);

// 缓存管理函数
void ClearIFCPropertyCache ();
UInt32 GetIFCPropertyCacheSize ();

// 版本信息函数
GS::UniString GetAddOnVersionString ();
int GetAddOnVersionMajor ();
int GetAddOnVersionMinor ();
int GetAddOnVersionPatch ();

} // namespace IFCPropertyUtils


#endif