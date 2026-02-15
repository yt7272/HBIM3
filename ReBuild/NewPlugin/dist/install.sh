#!/bin/bash
# HBIM Plugin 安装脚本
# 将构建好的插件安装到Archicad插件目录

set -e

echo "🔧 HBIM Plugin 安装脚本"
echo "========================================"

# 检查是否在项目根目录
if [ ! -f "CMakeLists.txt" ]; then
	echo "错误：请在项目根目录运行此脚本"
	exit 1
fi

# 检查是否已构建插件
if [ ! -d "build/Release/NewPlugin.bundle" ]; then
	echo "错误：插件未构建，请先运行 ./build.sh"
	exit 1
fi

# Archicad插件目录
ARCHICAD_VERSION="29"
ADDONS_DIR="$HOME/Library/Application Support/GRAPHISOFT/ARCHICAD $ARCHICAD_VERSION/Add-Ons"

echo "📁 Archicad插件目录: $ADDONS_DIR"

# 检查插件目录是否存在
if [ ! -d "$ADDONS_DIR" ]; then
	echo "创建插件目录..."
	mkdir -p "$ADDONS_DIR"
fi

# 备份现有插件
BACKUP_NAME="MergedPlugin.bundle.backup_$(date +%Y%m%d_%H%M%S)"
if [ -d "$ADDONS_DIR/MergedPlugin.bundle" ]; then
	echo "📦 备份现有插件..."
	cp -R "$ADDONS_DIR/MergedPlugin.bundle" "$ADDONS_DIR/$BACKUP_NAME"
	echo "  备份为: $BACKUP_NAME"
fi

# 安装新插件
echo "🚀 安装新插件..."
# 删除现有插件目录（已备份）
rm -rf "$ADDONS_DIR/MergedPlugin.bundle"
# 复制新插件
cp -R "build/Release/NewPlugin.bundle" "$ADDONS_DIR/MergedPlugin.bundle"

# 验证安装
if [ -d "$ADDONS_DIR/MergedPlugin.bundle" ]; then
	echo "✅ 安装成功!"
	echo ""
	echo "安装位置: $ADDONS_DIR/MergedPlugin.bundle"
	
	# 显示版本信息
	if [ -f "$ADDONS_DIR/MergedPlugin.bundle/Contents/Info.plist" ]; then
		VERSION=$(grep -A1 CFBundleVersion "$ADDONS_DIR/MergedPlugin.bundle/Contents/Info.plist" | tail -1 | sed 's/.*<string>//;s/<\/string>//')
		echo "插件版本: $VERSION"
		
		# 检查是否有旧版本插件
		if [ -d "$ADDONS_DIR/$BACKUP_NAME" ]; then
			OLD_VERSION=$(grep -A1 CFBundleVersion "$ADDONS_DIR/$BACKUP_NAME/Contents/Info.plist" 2>/dev/null | tail -1 | sed 's/.*<string>//;s/<\/string>//' || echo "未知")
			if [ "$OLD_VERSION" != "$VERSION" ]; then
				echo ""
				echo "⚠️  注意: 插件已从版本 $OLD_VERSION 更新到 $VERSION"
				echo "   如果Archicad中显示的版本不正确，可能是CEF浏览器缓存问题。"
				echo "   解决方案:"
				echo "   1. 在Archicad插件面板中按 F12 打开开发者工具"
				echo "   2. 切换到 Application → Storage → Clear storage"
				echo "   3. 点击 Clear site data，然后重启插件面板"
			fi
		fi
	fi
	
	echo ""
	echo "下一步:"
	echo "1. 启动Archicad"
	echo "2. 打开 文件 → 插件管理器"
	echo "3. 找到 'NewPlugin' 并启用"
	echo "4. 通过菜单 '测试' → 'NewPlugin' 打开插件面板"
	echo ""
	echo "📝 如果插件面板显示旧版本，请参考 TESTING.md 中的缓存清除说明。"
else
	echo "❌ 安装失败"
	exit 1
fi

echo "========================================"
echo "🎉 安装完成!"