// port: the original opened its preferences and savegame.dat in its own folder (the working directory); the port
// keeps them in the user dir (main.cpp's -userdir, the web page's IndexedDB folder). See Game.h.

#include "Game.h"

#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace tiptop
{

FILE* UserFopen(const char* theName, const char* theMode)
{
	std::string aPath = Platform::VFS::Get().GetUserDir() + "/" + theName;
#ifdef _WIN32
	// the user dir is UTF-8 (SDL_GetPrefPath)
	int n = MultiByteToWideChar(CP_UTF8, 0, aPath.c_str(), -1, nullptr, 0);
	if (n > 0)
	{
		std::wstring aWide((size_t)n, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, aPath.c_str(), -1, &aWide[0], n);
		std::wstring aMode(theMode, theMode + strlen(theMode));
		return _wfopen(aWide.c_str(), aMode.c_str());
	}
#endif
	return fopen(aPath.c_str(), theMode);
}

int UserRemove(const char* theName)
{
	return Platform::RemoveUserFile(theName) ? 0 : -1;
}

void UserFileWritten()
{
	Platform::UserDataChanged();
}

} // namespace tiptop
