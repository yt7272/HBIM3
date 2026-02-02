#!/bin/bash
# ComponentInfo 插件构建脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
BUNDLE_DIR="$BUILD_DIR/Release/ComponentInfo.bundle"

echo "=== 开始构建 ComponentInfo 插件 ==="

# 创建 build 目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 如果没有 Makefile，运行 cmake
if [ ! -f "Makefile" ]; then
    echo ">>> 运行 cmake..."
    cmake -DCMAKE_BUILD_TYPE=Release ..
fi

# 清理旧的 bundle（但保留 build 目录中的其他文件）
rm -rf "$BUNDLE_DIR"

# 创建完整的 bundle 目录结构（在 make 之前）
mkdir -p "$BUNDLE_DIR/Contents/MacOS"
mkdir -p "$BUNDLE_DIR/Contents/Resources/English.lproj"

# 强制重新编译资源
rm -f ResourceObjects/AddOnResources.stamp

# 编译
echo ">>> 编译源代码..."
make -j4

# 复制必要文件
echo ">>> 复制资源文件..."
cp "$SCRIPT_DIR/RFIX.mac/Info.plist" "$BUNDLE_DIR/Contents/"
cp "$SCRIPT_DIR/RFIX/Images/ArchiCADPlugin.icns" "$BUNDLE_DIR/Contents/Resources/"

# 代码签名
echo ">>> 代码签名..."
codesign --force --deep --sign - "$BUNDLE_DIR"

# 验证 bundle 结构
echo ">>> 验证 bundle 结构..."
if [ -f "$BUNDLE_DIR/Contents/Info.plist" ] && \
   [ -f "$BUNDLE_DIR/Contents/MacOS/ComponentInfo" ] && \
   [ -f "$BUNDLE_DIR/Contents/Resources/ArchiCADPlugin.icns" ] && \
   [ -f "$BUNDLE_DIR/Contents/Resources/English.lproj/MDID_32500.rsrd" ]; then
    echo "=== 构建完成 ==="
    echo "插件位置: $BUNDLE_DIR"
    echo ""
    echo "请将 ComponentInfo.bundle 复制到 ArchiCAD Add-Ons 目录"
else
    echo "=== 构建失败：Bundle 结构不完整 ==="
    ls -laR "$BUNDLE_DIR"
    exit 1
fi
