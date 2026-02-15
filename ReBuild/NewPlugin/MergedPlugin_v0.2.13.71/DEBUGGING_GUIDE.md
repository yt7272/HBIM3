# HBIM Plugin 调试指南 (v0.2.13.71)

## 问题描述
用户报告：选择构件后，构件信息栏目没有按钮可以操作。

## 已实施的修复
1. **修复 JavaScript 语法错误**：缺失的闭合大括号（第1931行）
2. **增强调试日志**：在整个HTML界面添加详细的调试输出
3. **改进UI状态逻辑**：`hasComponentInfo()` 函数现在为已选择的构件总是返回 `true`
4. **添加UI状态指示器**：在界面底部显示当前UI状态（未选择构件 / 已选择构件无信息 / 已选择构件有信息）
5. **修复版本显示**：确保HTML显示正确的版本号 (0.2.13.71)
6. **添加定期轮询**：每250ms检查一次选择变化，确保及时响应
7. **添加“添加信息”按钮**：在“已选择构件无信息”状态下显示按钮，确保用户始终有可操作的按钮

## 调试步骤

### 1. 验证插件版本
- 启动 ArchiCAD 29
- 打开菜单 测试 → NewPlugin
- 在插件面板顶部检查版本号应为 **0.2.13.71**
- 如果版本号不是 0.2.13.71，请重新启动 ArchiCAD 或重新安装插件

### 2. 启用调试模式
- 在插件面板中，按 F12 打开浏览器开发者工具
- 切换到 Console（控制台）标签页
- 观察是否有 `[HBIM Debug]` 日志输出

### 3. 测试构件选择
- 在 ArchiCAD 中选择一个构件（墙、门、窗等）
- 观察控制台输出，应该看到类似：
  ```
  [HBIM Debug] UpdateSelectedElements called
  [HBIM Debug] GetSelectedElements returned: 1 elements [...]
  [HBIM Debug] Element check: {currentElementGuid: null, elementGuid: "...", changed: true}
  [HBIM Debug] Element changed from null to ...
  [HBIM Debug] updateUIState: {hasSelection: true, hasInfo: true, ...}
  [HBIM Debug] UI State: state-has-info
  ```

### 4. 检查UI状态指示器
- 在插件面板底部，查找 "UI状态:" 显示
- 可能的显示值：
  - `未选择构件` - 未选择任何构件
  - `已选择构件无信息` - 已选择构件但未保存信息
  - `已选择构件有信息` - 已选择构件且可以操作
- 选择构件后应该显示 **`已选择构件有信息`**

### 5. 检查按钮状态
- 当UI状态为 `已选择构件有信息` 时，应该看到三个按钮：
  - `选择照片` (可能为禁用状态，需要GlobalId)
  - `查看照片` (可能为禁用状态，需要照片和GlobalId)
  - `编辑信息` (应该为可用状态)
- 当UI状态为 `已选择构件无信息` 时，应该看到一个按钮：
  - `添加信息` (点击进入编辑模式)
- 如果按钮仍然不可见，检查CSS类：
  - 按 F12 → Elements 标签页
  - 检查 `<body>` 元素的类，应该包含 `state-no-info` 或 `state-has-info`
  - 检查 `<div class="has-info-content">` 或 `<div class="no-info-message">` 的 `display` 属性

### 6. 运行诊断命令（在控制台中）
```javascript
// 检查 ACAPI 对象
console.log("ACAPI available:", typeof ACAPI !== 'undefined');
if (typeof ACAPI !== 'undefined') {
    console.log("ACAPI functions:", Object.keys(ACAPI));
}

// 检查当前状态变量
console.log("currentElementGuid:", currentElementGuid);
console.log("currentGlobalId:", currentGlobalId);
console.log("hasComponentInfo():", hasComponentInfo());
console.log("isEditMode:", isEditMode);

// 手动触发选择更新
UpdateSelectedElements();

// 手动更新UI状态
updateUIState();
```

### 7. 收集调试信息
如果问题仍然存在，请提供以下信息：

1. **控制台完整输出**（包含所有 `[HBIM Debug]` 日志）
2. **选择的构件类型**（墙、门、窗等）
3. **UI状态指示器的显示值**
4. **`<body>` 元素的类列表**（在 Elements 面板中查看）
5. **任何错误信息**（红色错误）

## 常见问题及解决方案

### 问题1：版本号不正确
**症状**：版本显示为 0.2.13.48 或其他旧版本
**解决**：
1. 删除插件目录：`~/Library/Application Support/GRAPHISOFT/ARCHICAD 29/Add-Ons/MergedPlugin.bundle`
2. 重新运行安装脚本：`cd NewPlugin && ./install.sh`
3. 重启 ArchiCAD

### 问题2：没有调试日志输出
**症状**：控制台没有 `[HBIM Debug]` 日志
**解决**：
1. 检查 HTML 是否已更新：确保插件版本为 0.2.13.71
2. 调试模式可能被禁用：检查 `const DEBUG = true;` 是否设置

### 问题3：UI状态卡在 "未选择构件"
**症状**：选择构件后UI状态仍显示 "未选择构件"
**可能原因**：
1. `UpdateSelectedElements()` 未被调用
2. `ACAPI.GetSelectedElements()` 返回空数组
3. JavaScript 错误导致执行中断

**诊断**：
- 检查控制台是否有 JavaScript 错误
- 检查 `ACAPI.GetSelectedElements()` 的返回值
- 检查 `currentElementGuid` 是否为 null

### 问题4：按钮可见但禁用
**症状**：按钮可见但呈灰色不可点击状态
**原因**：
- `选择照片` 和 `查看照片` 按钮需要 GlobalId 属性
- 如果构件没有 GlobalId 属性，这些按钮会被禁用（符合护栏要求）
- `编辑信息` 按钮应该始终可用

## 技术细节

### UI状态逻辑
```javascript
function hasComponentInfo() {
    // 如果有选择的构件，总是显示编辑界面（即使信息为空）
    if (currentElementGuid !== null) {
        return true;
    }
    // 否则检查是否有已保存的信息
    const componentId = document.getElementById('component-id').value.trim();
    const photoDescription = document.getElementById('photo-description').value.trim();
    return componentId !== '' || photoDescription !== '' || currentPhotoPaths.length > 0;
}
```

### 状态CSS类
- `.state-no-selection` - 未选择构件
- `.state-no-info` - 已选择构件但无信息
- `.state-has-info` - 已选择构件且有信息

### 护栏要求
- **无缓存系统**：IFC属性读取不使用LRU缓存
- **无性能优化**：不包含延迟加载、基准测试等高级功能
- **无GUID回退**：照片功能严格要求 GlobalId 属性
- **无单独模态窗口**：所有功能都在单个HTML界面中
- **无传统版本控制**：使用日期格式版本号 (0.month.day.build)

## 自动化诊断工具
我们提供了一个自动化诊断脚本，可以快速收集所有调试信息：

1. 在 ArchiCAD 中打开插件面板
2. 按 F12 打开浏览器开发者工具
3. 切换到 Console (控制台) 标签页
4. 复制并粘贴以下代码：
```javascript
// 从文件 debug_collector.js 复制内容
// 或直接运行：await import('/path/to/debug_collector.js')
```

或者，可以直接运行诊断脚本：
```bash
# 在终端中运行
node debug_collector.js
```

脚本将自动检查：
- ACAPI 对象可用性
- 全局变量状态
- UI 元素可见性
- 函数存在性
- 生成诊断报告

## 联系支持
如果经过以上调试步骤问题仍未解决，请提供：
1. 完整的控制台输出（包括诊断脚本输出）
2. 选择的构件类型
3. 插件版本号
4. ArchiCAD 版本号

这将帮助我们进一步诊断问题。