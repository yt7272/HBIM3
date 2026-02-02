# 空面板问题修复指南

## 问题描述
用户报告：插件加载后显示面板，但面板上没有任何内容。

## 根本原因分析
经过诊断，发现以下问题：

### 1. **控件ID冲突** (已修复)
- **问题**: `BrowserId = 1` 与 `TextComponentIdentifierId = 1` 冲突
- **资源文件**: GDLG 32520 只包含浏览器控件 (ID 1)
- **代码**: 引用多个控件 (ID 1-19)，但这些控件在资源文件中不存在
- **影响**: 控件初始化失败可能导致构造函数失败或JavaScript桥接无法工作

### 2. **资源不匹配** (已修复)
- **问题**: 代码期望GDLG 32520包含多个控件，但资源文件只有浏览器控件
- **原因**: 合并插件时资源文件未正确配置
- **影响**: 引用不存在控件的代码会失败

## 已应用的修复

### 修复1: 浏览器控件ID调整
- **更改前**: `BrowserId = 1`
- **更改后**: `BrowserId = 20`
- **资源文件**: 更新GDLG 32520，浏览器控件ID从1改为20
- **文件**: `MergedPluginPalette.hpp`, `MergedPluginFix.grc`

### 修复2: 禁用不存在控件的操作
- **注释掉**: 所有对不存在控件的操作（Disable(), Hide(), Attach(), SetText()）
- **原因**: 避免因引用不存在控件导致的运行时错误
- **文件**: `MergedPluginPalette.cpp` (第113-141行相关代码)

### 修复3: 使用极简测试HTML
- **文件**: `MinimalTest.html` 替代 `Selection_Test.html`
- **特点**: 简单、明确的调试信息，自动重试机制
- **资源**: 更新DATA 100资源指向新HTML文件

## 当前版本
- **插件版本**: 0.2.2.32
- **构建时间**: 2026-02-02 08:54:12
- **位置**: `MergedPlugin/build/Release/MergedPlugin.bundle`

## 测试步骤

### 1. 安装新版本
```bash
# 复制到Archicad插件目录
cp MergedPlugin/build/Release/MergedPlugin.bundle \
   ~/Library/Application\ Support/GRAPHISOFT/ARCHICAD\ 29/Add-Ons/
```

### 2. 启动Archicad并测试
1. **启动Archicad**
2. **启用插件**: 插件管理器 → 启用 "Merged Plugin"
3. **打开面板**: 菜单 → 测试 → Merged Plugin → "显示/隐藏构件信息面板"
4. **观察结果**:
   - ✅ 面板应显示
   - ✅ 应显示 "Minimal JavaScript Bridge Test" 标题
   - ✅ 状态指示器应显示连接状态

### 3. 收集调试信息

#### C++ 日志 (Add-On Output窗口)
1. 打开: **Window > Add-On Output**
2. 查找关键消息:
```
MergedPluginPalette构造函数: 开始初始化，PaletteResId=32520
MergedPluginPalette: 加载HTML资源，大小=XXXXX字节
InitializeJavaScriptBridge: 开始初始化JavaScript桥接
InitializeJavaScriptBridge: JavaScript桥接注册完成
```

#### JavaScript 控制台 (F12)
1. 在面板中按 **F12** 打开开发者工具
2. 查看控制台输出:
```
Page loaded, starting bridge check...
Attempt 1...
Checking ACAPI...
✅ ACAPI found with X functions
✅ Plugin version: v0.2.2.32
```

### 4. 预期成功结果
```
面板显示:
- 标题: "Minimal JavaScript Bridge Test"
- 状态: "Bridge Connected (X functions)" (绿色)
- 日志区域显示连接成功消息

控制台显示:
- ACAPI对象找到
- 插件版本正确显示
- 函数调用成功
```

## 故障排除

### 如果面板仍然空白
1. **检查C++日志**: 确认HTML资源加载成功（大小>0）
2. **检查JavaScript控制台**: 是否有错误消息
3. **启用CEF调试** (macOS):
```bash
defaults write com.graphisoft.debug DG.CefDebugPort 9222
defaults write com.graphisoft.debug DG.CefUseFixedDebugPort true
```
4. **重启Archicad**后重试

### 如果ACAPI未定义
1. **等待重试**: MinimalTest.html会自动重试10次（500ms间隔）
2. **检查时序**: JavaScript桥接可能在HTML加载后尚未注册
3. **检查构造函数日志**: 确认JavaScript桥接初始化完成

### 如果插件不加载
1. **验证插件位置**: 确认bundle在正确的Add-Ons目录
2. **检查Archicad版本**: 需要ARCHICAD 29.x
3. **查看系统日志**: 是否有崩溃报告

## 下一步

### 测试成功后的操作
1. **验证完整功能**: 测试GetSelectedElements等函数
2. **恢复控件功能**: 逐步添加回必要的控件
3. **更新HTML界面**: 使用完整功能的界面

### 需要进一步修复
如果问题仍然存在，可能需要:
1. **彻底重构**: 创建只包含浏览器控件的简化版本
2. **资源文件更新**: 在GDLG 32520中添加缺失的控件
3. **代码清理**: 移除对不存在控件的所有引用

## 联系支持
如果问题仍未解决，请提供:
1. **完整的C++日志** (Add-On Output窗口)
2. **JavaScript控制台截图**
3. **面板显示截图**
4. **Archicad版本信息**

使用我们的日志分析工具:
```bash
python3 log_analyzer.py --cpp-log cpp_output.txt --js-log js_console.txt
```

## 版本历史
- **0.2.2.32**: 修复控件ID冲突，禁用不存在控件的操作，使用MinimalTest.html
- **0.2.2.31**: 更新DATA资源使用MinimalTest.html
- **0.2.2.30**: 修复BrowserId从1改为20
- **0.2.2.29**: 初始修复版本（资源文件修复）