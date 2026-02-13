# ComponentInfo Plugin — AI Agent Guide

**Generated:** 2025-02-12  
**Plugin Version:** 1.0.0  
**ArchiCAD API:** Development Kit 29.3100

## OVERVIEW
Component property management plugin for ArchiCAD. Adds custom properties: component ID, photo description, photo paths (multiple). Photos copied to `{project_name}_构件照片` folder with relative paths stored in properties.

## STRUCTURE
```
ComponentInfo/
├── CMakeLists.txt      # Build configuration
├── RFIX/               # Resources (Windows)
│   └── ComponentInfoFix.grc
├── RFIX.mac/           # macOS resources  
│   └── Info.plist
├── Src/                # Source code
│   ├── ComponentInfoMain.cpp  # Plugin entry points
│   ├── ComponentInfo.cpp/hpp  # Core logic
│   ├── ComponentInfoDialog.cpp/hpp  # DG dialog (legacy)
│   ├── PropertyUtils.cpp/hpp  # Property utilities
│   └── APICommon.*     # API helpers
└── build/              # Build output (generated)
```

## WHERE TO LOOK
| Task | Location | Notes |
|------|----------|-------|
| Plugin entry points | `ComponentInfoMain.cpp` | `CheckEnvironment`, `RegisterInterface`, `Initialize`, `FreeData` |
| Core property logic | `ComponentInfo.cpp/hpp` | `EnsurePropertyGroupAndDefinitions`, `WritePropertiesToElement` |
| Photo handling | `ComponentInfo.cpp` | `CopyPhotosForComponent`, `GetComponentPhotoFolder` |
| Property utilities | `PropertyUtils.cpp/hpp` | Property definition and value access |
| Dialog interface | `ComponentInfoDialog.cpp/hpp` | Legacy DG dialog (not HTML) |
| Build configuration | `CMakeLists.txt` | Standard versioning (1.0.0) |

## KEY SYMBOLS
| Symbol | Type | Location | Role |
|--------|------|----------|------|
| `ComponentInfo::EnsurePropertyGroupAndDefinitions` | Function | ComponentInfo.hpp | Creates/ensures property group and definitions |
| `ComponentInfo::CopyPhotosForComponent` | Function | ComponentInfo.hpp | Copies photos to project folder, stores relative paths |
| `ComponentInfo::WritePropertiesToElement` | Function | ComponentInfo.hpp | Writes all component properties to element |
| `PropertyDefinitionGuids` | Struct | ComponentInfo.hpp | GUIDs for property group and definitions |
| `APIMenuCommandProc_Main` | Function | ComponentInfoMain.cpp | Menu command handler (ID 32500) |

## CONVENTIONS
- **Chinese property names**: Uses Chinese property names (`构件编号`, `照片说明`, `照片路径`)
- **Photo storage**: Photos copied to project folder with `{project_name}_构件照片/{element_guid}_photos/` structure
- **Relative paths only**: Properties store only relative paths to avoid bloating PLN files
- **GlobalId dependency**: Photo functionality requires element GlobalId property
- **Error handling**: Returns `GSErrCode`, checks early, uses `ACAPI_WriteReport` for errors

## ANTI-PATTERNS (THIS PLUGIN)
- **NO HTML interface**: Uses traditional DG dialog (not browser-based like merged plugin)
- **NO caching**: Simple property access without performance optimization
- **NO fallback to GUID**: Strictly requires GlobalId for photo functionality
- **NO version automation**: Traditional semantic versioning (1.0.0), not date-based

## BUILD COMMANDS
```bash
# Normal build
cd ComponentInfo
./build.sh

# Manual build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
make AddOnResources
```

## TESTING
- **Manual testing only**: No automated test suite
- **In ArchiCAD**: Use menu "Add-Ons → Component Info"
- **Verify**: Check properties appear in element settings, photos copy correctly

## NOTES
- Requires ArchiCAD API Development Kit 29.3100 in parent directory
- Photos only work for elements with GlobalId property
- Project must be saved before using photo functionality
- Property group created automatically on first use
- Chinese text in properties requires Unicode handling (`GS::UniString`)