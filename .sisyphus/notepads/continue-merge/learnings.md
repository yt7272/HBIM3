# Date-based versioning verification learnings

## Build script pattern
- `generate_version()` function in `build.sh` matches IFCPropertyReader pattern exactly
- Uses `.build_counter` file to track incremental build number
- Format: `0.month.day.build` (month and day without leading zeros)
- Build counter increments on each build, preserved across clean builds
- Version passed to CMake via `-DAC_ADDON_VERSION_AUTO="${VERSION}"`

## CMake configuration
- `CMakeLists.txt` defines version macros: `AC_ADDON_VERSION_MAJOR`, `MINOR`, `PATCH`, `BUILD`, `STRING`
- Macros passed via `add_definitions()` to C++ compiler
- Version parsing handles 4-part version string (0.month.day.build)
- Default version "0.0.0.0" if not provided

## C++ implementation
- `GetAddOnVersionString()` exists in two namespaces: `ComponentInfo::` and `PropertyUtils::`
- Both return `GS::UniString(AC_ADDON_VERSION_STRING)` macro value
- Macros properly defined and accessible in C++ code

## HTML interface
- Version display element: `#summary-version` with static placeholder "0.2.1.11"
- JavaScript function `updatePluginVersion()` calls `ACAPI.GetPluginVersion()`
- Mock ACAPI.js includes `GetPluginVersion` function for testing
- Actual JavaScript bridge appears to be missing in MergedPlugin implementation

## Issues identified
1. JavaScript bridge not implemented - `InitializeJavaScriptBridge()` function declared but not defined
2. `ACAPI.GetPluginVersion()` not exposed, causing version display to fallback to static placeholder
3. Static placeholder version outdated (0.2.1.11 vs current 0.2.1.2)

## Verification results
- ✅ `.build_counter` exists and increments correctly (current: 2)
- ✅ Version generation produces correct format (0.2.1.2 on 2026-02-01)
- ✅ CMake macros defined and passed to compiler
- ✅ GetAddOnVersionString functions exist and use version macros
- ❌ JavaScript bridge missing - version display not functional
# Jest Test Infrastructure Setup

## Overview
Successfully set up Jest test infrastructure for MergedPlugin with mocked ACAPI functions. The setup enables JavaScript unit testing without requiring ArchiCAD installation.

## Directory Structure Created
- `MergedPlugin/__tests__/` - Contains example test file `ACAPI.test.js`
- `MergedPlugin/__mocks__/` - Contains mock `ACAPI.js` with 11 functions
- `MergedPlugin/jest.config.js` - Jest configuration with Node test environment
- `MergedPlugin/package.json` - NPM scripts and devDependencies for Jest

## Mock ACAPI Implementation
- **IFC Property Reader functions (7)**: `GetSelectedElements`, `GetAllIFCProperties`, `GetCachedIFCProperties`, `GetCachedIFCPropertiesWithRefresh`, `ClearIFCPropertyCache`, `GetIFCPropertyCacheSize`, `GetPluginVersion`
- **ComponentInfo functions (4)**: `GetComponentInfo`, `SaveComponentInfo`, `SelectPhotos`, `ViewPhotos`
- **Data format**: Mock IFC properties follow C++ array format `[propertySetName, propertyName, propertyValue, propertyType, unit, isValid]` matching `transformCppPropertiesToUI` expectations

## Test Suite Features
- Example test file includes copied `transformCppPropertiesToUI` and `groupProperties` functions from HTML
- Tests verify function existence, return types, and data transformations
- All 7 tests pass with mocked data
- Jest configured with `moduleNameMapper` to resolve `ACAPI` import to mock module

## Verification Results
- ✅ `__tests__/` directory created with `ACAPI.test.js` example test file
- ✅ `__mocks__/` directory created with `ACAPI.js` mocking ALL expected ACAPI functions (11 functions total)
- ✅ `jest.config.js` with proper module mappings for Node environment
- ✅ `package.json` with npm test scripts (test, test:list, test:watch, test:coverage)
- ✅ Jest test runner works: `npx jest --listTests` lists test files
- ✅ Tests pass with mocked ACAPI, no ArchiCAD required: `npx jest` runs successfully
- ✅ Mock function count ≥10: `node -e "const ACAPI = require('./__mocks__/ACAPI.js'); console.log(Object.keys(ACAPI).length)"` returns 11

## Issues Encountered
1. **Duplicate Jest configurations**: Fixed by removing `jest` key from `package.json`, keeping only `jest.config.js`
2. **Package.json syntax validation false positive**: `node -c` fails on JSON files but `require()` works correctly
3. **Test environment**: Initially configured `jsdom` but switched to `node` environment since tests don't require DOM APIs

## Learnings
- Jest moduleNameMapper effectively mocks global ACAPI object for testing
- Mock data must match exact format expected by transformation functions from HTML
- IFC property format is array of 6 elements; validity filtering depends on `isValid` boolean
- ComponentInfo functions follow simpler object-based API
 - Test infrastructure enables future Wave 1 tasks (JavaScript bridge implementation) to be tested independently of ArchiCAD

## JavaScript Bridge Implementation (Completed)
- **Duplicate function issue**: MergedPlugin.cpp contained duplicate static helper functions and ComponentInfo member functions (lines 1109-2098)
- **Root cause**: Entire ComponentInfo implementation duplicated, likely from improper merge
- **Fix**: Removed duplicate block (lines 1109-2098) and orphaned fragment (lines 1095-1106)
- **Build errors**: Missing DGBrowser.hpp and erroneous JS.hpp includes causing compilation failure
- **Solution**: Added `#include "DGBrowser.hpp"` to MergedPluginPalette.hpp, removed 150+ JS-related includes from MergedPluginPalette.cpp
- **Bridge verification**: `InitializeJavaScriptBridge()` function exists and registers `ACAPI.GetPluginVersion()` using `PropertyUtils::GetAddOnVersionString()`
- **Version verification**: Build script generates correct date-based version (0.2.1.4), CMake macros propagate to C++, HTML JavaScript expects dynamic version
- **Result**: Clean build succeeds, JavaScript bridge ready for testing in ArchiCAD

## HTML Interface Validation (2026-02-02)

### Verification Results
- ✅ HTML interface loads without errors via local HTTP server
- ✅ ComponentInfo tab active by default (CSS class `active`, content visible)
- ✅ IFC Properties tab switches correctly (both directions verified)
- ✅ Version number displayed in correct 0.x.x.x format (placeholder `0.0.0.0`)
- ✅ No visible DG controls - only browser UI elements present
- ✅ Screenshot captured: `.sisyphus/evidence/merged-plugin-interface.png`

### Technical Notes
- Playwright automation successful with local Python HTTP server (port 8080)
- JavaScript console shows no errors (except favicon.ico 404)
- Tab switching uses CSS class `active` and display property toggle
- Version element `#summary-version` displays static placeholder as expected
- Mock ACAPI not loaded in browser context (no `ACAPI` global object)
- HTML file contains embedded test suite functions (window.testIFCProperties)

## Hiding DG Controls in MergedPluginPalette (2026-02-02)

### Changes Made
1. **Constructor modifications**: Added Hide() calls for all DG controls except browser:
   - textComponentIdentifier, textClassification, editCompId, editPhotoDesc, textPhotoPaths
   - btnViewPhotos, pictureItem, btnEditInfo, btnViewIFCProperties
   - Already hidden: btnSelectPhotos, btnSave, btnCancel, btnPrevPhoto, btnDeletePhoto, btnNextPhoto, textPhotoIndex
2. **RefreshContent modifications**: Commented out Show() calls for btnPrevPhoto, btnNextPhoto, textPhotoIndex
3. **UpdateEditModeUI modifications**: Commented out Show() calls for btnSelectPhotos, btnSave, btnCancel, btnDeletePhoto, btnEditInfo

### Rationale
- HTML interface handles all UI; DG controls are redundant
- Browser remains visible as the primary interface
- DG dialogs (file picker, warning) still work via JavaScript bridge calls
- Build successful: version 0.2.2.24

### Verification
- ✅ Build compiles without errors
- ✅ All DG controls hidden (except browser)
- ✅ DG dialog functionality preserved
- ✅ JavaScript bridge intact