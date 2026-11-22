// Entry point for the Talismania port (native; the web build comes with the first playable port).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing Talismania.saf, music/, splash/ and properties/
//   -userdir=<dir>   writable folder for the settings and the players
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the framework's test
// hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "TalismaniaApp.h"
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

// The game's data is Talismania.saf with the loose music/, splash/ and properties/
static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "Talismania.saf").empty();
}

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
	std::vector<std::string> candidates = {exe, exe + "/assets/talismania", exe + "/../assets/talismania",
		exe + "/../../assets/talismania", "assets/talismania"};
	for (const std::string& c : candidates)
		if (LooksLikeGameDir(c))
			return c;
	return "";
}

int main(int argc, char** argv)
{
	std::string gameDir = ArgValue(argc, argv, "gamedir");
	std::string userDir = ArgValue(argc, argv, "userdir");
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty())
	{
		char* pref = SDL_GetPrefPath("byo-pop", "Talismania");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}

	if (gameDir.empty() || !LooksLikeGameDir(gameDir))
	{
		std::string msg = "Could not find the Talismania game files.\n\n"
			"Run with -gamedir=\"<your Talismania folder>\" or copy Talismania.saf, music/, splash/ and properties/ into "
			"an assets/talismania folder next to the executable "
			"(games/talismania/tools/stage_talismania_assets.py).";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Talismania", msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}

	Platform::MakeDirs(userDir);
	Platform::VFS::Get().SetBaseDir(gameDir);
	Platform::VFS::Get().SetUserDir(userDir);
	SDL_Log("game: %s | user: %s", gameDir.c_str(), userDir.c_str());

	static TalismaniaApp* anApp = new TalismaniaApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
	anApp->Shutdown();
	delete anApp;
	return 0;
}
