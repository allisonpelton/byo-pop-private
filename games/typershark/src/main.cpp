// Entry point for the Typer Shark Deluxe port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing the game's images/, data/, sounds/, music/, properties/ and
//                    WinTS.exe (read for the game's texts, never run)
//   -userdir=<dir>   writable folder for the players' data and registry.ini
//   -modsdir=<dir>   folder containing data mods (one sub-folder per mod)
//   -nomods          start without any mods
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the
// framework's test hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "WinWSApp.h"
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

static bool HasFlag(int argc, char** argv, const char* theFlag)
{
	for (int i = 1; i < argc; i++)
	{
		const char* a = argv[i];
		while (*a == '-')
			a++;
		if (!strcmp(a, theFlag))
			return true;
	}
	return false;
}

// The game's texts are read from the original's WinTS.exe (GameText.h), so it is part of the game folder
static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "wints.exe").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "data/twords.txt").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "images/titlescreen.jpg").empty();
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
	std::vector<std::string> candidates = {exe, exe + "/assets/typershark", exe + "/../assets/typershark",
		exe + "/../../assets/typershark", "assets/typershark"};
#ifdef _WIN32
	// The retail install (the installer's default folder)
	for (const char* env : {"ProgramFiles", "ProgramFiles(x86)"})
	{
		const char* pf = SDL_getenv(env);
		if (pf)
			candidates.push_back(std::string(pf) + "/PopCap Games/Typer Shark Deluxe");
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
	// port (web): its page (web/shell.html, games/typershark/web/page.js) mounts its own IndexedDB-backed folder, so
	// Typer Shark's files never mix with the other games' on the same site
	if (gameDir.empty())
		gameDir = "/tspersist/game";
	if (userDir.empty())
		userDir = "/tspersist/userdata";
	if (modsDir.empty())
		modsDir = "/tspersist/mods";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty())
	{
		char* pref = SDL_GetPrefPath("byo-pop", "Typer Shark Deluxe");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
	if (modsDir.empty())
		modsDir = ExeDir() + "/mods-typershark";
#endif

#ifndef __EMSCRIPTEN__
	// -importsaves[=<install folder>]: brings the saves, high scores and settings of the original game installed on this computer into the
	// user dir (platform/SaveImport.h), then exits without starting the game. Without it nothing changes.
	{
		int anImportResult = 0;
		if (Platform::RunSaveImportIfRequested(argc, argv, "typershark", gameDir, userDir, &anImportResult))
			return anImportResult;
	}
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir))
	{
		std::string msg = "Could not find the Typer Shark Deluxe game files.\n\n"
			"Run with -gamedir=\"<your Typer Shark Deluxe folder>\" or copy the images, data, sounds, music "
			"and properties folders and the original WinTS.exe (the game's texts are read from it) into an "
			"assets/typershark folder next to the executable.";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Typer Shark Deluxe", msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}

	Platform::MakeDirs(userDir);
	// Web: data written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(userDir);
	Platform::MakeDirs(modsDir);
	Platform::VFS::Get().SetBaseDir(gameDir);
	Platform::VFS::Get().SetUserDir(userDir);
	Platform::ModManager::Get().SetModsDir(modsDir);
	if (!HasFlag(argc, argv, "nomods"))
		Platform::ModManager::Get().Refresh();
	SDL_Log("game: %s | user: %s | mods: %s", gameDir.c_str(), userDir.c_str(), modsDir.c_str());

	// The original keeps the app as a global object and calls Init, Start and Shutdown on it.
	static WinWSApp* anApp = new WinWSApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	delete anApp;
#endif
	return 0;
}
