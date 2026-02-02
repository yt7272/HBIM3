# Merged Plugin Testing Guide

## Plugin Information
- **Version**: 0.2.2.47 (Date-based: 0.month.day.build)
- **Interface**: Pure HTML with JavaScript bridge (no DG controls)
- **Theme**: Industrial dark theme from IFCPropertyReader
- **Features**: ComponentInfo + IFCPropertyReader merged with tabbed navigation

## Installation

### macOS
1. Copy the plugin bundle to ArchiCAD plugin directory:
   ```bash
   cp -r build/Release/MergedPlugin.bundle ~/Library/Application\ Support/GRAPHISOFT/ARCHICAD\ 29/Add-Ons/
   ```

2. Start ArchiCAD 29

3. Enable the plugin:
   - Go to **Options → Add-On Manager**
   - Find "Merged Plugin" in the list
   - Check the box to enable it
   - Click OK

### Windows
1. Copy `build/Release/MergedPlugin.bundle` to:
   ```
   C:\Users\[YourUsername]\AppData\Roaming\GRAPHISOFT\ARCHICAD 29\Add-Ons\
   ```

2. Start ArchiCAD 29 and enable the plugin in Add-On Manager

## Launching the Plugin

1. After enabling, go to menu: **测试 → Merged Plugin**
2. A palette window should appear with industrial dark theme
3. The palette has two tabs:
   - **构件信息** (Component Information)
   - **IFC属性** (IFC Properties)

## Testing ComponentInfo Functionality

### 1. Basic Selection
- Select any element (wall, door, window, etc.) in ArchiCAD
- The plugin should automatically display:
  - Element GUID
  - Element type
  - Element ID
  - Component ID (if previously saved)
  - Photo description (if previously saved)
  - Photo count (if photos exist)

### 2. Edit Mode
- Click the **编辑** (Edit) button in the ComponentInfo tab
- The UI should switch to edit mode with:
  - Text fields become editable
  - Save/Cancel buttons appear
  - Photo management controls enabled

### 3. Saving Component Information
- In edit mode, modify:
  - **构件编号** (Component ID) - enter a test value
  - **照片说明** (Photo Description) - enter a test description
- Click **选择照片** (Select Photos) button
  - Should open native file picker dialog
  - Select 1-2 test images (JPG/PNG)
  - Selected photo paths should appear in the UI
- Click **保存** (Save) button
  - Should save data to the element's properties
  - Should exit edit mode
  - Should display saved values

### 4. Photo Management
- After saving photos:
  - Click **查看照片** (View Photos) button
  - Should open system file explorer showing the component's photo folder
  - Photos should be copied to: `[Project Folder]/构件照片_[GUID]_photos/`
- Delete photos:
  - In edit mode, click trash icon next to a photo
  - Photo should be marked for deletion
  - Save to confirm deletion

### 5. Cross-Element Testing
- Select element A, enter edit mode, make changes
- Select element B without saving
  - **Expected**: No automatic save (feature disabled)
  - Changes remain in UI for element A (but element A is no longer selected)
  - This is a known limitation - user should save before switching

## Testing IFC Property Reader Functionality

### 1. IFC Property Display
- Select any element with IFC properties
- Switch to **IFC属性** (IFC Properties) tab
- Should display:
  - Property sets (grouped)
  - Property names and values
  - Data types and units
  - Chinese property name support

### 2. Cache System
- The plugin uses LRU cache for IFC properties (max 100 elements)
- Test cache behavior:
  - Select element, note load time
  - Select same element again - should be faster (cached)
  - Click **刷新缓存** (Refresh Cache) button
  - Cache should be cleared
  - Select element again - should reload from IFC

### 3. Performance Testing
- Select multiple different elements rapidly
- Cache should maintain performance
- Memory usage should remain stable

## JavaScript Bridge Testing

### 1. Console Testing (in ArchiCAD browser)
1. Open the palette
2. Press **F12** to open browser developer tools
3. In Console, run test commands:

```javascript
// Test ComponentInfo
window.testComponentInfo.runAllTests();

// Test IFC Properties
window.testIFCProperties.runAllTests();

// Test merged plugin
window.testMergedPlugin.runAllTests();
```

### 2. ACAPI Object Verification
```javascript
// Check if ACAPI object exists
console.log("ACAPI available:", typeof ACAPI !== 'undefined');

// List available functions
console.log("ACAPI functions:", Object.keys(ACAPI).filter(k => typeof ACAPI[k] === 'function'));
```

### 3. Individual Function Tests
```javascript
// Get selected elements
const elements = ACAPI.GetSelectedElements();
console.log("Selected elements:", elements);

// Get IFC properties for first element
if (elements.length > 0) {
  const props = ACAPI.GetAllIFCProperties(elements[0][0]); // guid
  console.log("IFC properties:", props);
}
```

## Validation Tests

### HTML Interface Validation
Run the validation script:
```bash
node validate_merged_interface.js
```
Checks:
- All required HTML elements exist
- JavaScript event handlers are properly attached
- CSS styles load correctly
- Tab navigation works

### Bundle Integrity Check
```bash
# Check bundle structure
find build/Release/MergedPlugin.bundle -type f | wc -l

# Check executable architecture
file build/Release/MergedPlugin.bundle/Contents/MacOS/MergedPlugin

# Check dependencies
otool -L build/Release/MergedPlugin.bundle/Contents/MacOS/MergedPlugin | head -10
```

## Known Issues & Limitations

### 1. Edit Mode State Management
- The C++ `isEditMode` flag is synchronized with JavaScript via `ComponentInfo_SetEditMode()`
- Automatic save on selection change is **disabled** (`HasUnsavedChanges()` always returns false)
- User must manually save before switching elements

### 2. Photo Management
- Photos are copied to project folder with GUID-based subdirectory
- Original photos remain in original location
- Deleted photos are removed from property but not from disk (intentional)

### 3. IFC Property Cache
- Cache is thread-safe with mutex protection
- Maximum 100 elements in LRU cache
- Cache persists for plugin session only

### 4. Property Group Availability
- Property group "构件扩展信息" is created automatically if missing
- If properties are not available for element type, the plugin attempts to recreate property group
- May require manual intervention in rare cases

### 5. Browser Compatibility
- Uses Chromium Embedded Framework (CEF) in ArchiCAD
- JavaScript must be enabled
- Console available via F12 for debugging

## Troubleshooting

### Plugin Doesn't Load
1. Check ArchiCAD version (29.x required)
2. Verify bundle structure (should have Contents/MacOS/MergedPlugin)
3. Check ArchiCAD Add-On Manager for error messages
4. Look for "Add-On loaded" messages in ArchiCAD report window

### Empty Palette
- The palette should show HTML interface, not empty controls
- If empty, check if HTML resource (ID 100) loads correctly
- Verify JavaScript bridge initialization in report window

### JavaScript Errors
1. Open browser console (F12)
2. Check for JavaScript errors
3. Verify ACAPI object exists
4. Check network tab for failed resource loads

### Performance Issues
- Large IFC models may cause slow property retrieval
- Enable cache for better performance
- Consider using `GetCachedIFCProperties()` instead of `GetAllIFCProperties()`

## Success Criteria

✅ Plugin loads without errors in ArchiCAD Add-On Manager  
✅ Palette opens with industrial dark theme and two tabs  
✅ ComponentInfo tab displays element information  
✅ IFC Properties tab displays IFC properties  
✅ Edit mode works (edit, save, cancel)  
✅ Photo selection opens file picker  
✅ Photo viewing opens folder  
✅ JavaScript bridge functions work (ACAPI object available)  
✅ Cache system works (performance improves on repeated selections)  
✅ No memory leaks (observe memory usage over time)  

## Reporting Issues

If you encounter issues:
1. Note the exact steps to reproduce
2. Check ArchiCAD report window for error messages
3. Open browser console (F12) for JavaScript errors
4. Include plugin version (0.2.2.47)
5. Include ArchiCAD version and operating system

---

*Last Updated: 2026-02-02 (Plugin version 0.2.2.47)*