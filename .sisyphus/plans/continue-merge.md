# Continue ComponentInfo + IFCPropertyReader Plugin Merge

## TL;DR

> **Quick Summary**: Complete the plugin merge that was started but left incomplete. Currently MergedPlugin is just renamed ComponentInfo without functional integration. Implement missing JavaScript API bridge, GlobalId photo storage, basic IFC reading, and unified test suite with Jest automation.
> 
> **Deliverables**:
> - Working JavaScript API bridge (10+ functions)
> - GlobalId-based photo storage with DG warning dialog for missing IDs
> - Basic IFC property reading (no caching, pass-through stubs)
> - ComponentInfo JavaScript API with DG file picker
> - Jest test infrastructure with mocked ACAPI
> - Updated HTML interface with hidden DG controls (keep DG dialogs for file picker/warning)
> - Complete tasks 3-8 from original plan
> 
> **Estimated Effort**: Large
> **Parallel Execution**: YES - 3 waves
> **Critical Path**: Test infrastructure → JavaScript API bridge → GlobalId integration → IFC reading

---

## Context

### Original Request
Complete the plugin merge that was started but left incomplete. User tested MergedPlugin and found it identical to ComponentInfo (no IFCPropertyReader functionality integrated).

### Current State Analysis
**MergedPlugin is just renamed ComponentInfo with missing core functions**:
1. **JavaScript API Bridge NOT Working**: C++ doesn't register necessary JavaScript functions, HTML can't communicate with C++
2. **GlobalId Photo Storage NOT Implemented**: Still uses GUID folder naming instead of GlobalId property value
3. **IFC Property Reading NOT Integrated**: HTML calls `ACAPI.GetAllIFCProperties` but C++ doesn't implement
4. **ComponentInfo JavaScript API Missing**: HTML expects `GetComponentInfo`, `SaveComponentInfo`, `SelectPhotos`, `ViewPhotos` but none exist
5. **No Unified Test Suite**: Only copied testIFCProperties from IFCPropertyReader, no tests for merged functionality

### Original Plan Status (componentinfo-ifc-merge.md)
- ✅ Task 1: Create merged project structure (COMPLETED)
- ✅ Task 2: Implement HTML interface with tabs (COMPLETED)  
- ⬜ Task 3: Set up date-based versioning (PARTIAL - build.sh exists, needs verification)
- ⬜ Task 4: Integrate ComponentInfo functionality with GlobalId (NOT STARTED)
- ⬜ Task 5: Integrate basic IFC property reading (NOT STARTED)
- ⬜ Task 6: Implement JavaScript API bridge (NOT STARTED)
- ⬜ Task 7: Implement unified test suite (NOT STARTED)
- ⬜ Task 8: Final integration and validation (NOT STARTED)

### Key Decisions Made (from interview)

**Scope & Priority**:
1. **JavaScript Bridge First**: Highest priority - fix HTML-C++ communication
2. **Basic IFC Reading Only**: No caching, no advanced features
3. **GlobalId Strict Requirement**: Require GlobalId property, no fallback to GUID
4. **HTML-Only UI with DG Exceptions**: Hide DG controls, keep DG dialogs for file picker and warning
5. **Test Automation**: JavaScript automation with Jest (no C++ unit tests)

**Detailed Decisions**:
- **JavaScript Bridge Scope**: Implement ALL functions HTML expects for full compatibility (10+ functions)
- **GlobalId Missing Behavior**: Warning + manual entry via DG dialog (exception to "hide DG controls")
- **ComponentInfo JS API**: Full API with DG file picker (DG dialog for file selection)
- **Data Format**: Convert IFC property string to PropertyInfo object array in C++
- **Caching**: Pass-through stubs - cache functions call basic IFC reading functions
- **DG Controls**: Hide DG UI elements, keep browser only, but allow DG dialogs for file picker and warning
- **Testing**: Comprehensive coverage with Jest, mock ACAPI, no ArchiCAD required

### Research Findings (from explore agent)
- **Test Infrastructure**: Manual JavaScript tests in ArchiCAD browser console, no automated framework
- **Existing Patterns**: IFCPropertyReader has complete JavaScript API bridge pattern (`IFCPropertyReaderPalette.cpp`)
- **GlobalId Lookup**: IFCPropertyReader already searches for GlobalId, Tag, ID, Identifier properties
- **Photo Storage**: ComponentInfo uses `GuidToFolderName()`, needs `GlobalIdToFolderName()`
- **Data Format**: `GetElementIFCProperties()` returns formatted string, HTML expects PropertyInfo array

### Metis Review
**Critical Gaps Identified and Addressed**:
1. **ComponentInfo JavaScript API Scope**: Will implement full API with DG file picker
2. **GlobalId Implementation**: DG warning dialog for missing GlobalId, manual entry stored as custom property
3. **Caching Implementation**: Pass-through stubs for cache functions (call `GetAllIFCProperties`)
4. **DG Controls Specifics**: Hide `MergedPluginPalette` DG controls, keep DG dialogs for file picker/warning
5. **Testing Infrastructure**: Jest with mocked ACAPI, comprehensive test coverage
6. **Build Integration**: Handle resource ID conflicts, integrate CMakeLists.txt properly

---

## Work Objectives

### Core Objective
Complete the plugin merge by implementing missing JavaScript API bridge, GlobalId photo storage, basic IFC reading, and Jest test infrastructure, delivering a fully functional merged plugin with HTML interface.

### Concrete Deliverables
1. **Working JavaScript API Bridge**: 10+ functions exposed via `ACAPI` object in HTML
2. **GlobalId Photo Storage**: `GlobalIdToFolderName()` function, DG warning dialog for missing GlobalId
3. **Basic IFC Property Reading**: `GetAllIFCProperties()` using IFCPropertyReader's implementation (no caching)
4. **ComponentInfo JavaScript API**: `GetComponentInfo`, `SaveComponentInfo`, `SelectPhotos` (DG file picker), `ViewPhotos`
5. **Jest Test Infrastructure**: Test suite with mocked ACAPI, comprehensive coverage
6. **Updated HTML Interface**: Hidden DG controls, functional tabs, version display
7. **Build Verification**: Date-based versioning working, bundle structure correct

### Definition of Done
- [ ] JavaScript API bridge works - all `ACAPI` functions respond without errors
- [ ] ComponentInfo tab allows entering component ID, photo description, photo selection via DG file picker
- [ ] Photos stored in `{project}_构件照片/{GlobalId}/` directory structure
- [ ] Elements without GlobalId show DG warning dialog allowing manual entry
- [ ] IFC Properties tab displays basic IFC properties when "查看构件的IFC属性" button clicked
- [ ] Cache functions (`GetCachedIFCProperties`, `ClearIFCPropertyCache`) exist as pass-through stubs
- [ ] Jest test suite passes with mocked ACAPI (no ArchiCAD required)
- [ ] DG controls hidden from palette (only browser visible)
- [ ] Version displays in date-based format (0.month.day.build)
- [ ] All unified manual tests pass in ArchiCAD browser console

### Must Have
- Complete JavaScript API bridge (ALL functions HTML expects)
- GlobalId-based photo storage with DG warning dialog
- Basic IFC reading (no caching, pass-through stubs for cache functions)
- ComponentInfo JavaScript API with DG file picker
- Jest test infrastructure with mocked ACAPI
- Hidden DG controls (keep DG dialogs for file picker/warning)
- Date-based versioning verification
- Comprehensive test coverage

### Must NOT Have (Guardrails)
- **NO caching system** - Pass-through stubs only, no LRU cache implementation
- **NO performance optimization** - Exclude lazy loading, benchmarking, advanced features
- **NO fallback to GUID** - Strictly require GlobalId (manual entry via warning dialog)
- **NO separate modals** - Keep everything in HTML interface except DG dialogs for file picker/warning
- **NO C++ unit tests** - Focus on JavaScript automation with Jest
- **NO AI slop patterns** - No over-engineering, no premature abstraction, minimal error handling

---

## Verification Strategy

### Test Decision
- **Infrastructure exists**: NO (no automated test framework)
- **User wants tests**: YES (JavaScript automation with Jest)
- **Framework**: Jest with mocked ACAPI
- **Approach**: Comprehensive test coverage, mocked ACAPI, no ArchiCAD required

### Jest Test Infrastructure Setup
**Before implementing features**, set up Jest test framework:

**Test Structure**:
1. **Test Directory**: `MergedPlugin/__tests__/` with `.test.js` files
2. **Mock ACAPI**: `MergedPlugin/__mocks__/ACAPI.js` simulating all expected functions
3. **Configuration**: `jest.config.js` with proper module mappings
4. **Test Runner**: `npm test` or `bun test` command

**TDD Workflow**:
1. **RED**: Write failing Jest test for feature
2. **GREEN**: Implement minimum code to pass (C++ + JavaScript)
3. **REFACTOR**: Clean up while keeping tests green
4. **VERIFY**: Run Jest test suite after each change

### Automated Verification (Agent-Executable)

**For Jest Tests** (using Bash):
```bash
# Agent runs:
cd MergedPlugin && npm test -- --coverage
# Assert: All tests pass (exit code 0)
# Assert: Coverage report generated (lines, statements, functions)
```

**For Build Verification** (using Bash):
```bash
# Agent runs:
cd MergedPlugin && ./build.sh 2>&1 | tail -10
# Assert: Output contains "Build succeeded" and no errors
# Assert: Bundle structure exists: Release/MergedPlugin.bundle/
```

**For JavaScript API Verification** (using Bash):
```bash
# Agent runs:
grep -o "ACAPI\.[A-Za-z]*" MergedPlugin/RFIX/Selection_Test.html | sort -u | wc -l
# Assert: At least 10 unique ACAPI functions found
```

**For HTML Interface Verification** (using playwright skill):
```bash
# Agent executes via playwright browser automation:
1. Load: MergedPlugin/RFIX/Selection_Test.html
2. Verify: ComponentInfo tab active by default
3. Verify: IFC Properties tab exists and accessible
4. Verify: No visible DG controls (browser only)
5. Verify: Version number displayed in 0.x.x.x format
6. Screenshot: .sisyphus/evidence/merged-plugin-interface.png
```

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Start Immediately):
├── Task 0: Set up Jest test infrastructure
├── Task 3: Verify date-based versioning
└── Task 6: Implement JavaScript API bridge (core)

Wave 2 (After Wave 1):
├── Task 4: Integrate ComponentInfo with GlobalId + DG warning
├── Task 5: Integrate basic IFC reading + pass-through stubs
└── Task 6b: Implement ComponentInfo JavaScript API

Wave 3 (After Wave 2):
├── Task 7: Convert unified test suite to Jest
├── Task 8: Final integration and validation
└── Task 9: Hide DG controls (palette cleanup)
```

### Dependency Matrix

| Task | Depends On | Blocks | Can Parallelize With |
|------|------------|--------|---------------------|
| 0 | None | 4, 5, 6, 6b | 3 |
| 3 | None | 4, 5, 6 | 0 |
| 6 | 0, 3 | 4, 5, 6b, 7 | None (critical path) |
| 4 | 0, 3, 6 | 7, 8 | 5, 6b |
| 5 | 0, 3, 6 | 7, 8 | 4, 6b |
| 6b | 0, 3, 6 | 7, 8 | 4, 5 |
| 7 | 4, 5, 6b | 8, 9 | None |
| 8 | 7 | 9 | None |
| 9 | 8 | None | None |

**Critical Path**: Task 0 → Task 3 → Task 6 → Task 4 → Task 7 → Task 8 → Task 9

### Agent Dispatch Summary

| Wave | Tasks | Recommended Agents |
|------|-------|-------------------|
| 1 | 0, 3, 6 | delegate_task(category="ultrabrain", load_skills=["frontend-ui-ux", "git-master"], run_in_background=true) |
| 2 | 4, 5, 6b | delegate_task(category="ultrabrain", load_skills=["git-master"], run_in_background=true) |
| 3 | 7, 8, 9 | delegate_task(category="ultrabrain", load_skills=["playwright", "git-master"], run_in_background=true) |

---

## TODOs

> Implementation + Test = ONE Task. Never separate.
> EVERY task MUST have: Recommended Agent Profile + Parallelization info.

- [ ] 0. Set up Jest test infrastructure

  **What to do**:
  - Create test directory structure: `MergedPlugin/__tests__/` and `MergedPlugin/__mocks__/`
  - Create `MergedPlugin/__mocks__/ACAPI.js` mocking ALL expected ACAPI functions (10+ functions)
  - Create `jest.config.js` with proper module mappings for browser environment
  - Create example test file `MergedPlugin/__tests__/ACAPI.test.js` verifying mock functions
  - Add npm scripts to `package.json` or configure `bun test` if not present
  - Ensure tests run without ArchiCAD (mocked ACAPI only)

  **Must NOT do**:
  - Don't create C++ unit tests
  - Don't require ArchiCAD installation for tests
  - Don't implement real ACAPI functionality (mock only)

  **Recommended Agent Profile**:
  - **Category**: `ultrabrain`
    - Reason: Test infrastructure setup requires understanding of both JavaScript testing and plugin architecture
  - **Skills**: [`frontend-ui-ux`, `git-master`]
    - `frontend-ui-ux`: For JavaScript test structure and mocking patterns
    - `git-master`: For proper file organization and configuration

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 3, 6)
  - **Blocks**: Tasks 4, 5, 6b, 7
  - **Blocked By**: None (can start immediately)

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing test patterns to follow):
  - `IFCPropertyReader/RFIX/Selection_Test.html:window.testIFCProperties` - JavaScript test object structure
  - `IFCPropertyReader/RFIX/Selection_Test.html:ACAPI function calls` - Expected ACAPI function signatures
  - `validate_merged_interface.js` - Node.js validation script structure

  **Jest Configuration References**:
  - Official Jest documentation for browser mocking: https://jestjs.io/docs/configuration
  - Example jest.config.js for browser environment:
    ```javascript
    module.exports = {
      testEnvironment: 'jsdom',
      moduleNameMapper: {
        '^ACAPI$': '<rootDir>/__mocks__/ACAPI.js'
      }
    };
    ```

  **WHY Each Reference Matters**:
  - Existing test patterns show expected ACAPI function signatures
  - Jest configuration ensures proper browser environment simulation
  - Mock ACAPI must match exactly what HTML expects

  **Acceptance Criteria**:

  **Automated Verification (Agent-Executable)**:

  **For Jest Setup** (using Bash):
  ```bash
  # Agent runs:
  ls MergedPlugin/__tests__/ MergedPlugin/__mocks__/
  # Assert: Both directories exist
  cat MergedPlugin/__mocks__/ACAPI.js | grep -c "function"
  # Assert: At least 10 mock functions defined
  node -e "const ACAPI = require('./MergedPlugin/__mocks__/ACAPI.js'); console.log(Object.keys(ACAPI).length)"
  # Assert: ACAPI mock exports expected functions
  cd MergedPlugin && npm test -- --listTests 2>/dev/null || bun test --list 2>/dev/null || echo "Tests found"
  # Assert: Test runner works and lists test files
  ```

  **Evidence to Capture**:
  - [ ] Directory structure listing
  - [ ] Mock ACAPI function count verification
  - [ ] Jest configuration file existence
  - [ ] Test runner execution output

  **Commit**: YES (groups with 0)
  - Message: `test(infra): set up Jest test infrastructure with mocked ACAPI`
  - Files: `MergedPlugin/__tests__/`, `MergedPlugin/__mocks__/`, `MergedPlugin/jest.config.js`
  - Pre-commit: `ls MergedPlugin/__tests__/ MergedPlugin/__mocks__/`

- [ ] 3. Verify date-based versioning

  **What to do**:
  - Verify date-based versioning is already implemented in `MergedPlugin/build.sh`
  - Check `.build_counter` file exists and increments on each build
  - Verify CMakeLists.txt defines version macros (`AC_ADDON_VERSION_MAJOR`, etc.)
  - Ensure version is accessible in C++ code via `GetAddOnVersionString()` function
  - Verify version displays in HTML interface (check RFIX/Selection_Test.html)
  - Fix any issues found (missing components, incorrect format)

  **Must NOT do**:
  - Don't rewrite versioning system if it already works
  - Don't change version format (must be 0.month.day.build)
  - Don't remove existing implementation

  **Recommended Agent Profile**:
  - **Category**: `ultrabrain`
    - Reason: Build system verification requires CMake and bash scripting expertise
  - **Skills**: [`git-master`]
    - `git-master`: For build script and CMake configuration analysis

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 0, 6)
  - **Blocks**: Tasks 4, 5, 6
  - **Blocked By**: None (can start immediately)

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing code to follow):
  - `IFCPropertyReader/build.sh:20-50` - Version generation function with build counter
  - `IFCPropertyReader/CMakeLists.txt:135-179` - CMake version parameter handling
  - `IFCPropertyReader/Src/IFCPropertyUtils.cpp:GetAddOnVersionString` - Version string retrieval
  - `MergedPlugin/build.sh` - Current implementation to verify

  **Version Format References**:
  - Expected format: `0.month.day.build` (e.g., 0.2.1.5 for Feb 1, build 5)
  - Build counter: `.build_counter` file stores incremental build number
  - CMake macros: `AC_ADDON_VERSION_MAJOR`, `AC_ADDON_VERSION_MINOR`, etc.

  **WHY Each Reference Matters**:
  - IFCPropertyReader already implements the exact versioning scheme requested
  - Current MergedPlugin build script may need fixes to match pattern
  - Version must be accessible in both C++ and HTML for display

  **Acceptance Criteria**:

  **Automated Verification (Agent-Executable)**:

  **For Version Verification** (using Bash):
  ```bash
  # Agent runs:
  cd MergedPlugin && ./build.sh clean
  # Assert: .build_counter file created
  cat .build_counter
  # Assert: Contains build number (should be 1 after clean)
  grep -A 10 "AC_ADDON_VERSION" CMakeLists.txt
  # Assert: Version macro definitions exist
  grep "generate_version" build.sh
  # Assert: Version generation function exists
  grep "0\.[0-9]*\.[0-9]*\.[0-9]*" build.sh
  # Assert: Version pattern found
  ```

  **For HTML Version Display** (using playwright skill):
  ```bash
  # Agent executes via playwright browser automation:
  1. Load: MergedPlugin/RFIX/Selection_Test.html
  2. Extract: Version text from page
  3. Assert: Version matches pattern 0.x.x.x
  4. Screenshot: .sisyphus/evidence/version-display.png
  ```

  **Evidence to Capture**:
  - [ ] Build counter file content
  - [ ] CMakeLists.txt version configuration
  - [ ] Build script version generation verification
  - [ ] HTML version display screenshot

  **Commit**: YES (groups with 3)
  - Message: `fix(version): verify and fix date-based versioning`
  - Files: `MergedPlugin/build.sh`, `MergedPlugin/CMakeLists.txt`, `MergedPlugin/RFIX/Selection_Test.html`
  - Pre-commit: `grep "0\." MergedPlugin/build.sh`

- [ ] 6. Implement JavaScript API bridge (core IFC functions)

  **What to do**:
  - Extract exact JavaScript API requirements from HTML: `grep -o "ACAPI\.[A-Za-z]*" MergedPlugin/RFIX/Selection_Test.html | sort -u`
  - Implement C++ JavaScript API bridge following IFCPropertyReader pattern (`IFCPropertyReaderPalette.cpp`)
  - Core IFC functions: `GetSelectedElements`, `GetAllIFCProperties`, `GetCachedIFCProperties`, `ClearIFCPropertyCache`, `GetIFCPropertyCacheSize`, `GetPluginVersion`
  - Pass-through stub implementation for cache functions (call `GetAllIFCProperties` internally)
  - Register JavaScript functions in `MergedPluginMain.cpp` using `ACAPI_RegisterJavaScriptAPI`
  - Ensure error handling for missing elements, API failures
  - Update HTML interface to use new API (if needed)

  **Must NOT do**:
  - Don't implement actual caching (pass-through stubs only)
  - Don't create separate API namespaces (use single `ACAPI` object)
  - Don't skip error handling for missing GlobalId
  - Don't modify existing ComponentInfo C++ logic (yet)

  **Recommended Agent Profile**:
  - **Category**: `ultrabrain`
    - Reason: Complex C++/JavaScript bridge implementation requiring ArchiCAD API expertise
  - **Skills**: [`git-master`, `frontend-ui-ux`]
    - `git-master`: For C++ API implementation and ArchiCAD API usage
    - `frontend-ui-ux`: For JavaScript integration and user feedback patterns

  **Parallelization**:
  - **Can Run In Parallel**: NO (critical path)
  - **Parallel Group**: Wave 1 (with Tasks 0, 3)
  - **Blocks**: Tasks 4, 5, 6b, 7
  - **Blocked By**: Tasks 0, 3 (test infrastructure, versioning)

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing code to follow):
  - `IFCPropertyReader/Src/IFCPropertyReaderPalette.cpp:170-210` - Complete JavaScript API bridge implementation
  - `IFCPropertyReader/Src/Main.cpp:RegisterInterface` - Interface registration pattern
  - `IFCPropertyReader/RFIX/Selection_Test.html:ACAPI function calls` - Expected JavaScript function signatures
  - `IFCPropertyReader/Src/IFCPropertyUtils.cpp:GetAllIFCPropertiesForElement` - Property reading implementation

  **Function Signature References**:
  - `GetSelectedElements()` → Returns `[{guid, typeName, elemID}, ...]`
  - `GetAllIFCProperties(guid)` → Returns `PropertyInfo[]` array
  - `GetCachedIFCProperties(guid)` → Pass-through to `GetAllIFCProperties`
  - `ClearIFCPropertyCache()` → No-op (log "cache cleared")
  - `GetIFCPropertyCacheSize()` → Returns `0` (no cache)
  - `GetPluginVersion()` → Returns version string from `GetAddOnVersionString()`

  **WHY Each Reference Matters**:
  - IFCPropertyReaderPalette.cpp shows complete bridge pattern to copy
  - Function signatures must match exactly what HTML expects
  - Property reading implementation already exists in IFCPropertyReader

  **Acceptance Criteria**:

  **Automated Verification (Agent-Executable)**:

  **For JavaScript API Bridge** (using Bash):
  ```bash
  # Agent runs:
  grep -c "ACAPI_RegisterJavaScriptAPI" MergedPlugin/Src/MergedPluginMain.cpp
  # Assert: At least 1 registration call exists
  grep -A 5 "GetAllIFCProperties" MergedPlugin/Src/MergedPluginMain.cpp
  # Assert: Function registration for GetAllIFCProperties exists
  grep -o "ACAPI\.[A-Za-z]*" MergedPlugin/RFIX/Selection_Test.html | sort -u > /tmp/expected.txt
  grep -o "GetAllIFCProperties\|GetCachedIFCProperties\|ClearIFCPropertyCache\|GetIFCPropertyCacheSize\|GetPluginVersion" /tmp/expected.txt | wc -l
  # Assert: At least 5 core IFC functions are expected
  ```

  **For Jest Tests** (using Bash):
  ```bash
  # Agent runs:
  cd MergedPlugin && npm test -- --testNamePattern="API bridge"
  # Assert: Tests for API bridge functions pass (exit code 0)
  ```

  **For Build Verification** (using Bash):
  ```bash
  # Agent runs:
  cd MergedPlugin && ./build.sh 2>&1 | grep -i error
  # Assert: No build errors
  ```

  **Evidence to Capture**:
  - [ ] JavaScript API function registration code
  - [ ] Core IFC function implementations
  - [ ] Jest test results for API bridge
  - [ ] Build success verification

  **Commit**: YES (groups with 6)
  - Message: `feat(api): implement JavaScript API bridge core IFC functions`
  - Files: `MergedPlugin/Src/MergedPluginMain.cpp`, `MergedPlugin/Src/MergedPluginPalette.cpp`, `MergedPlugin/Src/IFCPropertyUtils.cpp`
  - Pre-commit: `grep "ACAPI_RegisterJavaScriptAPI" MergedPlugin/Src/MergedPluginMain.cpp`

- [ ] 4. Integrate ComponentInfo with GlobalId + DG warning dialog

  **What to do**:
  - Adapt ComponentInfo C++ logic to use GlobalId property value as folder name instead of GUID
  - Create `GlobalIdToFolderName()` function (similar to `GuidToFolderName`)
  - Implement GlobalId property lookup from selected element (use IFCPropertyReader's pattern)
  - Add error handling for elements without GlobalId: show DG warning dialog allowing manual entry
  - Store manual GlobalId entry as custom property "GlobalId_Manual" on element
  - Disable photo features if GlobalId missing (but allow IFC viewing)
  - Expose ComponentInfo functionality via JavaScript API (`GetComponentInfo`, `SaveComponentInfo`)
  - Keep DG file picker dialog for photo selection (as per user decision)

  **Must NOT do**:
  - Don't fallback to GUID when GlobalId is missing
  - Don't enable photo functionality for elements without GlobalId (unless manual entry provided)
  - Don't remove existing photo storage logic (adapt it)
  - Don't implement HTML file picker (use DG dialog)

  **Recommended Agent Profile**:
  - **Category**: `ultrabrain`
    - Reason: Complex C++ logic integration requiring ArchiCAD API and property system expertise
  - **Skills**: [`git-master`]
    - `git-master`: For C++ code integration and ArchiCAD API usage

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 5, 6b)
  - **Blocks**: Tasks 7, 8
  - **Blocked By**: Tasks 0, 3, 6 (test infra, versioning, API bridge)

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing code to follow):
  - `ComponentInfo/Src/ComponentInfo.cpp:312-332` - GuidToFolderName function (adapt for GlobalId)
  - `ComponentInfo/Src/ComponentInfo.cpp:334-365` - CopyPhotosForComponent function (modify for GlobalId)
  - `IFCPropertyReader/Src/IFCPropertyUtils.cpp:256-266` - GlobalId property lookup logic
  - `ComponentInfo/Src/ComponentInfoDialog.cpp` - DG dialog patterns for warning and file picker

  **GlobalId Lookup References**:
  - Property names to search: "GlobalId", "Tag", "ID", "Identifier" (use IFCPropertyReader's search order)
  - Manual entry storage: Create custom property "GlobalId_Manual" with `ACAPI_Element_SetProperty`
  - DG warning dialog: Use `DG::WarningDialog` or similar pattern from ComponentInfoDialog

  **WHY Each Reference Matters**:
  - ComponentInfo.cpp contains the core photo management logic that needs adaptation
  - IFCPropertyReader already has GlobalId lookup logic that can be reused
  - DG dialog patterns show how to create warning and file picker dialogs

  **Acceptance Criteria**:

  **Automated Verification (Agent-Executable)**:

  **For GlobalId Integration** (using Bash):
  ```bash
  # Agent runs:
  grep -A 20 "GlobalIdToFolderName" MergedPlugin/Src/ComponentInfoUtils.cpp
  # Assert: GlobalId folder naming function exists
  grep -A 10 "GlobalId.*property" MergedPlugin/Src/ComponentInfoUtils.cpp
  # Assert: GlobalId property lookup logic exists
  grep -A 5 "DG.*Warning" MergedPlugin/Src/ComponentInfoUtils.cpp
  # Assert: DG warning dialog for missing GlobalId exists
  grep -A 5 "GlobalId_Manual" MergedPlugin/Src/ComponentInfoUtils.cpp
  # Assert: Manual GlobalId storage logic exists
  ```

  **For Jest Tests** (using Bash):
  ```bash
  # Agent runs:
  cd MergedPlugin && npm test -- --testNamePattern="GlobalId"
  # Assert: Tests for GlobalId functionality pass (exit code 0)
  ```

  **Evidence to Capture**:
  - [ ] GlobalIdToFolderName implementation
  - [ ] GlobalId property lookup code
  - [ ] DG warning dialog code
  - [ ] Manual GlobalId storage logic

  **Commit**: YES (groups with 4)
  - Message: `feat(componentinfo): integrate GlobalId photo storage with DG warning dialog`
  - Files: `MergedPlugin/Src/ComponentInfoUtils.cpp`, `MergedPlugin/Src/ComponentInfoUtils.hpp`
  - Pre-commit: `grep "GlobalId" MergedPlugin/Src/*.cpp`

- [ ] 5. Integrate basic IFC reading + pass-through stubs

  **What to do**:
  - Copy basic IFC property reading logic from IFCPropertyReader: `GetAllIFCPropertiesForElement()`
  - Exclude caching, performance optimization, and advanced features
  - Convert returned formatted string to PropertyInfo object array (as per user decision)
  - Implement pass-through stub functions: `GetCachedIFCProperties`, `ClearIFCPropertyCache`, `GetIFCPropertyCacheSize`
  - Ensure IFC viewing works even for elements without GlobalId
  - Connect "查看构件的IFC属性" button to `ACAPI.GetAllIFCProperties()`
  - Update HTML interface to display PropertyInfo array (should already expect this format)

  **Must NOT do**:
  - Don't include LRU caching system (pass-through stubs only)
  - Don't implement `GetAllIFCProperties` (preview, attributes, classifications) - just basic properties
  - Don't add performance benchmarking or lazy loading
  - Don't modify existing IFCPropertyReader logic beyond adaptation

  **Recommended Agent Profile**:
  - **Category**: `ultrabrain`
    - Reason: C++ logic integration requiring ArchiCAD IFC API expertise
  - **Skills**: [`git-master`]
    - `git-master`: For C++ code integration and API usage

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 4, 6b)
  - **Blocks**: Tasks 7, 8
  - **Blocked By**: Tasks 0, 3, 6 (test infra, versioning, API bridge)

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing code to follow):
  - `IFCPropertyReader/Src/IFCPropertyUtils.cpp:191-287` - GetIFCPropertiesForElement basic implementation
  - `IFCPropertyReader/Src/IFCPropertyUtils.cpp:226-287` - Property iteration and basic property extraction
  - `IFCPropertyReader/RFIX/Selection_Test.html:976-980` - ACAPI.GetAllIFCProperties usage
  - `IFCPropertyReader/Src/IFCPropertyUtils.hpp:24-26` - Basic function declarations

  **Data Format Conversion**:
  - Current: `GetElementIFCProperties()` returns `GS::UniString` (formatted text)
  - Required: `GetAllIFCPropertiesForElement()` returns `GS::Array<PropertyInfo>`
  - PropertyInfo structure: `[propertySetName, propertyName, propertyValue, propertyType, unit, isValid]`
  - Conversion logic: Parse formatted string or rewrite function to return array directly

  **WHY Each Reference Matters**:
  - Basic IFC property reading logic provides the core functionality needed
  - HTML expects PropertyInfo array format, not formatted string
  - Pass-through stubs maintain API compatibility without implementing caching

  **Acceptance Criteria**:

  **Automated Verification (Agent-Executable)**:

  **For IFC Integration** (using Bash):
  ```bash
  # Agent runs:
  grep -A 30 "GetAllIFCPropertiesForElement" MergedPlugin/Src/IFCPropertyUtils.cpp
  # Assert: Basic IFC property reading function exists
  grep -v "GetCached.*Complex\|Advanced" MergedPlugin/Src/IFCPropertyUtils.cpp
  # Assert: No advanced caching implementations
  grep "PropertyInfo" MergedPlugin/Src/IFCPropertyUtils.cpp
  # Assert: Returns PropertyInfo array, not formatted string
  grep "pass-through\|stub" MergedPlugin/Src/IFCPropertyUtils.cpp
  # Assert: Cache functions are marked as pass-through stubs
  ```

  **For Jest Tests** (using Bash):
  ```bash
  # Agent runs:
  cd MergedPlugin && npm test -- --testNamePattern="IFC reading"
  # Assert: Tests for IFC property reading pass (exit code 0)
  ```

  **Evidence to Capture**:
  - [ ] GetAllIFCPropertiesForElement implementation
  - [ ] PropertyInfo array return type verification
  - [ ] Pass-through stub implementations
  - [ ] Jest test results for IFC reading

  **Commit**: YES (groups with 5)
  - Message: `feat(ifc): integrate basic IFC reading with pass-through stubs`
  - Files: `MergedPlugin/Src/IFCPropertyUtils.cpp`, `MergedPlugin/Src/IFCPropertyUtils.hpp`
  - Pre-commit: `grep -v "Cache\|All" MergedPlugin/Src/IFCPropertyUtils.cpp`

- [ ] 6b. Implement ComponentInfo JavaScript API

  **What to do**:
  - Implement ComponentInfo JavaScript API functions: `GetComponentInfo`, `SaveComponentInfo`, `SelectPhotos`, `ViewPhotos`
  - `SelectPhotos` uses DG file picker dialog (as per user decision)
  - `ViewPhotos` opens photo folder using system file explorer
  - Connect ComponentInfo tab form fields to API functions
  - Ensure data persistence via custom properties (component ID, photo description, photo paths)
  - Integrate with GlobalId photo storage (from Task 4)

  **Must NOT do**:
  - Don't create HTML file picker (use DG dialog)
  - Don't implement complex photo preview in HTML
  - Don't modify existing ComponentInfo property definitions

  **Recommended Agent Profile**:
  - **Category**: `ultrabrain`
    - Reason: C++/JavaScript bridge implementation requiring ComponentInfo logic integration
  - **Skills**: [`git-master`, `frontend-ui-ux`]
    - `git-master`: For C++ API implementation
    - `frontend-ui-ux`: For JavaScript integration and UI feedback

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 4, 5)
  - **Blocks**: Tasks 7, 8
  - **Blocked By**: Tasks 0, 3, 6 (test infra, versioning, API bridge)

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing code to follow):
  - `ComponentInfo/Src/ComponentInfoDialog.cpp` - DG file picker dialog implementation
  - `IFCPropertyReaderPalette.cpp` - JavaScript API bridge pattern
  - `ComponentInfo/Src/ComponentInfo.cpp` - Property reading/writing logic
  - `IFCPropertyReader/RFIX/Selection_Test.html:ACAPI.ComponentInfo` - Expected function signatures

  **Acceptance Criteria**:

  **Automated Verification (Agent-Executable)**:

  **For ComponentInfo API** (using Bash):
  ```bash
  # Agent runs:
  grep -A 5 "GetComponentInfo\|SaveComponentInfo\|SelectPhotos\|ViewPhotos" MergedPlugin/Src/MergedPluginMain.cpp
  # Assert: ComponentInfo API functions are registered
  grep "ACAPI\.ComponentInfo" MergedPlugin/RFIX/Selection_Test.html
  # Assert: HTML calls ComponentInfo API functions
  ```

  **For Jest Tests** (using Bash):
  ```bash
  # Agent runs:
  cd MergedPlugin && npm test -- --testNamePattern="ComponentInfo"
  # Assert: Tests for ComponentInfo API pass (exit code 0)
  ```

  **Evidence to Capture**:
  - [ ] ComponentInfo API function registrations
  - [ ] DG file picker integration
  - [ ] Jest test results for ComponentInfo API

  **Commit**: YES (groups with 6b)
  - Message: `feat(api): implement ComponentInfo JavaScript API with DG file picker`
  - Files: `MergedPlugin/Src/MergedPluginMain.cpp`, `MergedPlugin/Src/ComponentInfoUtils.cpp`
  - Pre-commit: `grep "ComponentInfo" MergedPlugin/Src/MergedPluginMain.cpp`

- [ ] 7. Convert unified test suite to Jest

  **What to do**:
  - Convert existing manual JavaScript tests (`window.testMergedPlugin`) to Jest test cases
  - Create test files: `__tests__/merged-plugin.test.js` covering ComponentInfo, IFC reading, GlobalId, API bridge
  - Use mocked ACAPI (from Task 0) to simulate plugin behavior
  - Ensure comprehensive test coverage: selection, property reading, photo storage, error handling
  - Add test for DG warning dialog behavior (mock)
  - Run Jest test suite and ensure all tests pass

  **Must NOT do**:
  - Don't remove existing manual test suite (keep as backup)
  - Don't require ArchiCAD for tests
  - Don't test C++ directly (only through mocked API)

  **Recommended Agent Profile**:
  - **Category**: `ultrabrain`
    - Reason: Test conversion requiring understanding of both manual tests and Jest framework
  - **Skills**: [`frontend-ui-ux`, `playwright`]
    - `frontend-ui-ux`: For JavaScript test structure and mocking
    - `playwright`: For test automation patterns

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 3 (with Tasks 8, 9)
  - **Blocks**: Tasks 8, 9
  - **Blocked By**: Tasks 4, 5, 6b (core functionality)

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing code to follow):
  - `IFCPropertyReader/RFIX/Selection_Test.html:window.testIFCProperties` - Manual test structure
  - `MergedPlugin/__mocks__/ACAPI.js` - Mock API for tests
  - Jest documentation for async testing and mocking

  **Acceptance Criteria**:

  **Automated Verification (Agent-Executable)**:

  **For Jest Test Suite** (using Bash):
  ```bash
  # Agent runs:
  cd MergedPlugin && npm test -- --coverage
  # Assert: All tests pass (exit code 0)
  # Assert: Coverage report shows ≥80% coverage
  ls __tests__/*.test.js
  # Assert: Test files exist for ComponentInfo, IFC, GlobalId, API
  ```

  **Evidence to Capture**:
  - [ ] Jest test suite execution results
  - [ ] Coverage report screenshot
  - [ ] Test file structure verification

  **Commit**: YES (groups with 7)
  - Message: `test(suite): convert unified test suite to Jest`
  - Files: `MergedPlugin/__tests__/`, `MergedPlugin/__mocks__/ACAPI.js`
  - Pre-commit: `npm test`

- [ ] 8. Final integration and validation

  **What to do**:
  - Perform end-to-end integration testing of merged plugin
  - Validate HTML interface functionality with both tabs
  - Test ComponentInfo functionality with GlobalId-based photo storage
  - Test IFC property reading with "查看构件的IFC属性" button
  - Verify error handling for elements without GlobalId (DG warning dialog)
  - Confirm date-based versioning displays correctly
  - Ensure Jest test suite passes all tests
  - Validate build process and bundle structure
  - Run manual tests in ArchiCAD browser console (if possible)

  **Must NOT do**:
  - Don't add new features beyond specified requirements
  - Don't modify core functionality that's already working
  - Don't skip validation steps

  **Recommended Agent Profile**:
  - **Category**: `ultrabrain`
    - Reason: Comprehensive integration and validation requiring full system understanding
  - **Skills**: [`playwright`, `git-master`]
    - `playwright`: For end-to-end interface validation
    - `git-master`: For build process and bundle validation

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 3 (with Tasks 7, 9)
  - **Blocks**: Task 9
  - **Blocked By**: Task 7

  **References** (CRITICAL - Be Exhaustive):

  **Validation References**:
  - `Definition of Done section` - Complete validation checklist
  - `ComponentInfo/README.md:51-55` - ComponentInfo installation and usage
  - `IFCPropertyReader/README.md:60-63` - IFCPropertyReader installation and usage

  **Acceptance Criteria**:

  **Automated Verification (Agent-Executable)**:

  **For Final Validation** (using Bash and playwright):
  ```bash
  # Agent runs build validation:
  cd MergedPlugin && ./build.sh
  # Assert: Build completes successfully
  find Release/MergedPlugin.bundle -type f | wc -l
  # Assert: Bundle contains expected number of files
  
  # Agent runs interface validation via playwright:
  1. Load HTML interface
  2. Verify ComponentInfo tab active by default
  3. Switch to IFC Properties tab
  4. Verify "查看构件的IFC属性" button exists
  5. Verify version number displayed in correct format
  6. Screenshot: .sisyphus/evidence/final-validation.png
  
  # Agent runs test suite validation:
  cd MergedPlugin && npm test
  # Assert: All Jest tests pass (exit code 0)
  ```

  **Evidence to Capture**:
  - [ ] Build success verification
  - [ ] Bundle structure validation
  - [ ] HTML interface screenshot
  - [ ] Jest test suite passing verification

  **Commit**: YES (groups with 8)
  - Message: `feat(final): complete integration and validation`
  - Files: `MergedPlugin/`
  - Pre-commit: `./build.sh && echo "Build successful"`

- [ ] 9. Hide DG controls (palette cleanup)

  **What to do**:
  - Identify DG controls in `MergedPluginPalette` that need hiding (non-dialog controls)
  - Hide or remove DG UI elements while keeping browser visible
  - Ensure DG dialogs for file picker and warning still work
  - Verify palette appears correctly with only browser component
  - Update palette resource file if needed

  **Must NOT do**:
  - Don't remove DG dialogs (file picker, warning)
  - Don't break browser component functionality
  - Don't modify dialog logic

  **Recommended Agent Profile**:
  - **Category**: `ultrabrain`
    - Reason: DG palette modification requiring ArchiCAD UI expertise
  - **Skills**: [`git-master`]
    - `git-master`: For C++ palette code modification

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 3 (final task)
  - **Blocks**: None
  - **Blocked By**: Task 8

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing code to follow):
  - `MergedPlugin/Src/MergedPluginPalette.cpp` - Current palette implementation
  - `IFCPropertyReader/Src/IFCPropertyReaderPalette.cpp` - Browser-only palette pattern

  **Acceptance Criteria**:

  **Automated Verification (Agent-Executable)**:

  **For DG Controls Hiding** (using Bash):
  ```bash
  # Agent runs:
  grep -c "DG::" MergedPlugin/Src/MergedPluginPalette.cpp
  # Assert: DG control references exist (for dialogs)
  grep -c "browser" MergedPlugin/Src/MergedPluginPalette.cpp
  # Assert: Browser component still present
  ```

  **For Playwright Verification** (using playwright skill):
  ```bash
  # Agent executes via playwright browser automation:
  1. Load plugin interface
  2. Verify: No visible DG controls (only browser)
  3. Screenshot: .sisyphus/evidence/dg-controls-hidden.png
  ```

  **Evidence to Capture**:
  - [ ] Palette code modification verification
  - [ ] Screenshot showing hidden DG controls

  **Commit**: YES (groups with 9)
  - Message: `ui(palette): hide DG controls, keep browser only`
  - Files: `MergedPlugin/Src/MergedPluginPalette.cpp`
  - Pre-commit: `grep "DG::" MergedPlugin/Src/MergedPluginPalette.cpp`

---

## Commit Strategy

| After Task | Message | Files | Verification |
|------------|---------|-------|--------------|
| 0 | `test(infra): set up Jest test infrastructure with mocked ACAPI` | `MergedPlugin/__tests__/`, `MergedPlugin/__mocks__/`, `MergedPlugin/jest.config.js` | `ls MergedPlugin/__tests__/ MergedPlugin/__mocks__/` |
| 3 | `fix(version): verify and fix date-based versioning` | `MergedPlugin/build.sh`, `MergedPlugin/CMakeLists.txt`, `MergedPlugin/RFIX/Selection_Test.html` | `grep "0\." MergedPlugin/build.sh` |
| 6 | `feat(api): implement JavaScript API bridge core IFC functions` | `MergedPlugin/Src/MergedPluginMain.cpp`, `MergedPlugin/Src/MergedPluginPalette.cpp`, `MergedPlugin/Src/IFCPropertyUtils.cpp` | `grep "ACAPI_RegisterJavaScriptAPI" MergedPlugin/Src/MergedPluginMain.cpp` |
| 4 | `feat(componentinfo): integrate GlobalId photo storage with DG warning dialog` | `MergedPlugin/Src/ComponentInfoUtils.cpp`, `MergedPlugin/Src/ComponentInfoUtils.hpp` | `grep "GlobalId" MergedPlugin/Src/*.cpp` |
| 5 | `feat(ifc): integrate basic IFC reading with pass-through stubs` | `MergedPlugin/Src/IFCPropertyUtils.cpp`, `MergedPlugin/Src/IFCPropertyUtils.hpp` | `grep -v "Cache\|All" MergedPlugin/Src/IFCPropertyUtils.cpp` |
| 6b | `feat(api): implement ComponentInfo JavaScript API with DG file picker` | `MergedPlugin/Src/MergedPluginMain.cpp`, `MergedPlugin/Src/ComponentInfoUtils.cpp` | `grep "ComponentInfo" MergedPlugin/Src/MergedPluginMain.cpp` |
| 7 | `test(suite): convert unified test suite to Jest` | `MergedPlugin/__tests__/`, `MergedPlugin/__mocks__/ACAPI.js` | `npm test` |
| 8 | `feat(final): complete integration and validation` | `MergedPlugin/` | `./build.sh && echo "Build successful"` |
| 9 | `ui(palette): hide DG controls, keep browser only` | `MergedPlugin/Src/MergedPluginPalette.cpp` | `grep "DG::" MergedPlugin/Src/MergedPluginPalette.cpp` |

---

## Success Criteria

### Verification Commands
```bash
# Build verification
cd MergedPlugin && ./build.sh
# Expected: Build completes successfully

# Bundle structure verification  
find Release/MergedPlugin.bundle -type f | wc -l
# Expected: 8+ files (complete bundle structure)

# Version format verification
grep "0\.[0-9]\+\.[0-9]\+\.[0-9]\+" MergedPlugin/build.sh
# Expected: Date-based version pattern found

# Jest test verification
cd MergedPlugin && npm test -- --coverage
# Expected: All tests pass, coverage ≥80%

# JavaScript API verification
grep -o "ACAPI\.[A-Za-z]*" MergedPlugin/RFIX/Selection_Test.html | sort -u | wc -l
# Expected: ≥10 unique ACAPI functions found
```

### Final Checklist
- [ ] Plugin loads in ArchiCAD 29 without errors
- [ ] ComponentInfo tab is default and functional
- [ ] IFC Properties tab displays properties when button clicked
- [ ] Photos stored in `{project}_构件照片/{GlobalId}/` structure
- [ ] Elements without GlobalId show DG warning dialog allowing manual entry
- [ ] Version displays in 0.month.day.build format
- [ ] Jest test suite passes with ≥80% coverage
- [ ] DG controls hidden from palette (only browser visible)
- [ ] All manual tests pass in ArchiCAD browser console
- [ ] Build script generates correct bundle structure

