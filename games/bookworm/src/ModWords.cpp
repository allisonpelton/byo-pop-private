#include "ModWords.h"
#include "BookWormApp.h"
#include "Definitionator.h"
#include "platform/ModManager.h"
#include <algorithm>
#include <vector>

using namespace Sexy;

namespace
{

// The words one mod added, sorted, for its note
struct NoteList
{
	std::string				mModId;
	SexyString				mNote;
	std::vector<SexyString>	mWords;
};

std::vector<NoteList> gNoteLists;

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
	gNoteLists.clear();
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
	{
		if ((!aMod->mFeaturesJson.IsObject()) || (!aMod->mFeaturesJson.Has("words")))
			continue;
		const Platform::JsonValue& aFeature = aMod->mFeaturesJson["words"];
		NoteList aList;
		aList.mModId = aMod->mId;
		aList.mNote = aFeature["note"].AsString("");
		for (const std::string& aName : FileNames(aFeature["add"]))
		{
			std::vector<SexyString> aWords;
			theApp->LoadWordList(aName, NULL, &aWords);
			for (const SexyString& aWord : aWords)
			{
				if (theDictionary->insert(aWord).second)
					aList.mWords.push_back(aWord);
			}
		}
		if ((!aList.mNote.empty()) && (!aList.mWords.empty()))
		{
			std::sort(aList.mWords.begin(), aList.mWords.end());
			gNoteLists.push_back(aList);
		}
	}
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

SexyString Sexy::ModWordNote(const SexyString& theWord)
{
	for (const NoteList& aList : gNoteLists)
	{
		if (!std::binary_search(aList.mWords.begin(), aList.mWords.end(), theWord))
			continue;
		Platform::JsonValue aSetting = Platform::ModManager::Get().GetLiveValue(aList.mModId, "note");
		return ((!aSetting.IsBool()) || (aSetting.AsBool(true))) ? aList.mNote : SexyString();
	}
	return SexyString();
}
