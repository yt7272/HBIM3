// Mock ACAPI for Jest testing without ArchiCAD
// Includes both IFCPropertyReader and ComponentInfo functions

const ACAPI = {
  // IFC Property Reader functions
  GetSelectedElements: function() {
    console.log('Mock ACAPI.GetSelectedElements called');
    return Promise.resolve([
      ['{12345678-1234-1234-1234-123456789012}', 'Wall', 'Sample Wall'],
      ['{87654321-4321-4321-4321-210987654321}', 'Door', 'Sample Door']
    ]);
  },

  GetAllIFCProperties: function(elementGuid) {
    console.log(`Mock ACAPI.GetAllIFCProperties called with ${elementGuid}`);
    // Return mock IFC properties in C++ format: array of arrays
    // Each inner array: [propertySetName, propertyName, propertyValue, propertyType, unit, isValid]
    return Promise.resolve([
      ['IFC4', 'IfcWall', 'Standard', 'IfcLabel', 'mm', true],
      ['IFC4', 'FireRating', '60', 'IfcInteger', 'minutes', true],
      ['IFC2X3', 'LoadBearing', 'true', 'IfcBoolean', '', true],
      ['PSet_Revit_Constraints', 'Height', '3000', 'IfcLengthMeasure', 'mm', true]
    ]);
  },

  GetCachedIFCProperties: function(elementGuid) {
    console.log(`Mock ACAPI.GetCachedIFCProperties called with ${elementGuid}`);
    // Return cached properties in C++ format
    return Promise.resolve([
      ['IFC4', 'CachedProperty', 'CachedValue', 'IfcLabel', 'mm', true],
      ['IFC4', 'AnotherCached', 'Value', 'IfcText', '', true]
    ]);
  },

  GetCachedIFCPropertiesWithRefresh: function(elementGuid) {
    console.log(`Mock ACAPI.GetCachedIFCPropertiesWithRefresh called with ${elementGuid}`);
    // Simulate refreshed data in C++ format
    return Promise.resolve([
      ['IFC4', 'RefreshedProperty', 'RefreshedValue', 'IfcLabel', 'mm', true],
      ['IFC4', 'Timestamp', '2024-01-15', 'IfcDateTime', '', true],
      ['IFC4', 'Status', 'Updated', 'IfcText', '', true]
    ]);
  },

  ClearIFCPropertyCache: function() {
    console.log('Mock ACAPI.ClearIFCPropertyCache called');
    return Promise.resolve(true);
  },

  GetIFCPropertyCacheSize: function() {
    console.log('Mock ACAPI.GetIFCPropertyCacheSize called');
    return Promise.resolve(5); // Mock cache size
  },

  GetPluginVersion: function() {
    console.log('Mock ACAPI.GetPluginVersion called');
    return Promise.resolve('0.2.1.3'); // Mock version
  },

   // ComponentInfo functions (legacy unprefixed - kept for backward compatibility)
   GetComponentInfo: function(elementGuid) {
     console.log(`Mock ACAPI.GetComponentInfo called with ${elementGuid}`);
     return Promise.resolve({
       componentId: 'COMP-001',
       photoDescription: 'Sample component description',
       photoPaths: ['/path/to/photo1.jpg', '/path/to/photo2.jpg']
     });
   },

   SaveComponentInfo: function(elementGuid, componentInfo) {
     console.log(`Mock ACAPI.SaveComponentInfo called with ${elementGuid}`, componentInfo);
     return Promise.resolve(true);
   },

   SelectPhotos: function(elementGuid) {
     console.log(`Mock ACAPI.SelectPhotos called with ${elementGuid}`);
     return Promise.resolve(['/selected/photo1.jpg', '/selected/photo2.jpg']);
   },

   ViewPhotos: function(elementGuid) {
     console.log(`Mock ACAPI.ViewPhotos called with ${elementGuid}`);
     return Promise.resolve(true);
   },

   // ComponentInfo functions with prefix (matching JavaScript bridge)
   ComponentInfo_GetComponentInfo: function(elementGuid) {
     console.log(`Mock ACAPI.ComponentInfo_GetComponentInfo called with ${elementGuid}`);
     return Promise.resolve({
       componentId: 'COMP-001',
       photoDescription: 'Sample component description',
       photoPaths: ['/path/to/photo1.jpg', '/path/to/photo2.jpg']
     });
   },

   ComponentInfo_SaveComponentInfo: function(elementGuid, componentInfo) {
     console.log(`Mock ACAPI.ComponentInfo_SaveComponentInfo called with ${elementGuid}`, componentInfo);
     return Promise.resolve(true);
   },

   ComponentInfo_SelectPhotos: function(elementGuid) {
     console.log(`Mock ACAPI.ComponentInfo_SelectPhotos called with ${elementGuid}`);
     return Promise.resolve(['/selected/photo1.jpg', '/selected/photo2.jpg']);
   },

   ComponentInfo_ViewPhotos: function(elementGuid) {
     console.log(`Mock ACAPI.ComponentInfo_ViewPhotos called with ${elementGuid}`);
     return Promise.resolve(true);
   }
};

module.exports = ACAPI;