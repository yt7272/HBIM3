#!/bin/bash
# IFC Property Reader 构建脚本
# 自动处理常见的构建问题
# 支持自动日期格式版本号 (格式: 0.月.日.构建次数)

set -e  # 遇到错误时退出

echo "🔧 IFC Property Reader 构建脚本"
echo "========================================"

# 检查是否在项目根目录
if [ ! -f "CMakeLists.txt" ]; then
	echo "错误：请在项目根目录运行此脚本"
	exit 1
fi

# ============================================================================
# 版本号生成函数
# ============================================================================

# 生成日期格式版本号 (格式: 0.月.日.构建次数)
generate_version() {
    # 构建计数器文件路径
    local COUNTER_FILE=".build_counter"
    
    # 获取当前月份和日期
    local CURRENT_MONTH=$(date +%m)
    local CURRENT_DAY=$(date +%d)
    
    # 移除月份和日期的前导零
    CURRENT_MONTH=$((10#$CURRENT_MONTH))
    CURRENT_DAY=$((10#$CURRENT_DAY))
    
    # 读取或初始化构建计数器
    if [ -f "$COUNTER_FILE" ]; then
        local BUILD_COUNT=$(cat "$COUNTER_FILE")
        BUILD_COUNT=$((BUILD_COUNT + 1))
    else
        local BUILD_COUNT=1
    fi
    
    # 保存新的构建计数
    echo "$BUILD_COUNT" > "$COUNTER_FILE"
    
    # 生成版本字符串 (格式: 0.月.日.构建次数)
    local VERSION="0.${CURRENT_MONTH}.${CURRENT_DAY}.${BUILD_COUNT}"
    
    echo "$VERSION"
}

# 清理构建目录（可选）
if [ "$1" == "clean" ]; then
	echo "🧹 清理构建目录..."
	rm -rf build
    # 注意：清理时不重置构建计数器，以保持构建次数的连续性
fi

# 生成版本号
echo "📅 生成版本号..."
VERSION=$(generate_version)
echo "   版本号: ${VERSION}"
echo "   格式说明: 0.月.日.构建次数"
echo "   构建时间: $(date '+%Y-%m-%d %H:%M:%S')"

# 创建构建目录
if [ ! -d "build" ]; then
	echo "📁 创建构建目录..."
	mkdir build
fi

cd build

# 检查是否已配置CMake
if [ ! -f "CMakeCache.txt" ]; then
	echo "⚙️  配置CMake（版本: ${VERSION}）..."
	cmake .. -DCMAKE_BUILD_TYPE=Release -DAC_ADDON_VERSION_AUTO="${VERSION}"
else
	echo "⚙️  CMake已配置，跳过配置步骤"
    echo "    当前版本: ${VERSION}"
    echo "    注意：如果要更新版本号，请运行 './build.sh clean' 重新配置CMake"
fi

# 创建输出目录（解决链接器错误）
echo "📁 确保输出目录存在..."
mkdir -p Release/IFCPropertyReader.bundle/Contents/MacOS
mkdir -p Release/IFCPropertyReader.bundle/Contents/Resources
echo "  ✅ 输出目录已创建"

# 检查资源时间戳
echo "🔄 检查资源编译状态..."
if [ -f "ResourceObjects/AddOnResources.stamp" ]; then
    # 检查stamp文件是否为空（可能表示资源编译失败）
    if [ ! -s "ResourceObjects/AddOnResources.stamp" ]; then
        echo "  ⚠️  AddOnResources.stamp文件为空，资源编译可能失败，重新编译资源..."
        rm -f ResourceObjects/AddOnResources.stamp
        echo "  资源需要重新编译"
    else
        echo "  资源已编译"
    fi
else
    echo "  资源需要重新编译"
fi

# 开始构建
echo "🚀 开始构建插件..."
make -j4

# 确保Info.plist存在并更新版本号
echo "📝 检查Info.plist..."
BUNDLE_PATH="Release/IFCPropertyReader.bundle"
INFO_PLIST_PATH="${BUNDLE_PATH}/Contents/Info.plist"
INFO_PLIST_SOURCE="../RFIX.mac/Info.plist"

# 检查Info.plist是否存在，如果不存在则复制
if [ ! -f "${INFO_PLIST_PATH}" ]; then
    echo "  ℹ️  Info.plist不存在，从模板复制..."
    mkdir -p "${BUNDLE_PATH}/Contents"
    cp "${INFO_PLIST_SOURCE}" "${INFO_PLIST_PATH}"
fi

# 更新Info.plist中的版本号
if [ -f "${INFO_PLIST_PATH}" ]; then
    echo "  🔄 更新Info.plist版本号为: ${VERSION}"
    # 备份原始文件
    cp "${INFO_PLIST_PATH}" "${INFO_PLIST_PATH}.bak"
    
    # 更新CFBundleVersion
    sed -i '' "s/<string>1.0<\/string>/<string>${VERSION}<\/string>/" "${INFO_PLIST_PATH}"
    
    # 更新CFBundleGetInfoString，保留Archicad版本信息
    sed -i '' "s/IFCPropertyReader version 29, IFC属性读取器插件/IFCPropertyReader plugin version ${VERSION} for Archicad 29, 属性查看过滤器/" "${INFO_PLIST_PATH}"
    
    echo "  ✅ Info.plist更新完成"
else
    echo "  ⚠️  警告：无法找到或创建Info.plist"
fi

# 验证构建结果
echo "✅ 验证构建结果..."
if [ -f "Release/IFCPropertyReader.bundle/Contents/MacOS/IFCPropertyReader" ]; then
    echo "  成功：插件已构建"
    echo "  位置：build/Release/IFCPropertyReader.bundle"
    
    # 检查文件大小
    FILESIZE=$(stat -f%z "Release/IFCPropertyReader.bundle/Contents/MacOS/IFCPropertyReader" 2>/dev/null || stat -c%s "Release/IFCPropertyReader.bundle/Contents/MacOS/IFCPropertyReader" 2>/dev/null || echo "未知")
    echo "  可执行文件大小：${FILESIZE} 字节"
    
    # 检查架构
    echo "  架构信息："
    file "Release/IFCPropertyReader.bundle/Contents/MacOS/IFCPropertyReader" || true
    
    # 列出Bundle内容
    echo "  Bundle结构："
    find "Release/IFCPropertyReader.bundle" -type f | wc -l | xargs echo "    文件数量："
else
    echo "  ❌ 错误：插件构建失败"
    exit 1
fi

echo ""
echo "========================================"
echo "🎉 构建完成！"
echo ""
echo "安装说明："
echo "1. 将 build/Release/IFCPropertyReader.bundle 复制到Archicad插件目录："
echo "   macOS: ~/Library/Application Support/GRAPHISOFT/ARCHICAD 29/Add-Ons/"
echo "   Windows: C:\\Users\\[用户]\\AppData\\Roaming\\GRAPHISOFT\\ARCHICAD 29\\Add-Ons\\"
echo ""
echo "2. 启动Archicad，在插件管理器中启用 'IFC Property Reader'"
echo ""
echo "3. 通过菜单 '测试' → 'IFC Property Reader' 打开插件面板"
echo ""
echo "如需重新构建（完全清理）："
echo "  ./build.sh clean"
echo "========================================"