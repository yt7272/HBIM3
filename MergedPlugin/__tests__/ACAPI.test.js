// Example Jest test for MergedPlugin ACAPI mock
// Follows patterns from Selection_Test.html test suite

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

describe('ACAPI Mock', () => {
  test('should have all required IFC functions', () => {
    expect(typeof ACAPI.GetSelectedElements).toBe('function');
    expect(typeof ACAPI.GetAllIFCProperties).toBe('function');
    expect(typeof ACAPI.GetCachedIFCProperties).toBe('function');
    expect(typeof ACAPI.GetCachedIFCPropertiesWithRefresh).toBe('function');
    expect(typeof ACAPI.ClearIFCPropertyCache).toBe('function');
    expect(typeof ACAPI.GetIFCPropertyCacheSize).toBe('function');
    expect(typeof ACAPI.GetPluginVersion).toBe('function');
  });

  test('should have all required ComponentInfo functions', () => {
    expect(typeof ACAPI.GetComponentInfo).toBe('function');
    expect(typeof ACAPI.SaveComponentInfo).toBe('function');
    expect(typeof ACAPI.SelectPhotos).toBe('function');
    expect(typeof ACAPI.ViewPhotos).toBe('function');
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

  test('GetPluginVersion returns version string', async () => {
    const version = await ACAPI.GetPluginVersion();
    expect(typeof version).toBe('string');
    expect(version).toMatch(/^\d+\.\d+\.\d+\.\d+$/); // e.g., 0.2.1.3
  });
});