// Sexy::QuestBoard (the port's share of it): see the header
#include "QuestBoard.h"

#include "Bej3App.h"

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

// slot 85 (0x154), 0x6cda70 (bejeweled3): Board's reset (0x74a880), then the update counters at +0x3198 and +0x31a0 are 0. port: the rest of
// 0x6cda70 (the goal's text, the hypermixer flags at +0x3186 and +0x3187, +0x3194 and +0x319c at 7, +0x31a8 and the Quest's tutorial flags) is not read by
// any translated code
void QuestBoard::ResetGame()
{
	Board::ResetGame();
	mUpdatesPlayed = 0;
	mUpdatesRun = 0;
}

// slot 22 (0x58), 0x6dd8a0 (bejeweled3): Board's update (0x759170) first, then nothing more while a demonstration runs (+0x1414) or the board is
// paused (+0x12c4) or the app has the dialog 0x17 open. Otherwise: with the secret flag clear and the hold counter (+0xd1c) at 300 the Quest's end
// screen is started (0x6dd760 and a curve restart); the curve at +0x31b8 is copied to the widget at +0x2870's +0x148 while it runs; the counter
// at +0x31a0 goes up; the quest's clock (+0xd20, counting down from the value 0x6ca350 gave it) steps; the goal's slot 0xd8 runs; the two flags
// of the menu button (+0x65 and +0x66) are copied to the widget at +0x31b0; and the counter at +0x3198 goes up.
// port: the port's secret modes have no demonstration and no pause, so the first test is not made (the exe's +0x1414 and +0x12c4 are 0 in every game
// it plays). Of the rest, the end screen's start (needs the flag clear: every QuestBoard of the port has it set) and the clock are not translated: the
// clock (+0xd20) is only ever set by 0x6ca350, which slot 156 (0x6ca7e0) runs only when the secret flag is clear, so it is 0 in every secret mode and
// the block (which needs it above 0) never runs. The curve at +0x31b8 is never started (no function of the export names +0x31b8 but this), so
// its test (0x437f30: it is changing) is false. The goal's slot 0xd8 is ButterflyGoal's 0x6a9aa0 in Butterflies (the one secret mode with a goal: secret.cfg's
// Goal "Catch Butterflies"; Poker and Ice Storm have none): it adds 1 to a counter of the goal (+0x108) when the board's slot 103 (0x73c260) is
// false, and nothing in the export reads that counter, so the port keeps no goal object. The widget at +0x31b0 is made by 0x6ca5f0 (slot 83), which the
// port does not run, so there is none to copy to
void QuestBoard::Update()
{
	Board::Update();
	if (gApp->GetDialog(0x17) != nullptr)
		return;
	mUpdatesRun++;
	mUpdatesPlayed++;
}

// slot 234 (0x3a8), 0x6e5660 (bejeweled3): the goal's slot 0x2c when the board has a goal, then Board's step (0x767090). Butterflies' goal
// (ButterflyGoal, 0x86a90c) has a plain return there (0x401df0), and Poker and Ice Storm have no goal, so this is Board's step. (Ice Storm's own
// slot 234, 0x6e5850, runs 0x767090 itself and does not come here.)
void QuestBoard::UpdateTick()
{
	Board::UpdateTick();
}

// slot 211 (0x34c), 0x6c56a0 (bejeweled3): the goal's slot 0x44 when the board has one (ButterflyGoal's is a plain return, 0x6a9b90); with no goal, nothing.
// Board's 0x73ba10 returns at once as well, and Ice Storm's own 0x6e2250 takes over this slot
void QuestBoard::OnRunFound(const std::vector<Piece*>& thePieces)
{
	(void)thePieces;
}

}  // namespace Sexy
