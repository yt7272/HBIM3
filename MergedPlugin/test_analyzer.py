#!/usr/bin/env python3
"""
Comprehensive test for the Log Analyzer.

Tests various scenarios:
1. Successful bridge connection
2. HTML resource loading failure
3. JavaScript bridge initialization failure
4. Mixed success/failure cases
"""

import os
import sys
import tempfile
import subprocess
from pathlib import Path

# Test scenarios
TEST_SCENARIOS = {
    "success": {
        "cpp": """MergedPluginPalette构造函数: 开始初始化，PaletteResId=32520
MergedPluginPalette: 加载HTML资源，大小=12345 字节
MergedPluginPalette: HTML加载完成，初始化JavaScript桥接
InitializeJavaScriptBridge: 开始初始化JavaScript桥接
JavaScript桥接: GetSelectedElements被调用
InitializeJavaScriptBridge: JavaScript桥接注册完成""",
        "js": """HTML loaded, starting bridge check...
Checking ACAPI...
✅ ACAPI found with 5 functions: GetSelectedElements, GetAllIFCProperties, GetCachedIFCProperties, GetPluginVersion, ComponentInfo_GetComponentInfo
Bridge Connected (5 functions)
✅ Plugin version: v0.2.2.33
✅ GetPluginVersion succeeded: v0.2.2.33""",
    },
    "html_failure": {
        "cpp": """MergedPluginPalette构造函数: 开始初始化，PaletteResId=32520
MergedPluginPalette: 加载HTML资源，大小=0 字节
错误: HTML资源为空!
InitializeJavaScriptBridge: 开始初始化JavaScript桥接
错误: 无法加载HTML资源""",
        "js": """Page loaded, starting bridge check...
Checking ACAPI...
❌ ACAPI is undefined
Bridge NOT Connected""",
    },
    "js_bridge_failure": {
        "cpp": """MergedPluginPalette构造函数: 开始初始化，PaletteResId=32520
MergedPluginPalette: 加载HTML资源，大小=9876 字节
MergedPluginPalette: HTML加载完成，初始化JavaScript桥接
InitializeJavaScriptBridge: 开始初始化JavaScript桥接""",
        "js": """HTML loaded, starting bridge check...
Checking ACAPI...
❌ ACAPI is undefined
Attempt 2...
❌ ACAPI is undefined
Attempt 3...
❌ ACAPI is undefined
❌ Failed after 3 attempts""",
    },
    "partial_success": {
        "cpp": """MergedPluginPalette构造函数: 开始初始化，PaletteResId=32520
MergedPluginPalette: 加载HTML资源，大小=5432 字节
InitializeJavaScriptBridge: 开始初始化JavaScript桥接
InitializeJavaScriptBridge: JavaScript桥接注册完成""",
        "js": """HTML loaded, starting bridge check...
Checking ACAPI...
✅ ACAPI found with 2 functions: GetPluginVersion, GetSelectedElements
Bridge Connected (2 functions)
✅ Plugin version: v0.2.2.33
✗ GetAllIFCProperties failed: TypeError: ACAPI.GetAllIFCProperties is not a function""",
    },
}


def run_analyzer(cpp_log_path, js_log_path):
    """Run the log analyzer with given log files."""
    cmd = [
        sys.executable,
        "log_analyzer.py",
        "--cpp-log",
        str(cpp_log_path),
        "--js-log",
        str(js_log_path),
    ]

    result = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        cwd=os.path.dirname(os.path.abspath(__file__)),
    )

    return result.returncode, result.stdout, result.stderr


def test_scenario(name, cpp_content, js_content):
    """Test a specific scenario."""
    print(f"\n{'=' * 60}")
    print(f"Testing scenario: {name}")
    print(f"{'=' * 60}")

    with tempfile.NamedTemporaryFile(
        mode="w", suffix="_cpp.log", delete=False
    ) as cpp_file:
        cpp_file.write(cpp_content)
        cpp_path = cpp_file.name

    with tempfile.NamedTemporaryFile(
        mode="w", suffix="_js.log", delete=False
    ) as js_file:
        js_file.write(js_content)
        js_path = js_file.name

    try:
        returncode, stdout, stderr = run_analyzer(cpp_path, js_path)

        print("CPP Log:")
        print(cpp_content)
        print("\nJS Log:")
        print(js_content)
        print(f"\nAnalyzer Output (exit code: {returncode}):")
        print(stdout)

        if stderr:
            print(f"Analyzer Stderr:")
            print(stderr)

        # Expected exit codes: 0 for success, 1 for failure
        if name == "success":
            expected_code = 0
        else:
            expected_code = 1

        if returncode == expected_code:
            print(f"✓ Scenario '{name}' passed")
            return True
        else:
            print(
                f"✗ Scenario '{name}' failed: expected exit code {expected_code}, got {returncode}"
            )
            return False

    finally:
        # Clean up temp files
        os.unlink(cpp_path)
        os.unlink(js_path)


def main():
    """Run all test scenarios."""
    print("Merged Plugin Log Analyzer Test Suite")
    print("=" * 60)

    results = {}
    for name, logs in TEST_SCENARIOS.items():
        passed = test_scenario(name, logs["cpp"], logs["js"])
        results[name] = passed

    print(f"\n{'=' * 60}")
    print("Test Results Summary")
    print(f"{'=' * 60}")

    all_passed = True
    for name, passed in results.items():
        status = "✓ PASSED" if passed else "✗ FAILED"
        print(f"{name:20} {status}")
        if not passed:
            all_passed = False

    print(f"\nOverall: {'ALL TESTS PASSED' if all_passed else 'SOME TESTS FAILED'}")

    # Return exit code based on test results
    sys.exit(0 if all_passed else 1)


if __name__ == "__main__":
    main()
