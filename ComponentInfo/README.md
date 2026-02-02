# ComponentInfo — ArchiCAD 构件属性插件

为构件添加自定义属性：**构件编号**、**照片说明**、**照片路径**（多张）。照片拷贝到 `{项目名}_构件照片`，属性中仅存相对路径，不增加 PLN 体积。

## 目录结构

```
ComponentInfo/
├── CMakeLists.txt      # 构建配置
├── RFIX/
│   └── ComponentInfoFix.grc
├── RFIX.mac/
│   └── Info.plist
├── Src/
│   ├── APICommon.c, APICommon.h, APIEnvir.h
│   ├── ComponentInfo.cpp, ComponentInfo.hpp
│   ├── ComponentInfoDialog.cpp, ComponentInfoDialog.hpp
│   ├── ComponentInfoMain.cpp
│   ├── PropertyUtils.cpp, PropertyUtils.hpp
│   └── ...
└── README.md
```

## 依赖

- **ArchiCAD API Development Kit**：编译与资源需引用 `API.Development.Kit.MAC.29.3100`。
- `CMakeLists.txt` 中默认：

  `AC_API_DEVKIT_DIR = ${CMAKE_CURRENT_LIST_DIR}/../API.Development.Kit.MAC.29.3100`

  即要求 `ComponentInfo` 与 `API.Development.Kit.MAC.29.3100` 处于同一父目录下（例如都在 `HBIM2/` 下）。  
  若你的 API  Kit 路径不同，可配置时覆盖：

  ```bash
  cmake -DAC_API_DEVKIT_DIR=/绝对路径/API.Development.Kit.MAC.29.3100 ..
  ```

- 资源中的 `#include "MDIDs_APICD.h"` 以及 `MDID_APICD_ComponentInfo` 需在 API Kit 的 `Support/Inc/MDIDs_APICD.h` 中已定义（若从 API 示例拷贝过，通常已具备）。

## 编译

```bash
cd ComponentInfo
mkdir -p build && cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

产物：`build/Release/ComponentInfo.bundle`。

## 安装与使用

1. 将 `ComponentInfo.bundle` 拷贝到 ArchiCAD 的 Add-ons 目录。
2. 在 ArchiCAD 中：**Add-Ons → Component Info → 为选中构件填写扩展信息（编号 / 照片说明 / 照片）**。
3. 项目需已保存；照片会拷贝到项目同目录的 `{项目名}_构件照片`。
