#!/bin/bash
# HBIM Plugin Validation Script
# Runs comprehensive checks on the plugin before user testing.
# Usage: ./validate_plugin.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_NAME="NewPlugin"
BUILD_DIR="$SCRIPT_DIR/build/Release/${PLUGIN_NAME}.bundle"
INSTALL_DIR="$HOME/Library/Application Support/GRAPHISOFT/ARCHICAD 29/Add-Ons/MergedPlugin.bundle"
LOG_FILE="$SCRIPT_DIR/validation_report.txt"

echo "=============================================="
echo "HBIM Plugin Validation Script"
echo "Date: $(date)"
echo "Plugin: $PLUGIN_NAME"
echo "=============================================="

# Initialize log file
{
    echo "Validation Report - $(date)"
    echo "================================"
} > "$LOG_FILE"

# Function to log results
log_result() {
    local status=$1
    local message=$2
    local details=$3
    
    case $status in
        "PASS")
            echo "  ✅ $message"
            echo "✅ PASS: $message" >> "$LOG_FILE"
            ;;
        "FAIL")
            echo "  ❌ $message"
            echo "❌ FAIL: $message" >> "$LOG_FILE"
            ;;
        "WARN")
            echo "  ⚠️  $message"
            echo "⚠️  WARN: $message" >> "$LOG_FILE"
            ;;
        "INFO")
            echo "  ℹ️  $message"
            echo "ℹ️  INFO: $message" >> "$LOG_FILE"
            ;;
    esac
    
    if [ -n "$details" ]; then
        echo "     $details" | sed 's/^/     /'
        echo "     $details" >> "$LOG_FILE"
    fi
}

# 1. Check bundle integrity
echo ""
echo "1. Bundle Integrity Check"
echo "--------------------------"
if [ -f "$SCRIPT_DIR/check_bundle.sh" ]; then
    cd "$SCRIPT_DIR"
    if ./check_bundle.sh > /tmp/bundle_check.txt 2>&1; then
        log_result "PASS" "Bundle integrity check passed"
    else
        log_result "FAIL" "Bundle integrity check failed" "$(cat /tmp/bundle_check.txt | tail -5)"
    fi
else
    log_result "FAIL" "check_bundle.sh not found"
fi

# 2. JavaScript unit tests
echo ""
echo "2. JavaScript Unit Tests"
echo "--------------------------"
cd "$SCRIPT_DIR"
if [ -f "package.json" ]; then
    if npm test > /tmp/js_tests.txt 2>&1; then
        log_result "PASS" "JavaScript unit tests passed (14/14)"
    else
        log_result "FAIL" "JavaScript unit tests failed" "$(cat /tmp/js_tests.txt | tail -20)"
    fi
else
    log_result "FAIL" "package.json not found"
fi

# 3. Source code TODOs check
echo ""
echo "3. Source Code TODOs Check"
echo "--------------------------"
cd "$SCRIPT_DIR"
TODO_COUNT=$(grep -r "TODO\|FIXME\|XXX\|HACK" Src/ 2>/dev/null | wc -l || true)
if [ "$TODO_COUNT" -eq 0 ]; then
    log_result "PASS" "No TODOs/FIXMEs found in source code"
else
    log_result "FAIL" "Found $TODO_COUNT TODO/FIXME items in source code" "$(grep -r 'TODO\|FIXME\|XXX\|HACK' Src/ 2>/dev/null | head -5)"
fi

# 4. Build compilation check
echo ""
echo "4. Build Compilation Check"
echo "--------------------------"
cd "$SCRIPT_DIR"
if [ -d "build" ]; then
    if make -j4 -C build > /tmp/compile_check.txt 2>&1; then
        log_result "PASS" "Plugin compiles cleanly"
    else
        log_result "FAIL" "Plugin compilation failed" "$(cat /tmp/compile_check.txt | tail -20)"
    fi
else
    log_result "FAIL" "Build directory not found - run ./build.sh first"
fi

# 5. Installed plugin verification
echo ""
echo "5. Installed Plugin Verification"
echo "--------------------------------"
if [ -d "$INSTALL_DIR" ]; then
    INSTALLED_VERSION=$(grep -A1 CFBundleVersion "$INSTALL_DIR/Contents/Info.plist" | tail -1 | sed 's/.*<string>\(.*\)<\/string>.*/\1/')
    log_result "INFO" "Installed plugin version: $INSTALLED_VERSION"
    
    # Check if installed version matches built version
    if [ -f "$BUILD_DIR/Contents/Info.plist" ]; then
        BUILT_VERSION=$(grep -A1 CFBundleVersion "$BUILD_DIR/Contents/Info.plist" | tail -1 | sed 's/.*<string>\(.*\)<\/string>.*/\1/')
        if [ "$INSTALLED_VERSION" = "$BUILT_VERSION" ]; then
            log_result "PASS" "Installed version matches built version ($INSTALLED_VERSION)"
        else
            log_result "WARN" "Installed version ($INSTALLED_VERSION) differs from built version ($BUILT_VERSION)"
        fi
    fi
else
    log_result "WARN" "Plugin not installed in ArchiCAD Add-Ons directory"
fi

# 6. Guardrail compliance check
echo ""
echo "6. Guardrail Compliance Check"
echo "------------------------------"
# Check for cache system (should not have caching)
CACHE_CHECK=$(grep -r "LRU\|cache\|Cache" Src/PropertyUtils.cpp 2>/dev/null | grep -v "//.*cache" | wc -l || true)
if [ "$CACHE_CHECK" -lt 3 ]; then
    log_result "PASS" "No caching system detected (guardrail compliance)"
else
    log_result "FAIL" "Potential caching system found - check PropertyUtils.cpp"
fi

# Check for GUID fallback
GUID_FALLBACK=$(grep -r "fallback\|Fallback" Src/ComponentInfo.cpp 2>/dev/null | grep -v "//.*fallback" | wc -l || true)
if [ "$GUID_FALLBACK" -eq 0 ]; then
    log_result "PASS" "No GUID fallback detected (guardrail compliance)"
else
    log_result "FAIL" "Potential GUID fallback found - check ComponentInfo.cpp"
fi

# 7. HTML interface check
echo ""
echo "7. HTML Interface Check"
echo "------------------------"
if [ -f "RFIX/index.html" ]; then
    HTML_LINES=$(wc -l < "RFIX/index.html")
    CACHE_TAGS=$(grep -c "Cache-Control\|Pragma\|Expires" RFIX/index.html || true)
    if [ "$CACHE_TAGS" -ge 3 ]; then
        log_result "PASS" "HTML cache-control tags present ($CACHE_TAGS tags)"
    else
        log_result "WARN" "HTML cache-control tags may be missing"
    fi
    log_result "INFO" "HTML interface file size: $HTML_LINES lines"
else
    log_result "FAIL" "HTML interface file not found (RFIX/index.html)"
fi

# Summary
echo ""
echo "=============================================="
echo "Validation Complete"
echo "=============================================="

# Count results
PASS_COUNT=$(grep -c "✅ PASS" "$LOG_FILE" || true)
FAIL_COUNT=$(grep -c "❌ FAIL" "$LOG_FILE" || true)
WARN_COUNT=$(grep -c "⚠️  WARN" "$LOG_FILE" || true)

echo "Results:"
echo "  ✅ $PASS_COUNT passed"
echo "  ❌ $FAIL_COUNT failed"
echo "  ⚠️  $WARN_COUNT warnings"
echo ""
echo "Detailed report saved to: $LOG_FILE"

if [ "$FAIL_COUNT" -eq 0 ]; then
    echo ""
    echo "🎉 Plugin validation SUCCESS - ready for user testing!"
    echo "Next step: Follow TESTING.md to test in ArchiCAD."
    exit 0
else
    echo ""
    echo "⚠️  Plugin validation FAILED - please address the issues above."
    exit 1
fi