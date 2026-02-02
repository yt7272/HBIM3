# 面板打开但无内容 — 原因分析与修复

## 现象

在部分环境（如其他 CLI/机器）中：打开插件后**能读出面板**（Palette 窗口出现），但**面板内没有任何内容**（浏览器控件为空白）。

## 架构要点

- 面板由 **DG::Palette** (ResId 32520) 提供，其中**只有一个 DG 控件**：**Browser** (ID 20)。
- 所有可见 UI 来自**嵌入的 HTML**：C++ 从资源 **DATA 100** 读取 HTML 字符串，再调用 `browser.LoadHTML(htmlContent)`。
- 资源在 **.grc** 里通过**文件名**引用，由 ArchiCAD 的 **ResConv** 在编译时把对应文件内容嵌入为 DATA 100。

## 根本原因分析

### 1. HTML 资源未正确嵌入（最可能）

**DATA 100 在运行时空或内容不对。**

- **.grc 当前写法**（`MergedPluginFix.grc`）：
  ```text
  'DATA' 100 "Merged Plugin Html" {
      "MinimalTest.html"
  }
  ```
- ResConv 的**资源搜索路径**为：`RFIX/Images` 和 `RFIX`，且相对于 CMake 传入的 **resourcesPath**（即插件根目录，如 `MergedPlugin/`）。
- 因此 ResConv 会到 **`<插件根>/RFIX/MinimalTest.html`** 找文件并嵌入其内容。

若在「其他 CLI」中出现下列情况，就会导致 **DATA 100 为空或不是合法 HTML**，从而面板空白：

- 构建时 **resourcesPath** 或当前工作目录与预期不同，ResConv 找不到 `RFIX/MinimalTest.html`。
- 只复制了部分工程（例如没有带 `RFIX/` 或没有 `MinimalTest.html`），资源编译仍执行但无法读取文件。
- 使用了不同的构建目录/out-of-tree 构建，而资源脚本里路径假设不成立。

**结果**：`LoadHtmlFromResource()` 得到空字符串或无效内容 → `browser.LoadHTML(...)` 无有效 HTML → 面板只有空白浏览器。

### 2. 使用的 HTML 与文档不一致

- **README / DG_to_HTML_Migration** 中说明的正式界面是 **Selection_Test.html**（双 Tab、工业深色主题、完整功能）。
- **.grc 里实际引用的是 MinimalTest.html**（仅用于桥接测试的小页面）。

若在别的环境有人把 .grc 改成了 `Selection_Test.html`，但该环境下 **RFIX/Selection_Test.html 不存在或路径不对**，同样会变成「资源缺失 → DATA 100 空 → 面板无内容」。  
所以「面板无内容」既可能来自 **MinimalTest.html 未嵌入**，也可能来自 **Selection_Test.html 未嵌入**（若改过 .grc）。

### 3. 其他可能（次要）

- **编码**：Mac 上 ResConv 使用 UTF-16 等，若 C++ 端按「单字节/UTF-8」解读可能乱码；一般更可能导致乱码而非完全空白，但仍需在排查时注意。
- **空指针**：若资源完全不存在，`RSLoadResource` 可能返回 nullptr；当前代码在 `BMhGetSize(data)` 之后才判断 `data != nullptr`，若 API 对 nullptr 不友好，理论上可能异常；但若面板能正常「打开」，多数情况是资源存在但内容为空。

## 建议修复与预防

### 1. 统一使用完整 UI 并确保资源存在

- 在 **MergedPluginFix.grc** 中，将 DATA 100 改为引用 **Selection_Test.html**（与 README 一致，避免混淆）：
  ```text
  'DATA' 100 "Merged Plugin Html" {
      "Selection_Test.html"
  }
  ```
- 确保在**任意构建环境**下，**RFIX/Selection_Test.html** 都存在于插件资源目录中，且构建时传入的 resourcesPath 包含该 RFIX 目录。

### 2. C++ 端对空资源做防护与提示

- 在 `LoadHtmlFromResource()` 中：
  - 先判断 `data == nullptr` 或 `BMhGetSize(data) == 0`，再使用 data。
  - 若资源为空或加载失败，返回一段**内联的 fallback HTML**（例如仅包含一行提示：“HTML 资源加载失败，请检查构建与 RFIX 中是否包含 Selection_Test.html。”），而不是空字符串。
- 这样即使在资源未嵌入的环境，用户也会看到明确错误信息，而不是完全空白。

### 3. 构建与部署检查

- 在 CI/文档中说明：**必须从包含 `RFIX/` 的插件根目录（或 CMake 规定的 resourcesPath）进行资源编译**，且 **RFIX** 下应包含：
  - **Selection_Test.html**（主界面），或
  - 若暂时仍用 MinimalTest，则 **MinimalTest.html** 必须存在。
- 可在构建后增加简单检查（例如用脚本确认 bundle/资源中 DATA 100 非空），避免把「无 HTML 资源」的包发到其他环境。

## 小结

| 现象           | 最可能原因                         | 建议措施                                           |
|----------------|------------------------------------|----------------------------------------------------|
| 面板有框无内容 | DATA 100 未嵌入或内容为空/错误     | 1) .grc 改为主 UI：Selection_Test.html<br>2) 保证 RFIX 与路径正确<br>3) C++ 空资源时显示错误 HTML |

按上述修改后，同一套代码在「其他 CLI」构建时，只要 RFIX 和 resourcesPath 正确，面板应能稳定显示内容；若资源仍缺失，至少会显示错误提示而非空白。
