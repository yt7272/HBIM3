# Restore Full Functionality Plan

## **Status**
- ✅ JavaScript bridge connectivity confirmed (logs show ACAPI found)
- ✅ HTML resource loads successfully
- ✅ Basic JavaScript functions work (GetPluginVersion, etc.)
- ⏳ Ready to restore full merged plugin features

## **Current Architecture Issues**

### **1. Vestigial DG Controls**
- **Problem**: C++ code references DG controls (TextEdit, Button, etc.) that don't exist in palette resource
- **Evidence**: `MergedPluginPalette.cpp` uses `textComponentIdentifier.SetText()`, `editCompId.SetText()`, etc.
- **Resource**: GDLG 32520 only contains browser control (ID 20)
- **Impact**: Runtime errors when code tries to access non-existent controls

### **2. Hybrid Interface Design**
- **Current**: MinimalTest.html (debug interface)
- **Target**: Selection_Test.html (full merged interface)
- **Issue**: HTML interface designed for tabs (ComponentInfo + IFC Properties) but C++ code tries to use native DG controls

## **Restoration Steps**

### **Phase 1: Remove DG Control Dependencies**
```cpp
// Step 1: Remove DG control member variables from MergedPluginPalette.hpp
// Current:
private:
    DG::LeftText     textComponentIdentifier;  // Remove
    DG::LeftText     textClassification;       // Remove
    DG::TextEdit     editCompId;              // Remove
    // ... etc (all except browser)

// Step 2: Remove from constructor initializer list
MergedPluginPalette::MergedPluginPalette ()
    : DG::Palette (ACAPI_GetOwnResModule (), PaletteResId, ACAPI_GetOwnResModule (), s_paletteGuid)
    // Remove all DG control initializations except browser
    , browser (GetReference (), BrowserId)  // Keep only this
```

### **Phase 2: Update Resource to Full HTML Interface**
```grc
// Update RFIX/MergedPluginFix.grc
'DATA' 100 "Merged Plugin Html" {
    "Selection_Test.html"  // Change from MinimalTest.html
}
```

### **Phase 3: Migrate DG Control Logic to JavaScript**
Current C++ DG control operations → JavaScript HTML updates:

| DG Control Operation | JavaScript Equivalent |
|---------------------|----------------------|
| `textComponentIdentifier.SetText()` | `document.getElementById('selected-element').textContent` |
| `editCompId.SetText()` | `document.getElementById('component-id').value` |
| `editPhotoDesc.SetText()` | `document.getElementById('photo-description').value` |
| Button click handlers | HTML button `onclick` handlers calling ACAPI functions |

### **Phase 4: Update JavaScript Bridge Functions**
Ensure all required functions are registered in `InitializeJavaScriptBridge()`:

```cpp
// Current minimal set (from MinimalTest.html):
// - GetSelectedElements
// - GetPluginVersion

// Required full set (for Selection_Test.html):
// - GetAllIFCProperties
// - GetCachedIFCProperties
// - GetCachedIFCPropertiesWithRefresh
// - ClearIFCPropertyCache
// - GetIFCPropertyCacheSize
// - ComponentInfo_GetComponentInfo
// - ComponentInfo_UpdateComponentInfo
// - ComponentInfo_SelectPhotos
// - ComponentInfo_GetPhotoPaths
// - ComponentInfo_ViewPhotos
```

## **Testing Plan**

### **Test 1: Interface Loading**
1. Build plugin with Selection_Test.html
2. Open palette
3. Verify: Both tabs visible (ComponentInfo, IFC Properties)
4. Verify: Default tab is ComponentInfo (active)

### **Test 2: ComponentInfo Tab**
1. Select element in ArchiCAD
2. Verify: Component ID field auto-populates
3. Verify: Photo description field accessible
4. Test: "编辑信息" button toggles edit mode
5. Test: "选择照片" button (opens file dialog)
6. Test: "查看照片" button (opens photo viewer)

### **Test 3: IFC Properties Tab**
1. Switch to IFC Properties tab
2. Select element in ArchiCAD
3. Click "查看构件的IFC属性"
4. Verify: IFC properties displayed in table
5. Test: Cache operations (refresh, clear)

### **Test 4: JavaScript Bridge Functions**
Test each ACAPI function via browser console:
```javascript
// Test 1: GetSelectedElements
ACAPI.GetSelectedElements().then(result => console.log(result));

// Test 2: GetPluginVersion  
ACAPI.GetPluginVersion().then(version => console.log(version));

// Test 3: GetAllIFCProperties
ACAPI.GetAllIFCProperties("{GUID}").then(props => console.log(props));

// Test 4: ComponentInfo_GetComponentInfo
ACAPI.ComponentInfo_GetComponentInfo("{GUID}").then(info => console.log(info));

// Test 5: ComponentInfo_UpdateComponentInfo
ACAPI.ComponentInfo_UpdateComponentInfo("{GUID}", "ID123", "Photo desc").then(success => console.log(success));
```

## **Rollback Plan**

If issues arise during restoration:

1. **Immediate rollback**: Revert to MinimalTest.html
2. **Debug mode**: Enable CEF debugging (Chrome DevTools)
3. **Incremental restore**: Implement features one by one

## **Success Criteria**

✅ **Interface**: Full merged HTML interface loads correctly
✅ **Tabs**: Both ComponentInfo and IFC Properties tabs functional
✅ **Bridge**: All JavaScript functions work via ACAPI object
✅ **Performance**: No memory leaks, responsive UI
✅ **Error handling**: Graceful degradation on failures

## **Timeline Estimate**

- **Phase 1 (DG cleanup)**: 2-3 hours
- **Phase 2 (Resource update)**: 30 minutes
- **Phase 3 (JavaScript migration)**: 4-6 hours
- **Phase 4 (Testing)**: 2-3 hours
- **Total**: 9-12 hours

## **Risk Assessment**

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| DG control removal breaks event handling | High | High | Keep browser control only, move all events to JavaScript |
| JavaScript performance issues | Medium | Medium | Implement caching, batch operations |
| Memory leaks in JavaScript bridge | Low | High | Use RAII patterns, test with valgrind |
| Cross-thread JavaScript calls | Medium | Medium | Ensure callbacks on main thread |

## **Next Steps**

1. **Wait for user logs** confirming JavaScript bridge works
2. **If confirmed**: Begin Phase 1 implementation
3. **If issues**: Debug with CEF tools first

---

**Last Updated**: 2026-02-02  
**Plugin Version**: 0.2.2.33  
**Status**: Plan ready, awaiting bridge confirmation