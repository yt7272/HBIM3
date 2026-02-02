# Log Analysis Tools for Merged Plugin

This directory contains tools for analyzing debug logs from the Merged Plugin to diagnose JavaScript bridge issues.

## **Available Tools**

### **1. Python Log Analyzer (`log_analyzer.py`)**
A comprehensive log analysis tool that:
- Parses both C++ Add-On Output logs and JavaScript console logs
- Identifies common failure patterns based on ArchiCAD API research
- Provides specific recommendations for fixes
- Supports separate log files or combined logs

### **2. Debug Log Analysis Guide (`DebugLogAnalysis.md`)**
Step-by-step guide for:
- Collecting debug logs from ArchiCAD
- Identifying different failure scenarios
- Applying targeted fixes
- Expected log sequences for successful operation

### **3. Test Logs (`test_logs/`)**
Sample log files demonstrating:
- Successful plugin initialization
- Common failure scenarios
- Combined log format examples

## **Quick Start**

### **Installation**
No installation required. Ensure Python 3.6+ is available:

```bash
python3 --version
```

### **Basic Usage**

**Analyze separate log files:**
```bash
python3 log_analyzer.py --cpp-log cpp_output.txt --js-log js_console.txt
```

**Analyze combined log file:**
```bash
python3 log_analyzer.py --log combined_output.txt
```

**Verbose output:**
```bash
python3 log_analyzer.py --log combined_output.txt --verbose
```

### **Test with Sample Logs**
```bash
# Test successful scenario
python3 log_analyzer.py --cpp-log test_logs/cpp_success.log --js-log test_logs/js_success.log

# Test failure scenario  
python3 log_analyzer.py --cpp-log test_logs/cpp_failure.log --js-log test_logs/js_failure.log

# Test combined log
python3 log_analyzer.py --log test_logs/combined_success.log
```

## **Log Collection Instructions**

### **C++ Add-On Output Logs**
1. In ArchiCAD, open: **Window > Add-On Output**
2. Clear any existing logs
3. Load the Merged Plugin
4. Copy-paste all text from the Add-On Output window to a file

**Expected patterns:**
```
MergedPluginPalette构造函数: 开始初始化，PaletteResId=32520
MergedPluginPalette: 加载HTML资源，大小=12345 字节
InitializeJavaScriptBridge: 开始初始化JavaScript桥接
InitializeJavaScriptBridge: JavaScript桥接注册完成
```

### **JavaScript Console Logs**
1. Open the plugin palette in ArchiCAD
2. Press **F12** to open browser developer tools
3. Go to the **Console** tab
4. Copy-paste all console output to a file

**Expected patterns:**
```
HTML loaded, starting bridge check...
ACAPI object found with 5 functions
Bridge Connected (5 functions) - Version: v0.2.2.29
```

## **Common Analysis Results**

### **Success Indicators**
- ✅ C++: Palette constructor, HTML loaded, JavaScript bridge initialized
- ✅ JavaScript: ACAPI object found, bridge connected, plugin version displayed
- ✅ Function tests succeed (GetPluginVersion, GetSelectedElements)

### **Failure Scenarios & Fixes**

| Issue | Symptoms | Recommended Fix |
|-------|----------|----------------|
| **HTML resource empty** | `错误: HTML资源为空!` | Verify DATA resource 100 compilation |
| **JavaScript bridge not connected** | `ACAPI object is undefined` | Add JavaScript retry logic, enable CEF debugging |
| **Bridge initialized but functions fail** | ACAPI found but function calls fail | Check function signatures, thread safety |
| **No C++ logs at all** | Plugin not appearing in Add-On Manager | Check bundle structure, ArchiCAD version compatibility |

## **Advanced Features**

### **Automatic Log Separation**
The analyzer can automatically separate combined logs using:
- **Chinese characters** → Likely C++ logs
- **English text with timestamps** → Likely JavaScript logs
- **Pattern matching** for known log formats

### **Exit Codes**
- **Exit code 0**: No issues found
- **Exit code 1**: Issues detected (check recommendations)

### **Integration with CI/CD**
```bash
# Example: Fail build if logs show critical errors
python3 log_analyzer.py --log test_results.txt
if [ $? -eq 1 ]; then
    echo "Critical errors detected in logs"
    exit 1
fi
```

## **Troubleshooting the Analyzer**

### **Common Issues**
1. **Encoding problems**: Use `--encoding utf-8` if logs contain special characters
2. **Pattern mismatches**: Update regex patterns in `log_analyzer.py` if log formats change
3. **Missing dependencies**: Only requires Python standard library

### **Extending the Analyzer**
To add new log patterns:

1. **Add C++ patterns** in `cpp_patterns` dictionary:
```python
'new_pattern': re.compile(r'Your regex pattern here')
```

2. **Add JavaScript patterns** in `js_patterns` dictionary

3. **Add diagnosis logic** in `diagnose_issues()` method

## **Related Resources**

- **AGENTS.md**: Project overview, build commands, coding standards
- **DebugLogAnalysis.md**: Detailed log analysis methodology
- **SimpleBridgeTest.html**: Minimal JavaScript bridge test interface
- **Build output**: Check `build/ResourceObjects/` for compilation logs

## **Getting Help**

If the analyzer reports issues you can't resolve:

1. **Collect complete logs** (C++ and JavaScript)
2. **Run analyzer** and save the report
3. **Check `DebugLogAnalysis.md`** for step-by-step debugging
4. **Review background research** on ArchiCAD API JavaScript bridge issues
5. **Test with `SimpleBridgeTest.html`** for isolated bridge testing

## **Version History**

- **v1.0.0**: Initial release with multilingual log parsing, failure diagnosis, and recommendations
- **Test coverage**: Success/failure scenarios, combined log handling