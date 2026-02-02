# Merged Plugin - Quick Start Guide

## Installation (macOS)

1. **Copy plugin bundle** to ArchiCAD Add-Ons folder:
   ```bash
   cp -r build/Release/MergedPlugin.bundle ~/Library/Application\ Support/GRAPHISOFT/ARCHICAD\ 29/Add-Ons/
   ```

2. **Start ArchiCAD 29**

3. **Enable plugin**:
   - Go to **Options → Add-On Manager**
   - Find "Merged Plugin" in the list
   - Check the box to enable it
   - Click OK

## Launching the Plugin

1. **Open palette**:
   - Menu: **测试 → Merged Plugin**
   - A palette window appears with industrial dark theme

2. **Two tabs available**:
   - **构件信息** (Component Information) - default tab
   - **IFC属性** (IFC Properties)

## Basic Usage

### ComponentInfo Tab
1. **Select any element** in ArchiCAD (wall, door, window, etc.)
2. **View element information** (GUID, type, ID, component ID, photo description)
3. **Edit information**:
   - Click **编辑** (Edit) button
   - Modify Component ID, Photo Description
   - Click **选择照片** (Select Photos) to add photos
   - **Save** or **Cancel** changes

### IFC Properties Tab
1. **Switch to IFC属性 tab**
2. **View IFC properties** for selected element
3. **Cache management**:
   - Properties cached for performance (max 100 elements)
   - Click **刷新缓存** (Refresh Cache) to clear

## Testing the JavaScript Bridge

1. **Open browser console** in palette (F12)
2. **Verify ACAPI object**:
   ```javascript
   console.log("ACAPI available:", typeof ACAPI !== 'undefined');
   console.log("Functions:", Object.keys(ACAPI).filter(k => typeof ACAPI[k] === 'function'));
   ```

3. **Run test suites**:
   ```javascript
   window.testComponentInfo.runAllTests();
   window.testIFCProperties.runAllTests();
   window.testMergedPlugin.runAllTests();
   ```

## Troubleshooting Common Issues

### Plugin Doesn't Load
- Verify ArchiCAD version (29.x required)
- Check bundle structure: `Contents/MacOS/MergedPlugin` exists
- Check Add-On Manager for error messages

### Empty Palette
- The palette should show HTML interface, not empty controls
- If empty, check browser console (F12) for JavaScript errors
- Verify HTML resource loads correctly

### JavaScript Errors
1. Open browser console (F12)
2. Check for red error messages
3. Verify ACAPI object exists
4. Check network tab for failed resource loads

### Performance Issues
- Large IFC models may cause slow property retrieval
- Enable cache for better performance
- Use `GetCachedIFCProperties()` instead of `GetAllIFCProperties()`

## Success Checklist

- [ ] Plugin loads without errors in Add-On Manager
- [ ] Palette opens with industrial dark theme
- [ ] ComponentInfo tab displays element information
- [ ] IFC Properties tab displays IFC properties
- [ ] Edit mode works (edit, save, cancel)
- [ ] Photo selection opens file picker
- [ ] Photo viewing opens folder
- [ ] JavaScript bridge functions work (ACAPI object available)
- [ ] Cache system works (performance improves on repeated selections)

## Need More Help?

See detailed testing guide: [TESTING.md](./TESTING.md)

---

*Plugin Version: 0.2.2.47 | Quick Start Guide v1.0*