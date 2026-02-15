# HBIM3

ArchiCAD 29 插件项目，包含构件扩展信息与 IFC 属性查看。

## 子项目

- **ComponentInfo** — 构件编号、照片说明、照片路径等扩展属性
- **IFCPropertyReader** — IFC 属性读取与 HTML 面板
- **MergedPlugin** — 合并上述功能的统一 HTML 面板插件
- **HBIMComponentEntry** — HBIM属性信息管理插件，添加HBIM构件编号和说明

## 构建

详见 [AGENTS.md](AGENTS.md)。简要：

```bash
# MergedPlugin
cd MergedPlugin && ./build.sh

# ComponentInfo
cd ComponentInfo && ./build.sh

# IFCPropertyReader
cd IFCPropertyReader && ./build.sh

# HBIMComponentEntry
cd REREBuild/HBIMComponentEntry && ./build.sh
```

## 环境

- ArchiCAD API Development Kit 29.3100（见 `API.Development.Kit.MAC.29.3100/`）
- macOS / Windows，C++20
