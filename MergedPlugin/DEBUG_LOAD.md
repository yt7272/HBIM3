# Merged Plugin 加载调试日志

当 ArchiCAD 无法查看插件加载过程时，插件会将加载与面板初始化过程写入**文件日志**，便于排查“面板打开但无内容”等问题。

## 日志文件位置

| 系统   | 路径 |
|--------|------|
| macOS  | `~/Library/Logs/MergedPlugin.log` |
| Windows | `%USERPROFILE%\MergedPlugin_debug.log` |

即：**用户主目录** → macOS 下 `Library/Logs/MergedPlugin.log`，Windows 下 `MergedPlugin_debug.log`。

## 如何查看

1. **先启动 ArchiCAD 并打开/关闭一次插件面板**（测试 → Merged Plugin），以便产生日志。
2. **打开日志文件**：
   - macOS 终端：`open ~/Library/Logs/MergedPlugin.log` 或 `tail -f ~/Library/Logs/MergedPlugin.log`
   - 访达：前往文件夹 `~/Library/Logs/`，打开 `MergedPlugin.log`。
3. **实时查看**（终端）：`tail -f ~/Library/Logs/MergedPlugin.log`，再在 ArchiCAD 里打开/关闭面板，观察新输出。

## 日志标签含义

| 标签   | 含义 |
|--------|------|
| `[加载]` | 插件生命周期：CheckEnvironment、RegisterInterface、Initialize、FreeData |
| `[菜单]` | 用户点击菜单“显示/隐藏面板”及 CreateInstance/Show |
| `[回调]` | 面板回调：PaletteControlCallBack、APIPalMsg_OpenPalette 等 |
| `[面板]` | 面板构造函数：Palette 创建、BeginEventProcessing、LoadHTML、构造函数完成 |
| `[HTML]` | HTML 资源：DATA 100 是否找到、handleSize、内容前若干字节预览 |
| `[JS桥]` | JavaScript 桥：InitializeJavaScriptBridge、RegisterAsynchJSObject |

## 排查“面板无内容”时看什么

1. **是否有 `[加载] CheckEnvironment 开始`**  
   - 没有：插件未被加载或未执行到 CheckEnvironment。
2. **`[HTML] DATA 100`**  
   - `资源未找到` 或 `handleSize=0`：资源未嵌入，需检查构建与 RFIX/Selection_Test.html。
   - `size=... 前N字节: <!DO` 或类似：说明 HTML 已嵌入，问题可能在浏览器控件或 JS。
3. **是否有 `[面板] browser.LoadHTML 已调用`**  
   - 有且前面 HTML size>0：说明 C++ 已把 HTML 交给浏览器，若仍空白，可能是 CEF/浏览器或安全策略。
4. **是否有 `[JS桥] RegisterAsynchJSObject 完成`**  
   - 有：桥接已注册；若页面仍空白，可再检查 HTML 内容预览是否像完整页面开头。

每次启动 ArchiCAD 并打开一次面板，日志会**追加**到同一文件，不会覆盖之前记录。
