# HBIM构件信息录入插件 - 功能说明文档

## 插件概述

HBIM构件信息录入插件为ArchiCAD构件提供HBIM属性信息管理功能，允许用户为构件添加和编辑"HBIM构件编号"和"HBIM构件说明"两个自定义属性。

## 版本信息

- **当前版本**: 0.2.15.64
- **版本格式**: 0.月.日.构建次数
- **ArchiCAD版本**: 29.3100
- **MDID签名**: Developer ID 625438022, Local ID 170868903

## 主要功能

### 1. HBIM属性管理

#### 支持的属性
- **HBIM构件编号**: 文本类型，用于标识构件的编号
- **HBIM构件说明**: 文本类型，用于描述构件的详细信息

#### 功能流程

**添加新属性**:
1. 在ArchiCAD中选择一个构件
2. 打开插件面板
3. 点击"添加"按钮
4. 在输入框中填写"HBIM构件编号"和"HBIM构件说明"
5. 点击"保存"按钮确认

**编辑现有属性**:
1. 选择已有HBIM属性的构件
2. 面板会自动显示当前属性值（只读模式）
3. 点击"属性编辑"按钮进入编辑模式
4. 修改属性值
5. 点击"保存"按钮或"取消"按钮

**取消编辑**:
- 点击"取消"按钮可恢复到编辑前的值
- 切换构件时会自动保存并退出编辑模式

### 2. 选择验证与保护

#### 选择检查
- **未选择构件时点击"添加"**: 弹出提示"请先选择一个构件"，不进入编辑模式
- **防止空输入**: 确保必须有选中的构件才能编辑属性

#### 编辑模式保护
- **编辑时切换构件**: 自动保存当前编辑内容到原构件，退出编辑模式
- **提示消息**: "你已经切换构件，对当前构件的编辑已保存并退出。"
- **防止数据混淆**: 确保属性保存到正确的构件

### 3. IFC属性显示

插件集成IFC属性读取功能，显示：
- **构件类型**: 构件的IFC类型信息
- **构件编号**: 构件的GlobalId标识

### 4. HBIM图像管理

#### 支持的功能
- **选择图片**: 支持多选JPG/PNG格式图片
- **图片存储**: 自动复制到项目文件夹下的HBIM_Images目录
- **图片命名**: 使用时间戳重命名，防止文件名冲突
- **属性存储**: 图片路径以JSON格式存储在属性中
- **图片导航**: 支持上一张/下一张浏览
- **图片删除**: 支持删除当前图片

#### 图片存储结构
```
{ProjectFolder}/
└── HBIM_Images_{projectHash}/
    └── {elementGlobalId}/
        ├── 1705312345678_photo1.jpg
        ├── 1705312349000_photo2.png
        └── ...
```

#### 使用流程
1. 选中一个构件
2. 点击"选择图片"按钮
3. 在文件对话框中选择一张或多张图片
4. 图片会自动复制到项目文件夹
5. 图片路径会保存到构件属性中

#### 界面控件
- **图片数量**: 显示当前构件的图片数量
- **当前图片**: 显示当前查看的图片序号 (如 1/3)
- **选择图片**: 打开文件选择对话框
- **删除当前**: 删除当前显示的图片
- **上一张/下一张**: 浏览多张图片

## 用户界面

### 面板布局

```
┌─────────────────────────────────────────────┐
│  HBIM构件信息录入 v0.2.15.64          │
│  ────────────────────────────────────  │
│                                      │
│  构件类型: [IFC类型]                   │
│  构件编号: [GlobalId]                  │
│                                      │
│  ────────────────────────────────────  │
│                                      │
│  HBIM属性信息                          │
│  ────────────────────────────────────  │
│                                      │
│  HBIM构件编号: [输入框/显示值]         │
│  HBIM构件说明: [输入框/显示值]         │
│                                      │
│  [添加/属性编辑] [保存] [取消]         │
│                                      │
│  ────────────────────────────────────  │
│                                      │
│  HBIM构件图片                          │
│  ────────────────────────────────────  │
│                                      │
│  图片数量: [3]    当前图片: [1/3]     │
│  [选择图片] [删除当前] [←] [→]        │
│                                      │
│  图片预览区域                          │
│                                      │
└─────────────────────────────────────────────┘
```

### 控件状态

**无属性模式**:
- 显示"添加"按钮
- 隐藏"保存"和"取消"按钮
- 显示空的输入框

**显示属性模式**:
- 显示属性值（只读）
- 显示"属性编辑"按钮
- 隐藏"保存"和"取消"按钮

**编辑模式**:
- 显示可编辑的输入框
- 显示"保存"和"取消"按钮
- 按钮文本: "添加" → "保存"

## 技术实现

### 核心类

**PluginPalette** - 主面板类
- 继承自 `DG::Palette`
- 实现了单例模式
- 管理所有UI控件和业务逻辑

### 关键成员变量

```cpp
// HBIM属性状态
bool hasHBIMProperties;           // 当前构件是否有HBIM属性
bool isHBIMEditMode;             // 是否处于编辑模式
GS::UniString originalHBIMId;   // 编辑前的HBIM构件编号
GS::UniString originalHBIMDesc;  // 编辑前的HBIM构件说明

API_Guid hbimGroupGuid;           // HBIM属性组GUID
API_Guid hbimIdGuid;             // HBIM构件编号属性GUID
API_Guid hbimDescGuid;            // HBIM构件说明属性GUID
API_Guid currentElemGuid;         // 当前选中的构件GUID
```

### 关键方法

**属性管理**:
- `EnterHBIMEditMode()` - 进入编辑模式，保存当前值
- `ExitHBIMEditMode(bool save)` - 退出编辑模式，保存或恢复值
- `CheckHBIMProperties(const API_Guid&)` - 检查构件是否有HBIM属性
- `ReadHBIMProperties(const API_Guid&)` - 读取构件的HBIM属性
- `WriteHBIMProperties(const API_Guid&)` - 写入构件的HBIM属性

**初始化**:
- `EnsureHBIMPropertiesInitialized()` - 确保属性组和定义已创建
- `TryFindExistingHBIMPropertyGroupAndDefinitions()` - 查找现有属性组和定义

**UI更新**:
- `UpdateHBIMUI()` - 根据状态更新UI显示

**事件处理**:
- `ButtonClicked()` - 处理按钮点击事件
- `SelectionChangeHandler()` - 处理构件选择变化

### 选择处理逻辑

```cpp
GSErrCode PluginPalette::SelectionChangeHandler (const API_Neig* selElemNeig)
{
    // 1. 检测编辑模式下的构件切换
    if (instance.isHBIMEditMode && selElemNeig != nullptr) {
        if (selElemNeig->guid != instance.currentElemGuid) {
            // 2. 自动保存并退出编辑模式
            instance.ExitHBIMEditMode(true);  // 保存到 currentElemGuid
            // 3. 显示提示
            DG::InformationAlert("提示", "你已经切换构件，对当前构件的编辑已保存并退出。", "确定");
        }
    }
    // 4. 更新 currentElemGuid
    if (selElemNeig != nullptr) {
        instance.currentElemGuid = selElemNeig->guid;
        // ... 更新显示
    }
}
```

### 按钮处理逻辑

```cpp
void PluginPalette::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource() == &hbimActionButton) {
        if (isHBIMEditMode) {
            // 保存
            ExitHBIMEditMode(true);
        } else {
            // 添加/编辑：先检查是否有选中的构件
            if (currentElemGuid == APINULLGuid) {
                DG::InformationAlert("提示", "请先选择一个构件", "确定");
                return;  // 不进入编辑模式
            }
            EnterHBIMEditMode();
        }
    } else if (ev.GetSource() == &hbimCancelButton) {
        // 取消
        ExitHBIMEditMode(false);
    }
}
```

### 保存逻辑

```cpp
void PluginPalette::ExitHBIMEditMode (bool save)
{
    if (!isHBIMEditMode) return;
    
    if (save) {
        // 保存到 currentElemGuid（当前正在编辑的构件）
        if (currentElemGuid != APINULLGuid) {
            WriteHBIMProperties(currentElemGuid);
            hasHBIMProperties = true;
        }
    } else {
        // 恢复原始值
        if (hasHBIMProperties) {
            hbimIdValue.SetText(originalHBIMId);
            hbimDescValue.SetText(originalHBIMDesc);
        }
    }
    
    isHBIMEditMode = false;
    UpdateHBIMUI();
}
```

## 属性系统

### 属性组

- **名称**: "HBIM属性信息"
- **创建时机**: 首次保存属性时，使用 `ACAPI_CallUndoableCommand`

### 属性定义

1. **HBIM构件编号** (文本类型)
   - 可用性: 所有分类项
   - 默认值: 空字符串

2. **HBIM构件说明** (文本类型)
   - 可用性: 所有分类项
   - 默认值: 空字符串

### 数据存储

- 属性值存储在ArchiCAD Property系统中
- 使用API: `ACAPI_Property_SetPropertyValue`
- 支持撤销/重做操作

## 构建说明

### 构建脚本

```bash
cd HBIMComponentEntry
./build.sh              # 正常构建
./build.sh clean         # 清理构建（重置构建计数器）
```

### 输出

- **位置**: `build/Release/HBIMComponentEntry.bundle`
- **架构**: arm64 (macOS)
- **依赖**: ArchiCAD API Development Kit 29.3100

### 安装步骤

1. 将 `build/Release/HBIMComponentEntry.bundle` 复制到:
   ```
   ~/Library/Application Support/GRAPHISOFT/ARCHICAD 29/Add-Ons/
   ```

2. 启动 ArchiCAD 29

3. 在"选项" > "插件管理器"中启用 "HBIMComponentEntry"

4. 通过菜单"测试" > "HBIMComponentEntry"打开插件面板

## 已修复的问题

### v0.2.15.43 (版本0.2.15.46之前)

1. ✅ 添加选择验证
   - **问题**: 未选择构件时点击"添加"会允许空输入
   - **修复**: 在 `ButtonClicked` 中检查 `currentElemGuid == APINULLGuid`，显示提示

2. ✅ 添加编辑模式保护
   - **问题**: 编辑时点击另一个构件会静默切换，导致数据混乱
   - **修复**: 在 `SelectionChangeHandler` 中检测编辑模式下的构件切换
   - **行为**: 自动保存到原构件 + 显示提示 "你已经切换构件，对当前构件的编辑已保存并退出。"

3. ✅ 修复保存到错误构件的Bug
   - **问题**: `ExitHBIMEditMode` 从当前选择获取构件来保存，但选择已变成新构件
   - **修复**: 保存到 `currentElemGuid`（原构件），而不是当前选择

4. ✅ 移除调试弹窗
   - **问题**: 调试期间添加的 `DG::InformationAlert` 影响用户体验
   - **修复**: 清理所有调试弹窗

## 使用场景

### 场景1: 为新构件添加HBIM属性

1. 选择构件A
2. 打开插件面板
3. 面板显示 "添加"按钮
4. 点击"添加"按钮
5. 输入"HBIM构件编号"和"HBIM构件说明"
6. 点击"保存"按钮
7. 属性保存成功

### 场景2: 查看构件的HBIM属性

1. 选择已有HBIM属性的构件A
2. 面板自动显示构件A的HBIM属性值（只读）
3. 显示"属性编辑"按钮

### 场景3: 编辑构件的HBIM属性

1. 选择已有HBIM属性的构件A
2. 点击"属性编辑"按钮
3. 修改属性值
4. 点击"保存"或"取消"按钮

### 场景4: 编辑时切换构件

1. 选择构件A并进入编辑模式
2. 开始修改属性值（未保存）
3. 点击另一个构件B
4. **自动触发**: 构件A的编辑内容被保存
5. **显示提示**: "你已经切换构件，对当前构件的编辑已保存并退出。"
6. 面板切换到构件B

### 场景5: 未选择构件时点击添加

1. 确保没有选中任何构件
2. 点击"添加"按钮
3. **弹出提示**: "请先选择一个构件"
4. 不会进入编辑模式

## 已知限制

1. **同时编辑**: 同一时间只能编辑一个构件的HBIM属性
2. **撤销操作**: 每次保存都是独立的可撤销操作
3. **属性组**: HBIM属性组一旦创建无法删除（ArchiCAD限制）

## 技术规格

- **编程语言**: C++20
- **UI框架**: DG (Dialog Graphics)
- **API版本**: ArchiCAD API Development Kit 29.3100
- **编译器**: Clang (macOS)
- **目标平台**: macOS arm64

## 更新日志

### v0.2.15.46 (2026-02-15)
- 修复保存逻辑: `ExitHBIMEditMode` 现在保存到 `currentElemGuid`（原构件）而不是当前选择
- 修复选择检查: 添加编辑模式下的构件切换保护
- 移除调试弹窗: 清理所有临时调试 `DG::InformationAlert` 调用

### v0.2.15.40~45
- 添加选择验证: 未选择构件时点击"添加"会提示用户
- 实现自动保存: 编辑时切换构件会自动保存并退出编辑模式
- 显示友好提示: 告知用户编辑已保存

### 早期版本
- v0.2.15.0: 初始HBIM属性管理功能
- 实现属性组创建、属性定义、读写操作
- 集成IFC属性显示功能
