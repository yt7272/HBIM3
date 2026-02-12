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

// Property group and definitions
GSErrCode EnsurePropertyGroupAndDefinitions (PropertyDefinitionGuids& outGuids);
GSErrCode FindPropertyDefinitions (PropertyDefinitionGuids& outGuids);

// Folder management
GSErrCode GetProjectPhotoFolder (IO::Location& outPhotoFolderLoc, IO::Location& outProjectFolderLoc, GS::UniString& outProjectName);
GSErrCode GetComponentPhotoFolder (const API_Guid& elemGuid, IO::Location& outComponentPhotoFolder, IO::Location& outProjectFolderLoc, GS::UniString& outProjectName);

// GlobalId extraction and folder naming
GS::UniString GetGlobalIdPropertyValue (const API_Guid& elementGuid);
GS::UniString GlobalIdToFolderName (const API_Guid& elementGuid);

// Photo operations
GSErrCode CopyPhotosForComponent (const API_Guid& elemGuid, const GS::Array<IO::Location>& sourceLocs, GS::Array<GS::UniString>& outRelPaths, UInt32 startIndex = 0);
GSErrCode GetPhotoFullPath (const GS::UniString& relPath, IO::Location& outFullPath);

// Property operations
GSErrCode WritePropertiesToElement (API_Guid elemGuid, const PropertyDefinitionGuids& guids, const GS::UniString& compId, const GS::UniString& photoDesc, const GS::Array<GS::UniString>& relPaths);
GSErrCode ReadPropertiesFromElement (API_Guid elemGuid, const PropertyDefinitionGuids& guids, GS::UniString& outCompId, GS::UniString& outPhotoDesc, GS::Array<GS::UniString>& outRelPaths);

// Helper functions
GSErrCode GetPhotoPathsFromElement (API_Guid elemGuid, API_Guid photoPathsDefGuid, GS::Array<GS::UniString>& outRelPaths);
GSErrCode GetStringPropertyFromElement (API_Guid elemGuid, API_Guid defGuid, GS::UniString& outVal);
GSErrCode SetStringPropertyToElement (API_Guid elemGuid, API_Guid defGuid, const GS::UniString& val);

// Component identifier
GSErrCode GetComponentIdentifier (API_Guid elemGuid, GS::UniString& outIdentifier);

// Additional functions from original ComponentInfo plugin
GSErrCode DeleteAndRecreatePropertyGroup (PropertyDefinitionGuids& outGuids);
GS::UniString GuidToFolderName (const API_Guid& guid);
GSErrCode GetElementClassificationInfo (API_Guid elemGuid, GS::UniString& outClassification, GS::UniString& outId);
GSErrCode GetElementAllProperties (API_Guid elemGuid, GS::UniString& outFormattedProperties);
GSErrCode GetElementIFCProperties (API_Guid elemGuid, GS::UniString& outFormattedProperties);
GSErrCode ExportAllBeamIFCProperties (const IO::Location& outputPath);
void RunComponentInfoCommand ();
void RunViewComponentPhotosCommand ();

// Component info management (added for NewPlugin)
struct ComponentInfoData {
	GS::UniString componentId;
	GS::UniString photoDesc;
	GS::Array<GS::UniString> photoPaths;
	GS::UniString globalId;
	GS::UniString folderName;
};

GSErrCode GetComponentInfo (API_Guid elemGuid, ComponentInfoData& outInfo);
GSErrCode SaveComponentInfo (API_Guid elemGuid, const ComponentInfoData& info);

}

#endif
