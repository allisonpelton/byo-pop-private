// Entry point for the Heavy Weapon Deluxe port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder with the game's data/, fonts/, images/, music/, sounds/ and properties/ and its executable
//                    (Heavy Weapon Deluxe.exe, read for the game's texts and never run): the staged copy that
//                    games/heavyweapon/tools/stage_heavyweapon_assets.py makes (assets/heavyweapon)
//   -userdir=<dir>   writable folder for the game's userdata/ (players and high scores) and registry.ini
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the framework's test
// hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "HeavyWeaponApp.h"
#include "GameText.h"
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

// The game's texts are read from the original's executable (GameText.h), so it is part of the game folder
static bool LooksLikeGameDir(const std::string& theDir)
{
	return !FindGameExecutable(theDir).empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "data/craft.xml").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "properties/partner.xml").empty();
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

// next to the exe, or byo-pop's staged copy (games/heavyweapon/tools/stage_heavyweapon_assets.py) from build/native
static std::string FindGameDir()
{
	std::string exe = ExeDir();
	std::vector<std::string> candidates = {exe, exe + "/assets/heavyweapon", exe + "/../assets/heavyweapon",
		exe + "/../../assets/heavyweapon", "assets/heavyweapon"};
	for (const std::string& c : candidates)
		if (LooksLikeGameDir(c))
			return c;
	return "";
}
#endif

// port: the original's WinMain (0x40e210) makes no app (it is the static global 0x56b1a0); it calls Init, seeds the
// app's own generator from Rand(), and calls Start and Shutdown. The port reads its arguments first.
int main(int argc, char** argv)
{
	std::string gameDir = ArgValue(argc, argv, "gamedir");
	std::string userDir = ArgValue(argc, argv, "userdir");

#ifdef __EMSCRIPTEN__
	// port (web): its page (web/shell.html, games/heavyweapon/web/page.js) mounts its own IndexedDB-backed folder, so
	// the game's files never mix with the other games' on the same site
	if (gameDir.empty())
		gameDir = "/hwpersist/game";
	if (userDir.empty())
		userDir = "/hwpersist/userdata";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty())
	{
		char* pref = SDL_GetPrefPath("byo-pop", "Heavy Weapon Deluxe");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir))
	{
		std::string msg = "Could not find the Heavy Weapon Deluxe game files.\n\n"
			"Stage them with games/heavyweapon/tools/stage_heavyweapon_assets.py (it makes assets/heavyweapon) "
			"or run with -gamedir=\"<your Heavy Weapon Deluxe folder>\".";
		// port: a scripted test run shows no box (on its hidden desktop nobody could close it)
		if (ArgValue(argc, argv, "script").empty())
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Heavy Weapon Deluxe", msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}

	Platform::MakeDirs(userDir);
	// Web: saves written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(userDir);
	Platform::VFS::Get().SetBaseDir(gameDir);
	Platform::VFS::Get().SetUserDir(userDir);
	SDL_Log("game: %s | user: %s", gameDir.c_str(), userDir.c_str());

	// The music is one MO3 module played from order positions (BASS in the original): libopenmpt. Without this the
	// module does not load and the game plays no music
	Platform::RegisterOpenMPTModules();

	static HeavyWeaponApp* anApp = new HeavyWeaponApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	std::vector<std::string> aRestartArgs;
	bool restart = anApp->TakeRelaunch(&aRestartArgs);
	delete anApp;
	if (restart)
		Platform::Relaunch(aRestartArgs, "Heavy Weapon Deluxe");
#endif
	return 0;
}
