# DG to HTML Implementation Plan

Based on comprehensive analysis of `MergedPluginPalette.cpp` (129 references to 16 DG controls)

## Current Status Assessment

✅ **JavaScript Bridge**: Fully functional with 8+ ACAPI functions
✅ **HTML Interface**: `Selection_Test.html` provides complete UI
❌ **DG Dependencies**: 129 references across 15 functions
⚠️ **Resource Mismatch**: Palette only has browser control (ID 20), others don't exist

## Phase 1: Low-Hanging Fruit Removal (Easy - 1-2 hours)

### Step 1.1: Remove DG References from Data Access Functions
**Functions**: `DoSave()`, `HasUnsavedChanges()`, `EnterEditMode()`
**Changes**:
- Remove `editCompId.GetText()`, `editPhotoDesc.GetText()`, `originalCompId`, `originalPhotoDesc` references
- Keep logic intact - will call JavaScript functions instead
- **Lines**: 857-858, 961, 967, 987-988

### Step 1.2: Update Constructor Cleanup
**Function**: `MergedPluginPalette()` constructor
**Changes**:
- Remove DG control initializations (lines 89-104)
- Remove commented DG setup (lines 114-143)
- Keep browser initialization (line 105)
- **Result**: Constructor only initializes browser and member variables

### Step 1.3: Remove Simple UI Updates
**Function**: `UpdatePhotoIndexText()`, `DoSelectPhotos()`
**Changes**:
- Remove `textPhotoIndex.SetText()` (lines 656, 663)
- Remove `textPhotoPaths.SetText()` (line 905)
- **Note**: These will be handled by JavaScript

## Phase 2: Core UI Migration (Medium - 3-4 hours)

### Step 2.1: Rewrite RefreshContent() - THE MAIN FUNCTION
**Current**: 28 DG control references across 20+ lines
**New Approach**: Call JavaScript function to update HTML UI

```cpp
// BEFORE: 28 lines of DG control updates
void MergedPluginPalette::RefreshContent() {
    // ... DG control updates ...
    textComponentIdentifier.SetText(identifier);
    textClassification.SetText(typeName);
    editCompId.SetText(compId);
    // ... etc
}

// AFTER: Call JavaScript
void MergedPluginPalette::RefreshContent() {
    GS::Array<ElementInfo> selectedElements = GetSelectedElements();
    if (!selectedElements.IsEmpty()) {
        // Update current element GUID
        currentElemGuid = APIGuidFromString(selectedElements[0].guidStr.ToCStr().Get());
        
        // Call JavaScript to update UI
        browser.ExecuteJavaScript("if (typeof UpdateComponentInfoUI === 'function') { " +
            "UpdateComponentInfoUI(" + selectedElements[0].guidStr + ", " +
            selectedElements[0].typeName + ", " + selectedElements[0].elemID + "); }");
    } else {
        currentElemGuid = APINULLGuid;
        browser.ExecuteJavaScript("if (typeof ClearComponentInfoUI === 'function') ClearComponentInfoUI();");
    }
}
```

**Lines to modify**: 417-533

### Step 2.2: Replace UpdateEditModeUI()
**Current**: 8 DG control references
**New Approach**: JavaScript state management

```cpp
// AFTER: Call JavaScript to toggle edit mode
void MergedPluginPalette::UpdateEditModeUI() {
    browser.ExecuteJavaScript("if (typeof ToggleEditMode === 'function') ToggleEditMode(" + 
        GS::UniString(isEditMode ? "true" : "false") + ");");
}
```

**Lines to modify**: 819-848

## Phase 3: Event Handling Migration (Medium - 2-3 hours)

### Step 3.1: Remove ButtonClicked()
**Current**: Routes button events via DG control pointers
**New Approach**: All button events handled in HTML JavaScript

```cpp
// COMPLETE REMOVAL of ButtonClicked() function
// Lines 797-817 can be deleted entirely
```

**Rationale**: HTML buttons have `onclick` handlers calling ACAPI functions

### Step 3.2: Remove Event Attachments
**Current**: Constructor attaches button observers (commented out)
**New Approach**: No DG event attachments needed

```cpp
// Remove all commented button attachments (lines 114-122)
// btnViewPhotos.Attach (*this);
// btnPrevPhoto.Attach (*this);
// ... etc
```

## Phase 4: Photo Management Migration (Complex - 4-5 hours)

### Step 4.1: Replace Image Display Functions
**Functions**: `ShowPhotoAtIndex()`, `ShowPendingPhotoPreview()`
**Challenge**: DG's `pictureItem.SetPicture()` uses native image rendering
**Solution**: Base64 encoded images via JavaScript

```cpp
// BEFORE: DG image display
void MergedPluginPalette::ShowPhotoAtIndex(UInt32 index) {
    // ... Load image with DG::GXImage
    pictureItem.SetPicture(nativeImage);
}

// AFTER: Base64 to JavaScript
void MergedPluginPalette::ShowPhotoAtIndex(UInt32 index) {
    // Load image and convert to base64
    GS::UniString base64Image = ConvertImageToBase64(photoPath);
    
    // Send to JavaScript
    browser.ExecuteJavaScript("if (typeof ShowPhoto === 'function') { " +
        "ShowPhoto('" + base64Image + "', " + 
        GS::UniString::Printf("%d", index + 1) + ", " + 
        GS::UniString::Printf("%d", currentPhotoRelPaths.GetSize()) + "); }");
}
```

**Alternative**: Keep file paths, let JavaScript load images from filesystem

### Step 4.2: Photo Navigation Functions
**Functions**: `DoPrevPhoto()`, `DoNextPhoto()`, `DoDeletePhoto()`
**New Approach**: JavaScript array management with callbacks to C++

```cpp
// Example: Delete photo
void MergedPluginPalette::DoDeletePhoto() {
    if (currentPhotoRelPaths.IsEmpty() || currentPhotoIndex >= currentPhotoRelPaths.GetSize())
        return;
    
    // Remove from array
    deletedPhotoRelPaths.Push(currentPhotoRelPaths[currentPhotoIndex]);
    currentPhotoRelPaths.Delete(currentPhotoIndex);
    
    // Update JavaScript UI
    browser.ExecuteJavaScript("if (typeof UpdatePhotoList === 'function') { " +
        "UpdatePhotoList(" + ConvertToJavaScriptArray(currentPhotoRelPaths) + "); }");
    
    // Show next photo or clear
    if (!currentPhotoRelPaths.IsEmpty()) {
        if (currentPhotoIndex >= currentPhotoRelPaths.GetSize())
            currentPhotoIndex = currentPhotoRelPaths.GetSize() - 1;
        ShowPhotoAtIndex(currentPhotoIndex);
    } else {
        browser.ExecuteJavaScript("if (typeof ClearPhotoDisplay === 'function') ClearPhotoDisplay();");
    }
}
```

## Phase 5: Header and Member Variable Cleanup (Easy - 1 hour)

### Step 5.1: Update Header File
Replace `MergedPluginPalette.hpp` with clean version:

```cpp
// Use MergedPluginPalette_clean.hpp as new header
// Contains only browser member variable
// Remove all DG control member variables
```

### Step 5.2: Remove DG Control Member Variables
**Current**: 16 DG control variables
**After**: Only `browser` remains

## Phase 6: JavaScript Bridge Enhancement (Medium - 2-3 hours)

### Step 6.1: Add Missing JavaScript Functions
Ensure HTML interface can access all needed functionality:

```cpp
// In InitializeJavaScriptBridge(), verify all functions exist:
// ✅ GetSelectedElements
// ✅ GetAllIFCProperties
// ✅ GetCachedIFCProperties
// ✅ GetCachedIFCPropertiesWithRefresh
// ✅ ClearIFCPropertyCache
// ✅ GetIFCPropertyCacheSize
// ✅ GetPluginVersion
// ✅ ComponentInfo_GetComponentInfo
// ✅ ComponentInfo_SaveComponentInfo
// ✅ ComponentInfo_SelectPhotos
// ✅ ComponentInfo_ViewPhotos
// ➕ ComponentInfo_UpdatePhotoDisplay (new - for photo updates)
// ➕ ComponentInfo_NavigatePhotos (new - for prev/next)
```

### Step 6.2: Add JavaScript Callback Functions
C++ needs to call JavaScript for UI updates:

```cpp
// Add to InitializeJavaScriptBridge() or separate function
void MergedPluginPalette::RegisterJavaScriptCallbacks() {
    // These JavaScript functions must exist in HTML
    browser.RegisterJavaScriptCallback("UpdateComponentInfoUI");
    browser.RegisterJavaScriptCallback("ClearComponentInfoUI");
    browser.RegisterJavaScriptCallback("ToggleEditMode");
    browser.RegisterJavaScriptCallback("ShowPhoto");
    browser.RegisterJavaScriptCallback("ClearPhotoDisplay");
    browser.RegisterJavaScriptCallback("UpdatePhotoList");
}
```

## Phase 7: HTML Interface Updates (Medium - 2-3 hours)

### Step 7.1: Ensure HTML Has Required Elements
Verify `Selection_Test.html` contains:

1. **ComponentInfo Tab**:
   - `#selected-element` - Element identifier display
   - `#element-classification` - Type/tag display
   - `#component-id` - Editable component ID
   - `#photo-description` - Editable description
   - `#photo-preview` - Image display area
   - `#photo-index` - "1/3" display
   - `#photo-paths-count` - Photo count display
   - Edit mode toggle buttons
   - Photo navigation buttons

2. **IFC Properties Tab**: Already complete

### Step 7.2: Add Required JavaScript Functions
Add to `Selection_Test.html`:

```javascript
// UI Update Functions (called from C++)
function UpdateComponentInfoUI(guid, typeName, elemID) {
    document.getElementById('selected-element').textContent = elemID || guid;
    document.getElementById('element-classification').textContent = typeName || 'Unknown';
    // Load component info via ACAPI.ComponentInfo_GetComponentInfo
}

function ClearComponentInfoUI() {
    document.getElementById('selected-element').textContent = 'No element selected';
    document.getElementById('element-classification').textContent = '';
    document.getElementById('component-id').value = '';
    // ... clear all fields
}

function ToggleEditMode(isEditMode) {
    // Show/hide appropriate buttons, enable/disable inputs
}

function ShowPhoto(base64Image, currentIndex, totalCount) {
    document.getElementById('photo-preview').src = 'data:image/jpeg;base64,' + base64Image;
    document.getElementById('photo-index').textContent = `${currentIndex}/${totalCount}`;
}

// Photo management
let photoList = [];
let currentPhotoIndex = 0;

function UpdatePhotoList(newPhotoList) {
    photoList = newPhotoList;
    currentPhotoIndex = 0;
    if (photoList.length > 0) {
        // Load first photo
        LoadPhotoFromPath(photoList[0]);
    } else {
        ClearPhotoDisplay();
    }
}

function ClearPhotoDisplay() {
    document.getElementById('photo-preview').src = '';
    document.getElementById('photo-index').textContent = '0/0';
}
```

## Testing Strategy

### Pre-Migration Tests
1. **Build Test**: Ensure plugin builds with current code
2. **Bridge Test**: Verify ACAPI functions work via console
3. **Interface Test**: Open palette, check HTML loads

### Post-Phase Tests (After Each Phase)

#### Phase 1 Test
- [ ] Plugin builds without DG control references in data functions
- [ ] Constructor compiles without DG initializations
- [ ] Palette opens without errors

#### Phase 2 Test
- [ ] RefreshContent() calls JavaScript successfully
- [ ] Element selection updates HTML UI
- [ ] Edit mode toggles via JavaScript

#### Phase 3 Test
- [ ] ButtonClicked() function removed
- [ ] HTML button event handlers work
- [ ] No event handling crashes

#### Phase 4 Test
- [ ] Photo display works (base64 or file paths)
- [ ] Photo navigation functional
- [ ] Photo deletion works

#### Phase 5 Test
- [ ] Header file clean (only browser member)
- [ ] All DG control variables removed
- [ ] Plugin builds with clean header

#### Phase 6 Test
- [ ] All ACAPI functions accessible
- [ ] JavaScript callbacks work from C++
- [ ] No missing function errors

#### Phase 7 Test
- [ ] HTML interface complete
- [ ] All JavaScript functions exist
- [ ] UI responsive and functional

## Rollback Plan

### Immediate Issues
1. **Build Failure**: Revert to last working commit
2. **Runtime Crash**: Check C++ logs for missing controls
3. **JavaScript Errors**: Enable CEF debugging (`defaults write com.graphisoft.debug DG.CefDebugPort 9222`)

### Fallback Options
1. **Minimal Interface**: Revert to `MinimalTest.html`
2. **Hybrid Mode**: Keep some DG controls if migration too complex
3. **Incremental Rollout**: Migrate one feature at a time

## Timeline Summary

| Phase | Description | Effort | Risk |
|-------|-------------|--------|------|
| 1 | Low-hanging fruit removal | 1-2h | Low |
| 2 | Core UI migration | 3-4h | Medium |
| 3 | Event handling migration | 2-3h | Medium |
| 4 | Photo management migration | 4-5h | High |
| 5 | Header/variable cleanup | 1h | Low |
| 6 | JavaScript bridge enhancement | 2-3h | Medium |
| 7 | HTML interface updates | 2-3h | Medium |
| **Testing** | Verification at each phase | 3-4h | - |
| **Total** | | **18-25h** | |

## Success Criteria

✅ **Technical**:
- Plugin builds without errors or warnings
- No DG control references in code
- JavaScript bridge fully functional
- HTML interface loads and works

✅ **Functional**:
- Element selection updates UI
- Component info editing works
- Photo management functional
- IFC properties display works
- Edit mode toggles correctly

✅ **Performance**:
- UI responsive (<100ms updates)
- No memory leaks
- Photo loading efficient

✅ **User Experience**:
- Intuitive interface
- Clear error messages
- Consistent behavior

## Next Steps

1. **Wait for user test results** of current plugin
2. **If bridge works**: Begin Phase 1 implementation
3. **If bridge issues**: Debug CEF/JavaScript first
4. **Proceed incrementally**: One phase at a time with testing

---

**Last Updated**: 2026-02-02  
**Analysis Based On**: explore agent report (bg_335203f6)  
**Plugin Version**: 0.2.2.33  
**Status**: Plan ready, awaiting bridge confirmation