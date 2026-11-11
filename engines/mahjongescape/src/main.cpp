// Entry point for the Mahjong Escape port (native and Emscripten): Ancient Japan and Ancient China.
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing Resource.dll and the game's own exe (read for their resources, never run)
//   -userdir=<dir>   writable folder for the players, their settings and saved games, and the emulated registry
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the
// framework's test hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "Edition.h"
#include "MahjongApp.h"
#include "platform/ModManager.h"
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

static bool HasFlag(int argc, char** argv, const char* theKey)
{
	for (int i = 1; i < argc; i++)
	{
		const char* a = argv[i];
		while (*a == '-')
			a++;
		if (!strcmp(a, theKey))
			return true;
	}
	return false;
}

// The game's data is Resource.dll's; the exe has the rest of its resources and texts, so both are in the game folder
static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, EDITION_EXE_FILE).empty() &&
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
	std::vector<std::string> candidates = {exe, exe + "/assets/" EDITION_GAME_FOLDER,
		exe + "/../assets/" EDITION_GAME_FOLDER, exe + "/../../assets/" EDITION_GAME_FOLDER,
		"assets/" EDITION_GAME_FOLDER};
#ifdef _WIN32
	// The install of the PopCap Game Collection (read only)
	for (const char* env : {"ProgramFiles", "ProgramFiles(x86)"})
	{
		const char* pf = SDL_getenv(env);
		if (pf)
			candidates.push_back(std::string(pf) + "/Popcap Game Collection/" EDITION_INSTALL_FOLDER);
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
	std::string modsDir = ArgValue(argc, argv, "modsdir");

#ifdef __EMSCRIPTEN__
	// port (web): its page (web/shell.html) mounts its own IndexedDB-backed folder, so the game's files never mix
	// with the other games' on the same site
	if (gameDir.empty())
		gameDir = "/" EDITION_WEB_FOLDER "/game";
	if (userDir.empty())
		userDir = "/" EDITION_WEB_FOLDER "/userdata";
	if (modsDir.empty())
		modsDir = "/" EDITION_WEB_FOLDER "/mods";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty())
	{
		char* pref = SDL_GetPrefPath("byo-pop", EDITION_INSTALL_FOLDER);
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
	if (modsDir.empty())
		modsDir = ExeDir() + "/mods-" EDITION_GAME_FOLDER;
#endif

#ifndef __EMSCRIPTEN__
	// -importsaves[=<install folder>]: brings the saves, high scores and settings of the original game installed on
	// this computer into the user dir (platform/SaveImport.h), then exits without starting the game. Without it nothing
	// changes.
	{
		int anImportResult = 0;
		if (Platform::RunSaveImportIfRequested(argc, argv, EDITION_GAME_FOLDER, gameDir, userDir, &anImportResult))
			return anImportResult;
	}
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir))
	{
		std::string msg = "Could not find the " EDITION_GAME_TITLE " game files.\n\n"
			"Run with -gamedir=\"<your " EDITION_GAME_TITLE " folder>\" or copy Resource.dll and the original "
			EDITION_EXE_FILE " into an assets/" EDITION_GAME_FOLDER " folder next to the executable.";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, EDITION_GAME_TITLE, msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}

	Platform::MakeDirs(userDir);
	// Web: saves written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(userDir);
	Platform::VFS::Get().SetBaseDir(gameDir);
	Platform::VFS::Get().SetUserDir(userDir);
	// The mods (the fixes mod, the game's mods/fixes): none without a mod folder, and the game plays as the original
	Platform::MakeDirs(modsDir);
	Platform::ModManager::Get().SetModsDir(modsDir);
	if (!HasFlag(argc, argv, "nomods"))
		Platform::ModManager::Get().Refresh();
	SDL_Log("game: %s | user: %s | mods: %s", gameDir.c_str(), userDir.c_str(), modsDir.c_str());

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
