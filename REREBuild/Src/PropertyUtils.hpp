#ifndef GS_PROPERTYUTILS_HPP
#define GS_PROPERTYUTILS_HPP

#include "APIEnvir.h"
#include "ACAPinc.h"

struct API_Property;

namespace PropertyUtils {

struct PropertyInfo {
	GS::UniString propertySetName;
	GS::UniString propertyName;
	GS::UniString propertyValue;
	GS::UniString propertyType;
	GS::UniString unit;
	bool isValid;
};

	GSErrCode PropertyToString (const API_Property& property, GS::UniString& propertyValue);
	
	GS::UniString GetAddOnVersionString ();
	int GetAddOnVersionMajor ();
	int GetAddOnVersionMinor ();
	int GetAddOnVersionPatch ();
	int GetAddOnVersionBuild ();

	GS::Array<PropertyInfo> GetAllIFCPropertiesForElement (const API_Guid& elementGuid);
	GS::Array<PropertyInfo> GetIFCPropertiesForElement (const API_Guid& elementGuid, bool forceRefresh = false);


}

#endif