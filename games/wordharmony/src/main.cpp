// Entry point for the Word Harmony port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing WHArchive.saf, res/ and WordHarmony.exe (read for the game's texts, never run)
//   -userdir=<dir>   writable folder for the settings and high scores (the emulated registry)
//   -modsdir=<dir>   folder containing mods (one sub-folder per mod; the fixes mod is games/wordharmony/mods/fixes)
//   -nomods          start without any mods
//   -splashhold      the original's two seconds of splash before the menu (the port shows the menu once it has loaded);
//                    for runs compared with the original
//   /s, /c, /p       the screen saver, its settings and its preview, as Windows passes them
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the
// framework's test hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "WordHarmonyApp.h"
#include "game/Game.h"
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

// The game's data is WHArchive.saf and res/; its texts are read from the original WordHarmony.exe in the same folder
static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "WHArchive.saf").empty();
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
	std::vector<std::string> candidates = {exe, exe + "/assets/wordharmony", exe + "/../assets/wordharmony",
		exe + "/../../assets/wordharmony", "assets/wordharmony"};
#ifdef _WIN32
	// The retail install (the installer's default folder)
	for (const char* env : {"ProgramFiles", "ProgramFiles(x86)"})
	{
		const char* pf = SDL_getenv(env);
		if (pf)
			candidates.push_back(std::string(pf) + "/PopCap Games/Word Harmony");
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
	// port (web): its page (web/shell.html, games/wordharmony/web/page.js) mounts its own IndexedDB-backed folder, so
	// Word Harmony's files never mix with the other games' on the same site
	if (gameDir.empty())
		gameDir = "/whpersist/game";
	if (userDir.empty())
		userDir = "/whpersist/userdata";
	if (modsDir.empty())
		modsDir = "/whpersist/mods";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty())
	{
		char* pref = SDL_GetPrefPath("byo-pop", "Word Harmony");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
	if (modsDir.empty())
		modsDir = ExeDir() + "/mods-wordharmony";
#endif

#ifndef __EMSCRIPTEN__
	// -importsaves[=<install folder>]: brings the saves, high scores and settings of the original game installed on this computer into the
	// user dir (platform/SaveImport.h), then exits without starting the game. Without it nothing changes.
	{
		int anImportResult = 0;
		if (Platform::RunSaveImportIfRequested(argc, argv, "wordharmony", gameDir, userDir, &anImportResult))
			return anImportResult;
	}
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir))
	{
		std::string msg = "Could not find the Word Harmony game files.\n\n"
			"Run with -gamedir=\"<your Word Harmony folder>\" or copy WHArchive.saf, res/ and the original "
			"WordHarmony.exe (the game's texts are read from it) into an assets/wordharmony folder next to the "
			"executable (games/wordharmony/tools/stage_wordharmony_assets.py).";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Word Harmony", msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}

	Platform::MakeDirs(userDir);
	// Web: saves written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(userDir);
	Platform::VFS::Get().SetBaseDir(gameDir);
	Platform::VFS::Get().SetUserDir(userDir);
	// The mods (the fixes mod, games/wordharmony/docs/IDENTICAL_PLAY.md): none without a mod folder, and the game plays as
	// the original
	Platform::MakeDirs(modsDir);
	Platform::ModManager::Get().SetModsDir(modsDir);
	Game::sSplashHold = HasFlag(argc, argv, "splashhold");
	if (!HasFlag(argc, argv, "nomods"))
		Platform::ModManager::Get().Refresh();
	SDL_Log("game: %s | user: %s | mods: %s", gameDir.c_str(), userDir.c_str(), modsDir.c_str());

	static WordHarmonyApp* anApp = new WordHarmonyApp();
	anApp->SetCmdLine(argc, argv);
	anApp->SetGameCommandLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	delete anApp;
#endif
	return 0;
}
