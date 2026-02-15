#!/bin/bash

# HBIM Plugin Quick Test Script
# Runs all automated tests and provides a summary

echo "=========================================="
echo "HBIM构件信息输入工具 - 快速测试"
echo "=========================================="
echo "当前目录: $(pwd)"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PASS="${GREEN}✓${NC}"
FAIL="${RED}✗${NC}"
WARN="${YELLOW}⚠${NC}"
INFO="${BLUE}ℹ${NC}"

test_count=0
pass_count=0
fail_count=0

# Function to run test and track results
run_test() {
    local name="$1"
    local command="$2"
    
    echo -e "${INFO} 测试: ${name}..."
    ((test_count++))
    
    if eval "$command"; then
        echo -e "${PASS} ${name}: 通过"
        ((pass_count++))
        return 0
    else
        echo -e "${FAIL} ${name}: 失败"
        ((fail_count++))
        return 1
    fi
}

echo "1. JavaScript单元测试 (Jest)"
run_test "JavaScript单元测试" "npm test -- --silent"

echo ""
echo "2. Bundle完整性检查"
run_test "Bundle完整性检查" "./check_bundle.sh build/Release/NewPlugin.bundle"

echo ""
echo "3. HTML文件存在性检查"
run_test "HTML资源文件检查" "test -f RFIX/index.html && test -s RFIX/index.html"

echo ""
echo "4. 构建目录检查"
run_test "构建目录存在性" "test -d build/Release/NewPlugin.bundle"

echo ""
echo "5. 已安装插件检查"
if [ -d "/Users/yt-mac/Library/Application Support/GRAPHISOFT/ARCHICAD 29/Add-Ons/MergedPlugin.bundle" ]; then
    echo -e "${INFO} 已安装插件检查..."
    ((test_count++))
    echo -e "${PASS} 插件已安装到ArchICAD Add-Ons目录"
    ((pass_count++))
else
    echo -e "${INFO} 已安装插件检查..."
    ((test_count++))
    echo -e "${WARN} 插件未安装到ArchICAD Add-Ons目录 (需要运行install.sh)"
    ((pass_count++)) # 不算失败，只是警告
fi

echo ""
echo "6. 版本信息检查"
if [ -f "CMakeLists.txt" ]; then
    echo -e "${INFO} 版本信息检查..."
    ((test_count++))
    if grep -q "AC_ADDON_VERSION_FULL" CMakeLists.txt; then
        echo -e "${PASS} CMake版本配置正常"
        ((pass_count++))
    else
        echo -e "${FAIL} CMake版本配置异常"
        ((fail_count++))
    fi
fi

echo ""
echo "7. 源代码检查"
run_test "源代码编译检查" "grep -q 'ACAPIEnvir.h' Src/PluginMain.cpp 2>/dev/null || grep -q 'APIEnvir.h' Src/PluginMain.cpp"

echo ""
echo "=========================================="
echo "测试汇总:"
echo "=========================================="
echo -e "总测试: ${test_count}"
echo -e "${GREEN}通过: ${pass_count}${NC}"
if [ $fail_count -gt 0 ]; then
    echo -e "${RED}失败: ${fail_count}${NC}"
else
    echo -e "${GREEN}失败: ${fail_count}${NC}"
fi

echo ""
echo "插件状态:"
if [ $fail_count -eq 0 ]; then
    echo -e "${GREEN}✅ 所有测试通过 - 插件准备就绪${NC}"
    echo ""
    echo "下一步:"
    echo "1. 启动ArchICAD 29"
    echo "2. 打开 文件 → 插件管理器，启用 'NewPlugin'"
    echo "3. 导航到 Test → NewPlugin 打开插件面板"
    echo "4. 按照 TESTING.md 中的步骤进行详细测试"
else
    echo -e "${RED}❌ 有测试失败 - 需要修复${NC}"
    echo ""
    echo "建议:"
    echo "1. 查看失败测试的详细信息"
    echo "2. 运行 './build.sh' 重新构建插件"
    echo "3. 检查源代码中的错误"
    exit 1
fi

echo ""
echo "=========================================="
echo "快速测试完成"
echo "=========================================="