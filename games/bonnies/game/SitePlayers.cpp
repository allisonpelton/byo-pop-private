// port (web): the game's players for the site's player setting (Start.h; src/SiteProfiles.cpp). Each function does what
// the game's own dialog does, with the same calls, so that a player made or chosen by the site is one the game made.
#ifdef __EMSCRIPTEN__
#include "Start.h"
#include "Exe.h"
#include "File.h"
#include "Game.h"
#include "Globals.h"
#include "Music.h"
#include "Options.h"
#include "Platform.h"
#include "Str.h"

namespace bonnies
{

void (*gSiteStart)() = nullptr;
void (*gSitePlayersChanged)() = nullptr;
std::string (*gSiteNameFor)() = nullptr;

// PlayersPanel::refill (0x4188a0): the saved games' names
std::vector<std::string> sitePlayers()
{
	std::vector<std::string> aPlayers;
	int aCount;
	Str* aNames = nullptr;
	listFiles(savedGamesDir(false), Str(lit(0x4575cc)), &aCount, &aNames);
	for (int i = 0; i < aCount; i++)
		aPlayers.push_back(aNames[i]);
	delete[] aNames;
	return aPlayers;
}

std::string siteCurrent()
{
	if (gOptions == nullptr || gOptions->mName.empty() || playerSaveExists(gOptions->mName) == 0)
		return "";
	return gOptions->mName;
}

// askName (0x4175f0): mNamesOnly and mMaxLength 0x14; TextField's CHARACTER (Widget.cpp) keeps a character from 0x20 to
// 0xff but '|', and of those blanks, '_', digits, letters and 0xc4 up; NamePanel's OK trims the text
std::string siteFit(const std::string& theName)
{
	Str aText;
	for (size_t i = 0; i < theName.size(); i++)
	{
		int aChar = (unsigned char)theName[i];
		if (aChar < 0x20 || aChar == '|')
			continue;
		if ((int)aText.size() >= 0x14)
			break;
		if (aChar != ' ' && aChar != '_' && (aChar < '0' || '9' < aChar) && (aChar < 'A' || 'Z' < aChar) &&
			(aChar < 'a' || 'z' < aChar) && aChar < 0xc4)
			continue;
		aText += (char)aChar;
	}
	return trim(aText, false);
}

// NamePanel::handleEvent's OK (0x4173c0)
void siteCreate(const std::string& theName)
{
	saveGameAs(theName);
	gOptions->mName = theName;
	gOptions->save();
}

// PlayersPanel::handleEvent's choose (0x4189b0)
void siteSelect(const std::string& theName)
{
	gOptions->mName = theName;
	gOptions->save();
}

// PlayersPanel::handleEvent's Delete (0x4189b0), with the ".gam" the original left out (Screens.cpp)
void siteDelete(const std::string& theName)
{
	Str aPath = "";
	aPath += savedGamesDir(true);
	aPath += theName;
	aPath += exeGlobalString(0x475a54);
	fileRemove(aPath);
}

// OptionsPanel's sliders (Screens.cpp, the events 0xf of 0x26d and 0x26f): the music's is applied to the playing music
void siteVolumes(float theMusic, float theSound)
{
	if (theSound >= 0.0f)
		gOptions->mSoundVolume = theSound;
	if (theMusic >= 0.0f)
	{
		gOptions->mMusicVolume = theMusic;
		if (gMusic != nullptr)
			gMusic->setVolume(1.0f);
	}
}

} // namespace bonnies
#endif
