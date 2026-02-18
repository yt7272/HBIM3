# HBIM3

ArchiCAD 29 插件：为构件添加 HBIM 属性与照片信息。

## HBIMComponentEntry 功能

- **HBIM 属性**：构件编号、构件说明的自定义属性及添加/编辑流程
- **HBIM 构件图片**：多图选择、预览、上一张/下一张、删除
- **系统预览**：点击图片用系统默认程序打开大图
- **IFC 显示**：构件类型、GlobalId

## 构建

```bash
cd REREBuild && ./build.sh
```

清理构建：`./build.sh clean`

## 项目结构

```
HBIM3/
├── API.Development.Kit.MAC.29.3100/   # ArchiCAD API 开发包
├── REREBuild/                        # 主插件源码（HBIMComponentEntry）
├── AGENTS.md                         # AI 开发指南
└── README.md
```

## 环境

- ArchiCAD API Development Kit 29.3100
- macOS / Windows，C++20
