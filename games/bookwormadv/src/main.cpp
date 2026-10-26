// Entry point for the Bookworm Adventures Deluxe port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder with the game's images/, scripts/, properties/, data/, sounds/ and
//                    BookwormAdventures.exe (read for a few texts, never run): the staged copy that
//                    games/bookwormadv/tools/stage_bookwormadv_assets.py makes (its JPEG 2000 images as PNG)
//   -userdir=<dir>   writable folder for the profiles and registry.ini
//   -modsdir=<dir>   folder containing data mods (one sub-folder per mod)
//   -nomods          start without any mods
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the
// framework's test hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "GameApp.h"
#include "platform/ModManager.h"
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
	for (int i = 1; i < argc; i++) {
		const char* a = argv[i];
		while (*a == '-')
			a++;
		if (!strncmp(a, theKey, n) && a[n] == '=')
			return a + n + 1;
	}
	return "";
}

static bool HasFlag(int argc, char** argv, const char* theFlag)
{
	for (int i = 1; i < argc; i++) {
		const char* a = argv[i];
		while (*a == '-')
			a++;
		if (!strcmp(a, theFlag))
			return true;
	}
	return false;
}

// The game's few texts in C++ are read from the original's BookwormAdventures.exe (GameText.h), so it is part
// of the game folder
static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "bookwormadventures.exe").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "properties/resources.xml").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "scripts/main.luc").empty();
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

// port: not the retail install itself: its images are JPEG 2000, which the port reads only as the PNG copies
// that staging makes
static std::string FindGameDir()
{
	std::string exe = ExeDir();
	std::vector<std::string> candidates = {exe, exe + "/assets/bookwormadv", exe + "/../assets/bookwormadv",
		exe + "/../../assets/bookwormadv", "assets/bookwormadv"};
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
	// port (web): its page mounts its own IndexedDB-backed folder, so the game's files never mix with the
	// other games' on the same site
	if (gameDir.empty())
		gameDir = "/bwapersist/game";
	if (userDir.empty())
		userDir = "/bwapersist/userdata";
	if (modsDir.empty())
		modsDir = "/bwapersist/mods";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty()) {
		char* pref = SDL_GetPrefPath("wasm-bejeweled", "Bookworm Adventures Deluxe");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
	if (modsDir.empty())
		modsDir = ExeDir() + "/mods-bookwormadv";
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir)) {
		std::string msg = "Could not find the Bookworm Adventures Deluxe game files.\n\n"
			"Stage them with games/bookwormadv/tools/stage_bookwormadv_assets.py (it makes assets/bookwormadv) "
			"or run with -gamedir=\"<the staged folder>\".";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bookworm Adventures Deluxe", msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}

	Platform::MakeDirs(userDir);
	// Web: saves written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(userDir);
	Platform::MakeDirs(modsDir);
	Platform::VFS::Get().SetBaseDir(gameDir);
	Platform::VFS::Get().SetUserDir(userDir);
	Platform::ModManager::Get().SetModsDir(modsDir);
	if (!HasFlag(argc, argv, "nomods"))
		Platform::ModManager::Get().Refresh();
	SDL_Log("game: %s | user: %s | mods: %s", gameDir.c_str(), userDir.c_str(), modsDir.c_str());

	// The original keeps the app as a global object and calls Init, Start and Shutdown on it.
	static GameApp* anApp = new GameApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	delete anApp;
#endif
	return 0;
}
