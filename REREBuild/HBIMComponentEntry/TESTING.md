# HBIMComponentEntry Plugin Testing Guide

## Overview

This document outlines the testing strategy and procedures for the HBIMComponentEntry plugin, which adds HBIM property management functionality to ArchiCAD.

## Test Environment Requirements

- ArchiCAD 29.x
- macOS (for plugin bundle) or Windows
- Test project with various element types (walls, doors, windows, etc.)

## Test Categories

### 1. Plugin Installation and Loading
- [ ] Plugin loads without errors in ArchiCAD
- [ ] Menu item "HBIM构件信息录入" appears in Add-Ons menu
- [ ] Palette opens when menu item is selected
- [ ] Palette closes when menu item is toggled off

### 2. Basic UI Functionality
- [ ] Palette displays IFC type and Global ID for selected elements
- [ ] "HBIM属性信息" section appears below IFC properties with separator line
- [ ] UI elements are properly sized and positioned
- [ ] Chinese text displays correctly (labels: "HBIM属性信息", "HBIM构件编号", "HBIM构件说明")

### 3. Property Workflow Testing

#### Scenario A: Element without HBIM properties
1. Select an element that has no HBIM properties
2. Verify palette shows:
   - "添加" button displayed
   - Text edit fields hidden
   - "取消" button hidden
3. Click "添加" button
4. Verify:
   - Text edit fields become visible and editable
   - Button text changes to "保存"
   - "取消" button appears
5. Enter values:
   - HBIM构件编号: "HBIM-001"
   - HBIM构件说明: "测试构件说明"
6. Click "保存"
7. Verify:
   - Text edit fields hide
   - Button text changes to "编辑"
   - Values persist in the display
   - No error messages in ArchiCAD report window
8. Select a different element, then reselect original element
9. Verify HBIM properties are still displayed correctly

#### Scenario B: Element with existing HBIM properties
1. Select an element with existing HBIM properties (from Scenario A)
2. Verify palette shows:
   - Property values displayed as text (not editable)
   - "编辑" button displayed
3. Click "编辑" button
4. Verify:
   - Text edit fields become visible with pre-filled values
   - Button text changes to "保存"
   - "取消" button appears
5. Modify values
6. Click "保存"
7. Verify changes are persisted
8. Click "编辑" then "取消"
9. Verify original values are restored

#### Scenario C: Error Handling
1. Try to save empty values
2. Verify properties can be saved with empty values (should be allowed)
3. Test with special characters in text fields
4. Test with very long text strings
5. Verify appropriate error messages in report window if API calls fail

### 4. Property Group Creation Testing
- [ ] First time "添加" clicked: property group "HBIM属性信息" is created
- [ ] Property definitions "HBIM构件编号" and "HBIM构件说明" are created
- [ ] Properties are available to all element types (proper classification item assignment)
- [ ] No API error -2130312307 (APIERR_NOTINIT) during property group creation

### 5. Multi-element Selection
- [ ] Select multiple elements: palette shows information for first element only
- [ ] Add/edit HBIM properties when multiple elements selected
- [ ] Verify properties are only applied to first selected element (current implementation)

### 6. Performance Testing
- [ ] Rapid element selection changes don't cause crashes
- [ ] Property operations complete within reasonable time
- [ ] No memory leaks with repeated property operations

## Test Verification Methods

### 1. Visual Inspection
- Palette layout and controls
- Text display and alignment
- Button states and transitions

### 2. ArchiCAD Report Window
- Check for error messages during operations
- Verify debug logging (if enabled) shows property operations
- Monitor API error codes

### 3. Property Manager Verification
- Open ArchiCAD Property Manager
- Verify "HBIM属性信息" group exists
- Verify "HBIM构件编号" and "HBIM构件说明" property definitions exist
- Check that properties are assigned to test elements with correct values

### 4. File Save/Load Testing
- Save project with HBIM properties
- Close and reopen project
- Verify HBIM properties persist
- Verify palette displays properties correctly after reload

## Expected Behavior by Component

### PluginPalette Class
- `CheckHBIMProperties()`: Should not attempt to create property groups, only check existing ones
- `EnsureHBIMPropertiesInitialized()`: Should create property groups only when needed (on user action)
- `WriteHBIMProperties()`: Should create property groups if they don't exist, then save values
- `HasHBIMProperties()`: Should return false when property definitions don't exist yet

### Property Helper Functions
- `FindExistingHBIMPropertyGroupAndDefinitions()`: Should only enumerate, never create
- `FindOrCreateHBIMGroup()`: Should create only when called from user action flow
- `SetHBIMPropertyValue()`: Should handle APINULLGuid gracefully

## Debug Logging

The plugin includes debug logging via `ACAPI_WriteReport()`. Key logs to monitor:

1. Property group enumeration: "FindOrCreateHBIMGroup: 找到 X 个属性组"
2. Property group creation attempts: "FindOrCreateHBIMGroup: CreatePropertyGroup 失败"
3. Property initialization: "EnsureHBIMPropertiesInitialized: 初始化失败"
4. Property read/write operations

## Common Issues and Solutions

### Issue: API Error -2130312307 during property group creation
**Cause**: Property API not fully initialized when called during selection change
**Solution**: Defer property creation until user explicitly clicks "添加" or "编辑"

### Issue: Properties not appearing in Property Manager
**Cause**: Classification items not properly assigned to property definitions
**Solution**: Ensure `GetAllClassificationItems()` collects all classification items and sets `outDef.availability`

### Issue: Palette shows incorrect property values
**Cause**: GUIDs not properly initialized or cached
**Solution**: Verify `TryFindExistingHBIMPropertyGroupAndDefinitions()` works correctly

## Manual Test Script

```javascript
// In ArchiCAD browser console (if HTML interface) - NOT APPLICABLE for DG palette
// This plugin uses DG interface, so tests must be manual through UI
```

## Automation Considerations

Due to the DG (Dialog) interface, automated testing requires:
- UI automation tools (not available for ArchiCAD plugins)
- Manual test execution following this guide
- Screenshot comparison for UI validation

## Success Criteria

1. All test scenarios pass without crashes
2. No API errors in report window during normal operation
3. HBIM properties persist across project save/load
4. UI responds within 1 second for all operations
5. Chinese text displays correctly on all systems
6. Property groups and definitions created only once per project

## Version Compatibility

Tested with:
- ArchiCAD 29 API Development Kit
- macOS arm64 bundle
- Windows x64 (if cross-compiled)

---

*Last Updated: 2026-02-15*
*Plugin Version: 0.2.15.30*
## Code Verification and Fixes (2026-02-15)

### Critical Bugs Fixed

1. **State Consistency Bug**: `EnsureHBIMPropertyGroupAndDefinitions` and `EnsureHBIMPropertiesInitialized` could leave member variables in partially updated state if property definition creation failed after property group creation.

   **Fix**: Changed both functions to use local variables and only update output parameters/member variables after all operations succeed.

2. **Improved Error Reporting**: `WriteHBIMProperties` now reports specific error codes for ID and description property writes separately.

### Architectural Improvements

1. **Deferred Property Creation**: Property groups/definitions are now created only when user explicitly clicks "添加" or "编辑", not during passive selection monitoring.

2. **Safe State Checking**: `HasHBIMProperties` now checks for `APINULLGuid` before attempting property value reads.

3. **Graceful Degradation**: When property definitions don't exist yet, plugin shows "添加" button instead of attempting to create properties automatically.

### Build Verification
- Plugin builds successfully: version 0.2.15.30
- Bundle integrity check passes
- Arm64 architecture confirmed
- No compilation warnings or errors

### Remaining Considerations for Actual ArchiCAD Testing
While code analysis confirms logical correctness, actual ArchiCAD testing is required to verify:
- Property API availability during different plugin lifecycle stages
- Chinese text rendering in UI controls  
- Property persistence across project save/load
- Performance with large numbers of elements

