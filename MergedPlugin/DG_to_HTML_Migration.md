# DG Control to HTML Migration Plan

## Overview
This document details the migration of DG controls in MergedPluginPalette to pure HTML interface via JavaScript bridge. The goal is to remove all DG controls except the browser, moving all UI logic to HTML/JavaScript.

## Current DG Controls (from MergedPluginPalette.hpp)

| Control Variable | Type | Control ID | Purpose |
|------------------|------|------------|---------|
| textComponentIdentifier | DG::LeftText | 1 | Displays selected element identifier |
| textClassification | DG::LeftText | 2 | Displays element classification/tag |
| editCompId | DG::TextEdit | 4 | Editable component ID field |
| editPhotoDesc | DG::TextEdit | 6 | Editable photo description field |
| textPhotoPaths | DG::LeftText | 8 | Displays photo paths count |
| btnViewPhotos | DG::Button | 14 | "查看照片" button |
| pictureItem | DG::PictureItem | 9 | Displays photo preview |
| btnPrevPhoto | DG::Button | 10 | "<" previous photo button |
| btnDeletePhoto | DG::Button | 11 | "删除" delete photo button |
| textPhotoIndex | DG::CenterText | 12 | Displays photo index (e.g., "1/3") |
| btnNextPhoto | DG::Button | 13 | ">" next photo button |
| btnEditInfo | DG::Button | 15 | "填写信息" edit info button |
| btnSelectPhotos | DG::Button | 16 | "选择照片" select photos button |
| btnSave | DG::Button | 17 | "保存" save button |
| btnCancel | DG::Button | 18 | "取消" cancel button |
| btnViewIFCProperties | DG::Button | 19 | "查看IFC属性" view IFC properties button |
| browser | DG::Browser | 20 | HTML browser (kept) |

## Resource Reality
GDLG 32520 (palette) currently only contains browser control (ID 20). All other controls DO NOT EXIST in the resource, causing runtime errors when accessed.

## Migration Strategy

### Phase 1: Remove DG Control Dependencies from C++
1. **Update header**: Replace MergedPluginPalette.hpp with clean version (only browser)
2. **Remove member variables**: Delete all DG control variables from class definition
3. **Update constructor**: Remove initializations of DG controls
4. **Remove DG control references**: Update all methods that use DG controls

### Phase 2: Migrate UI Logic to JavaScript
For each DG control operation, implement equivalent HTML/JavaScript behavior:

#### Text Display Controls
- `textComponentIdentifier.SetText(text)` → Update HTML element `#selected-element`
- `textClassification.SetText(text)` → Update HTML element `#element-classification`
- `textPhotoPaths.SetText(text)` → Update HTML element `#photo-paths-count`
- `textPhotoIndex.SetText(text)` → Update HTML element `#photo-index`

#### Edit Controls
- `editCompId.SetText(text)` / `.GetText()` → HTML input `#component-id`
- `editPhotoDesc.SetText(text)` / `.GetText()` → HTML textarea `#photo-description`

#### Button Event Handling
Current C++ button click handlers (`ButtonClicked`) → HTML button `onclick` handlers calling ACAPI functions:

| Button | C++ Handler | JavaScript Equivalent |
|--------|-------------|------------------------|
| btnViewPhotos | `DoViewPhotos()` | `ACAPI.ComponentInfo_ViewPhotos(elementGuid)` |
| btnPrevPhoto | `DoPrevPhoto()` | Update photo index in JavaScript array |
| btnNextPhoto | `DoNextPhoto()` | Update photo index in JavaScript array |
| btnDeletePhoto | `DoDeletePhoto()` | Remove photo from array, update UI |
| btnEditInfo | `EnterEditMode()` | Toggle edit mode UI in HTML |
| btnSelectPhotos | `DoSelectPhotos()` | `ACAPI.ComponentInfo_SelectPhotos(elementGuid)` |
| btnSave | `ExitEditMode(true)` | `ACAPI.ComponentInfo_SaveComponentInfo([...])` |
| btnCancel | `ExitEditMode(false)` | Cancel edit, revert values |
| btnViewIFCProperties | `DoViewIFCProperties()` | Switch to IFC Properties tab, load properties |

#### Picture Control
- `pictureItem.SetPicture(image)` → HTML img element `#photo-preview` with base64 data URL

### Phase 3: Update HTML Interface
Ensure `Selection_Test.html` provides:
1. All necessary DOM elements with matching IDs
2. JavaScript functions to update UI based on C++ data
3. Event handlers for buttons
4. Edit mode toggle logic
5. Photo gallery with navigation

### Phase 4: Enhance JavaScript Bridge
Add any missing ACAPI functions needed by HTML interface:
- `ComponentInfo_GetComponentInfo` (already exists)
- `ComponentInfo_SaveComponentInfo` (exists)
- `ComponentInfo_SelectPhotos` (exists)
- `ComponentInfo_ViewPhotos` (exists)
- `ComponentInfo_GetPhotoPaths` (exists via GetComponentInfo)
- `ComponentInfo_UpdatePhotoPreview` (new? maybe not needed)

## Function-by-Function Analysis

### MergedPluginPalette.cpp Functions Using DG Controls

*(To be filled after explore agent analysis)*

### Safe Removal Order
1. **Low-risk**: Remove text display controls (no event handling)
2. **Medium-risk**: Remove edit controls (need JavaScript data binding)
3. **High-risk**: Remove button event handling (requires HTML button implementation)
4. **Complex**: Remove picture control (requires image loading via JavaScript)

## Testing Checklist
- [ ] Plugin builds without errors after each removal step
- [ ] Palette opens without crashes
- [ ] JavaScript bridge still functional (ACAPI object available)
- [ ] HTML interface loads correctly
- [ ] Element selection updates HTML UI
- [ ] Edit mode toggles via JavaScript
- [ ] Photo selection works via file dialog
- [ ] IFC properties load in tab
- [ ] No memory leaks or crashes

## Rollback Plan
If issues arise:
1. Revert to previous commit
2. Restore MinimalTest.html in resource
3. Enable CEF debugging for detailed diagnostics

## Timeline Estimate
- Phase 1: 3-4 hours
- Phase 2: 4-6 hours
- Phase 3: 1-2 hours
- Phase 4: 1-2 hours
- Testing: 2-3 hours
**Total**: 11-17 hours

## Notes
- Keep browser control (ID 20) as only DG control
- Maintain backward compatibility with existing JavaScript bridge API
- Ensure thread safety for JavaScript callbacks
- Add comprehensive logging for debugging

---

*Last Updated: 2026-02-02*
*Plugin Version: 0.2.2.33*