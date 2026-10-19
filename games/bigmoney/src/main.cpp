// Entry point for the Big Money Deluxe port (native and Emscripten).
//
// Arguments (all optional):
//   -gamedir=<dir>   folder containing the game's images/, data/, sounds/, music/, properties/
//   -userdir=<dir>   writable folder for savegame.dat, hiscores.dat, registry.ini and mods.json
//   -modsdir=<dir>   folder containing data mods (one sub-folder per mod)
//   -nomods          start without any mods
// Remaining "-key=value" arguments are passed to the app (SexyAppBase::HandleCmdLineParam), e.g. the
// framework's test hooks -seed=N, -script=<file>, -fast and -trace (see SexyAppBase.h).

#include "WinBMApp.h"
#include "platform/ModManager.h"
#include "platform/VFS.h"
#include "platform/WebPersist.h"

#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace Sexy;

// platform/SteamPath.cpp (part of sexyframework): Steam's install folder from the Windows registry ("" elsewhere)
std::string GetSteamInstallPath();

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

static bool LooksLikeGameDir(const std::string& theDir)
{
	return !Platform::VFS::FindCaseInsensitive(theDir, "images/mrmoneybags.jpg").empty() &&
		!Platform::VFS::FindCaseInsensitive(theDir, "data/bigmoneyfont1.txt").empty();
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

// Steam's library folders, read from <Steam>/steamapps/libraryfolders.vdf.
static std::vector<std::string> SteamLibraries()
{
	std::vector<std::string> libs;
	std::string steam = GetSteamInstallPath();
	if (steam.empty())
		return libs;
	libs.push_back(steam);
	FILE* f = fopen((steam + "/steamapps/libraryfolders.vdf").c_str(), "rb");
	if (f == NULL)
		return libs;
	std::string text;
	char buf[4096];
	size_t got;
	while ((got = fread(buf, 1, sizeof(buf), f)) > 0)
		text.append(buf, got);
	fclose(f);
	size_t pos = 0;
	while ((pos = text.find("\"path\"", pos)) != std::string::npos)
	{
		size_t q1 = text.find('"', pos + 6), q2 = q1 == std::string::npos ? q1 : text.find('"', q1 + 1);
		if (q2 == std::string::npos)
			break;
		std::string lib;
		for (size_t i = q1 + 1; i < q2; i++)
		{
			if (text[i] == '\\' && i + 1 < q2 && text[i + 1] == '\\')
				i++;
			lib += text[i];
		}
		libs.push_back(lib);
		pos = q2;
	}
	return libs;
}

static std::string FindGameDir()
{
	std::string exe = ExeDir();
	std::vector<std::string> candidates = {exe, exe + "/assets/bigmoney", exe + "/../assets/bigmoney",
		exe + "/../../assets/bigmoney", "assets/bigmoney"};
	for (const std::string& lib : SteamLibraries())
		candidates.push_back(lib + "/steamapps/common/Big Money Deluxe");
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
	// port (web): games/bigmoney/web/shell.html mounts its own IndexedDB-backed folder, so Big Money's files
	// never mix with Bejeweled's (/persist) on the same site
	if (gameDir.empty())
		gameDir = "/bmpersist/game";
	if (userDir.empty())
		userDir = "/bmpersist/userdata";
	if (modsDir.empty())
		modsDir = "/bmpersist/mods";
#else
	if (gameDir.empty())
		gameDir = FindGameDir();
	if (userDir.empty())
	{
		char* pref = SDL_GetPrefPath("wasm-bejeweled", "Big Money Deluxe");
		userDir = pref ? pref : "userdata";
		SDL_free(pref);
		while (!userDir.empty() && (userDir.back() == '/' || userDir.back() == '\\'))
			userDir.pop_back();
	}
	if (modsDir.empty())
		modsDir = ExeDir() + "/mods-bigmoney";
#endif

	if (gameDir.empty() || !LooksLikeGameDir(gameDir))
	{
		std::string msg = "Could not find the Big Money Deluxe game files.\n\n"
			"Run with -gamedir=\"<your Big Money Deluxe folder>\" or copy the images, data, sounds, music and "
			"properties folders next to the executable.";
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Big Money Deluxe", msg.c_str(), nullptr);
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

	// The original keeps the app as a global object and calls Init, Start and Shutdown on it.
	static WinBMApp* anApp = new WinBMApp();
	anApp->SetCmdLine(argc, argv);
	anApp->Init();
	anApp->Start();
#ifndef __EMSCRIPTEN__
	anApp->Shutdown();
	delete anApp;
#endif
	return 0;
}
