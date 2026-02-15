// Mock ACAPI object for testing
// Simulates the ACAPI object exposed by the C++ plugin

const mockACAPI = {
    // 1. GetSelectedElements
    GetSelectedElements: jest.fn(() => {
        return [
            ["{12345678-1234-1234-1234-123456789012}", "Wall", "ID-001"],
            ["{87654321-4321-4321-4321-210987654321}", "Door", "ID-002"]
        ];
    }),
    
    // 2. GetPluginVersion
    GetPluginVersion: jest.fn(() => {
        return "0.2.12.47";
    }),
    
    // 3. GetAllIFCProperties
    GetAllIFCProperties: jest.fn((elementGuid) => {
        return [
            ["IfcWall", "Name", "Wall-01", "IfcLabel", ""],
            ["IfcWall", "Description", "Exterior wall", "IfcText", ""]
        ];
    }),
    

    
    // 5. ComponentInfo_GetComponentInfo
    ComponentInfo_GetComponentInfo: jest.fn((elementGuid) => {
        return {
            componentId: "W-001",
            photoDesc: "West facade wall",
            photoPaths: ["_构件照片_12345678/wall_01.jpg", "_构件照片_12345678/wall_02.jpg"],
            globalId: "12345678-1234-1234-1234-123456789012",
            folderName: "12345678-1234-1234-1234-123456789012"
        };
    }),
    
    // 6. ComponentInfo_SaveComponentInfo
    ComponentInfo_SaveComponentInfo: jest.fn((params) => {
        // params: [elementGuid, componentId, photoDescription, photoPaths[]]
        return true;
    }),
    
    // 7. TestBridge
    TestBridge: jest.fn((message) => {
        console.log("TestBridge:", message);
        return true;
    }),
    
    // 8. LogDebug
    LogDebug: jest.fn((message) => {
        console.log("LogDebug:", message);
        return true;
    }),
    
    // 9. ComponentInfo_SelectPhotos
    ComponentInfo_SelectPhotos: jest.fn((elementGuid) => {
        return ["/path/to/photo1.jpg", "/path/to/photo2.jpg"];
    }),
    
    // 10. ComponentInfo_GetPhotoFullPath
    ComponentInfo_GetPhotoFullPath: jest.fn((relPath) => {
        return "/full/path/to/project/_构件照片_12345678/photo.jpg";
    }),
    
    // 11. ComponentInfo_ViewPhotos
    ComponentInfo_ViewPhotos: jest.fn((params) => {
        // params: [elementGuid, relPath]
        return true;
    }),
    
    // 12. ComponentInfo_GetPhotoBase64
    ComponentInfo_GetPhotoBase64: jest.fn((relPath) => {
        // Return a small base64 encoded image (1x1 pixel transparent PNG)
        return "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg==";
    }),
    
    // 13. ComponentInfo_SetEditMode
    ComponentInfo_SetEditMode: jest.fn((editMode) => {
        return true;
    }),
    
    // 14. ComponentInfo_SetUnsavedChanges
    ComponentInfo_SetUnsavedChanges: jest.fn((hasChanges) => {
        return true;
    })
};

// Setup global ACAPI object for tests
global.ACAPI = mockACAPI;

module.exports = mockACAPI;