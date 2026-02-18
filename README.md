# HBIM3

ArchiCAD 29 插件项目：为构件添加照片与 HBIM 属性信息的系列插件。

## 子项目

| 插件 | 路径 | 功能 |
|------|------|------|
| **ComponentInfo** | `ComponentInfo/` | 构件编号、照片说明、照片路径等扩展属性 |
| **IFCPropertyReader** | `IFCPropertyReader/` | IFC 属性读取与 HTML 面板，LRU 缓存 |
| **MergedPlugin** | `ReBuild/NewPlugin/` | ComponentInfo + IFCPropertyReader 统一 HTML 面板 |
| **HBIMComponentEntry** | `REREBuild/HBIMComponentEntry/` | HBIM 构件编号/说明、HBIM 构件图片（添加/编辑/删除、系统预览） |

## HBIMComponentEntry 主要功能

- **HBIM 属性**：构件编号、构件说明的自定义属性及添加/编辑流程
- **HBIM 构件图片**：多图选择、预览、上一张/下一张、删除
- **系统预览**：点击图片用系统默认程序打开大图
- **IFC 显示**：构件类型、GlobalId

## 构建

详见 [AGENTS.md](AGENTS.md)。

```bash
# HBIMComponentEntry（推荐，功能最全）
cd REREBuild/HBIMComponentEntry && ./build.sh

# MergedPlugin（ComponentInfo + IFC）
cd ReBuild/NewPlugin && ./build.sh

# ComponentInfo
cd ComponentInfo && ./build.sh

# IFCPropertyReader
cd IFCPropertyReader && ./build.sh
```

## 环境

- ArchiCAD API Development Kit 29.3100
- macOS / Windows，C++20
