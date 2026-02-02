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

GSErrCode GetProjectPhotoFolder (IO::Location& outPhotoFolderLoc, IO::Location& outProjectFolderLoc, GS::UniString& outProjectName);

GSErrCode CopyPhotosToProjectFolder (const IO::Location& photoFolderLoc, const GS::UniString& projectName, const GS::Array<IO::Location>& sourceLocs, GS::Array<GS::UniString>& outRelPaths);

GSErrCode WritePropertiesToElement (API_Guid elemGuid, const PropertyDefinitionGuids& guids, const GS::UniString& compId, const GS::UniString& photoDesc, const GS::Array<GS::UniString>& relPaths);

GSErrCode GetPhotoPathsFromElement (API_Guid elemGuid, API_Guid photoPathsDefGuid, GS::Array<GS::UniString>& outRelPaths);

void RunComponentInfoCommand ();

}

#endif
