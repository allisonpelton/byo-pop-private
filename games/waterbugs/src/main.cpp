// Entry point for the Water Bugs port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing WaterBugs.exe (read for the game's texts, never run), water_art.bbk, water_art8.bbk,
//                    wanims/ and audio/
//   -userdir=<dir>   writable folder for cp.cfg (the settings) and cb.dat (the players and high scores)
//   -modsdir=<dir>   folder containing mods (one sub-folder per mod; the fixes mod is games/waterbugs/mods/fixes)
//   -nomods          start without any mods
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the
// framework's test hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "WaterBugsApp.h"
#include "platform/ModManager.h"
#include "platform/SaveImport.h"
#include "platform/VFS.h"
#include "platform/WebPersist.h"
#include "platform/openmpt/OpenMPTPlayer.h"

#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace Sexy;

static bool HasFlag(int theArgc, char** theArgv, const char* theKey)
{
	for (int i = 1; i < theArgc; i++)
	{
		const char* a = theArgv[i];
		while (*a == '-')
			a++;
		if (!strcmp(a, theKey))
			return true;
	}
	return false;
}

static std::string ArgValue(int theArgc, char** theArgv, const char* theKey)
{
	size_t n = strlen(theKey);
	for (int i = 1; i < theArgc; i++)
	{
		const char* a = theArgv[i];
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
	char* aBase = SDL_GetBasePath();
	std::string aDir = aBase ? aBase : "./";
	SDL_free(aBase);
	while (!aDir.empty() && (aDir.back() == '/' || aDir.back() == '\\'))
		aDir.pop_back();
	return aDir;
}

static std::string FindGameDir()
{
	std::string anExe = ExeDir();
	std::vector<std::string> aCandidates = {anExe, anExe + "/assets/waterbugs", anExe + "/../assets/waterbugs",
		anExe + "/../../assets/waterbugs", "assets/waterbugs"};
#ifdef _WIN32
	// The retail installs (the collection's folder, and a single game's)
	for (const char* anEnv : {"ProgramFiles", "ProgramFiles(x86)"})
	{
		const char* aPf = SDL_getenv(anEnv);
		if (aPf)
		{
			aCandidates.push_back(std::string(aPf) + "/Popcap Game Collection/Water Bugs");
			aCandidates.push_back(std::string(aPf) + "/PopCap Games/Water Bugs");
		}
	}
#endif
	for (const std::string& c : aCandidates)
		if (LooksLikeGameDir(c))
			return c;
	return "";
}
#endif

int main(int theArgc, char** theArgv)
{
	std::string aGameDir = ArgValue(theArgc, theArgv, "gamedir");
	std::string anUserDir = ArgValue(theArgc, theArgv, "userdir");
	std::string aModsDir = ArgValue(theArgc, theArgv, "modsdir");

#ifdef __EMSCRIPTEN__
	// port (web): its page (web/shell.html, games/waterbugs/web/page.js) mounts its own IndexedDB-backed folder,
	// so Water Bugs' files never mix with the other games' on the same site
	if (gameDir.empty())
		gameDir = "/wbpersist/game";
	if (userDir.empty())
		userDir = "/wbpersist/userdata";
	if (modsDir.empty())
		modsDir = "/wbpersist/mods";
#else
	if (aGameDir.empty())
		aGameDir = FindGameDir();
	if (anUserDir.empty())
	{
		char* aPref = SDL_GetPrefPath("byo-pop", "Water Bugs");
		anUserDir = aPref ? aPref : "userdata";
		SDL_free(aPref);
		while (!anUserDir.empty() && (anUserDir.back() == '/' || anUserDir.back() == '\\'))
			anUserDir.pop_back();
	}
	if (aModsDir.empty())
		aModsDir = ExeDir() + "/mods-waterbugs";
#endif

#ifndef __EMSCRIPTEN__
	// -importsaves[=<install folder>]: brings the saves, high scores and settings of the original game installed on this computer into the
	// user dir (platform/SaveImport.h), then exits without starting the game. Without it nothing changes.
	{
		int anImportResult = 0;
		if (Platform::RunSaveImportIfRequested(theArgc, theArgv, "waterbugs", aGameDir, anUserDir, &anImportResult))
			return anImportResult;
	}
#endif

	if (aGameDir.empty() || !LooksLikeGameDir(aGameDir))
	{
		std::string aMsg = "Could not find the Water Bugs game files.\n\n"
			"Run with -gamedir=\"<your Water Bugs folder>\" or copy the game's folder (WaterBugs.exe, art.bbk, "
			"water_art8.bbk, wanims and audio; the game's texts are read from the exe) into an assets/waterbugs "
			"folder next to the executable.";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Water Bugs", aMsg.c_str(), nullptr);
		SDL_Log("%s", aMsg.c_str());
		return 1;
	}

	Platform::MakeDirs(anUserDir);
	// Web: saves written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(anUserDir);
	Platform::VFS::Get().SetBaseDir(aGameDir);
	Platform::VFS::Get().SetUserDir(anUserDir);
	// The mods (the fixes mod, games/waterbugs/docs/GAME_NOTES.md): none without a mod folder, and the game plays as the original
	Platform::MakeDirs(aModsDir);
	Platform::ModManager::Get().SetModsDir(aModsDir);
	if (!HasFlag(theArgc, theArgv, "nomods"))
		Platform::ModManager::Get().Refresh();
	SDL_Log("game: %s | user: %s | mods: %s", aGameDir.c_str(), anUserDir.c_str(), aModsDir.c_str());

	// The music is five MO3 modules (BASS in the original): libopenmpt
	Platform::RegisterOpenMPTModules();

	static WaterBugsApp* sAnApp = new WaterBugsApp();
	sAnApp->SetCmdLine(theArgc, theArgv);
	sAnApp->Init();
	sAnApp->Start();
#ifndef __EMSCRIPTEN__
	sAnApp->Shutdown();
	delete sAnApp;
#endif
	return 0;
}
