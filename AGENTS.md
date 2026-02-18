# ArchiCAD Plugin Development Guide for AI Agents

This file provides essential build, test, and coding standards for AI agents working in this ArchiCAD plugin codebase.

## Project Overview

This is an ArchiCAD plugin development project containing four main plugins:
- **ComponentInfo**: Adds custom properties (component ID, photo description, photo paths)
- **IFCPropertyReader**: Reads and displays IFC properties with HTML UI and LRU caching
- **MergedPlugin**: Combined ComponentInfo + IFCPropertyReader with unified HTML interface
- **HBIMComponentEntry**: Adds HBIM properties (HBIM构件编号, HBIM构件说明) with add/edit workflow

Based on ArchiCAD API Development Kit 29.3100, using C++20 standard.

## Quick Reference

### Build Commands

**ComponentInfo:**
```bash
cd ComponentInfo
./build.sh                  # Normal build
```

**IFCPropertyReader:**
```bash
cd IFCPropertyReader
./build.sh                  # Normal build with auto versioning (0.month.day.build)
./build.sh clean           # Clean build with counter reset
```

**MergedPlugin (in ReBuild/NewPlugin):**
```bash
cd ReBuild/NewPlugin
./build.sh                  # Build merged plugin
```

**HBIMComponentEntry (in REREBuild/):**
```bash
cd REREBuild
./build.sh                  # Normal build with auto versioning (0.month.day.build)
./build.sh clean           # Clean build with counter reset
```

**Manual Build:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4                   # Compile plugin
make AddOnResources       # Recompile resources only
```

### Test Commands

**JavaScript Testing (in ArchiCAD browser console F12):**
```javascript
// IFCPropertyReader tests
window.testIFCProperties.testSingleElement();
window.testIFCProperties.testIFCTypeRetrieval();
window.testIFCProperties.testChineseTagSupport();
window.testIFCProperties.testCompleteIFCProperties(); // Full property test
window.testIFCProperties.runAllTests();

// ComponentInfo tests
window.testComponentInfo.runAllTests();

// MergedPlugin tests  
window.testMergedPlugin.runAllTests();

// HBIMComponentEntry tests
// window.testHBIMComponentEntry.runAllTests(); // To be implemented
```

**Interface Validation:**
```bash
node validate_merged_interface.js  # Validate HTML interface structure
```

**Bundle Integrity Checks:**
```bash
# Check bundle structure
find Release/PluginName.bundle -type f | wc -l

# Check executable architecture
file Release/PluginName.bundle/Contents/MacOS/PluginName

# Check dependencies
otool -L Release/PluginName.bundle/Contents/MacOS/PluginName | head -10
```

## Coding Standards

### Language & Compiler
- **C++ Standard**: C++20
- **Compiler**: Clang (macOS) / MSVC (Windows)
- **API**: ArchiCAD API Development Kit 29.3100
- **Warning flags**: `-Wall -Werror` with specific suppressions (see CMakeLists.txt)

### File Organization

**Header Files (.hpp):**
```cpp
#ifndef GS_FILENAME_HPP
#define GS_FILENAME_HPP

#include "APIEnvir.h"
#include "ACAPinc.h"

namespace NamespaceName {
    // Class/struct declarations
    // Function declarations
}

#endif
```

**Source Files (.cpp):**
```cpp
#include "Header.hpp"
#include <unordered_map>  // Standard library headers
#include <list>

namespace NamespaceName {
namespace {
    // Static helper functions in anonymous namespace
    static ReturnType HelperFunction() { /* ... */ }
}

// Public function implementations
ReturnType PublicFunction() { /* ... */ }
}
```

### Naming Conventions

- **Namespaces**: lowercase (`ComponentInfo`, `IFCPropertyUtils`)
- **Classes/Structs**: PascalCase (`PropertyInfo`, `IFCPropertyCache`)
- **Functions**: PascalCase (`GetIFCPropertiesForElement()`, `EnsurePropertyGroup()`)
- **Variables**: camelCase (`elementGuid`, `propertyValue`)
- **Constants**: `k` prefix or UPPER_CASE (`kGroupName`, `TrueString`)
- **Macros**: UPPER_CASE_WITH_UNDERSCORES (`AC_ADDON_VERSION_MAJOR`, `GS_MAC`)

### Memory Management

**Use GS library containers (preferred):**
- `GS::Array<T>` instead of `std::vector<T>`
- `GS::UniString` instead of `std::string`
- `GS::HashTable<K,V>` for maps

**Example:**
```cpp
GS::Array<API_Guid> selectedElements;
GS::UniString propertyName = "Tag";
GS::HashTable<API_ElemTypeID, GS::UniString> typeNames;
```

### Error Handling

**Return GSErrCode and check early:**
```cpp
GSErrCode err = FindOrCreateGroup(group);
if (err != NoError) {
    ACAPI_WriteReport("FindOrCreateGroup failed: %s", true, ErrID_To_Name(err));
    return err;
}
```

**Try-catch for IFC API:**
```cpp
try {
    IFCAPI::ObjectAccessor objectAccessor = IFCAPI::GetObjectAccessor();
    // ... IFC operations
} catch (...) {
    return propertyInfos; // Return empty/default result
}
```

### String Handling

**Always use GS::UniString for Unicode support:**
```cpp
GS::UniString folderName;
folderName.Printf("构件照片_%s", elementGuidStr.ToPrintf());
folderName.Append("_photos");
```

**Chinese support:**
```cpp
static const GS::UniString kGroupName = "构件扩展信息";
static const GS::UniString kCompIdName = "构件编号";
```

### Plugin Architecture

**Standard entry points:**
```cpp
API_AddonType CheckEnvironment(API_EnvirParams* envir);
GSErrCode RegisterInterface(void);
GSErrCode Initialize(void);
GSErrCode FreeData(void);
```

**Menu command handling:**
```cpp
static GSErrCode APIMenuCommandProc_Main(const API_MenuParams* menuParams) {
    if (menuParams->menuItemRef.menuResID != 32500) return NoError;
    // Command implementation
}
```

## Performance Patterns

**LRU Cache Implementation (IFCPropertyReader):**
```cpp
class IFCPropertyCache {
private:
    struct CacheEntry {
        GS::Array<PropertyInfo> properties;
        std::chrono::steady_clock::time_point timestamp;
        std::list<API_Guid>::iterator lruIterator;
    };
    std::unordered_map<API_Guid, CacheEntry, API_GuidHash, API_GuidEqual> cacheMap;
    std::list<API_Guid> lruList;
    mutable std::mutex cacheMutex;
    // Max size: 100 elements, thread-safe access
};
```

**Thread safety for shared resources:**
```cpp
std::lock_guard<std::mutex> lock(cacheMutex);
```

## Build System Details

**Version Management:**
- **ComponentInfo**: Traditional versioning (1.0.0)
- **IFCPropertyReader/MergedPlugin**: Date-based versioning (0.month.day.build)

**CMake Version Macros:**
```cpp
// Defined in CMake: add_definitions(-DAC_ADDON_VERSION_MAJOR=1)
int version = AC_ADDON_VERSION_MAJOR;
```

**Platform Detection:**
```cpp
#if defined(GS_MAC)
    // macOS specific code
#elif defined(GS_WIN)
    // Windows specific code
#endif
```

## Common Issues & Solutions

**Panel opens but has no content (empty palette body):**
- **Cause**: HTML is loaded from resource DATA 100; the `.grc` file references an HTML file (e.g. `Selection_Test.html`) which ResConv embeds. If the file is missing relative to `RFIX/` at build time, DATA 100 is empty and the browser shows nothing.
- **Fix**: Ensure `RFIX/Selection_Test.html` exists and build is run from the plugin root so ResConv finds it. The plugin now shows a fallback error HTML when DATA 100 is null or empty (see `MergedPlugin/PANEL_EMPTY_ANALYSIS.md`).

**MergedPlugin bundle incomplete (plugin won't load):**
- **Prevention**: CMake now enforces completeness so the build **never succeeds** with an incomplete bundle:
  1. **PRE_LINK**: Creates `Contents/MacOS` before linking so `ld: open() failed` does not occur.
  2. **AddOnResources** (existing): Creates `Contents/Resources`, copies `Info.plist` and `PkgInfo`, compiles resources into the bundle.
  3. **POST_BUILD**: Runs `check_bundle.sh` on the built bundle; if any required file is missing (Info.plist, PkgInfo, MacOS/MergedPlugin, Resources/English.lproj/DATA_100.rsrd, CFBundleExecutable, LSMinimumSystemVersion), the script exits 1 and **the build fails**.
- **Result**: Both `make` and `./build.sh` will fail with a clear "BUNDLE INCOMPLETE" / "结果: 存在问题" message instead of producing a broken bundle. Fix the reported missing items and rebuild.

**Linker error "open() failed":**
```bash
mkdir -p Release/PluginName.bundle/Contents/MacOS
make -j4
```

**Resource compilation failure:**
```bash
rm -f ResourceObjects/AddOnResources.stamp
make AddOnResources
```

**Plugin won't load:**
1. Check bundle structure integrity
2. Verify Info.plist and PkgInfo files exist
3. Check executable permissions
4. Verify ArchiCAD version compatibility (29.x)

## Testing Strategy

**JavaScript API Testing:**
- Tests embedded in HTML interface (`Selection_Test.html`)
- Accessible via browser console in ArchiCAD
- Unified test suites for each plugin

**Test Coverage Includes:**
1. Single element selection
2. IFC type retrieval
3. Chinese property name support
4. Version display verification
5. Error handling UI
6. Performance with large datasets
7. Cache system functionality (IFCPropertyReader)
8. Photo storage with GlobalId (ComponentInfo)

**Validation Script:**
```bash
# Validate HTML interface structure
node validate_merged_interface.js
```

## Project Structure
```
HBIM3/
├── REREBuild/             # 主插件源码（HBIMComponentEntry）
│   ├── Src/
│   ├── RFIX/
│   ├── build.sh
│   └── build/Release/HBIMComponentEntry.bundle
├── API.Development.Kit.MAC.29.3100/  # ArchiCAD API 开发包
├── AGENTS.md              # AI 开发指南
└── README.md              # 项目说明
``````

## Notes for AI Agents

1. **Follow existing patterns** - Study ComponentInfo and IFCPropertyReader source files
2. **Use GS library** - Prefer GS containers over STL equivalents
3. **Error handling first** - Check GSErrCode returns early in functions
4. **Unicode support** - Always use GS::UniString for text
5. **Thread safety** - Use mutex for shared resources like caches
6. **Build verification** - Always run `./build.sh` to verify changes
7. **Test integration** - Update JavaScript test suites when adding features
8. **Version compatibility** - Maintain ArchiCAD 29 API compatibility

## DG to HTML Migration Guide

### Problem: Empty Palette in Hybrid DG/HTML Interface
The MergedPlugin originally used a **hybrid DG/HTML interface** where:
- Palette resource (ID 32520) contained only a browser control (ID 20)
- C++ code tried to access 16 other DG controls (IDs 1-16) that didn't exist
- Result: Empty palette with no functional UI

### Solution: Pure HTML Interface
**Migration steps completed**:
1. **Removed all DG control references** from `MergedPluginPalette.cpp`
2. **Established bidirectional JavaScript bridge** (11 ACAPI functions)
3. **Added explicit Save/Cancel workflow** with unsaved changes indicator (*)
4. **Removed unused dialog files** (`MergedPluginDialog.cpp/hpp`, `MergedPluginIFCPropertiesDialog.cpp/hpp`)
5. **Eliminated dead functions** (`RunComponentInfoCommand()`, `RunViewComponentPhotosCommand()`)
6. **Cleaned up resource file** (`MergedPluginFix.grc`) - removed unused dialog resources

### JavaScript Bridge Architecture
```cpp
// C++ exposes functions to JavaScript via ACAPI object
ACAPI_GetJavaScriptVariable("ACAPI", &jsVariable);
jsVariable.AddFunction("GetSelectedElements", &GetSelectedElements);
// ... 10 more functions
```

### Key Lessons for AI Agents

1. **Hybrid Interfaces Are Fragile**: DG controls in HTML palettes require exact resource IDs
2. **Resource Consistency**: Palette resources must match C++ control IDs exactly
3. **JavaScript Bridge Best Practices**:
   - Expose minimal, atomic functions
   - Handle serialization of complex types (GS::Array → JSON)
   - Provide error handling for edge cases
4. **Edit Mode State Management**:
   - Disable auto-save when switching elements to prevent data loss
   - Use visual indicators (*) for unsaved changes
   - Clear edit mode when palette closes or element changes

5. **Testing Strategy**:
   - Mock ACAPI object for JavaScript unit tests (31/31 tests pass)
   - HTML interface validation script (13/13 tests pass)
   - Comprehensive real-world testing guide (TESTING.md)

### Success Metrics
- **Version**: 0.2.2.47 (date-based auto-versioning)
- **Tests**: All JavaScript unit tests pass (31/31)
- **Validation**: HTML interface validation passes (13/13)
- **Bundle**: Clean build, 636KB arm64 bundle, 17 files
- **Documentation**: Complete TESTING.md and README.md

## HBIMComponentEntry Plugin - HBIM属性信息管理

### 功能概述
HBIMComponentEntry插件为ArchiCAD构件添加HBIM属性信息管理功能，包括：
- 创建"HBIM属性信息"属性组
- 添加"HBIM构件编号"和"HBIM构件说明"两个文本属性
- 提供添加/编辑/保存/取消的完整工作流程
- 与现有IFC属性读取功能集成在同一面板中

### 用户界面设计
- **布局**：在现有IFC属性显示下方添加分隔线，新增"HBIM属性信息"区域
- **控件**：包含标题、标签、文本编辑框、添加/编辑按钮、保存/取消按钮
- **状态管理**：
  - 无属性时：显示"添加"按钮
  - 有属性时：显示属性值和"编辑"按钮  
  - 编辑模式：显示编辑框和保存/取消按钮

### 核心实现模式
1. **属性组管理**：使用ArchiCAD Property API创建和管理属性组
2. **属性定义**：创建自定义属性定义，设置对所有分类项可用
3. **属性读写**：读取和写入构件属性值
4. **UI状态同步**：根据属性状态动态更新UI控件显示

### 关键代码模式
```cpp
// 创建HBIM属性组
static GSErrCode FindOrCreateHBIMGroup(API_PropertyGroup& outGroup)
// 创建属性定义  
static GSErrCode FindOrCreateHBIMDefinition(const API_PropertyGroup& group, const GS::UniString& name, API_PropertyDefinition& outDef, GS::Array<API_Guid>& allClassificationItems)
// 读取属性值
static bool HasHBIMProperties(const API_Guid& elementGuid, API_Guid& outIdGuid, API_Guid& outDescGuid)
```

### 构建与测试
- **构建命令**：`cd REREBuild && ./build.sh`
- **版本管理**：日期版本号 (0.month.day.build)
- **签名**：复用现有Developer ID 625438022, Local ID 170868903

---
*This file provides essential guidance for AI agents working in this ArchiCAD plugin codebase.*