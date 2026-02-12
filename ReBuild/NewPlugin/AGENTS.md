# Merged Plugin (NewPlugin) — AI Agent Guide

**Generated:** 2025-02-12  
**Plugin Version:** 0.month.day.build (date-based auto-versioning)  
**ArchiCAD API:** Development Kit 29.3100

## OVERVIEW
Combined ComponentInfo + IFCPropertyReader plugin with unified HTML interface. Merges component property management and IFC property reading into single plugin with modern browser-based UI.

## STRUCTURE
```
NewPlugin/
├── CMakeLists.txt          # Build config with date-based versioning
├── build.sh               # Auto-build script with version increment
├── check_bundle.sh        # Bundle integrity checker (fails build if incomplete)
├── Src/                   # Merged source code
│   ├── PluginMain.cpp    # Combined entry points
│   ├── ComponentInfo.cpp/hpp  # Component property logic
│   ├── PropertyUtils.cpp/hpp  # IFC property utilities
│   ├── PluginPalette.cpp/hpp  # HTML palette with JavaScript bridge
│   └── APICommon.*       # API helpers
├── RFIX/                  # Resources
│   └── index.html        # Modern HTML interface (industrial dark theme)
├── RFIX.mac/             # macOS resources
│   └── Info.plist
├── Resources/            # Additional resources
└── build/                # Build output (generated)
```

## WHERE TO LOOK
| Task | Location | Notes |
|------|----------|-------|
| Plugin entry points | `PluginMain.cpp` | Combined `CheckEnvironment`, `RegisterInterface`, `Initialize`, `FreeData` |
| Component property logic | `ComponentInfo.cpp/hpp` | `EnsurePropertyGroupAndDefinitions`, `CopyPhotosForComponent` |
| IFC property utilities | `PropertyUtils.cpp/hpp` | Basic IFC property reading (no caching per guardrails) |
| HTML palette | `PluginPalette.cpp/hpp` | Browser interface with 11 ACAPI JavaScript functions |
| JavaScript bridge | `PluginPalette.cpp` | `ACAPI` object with `GetSelectedElements`, `GetComponentProperties`, etc. |
| Build configuration | `CMakeLists.txt` | Date-based versioning (0.month.day.build) |
| Bundle validation | `check_bundle.sh` | Post-build validation, fails if bundle incomplete |

## KEY SYMBOLS
| Symbol | Type | Location | Role |
|--------|------|----------|------|
| `APIMenuCommandProc_Main` | Function | PluginMain.cpp | Menu handler (ID 32500) |
| `ComponentInfo::EnsurePropertyGroupAndDefinitions` | Function | ComponentInfo.hpp | Creates/ensures property group |
| `GetIFCPropertiesForElement` | Function | PropertyUtils.hpp | Basic IFC property reading |
| `PluginPalette::Initialize` | Function | PluginPalette.cpp | Sets up HTML palette and JavaScript bridge |
| `ACAPI_Notification_CatchSelectionChange` | Hook | PluginMain.cpp | Selection change notifications |

## CONVENTIONS
- **Date-based versioning**: `0.month.day.build` format, auto-incremented by `build.sh`
- **Bundle integrity**: `check_bundle.sh` validates bundle completeness (fails build if missing files)
- **HTML interface**: Pure HTML (no hybrid DG), industrial dark theme, tabs for Component/IFC
- **JavaScript bridge**: 11 ACAPI functions exposed for bidirectional communication
- **Edit mode management**: Visual (*) indicator for unsaved changes, disable auto-save on element switch
- **Chinese support**: Chinese property names (`构件编号`, `照片说明`), Unicode handling with `GS::UniString`

## ANTI-PATTERNS (GUARDRAILS FROM MERGE PLAN)
**STRICT REQUIREMENTS from `.sisyphus/plans/componentinfo-ifc-merge.md`:**
- **NO caching system** - Exclude IFCPropertyReader's LRU cache (basic only requirement)
- **NO performance optimization** - Exclude advanced features like lazy loading, benchmarking
- **NO fallback to GUID** - Strictly require GlobalId property for photo functionality
- **NO separate modals** - Keep everything in single HTML interface with tabs
- **NO traditional versioning** - Must use date-based format only
- **NO AI slop patterns** - No over-engineering, no premature abstraction, minimal error handling

**Specific exclusions:**
- Don't copy IFCPropertyReader's caching or performance features
- Don't implement caching or performance features in IFC tab
- Don't add complex photo preview functionality beyond basic selection
- Don't include separate modal dialogs
- Don't implement traditional semantic versioning
- Don't hardcode version numbers
- Don't include version history tracking
- Don't fallback to GUID when GlobalId is missing
- Don't enable photo functionality for elements without GlobalId

## BUILD COMMANDS
```bash
# Normal build (auto-increments build counter)
cd NewPlugin
./build.sh

# Build output
ls -la build/Release/NewPlugin.bundle/

# Check bundle integrity (runs automatically post-build)
./check_bundle.sh

# Manual build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
make AddOnResources
```

## TESTING
**JavaScript tests (in ArchiCAD browser console F12):**
```javascript
// Merged plugin tests
window.testMergedPlugin.runAllTests();

// ComponentInfo tests
window.testComponentInfo.runAllTests();

// IFC property tests  
window.testIFCProperties.runAllTests();
```

**Bundle validation:**
```bash
# Run validation script
node ../validate_merged_interface.js
```

## NOTES
- **Merged architecture**: Combines ComponentInfo and basic IFCPropertyReader functionality
- **HTML interface only**: No legacy DG dialogs, pure browser-based UI
- **Bundle requirements**: Must have `Info.plist`, `PkgInfo`, executable, `DATA_100.rsrd` (HTML resource)
- **Build enforcement**: CMake ensures bundle completeness via `check_bundle.sh` post-build
- **Version automation**: Date-based versioning with auto-incrementing build counter
- **Guardrail compliance**: Follows strict merge plan requirements (no caching, no performance features)