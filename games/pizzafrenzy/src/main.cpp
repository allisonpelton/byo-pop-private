// Entry point for the Pizza Frenzy port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing PizzaFrenzy.saf, splash/, properties/ and PizzaFrenzy.exe (read for the game's
//                    texts, never run)
//   -userdir=<dir>   writable folder for the settings, the players and their pizzas (the emulated registry and files)
//   -modsdir=<dir>   folder containing mods (one sub-folder per mod)
//   -nomods          start without any mods
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the framework's test
// hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "PizzaFrenzyApp.h"
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

// The game's data is PizzaFrenzy.saf with the loose splash/ and properties/; its texts are read from the original
// PizzaFrenzy.exe in the same folder
static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "PizzaFrenzy.saf").empty();
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
	std::vector<std::string> candidates = {exe, exe + "/assets/pizzafrenzy", exe + "/../assets/pizzafrenzy",
		exe + "/../../assets/pizzafrenzy", "assets/pizzafrenzy"};
#ifdef _WIN32
	// The retail install (the installer's default folder)
	for (const char* env : {"ProgramFiles", "ProgramFiles(x86)"})
	{
		const char* pf = SDL_getenv(env);
		if (pf)
			candidates.push_back(std::string(pf) + "/PopCap Games/Pizza Frenzy");
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
	// port (web): its page mounts its own IndexedDB-backed folder, so Pizza Frenzy's files never mix with the other
	// games' on the same site
	if (gameDir.empty())
		gameDir = "/pfpersist/game";
	if (userDir.empty())
		userDir = "/pfpersist/userdata";
	if (modsDir.empty())
		modsDir = "/pfpersist/mods";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty())
	{
		char* pref = SDL_GetPrefPath("byo-pop", "Pizza Frenzy");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
	if (modsDir.empty())
		modsDir = ExeDir() + "/mods-pizzafrenzy";
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir))
	{
		std::string msg = "Could not find the Pizza Frenzy game files.\n\n"
			"Run with -gamedir=\"<your Pizza Frenzy folder>\" or copy PizzaFrenzy.saf, splash/, properties/ and the "
			"original PizzaFrenzy.exe (the game's texts are read from it) into an assets/pizzafrenzy folder next to the "
			"executable (games/pizzafrenzy/tools/stage_pizzafrenzy_assets.py).";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Pizza Frenzy", msg.c_str(), nullptr);
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

	static PizzaFrenzyApp* anApp = new PizzaFrenzyApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	delete anApp;
#endif
	return 0;
}
