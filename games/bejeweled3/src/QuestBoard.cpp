// Sexy::QuestBoard (the port's share of it): see the header
#include "QuestBoard.h"

#include "Bej3App.h"
#include "HintDialog.h"
#include "Profile.h"

#include <SDL.h>

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

// slot 85 (0x154), 0x6cda70 (bejeweled3): Board's reset (0x74a880), then the update counters at +0x3198 and +0x31a0 are 0, the secret flag is copied to the
// flags that say whether the board scores and records high scores (+0x3187 and +0x31ac; 0x6ddc70 sets them again from the quest's settings), and the clock
// of the level's end (+0xd20) is 0. port: the rest of 0x6cda70 (the goal's text, the hypermixer flag at +0x3186, +0x3194 and +0x319c at 7, +0x31a8, the
// reading of PowerGemThreshold and the Quest's tutorial flags) is not read by any translated code
void QuestBoard::ResetGame()
{
	Board::ResetGame();
	mUpdatesPlayed = 0;
	mUpdatesRun = 0;
	mQuestClock = 0;
	mScoresPoints = mIsSecretMode;
	mRecordsHighScores = mIsSecretMode;
	mQuestWon = false;
}

QuestBoard::~QuestBoard()
{
	delete mGoal;
}

// the text of a setting: the quotes of a text in quotes are taken off and the letters made upper case
std::string QuestBoard::SettingText(const char* theName) const
{
	auto anIter = mSettings.find(theName);
	if (anIter == mSettings.end())
		return std::string();
	std::string aText = anIter->second;
	if (aText.size() >= 2 && aText.front() == '"' && aText.back() == '"')
		aText = aText.substr(1, aText.size() - 2);
	for (char& aChar : aText)
		aChar = (char)toupper((unsigned char)aChar);
	return aText;
}

// 0x6ddc70 (bejeweled3), from "Goal" to the call of slot 52: the goal that the quest's setting names is made (MakeQuestGoal); the one before it goes
void QuestBoard::CreateGoal()
{
	delete mGoal;
	mGoal = MakeQuestGoal(SettingText("Goal"), this);
}

// slot 135 (0x21c), 0x6ddc70 (bejeweled3): the update counter (+0x3198) is 0, the board scores and records high scores as a secret mode does (+0x3187 and
// +0x31ac are the secret flag), the setting RecordHighScores TRUE puts +0x31ac up; the goal is made (CreateGoal), Board's new game (0x74e410) deals the
// grid, and the goal reads its settings (slot 53, Load). port: ColorCount (slot 261), HighScoreBase, HighScoreIncr, WantPointComplements, NeverAllowCascades
// (+0x3228), ShowAutohints (+0x4dc), the description text and HyperMixers (+0x3186), and the board's slot 84 (0x150) after the goal's load are not read by
// any translated code
void QuestBoard::NewGame()
{
	mUpdatesPlayed = 0;
	mScoresPoints = mIsSecretMode;
	mRecordsHighScores = mIsSecretMode;
	if (SettingText("RecordHighScores") == "TRUE")
		mRecordsHighScores = true;
	CreateGoal();
	Board::NewGame();
	if (mGoal != nullptr)
		mGoal->Load();
	// the exe's slot 233 (0x6ded60) makes the end dialog only for a board that records high scores: a quest that does not has none
	if (!mRecordsHighScores)
		mEndScreenShown = true;
}

// slot 154 (0x268): the goal's slot 22 (0x58) when there is one, else Board's (0x73bf70); a secret mode's is 0
int QuestBoard::GetLevelMax() const
{
	if (mIsSecretMode)
		return 0;
	if (mGoal != nullptr)
		return mGoal->GetLevelMax();
	return Board::GetLevelMax();
}

// slot 143 (0x23c) and slots 176 and 142: a piece is cleared or taken out: the goal's slots 40 (0xa0), 18 (0x48) and 39 (0x9c) when the board has one
void QuestBoard::OnPieceCleared(Piece* thePiece)
{
	if (mGoal != nullptr)
		mGoal->OnPieceCleared(thePiece);
}

bool QuestBoard::OnPieceRemoving(Piece* thePiece)
{
	return mGoal != nullptr && mGoal->OnPieceRemoving(thePiece);
}

// slot 152 (0x6cf290, bejeweled3): the points of a match are added (Board's 0x74df80) only when the board scores (+0x3187) or its goal says so (+0x10c)
void QuestBoard::AddPoints(int thePoints, int theMoveId, bool theCountsForProgress, bool theUsesMultiplier)
{
	if (!mScoresPoints && (mGoal == nullptr || !mGoal->mScores))
		return;
	Board::AddPoints(thePoints, theMoveId, theCountsForProgress, theUsesMultiplier);
}

// slot 156 (0x6ca7e0) with 0x6ca350 (bejeweled3): the level is full. A secret mode has no level (the slot does nothing). A quest, when the fill is not held
// (+0x2ef8), the level is played (+0x3185, which 0x6cda70 sets) and no clock runs: the game's flag (+0xd8c) is set and the clock starts at +0x318c. A board
// that records high scores hands its score in (0x6c7c60). port: the text of the finished level (0x7411f0, slot 160) and the music events "Break", "Play" and
// "Wait" are left out
void QuestBoard::OnLevelFull()
{
	if (mIsSecretMode)
		return;
	if (mFillHeld == 0 && mQuestClock == 0) {
		mGameOver = true;
		mQuestClock = mQuestClockStart;
		if (mRecordsHighScores)
			SubmitScore(mScorePoints);   // 0x6c7c60 and the leaderboard's call
	}
}

// slot 161 (0x284), 0x6d8e30 (bejeweled3): the game ends, unless the goal's slot 10 says it may not or the level is full (slot 217: false in a secret mode). A board
// that records high scores hands its score in (0x6ca240: the wait at +0x307c) and the game ends (0x7602c0). port: for a board that records high scores
// Board's NoMoreMoves does both; the "Wait" music event when the game's end counter is 0 or less is left out
void QuestBoard::NoMoreMoves(bool theAnnounce)
{
	if (mGoal != nullptr && !mGoal->CanEndGame())
		return;
	if (!mIsSecretMode && IsLevelFull())
		return;
	if (mRecordsHighScores) {
		Board::NoMoreMoves(theAnnounce);
		return;
	}
	EndGame();
}

// slot 232 (0x6d8e20, a jump to Board's 0x756730): Board's for the secret modes. A quest goes back to the quest map (the exe's 0x756730 has the other branch for a
// Quest game, which 0x76c500 runs), and the board goes
void QuestBoard::LeaveToMenu()
{
	if (mIsSecretMode) {
		Board::LeaveToMenu();
		return;
	}
	gApp->ShowQuestMenu(true);
}

// 0x6dd8a0 (bejeweled3), the quest's clock (it is above 0). At its start (200) it counts down once the board is at rest. At 198, with the board at rest and the
// level bar as full as it is meant to be (slot 188 equals +0xd68), the win is told (0x76ab60, a sound, the dialog of string 0x1a1, and 0x6dd760: the quest map is
// shown over the board). Else it counts down (above 50, or with the effects list at +0x26b8 empty); at 100 slot 162 (the fanfare) runs with the menu's quest flag put
// to done for its call, and at 0 slot 233 (0x6ded60) ends the level. port: the fanfare and the sound are left out, the effects list is not made, and the quest is marked
// done in the profile at the win (see MarkQuestDone)
void QuestBoard::StepQuestClock()
{
	if (mQuestClock > mQuestClockWait - 1) {
		if (IsSettled())
			mQuestClock--;
	}
	else if (mQuestClock == mQuestClockWait - QUEST_CLOCK_WIN_BELOW_WAIT) {
		if (IsSettled() && GetBarTarget() == mLevelBar) {
			mQuestClock--;
			MarkQuestDone();
			gApp->ShowQuestMenu(false);   // 0x6dd760
		}
	}
	else {
		mQuestClock--;
		if (mQuestClock == QUEST_CLOCK_FANFARE) {
			// slot 162 (0x288), the fanfare: not translated
		}
		if (mQuestClock == 0) {
			// slot 233 (0x6ded60), the level's end: +0x307c counts down; the goal's slot 12 (0x30) runs; a quest that does not record high scores goes to 0x6ded00,
			// which shows the quest map if it is not up (the win's call of 0x6dd760 has it up already). port: the board stays under the quest map until the map starts
			// another quest or leaves, which deletes it
			mEndDialogWait--;
			if (mEndDialogWait <= 0 && !gApp->IsQuestMenuVisible())
				gApp->ShowQuestMenu(false);
		}
	}
}

// the win (0x684380, run by the quest map's update when the countdown that 0x67ed00 started has run out; the countdown is 0x118 updates): a quest that is not yet done
// has its flag put (the profile's +0x158 plus 8 for the section and the quest's place in it), the profile is saved (0x71bb10) and the quest's completion is logged
void QuestBoard::MarkQuestDone()
{
	mQuestWon = true;
	if (mQuestIndex < 0 || gApp->mProfile == nullptr)
		return;
	const int aSection = mQuestIndex / Profile::FLAG_ROW_SIZE;
	const int aPlace = mQuestIndex % Profile::FLAG_ROW_SIZE;
	if (aSection >= Profile::FLAG_ROW_COUNT || gApp->mProfile->mQuestsDone[aSection][aPlace] != 0)
		return;
	gApp->mProfile->mQuestsDone[aSection][aPlace] = 1;
	gApp->mProfile->SavePlayer();
	SDL_Log("QuestComplete quest=%d", mQuestIndex);
}

// slot 22 (0x58), 0x6dd8a0 (bejeweled3): Board's update (0x759170) first, then nothing more while a demonstration runs (+0x1414) or the board is
// paused (+0x12c4) or the app has the dialog 0x17 open. Otherwise: with the secret flag clear and the hold counter (+0xd1c) at 300 the quest map is shown
// again (0x6dd760: a lost quest); the curve at +0x31b8 is copied to the widget at +0x2870's +0x148 while it runs; the counter at +0x31a0 goes up; the quest's
// clock (+0xd20) steps (StepQuestClock); the goal's slot 0xd8 runs; the two flags of the menu button (+0x65 and +0x66) are copied to the widget at +0x31b0; and the
// counter at +0x3198 goes up.
// port: the port has no demonstration and no pause, so the first test is not made. The curve at +0x31b8 is never started (no function of the export names it but
// this), so its test (0x437f30: it is changing) is false. The widget at +0x31b0 is made by 0x6ca5f0 (slot 83: a tutorial's helper that the port does not make), so there
// is none to copy to. A lost quest's end screen is not made (see NewGame), and the map is shown over the board at 300 as in the exe
void QuestBoard::Update()
{
	Board::Update();
	if (gApp->GetDialog(0x17) != nullptr)
		return;
	if (!mIsSecretMode && mGameOverTicks == QUEST_LOST_LEAVE_TICKS)
		gApp->ShowQuestMenu(false);   // 0x6dd760
	mUpdatesRun++;
	if (mQuestClock > 0)
		StepQuestClock();
	if (mGoal != nullptr)
		mGoal->Update();
	mUpdatesPlayed++;
}

// slot 234 (0x3a8), 0x6e5660 (bejeweled3): the goal's slot 11 (0x2c) when the board has a goal, then Board's step (0x767090). Butterflies' goal
// (ButterflyGoal, 0x86a90c) has a plain return there (0x401df0), and Poker and Ice Storm have no goal. (Ice Storm's own slot 234, 0x6e5850, runs 0x767090 itself and does
// not come here.)
void QuestBoard::UpdateTick()
{
	if (mGoal != nullptr)
		mGoal->UpdateTick();
	Board::UpdateTick();
}

// slot 211 (0x34c), 0x6c56a0 (bejeweled3): the goal's slot 17 (0x44) when the board has one (ButterflyGoal's is a plain return, 0x6a9b90); with no goal, nothing.
// Board's 0x73ba10 returns at once as well, and Ice Storm's own 0x6e2250 takes over this slot
void QuestBoard::OnRunFound(const std::vector<Piece*>& thePieces)
{
	if (mGoal != nullptr)
		mGoal->OnRunFound(thePieces);
}

// slot 142 (0x238), 0x6c63c0 (bejeweled3): the goal's slot 39 (0x9c) with the piece and the flag, then Board's 0x73d2b0 (which calls slot 143)
void QuestBoard::OnPieceClearing(Piece* thePiece, bool theFlag)
{
	if (mGoal != nullptr)
		mGoal->OnPieceClearing(thePiece, theFlag);
}

// slot 197 (0x314), 0x6c4b80 (bejeweled3): the goal's slot 16 (0x40) with the cell
void QuestBoard::OnCellHit(int theCol, int theRow)
{
	if (mGoal != nullptr)
		mGoal->OnCellHit(theCol, theRow);
}

// slot 217 (0x364), 0x6c5460 (bejeweled3): false in a secret mode; else the goal's slot 26 (0x68) when there is a goal, else Board's 0x73b810
bool QuestBoard::IsLevelFull() const
{
	if (mIsSecretMode)
		return false;
	if (mGoal != nullptr)
		return mGoal->IsLevelFull();
	return Board::IsLevelFull();
}

void QuestBoard::DrawBehindPieces(Graphics* g)
{
	if (mGoal != nullptr)
		mGoal->DrawBehindPieces(g);
}

void QuestBoard::DrawModeOverlay(Graphics* g)
{
	if (mGoal != nullptr)
		mGoal->DrawOverPieces(g);
}

// slot 140, 0x6d0af0 (bejeweled3). port: the exe also tests a curve (0x437c60 of an object it keeps, which is not read) and reads the profile's flag at the
// quest's index even for the secret modes (index 1000 and more: past the flags, where the object holds other bytes); the port shows the help of the
// quests of the map (index below 0x28) and none for the secret modes, which have their own help dialog (HelpDialog)
void QuestBoard::OnFirstSettled()
{
	if (!mIsSecretMode && mQuestIndex >= 0 && mQuestIndex < Profile::HINT_FLAG_COUNT && gApp->mProfile != nullptr && gApp->mProfile->mQuestHelpSeen[mQuestIndex] == 0) {
		gApp->mProfile->mQuestHelpSeen[mQuestIndex] = 1;
		new QuestHelpDialog(this);
		return;
	}
	mPlayStarted = true;
	OnQuestStart();
}

// slot 141, 0x6d0ac0 (bejeweled3): the play begins once the help dialog has faded out. port: the exe's 0x6d09c0 then starts two curves of the board (which are not read)
void QuestBoard::OnDialogClosed(int theDialogId)
{
	if (theDialogId == 0x28 && !mPlayStarted) {
		OnQuestStart();
		mPlayStarted = true;
	}
}

}  // namespace Sexy
