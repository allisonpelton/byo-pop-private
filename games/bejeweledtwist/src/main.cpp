// Entry point for the Bejeweled Twist port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder with the game's images/, fonts/, sounds/, music/, properties/ and bejeweledtwist.exe (read
//                    for its texts and curves, never run): the staged copy that
//                    games/bejeweledtwist/tools/stage_bejeweledtwist_assets.py makes
//   -userdir=<dir>   writable folder for the profiles and registry.ini
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the framework's test
// hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "WinTwistApp.h"
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

// The game's curves and texts in C++ are read from the original's exe (GameText.h), so it is part of the game folder
static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "bejeweledtwist.exe").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "properties/resources.xml").empty();
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

// port: not the install itself: its pictures are packed in main.pak, which staging unpacks
static std::string FindGameDir()
{
	std::string exe = ExeDir();
	std::vector<std::string> candidates = {exe, exe + "/assets/bejeweledtwist", exe + "/../assets/bejeweledtwist",
		exe + "/../../assets/bejeweledtwist", "assets/bejeweledtwist"};
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
	if (gameDir.empty())
		gameDir = "/bjtpersist/game";
	if (userDir.empty())
		userDir = "/bjtpersist/userdata";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty()) {
		char* pref = SDL_GetPrefPath("byo-pop", "Bejeweled Twist");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir)) {
		std::string msg = "Could not find the Bejeweled Twist game files.\n\n"
			"Stage them with games/bejeweledtwist/tools/stage_bejeweledtwist_assets.py (it makes assets/bejeweledtwist) "
			"or run with -gamedir=\"<the staged folder>\".";
		// port: a scripted test run shows no box (on its hidden desktop nobody could close it)
		if (ArgValue(argc, argv, "script").empty())
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bejeweled Twist", msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}

	Platform::MakeDirs(userDir);
	// Web: saves written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(userDir);
	Platform::VFS::Get().SetBaseDir(gameDir);
	Platform::VFS::Get().SetUserDir(userDir);
	SDL_Log("game: %s | user: %s", gameDir.c_str(), userDir.c_str());

	// The music is two MO3 modules (music/), which libopenmpt plays, and the pictures are JPEG 2000
	Platform::RegisterOpenMPTModules();
	Platform::RegisterOpenJPEGImages();

	// The original keeps the app as a global object and calls Init, Start and Shutdown on it.
	static WinTwistApp* anApp = new WinTwistApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	delete anApp;
#endif
	return 0;
}
