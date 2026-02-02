# Merged ArchiCAD Plugin

A unified plugin combining **ComponentInfo** (photo management, component metadata) and **IFCPropertyReader** (IFC property display) with a pure HTML/JavaScript interface.

## Features

### ComponentInfo Tab
- **Component metadata management**: Store component ID, photo descriptions
- **Photo management**: Select, view, and manage photos for selected elements
- **Edit workflow**: Edit → Save/Cancel with unsaved changes indicator (*)
- **Automatic folder creation**: Photos stored in `[Project Folder]/构件照片_[GUID]_photos/`

### IFC Properties Tab  
- **IFC property display**: Show all IFC properties for selected elements
- **LRU caching**: 100-element cache for performance optimization
- **Chinese property support**: Proper handling of Chinese property names
- **Refresh capabilities**: Manual cache refresh option

### Unified Interface
- **Industrial dark theme**: Consistent styling from IFCPropertyReader
- **Tabbed navigation**: Switch between ComponentInfo and IFC Properties
- **JavaScript bridge**: 11 ACAPI functions for C++ ↔ JavaScript communication
- **Auto-selection tracking**: Updates display when elements are selected in ArchiCAD

## Architecture

### Pure HTML Interface
The plugin uses a **100% HTML/JavaScript interface** with no DG controls. This resolves the "empty palette" issue that occurred with the hybrid DG/HTML interface where palette resources contained only a browser control while C++ code tried to access missing DG controls.

### JavaScript Bridge (`ACAPI` object)
The C++ backend exposes these functions to JavaScript:

```javascript
// Selection & IFC Properties
GetSelectedElements() → [[guid, typeName, elementId], ...]
GetAllIFCProperties(guid) → Array<PropertyInfo>
GetCachedIFCProperties(guid) → Array<PropertyInfo> (cached)
GetCachedIFCPropertiesWithRefresh(guid) → Array<PropertyInfo> (refresh if stale)
ClearIFCPropertyCache() → void
GetIFCPropertyCacheSize() → number

// ComponentInfo
ComponentInfo_GetComponentInfo(guid) → {id, description, photoPaths}
ComponentInfo_SaveComponentInfo([guid, id, description, paths]) → bool
ComponentInfo_SelectPhotos() → Array<paths> (opens file picker)
ComponentInfo_ViewPhotos([paths]) → void (opens in default viewer)

// Plugin info
GetPluginVersion() → "0.2.2.47"
```

### Key Technical Decisions

1. **Edit Mode State Management**: 
   - Automatic save on selection change is **disabled** (`HasUnsavedChanges()` always returns false)
   - User must manually save before switching elements
   - Visual indicator (*) shows unsaved changes

2. **Photo Management**:
   - Photos are copied to project folder (original files remain untouched)
   - Deleted photos are removed from properties but not from disk (intentional safety)
   - Photo folders use element GUID for uniqueness

3. **IFC Property Cache**:
   - Thread-safe LRU cache with mutex protection
   - Maximum 100 elements, session-only persistence
   - Stale detection (10-minute timeout)

4. **Property Group Handling**:
   - "构件扩展信息" property group created automatically if missing
   - Graceful fallback if properties unavailable for element type

## Building the Plugin

### Prerequisites
- ArchiCAD API Development Kit 29.3100
- CMake 3.10+
- C++20 compatible compiler (Clang on macOS, MSVC on Windows)
- Node.js 16+ (for JavaScript testing only)

### Build Steps

```bash
# Clone the repository
git clone [repository-url]

# Navigate to plugin directory
cd MergedPlugin

# Build the plugin
./build.sh

# For clean rebuild
./build.sh clean
```

### Versioning
The plugin uses date-based versioning: `0.month.day.buildCount`
- Automatically increments build counter in `.build_counter`
- Version updated in `Info.plist` and displayed in UI

## Testing

### JavaScript Unit Tests
```bash
cd MergedPlugin
npm install
npm test
```
**31/31 tests pass** with mock ACAPI implementation.

### HTML Interface Validation
```bash
node ../validate_merged_interface.js
```
**13/13 tests pass** for industrial dark theme preservation and required elements.

### ArchiCAD Testing
Follow the comprehensive testing guide in [TESTING.md](./TESTING.md):
1. Install plugin bundle to ArchiCAD Add-Ons folder
2. Enable in Add-On Manager
3. Open palette via **测试 → Merged Plugin**
4. Verify all functionality as outlined in TESTING.md

## Migration History

### From Hybrid DG/HTML to Pure HTML
**Problem**: Empty palette due to missing DG controls in hybrid interface
**Solution**: Removed all DG controls, converted to pure HTML/JavaScript interface

**Changes Made**:
- Removed DG control references from `MergedPluginPalette.cpp`
- Established bidirectional JavaScript bridge
- Added explicit Save/Cancel buttons with edit workflow
- Removed unused dialog files (`MergedPluginDialog.cpp/hpp`, `MergedPluginIFCPropertiesDialog.cpp/hpp`)
- Eliminated dead functions (`RunComponentInfoCommand()`, `RunViewComponentPhotosCommand()`)

### Key Fixes
1. **Version consistency**: Fixed mismatch between CMakeCache and actual version
2. **Build script enhancement**: Proper version updates in Info.plist
3. **Resource cleanup**: Removed unused dialog resources from `.grc` file
4. **JavaScript bridge**: 11 functions with proper error handling

## Project Structure

```
MergedPlugin/
├── Src/                          # C++ source files
│   ├── MergedPlugin.cpp          # Core plugin logic
│   ├── MergedPlugin.hpp          # Header file
│   ├── MergedPluginPalette.cpp   # HTML palette implementation
│   └── MergedPluginResources.cpp # Resource management
├── RFIX/                         # Resources
│   └── Selection_Test.html       # HTML interface (industrial dark theme)
├── __tests__/                    # JavaScript unit tests
├── __mocks__/                    # Mock ACAPI for testing
├── build/                        # Build output
│   └── Release/MergedPlugin.bundle # Plugin bundle for installation
├── TESTING.md                    # Comprehensive testing guide
└── README.md                     # This file
```

## Known Limitations

| Area | Limitation | Workaround |
|------|------------|------------|
| **Edit Mode** | No auto-save when switching elements | User **must save** before selecting different element |
| **Photo Management** | Deleted photos remain on disk | Intentional - prevents accidental data loss |
| **IFC Cache** | Session-only, max 100 elements | Manual refresh via button if needed |
| **Cross-Element** | Unsaved changes persist in UI for deselected element | Visual indicator (*) shows unsaved state |

## Success Criteria

✅ Plugin loads without errors in ArchiCAD Add-On Manager  
✅ Palette opens with industrial dark theme and two tabs  
✅ ComponentInfo tab displays element information  
✅ IFC Properties tab displays IFC properties  
✅ Edit mode works (edit, save, cancel)  
✅ Photo selection opens file picker  
✅ Photo viewing opens folder  
✅ JavaScript bridge functions work (ACAPI object available)  
✅ Cache system works (performance improves on repeated selections)  
✅ No memory leaks (observe memory usage over time)  

## Troubleshooting

Common issues and solutions documented in [TESTING.md](./TESTING.md#troubleshooting).

## License & Credits

Built on ArchiCAD API Development Kit 29.3100.  
Combines functionality from ComponentInfo and IFCPropertyReader plugins.

---

*Version: 0.2.2.47 | Last Updated: 2026-02-02*