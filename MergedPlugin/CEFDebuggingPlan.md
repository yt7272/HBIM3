# CEF Debugging Plan for JavaScript Bridge Failures

## **When to Use This Plan**
Use when JavaScript bridge fails to connect:
- ❌ ACAPI object is undefined in JavaScript console
- ❌ C++ logs show HTML loads but bridge doesn't initialize
- ❌ Palette shows HTML but JavaScript functions don't work

## **Debugging Tools Available**

### **1. Log Analyzer** (Already prepared)
```bash
python3 log_analyzer.py --cpp-log cpp.txt --js-log js.txt
```
- Analyzes C++ and JavaScript logs
- Identifies common failure patterns
- Provides specific recommendations

### **2. CEF (Chrome Embedded Framework) DevTools**
- Chrome Developer Tools for embedded browser
- Real-time JavaScript debugging
- Network requests, console, elements inspection

### **3. System Logs**
- macOS: Console.app for system logs
- Windows: Event Viewer
- ArchiCAD-specific debug output

## **CEF Debugging Setup**

### **macOS Configuration**
```bash
# Enable CEF debugging on port 9222
defaults write com.graphisoft.debug DG.CefDebugPort 9222

# Enable verbose CEF logging
defaults write com.graphisoft.debug DG.CefLogSeverity verbose

# Disable sandbox (if needed for debugging)
defaults write com.graphisoft.debug DG.CefDisableSandbox -bool true

# Restart ArchiCAD for changes to take effect
```

### **Windows Configuration**
```cmd
# Create registry key for CEF debugging
reg add "HKCU\Software\GRAPHISOFT\DEBUG" /v "DG.CefDebugPort" /t REG_DWORD /d 9222 /f

# Enable verbose logging
reg add "HKCU\Software\GRAPHISOFT\DEBUG" /v "DG.CefLogSeverity" /t REG_SZ /d "verbose" /f
```

## **Debugging Procedure**

### **Step 1: Collect Basic Logs**
1. **C++ Logs**: Window > Add-On Output
2. **JavaScript Console**: F12 in palette
3. **Run analyzer**: `python3 log_analyzer.py --cpp-log cpp.txt --js-log js.txt`

### **Step 2: Enable CEF Debugging**
```bash
# macOS
defaults write com.graphisoft.debug DG.CefDebugPort 9222
killall ArchiCAD  # Restart ArchiCAD

# Verify CEF is listening
lsof -i :9222 | grep ArchiCAD
```

### **Step 3: Connect Chrome DevTools**
1. **Open Chrome**: `chrome://inspect`
2. **Configure**: Add `localhost:9222` as discovery target
3. **Connect**: Click "inspect" on ArchiCAD entry
4. **Debug**: Use Console, Network, Sources tabs

### **Step 4: Analyze Common Failure Points**

#### **A. HTML Loading Issues**
**Symptoms**: Empty HTML, broken resources
**Debug**:
```javascript
// Check if HTML loaded
console.log('HTML loaded:', document.documentElement.outerHTML.length);

// Check resource loading
fetch('data:application/javascript,console.log("test")')
  .then(() => console.log('Fetch works'))
  .catch(err => console.error('Fetch failed:', err));
```

#### **B. JavaScript Bridge Registration**
**Symptoms**: ACAPI undefined
**Debug**:
```javascript
// Check browser object registration
console.log('typeof ACAPI:', typeof ACAPI);
console.log('window.ACAPI:', window.ACAPI);

// Check if RegisterAsynchJSObject was called
// This requires C++ side debugging
```

#### **C. Timing Issues**
**Symptoms**: ACAPI sometimes works after retry
**Debug**:
```javascript
// Add retry logic with diagnostics
let attempts = 0;
function waitForACAPI() {
    attempts++;
    console.log(`Attempt ${attempts}: ACAPI =`, typeof ACAPI);
    
    if (typeof ACAPI !== 'undefined') {
        console.log('✅ ACAPI found after', attempts, 'attempts');
        console.log('Functions:', Object.keys(ACAPI).filter(k => typeof ACAPI[k] === 'function'));
        return true;
    }
    
    if (attempts < 20) {
        setTimeout(waitForACAPI, 250);
        return false;
    }
    
    console.error('❌ ACAPI not found after', attempts, 'attempts');
    return false;
}
setTimeout(waitForACAPI, 100);
```

## **Common Failure Patterns & Solutions**

### **Pattern 1: HTML Resource Empty**
**Symptoms**:
- C++: "HTML资源为空" (0 bytes)
- JavaScript: No content, white screen

**Causes**:
1. DATA resource 100 not compiled into bundle
2. HTML file missing from RFIX/ folder
3. Encoding issues (UTF-8 BOM required for GRC)

**Solutions**:
1. Verify resource compilation:
   ```bash
   # Check compiled resource size
   stat -f%z MergedPlugin/build/Release/MergedPlugin.bundle/Contents/Resources/English.lproj/DATA_100.rsrd
   
   # Expected: > 1000 bytes
   ```

2. Check HTML file exists:
   ```bash
   ls -la MergedPlugin/RFIX/*.html
   ```

3. Recompile resources:
   ```bash
   cd MergedPlugin
   rm -f build/ResourceObjects/AddOnResources.stamp
   make AddOnResources
   ```

### **Pattern 2: JavaScript Bridge Not Registered**
**Symptoms**:
- C++: "InitializeJavaScriptBridge: 开始初始化JavaScript桥接" but no completion
- JavaScript: ACAPI undefined even after retries

**Causes**:
1. `browser.RegisterAsynchJSObject()` not called
2. JS::Object creation fails
3. Thread safety issues

**Solutions**:
1. Add debug logging in C++:
   ```cpp
   ACAPI_WriteReport("Before RegisterAsynchJSObject", false);
   GSErrCode err = browser.RegisterAsynchJSObject("ACAPI", jsACAPI);
   ACAPI_WriteReport("After RegisterAsynchJSObject, err=%d", false, err);
   ```

2. Check JS::Object creation:
   ```cpp
   GS::Ref<JS::Object> jsACAPI = new JS::Object("ACAPI");
   if (jsACAPI == nullptr) {
       ACAPI_WriteReport("ERROR: JS::Object creation failed", true);
   }
   ```

3. Ensure main thread execution:
   ```cpp
   // JavaScript bridge must be initialized on main thread
   DG::RunOnMainThread([&]() {
       InitializeJavaScriptBridge();
   });
   ```

### **Pattern 3: Timing/Race Conditions**
**Symptoms**:
- ACAPI works sometimes, fails other times
- Needs multiple retries to connect

**Causes**:
1. HTML loads before JavaScript bridge is ready
2. `LoadHTML()` completes before `RegisterAsynchJSObject()`
3. Asynchronous initialization order

**Solutions**:
1. Add delay in JavaScript:
   ```javascript
   // Wait for bridge to be ready
   setTimeout(() => {
       if (typeof ACAPI !== 'undefined') {
           // Bridge ready
       } else {
           // Retry
       }
   }, 500);
   ```

2. Ensure proper order in C++:
   ```cpp
   // WRONG: Parallel initialization
   browser.LoadHTML(htmlContent);
   InitializeJavaScriptBridge();  // Might run before HTML loads
   
   // BETTER: Sequential with callbacks
   browser.LoadHTML(htmlContent);
   DG::RunDelayed(100, [&]() {  // Wait 100ms
       InitializeJavaScriptBridge();
   });
   ```

### **Pattern 4: CEF Initialization Failures**
**Symptoms**:
- Browser control shows "CEF not initialized"
- White screen with Chrome error icon
- System logs show CEF DLL loading errors

**Causes**:
1. Missing CEF dependencies
2. Sandbox restrictions
3. GPU/compositor issues

**Solutions**:
1. Disable sandbox (for debugging):
   ```bash
   defaults write com.graphisoft.debug DG.CefDisableSandbox -bool true
   ```

2. Check CEF logs:
   ```bash
   # macOS
   console.app  # Search for "CEF", "Chrome"
   
   # Look for errors like:
   # "Failed to load CEF library"
   # "CEF sandbox error"
   ```

3. Test with external URL:
   ```cpp
   // Debug: Load external website to test CEF
   browser.LoadURL("http://example.com");
   // If this works, CEF is initialized but HTML loading has issues
   ```

## **Advanced Diagnostics**

### **Memory Corruption Detection**
```cpp
// Add memory guards
#ifdef _DEBUG
    #define MEMORY_GUARD_START BM::MemoryGuard memoryGuard;
    #define MEMORY_GUARD_END memoryGuard.Check();
#else
    #define MEMORY_GUARD_START
    #define MEMORY_GUARD_END
#endif

void InitializeJavaScriptBridge() {
    MEMORY_GUARD_START
    // ... bridge initialization
    MEMORY_GUARD_END
}
```

### **Thread Safety Verification**
```cpp
// Ensure JavaScript callbacks on main thread
jsACAPI->AddItem(new JS::Function("GetSelectedElements", [](GS::Ref<JS::Base>) {
    if (!DG::IsMainThread()) {
        ACAPI_WriteReport("WARNING: GetSelectedElements called from non-main thread!", true);
        // Schedule on main thread
        return DG::ExecuteOnMainThreadAndWait([]() {
            return ConvertToJavaScriptVariable(GetSelectedElements());
        });
    }
    return ConvertToJavaScriptVariable(GetSelectedElements());
}));
```

### **Performance Profiling**
```cpp
// Time critical operations
auto start = std::chrono::high_resolution_clock::now();
browser.LoadHTML(htmlContent);
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
ACAPI_WriteReport("LoadHTML took %lld ms", false, duration.count());
```

## **Emergency Recovery**

If debugging fails to resolve issues:

### **1. Fallback to Minimal Interface**
```cpp
// Always load minimal test first
GS::UniString htmlContent = LoadHtmlFromResource();
if (htmlContent.IsEmpty()) {
    // Fallback to hardcoded minimal HTML
    htmlContent = "<html><body><h1>Error: Loading...</h1></body></html>";
    browser.LoadHTML(htmlContent);
}
```

### **2. Graceful Degradation**
```javascript
// JavaScript fallbacks
if (typeof ACAPI === 'undefined') {
    // Show error message instead of broken interface
    document.body.innerHTML = `
        <div style="padding: 20px; color: white; background: #c00;">
            <h2>⚠️ Plugin Connection Error</h2>
            <p>JavaScript bridge not available. Please restart ArchiCAD.</p>
            <button onclick="location.reload()">Retry</button>
        </div>
    `;
}
```

### **3. Safe Mode Configuration**
Add plugin configuration option to disable advanced features:
```cpp
bool useAdvancedFeatures = GetConfigBool("UseAdvancedJavaScript", false);
if (useAdvancedFeatures) {
    // Load full interface
    browser.LoadHTML(fullHtml);
} else {
    // Load minimal interface
    browser.LoadHTML(minimalHtml);
}
```

## **Success Verification**

After applying fixes, verify:

1. **CEF DevTools connects**: `chrome://inspect` shows ArchiCAD
2. **JavaScript console works**: Can execute commands in DevTools
3. **ACAPI object exists**: `typeof ACAPI !== 'undefined'`
4. **Functions work**: `ACAPI.GetPluginVersion()` returns version
5. **Interface loads**: HTML content visible in palette

## **Documentation References**

- **CEF Documentation**: https://bitbucket.org/chromiumembedded/cef
- **ArchiCAD API Guide**: JavaScript Bridge章节
- **GS Library**: DG::Browser class documentation
- **System Logs**: macOS Console.app, Windows Event Viewer

---

**Last Updated**: 2026-02-02  
**Plugin Version**: 0.2.2.33  
**Status**: Debugging tools ready, await user logs