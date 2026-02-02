#if !defined (COMPONENTINFO_HPP)
#define COMPONENTINFO_HPP

#include "APIEnvir.h"
#include "ACAPinc.h"

namespace ComponentInfo {

struct PropertyDefinitionGuids {
	API_Guid groupGuid;
	API_Guid compIdGuid;
	API_Guid photoDescGuid;
	API_Guid photoPathsGuid;
};

GSErrCode EnsurePropertyGroupAndDefinitions (PropertyDefinitionGuids& outGuids);

GSErrCode DeleteAndRecreatePropertyGroup (PropertyDefinitionGuids& outGuids);

GSErrCode FindPropertyDefinitions (PropertyDefinitionGuids& outGuids);

GSErrCode GetProjectPhotoFolder (IO::Location& outPhotoFolderLoc, IO::Location& outProjectFolderLoc, GS::UniString& outProjectName);

GSErrCode GetComponentPhotoFolder (const API_Guid& elemGuid, IO::Location& outComponentPhotoFolder, IO::Location& outProjectFolderLoc, GS::UniString& outProjectName);

GS::UniString GuidToFolderName (const API_Guid& guid);

GSErrCode CopyPhotosForComponent (const API_Guid& elemGuid, const GS::Array<IO::Location>& sourceLocs, GS::Array<GS::UniString>& outRelPaths, UInt32 startIndex = 0);

GSErrCode WritePropertiesToElement (API_Guid elemGuid, const PropertyDefinitionGuids& guids, const GS::UniString& compId, const GS::UniString& photoDesc, const GS::Array<GS::UniString>& relPaths);

GSErrCode GetPhotoPathsFromElement (API_Guid elemGuid, API_Guid photoPathsDefGuid, GS::Array<GS::UniString>& outRelPaths);

GSErrCode GetStringPropertyFromElement (API_Guid elemGuid, API_Guid defGuid, GS::UniString& outVal);

GSErrCode GetElementClassificationInfo (API_Guid elemGuid, GS::UniString& outClassification, GS::UniString& outId);

GSErrCode GetComponentIdentifier (API_Guid elemGuid, GS::UniString& outIdentifier);

GSErrCode GetElementIFCProperties (API_Guid elemGuid, GS::UniString& outFormattedProperties);
GSErrCode GetElementAllProperties (API_Guid elemGuid, GS::UniString& outFormattedProperties);
GSErrCode ExportAllBeamIFCProperties (const IO::Location& outputPath);

void RunComponentInfoCommand ();

void RunViewComponentPhotosCommand ();

}

#endif
