// Entry point for the Bejeweled 3 port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder with the game's images/, fonts/, sounds/, music/, properties/ and bejeweled3.exe (read
//                    for the table of the gems' edges that the time gems' bolts follow, 0x83a548, never run): the staged copy that
//                    games/bejeweled3/tools/stage_bejeweled3_assets.py makes
//   -userdir=<dir>   writable folder for the profiles and registry.ini
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the framework's test
// hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).
#include "Bej3App.h"
#include "platform/ModManager.h"
#include "platform/Relaunch.h"
#include "platform/VFS.h"
#include "platform/WebPersist.h"
#include "platform/openjpeg/JP2Decoder.h"
#include "platform/openmpt/OpenMPTPlayer.h"

#include <SDL.h>
#include <cstring>
#include <string>
#include <vector>

using namespace Sexy;

static std::string ArgValue(int argc, char** argv, const char* theKey)
{
	size_t aKeyLength = strlen(theKey);
	for (int i = 1; i < argc; i++) {
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
	for (int i = 1; i < argc; i++) {
		const char* anArg = argv[i];
		while (*anArg == '-')
			anArg++;
		if (!strcmp(anArg, theFlag))
			return true;
	}
	return false;
}

// The gems' edge table is read from the original's exe (GemSilhouettes), so it is part of the game folder
static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "bejeweled3.exe").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "properties/resources.xml").empty();
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

// port: not the install itself: its pictures are packed in main.pak, which staging unpacks
static std::string FindGameDir()
{
	std::string anExeDir = ExeDir();
	std::vector<std::string> aCandidates = {anExeDir, anExeDir + "/assets/bejeweled3", anExeDir + "/../assets/bejeweled3",
		anExeDir + "/../../assets/bejeweled3", "assets/bejeweled3"};
	for (const std::string& aCandidate : aCandidates) {
		if (LooksLikeGameDir(aCandidate))
			return aCandidate;
	}
	return "";
}
#endif

int main(int argc, char** argv)
{
	std::string aGameDir = ArgValue(argc, argv, "gamedir");
	std::string aUserDir = ArgValue(argc, argv, "userdir");
	std::string aModsDir = ArgValue(argc, argv, "modsdir");
#ifdef __EMSCRIPTEN__
	if (aGameDir.empty())
		aGameDir = "/bj3persist/game";
	if (aUserDir.empty())
		aUserDir = "/bj3persist/userdata";
	if (aModsDir.empty())
		aModsDir = "/bj3persist/mods";
#else
	if (aGameDir.empty())
		aGameDir = FindGameDir();
	if (aUserDir.empty()) {
		char* aPrefPath = SDL_GetPrefPath("byo-pop", "Bejeweled 3");
		aUserDir = aPrefPath ? aPrefPath : "userdata";
		SDL_free(aPrefPath);
		while (!aUserDir.empty() && (aUserDir.back() == '/' || aUserDir.back() == '\\'))
			aUserDir.pop_back();
	}
	if (aModsDir.empty())
		aModsDir = ExeDir() + "/mods-bejeweled3";
#endif
	if (aGameDir.empty() || !LooksLikeGameDir(aGameDir)) {
		std::string aMessage = "Could not find the Bejeweled 3 game files.\n\n"
			"Stage them with games/bejeweled3/tools/stage_bejeweled3_assets.py (it makes assets/bejeweled3) "
			"or run with -gamedir=\"<the staged folder>\".";
		// port: a scripted test run shows no box (on its hidden desktop nobody could close it)
		if (ArgValue(argc, argv, "script").empty())
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bejeweled 3", aMessage.c_str(), nullptr);
		SDL_Log("%s", aMessage.c_str());
		return 1;
	}

	Platform::MakeDirs(aUserDir);
	// Web: saves written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(aUserDir);
	Platform::VFS::Get().SetBaseDir(aGameDir);
	Platform::VFS::Get().SetUserDir(aUserDir);
	// port(mod): the mods (docs/MOD_MENU.md); the port reads only its fixes mod's toggles (Platform::FixOn)
	Platform::MakeDirs(aModsDir);
	Platform::ModManager::Get().SetModsDir(aModsDir);
	if (!HasFlag(argc, argv, "nomods"))
		Platform::ModManager::Get().Refresh();
	SDL_Log("game: %s | user: %s | mods: %s", aGameDir.c_str(), aUserDir.c_str(), aModsDir.c_str());

	// The music is one MO3 module (music/bejeweled3_suite.mo3), which libopenmpt plays, and most pictures are JPEG 2000
	Platform::RegisterOpenMPTModules();
	Platform::RegisterOpenJPEGImages();

	// The original keeps the app as a global object and calls Init, Start and Shutdown on it.
	static Bej3App* anApp = new Bej3App();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	// The options' 3D box restarts the game: platform/Relaunch.h; the web page reloads
	std::vector<std::string> aRestartArgs;
	bool restart = anApp->TakeRelaunch(&aRestartArgs);
	delete anApp;
	if (restart)
		Platform::Relaunch(aRestartArgs, "Bejeweled 3");
#endif
	return 0;
}
