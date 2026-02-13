# HBIM构件信息输入工具 - 测试指南

本文档提供详细的测试步骤，确保插件在Archicad环境中正常工作。

## 测试环境要求

- **Archicad版本**: 29.x (与插件开发版本匹配)
- **操作系统**: macOS 10.15+ 或 Windows 10+
- **插件版本**: 0.2.13.47 (日期版本: 0.月.日.构建号)

## 1. 安装验证测试

### 1.1 检查插件安装
```bash
# macOS
ls -la ~/Library/Application\ Support/GRAPHISOFT/ARCHICAD\ 29/Add-Ons/MergedPlugin.bundle/

# Windows
dir "C:\Users\[用户名]\AppData\Roaming\GRAPHISOFT\ARCHICAD 29\Add-Ons\MergedPlugin.bundle"
```

**预期结果**:
- `MergedPlugin.bundle` 目录存在
- 包含以下关键文件:
  - `Contents/MacOS/MergedPlugin` (macOS可执行文件)
  - `Contents/Info.plist` (插件配置文件)
  - `Contents/Resources/English.lproj/DATA_100.rsrd` (HTML资源)

### 1.2 版本验证
```bash
# 查看插件版本
grep -A1 CFBundleVersion ~/Library/Application\ Support/GRAPHISOFT/ARCHICAD\ 29/Add-Ons/MergedPlugin.bundle/Contents/Info.plist
```

**预期结果**: 显示当前版本号 (如: `0.2.13.46`)

## 2. Archicad集成测试

### 2.1 启动Archicad
1. 启动Archicad 29
2. 打开任意项目文件 (新建或现有项目)

### 2.2 启用插件
1. 菜单: **文件 → 插件管理器**
2. 在插件列表中找到 **"NewPlugin"**
3. 确保插件已勾选启用
4. 点击 **确定** 保存设置

### 2.3 打开插件面板
1. 菜单: **测试 → NewPlugin**
2. 预期结果: 弹出插件面板窗口

**面板初始状态验证**:
- 窗口标题: "HBIM构件信息输入工具"
- 右上角显示版本号: "V0.2.13.46"
- 界面分为三个信息组:
  - **构件信息** (默认展开)
  - **构件照片** (默认展开)
  - **IFC属性** (默认展开)
- 初始状态应为 **"未选择构件"**

## 3. 功能测试

### 3.1 三状态UI测试

#### 状态1: 未选择构件
1. 确保Archicad中没有选择任何构件
2. 观察插件面板显示:
   - 显示提示: "未选择构件"
   - 构件信息组: 显示构件GUID和类型为空
   - 构件照片组: 显示"选择构件以查看照片"
   - IFC属性组: 显示"选择构件以查看IFC属性"

#### 状态2: 选择构件但无信息
1. 在Archicad中选择一个构件 (墙、门、窗等)
2. 观察插件面板变化:
   - 构件信息组: 显示构件的GUID和类型
   - 构件照片组: 显示"该构件暂无照片信息"
   - IFC属性组:
     - 如果构件有IFC属性: 显示属性表格
     - 如果无IFC属性: 显示"该构件没有IFC属性"

#### 状态3: 选择构件且有信息
1. 在状态2的基础上:
   - 输入 **构件编号** (如: "W-001")
   - 输入 **照片说明** (如: "西立面墙体")
   - 点击 **"选择照片"** 按钮，选择1-2张测试照片
   - 点击 **"保存"** 按钮
2. 观察变化:
   - 构件照片组: 显示照片预览和导航按钮
   - 界面显示照片缩略图
   - 可以点击左右箭头切换照片

### 3.2 照片功能测试

#### 照片选择
1. 选择构件后，点击 **"选择照片"** 按钮
2. 在文件选择对话框中选择测试图片 (JPG/PNG格式)
3. 预期结果:
   - 照片被添加到构件
   - 照片预览区域显示第一张照片
   - 显示照片计数 (如: "1/2")

#### 照片导航
1. 如果有2张以上照片:
   - 点击 **"上一张"** 箭头按钮: 显示前一张照片
   - 点击 **"下一张"** 箭头按钮: 显示下一张照片
2. 预期结果: 照片切换流畅，计数器更新

#### 照片查看
1. 点击 **"查看照片"** 按钮
2. 预期结果: 在默认图片查看器中打开当前照片

#### 照片限制验证
1. 选择 **没有GlobalId属性** 的构件
2. 尝试点击 **"选择照片"** 按钮
3. 预期结果: 按钮禁用或显示错误提示 (根据实现)

### 3.3 IFC属性测试

#### IFC属性显示
1. 选择一个 **IFC兼容** 的构件 (如导入的IFC模型)
2. 观察IFC属性组:
   - 显示属性表格，包含: 属性集、属性名、值、类型、单位
   - 支持搜索过滤 (输入关键词)
   - 支持分组展开/折叠

#### 非IFC构件
1. 选择一个 **非IFC** 构件 (普通Archicad元素)
2. 观察IFC属性组:
   - 显示"该构件没有IFC属性"或空状态

### 3.4 编辑模式测试

#### 进入编辑模式
1. 选择有信息的构件
2. 点击 **"编辑"** 按钮
3. 预期结果:
   - 输入框变为可编辑状态
   - 显示 **"保存"** 和 **"取消"** 按钮
   - **"编辑"** 按钮隐藏

#### 保存修改
1. 修改构件编号或照片说明
2. 点击 **"保存"** 按钮
3. 预期结果:
   - 修改被保存
   - 返回查看模式
   - 显示更新后的信息

#### 取消修改
1. 修改信息但不保存
2. 点击 **"取消"** 按钮
3. 预期结果:
   - 修改被丢弃
   - 返回查看模式
   - 显示原始信息

#### 编辑模式切换构件
1. 在编辑模式下，选择另一个构件
2. 预期结果:
   - 自动退出编辑模式
   - 显示新构件的信息
   - 未保存的修改被丢弃

### 3.5 未保存更改指示器
1. 在编辑模式下修改信息
2. 预期结果: 标题栏显示 **"*"** 符号表示有未保存更改
3. 保存后: **"*"** 符号消失

## 4. JavaScript API测试

### 4.1 控制台测试
1. 在Archicad中打开插件面板
2. 按 **F12** 打开浏览器开发者工具
3. 在控制台中执行以下测试:

```javascript
// 运行所有测试
window.testMergedPlugin.runAllTests();

// 单独运行组件测试
window.testComponentInfo.runAllTests();

// 单独运行IFC属性测试
window.testIFCProperties.runAllTests();

// 测试ACAPI桥接函数
console.log("ACAPI对象:", window.ACAPI);
console.log("可用函数:", Object.keys(window.ACAPI || {}));
```

### 4.2 ACAPI函数测试
所有14个ACAPI函数都应正常工作:
1. `GetSelectedElements()` - 获取选中构件
2. `GetPluginVersion()` - 获取插件版本
3. `GetAllIFCProperties()` - 获取IFC属性
4. `GetCachedIFCProperties()` - 获取缓存的IFC属性
5. `ComponentInfo_GetComponentInfo()` - 获取构件信息
6. `ComponentInfo_SaveComponentInfo()` - 保存构件信息
7. `TestBridge()` - 测试桥接
8. `LogDebug()` - 调试日志
9. `ComponentInfo_SelectPhotos()` - 选择照片
10. `ComponentInfo_GetPhotoFullPath()` - 获取照片完整路径
11. `ComponentInfo_ViewPhotos()` - 查看照片
12. `ComponentInfo_GetPhotoBase64()` - 获取Base64编码照片
13. `ComponentInfo_SetEditMode()` - 设置编辑模式
14. `ComponentInfo_SetUnsavedChanges()` - 设置未保存更改状态

## 5. 性能测试

### 5.1 响应时间
1. 选择构件: 面板应在 **1秒内** 更新
2. 加载IFC属性: 对于复杂构件，应在 **3秒内** 完成
3. 照片预览: 应在 **2秒内** 显示

### 5.2 内存使用
1. 观察Archicad内存使用情况
2. 多次打开/关闭插件面板，内存不应持续增长

## 6. 边界条件测试

### 6.1 空选择
- 取消所有构件选择 → 面板应返回状态1
- 快速切换选择 → 不应崩溃或卡死

### 6.2 多构件选择
- 选择多个构件 → 面板应显示第一个构件的信息

### 6.3 大文件处理
- 选择包含大量IFC属性的构件 → 应能正常显示
- 选择包含多张高分辨率照片的构件 → 应能正常预览

### 6.4 特殊字符
- 构件编号包含特殊字符: `#`, `&`, `中文` → 应能正确保存和显示
- 照片说明包含换行符等 → 应能正确处理

## 7. 错误处理测试

### 7.1 插件加载失败
1. 删除必要的插件文件
2. 启动Archicad
3. 预期结果: Archicad应正常启动，插件管理器显示加载失败

### 7.2 资源加载失败
1. 模拟HTML资源加载失败
2. 预期结果: 显示友好的错误提示

### 7.3 API调用失败
1. 模拟ACAPI函数调用异常
2. 预期结果: 在控制台显示错误日志，界面不崩溃

## 8. 跨平台测试 (如适用)

### 8.1 macOS
- [ ] Archicad 29.3100
- [ ] macOS 10.15 Catalina
- [ ] macOS 11 Big Sur
- [ ] macOS 12 Monterey
- [ ] macOS 13 Ventura
- [ ] macOS 14 Sonoma

### 8.2 Windows
- [ ] Archicad 29.3100
- [ ] Windows 10
- [ ] Windows 11

## 9. 测试记录表

| 测试项目 | 测试日期 | 测试结果 | 问题描述 | 解决状态 |
|---------|---------|---------|---------|---------|
| 安装验证 | | ✅/❌ | | |
| 三状态UI | | ✅/❌ | | |
| 照片功能 | | ✅/❌ | | |
| IFC属性 | | ✅/❌ | | |
| 编辑模式 | | ✅/❌ | | |
| JavaScript API | | ✅/❌ | | |
| 性能测试 | | ✅/❌ | | |
| 边界条件 | | ✅/❌ | | |
| 错误处理 | | ✅/❌ | | |

## 10. 故障排除

### 10.1 常见问题

#### 问题: 插件面板打开但显示空白
**可能原因**:
1. HTML资源未正确嵌入
2. JavaScript加载错误

**解决方案**:
1. 检查插件bundle完整性
2. 在控制台中查看JavaScript错误
3. 重新运行 `./build.sh` 重新构建插件

#### 问题: 照片功能不可用
**可能原因**:
1. 构件没有GlobalId属性
2. 照片文件夹权限问题

**解决方案**:
1. 确保构件是IFC兼容的 (具有GlobalId)
2. 检查项目文件夹写入权限

#### 问题: IFC属性不显示
**可能原因**:
1. 构件没有IFC属性
2. IFC API调用失败

**解决方案**:
1. 测试其他IFC兼容构件
2. 在控制台中检查错误日志

### 10.2 调试信息收集
1. 打开浏览器控制台 (F12)
2. 收集所有错误和警告信息
3. 测试ACAPI函数可用性:
```javascript
// 检查ACAPI对象
console.log("ACAPI available:", typeof ACAPI !== 'undefined');
console.log("ACAPI functions:", Object.keys(ACAPI || {}));

// 测试基本功能
try {
    const version = ACAPI.GetPluginVersion();
    console.log("Plugin version:", version);
} catch (err) {
    console.error("GetPluginVersion failed:", err);
}
```

## 11. 测试完成标准

- [ ] 所有核心功能正常工作
- [ ] 三状态UI逻辑正确
- [ ] 照片功能完整 (选择、预览、导航、查看)
- [ ] IFC属性正确显示
- [ ] 编辑模式逻辑正确
- [ ] JavaScript API全部可用
- [ ] 无崩溃或内存泄漏
- [ ] 错误处理合理
- [ ] 文档与实现一致

---

**最后更新**: 2026-02-13  
**插件版本**: 0.2.13.47  
**测试状态**: 待执行