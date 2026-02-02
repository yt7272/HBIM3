// MergedPlugin Jest test suite covering IFC and ComponentInfo APIs
// Follows patterns from ACAPI.test.js and Selection_Test.html test suite

const ACAPI = require('ACAPI');

// Copy transform function from HTML for testing
function transformCppPropertiesToUI(cppProperties) {
  console.log('transformCppPropertiesToUI called with:', cppProperties);
  if (!cppProperties) {
    console.warn('cppProperties is null or undefined');
    return [];
  }
  if (!Array.isArray(cppProperties)) {
    console.warn('cppProperties is not an array:', typeof cppProperties, cppProperties);
    return [];
  }
  console.log(`Received ${cppProperties.length} properties from C++ backend`);
  const uiProperties = [];
  for (let i = 0; i < cppProperties.length; i++) {
    const prop = cppProperties[i];
    if (!Array.isArray(prop) || prop.length < 6) {
      console.warn(`Property at index ${i} is invalid:`, prop);
      continue;
    }
    const [propertySetName, propertyName, propertyValue, propertyType, unit, isValid] = prop;
    // Filter out invalid properties
    if (isValid === false) {
      console.log(`Property ${propertySetName}.${propertyName} is invalid (isValid=false), skipping`);
      continue;
    }
    // Map propertySetName -> category
    // For schema, try to extract IFC version from propertySetName, default to "IFC"
    let schema = "IFC";
    const schemaMatch = propertySetName.match(/IFC(2X3|4|2x3)?/i);
    if (schemaMatch) schema = schemaMatch[0].toUpperCase();
    uiProperties.push({
      schema: schema,
      category: propertySetName,
      name: propertyName,
      value: propertyValue,
      type: propertyType,
      unit: unit || null
    });
    // Log DEBUG_INFO properties
    if (propertySetName === 'DEBUG_INFO') {
      console.log(`DEBUG_INFO: ${propertyName} = ${propertyValue}`);
    }
  }
  console.log(`Transformed to ${uiProperties.length} UI properties`);
  return uiProperties;
}

// Copy groupProperties function from HTML for testing
function groupProperties(properties) {
  const groups = {};
  properties.forEach(prop => {
    const key = prop.category; // propertySetName
    if (!groups[key]) {
      groups[key] = {
        schema: prop.schema,
        category: prop.category,
        properties: []
      };
    }
    groups[key].properties.push(prop);
  });
  return Object.values(groups);
}

// Helper to generate a mock element GUID
function mockGuid() {
  return '{' + 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
    const r = Math.random() * 16 | 0, v = c == 'x' ? r : (r & 0x3 | 0x8);
    return v.toString(16);
  }) + '}';
}

describe('MergedPlugin API Integration', () => {
  // ==================== IFC Property Reader API Tests ====================
  describe('IFC Property Reader API', () => {
    test('should have all required IFC functions', () => {
      expect(typeof ACAPI.GetSelectedElements).toBe('function');
      expect(typeof ACAPI.GetAllIFCProperties).toBe('function');
      expect(typeof ACAPI.GetCachedIFCProperties).toBe('function');
      expect(typeof ACAPI.GetCachedIFCPropertiesWithRefresh).toBe('function');
      expect(typeof ACAPI.ClearIFCPropertyCache).toBe('function');
      expect(typeof ACAPI.GetIFCPropertyCacheSize).toBe('function');
      expect(typeof ACAPI.GetPluginVersion).toBe('function');
    });

    test('GetSelectedElements returns array of elements', async () => {
      const elements = await ACAPI.GetSelectedElements();
      expect(Array.isArray(elements)).toBe(true);
      expect(elements.length).toBeGreaterThan(0);
      expect(Array.isArray(elements[0])).toBe(true);
      expect(elements[0].length).toBe(3); // [guid, type, name]
    });

    test('GetAllIFCProperties returns array of properties in C++ format', async () => {
      const properties = await ACAPI.GetAllIFCProperties('{guid}');
      expect(Array.isArray(properties)).toBe(true);
      expect(properties.length).toBeGreaterThan(0);
      // Each property should be array of 6 elements
      properties.forEach(prop => {
        expect(Array.isArray(prop)).toBe(true);
        expect(prop.length).toBe(6);
      });
    });

    test('GetCachedIFCProperties returns cached properties', async () => {
      const properties = await ACAPI.GetCachedIFCProperties('{guid}');
      expect(Array.isArray(properties)).toBe(true);
      properties.forEach(prop => {
        expect(Array.isArray(prop)).toBe(true);
        expect(prop.length).toBe(6);
      });
    });

    test('GetCachedIFCPropertiesWithRefresh returns refreshed properties', async () => {
      const properties = await ACAPI.GetCachedIFCPropertiesWithRefresh('{guid}');
      expect(Array.isArray(properties)).toBe(true);
      properties.forEach(prop => {
        expect(Array.isArray(prop)).toBe(true);
        expect(prop.length).toBe(6);
      });
    });

    test('ClearIFCPropertyCache returns true', async () => {
      const result = await ACAPI.ClearIFCPropertyCache();
      expect(result).toBe(true);
    });

    test('GetIFCPropertyCacheSize returns number', async () => {
      const size = await ACAPI.GetIFCPropertyCacheSize();
      expect(typeof size).toBe('number');
      expect(size).toBeGreaterThanOrEqual(0);
    });

    test('GetPluginVersion returns version string', async () => {
      const version = await ACAPI.GetPluginVersion();
      expect(typeof version).toBe('string');
      expect(version).toMatch(/^\d+\.\d+\.\d+\.\d+$/); // e.g., 0.2.1.3
    });

    test('transformCppPropertiesToUI transforms mock data correctly', () => {
      const mockCppProperties = [
        ['IFC4', 'IfcWall', 'Standard', 'IfcLabel', 'mm', true],
        ['IFC2X3', 'LoadBearing', 'true', 'IfcBoolean', '', true]
      ];
      const uiProperties = transformCppPropertiesToUI(mockCppProperties);
      expect(uiProperties.length).toBe(2);
      expect(uiProperties[0]).toMatchObject({
        schema: 'IFC4',
        category: 'IFC4',
        name: 'IfcWall',
        value: 'Standard',
        type: 'IfcLabel',
        unit: 'mm'
      });
    });

    test('groupProperties groups by category', () => {
      const uiProperties = [
        { schema: 'IFC4', category: 'IFC4', name: 'IfcWall', value: 'Standard', type: 'IfcLabel', unit: 'mm' },
        { schema: 'IFC4', category: 'IFC4', name: 'FireRating', value: '60', type: 'IfcInteger', unit: 'minutes' },
        { schema: 'IFC2X3', category: 'IFC2X3', name: 'LoadBearing', value: 'true', type: 'IfcBoolean', unit: null }
      ];
      const groups = groupProperties(uiProperties);
      expect(groups.length).toBe(2); // IFC4 and IFC2X3 categories
      const ifc4Group = groups.find(g => g.category === 'IFC4');
      expect(ifc4Group.properties.length).toBe(2);
    });
  });

  // ==================== ComponentInfo API Tests ====================
  describe('ComponentInfo API', () => {
    test('should have all required ComponentInfo functions (prefixed)', () => {
      expect(typeof ACAPI.ComponentInfo_GetComponentInfo).toBe('function');
      expect(typeof ACAPI.ComponentInfo_SaveComponentInfo).toBe('function');
      expect(typeof ACAPI.ComponentInfo_SelectPhotos).toBe('function');
      expect(typeof ACAPI.ComponentInfo_ViewPhotos).toBe('function');
    });

    test('should also have legacy unprefixed functions for backward compatibility', () => {
      expect(typeof ACAPI.GetComponentInfo).toBe('function');
      expect(typeof ACAPI.SaveComponentInfo).toBe('function');
      expect(typeof ACAPI.SelectPhotos).toBe('function');
      expect(typeof ACAPI.ViewPhotos).toBe('function');
    });

    test('ComponentInfo_GetComponentInfo returns component info object', async () => {
      const elementGuid = mockGuid();
      const info = await ACAPI.ComponentInfo_GetComponentInfo(elementGuid);
      expect(typeof info).toBe('object');
      expect(info).toHaveProperty('componentId');
      expect(info).toHaveProperty('photoDescription');
      expect(info).toHaveProperty('photoPaths');
      expect(Array.isArray(info.photoPaths)).toBe(true);
      expect(info.componentId).toBe('COMP-001');
      expect(info.photoDescription).toBe('Sample component description');
      expect(info.photoPaths).toEqual(['/path/to/photo1.jpg', '/path/to/photo2.jpg']);
    });

    test('ComponentInfo_SaveComponentInfo saves component info and returns true', async () => {
      const elementGuid = mockGuid();
      const componentInfo = {
        componentId: 'TEST-123',
        photoDescription: 'Test description',
        photoPaths: ['/test/photo.jpg']
      };
      const result = await ACAPI.ComponentInfo_SaveComponentInfo(elementGuid, componentInfo);
      expect(result).toBe(true);
    });

    test('ComponentInfo_SelectPhotos returns array of selected photo paths', async () => {
      const elementGuid = mockGuid();
      const paths = await ACAPI.ComponentInfo_SelectPhotos(elementGuid);
      expect(Array.isArray(paths)).toBe(true);
      expect(paths.length).toBeGreaterThan(0);
      expect(paths[0]).toMatch(/\.jpg$/);
    });

    test('ComponentInfo_ViewPhotos returns true', async () => {
      const elementGuid = mockGuid();
      const result = await ACAPI.ComponentInfo_ViewPhotos(elementGuid);
      expect(result).toBe(true);
    });

    // Legacy function tests (ensure they still work)
    test('GetComponentInfo returns component info object', async () => {
      const elementGuid = mockGuid();
      const info = await ACAPI.GetComponentInfo(elementGuid);
      expect(typeof info).toBe('object');
      expect(info).toHaveProperty('componentId');
      expect(info).toHaveProperty('photoDescription');
      expect(info).toHaveProperty('photoPaths');
    });

    test('SaveComponentInfo saves component info and returns true', async () => {
      const elementGuid = mockGuid();
      const componentInfo = {
        componentId: 'TEST-456',
        photoDescription: 'Legacy test',
        photoPaths: []
      };
      const result = await ACAPI.SaveComponentInfo(elementGuid, componentInfo);
      expect(result).toBe(true);
    });
  });

  // ==================== Integration & Edge Case Tests ====================
  describe('Integration & Edge Cases', () => {
    test('transformCppPropertiesToUI handles empty input', () => {
      expect(transformCppPropertiesToUI(null)).toEqual([]);
      expect(transformCppPropertiesToUI(undefined)).toEqual([]);
      expect(transformCppPropertiesToUI([])).toEqual([]);
    });

    test('transformCppPropertiesToUI filters invalid properties', () => {
      const invalidProps = [
        ['IFC4', 'IfcWall', 'Standard', 'IfcLabel', 'mm', false], // isValid = false
        ['IFC4', 'Invalid', 'Value', 'IfcLabel', 'mm', true], // valid
        'not an array', // invalid item
        ['too', 'short'] // too short
      ];
      const uiProps = transformCppPropertiesToUI(invalidProps);
      expect(uiProps.length).toBe(1); // only the valid one
      expect(uiProps[0].name).toBe('Invalid');
    });

    test('transformCppPropertiesToUI extracts schema from propertySetName', () => {
      const props = [
        ['IFC4_Wall', 'Name', 'Value', 'IfcLabel', '', true],
        ['IFC2X3_Beam', 'Name', 'Value', 'IfcLabel', '', true],
        ['Pset_Revit', 'Name', 'Value', 'IfcLabel', '', true] // no IFC prefix
      ];
      const uiProps = transformCppPropertiesToUI(props);
      expect(uiProps[0].schema).toBe('IFC4');
      expect(uiProps[1].schema).toBe('IFC2X3');
      expect(uiProps[2].schema).toBe('IFC'); // default
    });

    test('groupProperties handles empty properties', () => {
      expect(groupProperties([])).toEqual([]);
    });

    test('mockGuid generates valid GUID format', () => {
      const guid = mockGuid();
      expect(guid).toMatch(/^\{[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}\}$/);
    });

    test('IFC cache functions integration', async () => {
      // Clear cache first
      await ACAPI.ClearIFCPropertyCache();
      const size = await ACAPI.GetIFCPropertyCacheSize();
      expect(size).toBe(5); // mock returns 5
    });
  });
});