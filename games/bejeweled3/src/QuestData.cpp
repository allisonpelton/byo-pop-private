// Sexy::QuestDataParser: see the header.
#include "QuestData.h"

#include "Common.h"

using namespace Sexy;

QuestDataParser::QuestDataParser()
{
	// 0x54d0b0 (the original's DescParser): commands end at semicolons
	mCmdSep = CMDSEP_SEMICOLON;
}

bool QuestDataParser::HandleCommand(const ListDataElement& theParams)
{
	if (theParams.mElementVector.empty())
		return Error("Unknown command");
	std::string aCommand;
	if (!DataToString(theParams.mElementVector[0], &aCommand))
		return Error("Unknown command");
	if (sexystricmp(aCommand.c_str(), "Quest") != 0)
		return Error("Unknown command: " + aCommand);
	if (theParams.mElementVector.size() <= 2)
		return Error("Not enough params");

	// port: the original stores each word after the name with the value it is set to (0x54d390, 0x4035d0); this
	// framework's parser gives "<setting> = <value>" as three words, which are read here as such. A setting that the quest gives twice keeps its FIRST
	// value: the exe puts each pair in its map with std::map::insert (0x403740, which finds the key's place and does not replace a value that is there), so
	// quest.cfg's Alchemy and Wall Blast ("Challenge = "Time Limit"" and then "Challenge = "No Lose"") are Time Limit quests
	Quest aQuest;
	DataToString(theParams.mElementVector[1], &aQuest.mName);
	aQuest.mSettings.insert({ "Title", aQuest.mName });
	size_t i = 2;
	while (i < theParams.mElementVector.size()) {
		std::string aSetting;
		if (!DataToString(theParams.mElementVector[i], &aSetting))
			return Error("Bad setting");
		std::string aValue;
		// port: two spellings the shipped settings use that this framework's parser splits differently from the original's: "Name= value" (the
		// equals sign sticks to the name: the next word is the value) and "Name =\"value\"" (the equals sign sticks to the value's quote: the word is the
		// equals sign and the quoted text). secret.cfg's Diamond Mine has both (Artifact10, Artifact21, Grids)
		if (!aSetting.empty() && aSetting.back() == '=' && i + 1 < theParams.mElementVector.size() && !theParams.mElementVector[i + 1]->mIsList) {
			DataToString(theParams.mElementVector[i + 1], &aValue);
			aQuest.mSettings.insert({ aSetting.substr(0, aSetting.size() - 1), Unquote(aValue) });
			i += 2;
			continue;
		}
		if (i + 1 < theParams.mElementVector.size() && !theParams.mElementVector[i + 1]->mIsList) {
			std::string aGlued;
			DataToString(theParams.mElementVector[i + 1], &aGlued);
			if (aGlued.size() > 1 && aGlued[0] == '=' && aGlued[1] == '"') {
				aQuest.mSettings.insert({ aSetting, Unquote(aGlued.substr(1)) });
				i += 2;
				continue;
			}
		}
		if (i + 2 < theParams.mElementVector.size() && !theParams.mElementVector[i + 1]->mIsList) {
			std::string anEquals;
			DataToString(theParams.mElementVector[i + 1], &anEquals);
			if (anEquals == "=") {
				DataToString(theParams.mElementVector[i + 2], &aValue);
				i += 3;
				aQuest.mSettings.insert({ aSetting, Unquote(aValue) });
				continue;
			}
		}
		aQuest.mSettings.insert({ aSetting, std::string() });
		i++;
	}
	if (aQuest.mSettings.find("TitleText") == aQuest.mSettings.end())
		aQuest.mSettings.insert({ "TitleText", aQuest.mSettings["Title"] });
	mQuests.push_back(aQuest);
	return true;
}
