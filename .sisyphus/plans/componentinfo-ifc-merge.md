# ComponentInfo + IFCPropertyReader Merge Plan

## TL;DR

> **Quick Summary**: Merge IFCPropertyReader functionality into ComponentInfo plugin, creating a unified HTML-based interface with tabs for ComponentInfo form and IFC properties display. Use GlobalId as photo folder name (require GlobalId property), implement date-based versioning, and provide unified testing.
> 
> **Deliverables**:
> - Single merged plugin with HTML interface
> - ComponentInfo tab with photo management using GlobalId folders
> - IFC Properties tab with basic property reading
> - Date-based versioning (0.month.day.build format)
> - Unified JavaScript test suite
> 
> **Estimated Effort**: Large
> **Parallel Execution**: YES - 3 waves
> **Critical Path**: HTML interface → ComponentInfo integration → IFC integration → Testing

---

## Context

### Original Request
Merge two ArchiCAD plugins into one:
- **ComponentInfo**: Adds custom properties (component ID, photo description, photo paths) with C++ native UI
- **IFCPropertyReader**: Reads and displays IFC properties with HTML-based UI

Key requirements:
1. Use ComponentInfo as base plugin
2. Add "查看构件的IFC属性" button to show IFC properties  
3. Convert ComponentInfo UI to HTML format like IFCPropertyReader
4. Use GlobalId Value as photo storage folder name (require GlobalId property)
5. Add IFCPropertyReader version control functionality

### Interview Summary
**Key Decisions**:
- **UI Integration**: Full HTML with tabs (ComponentInfo form in one tab, IFC properties in another)
- **GlobalId Usage**: Require GlobalId property, disable photo features if missing (allow IFC viewing)
- **Version Control**: Date-based versioning (0.month.day.build format)
- **Feature Scope**: Basic IFC property reading only (no caching, performance optimization)
- **Default Behavior**: ComponentInfo tab active by default
- **Photo Management**: Upload on selection, save on submit
- **Testing**: Unified test suite

### Research Findings
- Both plugins use ArchiCAD API Development Kit 29.3100 with C++20
- ComponentInfo already has `GuidToFolderName` function for GUID-based photo storage
- IFCPropertyReader has robust HTML interface with industrial dark theme
- IFCPropertyReader already searches for GlobalId, Tag, ID, Identifier properties
- Both plugins use JavaScript API bridge for HTML ↔ C++ communication
- Photo storage logic exists in ComponentInfo (`CopyPhotosForComponent` function)

### Metis Review
**Identified Gaps** (addressed):
- **Error handling specificity**: Defined exact behavior for missing GlobalId (disable photo only)
- **Tab persistence**: Confirmed default tab behavior (ComponentInfo tab, no persistence needed)
- **Photo workflow**: Clarified upload vs save timing (upload on selection, save on submit)
- **Test integration**: Unified test suite approach confirmed
- **Version scope**: Version control applies to entire merged plugin

---

## Work Objectives

### Core Objective
Create a single ArchiCAD plugin that combines ComponentInfo's photo management functionality with IFCPropertyReader's IFC property reading capabilities, using a unified HTML-based interface with tabbed navigation.

### Concrete Deliverables
- `MergedPlugin.bundle` - Single plugin bundle with merged functionality
- `RFIX/Selection_Test.html` - Unified HTML interface with ComponentInfo and IFC tabs
- `Src/MergedPluginMain.cpp` - Main entry point integrating both functionalities
- `Src/ComponentInfoUtils.cpp` - ComponentInfo logic with GlobalId photo storage
- `Src/IFCPropertyUtils.cpp` - Basic IFC property reading (copied from IFCPropertyReader)
- `build.sh` - Build script with date-based versioning
- Unified JavaScript test functions in HTML interface

### Definition of Done
- [ ] Plugin loads successfully in ArchiCAD 29
- [ ] ComponentInfo tab allows entering component ID, photo description, and photo selection
- [ ] Photos are stored in `{project}_构件照片/{GlobalId}/` directory structure
- [ ] IFC Properties tab displays basic IFC properties when "查看构件的IFC属性" button clicked
- [ ] Elements without GlobalId show error and disable photo functionality but allow IFC viewing
- [ ] Version number displayed in interface using date-based format (0.month.day.build)
- [ ] All unified test functions pass without errors
- [ ] Build script generates correct version and bundle structure

### Must Have
- HTML-based interface with tabbed navigation
- ComponentInfo functionality with GlobalId-based photo storage
- Basic IFC property reading (local properties only)
- Date-based versioning system
- Unified JavaScript test suite
- Proper error handling for missing GlobalId
- ComponentInfo tab as default view

### Must NOT Have (Guardrails)
- **NO caching system** - Exclude IFCPropertyReader's LRU cache (basic only requirement)
- **NO performance optimization** - Exclude advanced features like lazy loading, benchmarking
- **NO fallback to GUID** - Strictly require GlobalId property for photo functionality
- **NO separate modals** - Keep everything in single HTML interface with tabs
- **NO traditional versioning** - Must use date-based format only
- **NO AI slop patterns** - No over-engineering, no premature abstraction, minimal error handling

---

## Verification Strategy

### Test Decision
- **Infrastructure exists**: NO (no traditional test framework)
- **User wants tests**: YES (TDD via JavaScript API)
- **Framework**: JavaScript API through browser console
- **Approach**: Unified test suite in HTML interface

### If TDD Enabled
Each TODO follows RED-GREEN-REFACTOR:

**Task Structure:**
1. **RED**: Write failing test first in HTML interface
2. **GREEN**: Implement minimum code to pass
3. **REFACTOR**: Clean up while keeping green

**Test Setup**: Tests are embedded in HTML interface and accessible via browser console

### Automated Verification (Agent-Executable)

**For HTML Interface changes** (using playwright skill):
```bash
# Agent executes via playwright browser automation:
1. Navigate to: http://localhost:3000/plugin-interface (simulated)
2. Verify: ComponentInfo tab is active by default
3. Verify: IFC Properties tab exists and is accessible
4. Assert: Version number displayed in expected format (0.x.x.x)
5. Screenshot: .sisyphus/evidence/merged-plugin-interface.png
```

**For Plugin Functionality** (using Bash curl/API calls):
```bash
# Since this is ArchiCAD plugin, verification is through build and structure checks:
# Agent runs:
make -j4
# Assert: Build completes without errors
# Assert: Bundle structure is correct
find Release/MergedPlugin.bundle -type f | wc -l
# Assert: File count matches expected structure
```

**For JavaScript API Functions** (using Bash node):
```bash
# Agent validates JavaScript function signatures exist:
grep -A 5 "window.testMergedPlugin" RFIX/Selection_Test.html
# Assert: Unified test functions are present
```

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Start Immediately):
├── Task 1: Create merged project structure
├── Task 2: Implement HTML interface with tabs
└── Task 3: Set up date-based versioning

Wave 2 (After Wave 1):
├── Task 4: Integrate ComponentInfo functionality with GlobalId
├── Task 5: Integrate basic IFC property reading
└── Task 6: Implement JavaScript API bridge

Wave 3 (After Wave 2):
├── Task 7: Implement unified test suite
└── Task 8: Final integration and validation
```

### Dependency Matrix

| Task | Depends On | Blocks | Can Parallelize With |
|------|------------|--------|---------------------|
| 1 | None | 2, 3 | None |
| 2 | 1 | 4, 5, 6 | 3 |
| 3 | 1 | 4, 5, 6 | 2 |
| 4 | 2, 3 | 7, 8 | 5, 6 |
| 5 | 2, 3 | 7, 8 | 4, 6 |
| 6 | 2, 3 | 7, 8 | 4, 5 |
| 7 | 4, 5, 6 | 8 | None |
| 8 | 7 | None | None |

### Agent Dispatch Summary

| Wave | Tasks | Recommended Agents |
|------|-------|-------------------|
| 1 | 1, 2, 3 | delegate_task(category="ultrabrain", load_skills=["frontend-ui-ux"], run_in_background=true) |
| 2 | 4, 5, 6 | delegate_task(category="ultrabrain", load_skills=["git-master"], run_in_background=true) |
| 3 | 7, 8 | delegate_task(category="ultrabrain", load_skills=["playwright"], run_in_background=true) |

---

## TODOs

- [x] 1. Create merged project structure

  **What to do**:
  - Create new directory structure based on ComponentInfo as base
  - Copy ComponentInfo source files and modify names to MergedPlugin
  - Copy IFCPropertyReader HTML interface files
  - Set up unified CMakeLists.txt combining both plugins
  - Configure resource directories for HTML, images, and localization

  **Must NOT do**:
  - Don't copy IFCPropertyReader's caching or performance features
  - Don't include unnecessary IFCPropertyReader source files beyond basic property reading

  **Recommended Agent Profile**:
  > Select category + skills based on task domain. Justify each choice.
  - **Category**: `ultrabrain`
    - Reason: Complex project structure setup requiring understanding of both codebases
  - **Skills**: [`frontend-ui-ux`, `git-master`]
    - `frontend-ui-ux`: For HTML interface structure and resource organization
    - `git-master`: For proper file organization and naming conventions
  - **Skills Evaluated but Omitted**:
    - `playwright`: Not needed for initial structure setup

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 2, 3)
  - **Blocks**: Tasks 2, 3
  - **Blocked By**: None (can start immediately)

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing code to follow):
  - `ComponentInfo/CMakeLists.txt:1-249` - CMake configuration pattern for ComponentInfo
  - `IFCPropertyReader/CMakeLists.txt:1-349` - CMake configuration with version management
  - `ComponentInfo/Src/ComponentInfoMain.cpp:1-50` - Main entry point structure
  - `IFCPropertyReader/RFIX/Selection_Test.html:1-200` - HTML interface structure and CSS

  **Project Structure References**:
  - `ComponentInfo/README.md:5-22` - ComponentInfo directory structure
  - `IFCPropertyReader/README.md:147-163` - IFCPropertyReader directory structure

  **WHY Each Reference Matters**:
  - CMakeLists.txt files show how to configure the build system for ArchiCAD plugins
  - Main.cpp files show the required entry point structure
  - HTML file shows the industrial dark theme styling and interface layout
  - README files show the expected directory organization

  **Acceptance Criteria**:

  **Automated Verification (ALWAYS include, choose by deliverable type):**

  **For Project Structure** (using Bash):
  ```bash
  # Agent runs:
  ls -la MergedPlugin/
  # Assert: Contains Src/, RFIX/, RFIX.mac/, CMakeLists.txt, build.sh
  ls -la MergedPlugin/Src/
  # Assert: Contains main source files with MergedPlugin naming
  ls -la MergedPlugin/RFIX/
  # Assert: Contains Selection_Test.html and resource files
  ```

  **Evidence to Capture**:
  - [ ] Directory structure listing
  - [ ] File count verification

  **Commit**: YES (groups with 1)
  - Message: `feat(merge): create merged plugin project structure`
  - Files: `MergedPlugin/`
  - Pre-commit: `ls MergedPlugin`

- [x] 2. Implement HTML interface with tabs

  **What to do**:
  - Create unified HTML interface based on IFCPropertyReader's industrial dark theme
  - Implement tabbed navigation with ComponentInfo and IFC Properties tabs
  - ComponentInfo tab should contain form fields for component ID, photo description, and photo selection
  - IFC Properties tab should contain property display area and "查看构件的IFC属性" button
  - Ensure ComponentInfo tab is active by default
  - Include version display area in header

  **Must NOT do**:
  - Don't implement caching or performance features in IFC tab
  - Don't add complex photo preview functionality beyond basic selection
  - Don't include separate modal dialogs

  **Recommended Agent Profile**:
  > Select category + skills based on task domain. Justify each choice.
  - **Category**: `visual-engineering`
    - Reason: HTML/CSS interface design and implementation
  - **Skills**: [`frontend-ui-ux`, `playwright`]
    - `frontend-ui-ux`: For creating professional UI with industrial dark theme
    - `playwright`: For ensuring proper interface structure and accessibility
  - **Skills Evaluated but Omitted**:
    - `git-master`: Not needed for HTML/CSS work

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1, 3)
  - **Blocks**: Tasks 4, 5, 6
  - **Blocked By**: Task 1

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing code to follow):
  - `IFCPropertyReader/RFIX/Selection_Test.html:1-400` - Industrial dark theme CSS variables and styling
  - `IFCPropertyReader/RFIX/Selection_Test.html:500-800` - Property display structure and layout
  - `ComponentInfo/README.md:53-55` - ComponentInfo form requirements (component ID, photo description, photos)

  **UI Pattern References**:
  - `IFCPropertyReader/RFIX/Selection_Test.html:plugin-title` - Header and title styling
  - `IFCPropertyReader/RFIX/Selection_Test.html:element-summary` - Element information display
  - `IFCPropertyReader/RFIX/Selection_Test.html:property-list` - Property list styling

  **WHY Each Reference Matters**:
  - Industrial dark theme provides professional appearance consistent with ArchiCAD
  - Property display structure shows how to organize IFC properties clearly
  - ComponentInfo requirements define the necessary form fields for the ComponentInfo tab

  **Acceptance Criteria**:

  **Automated Verification (ALWAYS include, choose by deliverable type):**

  **For HTML Interface** (using playwright skill):
  ```
  # Agent executes via playwright browser automation:
  1. Load: MergedPlugin/RFIX/Selection_Test.html
  2. Verify: Two tabs exist - "ComponentInfo" and "IFC Properties"
  3. Verify: ComponentInfo tab is active by default
  4. Verify: ComponentInfo tab contains input fields for component ID and photo description
  5. Verify: ComponentInfo tab contains photo selection button
  6. Verify: IFC Properties tab contains "查看构件的IFC属性" button
  7. Verify: Version display area exists in header
  8. Screenshot: .sisyphus/evidence/html-interface-tabs.png
  ```

  **Evidence to Capture**:
  - [ ] HTML interface screenshot
  - [ ] Tab functionality verification

  **Commit**: YES (groups with 2)
  - Message: `feat(ui): implement HTML interface with tabs`
  - Files: `MergedPlugin/RFIX/Selection_Test.html`
  - Pre-commit: `grep -A 10 "tabs" MergedPlugin/RFIX/Selection_Test.html`

- [ ] 3. Set up date-based versioning

  **What to do**:
  - Implement automatic date-based versioning (format: 0.month.day.build)
  - Create build counter file (.build_counter) to track build numbers
  - Modify CMakeLists.txt to accept version parameter and define version macros
  - Update build.sh script to generate version and pass to CMake
  - Ensure version is accessible in C++ code and HTML interface

  **Must NOT do**:
  - Don't implement traditional semantic versioning
  - Don't hardcode version numbers
  - Don't include version history tracking

  **Recommended Agent Profile**:
  > Select category + skills based on task domain. Justify each choice.
  - **Category**: `ultrabrain`
    - Reason: Build system configuration requiring CMake and bash scripting expertise
  - **Skills**: [`git-master`]
    - `git-master`: For build script and CMake configuration
  - **Skills Evaluated but Omitted**:
    - `frontend-ui-ux`: Not needed for build system work
    - `playwright`: Not needed for backend versioning

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1, 2)
  - **Blocks**: Tasks 4, 5, 6
  - **Blocked By**: Task 1

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing code to follow):
  - `IFCPropertyReader/build.sh:20-50` - Version generation function with build counter
  - `IFCPropertyReader/CMakeLists.txt:135-179` - CMake version parameter handling and macro definitions
  - `IFCPropertyReader/Src/IFCPropertyUtils.cpp:GetAddOnVersionString` - Version string retrieval function

  **Build Script References**:
  - `IFCPropertyReader/build.sh:generate_version` - Date-based version generation logic
  - `IFCPropertyReader/build.sh:.build_counter` - Build counter file usage

  **WHY Each Reference Matters**:
  - IFCPropertyReader already implements the exact versioning scheme requested
  - CMakeLists.txt shows how to pass version information to C++ code
  - Build script demonstrates proper integration with the build process

  **Acceptance Criteria**:

  **Automated Verification (ALWAYS include, choose by deliverable type):**

  **For Build System** (using Bash):
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
  ```

  **Evidence to Capture**:
  - [ ] Build counter file content
  - [ ] CMakeLists.txt version configuration
  - [ ] Build script version generation

  **Commit**: YES (groups with 3)
  - Message: `feat(version): implement date-based versioning`
  - Files: `MergedPlugin/build.sh`, `MergedPlugin/CMakeLists.txt`
  - Pre-commit: `grep "0\." MergedPlugin/build.sh`

- [ ] 4. Integrate ComponentInfo functionality with GlobalId

  **What to do**:
  - Adapt ComponentInfo C++ logic to work with HTML interface
  - Modify photo storage to use GlobalId value as folder name instead of GUID
  - Implement GlobalId property lookup from selected element
  - Add error handling for elements without GlobalId (disable photo features only)
  - Expose ComponentInfo functionality via JavaScript API (ACAPI functions)
  - Implement photo upload on selection, save on submit workflow

  **Must NOT do**:
  - Don't fallback to GUID when GlobalId is missing
  - Don't enable photo functionality for elements without GlobalId
  - Don't implement immediate photo saving

  **Recommended Agent Profile**:
  > Select category + skills based on task domain. Justify each choice.
  - **Category**: `ultrabrain`
    - Reason: Complex C++ logic integration requiring deep understanding of ArchiCAD API
  - **Skills**: [`git-master`]
    - `git-master`: For C++ code integration and API usage
  - **Skills Evaluated but Omitted**:
    - `frontend-ui-ux`: Backend logic doesn't require UI skills
    - `playwright`: Not needed for C++ implementation

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 5, 6)
  - **Blocks**: Tasks 7, 8
  - **Blocked By**: Tasks 2, 3

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing code to follow):
  - `ComponentInfo/Src/ComponentInfo.cpp:312-332` - GuidToFolderName function (adapt for GlobalId)
  - `ComponentInfo/Src/ComponentInfo.cpp:334-365` - CopyPhotosForComponent function (modify for GlobalId)
  - `IFCPropertyReader/Src/IFCPropertyUtils.cpp:256-266` - GlobalId property lookup logic
  - `IFCPropertyReader/RFIX/Selection_Test.html:ACAPI functions` - JavaScript API integration pattern

  **API References**:
  - `ComponentInfo/Src/ComponentInfo.cpp:GetComponentPhotoFolder` - Photo folder creation logic
  - `ComponentInfo/Src/ComponentInfo.cpp:WritePropertiesToElement` - Property writing logic
  - `IFCPropertyReader/Src/IFCPropertyUtils.cpp:GetIFCPropertiesForElement` - Element property reading

  **WHY Each Reference Matters**:
  - ComponentInfo.cpp contains the core photo management logic that needs adaptation
  - IFCPropertyReader already has GlobalId lookup logic that can be reused
  - JavaScript API integration pattern shows how to expose C++ functions to HTML

  **Acceptance Criteria**:

  **Automated Verification (ALWAYS include, choose by deliverable type):**

  **For C++ Integration** (using Bash):
  ```bash
  # Agent runs:
  grep -A 20 "GlobalId" MergedPlugin/Src/ComponentInfoUtils.cpp
  # Assert: GlobalId property lookup logic exists
  grep -A 10 "photo.*GlobalId" MergedPlugin/Src/ComponentInfoUtils.cpp
  # Assert: Photo storage uses GlobalId folder naming
  grep -A 5 "disable.*photo" MergedPlugin/Src/ComponentInfoUtils.cpp
  # Assert: Error handling for missing GlobalId exists
  grep "ACAPI.*ComponentInfo" MergedPlugin/Src/MergedPluginMain.cpp
  # Assert: JavaScript API functions are exposed
  ```

  **Evidence to Capture**:
  - [ ] GlobalId lookup code
  - [ ] Photo storage logic with GlobalId
  - [ ] Error handling implementation

  **Commit**: YES (groups with 4)
  - Message: `feat(componentinfo): integrate with GlobalId photo storage`
  - Files: `MergedPlugin/Src/ComponentInfoUtils.cpp`, `MergedPlugin/Src/MergedPluginMain.cpp`
  - Pre-commit: `grep "GlobalId" MergedPlugin/Src/*.cpp`

- [ ] 5. Integrate basic IFC property reading

  **What to do**:
  - Copy basic IFC property reading logic from IFCPropertyReader
  - Exclude caching, performance optimization, and advanced features
  - Adapt IFC property reading to work with merged plugin structure
  - Implement "查看构件的IFC属性" button functionality
  - Display basic IFC properties (local properties only) in IFC Properties tab
  - Ensure IFC viewing works even for elements without GlobalId

  **Must NOT do**:
  - Don't include LRU caching system
  - Don't implement GetAllIFCProperties (preview, attributes, classifications)
  - Don't add performance benchmarking or lazy loading
  - Don't include advanced property filtering or search

  **Recommended Agent Profile**:
  > Select category + skills based on task domain. Justify each choice.
  - **Category**: `ultrabrain`
    - Reason: C++ logic integration requiring ArchiCAD IFC API expertise
  - **Skills**: [`git-master`]
    - `git-master`: For C++ code integration and API usage
  - **Skills Evaluated but Omitted**:
    - `frontend-ui-ux`: Backend logic doesn't require UI skills
    - `playwright`: Not needed for C++ implementation

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 4, 6)
  - **Blocks**: Tasks 7, 8
  - **Blocked By**: Tasks 2, 3

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing code to follow):
  - `IFCPropertyReader/Src/IFCPropertyUtils.cpp:191-287` - GetIFCPropertiesForElement basic implementation
  - `IFCPropertyReader/Src/IFCPropertyUtils.cpp:226-287` - Property iteration and basic property extraction
  - `IFCPropertyReader/RFIX/Selection_Test.html:976-980` - ACAPI.GetAllIFCProperties usage (use GetIFCProperties instead)
  - `IFCPropertyReader/Src/IFCPropertyUtils.hpp:24-26` - Basic function declarations

  **Exclusion References** (what NOT to include):
  - `IFCPropertyReader/Src/IFCPropertyUtils.cpp:GetAllIFCPropertiesForElement` - Advanced property reading (exclude)
  - `IFCPropertyReader/Src/IFCPropertyUtils.cpp:GetCachedIFCPropertiesForElement` - Caching system (exclude)
  - `IFCPropertyReader/RFIX/Selection_Test.html:cache test functions` - Performance tests (exclude)

  **WHY Each Reference Matters**:
  - Basic IFC property reading logic provides the core functionality needed
  - Advanced features are explicitly excluded per user requirements
  - JavaScript integration pattern shows how to connect C++ to HTML interface

  **Acceptance Criteria**:

  **Automated Verification (ALWAYS include, choose by deliverable type):**

  **For IFC Integration** (using Bash):
  ```bash
  # Agent runs:
  grep -A 30 "GetIFCPropertiesForElement" MergedPlugin/Src/IFCPropertyUtils.cpp
  # Assert: Basic IFC property reading function exists
  grep -v "GetAllIFCProperties\|GetCached" MergedPlugin/Src/IFCPropertyUtils.cpp
  # Assert: Advanced functions are NOT included
  grep "LocalProperties" MergedPlugin/Src/IFCPropertyUtils.cpp
  # Assert: Only local properties are read
  grep "查看构件的IFC属性" MergedPlugin/RFIX/Selection_Test.html
  # Assert: Button exists in HTML interface
  ```

  **Evidence to Capture**:
  - [ ] Basic IFC property reading code
  - [ ] Exclusion of advanced features verification
  - [ ] HTML button implementation

  **Commit**: YES (groups with 5)
  - Message: `feat(ifc): integrate basic IFC property reading`
  - Files: `MergedPlugin/Src/IFCPropertyUtils.cpp`, `MergedPlugin/Src/IFCPropertyUtils.hpp`
  - Pre-commit: `grep -v "Cache\|All" MergedPlugin/Src/IFCPropertyUtils.cpp`

- [ ] 6. Implement JavaScript API bridge

  **What to do**:
  - Create unified JavaScript API functions for both ComponentInfo and IFC functionality
  - Implement ACAPI.ComponentInfo functions for photo management and property setting
  - Implement ACAPI.IFC functions for basic property reading
  - Connect HTML interface buttons and forms to C++ backend functions
  - Handle error responses and user feedback in HTML interface
  - Ensure proper data flow between HTML and C++

  **Must NOT do**:
  - Don't create separate API namespaces for each functionality
  - Don't implement complex error handling beyond basic user feedback
  - Don't add unnecessary API functions beyond core requirements

  **Recommended Agent Profile**:
  > Select category + skills based on task domain. Justify each choice.
  - **Category**: `ultrabrain`
    - Reason: Complex API bridge implementation requiring both C++ and JavaScript expertise
  - **Skills**: [`git-master`, `frontend-ui-ux`]
    - `git-master`: For C++ API implementation
    - `frontend-ui-ux`: For JavaScript integration and user feedback
  - **Skills Evaluated but Omitted**:
    - `playwright`: Not needed for API bridge implementation

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 4, 5)
  - **Blocks**: Tasks 7, 8
  - **Blocked By**: Tasks 2, 3, 4, 5

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing code to follow):
  - `IFCPropertyReader/RFIX/Selection_Test.html:ACAPI functions` - JavaScript API function structure
  - `IFCPropertyReader/Src/IFCPropertyReaderPalette.cpp` - C++ to JavaScript bridge implementation
  - `ComponentInfo/Src/ComponentInfoDialog.cpp` - ComponentInfo dialog logic (adapt for API)
  - `IFCPropertyReader/RFIX/Selection_Test.html:1041-1047` - Element selection and property loading

  **API Bridge References**:
  - `IFCPropertyReader/Src/Main.cpp:RegisterInterface` - Interface registration pattern
  - `IFCPropertyReader/RFIX/Selection_Test.html:window.testIFCProperties` - Test function structure

  **WHY Each Reference Matters**:
  - IFCPropertyReader shows the complete JavaScript API integration pattern
  - ComponentInfo dialog logic needs to be converted to API functions
  - Interface registration is required for ArchiCAD plugin API exposure

  **Acceptance Criteria**:

  **Automated Verification (ALWAYS include, choose by deliverable type):**

  **For API Bridge** (using Bash):
  ```bash
  # Agent runs:
  grep -A 10 "ACAPI\.ComponentInfo" MergedPlugin/RFIX/Selection_Test.html
  # Assert: ComponentInfo API functions exist
  grep -A 10 "ACAPI\.IFC" MergedPlugin/RFIX/Selection_Test.html
  # Assert: IFC API functions exist
  grep "RegisterInterface" MergedPlugin/Src/MergedPluginMain.cpp
  # Assert: Interface registration exists
  grep -A 5 "error.*GlobalId" MergedPlugin/RFIX/Selection_Test.html
  # Assert: Error handling for missing GlobalId exists
  ```

  **Evidence to Capture**:
  - [ ] JavaScript API function definitions
  - [ ] C++ interface registration
  - [ ] Error handling implementation

  **Commit**: YES (groups with 6)
  - Message: `feat(api): implement JavaScript API bridge`
  - Files: `MergedPlugin/Src/MergedPluginMain.cpp`, `MergedPlugin/RFIX/Selection_Test.html`
  - Pre-commit: `grep "ACAPI\." MergedPlugin/RFIX/Selection_Test.html`

- [ ] 7. Implement unified test suite

  **What to do**:
  - Create unified JavaScript test functions covering both ComponentInfo and IFC functionality
  - Implement tests for ComponentInfo form submission and photo management
  - Implement tests for IFC property reading and display
  - Add tests for GlobalId validation and error handling
  - Include version verification tests
  - Ensure all tests can be run from browser console

  **Must NOT do**:
  - Don't create separate test suites for each functionality
  - Don't include performance or caching tests (excluded per requirements)
  - Don't add complex test scenarios beyond basic functionality

  **Recommended Agent Profile**:
  > Select category + skills based on task domain. Justify each choice.
  - **Category**: `ultrabrain`
    - Reason: Test suite implementation requiring understanding of both functionalities
  - **Skills**: [`frontend-ui-ux`, `playwright`]
    - `frontend-ui-ux`: For JavaScript test function implementation
    - `playwright`: For test structure and verification
  - **Skills Evaluated but Omitted**:
    - `git-master`: Not needed for JavaScript test implementation

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Sequential (after Tasks 4, 5, 6)
  - **Blocks**: Task 8
  - **Blocked By**: Tasks 4, 5, 6

  **References** (CRITICAL - Be Exhaustive):

  **Pattern References** (existing code to follow):
  - `IFCPropertyReader/RFIX/Selection_Test.html:test functions` - JavaScript test function structure
  - `IFCPropertyReader/README.md:113-130` - Test command documentation
  - `ComponentInfo functionality requirements` - ComponentInfo test scenarios

  **Test Pattern References**:
  - `IFCPropertyReader/RFIX/Selection_Test.html:window.testIFCProperties` - Test object structure
  - `IFCPropertyReader/RFIX/Selection_Test.html:runAllTests` - Test execution pattern
  - `IFCPropertyReader/README.md:132-144` - Test coverage requirements

  **WHY Each Reference Matters**:
  - IFCPropertyReader provides the complete test framework structure
  - Test coverage requirements define what needs to be tested
  - Unified test suite must cover both ComponentInfo and IFC functionality

  **Acceptance Criteria**:

  **Automated Verification (ALWAYS include, choose by deliverable type):**

  **For Test Suite** (using Bash):
  ```bash
  # Agent runs:
  grep -A 20 "window\.testMergedPlugin" MergedPlugin/RFIX/Selection_Test.html
  # Assert: Unified test object exists
  grep "testComponentInfo" MergedPlugin/RFIX/Selection_Test.html
  # Assert: ComponentInfo tests exist
  grep "testIFCProperties" MergedPlugin/RFIX/Selection_Test.html
  # Assert: IFC tests exist
  grep "testGlobalId" MergedPlugin/RFIX/Selection_Test.html
  # Assert: GlobalId validation tests exist
  grep "runAllTests" MergedPlugin/RFIX/Selection_Test.html
  # Assert: Unified test execution function exists
  ```

  **Evidence to Capture**:
  - [ ] Unified test function definitions
  - [ ] ComponentInfo test coverage
  - [ ] IFC test coverage
  - [ ] GlobalId validation tests

  **Commit**: YES (groups with 7)
  - Message: `feat(test): implement unified test suite`
  - Files: `MergedPlugin/RFIX/Selection_Test.html`
  - Pre-commit: `grep "testMergedPlugin" MergedPlugin/RFIX/Selection_Test.html`

- [ ] 8. Final integration and validation

  **What to do**:
  - Perform end-to-end integration testing of merged plugin
  - Validate HTML interface functionality with both tabs
  - Test ComponentInfo functionality with GlobalId-based photo storage
  - Test IFC property reading with "查看构件的IFC属性" button
  - Verify error handling for elements without GlobalId
  - Confirm date-based versioning displays correctly
  - Ensure unified test suite passes all tests
  - Validate build process and bundle structure

  **Must NOT do**:
  - Don't add new features beyond specified requirements
  - Don't modify core functionality that's already working
  - Don't skip validation steps

  **Recommended Agent Profile**:
  > Select category + skills based on task domain. Justify each choice.
  - **Category**: `ultrabrain`
    - Reason: Comprehensive integration and validation requiring full system understanding
  - **Skills**: [`playwright`, `git-master`]
    - `playwright`: For end-to-end interface validation
    - `git-master`: For build process and bundle validation
  - **Skills Evaluated but Omitted**:
    - `frontend-ui-ux`: Validation doesn't require UI design skills

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Sequential (final task)
  - **Blocks**: None
  - **Blocked By**: Task 7

  **References** (CRITICAL - Be Exhaustive):

  **Validation References**:
  - `ComponentInfo/README.md:51-55` - ComponentInfo installation and usage
  - `IFCPropertyReader/README.md:60-63` - IFCPropertyReader installation and usage
  - `Definition of Done section` - Complete validation checklist

  **Build Validation References**:
  - `ComponentInfo/build.sh` - Build process validation
  - `IFCPropertyReader/build.sh` - Build process with versioning validation
  - `IFCPropertyReader/README.md:191-207` - Bundle integrity checks

  **WHY Each Reference Matters**:
  - Installation and usage documentation defines expected behavior
  - Definition of Done provides complete validation checklist
  - Bundle integrity checks ensure proper plugin structure

  **Acceptance Criteria**:

  **Automated Verification (ALWAYS include, choose by deliverable type):**

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
  grep -c "testMergedPlugin" MergedPlugin/RFIX/Selection_Test.html
  # Assert: Unified test suite exists
  ```

  **Evidence to Capture**:
  - [ ] Build success verification
  - [ ] Bundle structure validation
  - [ ] HTML interface screenshot
  - [ ] Test suite existence verification

  **Commit**: YES (groups with 8)
  - Message: `feat(final): complete integration and validation`
  - Files: `MergedPlugin/`
  - Pre-commit: `./build.sh && echo "Build successful"`

---

## Commit Strategy

| After Task | Message | Files | Verification |
|------------|---------|-------|--------------|
| 1 | `feat(merge): create merged plugin project structure` | MergedPlugin/ | ls MergedPlugin |
| 2 | `feat(ui): implement HTML interface with tabs` | MergedPlugin/RFIX/Selection_Test.html | grep -A 10 "tabs" |
| 3 | `feat(version): implement date-based versioning` | MergedPlugin/build.sh, MergedPlugin/CMakeLists.txt | grep "0\." |
| 4 | `feat(componentinfo): integrate with GlobalId photo storage` | MergedPlugin/Src/ComponentInfoUtils.cpp, MergedPlugin/Src/MergedPluginMain.cpp | grep "GlobalId" |
| 5 | `feat(ifc): integrate basic IFC property reading` | MergedPlugin/Src/IFCPropertyUtils.cpp, MergedPlugin/Src/IFCPropertyUtils.hpp | grep -v "Cache\|All" |
| 6 | `feat(api): implement JavaScript API bridge` | MergedPlugin/Src/MergedPluginMain.cpp, MergedPlugin/RFIX/Selection_Test.html | grep "ACAPI\." |
| 7 | `feat(test): implement unified test suite` | MergedPlugin/RFIX/Selection_Test.html | grep "testMergedPlugin" |
| 8 | `feat(final): complete integration and validation` | MergedPlugin/ | ./build.sh |

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
```

### Final Checklist
- [ ] Plugin loads in ArchiCAD 29 without errors
- [ ] ComponentInfo tab is default and functional
- [ ] IFC Properties tab displays properties when button clicked
- [ ] Photos stored in `{project}_构件照片/{GlobalId}/` structure
- [ ] Elements without GlobalId disable photo features but allow IFC viewing
- [ ] Version displays in 0.month.day.build format
- [ ] All unified tests pass via browser console
- [ ] Build script generates correct bundle structure