#include "Placeholders.h"
#include "ModManager.h"
#include "VFS.h"

#include <SDL.h>
#include <cstdio>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS(void, byopop_post_missing_file, (const char* thePath), {
	if (typeof Module['onMissingFile'] === 'function')
		Module['onMissingFile'](UTF8ToString(thePath));
});
#endif

namespace Platform
{

static bool gPlaceholders = false;
static std::vector<std::string> gMissing;

#ifndef __EMSCRIPTEN__
// The whole list, so the file is current whenever the run ends
static void WriteMissingList()
{
	std::string aFileName = VFS::Get().GetUserDir() + "/missing-files.txt";
	FILE* aFile = fopen(aFileName.c_str(), "w");
	if (aFile == NULL)
		return;
	for (const std::string& aMissing : gMissing)
		fprintf(aFile, "%s\n", aMissing.c_str());
	fclose(aFile);
}
#endif

bool InitPlaceholders(int argc, char* argv[])
{
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-placeholders") == 0)
			SetPlaceholders(true);
	}
	return gPlaceholders;
}

void SetPlaceholders(bool on)
{
	if (on == gPlaceholders)
		return;
	gPlaceholders = on;
	// the applied score key has a "placeholders" entry while the switch is on, -nomods runs included; the framework
	// turns it on in SexyAppBase::Init, after main() has had the mods applied
	ModManager::Get().ComputeScoreKey();
}

bool Placeholders()
{
	return gPlaceholders;
}

void NoteMissing(const std::string& theGamePath)
{
	std::string aPath = theGamePath;
	for (char& c : aPath)
	{
		if (c == '\\')
			c = '/';
	}
	for (const std::string& aKnown : gMissing)
	{
		if (aKnown == aPath)
			return;
	}
	gMissing.push_back(aPath);
	SDL_Log("placeholder: %s", aPath.c_str());

#ifdef __EMSCRIPTEN__
	byopop_post_missing_file(aPath.c_str());
#else
	WriteMissingList();
#endif
}

const std::vector<std::string>& GetMissing()
{
	return gMissing;
}

void ReportMissing()
{
	if (!gPlaceholders)
		return;
	SDL_Log("placeholders: %d game file%s missing", (int) gMissing.size(), (gMissing.size() == 1) ? "" : "s");
#ifndef __EMSCRIPTEN__
	WriteMissingList();
#endif
}

}
