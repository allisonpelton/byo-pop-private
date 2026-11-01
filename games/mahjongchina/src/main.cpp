// Entry point for the Mahjong Escape: Ancient China port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing Resource.dll and MahjongEscapeAC.exe (read for their resources, never run)
//   -userdir=<dir>   writable folder for the players, their settings and saved games, and the emulated registry
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the
// framework's test hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "MahjongApp.h"
#include "platform/SaveImport.h"
#include "platform/VFS.h"
#include "platform/WebPersist.h"

#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace Sexy;

static std::string ArgValue(int argc, char** argv, const char* theKey)
{
	size_t n = strlen(theKey);
	for (int i = 1; i < argc; i++)
	{
		const char* a = argv[i];
		while (*a == '-')
			a++;
		if (!strncmp(a, theKey, n) && a[n] == '=')
			return a + n + 1;
	}
	return "";
}

// The game's data is Resource.dll's; the exe has the rest of its resources and texts, so both are in the game folder
static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "mahjongescapeac.exe").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "resource.dll").empty();
}

#ifndef __EMSCRIPTEN__
static std::string ExeDir()
{
	char* base = SDL_GetBasePath();
	std::string dir = base ? base : "./";
	SDL_free(base);
	while (!dir.empty() && (dir.back() == '/' || dir.back() == '\\'))
		dir.pop_back();
	return dir;
}

static std::string FindGameDir()
{
	std::string exe = ExeDir();
	std::vector<std::string> candidates = {exe, exe + "/assets/mahjongchina", exe + "/../assets/mahjongchina",
		exe + "/../../assets/mahjongchina", "assets/mahjongchina"};
#ifdef _WIN32
	// The install of the PopCap Game Collection (read only)
	for (const char* env : {"ProgramFiles", "ProgramFiles(x86)"})
	{
		const char* pf = SDL_getenv(env);
		if (pf)
			candidates.push_back(std::string(pf) + "/Popcap Game Collection/Mahjong Escape Ancient China");
	}
#endif
	for (const std::string& c : candidates)
		if (LooksLikeGameDir(c))
			return c;
	return "";
}
#endif

int main(int argc, char** argv)
{
	std::string gameDir = ArgValue(argc, argv, "gamedir");
	std::string userDir = ArgValue(argc, argv, "userdir");

#ifdef __EMSCRIPTEN__
	// port (web): its page (web/shell.html) mounts its own IndexedDB-backed folder, so the game's files never mix
	// with the other games' on the same site
	if (gameDir.empty())
		gameDir = "/mjpersist/game";
	if (userDir.empty())
		userDir = "/mjpersist/userdata";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty())
	{
		char* pref = SDL_GetPrefPath("byo-pop", "Mahjong Escape Ancient China");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
#endif

#ifndef __EMSCRIPTEN__
	// -importsaves[=<install folder>]: brings the saves, high scores and settings of the original game installed on this computer into the
	// user dir (platform/SaveImport.h), then exits without starting the game. Without it nothing changes.
	{
		int anImportResult = 0;
		if (Platform::RunSaveImportIfRequested(argc, argv, "mahjongchina", gameDir, userDir, &anImportResult))
			return anImportResult;
	}
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir))
	{
		std::string msg = "Could not find the Mahjong Escape: Ancient China game files.\n\n"
			"Run with -gamedir=\"<your Mahjong Escape: Ancient China folder>\" or copy Resource.dll and the original "
			"MahjongEscapeAC.exe into an assets/mahjongchina folder next to the executable.";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Mahjong Escape: Ancient China", msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}

	Platform::MakeDirs(userDir);
	// Web: saves written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(userDir);
	Platform::VFS::Get().SetBaseDir(gameDir);
	Platform::VFS::Get().SetUserDir(userDir);
	SDL_Log("game: %s | user: %s", gameDir.c_str(), userDir.c_str());

	static MahjongApp* anApp = new MahjongApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	delete anApp;
#endif
	return 0;
}
