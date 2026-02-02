#!/bin/bash
# Merged Plugin Bundle 完整性检查
# 用法: ./check_bundle.sh [bundle路径]
# 默认: build/Release/MergedPlugin.bundle

BUNDLE="${1:-build/Release/MergedPlugin.bundle}"
FAIL=0

echo "检查 Bundle: $BUNDLE"
echo "=========================================="

# 必需文件
for f in "Contents/Info.plist" "Contents/PkgInfo" "Contents/MacOS/MergedPlugin"; do
  if [ -f "$BUNDLE/$f" ]; then
    echo "  OK   $f"
  else
    echo "  FAIL $f (缺失)"
    FAIL=1
  fi
done

# Resources
if [ -d "$BUNDLE/Contents/Resources/English.lproj" ]; then
  N=$(find "$BUNDLE/Contents/Resources/English.lproj" -type f | wc -l)
  echo "  OK   Contents/Resources/English.lproj ($N 个文件)"
  if [ ! -f "$BUNDLE/Contents/Resources/English.lproj/DATA_100.rsrd" ]; then
    echo "  FAIL DATA_100.rsrd 缺失 (HTML 资源)"
    FAIL=1
  fi
else
  echo "  FAIL Contents/Resources/English.lproj (目录缺失)"
  FAIL=1
fi

# 可执行文件
if [ -f "$BUNDLE/Contents/MacOS/MergedPlugin" ]; then
  ARCH=$(file "$BUNDLE/Contents/MacOS/MergedPlugin")
  echo "  $ARCH"
  if echo "$ARCH" | grep -q "arm64"; then
    echo "  OK   架构 arm64"
  else
    echo "  WARN 非 arm64，请确认与当前 Mac 匹配"
  fi
fi

# Info.plist 关键键
if [ -f "$BUNDLE/Contents/Info.plist" ]; then
  EXE=$(/usr/libexec/PlistBuddy -c "Print :CFBundleExecutable" "$BUNDLE/Contents/Info.plist" 2>/dev/null)
  LSMIN=$(/usr/libexec/PlistBuddy -c "Print :LSMinimumSystemVersion" "$BUNDLE/Contents/Info.plist" 2>/dev/null)
  if [ "$EXE" != "MergedPlugin" ]; then
    echo "  FAIL CFBundleExecutable 应为 MergedPlugin，当前: $EXE"
    FAIL=1
  else
    echo "  OK   CFBundleExecutable = MergedPlugin"
  fi
  if [ "$LSMIN" != "11.0" ]; then
    echo "  FAIL LSMinimumSystemVersion 应为 11.0，当前: $LSMIN"
    FAIL=1
  else
    echo "  OK   LSMinimumSystemVersion = 11.0"
  fi
fi

echo "=========================================="
if [ $FAIL -eq 0 ]; then
  echo "结果: 通过"
  exit 0
else
  echo "结果: 存在问题，请检查上述 FAIL 项"
  exit 1
fi
