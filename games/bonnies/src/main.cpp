// Entry point for the Bonnie's Bookstore Deluxe port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing BonniesBookstore.exe and Data/Bonnie.pak (the exe is read for the game's
//                    data, never run)
//   -userdir=<dir>   writable folder for the players' saved games and the preferences (SavedGames/)
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the
// framework's test hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "BonniesHost.h"
#include "../engine/Exe.h"
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

namespace bonnies
{
// Str.cpp: the seed that seedRandom(0) takes instead of the time (-seed=N, a test hook)
extern unsigned long gTestSeed;
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

static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "BonniesBookstore.exe").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "Data/Bonnie.pak").empty();
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
	std::vector<std::string> candidates = {exe, exe + "/assets/bonnies", exe + "/../assets/bonnies",
		exe + "/../../assets/bonnies", "assets/bonnies"};
#ifdef _WIN32
	// The retail install (the installer's default folder)
	for (const char* env : {"ProgramFiles", "ProgramFiles(x86)"})
	{
		const char* pf = SDL_getenv(env);
		if (pf)
			candidates.push_back(std::string(pf) + "/PopCap Games/Bonnie's Bookstore Deluxe");
	}
#endif
	for (const std::string& c : candidates)
		if (LooksLikeGameDir(c))
			return c;
	return "";
}
#endif

// The game inside the user's BonniesBookstore.exe: the distributor's wrapper holds it after its marker
static bool LoadGameExe()
{
	std::vector<uint8_t> aData;
	if (!Platform::VFS::Get().ReadAll("BonniesBookstore.exe", aData) || aData.empty())
		return false;
	size_t aBase = 0;
	const char* aMarkers[] = {"!popcapdrmprotect!", "!popcapcdprotect!"};
	for (const char* aMarker : aMarkers)
	{
		size_t aMarkerLen = strlen(aMarker);
		bool aFoundMarker = false;
		for (size_t i = 0; i + aMarkerLen < aData.size(); i++)
		{
			if (aData[i] == '!' && memcmp(&aData[i], aMarker, aMarkerLen) == 0)
			{
				aBase = i + aMarkerLen;
				while (aBase + 1 < aData.size() && !(aData[aBase] == 'M' && aData[aBase + 1] == 'Z'))
					aBase++;
				aFoundMarker = true;
				break;
			}
		}
		if (aFoundMarker)
			break;
	}
	std::vector<uint8_t> aGame(aData.begin() + aBase, aData.end());
	if (!bonnies::loadExe(aGame))
		return false;
	std::vector<uint32_t> anUnknown;
	bonnies::runStaticInitializers(&anUnknown);
	if (!anUnknown.empty())
		SDL_Log("BonniesBookstore.exe: %d static initializers not understood", (int)anUnknown.size());
	return true;
}

int main(int argc, char** argv)
{
	std::string gameDir = ArgValue(argc, argv, "gamedir");
	std::string userDir = ArgValue(argc, argv, "userdir");

#ifdef __EMSCRIPTEN__
	// port (web): its page (web/shell.html, games/bonnies/web/page.js) mounts its own IndexedDB-backed folder, so
	// Bonnie's files never mix with the other games' on the same site
	if (gameDir.empty())
		gameDir = "/bbpersist/game";
	if (userDir.empty())
		userDir = "/bbpersist/userdata";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty())
	{
		char* pref = SDL_GetPrefPath("byo-pop", "Bonnie's Bookstore Deluxe");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
#endif

#ifndef __EMSCRIPTEN__
	// -importsaves[=<install folder>]: brings the players and settings of the original game installed on this
	// computer into the user dir (platform/SaveImport.h), then exits without starting the game
	{
		int anImportResult = 0;
		if (Platform::RunSaveImportIfRequested(argc, argv, "bonnies", gameDir, userDir, &anImportResult))
			return anImportResult;
	}
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir))
	{
		std::string msg = "Could not find the Bonnie's Bookstore Deluxe game files.\n\n"
			"Run with -gamedir=\"<your Bonnie's Bookstore Deluxe folder>\" or copy the game's folder (BonniesBookstore.exe, "
			"Data, Graphics and Sound) into an assets/bonnies folder next to the executable.";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bonnie's Bookstore Deluxe", msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}

	Platform::MakeDirs(userDir);
	// Web: saves written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(userDir);
	Platform::VFS::Get().SetBaseDir(gameDir);
	Platform::VFS::Get().SetUserDir(userDir);
	SDL_Log("game: %s | user: %s", gameDir.c_str(), userDir.c_str());

	if (!LoadGameExe())
	{
		std::string msg = "BonniesBookstore.exe in the game folder is not the expected version of Bonnie's Bookstore "
			"Deluxe; the port reads the game's data from it.";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bonnie's Bookstore Deluxe", msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}
	// the music is an OXM (an XM with Ogg Vorbis samples)
	Platform::RegisterOpenMPTOggXM();

	std::string aSeed = ArgValue(argc, argv, "seed");
	if (!aSeed.empty())
		bonnies::gTestSeed = strtoul(aSeed.c_str(), nullptr, 10);

	static BonniesHostApp* anApp = new BonniesHostApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	delete anApp;
#endif
	return 0;
}
