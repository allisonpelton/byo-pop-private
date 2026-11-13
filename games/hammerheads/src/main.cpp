// Entry point for the Hammer Heads Deluxe port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder with the game's boards/, gnomedata/, images/, sounds/, fonts/ and properties/: the staged copy
//                    that games/hammerheads/tools/stage_hammerheads_assets.py makes (assets/hammerheads)
//   -userdir=<dir>   writable folder for the player roster and scores (userdata) and registry.ini
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the framework's test
// hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "Application.h"
#include "platform/Relaunch.h"
#include "platform/VFS.h"
#include "platform/WebPersist.h"

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

static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "properties/resources.xml").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "boards/classic.csv").empty();
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

// next to the exe, or byo-pop's staged copy (games/hammerheads/tools/stage_hammerheads_assets.py) from build/native
static std::string FindGameDir()
{
	std::string exe = ExeDir();
	std::vector<std::string> candidates = {exe, exe + "/assets", exe + "/assets/hammerheads", exe + "/../assets/hammerheads",
		exe + "/../../assets/hammerheads", "assets/hammerheads"};
	for (const std::string& c : candidates)
		if (LooksLikeGameDir(c))
			return c;
	return "";
}
#endif

// port: the original's WinMain makes the app, calls Init, Start and Shutdown on it; the port reads its arguments first
int main(int argc, char** argv)
{
	std::string gameDir = ArgValue(argc, argv, "gamedir");
	std::string userDir = ArgValue(argc, argv, "userdir");

#ifdef __EMSCRIPTEN__
	if (gameDir.empty())
		gameDir = "/hammerheadspersist/game";
	if (userDir.empty())
		userDir = "/hammerheadspersist/userdata";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty()) {
		char* pref = SDL_GetPrefPath("byo-pop", "Hammer Heads Deluxe");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir)) {
		std::string msg = "Could not find the Hammer Heads Deluxe game files.\n\n"
			"Stage them with games/hammerheads/tools/stage_hammerheads_assets.py (it makes assets/hammerheads) "
			"or run with -gamedir=\"<the staged folder>\".";
		// port: a scripted test run shows no box (on its hidden desktop nobody could close it)
		if (ArgValue(argc, argv, "script").empty())
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Hammer Heads Deluxe", msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}

	Platform::MakeDirs(userDir);
	// Web: saves written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(userDir);
	Platform::VFS::Get().SetBaseDir(gameDir);
	Platform::VFS::Get().SetUserDir(userDir);
	SDL_Log("game: %s | user: %s", gameDir.c_str(), userDir.c_str());

	// The original keeps the app as a global object and calls Init, Start and Shutdown on it.
	static Application* anApp = new Application();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	std::vector<std::string> aRestartArgs;
	bool restart = anApp->TakeRelaunch(&aRestartArgs);
	delete anApp;
	if (restart)
		Platform::Relaunch(aRestartArgs, "Hammer Heads Deluxe");
#endif
	return 0;
}
