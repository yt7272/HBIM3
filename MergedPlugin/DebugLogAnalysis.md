# Debug Log Analysis Guide for Merged Plugin

This document outlines how to analyze debug logs from the Merged Plugin to diagnose JavaScript bridge issues.

## **Log Sources**

### **1. C++ Debug Output (Add-On Output Window)**
**Location**: Window > Add-On Output in ArchiCAD
**Critical Messages to Capture**:

```text
# Palette initialization
"MergedPluginPalette构造函数: 开始初始化，PaletteResId=%d"
"MergedPluginPalette: 加载HTML资源，大小=%d 字节"
"错误: HTML资源为空!"  (if present)
"MergedPluginPalette: HTML加载完成，初始化JavaScript桥接"

# JavaScript bridge initialization  
"InitializeJavaScriptBridge: 开始初始化JavaScript桥接"
"InitializeJavaScriptBridge: JavaScript桥接注册完成"

# Function calls (indicate bridge is working)
"JavaScript桥接: GetSelectedElements被调用"
"JavaScript桥接: GetAllIFCProperties被调用"
"JavaScript桥接: ComponentInfo_GetComponentInfo被调用"

# Errors
任何包含 "错误" 或 "失败" 的行
```

### **2. JavaScript Console Output (F12 Developer Tools)**
**Location**: Browser console (F12 in palette)
**Critical Messages**:

```javascript
// Initialization
"HTML loaded, starting bridge check..."  (from SimpleBridgeTest.html)
"Checking JavaScript bridge..."
"ACAPI object is undefined"  (ERROR - bridge not connected)
"ACAPI object found with X functions"  (SUCCESS - bridge connected)

// Function tests
"Testing GetPluginVersion..."
"✓ GetPluginVersion succeeded: v0.2.2.29"
"✗ GetPluginVersion failed: [error]"

// Bridge status
"Bridge Connected (X functions)"  (green status)
"Bridge Not Connected"  (red status)
```

## **Common Failure Scenarios & Diagnosis**

### **Scenario 1: No C++ Logs Appear**
**Problem**: Plugin not loading or palette not showing
**Diagnosis Steps**:
1. Check if plugin appears in Add-On Manager
2. Verify plugin compiled for correct ArchiCAD version (29.x)
3. Check bundle structure integrity
4. Look for crash reports in system logs

### **Scenario 2: C++ Logs Show But HTML Not Loading**
**Expected Log**: `"MergedPluginPalette: 加载HTML资源，大小=XXXX字节"`
**Missing Log**: `"错误: HTML资源为空!"`

**Root Causes**:
1. DATA resource 100 not compiled into plugin
2. Resource ID mismatch between code (100) and GRC file
3. HTML file encoding issues (UTF-8 with BOM required for GRC)

**Verification**:
- Check `MergedPluginFix.grc.reslist.txt` for `'DATA' 100`
- Verify `SimpleBridgeTest.html` exists in `RFIX/` folder
- Test with external URL: `browser.LoadURL("http://example.com")`

### **Scenario 3: HTML Loads But Bridge Not Connected**
**Expected Logs**:
```
"MergedPluginPalette: HTML加载完成，初始化JavaScript桥接"
"InitializeJavaScriptBridge: 开始初始化JavaScript桥接"
"InitializeJavaScriptBridge: JavaScript桥接注册完成"
```

**JavaScript Console**: "ACAPI object is undefined"

**Root Causes**:
1. JavaScript registration timing issue (HTML loads before objects registered)
2. Browser control not fully initialized before registration
3. CEF (Chromium Embedded Framework) initialization failure

**Solutions**:
1. **Timing fix**: Add delay in JavaScript before accessing ACAPI
2. **Registration sequence**: Ensure `RegisterAsynchJSObject()` called after `LoadHTML()`
3. **CEF debugging**: Enable Chrome DevTools debugging

### **Scenario 4: Bridge Connects But Functions Fail**
**JavaScript Console**: "ACAPI object found with X functions" but function calls fail

**Root Causes**:
1. Function signature mismatch between C++ and JavaScript
2. Thread safety issues (JavaScript callbacks on main thread)
3. Memory management problems with JS::Object

**Diagnosis**:
1. Check function list in JavaScript console
2. Verify function names match between C++ and JavaScript
3. Test with minimal function (GetPluginVersion)

## **Step-by-Step Analysis Procedure**

### **Phase 1: Log Collection**
1. **Clear existing logs**: Restart ArchiCAD for clean logs
2. **Enable all logging**: Ensure plugin built with debug statements
3. **Capture both sources simultaneously**:
   - Open Add-On Output window
   - Open palette and press F12 for console
   - Take screenshots of both

### **Phase 2: Log Analysis**
1. **Check C++ initialization sequence**:
```
✓ Palette constructor called
✓ HTML resource loaded (size > 0)
✓ JavaScript bridge initialization started
✓ JavaScript bridge registration completed
```

2. **Check JavaScript initialization**:
```
✓ HTML loaded message appears
✓ ACAPI object defined (or error)
✓ Bridge status shows connected
✓ Function list populated
```

3. **Test minimal functionality**:
```
✓ GetPluginVersion returns version string
✓ GetSelectedElements returns array (empty if nothing selected)
```

### **Phase 3: Problem Identification**
Use this decision tree:

```
No C++ logs → Plugin not loading → Check Add-On Manager
C++ logs but no HTML size → Resource loading failed → Check DATA resource 100
HTML size >0 but no bridge init → Registration timing → Add JavaScript retry
Bridge init but no ACAPI → CEF issues → Enable CEF debugging
ACAPI defined but functions fail → Signature mismatch → Check function names
```

## **Quick Fixes to Try**

### **Fix 1: JavaScript Retry Logic**
Add to HTML before accessing ACAPI:
```javascript
function waitForACAPI(maxAttempts=50, interval=100) {
    return new Promise((resolve, reject) => {
        let attempts = 0;
        const check = () => {
            if (typeof ACAPI !== 'undefined') {
                resolve(ACAPI);
            } else if (attempts++ < maxAttempts) {
                setTimeout(check, interval);
            } else {
                reject(new Error('ACAPI not available'));
            }
        };
        check();
    });
}

// Usage:
waitForACAPI().then(() => {
    console.log('ACAPI ready');
    checkBridge();
});
```

### **Fix 2: CEF Debugging Enable**
**macOS**:
```bash
defaults write com.graphisoft.debug DG.CefDebugPort 9222
defaults write com.graphisoft.debug DG.CefUseFixedDebugPort true
```

**Windows** (Registry):
```
HKEY_CURRENT_USER\Software\Graphisoft\Debug
DG.CefDebugPort = 9222 (DWORD)
DG.CefUseFixedDebugPort = 1 (DWORD)
```

### **Fix 3: Resource Verification**
Check resource compilation:
```bash
# Verify DATA resource exists
grep "'DATA' 100" MergedPlugin/build/ResourceObjects/MergedPluginFix.grc.reslist.txt

# Check browser control in palette
grep "DLG_MergedPluginPalette-Browser_0" MergedPlugin/build/ResourceObjects/MergedPluginFix.grc.ro.res.txt
```

## **Expected Successful Log Sequence**

### **C++ Add-On Output**:
```
MergedPluginPalette构造函数: 开始初始化，PaletteResId=32520
MergedPluginPalette: 加载HTML资源，大小=12345 字节  
MergedPluginPalette: HTML加载完成，初始化JavaScript桥接
InitializeJavaScriptBridge: 开始初始化JavaScript桥接
InitializeJavaScriptBridge: JavaScript桥接注册完成
```

### **JavaScript Console**:
```
HTML loaded, starting bridge check...
Checking JavaScript bridge...
ACAPI object found with 5 functions
Plugin version: v0.2.2.29
Bridge Connected (5 functions) - Version: v0.2.2.29
```

## **Submission Format for Analysis**

When submitting logs for analysis, please provide:

1. **Complete C++ log** (copy-paste from Add-On Output window)
2. **JavaScript console screenshot** or copy-paste
3. **Bridge status indicator** (screenshot of palette)
4. **ArchiCAD version** (Help > About)
5. **Operating system** version

## **Contact & Escalation**

If logs show unexpected patterns not covered here:
1. Check [AGENTS.md](../AGENTS.md) for build and test commands
2. Review known issues from background research
3. Capture screenshots of error messages
4. Provide plugin bundle for binary analysis