// Entry point for the BookWorm Deluxe port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing the game's images/, fonts/, sounds/, music/, properties/, the word
//                    files (wordlist.txt, bonus_word_dictionary.txt, popdefs.txt) and Bookworm.exe (read for
//                    the game's texts, never run)
//   -userdir=<dir>   writable folder for the players' profiles and saves, registry.ini and mods.json
//   -modsdir=<dir>   folder containing data mods (one sub-folder per mod)
//   -nomods          start without any mods
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the
// framework's test hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "BookWormApp.h"
#include "platform/ModManager.h"
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

static bool HasFlag(int argc, char** argv, const char* theFlag)
{
	for (int i = 1; i < argc; i++)
	{
		const char* a = argv[i];
		while (*a == '-')
			a++;
		if (!strcmp(a, theFlag))
			return true;
	}
	return false;
}

// The game's texts are read from the original's Bookworm.exe (GameText.h), so it is part of the game folder
static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "wordlist.txt").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "music/bookworm1.mo3").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "bookworm.exe").empty();
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
	std::vector<std::string> candidates = {exe, exe + "/assets/bookworm", exe + "/../assets/bookworm",
		exe + "/../../assets/bookworm", "assets/bookworm"};
#ifdef _WIN32
	// The retail install (the CD-protect edition's default folder)
	for (const char* env : {"ProgramFiles", "ProgramFiles(x86)"})
	{
		const char* pf = SDL_getenv(env);
		if (pf)
			candidates.push_back(std::string(pf) + "/PopCap Games/BookWorm Deluxe");
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
	// port (web): games/bookworm/web/shell.html mounts its own IndexedDB-backed folder, so BookWorm's files
	// never mix with the other games' on the same site
	if (gameDir.empty())
		gameDir = "/bwpersist/game";
	if (userDir.empty())
		userDir = "/bwpersist/userdata";
	if (modsDir.empty())
		modsDir = "/bwpersist/mods";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty())
	{
		char* pref = SDL_GetPrefPath("wasm-bejeweled", "BookWorm Deluxe");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
	if (modsDir.empty())
		modsDir = ExeDir() + "/mods-bookworm";
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir))
	{
		std::string msg = "Could not find the BookWorm Deluxe game files.\n\n"
			"Run with -gamedir=\"<your BookWorm Deluxe folder>\" or copy the images, fonts, sounds, music and "
			"properties folders, the word files (wordlist.txt, bonus_word_dictionary.txt, popdefs.txt) and the "
			"original Bookworm.exe (the game's texts are read from it) into an assets/bookworm folder next to "
			"the executable.";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "BookWorm Deluxe", msg.c_str(), nullptr);
		SDL_Log("%s", msg.c_str());
		return 1;
	}

	Platform::MakeDirs(userDir);
	// Web: saves written as the page was closed may only have reached localStorage
	Platform::RestoreUserDataBackup(userDir);
	Platform::MakeDirs(modsDir);
	Platform::VFS::Get().SetBaseDir(gameDir);
	Platform::VFS::Get().SetUserDir(userDir);
	Platform::ModManager::Get().SetModsDir(modsDir);
	if (!HasFlag(argc, argv, "nomods"))
		Platform::ModManager::Get().Refresh();
	SDL_Log("game: %s | user: %s | mods: %s", gameDir.c_str(), userDir.c_str(), modsDir.c_str());

	// The music is one MO3 module played by order position (BASS in the original): libopenmpt
	Platform::RegisterOpenMPTModules();

	// The original keeps the app as a global object and calls Init, Start and Shutdown on it.
	static BookWormApp* anApp = new BookWormApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	delete anApp;
#endif
	return 0;
}
