// port(mod): feature "words", word lists from mods (ModWords.h). Not part of the original game.

#include "ModWords.h"
#include "BookWormApp.h"
#include "Definitionator.h"
#include "ModFeatures.h"
#include "PakInterface.h"
#include "platform/ModManager.h"
#include <algorithm>
#include <cmath>
#include <vector>

using namespace Sexy;

namespace
{

// The words one mod adds that the game's own list lacks, sorted, for its note, effects and stats
struct ModWordList
{
	std::string				mModId;
	SexyString				mNote;
	bool					mHasFire;       // the words have effects (ModWordEffects)
	int						mFire;          // "fire", the default of the mod's setting "fire"
	std::vector<SexyString>	mReactions;     // Lex's comments
	SexyString				mStatsLabel;    // "stats"
	std::vector<SexyString>	mWords;
};

std::vector<ModWordList> gModWordLists;
int gMaxWordLength = 12;   // wordlist.txt keeps words of 3..12 letters (GAME_NOTES section 1)

// The mod's live setting, or theDefault without it
Platform::JsonValue Setting(const ModWordList& theList, const char* theSetting)
{
	return Platform::ModManager::Get().GetLiveValue(theList.mModId, theSetting);
}

bool SettingBool(const ModWordList& theList, const char* theSetting, bool theDefault)
{
	Platform::JsonValue aValue = Setting(theList, theSetting);
	return aValue.IsBool() ? aValue.AsBool(theDefault) : theDefault;
}

int SettingInt(const ModWordList& theList, const char* theSetting, int theDefault, int theMin, int theMax)
{
	Platform::JsonValue aValue = Setting(theList, theSetting);
	int anInt = aValue.IsNumber() ? aValue.AsInt(theDefault) : theDefault;
	return (anInt < theMin) ? theMin : ((anInt > theMax) ? theMax : anInt);
}

// The list holding theWord that has effects ("fire"), or null
const ModWordList* EffectsList(const SexyString& theWord)
{
	for (const ModWordList& aList : gModWordLists)
	{
		if ((aList.mHasFire) && (std::binary_search(aList.mWords.begin(), aList.mWords.end(), theWord)))
			return &aList;
	}
	return NULL;
}

// One comment a line (CR LF or LF), empty lines skipped
void LoadLines(const std::string& theFileName, std::vector<SexyString>* theLines)
{
	PFILE* aFile = p_fopen(theFileName.c_str(), "r");
	if (aFile == NULL)
		return;
	SexyString aLine;
	for (;;)
	{
		int aChar = p_fgetc(aFile);
		if ((aChar == EOF) || (aChar == '\n'))
		{
			while ((!aLine.empty()) && ((aLine.back() == '\r') || (aLine.back() == ' ')))
				aLine.pop_back();
			if (!aLine.empty())
				theLines->push_back(aLine);
			aLine.clear();
			if (aChar == EOF)
				break;
		}
		else
			aLine += (char)aChar;
	}
	p_fclose(aFile);
}

// A file name or a list of them
std::vector<std::string> FileNames(const Platform::JsonValue& theValue)
{
	std::vector<std::string> aNames;
	if (theValue.IsString())
		aNames.push_back(theValue.AsString());
	for (const Platform::JsonValue& anItem : theValue.Items())
	{
		if (anItem.IsString())
			aNames.push_back(anItem.AsString());
	}
	return aNames;
}

}

void Sexy::LoadModWords(BookWormApp* theApp, std::set<SexyString>* theDictionary)
{
	gModWordLists.clear();
	gMaxWordLength = 12;
	bool isLong = ModLongWords();
	// every mod's words against the game's list alone (a word two mods add is each one's), then merged
	std::vector<std::vector<SexyString> > aMerged;
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
	{
		if ((!aMod->mFeaturesJson.IsObject()) || (!aMod->mFeaturesJson.Has("words")))
			continue;
		const Platform::JsonValue& aFeature = aMod->mFeaturesJson["words"];
		ModWordList aList;
		aList.mModId = aMod->mId;
		aList.mNote = aFeature["note"].AsString("");
		aList.mHasFire = aFeature["fire"].IsNumber();
		aList.mFire = aFeature["fire"].AsInt(0);
		aList.mStatsLabel = aFeature["stats"].AsString("");
		for (const std::string& aName : FileNames(aFeature["reactions"]))
			LoadLines(aName, &aList.mReactions);
		for (const std::string& aName : FileNames(aFeature["add"]))
		{
			std::vector<SexyString> aWords;
			theApp->LoadWordList(aName, NULL, &aWords);
			for (const SexyString& aWord : aWords)
			{
				if ((aWord.size() > 12) && (!isLong))
					continue;
				if (theDictionary->find(aWord) == theDictionary->end())
				{
					aList.mWords.push_back(aWord);
					gMaxWordLength = std::max(gMaxWordLength, (int)aWord.size());
				}
			}
		}
		std::sort(aList.mWords.begin(), aList.mWords.end());
		aList.mWords.erase(std::unique(aList.mWords.begin(), aList.mWords.end()), aList.mWords.end());
		if ((aList.mNote.empty()) && (!aList.mHasFire))
		{
			aMerged.push_back(std::vector<SexyString>());
			aMerged.back().swap(aList.mWords);
		}
		else if (!aList.mWords.empty())
			gModWordLists.push_back(aList);
	}
	for (const std::vector<SexyString>& aWords : aMerged)
		theDictionary->insert(aWords.begin(), aWords.end());
	for (const ModWordList& aList : gModWordLists)
		theDictionary->insert(aList.mWords.begin(), aList.mWords.end());
}

int Sexy::ModWordMaxLength()
{
	return gMaxWordLength;
}

bool Sexy::ModLongWords()
{
	return ModFeatureEnabled("longWords");
}

void Sexy::LoadModDefinitions(Definitionator* theDefinitionator)
{
	// the lowest priority first, so a higher one's definition replaces it
	std::vector<const Platform::ModInfo*> aMods = Platform::ModManager::Get().GetEnabledMods();
	for (std::vector<const Platform::ModInfo*>::reverse_iterator anItr = aMods.rbegin(); anItr != aMods.rend(); ++anItr)
	{
		const Platform::ModInfo* aMod = *anItr;
		if ((!aMod->mFeaturesJson.IsObject()) || (!aMod->mFeaturesJson.Has("words")))
			continue;
		for (const std::string& aName : FileNames(aMod->mFeaturesJson["words"]["definitions"]))
			theDefinitionator->Load(aName);
	}
}

std::string Sexy::ModWordLookupURL(const SexyString& theWord)
{
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
	{
		if ((!aMod->mFeaturesJson.IsObject()) || (!aMod->mFeaturesJson.Has("words")))
			continue;
		const Platform::JsonValue& aLookup = aMod->mFeaturesJson["words"]["lookup"];
		std::string aURL;
		if (aLookup.IsString())
			aURL = aLookup.AsString();
		else if (aLookup.IsObject())
		{
			Platform::JsonValue aChoice = Platform::ModManager::Get().GetLiveValue(aMod->mId, "lookup");
			aURL = aLookup[aChoice.AsString("")].AsString("");
		}
		else
			continue;

		std::string aWord;
		for (char c : theWord)
			aWord += ((c >= 'A') && (c <= 'Z')) ? (char)(c - 'A' + 'a') : c;
		std::string::size_type aPos = aURL.find("{word}");
		if (aPos != std::string::npos)
			aURL.replace(aPos, 6, aWord);
		return aURL;
	}
	return std::string();
}

bool Sexy::ModWordGetEffects(const SexyString& theWord, ModWordEffects* theEffects)
{
	const ModWordList* aList = EffectsList(theWord);
	if (aList == NULL)
		return false;
	Platform::JsonValue aPoints = Setting(*aList, "points");
	theEffects->mPoints = aPoints.IsNumber() ? aPoints.AsNumber(1.0) : 1.0;
	theEffects->mEveryTile = SettingBool(*aList, "everyTile", false);
	if (Setting(*aList, "fireCount").AsString("fixed") == "length")
	{
		int aLetters = SettingInt(*aList, "letters", 2, 1, 15);
		theEffects->mFire = std::max(1, std::min(9, (int)theWord.size() / aLetters));
	}
	else
		theEffects->mFire = SettingInt(*aList, "fire", aList->mFire, 0, 9);
	std::string aBonus = Setting(*aList, "bonus").AsString("keep");
	theEffects->mBonus = (aBonus == "remove") ? MODWORD_BONUS_REMOVE :
		((aBonus == "none") ? MODWORD_BONUS_NONE : MODWORD_BONUS_KEEP);
	theEffects->mComment = SettingBool(*aList, "note", true);
	return true;
}

int Sexy::ModWordScore(const SexyString& theWord, int theScore)
{
	if (gModWordLists.empty())
		return theScore;
	ModWordEffects anEffects;
	if ((!ModWordGetEffects(theWord, &anEffects)) || (anEffects.mPoints == 1.0))
		return theScore;
	// in tens, as every score is
	return (int)std::floor((double)theScore * anEffects.mPoints / 10.0 + 0.5) * 10;
}

SexyString Sexy::ModWordReaction(const SexyString& theWord, int theCount)
{
	const ModWordList* aList = EffectsList(theWord);
	if ((aList == NULL) || (aList->mReactions.empty()) || (!SettingBool(*aList, "note", true)))
		return SexyString();
	// the same word and count give the same comment (an undo and the word again), the next word another
	unsigned int aHash = 0;
	for (char c : theWord)
		aHash = aHash * 31 + (unsigned char)c;
	unsigned int aSize = (unsigned int)aList->mReactions.size();
	return aList->mReactions[(aHash + (unsigned int)theCount * 7) % aSize];
}

bool Sexy::ModWordCounted(const SexyString& theWord)
{
	const ModWordList* aList = EffectsList(theWord);
	return (aList != NULL) && (!aList->mStatsLabel.empty());
}

SexyString Sexy::ModWordStatsLabel()
{
	for (const ModWordList& aList : gModWordLists)
	{
		if ((aList.mHasFire) && (!aList.mStatsLabel.empty()))
			return aList.mStatsLabel;
	}
	return SexyString();
}

SexyString Sexy::ModWordStatsValueName()
{
	for (const ModWordList& aList : gModWordLists)
	{
		if ((aList.mHasFire) && (!aList.mStatsLabel.empty()))
			return "ModWords-" + aList.mModId;
	}
	return SexyString();
}

SexyString Sexy::ModWordNote(const SexyString& theWord)
{
	for (const ModWordList& aList : gModWordLists)
	{
		if ((aList.mNote.empty()) || (!std::binary_search(aList.mWords.begin(), aList.mWords.end(), theWord)))
			continue;
		Platform::JsonValue aSetting = Platform::ModManager::Get().GetLiveValue(aList.mModId, "note");
		return ((!aSetting.IsBool()) || (aSetting.AsBool(true))) ? aList.mNote : SexyString();
	}
	return SexyString();
}
