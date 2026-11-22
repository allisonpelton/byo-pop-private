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
	// framework's parser gives "<setting> = <value>" as three words, which are read here as such
	Quest aQuest;
	DataToString(theParams.mElementVector[1], &aQuest.mName);
	aQuest.mSettings["Title"] = aQuest.mName;
	size_t i = 2;
	while (i < theParams.mElementVector.size()) {
		std::string aSetting;
		if (!DataToString(theParams.mElementVector[i], &aSetting))
			return Error("Bad setting");
		std::string aValue;
		if (i + 2 < theParams.mElementVector.size() && !theParams.mElementVector[i + 1]->mIsList) {
			std::string anEquals;
			DataToString(theParams.mElementVector[i + 1], &anEquals);
			if (anEquals == "=") {
				DataToString(theParams.mElementVector[i + 2], &aValue);
				i += 3;
				aQuest.mSettings[aSetting] = Unquote(aValue);
				continue;
			}
		}
		aQuest.mSettings[aSetting] = "";
		i++;
	}
	if (aQuest.mSettings.find("TitleText") == aQuest.mSettings.end())
		aQuest.mSettings["TitleText"] = aQuest.mSettings["Title"];
	mQuests.push_back(aQuest);
	return true;
}
