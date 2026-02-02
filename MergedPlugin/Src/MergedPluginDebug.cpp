#include "MergedPluginDebug.hpp"
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <cstring>

#if defined (GS_MAC)
#include <stdlib.h>
#endif

static const char* GetLogPath ()
{
	static char path[1024] = { 0 };
	if (path[0] != '\0')
		return path;
#if defined (GS_MAC)
	const char* home = getenv ("HOME");
	if (home != nullptr) {
		snprintf (path, sizeof (path), "%s/Library/Logs/MergedPlugin.log", home);
	} else {
		snprintf (path, sizeof (path), "/tmp/MergedPlugin.log");
	}
#elif defined (GS_WIN)
	const char* userProfile = getenv ("USERPROFILE");
	if (userProfile != nullptr) {
		snprintf (path, sizeof (path), "%s\\MergedPlugin_debug.log", userProfile);
	} else {
		snprintf (path, sizeof (path), "C:\\MergedPlugin_debug.log");
	}
#else
	snprintf (path, sizeof (path), "/tmp/MergedPlugin.log");
#endif
	return path;
}

void MergedPluginDebugLog (const char* fmt, ...)
{
	va_list args;
	va_start (args, fmt);

	time_t now = time (nullptr);
	struct tm* t = localtime (&now);
	char timeBuf[64];
	if (t != nullptr)
		strftime (timeBuf, sizeof (timeBuf), "%Y-%m-%d %H:%M:%S", t);
	else
		snprintf (timeBuf, sizeof (timeBuf), "?");

	FILE* f = fopen (GetLogPath (), "a");
	if (f != nullptr) {
		fprintf (f, "[%s] ", timeBuf);
		vfprintf (f, fmt, args);
		fprintf (f, "\n");
		fflush (f);
		fclose (f);
	}

	va_end (args);
}

const char* MergedPluginDebugLogPath (void)
{
	return GetLogPath ();
}
