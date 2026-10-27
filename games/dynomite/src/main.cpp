// Entry point for the Dynomite Deluxe port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing the game's images/, data/, sounds/, music/, stomped/, DEFAULT.EGG and
//                    properties/ (default.xml holds the game's texts)
//   -userdir=<dir>   writable folder for the saved games, high scores and registry.ini
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the
// framework's test hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "FossilizedApp.h"
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

static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "properties/default.xml").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "music/amazonas.it").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "images/title.jpg").empty();
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
	std::vector<std::string> candidates = {exe, exe + "/assets/dynomite", exe + "/../assets/dynomite",
		exe + "/../../assets/dynomite", "assets/dynomite"};
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
	// port (web): its page mounts its own IndexedDB-backed folder, so Dynomite's files never mix with the other
	// games' on the same site
	if (gameDir.empty())
		gameDir = "/dypersist/game";
	if (userDir.empty())
		userDir = "/dypersist/userdata";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty())
	{
		char* pref = SDL_GetPrefPath("wasm-bejeweled", "Dynomite Deluxe");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir))
	{
		std::string msg = "Could not find the Dynomite Deluxe game files.\n\n"
			"Run with -gamedir=\"<your Dynomite Deluxe folder>\" or copy the images, data, sounds, music, stomped "
			"and properties folders and DEFAULT.EGG into an assets/dynomite folder next to the executable.";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Dynomite Deluxe", msg.c_str(), nullptr);
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
	static FossilizedApp* anApp = new FossilizedApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	delete anApp;
#endif
	return 0;
}
