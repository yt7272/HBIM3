#!/usr/bin/env python3
"""
Log Analyzer for Merged Plugin Debug Output

This script analyzes debug logs from the Merged Plugin to diagnose
JavaScript bridge issues. It parses both C++ Add-On Output logs and
JavaScript console logs to identify problems and suggest fixes.

Usage:
  python3 log_analyzer.py --cpp-log cpp_output.txt --js-log js_console.txt
  python3 log_analyzer.py --log combined_log.txt
"""

import argparse
import re
import sys
from datetime import datetime
from typing import List, Dict, Tuple, Optional


class LogAnalyzer:
    """Analyzes Merged Plugin debug logs."""

    def __init__(self):
        # C++ log patterns
        self.cpp_patterns = {
            "palette_constructor": re.compile(
                r"MergedPluginPalette构造函数.*PaletteResId=(\d+)"
            ),
            "html_loaded": re.compile(r"加载HTML资源.*大小=(\d+) 字节"),
            "html_empty_error": re.compile(r"错误: HTML资源为空"),
            "js_bridge_init_start": re.compile(
                r"InitializeJavaScriptBridge: 开始初始化JavaScript桥接"
            ),
            "js_bridge_init_complete": re.compile(
                r"InitializeJavaScriptBridge: JavaScript桥接注册完成"
            ),
            "js_function_called": re.compile(r"JavaScript桥接: (\w+)被调用"),
            "any_error": re.compile(r"错误:|失败:|error:|fail:", re.IGNORECASE),
        }

        # JavaScript log patterns
        self.js_patterns = {
            "html_loaded": re.compile(r"HTML loaded.*starting bridge check"),
            "checking_bridge": re.compile(r"Checking JavaScript bridge"),
            "acapi_undefined": re.compile(r"ACAPI object is undefined"),
            "acapi_found": re.compile(r"ACAPI object found with (\d+) functions"),
            "bridge_connected": re.compile(r"Bridge Connected \((\d+) functions\)"),
            "bridge_not_connected": re.compile(r"Bridge Not Connected"),
            "plugin_version": re.compile(r"Plugin version: (v[\d.]+)"),
            "function_test_success": re.compile(r"✓ (\w+) succeeded"),
            "function_test_failed": re.compile(r"✗ (\w+) failed"),
            "js_error": re.compile(r"error|Error|ERROR|失败|错误", re.IGNORECASE),
        }

        self.results = {
            "cpp": {"lines": [], "matches": {}},
            "js": {"lines": [], "matches": {}},
            "issues": [],
            "recommendations": [],
        }

    def analyze_cpp_log(self, lines: List[str]) -> Dict:
        """Analyze C++ Add-On Output logs."""
        matches = {key: [] for key in self.cpp_patterns.keys()}

        for i, line in enumerate(lines, 1):
            line = line.strip()
            if not line:
                continue

            for pattern_name, pattern in self.cpp_patterns.items():
                match = pattern.search(line)
                if match:
                    matches[pattern_name].append(
                        {"line": i, "text": line, "match": match.groups()}
                    )

        return matches

    def analyze_js_log(self, lines: List[str]) -> Dict:
        """Analyze JavaScript console logs."""
        matches = {key: [] for key in self.js_patterns.keys()}

        for i, line in enumerate(lines, 1):
            line = line.strip()
            if not line:
                continue

            for pattern_name, pattern in self.js_patterns.items():
                match = pattern.search(line)
                if match:
                    matches[pattern_name].append(
                        {"line": i, "text": line, "match": match.groups()}
                    )

        return matches

    def diagnose_issues(
        self, cpp_matches: Dict, js_matches: Dict
    ) -> Tuple[List[str], List[str]]:
        """Diagnose issues based on log patterns."""
        issues = []
        recommendations = []

        # Check C++ initialization sequence
        if not cpp_matches["palette_constructor"]:
            issues.append("Plugin may not be loading - no palette constructor logs")
            recommendations.append("Check if plugin appears in Add-On Manager")

        if cpp_matches["html_empty_error"]:
            issues.append("HTML resource failed to load (empty)")
            recommendations.append(
                "1. Verify DATA resource 100 exists in compiled bundle"
            )
            recommendations.append(
                "2. Check SimpleBridgeTest.html exists in RFIX/ folder"
            )
            recommendations.append(
                "3. Verify HTML file encoding (UTF-8 with BOM for GRC)"
            )

        if cpp_matches["html_loaded"]:
            size_match = cpp_matches["html_loaded"][0]["match"]
            if size_match and int(size_match[0]) > 0:
                # HTML loaded successfully
                if not cpp_matches["js_bridge_init_start"]:
                    issues.append(
                        "HTML loaded but JavaScript bridge initialization not started"
                    )
                    recommendations.append(
                        "Check timing: RegisterAsynchJSObject() should be called after LoadHTML()"
                    )
                elif not cpp_matches["js_bridge_init_complete"]:
                    issues.append(
                        "JavaScript bridge initialization started but not completed"
                    )
                    recommendations.append(
                        "1. Check browser.RegisterAsynchJSObject() call"
                    )
                    recommendations.append(
                        "2. Verify JS::Object creation and function registration"
                    )
                    recommendations.append(
                        "3. Check for exceptions in JavaScript bridge registration"
                    )

        # Check JavaScript bridge connection
        if js_matches["acapi_undefined"]:
            issues.append("JavaScript bridge not connected - ACAPI object undefined")
            recommendations.append(
                "1. Add JavaScript retry logic (waitForACAPI function)"
            )
            recommendations.append(
                "2. Enable CEF debugging: defaults write com.graphisoft.debug DG.CefDebugPort 9222"
            )
            recommendations.append(
                "3. Check CEF initialization - may need system dependencies"
            )

        if js_matches["acapi_found"]:
            func_count = int(js_matches["acapi_found"][0]["match"][0])
            if func_count == 0:
                issues.append("ACAPI object found but contains 0 functions")
                recommendations.append(
                    "1. Verify JavaScript function registration in C++"
                )
                recommendations.append(
                    "2. Check function names match between C++ and JavaScript"
                )

            if not js_matches["bridge_connected"]:
                issues.append("ACAPI found but bridge status not showing as connected")
                recommendations.append("Check JavaScript bridge status update logic")

        # Check function tests
        failed_tests = []
        for failed in js_matches["function_test_failed"]:
            func_name = failed["match"][0] if failed["match"] else "unknown"
            failed_tests.append(func_name)

        if failed_tests:
            issues.append(f"Function tests failed: {', '.join(failed_tests)}")
            recommendations.append(
                "1. Check function signatures match between C++ and JavaScript"
            )
            recommendations.append(
                "2. Verify function implementations handle errors properly"
            )
            recommendations.append(
                "3. Check thread safety (JavaScript callbacks on main thread)"
            )

        # Check for any errors
        if cpp_matches["any_error"]:
            error_count = len(cpp_matches["any_error"])
            issues.append(f"Found {error_count} error(s) in C++ logs")
            for error in cpp_matches["any_error"][:3]:  # Show first 3
                recommendations.append(
                    f"C++ Error (line {error['line']}): {error['text'][:100]}..."
                )

        if js_matches["js_error"]:
            error_count = len(
                [
                    e
                    for e in js_matches["js_error"]
                    if not any(
                        p.search(e["text"])
                        for p in [
                            re.compile(r"ACAPI object is undefined"),
                            re.compile(r"GetPluginVersion failed"),
                            re.compile(r"GetSelectedElements failed"),
                        ]
                    )
                ]
            )
            if error_count > 0:
                issues.append(f"Found {error_count} error(s) in JavaScript logs")

        # Add general recommendations based on common issues
        if not recommendations:
            if (
                cpp_matches["js_bridge_init_complete"]
                and js_matches["bridge_connected"]
            ):
                recommendations.append(
                    "✓ JavaScript bridge appears to be working correctly"
                )
                recommendations.append("Proceed to test full plugin functionality")
            else:
                recommendations.append(
                    "Enable CEF debugging for detailed Chrome DevTools inspection"
                )
                recommendations.append(
                    "Test with external URL: browser.LoadURL('http://example.com')"
                )
                recommendations.append(
                    "Verify resource compilation with: grep \"'DATA' 100\" build logs"
                )

        return issues, recommendations

    def print_report(
        self,
        cpp_matches: Dict,
        js_matches: Dict,
        issues: List[str],
        recommendations: List[str],
    ):
        """Print analysis report."""
        print("=" * 80)
        print("MERGED PLUGIN LOG ANALYSIS REPORT")
        print(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print("=" * 80)

        # Summary statistics
        print("\n📊 SUMMARY STATISTICS")
        print("-" * 40)

        cpp_stats = []
        if cpp_matches["palette_constructor"]:
            cpp_stats.append("✓ Palette constructor called")
        if cpp_matches["html_loaded"]:
            size = (
                cpp_matches["html_loaded"][0]["match"][0]
                if cpp_matches["html_loaded"][0]["match"]
                else "?"
            )
            cpp_stats.append(f"✓ HTML loaded ({size} bytes)")
        if cpp_matches["js_bridge_init_start"]:
            cpp_stats.append("✓ JavaScript bridge initialization started")
        if cpp_matches["js_bridge_init_complete"]:
            cpp_stats.append("✓ JavaScript bridge registration completed")
        if cpp_matches["js_function_called"]:
            funcs = [
                m["match"][0] for m in cpp_matches["js_function_called"] if m["match"]
            ]
            cpp_stats.append(f"✓ JavaScript functions called: {', '.join(set(funcs))}")

        js_stats = []
        if js_matches["acapi_found"]:
            count = (
                js_matches["acapi_found"][0]["match"][0]
                if js_matches["acapi_found"][0]["match"]
                else "?"
            )
            js_stats.append(f"✓ ACAPI object found ({count} functions)")
        if js_matches["bridge_connected"]:
            js_stats.append("✓ Bridge status: Connected")
        if js_matches["plugin_version"]:
            version = (
                js_matches["plugin_version"][0]["match"][0]
                if js_matches["plugin_version"][0]["match"]
                else "?"
            )
            js_stats.append(f"✓ Plugin version: {version}")

        if cpp_stats:
            print("C++ Logs:")
            for stat in cpp_stats:
                print(f"  {stat}")

        if js_stats:
            print("\nJavaScript Logs:")
            for stat in js_stats:
                print(f"  {stat}")

        # Issues
        if issues:
            print(f"\n🔴 ISSUES FOUND ({len(issues)})")
            print("-" * 40)
            for i, issue in enumerate(issues, 1):
                print(f"{i}. {issue}")
        else:
            print("\n✅ NO ISSUES FOUND")

        # Recommendations
        if recommendations:
            print(f"\n💡 RECOMMENDATIONS ({len(recommendations)})")
            print("-" * 40)
            for i, rec in enumerate(recommendations, 1):
                print(f"{i}. {rec}")

        # Detailed matches
        print(f"\n🔍 DETAILED FINDINGS")
        print("-" * 40)

        if cpp_matches["any_error"]:
            print(f"C++ Errors ({len(cpp_matches['any_error'])}):")
            for error in cpp_matches["any_error"][:5]:  # Show first 5
                print(f"  Line {error['line']}: {error['text'][:80]}...")
            if len(cpp_matches["any_error"]) > 5:
                print(f"  ... and {len(cpp_matches['any_error']) - 5} more")

        if js_matches["function_test_failed"]:
            print(
                f"\nFailed JavaScript Tests ({len(js_matches['function_test_failed'])}):"
            )
            for failed in js_matches["function_test_failed"]:
                func = failed["match"][0] if failed["match"] else "unknown"
                print(f"  {func} (line {failed['line']})")

        print("\n" + "=" * 80)
        print("ANALYSIS COMPLETE")
        print("=" * 80)


def main():
    parser = argparse.ArgumentParser(description="Analyze Merged Plugin debug logs")
    parser.add_argument("--cpp-log", help="C++ Add-On Output log file")
    parser.add_argument("--js-log", help="JavaScript console log file")
    parser.add_argument(
        "--log", help="Combined log file (will attempt to separate C++ and JS logs)"
    )
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")

    args = parser.parse_args()

    if not any([args.cpp_log, args.js_log, args.log]):
        print("Error: No log files specified")
        parser.print_help()
        sys.exit(1)

    analyzer = LogAnalyzer()

    # Read log files
    cpp_lines = []
    js_lines = []

    if args.log:
        # Try to separate combined log
        with open(args.log, "r", encoding="utf-8", errors="ignore") as f:
            all_lines = f.readlines()

        # Simple heuristic: lines with Chinese chars likely C++, English likely JS
        for line in all_lines:
            # Check for Chinese characters or C++ patterns
            if any(char >= "\u4e00" and char <= "\u9fff" for char in line):
                cpp_lines.append(line)
            elif "ACAPI" in line or "Bridge" in line or "JavaScript" in line:
                js_lines.append(line)
            elif line.strip().startswith("[") and "]" in line:  # Timestamp format
                js_lines.append(line)
            else:
                # Default to C++ for unknown
                cpp_lines.append(line)

        if args.verbose:
            print(
                f"Separated {len(cpp_lines)} C++ lines and {len(js_lines)} JS lines from combined log"
            )

    if args.cpp_log:
        with open(args.cpp_log, "r", encoding="utf-8", errors="ignore") as f:
            cpp_lines.extend(f.readlines())

    if args.js_log:
        with open(args.js_log, "r", encoding="utf-8", errors="ignore") as f:
            js_lines.extend(f.readlines())

    if not cpp_lines and not js_lines:
        print("Error: No log content found")
        sys.exit(1)

    # Analyze logs
    cpp_matches = analyzer.analyze_cpp_log(cpp_lines) if cpp_lines else {}
    js_matches = analyzer.analyze_js_log(js_lines) if js_lines else {}

    # Diagnose issues
    issues, recommendations = analyzer.diagnose_issues(cpp_matches, js_matches)

    # Print report
    analyzer.print_report(cpp_matches, js_matches, issues, recommendations)

    # Exit code based on issues found
    if issues:
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == "__main__":
    main()
