// Entry point for the Bejeweled 2 Deluxe port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder with the game's images/, data/, edata/, sounds/, music/, properties/ and puzzles/: the
//                    staged copy that games/bejeweled2/tools/stage_assets.py makes (assets/bejeweled2)
//   -userdir=<dir>   writable folder for the players' profiles, the saved game and registry.ini
//   -modsdir=<dir>   folder containing data mods (one sub-folder per mod); native default: mods-bejeweled2 next to the exe
//   -nomods          start without any mods
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the framework's test
// hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "WinDMApp.h"
#include "platform/ModManager.h"
#include "platform/Relaunch.h"
#include "platform/VFS.h"
#include "platform/WebPersist.h"
#include "platform/openmpt/OpenMPTPlayer.h"

#include <SDL.h>
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

static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "properties/resources.xml").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "properties/default.xml").empty();
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

// next to the exe, or byo-pop's staged copy (games/bejeweled2/tools/stage_assets.py) from build/native
static std::string FindGameDir()
{
	std::string exe = ExeDir();
	std::vector<std::string> candidates = {exe, exe + "/assets", exe + "/assets/bejeweled2", exe + "/../assets/bejeweled2",
		exe + "/../../assets/bejeweled2", "assets/bejeweled2"};
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
	if (gameDir.empty())
		gameDir = "/bej2persist/game";
	if (userDir.empty())
		userDir = "/bej2persist/userdata";
	if (modsDir.empty())
		modsDir = "/bej2persist/mods";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty()) {
		char* pref = SDL_GetPrefPath("byo-pop", "Bejeweled 2 Deluxe");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
	if (modsDir.empty())
		modsDir = ExeDir() + "/mods-bejeweled2";
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir)) {
		std::string msg = "Could not find the Bejeweled 2 Deluxe game files.\n\n"
			"Stage them with games/bejeweled2/tools/stage_assets.py (it makes assets/bejeweled2) "
			"or run with -gamedir=\"<the staged folder>\".";
		// port: a scripted test run shows no box (on its hidden desktop nobody could close it)
		if (ArgValue(argc, argv, "script").empty())
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bejeweled 2 Deluxe", msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}

	Platform::MakeDirs(userDir);
	// Web: saves written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(userDir);
	Platform::VFS::Get().SetBaseDir(gameDir);
	Platform::VFS::Get().SetUserDir(userDir);
	Platform::MakeDirs(modsDir);
	Platform::ModManager::Get().SetModsDir(modsDir);
	if (!HasFlag(argc, argv, "nomods"))
		Platform::ModManager::Get().Refresh();
	SDL_Log("game: %s | user: %s | mods: %s", gameDir.c_str(), userDir.c_str(), modsDir.c_str());

	// The music is one MO3 module (music/BeyondNetwork.mo3), which libopenmpt plays
	Platform::RegisterOpenMPTModules();

	// The original keeps the app as a global object and calls Init, Start and Shutdown on it.
	static WinDMApp* anApp = new WinDMApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	std::vector<std::string> aRestartArgs;
	bool restart = anApp->TakeRelaunch(&aRestartArgs);
	delete anApp;
	if (restart)
		Platform::Relaunch(aRestartArgs, "Bejeweled 2 Deluxe");
#endif
	return 0;
}
