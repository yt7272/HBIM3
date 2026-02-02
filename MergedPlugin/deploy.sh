#!/bin/bash

# Merged Plugin Deployment Script
# Copies the plugin bundle to ArchiCAD Add-Ons folder (macOS)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_NAME="MergedPlugin"
BUNDLE_NAME="${PLUGIN_NAME}.bundle"
BUILD_DIR="${SCRIPT_DIR}/build/Release"
SOURCE_BUNDLE="${BUILD_DIR}/${BUNDLE_NAME}"

# macOS default ArchiCAD Add-Ons location
ARCHICAD_VERSION="29"
TARGET_DIR="${HOME}/Library/Application Support/GRAPHISOFT/ARCHICAD ${ARCHICAD_VERSION}/Add-Ons"
TARGET_BUNDLE="${TARGET_DIR}/${BUNDLE_NAME}"

echo "=== Merged Plugin Deployment ==="
echo "Plugin Version: 0.2.2.47"
echo ""

# Check if source bundle exists
if [ ! -d "${SOURCE_BUNDLE}" ]; then
    echo "❌ ERROR: Plugin bundle not found at: ${SOURCE_BUNDLE}"
    echo "Please build the plugin first: ./build.sh"
    exit 1
fi

# Check if target directory exists
if [ ! -d "${TARGET_DIR}" ]; then
    echo "⚠️  WARNING: ArchiCAD Add-Ons directory not found: ${TARGET_DIR}"
    echo "Creating directory..."
    mkdir -p "${TARGET_DIR}"
    echo "✅ Created directory: ${TARGET_DIR}"
fi

# Check if previous version exists
if [ -d "${TARGET_BUNDLE}" ]; then
    echo "⚠️  WARNING: Previous plugin bundle found at: ${TARGET_BUNDLE}"
    read -p "Remove existing plugin? (y/N): " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Removing existing plugin..."
        rm -rf "${TARGET_BUNDLE}"
        echo "✅ Removed existing plugin"
    else
        echo "❌ Deployment cancelled"
        exit 0
    fi
fi

# Copy the bundle
echo "Copying plugin bundle..."
cp -r "${SOURCE_BUNDLE}" "${TARGET_DIR}/"

# Verify copy succeeded
if [ -d "${TARGET_BUNDLE}" ]; then
    echo "✅ Plugin deployed successfully to: ${TARGET_BUNDLE}"
    echo ""
    echo "Next steps:"
    echo "1. Start ArchiCAD ${ARCHICAD_VERSION}"
    echo "2. Go to Options → Add-On Manager"
    echo "3. Enable 'Merged Plugin'"
    echo "4. Menu: 测试 → Merged Plugin"
else
    echo "❌ ERROR: Deployment failed - target bundle not found"
    exit 1
fi