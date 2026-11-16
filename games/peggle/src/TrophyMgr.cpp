// TrophyMgr.cpp: see TrophyMgr.h.

#include "TrophyMgr.h"

#include "CharacterMgr.h"
#include "Common.h"
#include "ConfigParser.h"
#include "SexyAppBase.h"
#include "ThunderballApp.h"

using namespace Sexy;

static const char* const TROPHY_FILE = "levels/trophy.cfg";

// 0x471960
void TrophyMgr::Clear()
{
	mPages.clear();
	mTrophyById.clear();
}

// 0x4430b0
TrophyInfo* TrophyMgr::GetTrophyInfoById(int theId)
{
	std::map<int, TrophyInfoPtr>::iterator anItr = mTrophyById.find(theId);
	if (anItr == mTrophyById.end())
		return nullptr;
	return anItr->second.get();
}

// 0x468900: one "Key: value" line of a trophy
void TrophyMgr::ReadTrophyConfig(ConfigParser& theParser, TrophyInfo& theTrophy)
{
	StringParser& aValue = theParser.GetValParser();

	if (theParser.KeyIs("Levels"))
	{
		aValue.ReadStringList(theTrophy.mLevels, true);
		for (size_t i = 0; i < theTrophy.mLevels.size(); i++)
		{
			const std::string& aLevel = theTrophy.mLevels[i];
			// "rand..." and "pick" stand for a level the game chooses
			if (StrPrefixNoCase(aLevel.c_str(), "rand"))
				continue;
			if (sexy_stricmp(aLevel.c_str(), "pick") == 0)
				continue;
			if (!gSexyAppBase->FileExists("levels\\" + aLevel + ".dat"))
				theParser.Error("Level file not found: " + aLevel);
		}
	}
	else if (theParser.KeyIs("Opponents"))
	{
		std::vector<std::string> aNames;
		aValue.ReadStringList(aNames, true);
		for (size_t i = 0; i < aNames.size(); i++)
		{
			int aCharacter;
			if (sexy_stricmp(aNames[i].c_str(), "None") == 0)
				aCharacter = TROPHY_OPPONENT_NONE;
			else if (sexy_stricmp(aNames[i].c_str(), "Rand") == 0)
				aCharacter = TROPHY_OPPONENT_RANDOM;
			else
			{
				aCharacter = gThunderballApp->mCharacterMgr.GetCharacterIndex(aNames[i]);
				if (aCharacter < 0)
					theParser.Error("Character not found: " + aNames[i]);
			}
			theTrophy.mOpponents.push_back(std::make_pair(aCharacter, 0));
		}
	}
	else if (theParser.KeyIs("OpponentDifficulty"))
	{
		std::vector<int> aDifficulties;
		aValue.ReadIntList(aDifficulties, true);
		if (aDifficulties.size() != theTrophy.mOpponents.size())
			theParser.Error("OpponentDifficulty must have same number of entries as the number of opponents.  And be "
							"specified second.");
		for (size_t i = 0; i < aDifficulties.size(); i++)
			theTrophy.mOpponents[i].second = aDifficulties[i];
	}
	else if (theParser.KeyIs("Id"))
		theTrophy.mId = aValue.ReadInt();
	else if (theParser.KeyIs("Balls"))
		theTrophy.mBalls = aValue.ReadInt();
	else if (theParser.KeyIs("NumOrange"))
		theTrophy.mNumOrange = aValue.ReadInt();
	else if (theParser.KeyIs("Lives"))
		theTrophy.mLives = aValue.ReadInt();
	else if (theParser.KeyIs("ScoreReset"))
		theTrophy.mScoreReset = aValue.ReadBool();
	else if (theParser.KeyIs("ClearLevel"))
		theTrophy.mClearLevel = aValue.ReadBool();
	else if (theParser.KeyIs("ScoreReq"))
	{
		theTrophy.mScoreReq = aValue.ReadInt();
		if (theTrophy.mScoreReq > 0)
			theTrophy.mScoreReset = false;
	}
	else if (theParser.KeyIs("Desc"))
		aValue.ReadString(theTrophy.mDesc, false, true);
	else if (theParser.KeyIs("SmallDesc"))
		aValue.ReadString(theTrophy.mSmallDesc, false, true);
	else
		theParser.ErrorUnexpectedKey();
}

// 0x473e70
void TrophyMgr::ReadTrophyPage(ConfigParser& theParser)
{
	mPages.push_back(TrophyPage());
	size_t aPageIndex = mPages.size() - 1;
	mPages[aPageIndex].mName = theParser.mGroups.empty() ? std::string() : theParser.mGroups.back().mName;

	bool inTrophy = false;
	while (theParser.ReadNext())
	{
		TrophyPage& aPage = mPages[aPageIndex];
		if (theParser.mEvent == ConfigParser::EVENT_GROUP_BEGIN)
		{
			if (!theParser.GroupIs("Trophy", 2))
			{
				theParser.ErrorUnexpectedGroupType();
				continue;
			}
			inTrophy = true;
			TrophyInfoPtr aTrophy = std::make_shared<TrophyInfo>();
			aPage.mTrophies.push_back(aTrophy);
			aTrophy->mName = theParser.mGroups.back().mName;
		}
		else if (theParser.mEvent == ConfigParser::EVENT_KEY)
		{
			if (!inTrophy)
			{
				if (theParser.KeyIs("Desc"))
					theParser.GetValParser().ReadString(aPage.mDesc, false, true);
				else if (theParser.KeyIs("SmallDesc"))
					theParser.GetValParser().ReadString(aPage.mSmallDesc, false, true);
				else
					theParser.ErrorUnexpectedKey();
			}
			else
				ReadTrophyConfig(theParser, *aPage.mTrophies.back());
		}
		else
		{
			if (!inTrophy)
				return; // the page's own "}"
			inTrophy = false;
			TrophyInfoPtr aTrophy = aPage.mTrophies.back();
			if (aTrophy->mId == 0)
				theParser.Error("Must specifty trophy id.");
			if (!mTrophyById.insert(std::make_pair(aTrophy->mId, aTrophy)).second)
				theParser.Error("Id already exists: " + std::to_string(aTrophy->mId));
			if (aTrophy->mSmallDesc.empty())
				aTrophy->mSmallDesc = aPage.mSmallDesc;
		}
	}
}

// 0x474390
void TrophyMgr::ReadConfig(const std::string& theFileName)
{
	Clear();

	ConfigParser aParser(true);
	aParser.Open(theFileName, true);
	while (aParser.ReadNext())
	{
		if (aParser.mEvent == ConfigParser::EVENT_GROUP_BEGIN)
		{
			if (aParser.GroupIs("Page", 1))
				ReadTrophyPage(aParser);
			else
				aParser.ErrorUnexpectedGroupType();
		}
		else
			aParser.Error("Unexpected stuff");
	}
}

// 0x474550
void TrophyMgr::Load(const char* theFileName)
{
	ReadConfig(theFileName == nullptr ? TROPHY_FILE : theFileName);
}
