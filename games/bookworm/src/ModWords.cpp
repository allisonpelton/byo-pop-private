#include "ModWords.h"
#include "BookWormApp.h"
#include "Definitionator.h"
#include "platform/ModManager.h"
#include <algorithm>
#include <vector>

using namespace Sexy;

namespace
{

// The words one mod adds that the game's own list lacks, sorted, for its note and its fire tiles
struct ModWordList
{
	std::string				mModId;
	SexyString				mNote;
	bool					mHasFire;
	int						mFire;          // "fire", the mod's setting "fire" taking its place
	std::vector<SexyString>	mWords;
};

std::vector<ModWordList> gModWordLists;

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
		for (const std::string& aName : FileNames(aFeature["add"]))
		{
			std::vector<SexyString> aWords;
			theApp->LoadWordList(aName, NULL, &aWords);
			for (const SexyString& aWord : aWords)
			{
				if (theDictionary->find(aWord) == theDictionary->end())
					aList.mWords.push_back(aWord);
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

int Sexy::ModWordFire(const SexyString& theWord)
{
	for (const ModWordList& aList : gModWordLists)
	{
		if ((!aList.mHasFire) || (!std::binary_search(aList.mWords.begin(), aList.mWords.end(), theWord)))
			continue;
		Platform::JsonValue aSetting = Platform::ModManager::Get().GetLiveValue(aList.mModId, "fire");
		int aFire = aSetting.IsNumber() ? aSetting.AsInt(aList.mFire) : aList.mFire;
		return (aFire < 0) ? 0 : ((aFire > 9) ? 9 : aFire);
	}
	return 0;
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
