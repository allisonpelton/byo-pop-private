#include "Scoreboards.h"

#include <cstdio>

using namespace engine;

namespace
{
Scoreboards* gScoreboards = nullptr;  // 0x532f68 (pizzafrenzy)
const char* const COUNT_VALUE = "count";  // 0x52120c (pizzafrenzy)

// The name of a value of an entry: its index in three digits and the parameter's name (the format is at
// 0x521208 (pizzafrenzy))
std::string entryValueName(int theIndex, const std::string& theName)
{
	char aNumber[16];
	snprintf(aNumber, sizeof(aNumber), "%03d", theIndex);
	return aNumber + theName;
}
}

// 0x469580 (pizzafrenzy)
void Scoreboards::loadEntry(PersistKey* theKey, int theIndex, ScoreEntry* theEntry)
{
	Params aParams;
	theEntry->defaultParams(aParams);
	for (Params::iterator anIt = aParams.begin(); anIt != aParams.end(); ++anIt)
		anIt->second = theKey->readString(entryValueName(theIndex, anIt->first), anIt->second);
	theEntry->fromParams(aParams);
}

// 0x469ac0 (pizzafrenzy)
void Scoreboards::saveEntry(PersistKey* theKey, int theIndex, ScoreEntry* theEntry)
{
	Params aParams;
	theEntry->toParams(aParams);
	for (Params::iterator anIt = aParams.begin(); anIt != aParams.end(); ++anIt)
		theKey->writeString(entryValueName(theIndex, anIt->first), anIt->second);
}

// 0x469bf0 (pizzafrenzy)
bool Scoreboards::loadBoard(Scoreboard* theBoard)
{
	Ref<PersistKey> aKey = mKey->openSubKey(theBoard->getName());
	int aCount = aKey->readInt(COUNT_VALUE, 0);
	for (int i = 0; i < aCount; i++)
	{
		Ref<ScoreEntry> anEntry = theBoard->createEntry();
		loadEntry(aKey, i, anEntry);
		theBoard->add(anEntry);
	}
	theBoard->forgetLastAdded();
	theBoard->setState(Scoreboard::STATE_READY);
	return true;
}

// 0x469cd0 (pizzafrenzy)
bool Scoreboards::saveBoard(Scoreboard* theBoard)
{
	Ref<PersistKey> aKey = mKey->openSubKey(theBoard->getName());
	aKey->writeInt(COUNT_VALUE, theBoard->getCount());
	for (int i = 0; i < theBoard->getCount(); i++)
		saveEntry(aKey, i, theBoard->getEntry(i));
	return true;
}

// 0x4691e0 (pizzafrenzy)
Scoreboards* Scoreboards::instance()
{
	if (gScoreboards == nullptr)
		gScoreboards = new Scoreboards();
	return gScoreboards;
}

// 0x469250 (pizzafrenzy): the key is opened by the application's slot 0x98 (0x4bccf0 (pizzafrenzy)); the port's
// registry is the host's (see PersistKey.h)
bool Scoreboards::open(const std::string& thePublisher, const std::string& theGame)
{
	std::string aPath = thePublisher + "\\" + theGame + "\\scoreboards";
	mKey = new PersistKey(aPath, "scoreboards\\");
	return true;
}
