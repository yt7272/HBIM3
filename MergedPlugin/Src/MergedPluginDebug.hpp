#if !defined (MERGEDPLUGINDEBUG_HPP)
#define MERGEDPLUGINDEBUG_HPP

/** 将一条调试信息追加到日志文件，便于在无法查看 ArchiCAD 加载过程时排查问题。
 *  日志路径: macOS ~/Library/Logs/MergedPlugin.log
 *            Windows %USERPROFILE%\MergedPlugin_debug.log
 *  用法: MergedPluginDebugLog("message"); 或 MergedPluginDebugLog("format %d", value);
 */
void MergedPluginDebugLog (const char* fmt, ...);

/** 返回当前使用的日志文件路径（用于文档/提示）。 */
const char* MergedPluginDebugLogPath (void);

#endif
