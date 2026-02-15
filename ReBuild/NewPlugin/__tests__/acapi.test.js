// Test file for ACAPI functions
// This tests the mock ACAPI object defined in mock-acapi.js

describe('ACAPI Mock Functions', () => {
    // Test 1: GetSelectedElements
    test('GetSelectedElements returns array of elements', () => {
        const result = ACAPI.GetSelectedElements();
        expect(result).toBeDefined();
        expect(Array.isArray(result)).toBe(true);
        expect(result.length).toBe(2);
        expect(result[0][0]).toBe('{12345678-1234-1234-1234-123456789012}');
        expect(result[0][1]).toBe('Wall');
        expect(result[0][2]).toBe('ID-001');
    });

    // Test 2: GetPluginVersion
    test('GetPluginVersion returns version string', () => {
        const result = ACAPI.GetPluginVersion();
        expect(result).toBeDefined();
        expect(typeof result).toBe('string');
        expect(result).toBe('0.2.12.47');
    });

    // Test 3: GetAllIFCProperties
    test('GetAllIFCProperties returns property array', () => {
        const elementGuid = '{12345678-1234-1234-1234-123456789012}';
        const result = ACAPI.GetAllIFCProperties(elementGuid);
        expect(result).toBeDefined();
        expect(Array.isArray(result)).toBe(true);
        expect(result.length).toBe(2);
        expect(result[0][0]).toBe('IfcWall');
        expect(result[0][1]).toBe('Name');
        expect(result[0][2]).toBe('Wall-01');
    });



    // Test 5: ComponentInfo_GetComponentInfo
    test('ComponentInfo_GetComponentInfo returns component info object', () => {
        const elementGuid = '{12345678-1234-1234-1234-123456789012}';
        const result = ACAPI.ComponentInfo_GetComponentInfo(elementGuid);
        expect(result).toBeDefined();
        expect(typeof result).toBe('object');
        expect(result.componentId).toBe('W-001');
        expect(result.photoDesc).toBe('West facade wall');
        expect(Array.isArray(result.photoPaths)).toBe(true);
        expect(result.photoPaths.length).toBe(2);
    });

    // Test 6: ComponentInfo_SaveComponentInfo
    test('ComponentInfo_SaveComponentInfo returns true', () => {
        const params = ['{12345678-1234-1234-1234-123456789012}', 'W-001', 'Description', []];
        const result = ACAPI.ComponentInfo_SaveComponentInfo(params);
        expect(result).toBe(true);
    });

    // Test 7: TestBridge
    test('TestBridge returns true', () => {
        const result = ACAPI.TestBridge('test message');
        expect(result).toBe(true);
    });

    // Test 8: LogDebug
    test('LogDebug returns true', () => {
        const result = ACAPI.LogDebug('debug message');
        expect(result).toBe(true);
    });

    // Test 9: ComponentInfo_SelectPhotos
    test('ComponentInfo_SelectPhotos returns photo paths', () => {
        const elementGuid = '{12345678-1234-1234-1234-123456789012}';
        const result = ACAPI.ComponentInfo_SelectPhotos(elementGuid);
        expect(result).toBeDefined();
        expect(Array.isArray(result)).toBe(true);
        expect(result.length).toBe(2);
        expect(result[0]).toBe('/path/to/photo1.jpg');
    });

    // Test 10: ComponentInfo_GetPhotoFullPath
    test('ComponentInfo_GetPhotoFullPath returns full path', () => {
        const relPath = '_构件照片_12345678/photo.jpg';
        const result = ACAPI.ComponentInfo_GetPhotoFullPath(relPath);
        expect(result).toBeDefined();
        expect(typeof result).toBe('string');
        expect(result).toBe('/full/path/to/project/_构件照片_12345678/photo.jpg');
    });

    // Test 11: ComponentInfo_ViewPhotos
    test('ComponentInfo_ViewPhotos returns true', () => {
        const params = ['{12345678-1234-1234-1234-123456789012}', '_构件照片_12345678/photo.jpg'];
        const result = ACAPI.ComponentInfo_ViewPhotos(params);
        expect(result).toBe(true);
    });

    // Test 12: ComponentInfo_GetPhotoBase64
    test('ComponentInfo_GetPhotoBase64 returns base64 string', () => {
        const relPath = '_构件照片_12345678/photo.jpg';
        const result = ACAPI.ComponentInfo_GetPhotoBase64(relPath);
        expect(result).toBeDefined();
        expect(typeof result).toBe('string');
        expect(result.startsWith('data:image/png;base64,')).toBe(true);
    });

    // Test 13: ComponentInfo_SetEditMode
    test('ComponentInfo_SetEditMode returns true', () => {
        const result = ACAPI.ComponentInfo_SetEditMode(true);
        expect(result).toBe(true);
    });

    // Test 14: ComponentInfo_SetUnsavedChanges
    test('ComponentInfo_SetUnsavedChanges returns true', () => {
        const result = ACAPI.ComponentInfo_SetUnsavedChanges(true);
        expect(result).toBe(true);
    });
});