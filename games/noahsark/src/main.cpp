// port: the entry point of Noah's Ark Deluxe, native and Emscripten.
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing the game's images/, data/, sounds/, music/ and WinNoah.exe (read for the
//                    game's texts, never run)
//   -userdir=<dir>   writable folder for the saved game, high scores and registry.ini
//   -modsdir=<dir>   folder containing data mods (one sub-folder per mod)
//   -nomods          start without any mods
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the
// framework's test hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "NoahApp.h"
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
	size_t aKeyLength = strlen(theKey);
	for (int i = 1; i < argc; i++)
	{
		const char* anArg = argv[i];
		while (*anArg == '-')
			anArg++;
		if (!strncmp(anArg, theKey, aKeyLength) && anArg[aKeyLength] == '=')
			return anArg + aKeyLength + 1;
	}
	return "";
}

static bool HasFlag(int argc, char** argv, const char* theFlag)
{
	for (int i = 1; i < argc; i++)
	{
		const char* anArg = argv[i];
		while (*anArg == '-')
			anArg++;
		if (!strcmp(anArg, theFlag))
			return true;
	}
	return false;
}

// The game's texts are read from the original's WinNoah.exe (GameText.h), so it is part of the game folder
static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "winnoah.exe").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "music/mainloop.ogg").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "images/title.jpg").empty();
}

#ifndef __EMSCRIPTEN__
static std::string ExeDir()
{
	char* aBasePath = SDL_GetBasePath();
	std::string aDir = aBasePath ? aBasePath : "./";
	SDL_free(aBasePath);
	while (!aDir.empty() && (aDir.back() == '/' || aDir.back() == '\\'))
		aDir.pop_back();
	return aDir;
}

static std::string FindGameDir()
{
	std::string anExeDir = ExeDir();
	std::vector<std::string> aCandidates = {anExeDir, anExeDir + "/assets/noahsark", anExeDir + "/../assets/noahsark",
		anExeDir + "/../../assets/noahsark", "assets/noahsark"};
#ifdef _WIN32
	// The retail install (the installer's default folder)
	for (const char* anEnvName : {"ProgramFiles", "ProgramFiles(x86)"})
	{
		const char* aProgramFiles = SDL_getenv(anEnvName);
		if (aProgramFiles)
			aCandidates.push_back(std::string(aProgramFiles) + "/PopCap Games/Noah's Ark Deluxe");
	}
#endif
	for (const std::string& aCandidate : aCandidates)
		if (LooksLikeGameDir(aCandidate))
			return aCandidate;
	return "";
}
#endif

// 0x411380: the original's WinMain: it keeps the instance handle and runs Init, Start and Shutdown on the global app
// object. The port does the same with an app it makes here; the platform layer replaces the instance handle.
int main(int argc, char** argv)
{
	std::string aGameDir = ArgValue(argc, argv, "gamedir");
	std::string aUserDir = ArgValue(argc, argv, "userdir");
	std::string aModsDir = ArgValue(argc, argv, "modsdir");

#ifdef __EMSCRIPTEN__
	// port (web): its page (web/shell.html, games/noahsark/web/page.js) mounts its own IndexedDB-backed folder, so
	// Noah's Ark's files never mix with the other games' on the same site
	if (aGameDir.empty())
		aGameDir = "/napersist/game";
	if (aUserDir.empty())
		aUserDir = "/napersist/userdata";
	if (aModsDir.empty())
		aModsDir = "/napersist/mods";
#else
	if (aGameDir.empty())
		aGameDir = FindGameDir();
	if (aUserDir.empty())
	{
		char* pref = SDL_GetPrefPath("byo-pop", "Noahs Ark Deluxe");
		aUserDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!aUserDir.empty() && (aUserDir.back() == '/' || aUserDir.back() == '\\'))
			aUserDir.pop_back();
	}
	if (aModsDir.empty())
		aModsDir = ExeDir() + "/mods-noahsark";
#endif

#ifndef __EMSCRIPTEN__
	// -importsaves[=<install folder>]: brings the saves, high scores and settings of the original game installed on this computer into the
	// user dir (platform/SaveImport.h), then exits without starting the game. Without it nothing changes.
	{
		int anImportResult = 0;
		if (Platform::RunSaveImportIfRequested(argc, argv, "noahsark", aGameDir, aUserDir, &anImportResult))
			return anImportResult;
	}
#endif

	if (aGameDir.empty() || !LooksLikeGameDir(aGameDir))
	{
		std::string aMessage = "Could not find the Noah's Ark Deluxe game files.\n\n"
			"Run with -gamedir=\"<your Noah's Ark Deluxe folder>\" or copy the images, data, sounds and music "
			"folders and the original WinNoah.exe (the game's texts are read from it) into an assets/noahsark "
			"folder next to the executable.";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Noah's Ark Deluxe", aMessage.c_str(), nullptr);
		SDL_Log("%s", aMessage.c_str());
		return 1;
	}

	Platform::MakeDirs(aUserDir);
	// Web: saves written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(aUserDir);
	Platform::MakeDirs(aModsDir);
	Platform::VFS::Get().SetBaseDir(aGameDir);
	Platform::VFS::Get().SetUserDir(aUserDir);
	Platform::ModManager::Get().SetModsDir(aModsDir);
	if (!HasFlag(argc, argv, "nomods"))
		Platform::ModManager::Get().Refresh();
	SDL_Log("game: %s | user: %s | mods: %s", aGameDir.c_str(), aUserDir.c_str(), aModsDir.c_str());

	// The original keeps the app as a global object and calls Init, Start and Shutdown on it.
	static NoahApp* anApp = new NoahApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	delete anApp;
#endif
	return 0;
}
