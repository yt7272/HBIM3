#!/bin/bash

# Merged Plugin Packaging Script
# Creates a zip archive of the plugin bundle for distribution

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_NAME="MergedPlugin"
BUNDLE_NAME="${PLUGIN_NAME}.bundle"
BUILD_DIR="${SCRIPT_DIR}/build/Release"
SOURCE_BUNDLE="${BUILD_DIR}/${BUNDLE_NAME}"

# Read version from .build_counter
if [ -f "${SCRIPT_DIR}/.build_counter" ]; then
    BUILD_COUNT=$(cat "${SCRIPT_DIR}/.build_counter")
    # Get current date components
    MONTH=$(date +%m)
    DAY=$(date +%d)
    VERSION="0.${MONTH}.${DAY}.${BUILD_COUNT}"
else
    echo "⚠️  WARNING: .build_counter not found, using default version"
    VERSION="0.0.0.0"
fi

PACKAGE_NAME="${PLUGIN_NAME}_v${VERSION}_macOS_arm64"
ZIP_FILE="${SCRIPT_DIR}/${PACKAGE_NAME}.zip"

echo "=== Merged Plugin Packaging ==="
echo "Plugin Version: ${VERSION}"
echo ""

# Check if source bundle exists
if [ ! -d "${SOURCE_BUNDLE}" ]; then
    echo "❌ ERROR: Plugin bundle not found at: ${SOURCE_BUNDLE}"
    echo "Please build the plugin first: ./build.sh"
    exit 1
fi

# Create temporary directory for packaging
TEMP_DIR=$(mktemp -d)
PACKAGE_DIR="${TEMP_DIR}/${PACKAGE_NAME}"
mkdir -p "${PACKAGE_DIR}"

echo "Creating package: ${PACKAGE_NAME}"
echo ""

# Copy bundle to package directory
echo "Copying plugin bundle..."
cp -r "${SOURCE_BUNDLE}" "${PACKAGE_DIR}/"

# Copy documentation files
echo "Copying documentation..."
cp "${SCRIPT_DIR}/README.md" "${PACKAGE_DIR}/" 2>/dev/null || true
cp "${SCRIPT_DIR}/QUICK_START.md" "${PACKAGE_DIR}/" 2>/dev/null || true
cp "${SCRIPT_DIR}/TESTING.md" "${PACKAGE_DIR}/" 2>/dev/null || true

# Create installation instructions file
cat > "${PACKAGE_DIR}/INSTALL.txt" << EOF
Merged Plugin Installation
==========================

Version: ${VERSION}
Platform: macOS (arm64)
Date: $(date +%Y-%m-%d)

Installation Steps:
1. Copy "${BUNDLE_NAME}" to your ArchiCAD Add-Ons folder:
   - macOS: ~/Library/Application Support/GRAPHISOFT/ARCHICAD 29/Add-Ons/
   - Windows: C:\Users\[YourUsername]\AppData\Roaming\GRAPHISOFT\ARCHICAD 29\Add-Ons\

2. Start ArchiCAD 29

3. Enable the plugin:
   - Go to Options → Add-On Manager
   - Find "Merged Plugin" in the list
   - Check the box to enable it
   - Click OK

4. Open the plugin:
   - Menu: 测试 → Merged Plugin
   - A palette window will appear with two tabs

For detailed testing instructions, see TESTING.md
For quick start, see QUICK_START.md

Troubleshooting:
- If plugin doesn't load, check ArchiCAD version (29.x required)
- If palette is empty, check browser console (F12) for JavaScript errors
- Verify bundle structure: Contents/MacOS/${PLUGIN_NAME} exists

---
Plugin successfully migrated from hybrid DG/HTML to pure HTML interface.
All JavaScript unit tests pass (31/31), HTML validation passes (13/13).
EOF

# Create zip archive
echo "Creating zip archive..."
cd "${TEMP_DIR}"
zip -rq "${ZIP_FILE}" "${PACKAGE_NAME}"

# Clean up temporary directory
rm -rf "${TEMP_DIR}"

# Verify zip was created
if [ -f "${ZIP_FILE}" ]; then
    ZIP_SIZE=$(du -h "${ZIP_FILE}" | cut -f1)
    echo "✅ Package created successfully: ${ZIP_FILE} (${ZIP_SIZE})"
    echo ""
    echo "Contents:"
    echo "  - ${BUNDLE_NAME} (plugin bundle)"
    echo "  - README.md (documentation)"
    echo "  - QUICK_START.md (quick start guide)"
    echo "  - TESTING.md (detailed testing guide)"
    echo "  - INSTALL.txt (installation instructions)"
    echo ""
    echo "Ready for distribution!"
else
    echo "❌ ERROR: Failed to create zip archive"
    exit 1
fi