// Sexy::QuestBoard (the port's share of it): see the header
#include "QuestBoard.h"

#include <cstdlib>

namespace Sexy {

int QuestBoard::SettingInt(const char* theName) const
{
	auto anIter = mSettings.find(theName);
	return anIter == mSettings.end() ? 0 : atoi(anIter->second.c_str());
}

double QuestBoard::SettingDouble(const char* theName) const
{
	auto anIter = mSettings.find(theName);
	return anIter == mSettings.end() ? 0.0 : atof(anIter->second.c_str());
}

std::vector<double> QuestBoard::SettingList(const char* theName) const
{
	std::vector<double> aValues;
	auto anIter = mSettings.find(theName);
	if (anIter == mSettings.end())
		return aValues;
	const std::string& aText = anIter->second;
	size_t aStart = 0;
	while (aStart <= aText.size()) {
		size_t anEnd = aText.find(',', aStart);
		if (anEnd == std::string::npos)
			anEnd = aText.size();
		std::string aPart = aText.substr(aStart, anEnd - aStart);
		size_t aFirst = aPart.find_first_not_of(" 	\"");
		if (aFirst != std::string::npos)
			aValues.push_back(atof(aPart.c_str() + aFirst));
		aStart = anEnd + 1;
	}
	return aValues;
}

// slot 100 on Board (0x74e410's call): the quest's own background. 0x74e410 reads the setting BackgroundIdx into +0x2864 and, for the boards
// made from a quest (not Classic's), has the background made with a step of 0 (0x749c20), so the index is the quest's and not the next one
void QuestBoard::ChooseBackground()
{
	mBackgroundIndex = SettingInt("BackgroundIdx");
	SetBackground(0);
}

// slot 231 (0x767e20 for the Quest boards, whose own slots 231 run it first): the game's step, then 0x759840, which the exe runs inside it
void QuestBoard::UpdateGame()
{
	Board::UpdateGame();
	UpdateMoveRecords();
}

}  // namespace Sexy
