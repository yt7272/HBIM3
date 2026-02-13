# IFCPropertyReader Plugin — AI Agent Guide

**Generated:** 2025-02-12  
**Plugin Version:** 1.0.0 (traditional versioning)  
**ArchiCAD API:** Development Kit 29.3100

## OVERVIEW
IFC property reading plugin with LRU caching, HTML UI, and comprehensive property access. Reads IFC properties from ArchiCAD elements with performance optimization.

## STRUCTURE
```
IFCPropertyReader/
├── CMakeLists.txt          # Build configuration
├── RFIX/                   # Resources (Windows)
│   ├── Selection_Test.html # HTML interface with test suite
│   └── IFCPropertyReaderFix.grc
├── RFIX.mac/               # macOS resources
│   └── Info.plist
├── Src/                    # Source code
│   ├── Main.cpp           # Plugin entry points
│   ├── IFCPropertyUtils.cpp/hpp  # Core IFC property logic
│   ├── IFCPropertyReaderPalette.cpp/hpp  # HTML palette
│   └── ...
└── build/                  # Build output (generated)
```

## WHERE TO LOOK
| Task | Location | Notes |
|------|----------|-------|
| Plugin entry points | `Main.cpp` | Standard ArchiCAD plugin functions |
| IFC property logic | `IFCPropertyUtils.cpp/hpp` | `GetIFCPropertiesForElement`, `GetAllIFCPropertiesForElement` |
| LRU cache system | `IFCPropertyUtils.cpp` | `IFCPropertyCache` class (100 element max) |
| HTML palette | `IFCPropertyReaderPalette.cpp/hpp` | Browser-based UI with JavaScript bridge |
| Property definitions | `IFCPropertyUtils.hpp` | `PropertyInfo` struct, property access helpers |
| Test framework | `RFIX/Selection_Test.html` | JavaScript test suite (31 tests) |

## KEY SYMBOLS
| Symbol | Type | Location | Role |
|--------|------|----------|------|
| `IFCPropertyCache` | Class | IFCPropertyUtils.cpp | LRU cache for IFC properties (thread-safe) |
| `GetIFCPropertiesForElement` | Function | IFCPropertyUtils.hpp | Basic IFC property retrieval |
| `GetAllIFCPropertiesForElement` | Function | IFCPropertyUtils.hpp | Complete property retrieval (all types) |
| `PropertyInfo` | Struct | IFCPropertyUtils.hpp | Property data structure |
| `APIMenuCommandProc_Main` | Function | Main.cpp | Menu handler (ID 32600) |

## CONVENTIONS
- **Chinese property support**: Recognizes Chinese property names ("名称", "名稱", "标签")
- **Property categorization**: Groups properties by type (Local, Preview, Attributes, Classifications)
- **HTML interface**: Modern industrial dark theme with responsive design
- **JavaScript bridge**: 11 ACAPI functions exposed to HTML
- **Performance optimization**: LRU caching with 80-95% performance improvement on repeated access
- **Comprehensive testing**: 31 JavaScript tests accessible via browser console

## ANTI-PATTERNS (THIS PLUGIN)
- **NO date-based versioning**: Uses traditional semantic versioning (1.0.0)
- **NO bundle integrity enforcement**: No post-build bundle validation script
- **Complex caching**: Advanced LRU cache system (excluded from merged plugin per guardrails)
- **Performance features**: Includes benchmarking, lazy loading (excluded from merged plugin)

## BUILD COMMANDS
```bash
# Normal build (auto-increments build counter)
cd IFCPropertyReader
./build.sh

# Clean build (resets counter)
./build.sh clean

# Manual build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
make AddOnResources
```

## TESTING
**JavaScript tests (in ArchiCAD browser console F12):**
```javascript
// Run all tests
window.testIFCProperties.runAllTests();

// Individual tests
window.testIFCProperties.testSingleElement();
window.testIFCProperties.testIFCTypeRetrieval();
window.testIFCProperties.testChineseTagSupport();
window.testIFCProperties.testCompleteIFCProperties(); // Full property test
window.testIFCProperties.testCachedProperties(); // Cache performance test
window.testIFCProperties.clearCache(); // Clear LRU cache
```

## NOTES
- **Property types**: Reads Local, Preview, Attributes, and Classification properties
- **Cache size**: LRU cache limited to 100 elements (configurable)
- **Thread safety**: Uses `std::mutex` for thread-safe cache access
- **Chinese support**: Handles Unicode property names correctly
- **HTML interface**: Requires `Selection_Test.html` in RFIX/ directory
- **Performance**: Caching provides 80-95% speedup on repeated element access