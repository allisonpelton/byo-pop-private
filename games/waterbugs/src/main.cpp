// Entry point for the Water Bugs port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing WaterBugs.exe (read for the game's texts, never run), water_art.bbk, water_art8.bbk,
//                    wanims/ and audio/
//   -userdir=<dir>   writable folder for cp.cfg (the settings) and cb.dat (the players and high scores)
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the
// framework's test hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "WaterBugsApp.h"
#include "platform/VFS.h"
#include "platform/WebPersist.h"
#include "platform/openmpt/OpenMPTPlayer.h"

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

// The game's art banks, data and sounds, and its exe for the texts
static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "waterbugs.exe").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "water_art.bbk").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "water_art8.bbk").empty();
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
	std::vector<std::string> candidates = {exe, exe + "/assets/waterbugs", exe + "/../assets/waterbugs",
		exe + "/../../assets/waterbugs", "assets/waterbugs"};
#ifdef _WIN32
	// The retail installs (the collection's folder, and a single game's)
	for (const char* env : {"ProgramFiles", "ProgramFiles(x86)"})
	{
		const char* pf = SDL_getenv(env);
		if (pf)
		{
			candidates.push_back(std::string(pf) + "/Popcap Game Collection/Water Bugs");
			candidates.push_back(std::string(pf) + "/PopCap Games/Water Bugs");
		}
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
	// port (web): its page (web/shell.html, games/waterbugs/web/page.js) mounts its own IndexedDB-backed folder,
	// so Water Bugs' files never mix with the other games' on the same site
	if (gameDir.empty())
		gameDir = "/wbpersist/game";
	if (userDir.empty())
		userDir = "/wbpersist/userdata";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty())
	{
		char* pref = SDL_GetPrefPath("wasm-bejeweled", "Water Bugs");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir))
	{
		std::string msg = "Could not find the Water Bugs game files.\n\n"
			"Run with -gamedir=\"<your Water Bugs folder>\" or copy the game's folder (WaterBugs.exe, art.bbk, "
			"water_art8.bbk, wanims and audio; the game's texts are read from the exe) into an assets/waterbugs "
			"folder next to the executable.";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Water Bugs", msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}

	Platform::MakeDirs(userDir);
	// Web: saves written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(userDir);
	Platform::VFS::Get().SetBaseDir(gameDir);
	Platform::VFS::Get().SetUserDir(userDir);
	SDL_Log("game: %s | user: %s", gameDir.c_str(), userDir.c_str());

	// The music is five MO3 modules (BASS in the original): libopenmpt
	Platform::RegisterOpenMPTModules();

	static WaterBugsApp* anApp = new WaterBugsApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	delete anApp;
#endif
	return 0;
}
