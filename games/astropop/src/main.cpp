// Entry point for the AstroPop Deluxe port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing the game's images/, data/, fonts/, sounds/, music/, properties/ and
//                    WinAP.exe or AstroPop.exe (read for the game's texts, never run)
//   -userdir=<dir>   writable folder for the players' data and registry.ini
//   -modsdir=<dir>   folder containing data mods (one sub-folder per mod)
//   -nomods          start without any mods
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the
// framework's test hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "AstroPopApp.h"
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

// The game's texts are read from the original's WinAP.exe (the Steam copy) or AstroPop.exe (the PopCap Game
// Collection's; GameText.h), so one of them is part of the game folder
static bool LooksLikeGameDir(const std::string& theDir)
{
	return (!Platform::VFS::FindCaseInsensitive(theDir, "winap.exe").empty() ||
			!Platform::VFS::FindCaseInsensitive(theDir, "astropop.exe").empty()) &&
		!Platform::VFS::FindCaseInsensitive(theDir, "properties/resources.xml").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "music/music.mo3").empty();
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
	std::vector<std::string> candidates = {exe, exe + "/assets/astropop", exe + "/../assets/astropop",
		exe + "/../../assets/astropop", "assets/astropop"};
#ifdef _WIN32
	// The retail install (the installer's default folder)
	for (const char* env : {"ProgramFiles", "ProgramFiles(x86)"})
	{
		const char* pf = SDL_getenv(env);
		if (pf)
			candidates.push_back(std::string(pf) + "/PopCap Games/AstroPop Deluxe");
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
	// port (web): its page (web/shell.html, games/astropop/web/page.js) mounts its own IndexedDB-backed folder, so
	// AstroPop's files never mix with the other games' on the same site
	if (gameDir.empty())
		gameDir = "/appersist/game";
	if (userDir.empty())
		userDir = "/appersist/userdata";
	if (modsDir.empty())
		modsDir = "/appersist/mods";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty())
	{
		char* pref = SDL_GetPrefPath("byo-pop", "AstroPop Deluxe");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
	if (modsDir.empty())
		modsDir = ExeDir() + "/mods-astropop";
#endif

#ifndef __EMSCRIPTEN__
	// -importsaves[=<install folder>]: brings the saves, high scores and settings of the original game installed on this computer into the
	// user dir (platform/SaveImport.h), then exits without starting the game. Without it nothing changes.
	{
		int anImportResult = 0;
		if (Platform::RunSaveImportIfRequested(argc, argv, "astropop", gameDir, userDir, &anImportResult))
			return anImportResult;
	}
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir))
	{
		std::string msg = "Could not find the AstroPop Deluxe game files.\n\n"
			"Run with -gamedir=\"<your AstroPop Deluxe folder>\" or copy the images, data, fonts, sounds, music "
			"and properties folders and the original WinAP.exe or AstroPop.exe (the game's texts are read from it) into an "
			"assets/astropop folder next to the executable.";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "AstroPop Deluxe", msg.c_str(), nullptr);
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
	static AstroPopApp* anApp = new AstroPopApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	delete anApp;
#endif
	return 0;
}
