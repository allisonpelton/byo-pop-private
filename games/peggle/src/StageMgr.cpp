// StageMgr.cpp: see StageMgr.h.

#include "StageMgr.h"

#include "ConfigParser.h"
#include "PlayerInfo.h"
#include "SexyAppBase.h"
#include "MTRand.h"

using namespace Sexy;

static const char* const STAGES_FILE = "levels/stages.cfg";

// 0x43d7d0
LevelInfo* StageInfo::GetLevelInfo(int theLevel)
{
	if (theLevel >= 0 && theLevel < (int)mLevels.size())
		return mLevels[theLevel].get();
	return nullptr;
}

// 0x43d820
StageInfo* StageMgr::GetStageInfo(int theStage)
{
	if (theStage >= 0 && theStage < (int)mStages.size())
		return mStages[theStage].get();
	return nullptr;
}

// 0x442ff0
LevelInfo* StageMgr::FindLevel(const std::string& theFileName)
{
	std::map<std::string, LevelInfoPtr, StringLessNoCase>::iterator anItr = mLevelsByName.find(theFileName);
	if (anItr == mLevelsByName.end())
		return nullptr;
	return anItr->second.get();
}

// 0x43d890
void StageMgr::ResetLastUseTimes()
{
	mUseCounter = 0;
	for (size_t aStage = 0; aStage < mStages.size(); aStage++)
	{
		for (size_t aLevel = 0; aLevel < mStages[aStage]->mLevels.size(); aLevel++)
			mStages[aStage]->mLevels[aLevel]->mLastUseTime = LEVEL_NEVER_USED;
	}
}

// 0x437620
void StageMgr::MarkLastUseTime(LevelInfo& theLevel)
{
	mUseCounter++;
	theLevel.mLastUseTime = mUseCounter;
}

// 0x45f460
void StageMgr::Clear()
{
	mUseCounter = 0;
	mStages.clear();
	mLevelsByName.clear();
	mTips.clear();
}

// 0x467d50: one "Key: value" line inside a stage
void StageMgr::ReadStageConfig(ConfigParser& theParser, StageInfo& theStage)
{
	StringParser& aValue = theParser.GetValParser();

	if (theParser.KeyIs("Level"))
	{
		// "Level: FileName[, Name]"
		LevelInfoPtr aLevel = std::make_shared<LevelInfo>();
		theStage.mLevels.push_back(aLevel);
		aValue.ReadString(aLevel->mFileName, true, true);
		aLevel->mLevelIndex = (int)theStage.mLevels.size() - 1;
		aLevel->mStageIndex = (int)mStages.size() - 1;
		if (aValue.CheckNextChar(','))
			aValue.ReadString(aLevel->mName, true, true);
		else
			aLevel->mName = aLevel->mFileName;
		if (!gSexyAppBase->FileExists("levels\\" + aLevel->mFileName + ".dat"))
			theParser.Error("Level file not found: " + aLevel->mFileName);
		mLevelsByName[aLevel->mFileName] = aLevel;
	}
	else if (theParser.KeyIs("Dialog"))
	{
		// "Dialog: levelnumber, text[, title]"
		int aLevelIndex = aValue.ReadInt() - 1;
		aValue.CheckNextChar(',');
		if (aLevelIndex < 0 || aLevelIndex >= (int)theStage.mLevels.size())
			theParser.Error("Invalid level num.");
		LevelInfo* aLevel = theStage.GetLevelInfo(aLevelIndex);
		aValue.ReadHTMLString(aLevel->mDialogText);
		if (aValue.CheckNextChar(','))
			aValue.ReadString(aLevel->mDialogTitle, true, true);
	}
	else if (theParser.KeyIs("StageDialog"))
	{
		// "StageDialog: page, text[, number]": every line of a page is added to the page's text
		int aPage = aValue.ReadInt() - 1;
		aValue.CheckNextChar(',');
		if (aPage < 0 || aPage >= StageInfo::NUM_TEXT_PAGES)
			theParser.Error("Invalid text num.");
		std::vector<StageText>& aTexts = theStage.mTexts[aPage];
		if (aTexts.empty())
			aTexts.push_back(StageText());
		std::string aText;
		aValue.ReadHTMLString(aText);
		aTexts[0].mText += aText;
		if (aValue.CheckNextChar(','))
			aTexts[0].mValue = aValue.ReadInt(-1);
	}
	else if (theParser.KeyIs("Credit"))
	{
		// "Credit: page, text[, number]": a line of the credits shown after the page
		int aPage = aValue.ReadInt();
		aValue.CheckNextChar(',');
		if (aPage - 1 < 0 || aPage > StageInfo::NUM_TEXT_PAGES)
			theParser.Error("Invalid text num.");
		std::vector<StageText>& aTexts = theStage.mTexts[aPage - 1];
		aTexts.push_back(StageText());
		aValue.ReadHTMLString(aTexts.back().mText);
		if (aValue.CheckNextChar(','))
			aTexts.back().mValue = aValue.ReadInt(-1);
	}
	else
		theParser.ErrorUnexpectedKey();
}

// 0x468300: one "Key: value" line outside the stages
void StageMgr::ReadKeyVal(ConfigParser& theParser)
{
	StringParser& aValue = theParser.GetValParser();

	if (theParser.KeyIs("ExcludeRandStages"))
	{
		// the numbers of stages whose levels are never picked at random
		while (!aValue.AtEnd())
		{
			int aStageNumber = aValue.ReadInt();
			StageInfo* aStage = GetStageInfo(aStageNumber - 1);
			if (aStage == nullptr)
			{
				char aMessage[64];
				snprintf(aMessage, sizeof(aMessage), "Stage not found: %d", aStageNumber - 1);
				theParser.Error(aMessage);
			}
			for (size_t i = 0; i < aStage->mLevels.size(); i++)
				aStage->mLevels[i]->mExcludedFromRandom = true;
			if (!aValue.CheckNextChar(',') || aValue.AtEnd())
				break;
		}
	}
	else if (theParser.KeyIs("ExcludeRandLevels") || theParser.KeyIs("IncludeRandLevels"))
	{
		bool isExcluding = theParser.KeyIs("ExcludeRandLevels");
		while (!aValue.AtEnd())
		{
			std::string aName;
			aValue.ReadString(aName, true, true);
			LevelInfo* aLevel = FindLevel(aName);
			if (aLevel == nullptr)
				theParser.Error("Level not found: " + aName);
			aLevel->mExcludedFromRandom = isExcluding;
			if (!aValue.CheckNextChar(','))
				break;
		}
	}
	else if (theParser.KeyIs("Tip"))
	{
		mTips.push_back(std::string());
		aValue.ReadString(mTips.back(), false, true);
	}
	else
		theParser.ErrorUnexpectedKey();
}

// 0x4685f0
void StageMgr::ReadConfig(const std::string& theFileName)
{
	Clear();

	ConfigParser aParser(true);
	aParser.Open(theFileName, true);
	bool inStage = false;
	while (aParser.ReadNext())
	{
		if (aParser.mEvent == ConfigParser::EVENT_GROUP_BEGIN)
		{
			if (!aParser.GroupIs("Stage", 1))
			{
				aParser.ErrorUnexpectedGroupType();
				continue;
			}
			inStage = true;
			mStages.push_back(std::make_shared<StageInfo>());
		}
		else if (aParser.mEvent == ConfigParser::EVENT_KEY)
		{
			if (!inStage)
				ReadKeyVal(aParser);
			else
				ReadStageConfig(aParser, *mStages.back());
		}
		else
			inStage = false;
	}
}

// 0x468830
void StageMgr::Load(const char* theFileName)
{
	ReadConfig(theFileName == nullptr ? STAGES_FILE : theFileName);
}

// 0x459060 (the original first lowers the limits to what an unregistered game may play: a registered game, the only kind here,
// is not held back)
void StageMgr::GetRandomLevel(int theMaxStage, int theMaxLevel, int* theStage, int* theLevel, bool theSkipExcluded, int theMinGap,
							  bool theRetry)
{
	std::vector<LevelInfo*> aChoices;
	for (int aStage = 0; aStage < (int)mStages.size(); aStage++)
	{
		StageInfo& aStageInfo = *mStages[aStage];
		for (int aLevel = 0; aLevel < (int)aStageInfo.mLevels.size(); aLevel++)
		{
			LevelInfo* aLevelInfo = aStageInfo.mLevels[aLevel].get();
			if ((aStage == *theStage && aLevel == *theLevel) || theMaxStage < aStage || (aStage == theMaxStage && theMaxLevel < aLevel) ||
				(theSkipExcluded && aLevelInfo->mExcludedFromRandom) || (0 < theMinGap && mUseCounter - aLevelInfo->mLastUseTime < theMinGap))
				continue;
			aChoices.push_back(aLevelInfo);
		}
	}
	if (aChoices.empty())
	{
		if (theRetry)
		{
			ResetLastUseTimes();
			GetRandomLevel(theMaxStage, theMaxLevel, theStage, theLevel, theSkipExcluded, theMinGap, false);
		}
		return;
	}
	LevelInfo* aPick = aChoices[Rand() % (int)aChoices.size()];
	if (0 < theMinGap)
		MarkLastUseTime(*aPick);
	*theStage = aPick->mStageIndex;
	*theLevel = aPick->mLevelIndex;
}

// 0x4592b0 and 0x4592e0
LevelInfo* StageMgr::GetRandomLevel(const PlayerInfo* thePlayer, bool theSkipExcluded, int theMinGap)
{
	int aStage = -1;
	int aLevel = -1;
	GetRandomLevel(thePlayer->mMaxStage, thePlayer->mMaxLevel, &aStage, &aLevel, theSkipExcluded, theMinGap, true);
	StageInfo* aStageInfo = GetStageInfo(aStage);
	if (aStageInfo == nullptr)
		return nullptr;
	return aStageInfo->GetLevelInfo(aLevel);
}
