// Entry point for the Feeding Frenzy 2 port (native; the web build comes with the first playable port).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing FF2.saf, music/, splash/ and properties/
//   -userdir=<dir>   writable folder for the settings and the players
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the framework's test
// hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "FeedingFrenzy2App.h"
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

// The game's data is FF2.saf with the loose music/, splash/ and properties/
static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "FF2.saf").empty();
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
	std::vector<std::string> candidates = {exe, exe + "/assets/feedingfrenzy2", exe + "/../assets/feedingfrenzy2",
		exe + "/../../assets/feedingfrenzy2", "assets/feedingfrenzy2"};
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
		char* pref = SDL_GetPrefPath("byo-pop", "Feeding Frenzy 2");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}

	if (gameDir.empty() || !LooksLikeGameDir(gameDir))
	{
		std::string msg = "Could not find the Feeding Frenzy 2 game files.\n\n"
			"Run with -gamedir=\"<your Feeding Frenzy 2 folder>\" or copy FF2.saf, music/, splash/ and properties/ into "
			"an assets/feedingfrenzy2 folder next to the executable "
			"(games/feedingfrenzy2/tools/stage_feedingfrenzy2_assets.py).";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Feeding Frenzy 2", msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}

	Platform::MakeDirs(userDir);
	Platform::VFS::Get().SetBaseDir(gameDir);
	Platform::VFS::Get().SetUserDir(userDir);
	SDL_Log("game: %s | user: %s", gameDir.c_str(), userDir.c_str());

	static FeedingFrenzy2App* anApp = new FeedingFrenzy2App();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
	anApp->Shutdown();
	delete anApp;
	return 0;
}
