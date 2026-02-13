# HBIM 构件信息输入工具插件

Archicad 插件，用于管理构件扩展信息、照片和IFC属性。

## 功能特性

- **构件信息管理**: 添加构件编号、照片说明等自定义属性
- **构件照片管理**: 为构件选择照片并存储到项目文件夹
- **IFC属性查看**: 查看构件的IFC属性信息
- **现代化界面**: 工业深色主题的HTML界面，支持三种状态显示
- **照片预览**: 支持照片分页浏览和预览

## 安装

### macOS
1. 构建插件:
   ```bash
   cd NewPlugin
   ./build.sh
   ```
2. 安装插件（任选一种方式）:
   - **使用安装脚本**（推荐）:
     ```bash
     ./install.sh
     ```
   - **手动复制**:
     ```bash
     cp -R build/Release/NewPlugin.bundle ~/Library/Application\ Support/GRAPHISOFT/ARCHICAD\ 29/Add-Ons/
     ```
3. 启动Archicad，在插件管理器中启用 "NewPlugin"

### Windows
1. 使用Visual Studio或CMake构建插件
2. 将生成的插件复制到:
   ```
   C:\Users\[用户名]\AppData\Roaming\GRAPHISOFT\ARCHICAD 29\Add-Ons\
   ```
3. 启动Archicad，在插件管理器中启用 "NewPlugin"

## 使用说明

1. 在Archicad中通过菜单 **测试 → NewPlugin** 打开插件面板
2. 选择构件后，面板将显示三种状态:
   - **未选择构件**: 提示选择构件
   - **已选择构件但无信息**: 显示构件GUID和类型，可输入信息
   - **已选择构件且有信息**: 显示已有信息，可编辑

### 构件信息
- **构件编号**: 构件的唯一标识符
- **照片说明**: 对构件照片的描述
- **照片管理**: 点击"选择照片"按钮选择照片文件

### 照片预览
- 支持多张照片预览
- 使用左右箭头切换照片
- 显示当前照片位置（如 "1/3"）

### IFC属性
- 自动显示构件的IFC属性
- 按属性组分类显示

## 开发

### 项目结构
```
NewPlugin/
├── Src/                    # 源代码
│   ├── PluginMain.cpp     # 插件入口点
│   ├── ComponentInfo.cpp  # 构件信息管理
│   ├── PropertyUtils.cpp  # IFC属性工具
│   └── PluginPalette.cpp  # HTML界面和JavaScript桥接
├── RFIX/                  # 资源文件
│   └── index.html        # HTML界面
├── RFIX.mac/             # macOS资源
├── Resources/            # 其他资源
└── tests/               # 测试文件
```

### 构建
```bash
# 正常构建（自动版本号递增）
./build.sh

# 清理构建
./build.sh clean
```

### 版本号
插件使用日期格式版本号: `0.月.日.构建次数`
示例: `0.2.13.42` 表示2月13日第42次构建

### 测试
```bash
# JavaScript单元测试（使用Jest）
npm test

# 端到端测试
node test-e2e.js

# UI状态测试
node test-ui-states.js

# 在Archicad浏览器控制台中测试
window.testIFCProperties.runAllTests();  # IFC属性测试
# 运行所有可用的测试函数
```

详细测试指南请参阅 [TESTING.md](TESTING.md)

## JavaScript API

插件通过 `ACAPI` 对象暴露以下函数:

| 函数 | 描述 |
|------|------|
| `GetSelectedElements()` | 获取当前选中的构件 |
| `GetPluginVersion()` | 获取插件版本 |
| `GetAllIFCProperties(elementGuid)` | 获取构件的所有IFC属性 |
| `GetCachedIFCProperties(elementGuid)` | 获取缓存的IFC属性 |
| `ComponentInfo_GetComponentInfo(elementGuid)` | 获取构件信息 |
| `ComponentInfo_SaveComponentInfo(elementGuid, data)` | 保存构件信息 |
| `ComponentInfo_SelectPhotos()` | 选择照片文件 |
| `ComponentInfo_GetPhotoFullPath(relativePath)` | 获取照片完整路径 |
| `ComponentInfo_ViewPhotos(photoPaths)` | 查看照片 |
| `ComponentInfo_GetPhotoBase64(photoPath)` | 获取照片Base64编码 |
| `ComponentInfo_SetEditMode(enabled)` | 设置编辑模式 |
| `ComponentInfo_SetUnsavedChanges(hasChanges)` | 设置未保存更改状态 |

## 限制与要求

- **GlobalId要求**: 照片功能严格需要构件的GlobalId属性，不支持GUID回退
- **无缓存**: IFC属性读取使用基本实现，无性能优化或缓存
- **单一界面**: 所有功能集成在单一HTML界面中，无独立模态窗口
- **最小化错误处理**: 遵循最小错误处理原则，避免过度工程化

## 故障排除

### 常见问题

#### 插件面板打开但显示空白
**可能原因**: HTML资源未正确嵌入或JavaScript加载错误
**解决方案**:
1. 检查插件bundle完整性: `./check_bundle.sh`
2. 在Archicad中按F12打开浏览器控制台，查看JavaScript错误
3. 重新构建插件: `./build.sh clean && ./build.sh`

#### 照片功能不可用
**可能原因**: 构件没有GlobalId属性
**解决方案**:
1. 确保选择的构件是IFC兼容的 (具有GlobalId属性)
2. 检查项目文件夹的写入权限

#### IFC属性不显示
**可能原因**: 构件没有IFC属性或IFC API调用失败
**解决方案**:
1. 测试其他IFC兼容构件
2. 在浏览器控制台中检查错误日志

#### 版本号不正确
**可能原因**: 构建时版本号未正确更新
**解决方案**:
1. 清理构建: `./build.sh clean`
2. 重新构建: `./build.sh`

### 调试信息收集
1. 在Archicad中打开插件面板，按F12打开浏览器控制台
2. 运行以下命令检查ACAPI对象:
   ```javascript
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

## 许可证

本项目基于Archicad API开发工具包29.3100构建。

## 技术支持

如有问题，请参考:
- Archicad API文档
- 项目中的AGENTS.md文件（AI开发指南）
- TESTING.md (详细测试指南)
- JavaScript测试用例