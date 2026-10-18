// Steam's install folder, read from the registry. Kept apart from main.cpp because <windows.h> clashes
// with the Win32 stand-in typedefs of the portable framework (SexyAppFramework/PortCompat.h).

#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

std::string GetSteamInstallPath()
{
#ifdef _WIN32
	char path[MAX_PATH];
	DWORD size = sizeof(path);
	if (RegGetValueA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath", RRF_RT_REG_SZ, nullptr, path, &size) == ERROR_SUCCESS)
		return path;
#endif
	return "";
}
