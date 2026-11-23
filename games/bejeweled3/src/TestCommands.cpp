// The standard test commands (docs/TEST_COMMANDS.md) of Bejeweled 3, handled by the app's HandleScriptCommand. A command
// that is never given changes nothing. Each answers on SDL's log with one line: STATE {...} for state, OK <command> when
// it has done its work, ERROR <command>: <reason> when it cannot.
#include "Bej3App.h"

#include "Bej3Dialog.h"
#include "DialogButton.h"
#include "EndLevelDialog.h"
#include "ButterflyBoard.h"
#include <algorithm>
#include "IceStormBoard.h"
#include "DiamondMineBoard.h"
#include "PokerBoard.h"
#include "QuestGoal.h"
#include "MoveLimitBoard.h"
#include "FillerBoard.h"
#include "BalanceBoard.h"
#include "BadMoveLimitBoard.h"
#include "TimeBombBoard.h"
#include "QuestMenu.h"
#include "TimeLimitBoard.h"
#include "Board.h"
#include "Common.h"
#include "LightningBoard.h"
#include "MainMenu.h"
#include "PlayMenu.h"
#include "Profile.h"

#include <SDL.h>

#include <cstdlib>
#include <map>
#include <cstring>
#include <string>

using namespace Sexy;

namespace {

// the mode's key in the state line: the board's mode name in lower case, Lightning's (the exe's "Speed") by the name the player sees
const char* ModeKey(const Board* theBoard)
{
	const std::string aName = theBoard->GetModeName();
	if (aName == "Zen")
		return "zen";
	if (aName == "Speed")
		return "lightning";
	if (aName == "Butterflies")
		return "butterflies";
	if (aName == "Poker")
		return "poker";
	if (aName == "Icestorm")
		return "icestorm";
	if (aName == "BuriedTreasure")
		return "diamondmine";
	if (aName == "QuestTurnBased" || aName == "QuestTimeBased" || aName == "QuestBomb")
		return "quest";
	return "classic";
}

// port: put before every answer while `move` runs an atmove's command, so that the command's own OK or ERROR is a log line
// and the move's answer (MOVE or a wait) stays the one answer a driver reads
const char* gReplyPrefix = "";

void Reply(const std::string& theLine)
{
	SDL_Log("%s%s", gReplyPrefix, theLine.c_str());
	fflush(stdout);
	fflush(stderr);
}

void ReplyOK(const ScriptCommand& theCommand)
{
	Reply("OK " + theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs));
}

void ReplyError(const ScriptCommand& theCommand, const std::string& theReason)
{
	Reply("ERROR " + theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs) + ": " + theReason);
}

// A gem's colour as a letter: red, white, green, yellow, purple, orange, blue (the order of IMAGE_GEMS_*); . for none
char ColorLetter(const Piece* thePiece)
{
	static const char aLetters[Piece::COLOR_COUNT + 1] = {'R', 'W', 'G', 'Y', 'P', 'O', 'B', '?'};
	if (thePiece == nullptr || thePiece->mColor < 0)
		return '.';
	return aLetters[thePiece->mColor < Piece::COLOR_COUNT ? thePiece->mColor : Piece::COLOR_COUNT];
}

// What a gem is, as a letter: f flame, c hypercube, s star, x star and flame, . plain (Piece::FLAG_*)
char SpecialLetter(const Piece* thePiece)
{
	if (thePiece == nullptr)
		return '.';
	bool aFlame = (thePiece->mFlags & Piece::FLAG_FLAME) != 0;
	bool aStar = (thePiece->mFlags & Piece::FLAG_STAR) != 0;
	if (aFlame && aStar)
		return 'x';
	if (aFlame)
		return 'f';
	if (aStar)
		return 's';
	if ((thePiece->mFlags & Piece::FLAG_HYPERCUBE) != 0)
		return 'c';
	if ((thePiece->mFlags & Piece::FLAG_BUTTERFLY) != 0)
		return 'b';
	return '.';
}

// The colour a letter names (see ColorLetter), or -1
int LetterColor(char theLetter)
{
	static const char aLetters[] = "RWGYPOB";
	for (int aColor = 0; aColor < Piece::COLOR_COUNT; aColor++) {
		if (aLetters[aColor] == theLetter)
			return aColor;
	}
	return -1;
}

// The state's "over": the game has ended, as a player sees it. A board's game over with its end dialog up (the dialog waits for the others to go), or a
// quest's with the quest map back over the board (its win or loss)
bool IsGameOver(const Bej3App* theApp)
{
	const Board* aBoard = theApp->mBoard;
	if (aBoard == nullptr || !aBoard->mGameOver)
		return false;
	if (aBoard->mEndScreenShown)
		return true;
	const QuestBoard* aQuest = dynamic_cast<const QuestBoard*>(aBoard);
	return aQuest != nullptr && !aQuest->mIsSecretMode && theApp->mQuestMenu != nullptr;
}

// The steps of a click that a button takes: the pointer moves there, the press and the release come a few updates apart
// (a button reads its state once an update, docs/REAL_INPUT.md)
std::string ButtonClick(int theX, int theY)
{
	const std::string aPoint = "[" + std::to_string(theX) + "," + std::to_string(theY) + "]";
	return "{\"move\":" + aPoint + "},{\"wait\":3},{\"press\":" + aPoint + "},{\"wait\":3},{\"release\":" + aPoint + "}";
}

// The answer of a `move` that gives input, and the updates to wait before the next question
void ReplyMove(const std::string& theSteps, int theUpdates)
{
	Reply("MOVE {\"input\":[" + theSteps + "],\"updates\":" + std::to_string(theUpdates) + "}");
}

// The answer of a `move` that has nothing to press yet while the game goes on (a fade, a fall, a level change): wait and ask again
void ReplyWait(int theUpdates)
{
	ReplyMove("", theUpdates);
}

// A click on the middle of a widget, in game pixels
std::string ClickWidget(Widget* theWidget)
{
	const Point aPos = theWidget->GetAbsPos();
	return ButtonClick(aPos.mX + theWidget->mWidth / 2, aPos.mY + theWidget->mHeight / 2);
}

// A key for each character of a text, then Enter
std::string TypeText(const std::string& theText)
{
	std::string aSteps;
	for (char aCharacter : theText)
		aSteps += std::string("{\"key\":\"") + aCharacter + "\"},";
	return aSteps + "{\"key\":\"Enter\"}";
}

}  // namespace

bool Bej3App::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aCommand = theCommand.mCommand;
	if (aCommand == "seed") {
		// the app's Rand() is what seeds each board (Board::Board), so a seed given before a game starts fixes its first deal
		if (theCommand.mArgs.empty()) {
			ReplyError(theCommand, "a number is needed");
			return true;
		}
		SRand((ulong)strtoul(theCommand.mArgs.c_str(), nullptr, 10));
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "name") {
		// the first player's name, as typed into the welcome dialog (a folder with a player already has none: nothing happens)
		if (theCommand.mArgs.empty()) {
			ReplyError(theCommand, "a name is needed");
			return true;
		}
		if (mProfile != nullptr && mProfile->mName.empty()) {
			mScriptName = theCommand.mArgs;
			EnterScriptName();
		}
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "unlock") {
		// unlock all: every mode the play menu keeps shut is opened through the profile's own flags (+0x180: the four modes that start locked, Poker,
		// Butterflies, Ice Storm and Diamond Mine, by the ball's number less 4)
		if (theCommand.mArgs != "all") {
			ReplyError(theCommand, "unlock all");
			return true;
		}
		if (mProfile == nullptr || mProfile->mName.empty()) {
			// no player yet (a real-input run unlocks before the name box is answered): `move` does it once the profile has a name
			mUnlockPending = true;
			ReplyOK(theCommand);
			return true;
		}
		for (int i = 0; i < 4; i++)
			mProfile->mTutorialFlags[i] = 1;
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "goto") {
		if (theCommand.mArgs == "butterflies" || theCommand.mArgs == "poker" || theCommand.mArgs == "icestorm" || theCommand.mArgs == "diamondmine") {
			// as the play menu starts it: the ball of the mode must be open (unlock all), else the exe asks why it is shut instead
			int aIndex = theCommand.mArgs == "poker" ? 0 : theCommand.mArgs == "icestorm" ? 2 : theCommand.mArgs == "diamondmine" ? 3 : 1;
			if (mProfile == nullptr || mProfile->mTutorialFlags[aIndex] == 0) {
				ReplyError(theCommand, "that mode is locked: unlock all first");
				return true;
			}
			if (mMainMenu != nullptr && mMainMenu->mPlayMenu != nullptr && !mMainMenu->mPlayMenu->IsActive())
				mMainMenu->OpenPlayMenu();
			if (!StartSecretMode(aIndex)) {
				ReplyError(theCommand, "that mode is not made yet");
				return true;
			}
			ReplyOK(theCommand);
			return true;
		}
		if (theCommand.mArgs == "quest") {
			// the quest map, as the play menu's QUEST ball shows it (0x773b70)
			if (mMainMenu != nullptr && mMainMenu->mPlayMenu != nullptr && !mMainMenu->mPlayMenu->IsActive())
				mMainMenu->OpenPlayMenu();
			ShowQuestMenu(false);
			ReplyOK(theCommand);
			return true;
		}
		if (theCommand.mArgs == "classic" || theCommand.mArgs == "zen" || theCommand.mArgs == "lightning") {
			// the game is reached as from the screens: PLAY is pressed (the play menu opens) and then the mode's ball
			if (mMainMenu != nullptr && mMainMenu->mPlayMenu != nullptr && !mMainMenu->mPlayMenu->IsActive())
				mMainMenu->OpenPlayMenu();
			if (theCommand.mArgs == "zen")
				StartZen();
			else if (theCommand.mArgs == "lightning")
				StartLightning();
			else
				StartClassic();
			ReplyOK(theCommand);
		}
		else
			ReplyError(theCommand, "only classic, zen, lightning, quest, butterflies, poker, icestorm and diamondmine are made yet");
		return true;
	}
	if (aCommand == "quest") {
		// quest <index> [Name=Value ...]: the quest at that index of quest.cfg is started as the quest map's button starts it (0x774060); each Name=Value after the index
		// replaces a setting of the quest (a test's way to a short quest: "quest 3 Gems=20"; a value in quotes may hold spaces: Goal="Clear Gems"). quest done <index>: the quest is marked done in the profile, which is saved
		// (a test's way to open the sections); quest undo: every quest is marked not done
		if (mProfile == nullptr) {
			ReplyError(theCommand, "a player is needed");
			return true;
		}
		if (theCommand.mArgs.compare(0, 4, "done") == 0) {
			int aDoneIndex = -1;
			sscanf(theCommand.mArgs.c_str() + 4, "%d", &aDoneIndex);
			if (aDoneIndex < 0 || aDoneIndex >= QuestMenu::QUEST_COUNT) {
				ReplyError(theCommand, "quest done <index>");
				return true;
			}
			mProfile->mQuestsDone[aDoneIndex / QuestMenu::QUESTS_PER_SECTION][aDoneIndex % QuestMenu::QUESTS_PER_SECTION] = 1;
			ReplyOK(theCommand);
			return true;
		}
		if (theCommand.mArgs == "undo") {
			memset(mProfile->mQuestsDone, 0, sizeof(mProfile->mQuestsDone));
			ReplyOK(theCommand);
			return true;
		}
		int aQuestIndex = -1;
		int aRead = 0;
		if (sscanf(theCommand.mArgs.c_str(), "%d%n", &aQuestIndex, &aRead) < 1 || aQuestIndex < 0) {
			ReplyError(theCommand, "quest <index>");
			return true;
		}
		if (mMainMenu != nullptr && mMainMenu->mPlayMenu != nullptr && !mMainMenu->mPlayMenu->IsActive())
			mMainMenu->OpenPlayMenu();
		std::map<std::string, std::string> aSettingChanges;
		std::string aRest = theCommand.mArgs.substr((size_t)aRead);
		size_t aPosition = 0;
		while (aPosition < aRest.size()) {
			size_t aStart = aRest.find_first_not_of(' ', aPosition);
			if (aStart == std::string::npos)
				break;
			size_t anEnd = aStart;
			bool isQuoted = false;
			while (anEnd < aRest.size() && (isQuoted || aRest[anEnd] != ' ')) {
				if (aRest[anEnd] == '"')
					isQuoted = !isQuoted;
				anEnd++;
			}
			const std::string aPair = aRest.substr(aStart, anEnd - aStart);
			const size_t anEquals = aPair.find('=');
			if (anEquals != std::string::npos)
				aSettingChanges[aPair.substr(0, anEquals)] = aPair.substr(anEquals + 1);
			aPosition = anEnd;
		}
		mQuestSettingChanges = aSettingChanges;
		const bool isMade = StartQuest(aQuestIndex);
		mQuestSettingChanges.clear();
		if (!isMade) {
			ReplyError(theCommand, "that quest is not made yet");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "dig") {
		// dig clear [lastrow]: Diamond Mine's dirt in the rows from the top to lastrow (5 by default) is hit with powered hits until it is gone, as a game that
		// had dug that far would have it (the scroll starts when the board is at rest); dig hit <col> <row>: one plain hit on the brick there
		DigBoard* aDig = dynamic_cast<DigBoard*>(mBoard);
		if (aDig == nullptr) {
			ReplyError(theCommand, "no Diamond Mine game is on");
			return true;
		}
		int aLastRow = 5;
		int aCol = -1;
		int aRow = -1;
		if (theCommand.mArgs.compare(0, 5, "clear") == 0) {
			sscanf(theCommand.mArgs.c_str() + 5, "%d", &aLastRow);
			for (int aClearRow = 0; aClearRow <= aLastRow && aClearRow < Board::ROWS; aClearRow++) {
				for (int aClearCol = 0; aClearCol < Board::COLS; aClearCol++) {
					Piece* aPiece = aDig->mGrid[aClearRow][aClearCol];
					for (int aTry = 0; aTry < 8 && aPiece != nullptr && (aPiece->mFlags & DigGoal::FLAG_BRICK) != 0; aTry++) {
						if (aDig->DigHit(aPiece, true, false))
							break;
					}
				}
			}
		}
		else if (sscanf(theCommand.mArgs.c_str(), "hit %d %d", &aCol, &aRow) == 2 && aCol >= 0 && aCol < Board::COLS && aRow >= 0 && aRow < Board::ROWS) {
			Piece* aPiece = aDig->mGrid[aRow][aCol];
			if (aPiece == nullptr || (aPiece->mFlags & DigGoal::FLAG_BRICK) == 0) {
				ReplyError(theCommand, "no brick there");
				return true;
			}
			aDig->DigHit(aPiece, false, false);
		}
		else {
			ReplyError(theCommand, "dig clear [lastrow] or dig hit <col> <row>");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "swap") {
		// swap <col> <row> <up|down|left|right>: asks for the move the player would make, and prints the board and score
		int aCol = -1;
		int aRow = -1;
		char aDir[16] = "";
		if (mBoard == nullptr || sscanf(theCommand.mArgs.c_str(), "%d %d %15s", &aCol, &aRow, aDir) != 3 || aCol < 0 ||
			aCol >= Board::COLS || aRow < 0 || aRow >= Board::ROWS) {
			ReplyError(theCommand, "swap <col> <row> <up|down|left|right> on a board");
			return true;
		}
		int aToRow = aRow;
		int aToCol = aCol;
		std::string aWay = aDir;
		if (aWay == "up")
			aToRow--;
		else if (aWay == "down")
			aToRow++;
		else if (aWay == "left")
			aToCol--;
		else if (aWay == "right")
			aToCol++;
		if (aToRow < 0 || aToRow >= Board::ROWS || aToCol < 0 || aToCol >= Board::COLS || mBoard->mGrid[aRow][aCol] == nullptr ||
			!mBoard->TrySwap(mBoard->mGrid[aRow][aCol], aToRow, aToCol)) {
			ReplyError(theCommand, "that swap is not allowed now");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "board") {
		// board <8 words of 8 colour letters, R W G Y P O B, top row first>: puts those colours on the gems of a board that has
		// settled, takes the specials off them and leaves the rest alone (the way a test makes a run). A test command of its own
		char aRows[Board::ROWS][32];
		bool aOk = mBoard != nullptr;
		if (aOk) {
			int aRead = sscanf(theCommand.mArgs.c_str(), "%31s %31s %31s %31s %31s %31s %31s %31s", aRows[0], aRows[1], aRows[2], aRows[3],
				aRows[4], aRows[5], aRows[6], aRows[7]);
			aOk = aRead == Board::ROWS;
			for (int aRow = 0; aOk && aRow < Board::ROWS; aRow++) {
				aOk = strlen(aRows[aRow]) == (size_t)Board::COLS;
				for (int aCol = 0; aOk && aCol < Board::COLS; aCol++)
					aOk = LetterColor(aRows[aRow][aCol]) >= 0 && mBoard->mGrid[aRow][aCol] != nullptr;
			}
		}
		if (!aOk) {
			ReplyError(theCommand, "board <8 words of 8 colour letters RWGYPOB> on a full board");
			return true;
		}
		for (int aRow = 0; aRow < Board::ROWS; aRow++) {
			for (int aCol = 0; aCol < Board::COLS; aCol++) {
				Piece* aPiece = mBoard->mGrid[aRow][aCol];
				aPiece->mColor = LetterColor(aRows[aRow][aCol]);
				aPiece->mFlags = 0;
				aPiece->mSpecialCountdown = 0;
			}
		}
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "put") {
		// put <col> <row> <letter> [flame|cube|star]: gives the gem at that cell a colour and, when named, makes it that special
		// through the same makers a run uses, then lets it go off at once (its countdown, +0x184, is put to 0); a fifth word "hold" leaves the
		// countdown as the maker set it, so the special stays until a match or a swap uses it
		int aCol = -1;
		int aRow = -1;
		char aLetter[8] = "";
		char aKind[16] = "";
		char aHold[16] = "";
		int aRead = mBoard == nullptr ? 0 : sscanf(theCommand.mArgs.c_str(), "%d %d %7s %15s %15s", &aCol, &aRow, aLetter, aKind, aHold);
		if (aRead < 3 || aCol < 0 || aCol >= Board::COLS || aRow < 0 || aRow >= Board::ROWS || mBoard->mGrid[aRow][aCol] == nullptr ||
			LetterColor(aLetter[0]) < 0) {
			ReplyError(theCommand, "put <col> <row> <RWGYPOB> [flame|cube|star] on a board");
			return true;
		}
		Piece* aPiece = mBoard->mGrid[aRow][aCol];
		aPiece->mColor = LetterColor(aLetter[0]);
		aPiece->mFlags = 0;
		std::string aWhat = aKind;
		if (aWhat == "flame")
			mBoard->MakeFlameGem(aPiece);
		else if (aWhat == "star")
			mBoard->MakeStarGem(aPiece);
		else if (aWhat == "cube")
			mBoard->MakeHypercube(aPiece, true);
		else if (!aWhat.empty()) {
			ReplyError(theCommand, "the kind is flame, cube or star");
			return true;
		}
		if (strcmp(aHold, "hold") != 0)
			aPiece->mSpecialCountdown = 0;
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "play") {
		// play <mode> [quest index]: what the bot's `move` clicks its way to from wherever the screens are (the first player's name, PLAY, the mode's ball, on the
		// quest map the quest's button), as a hand does. The mode is classic, zen, lightning, quest, poker, butterflies, icestorm or diamondmine
		static const char* const aModes[PlayMenu::BALL_COUNT] = {"classic", "zen", "lightning", "quest", "poker", "butterflies", "icestorm", "diamondmine"};
		char aMode[32] = "";
		int aQuest = 0;
		sscanf(theCommand.mArgs.c_str(), "%31s %d", aMode, &aQuest);
		bool isKnown = false;
		for (const char* aName : aModes)
			isKnown = isKnown || strcmp(aName, aMode) == 0;
		if (!isKnown || aQuest < 0 || aQuest >= QuestMenu::QUEST_COUNT) {
			ReplyError(theCommand, "play <classic|zen|lightning|quest|poker|butterflies|icestorm|diamondmine> [quest index]");
			return true;
		}
		mPlayTarget = aMode;
		mPlayQuest = aQuest;
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "idle") {
		// idle [off]: from now on `move` makes no swap, as a player who has walked away (Ice Storm, a bot that always has a move, ends only so)
		mIdle = theCommand.mArgs != "off";
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "atmove") {
		// atmove <n> <command> [args]: once `move` has given n swaps and is at rest, the bot's `move` first runs that test command (a real-input run's way to
		// shorten a long mode after it has started as a hand starts it, e.g. `atmove 40 board <a board with no move>`); atmove off forgets it
		int aCount = -1;
		int aRead = 0;
		if (theCommand.mArgs == "off") {
			mAtMoveCount = -1;
			ReplyOK(theCommand);
			return true;
		}
		if (sscanf(theCommand.mArgs.c_str(), "%d%n", &aCount, &aRead) < 1 || aCount < 0) {
			ReplyError(theCommand, "atmove <n> <command> [args]");
			return true;
		}
		const std::string aRest = theCommand.mArgs.substr((size_t)aRead);
		const size_t aStart = aRest.find_first_not_of(' ');
		if (aStart == std::string::npos) {
			ReplyError(theCommand, "atmove <n> <command> [args]");
			return true;
		}
		const size_t aSpace = aRest.find(' ', aStart);
		mAtMoveCount = aCount;
		mAtMoveCommand = aRest.substr(aStart, aSpace == std::string::npos ? std::string::npos : aSpace - aStart);
		mAtMoveArgs = aSpace == std::string::npos ? "" : aRest.substr(aRest.find_first_not_of(' ', aSpace) == std::string::npos ? aRest.size() : aRest.find_first_not_of(' ', aSpace));
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "move") {
		// move: the input a player's hand gives to get on, as steps in game pixels. On a board at rest: the bot's swap, the first the exe's move search (0x75a8e0,
		// rows from the top) finds, as the two clicks of the player (the gem is picked up, then its neighbour is clicked). On the screens before it (the player's
		// name box, a dialog's button, PLAY, the mode's ball named by `play`, the quest map's button): the keys or the click that gets on. A wait with no input while
		// something fades, falls or changes level. MOVE null once the game has ended (state's "over"), or when there is nothing to do. Asking changes nothing but
		// one thing: an `unlock all` given before the player existed is done here, once the name has made the profile
		if (mUnlockPending && mProfile != nullptr && !mProfile->mName.empty()) {
			for (int i = 0; i < 4; i++)
				mProfile->mTutorialFlags[i] = 1;
			mUnlockPending = false;
		}
		// the dialogs first: the new player's name box, then any other one by its button
		for (Dialog* aDialog : mDialogList) {
			Bej3Dialog* aBej3Dialog = dynamic_cast<Bej3Dialog*>(aDialog);
			if (aBej3Dialog == nullptr || aBej3Dialog->mClosing || aBej3Dialog->mFadeCurve.GetOutVal() < 1.0) {
				ReplyWait(10);
				return true;
			}
			if (NewUserDialog* aNameDialog = dynamic_cast<NewUserDialog*>(aDialog)) {
				if (aNameDialog->mEdit->mString.empty())
					ReplyMove(TypeText("Tester"), 10);
				else
					ReplyWait(10);
			}
			else if (aDialog->mId == ClassicEndLevelDialog::DIALOG_ID)
				Reply("MOVE null");
			else if (aDialog->mYesButton != nullptr)
				ReplyMove(ClickWidget(aDialog->mYesButton), 10);
			else
				Reply("MOVE null");
			return true;
		}
		if (mBoard == nullptr) {
			if (mQuestMenu != nullptr) {
				// the quest map: the button of the quest asked for, when it is on the page on show
				if (mPlayTarget != "quest" || mQuestMenu->mSection != mPlayQuest / QuestMenu::QUESTS_PER_SECTION ||
					mQuestMenu->mQuestButtons.size() != (size_t)QuestMenu::QUESTS_PER_SECTION)
					ReplyWait(10);
				else
					ReplyMove(ClickWidget(mQuestMenu->mQuestButtons[mPlayQuest % QuestMenu::QUESTS_PER_SECTION]), 30);
				return true;
			}
			if (mMainMenu == nullptr || !mMainMenu->mIntroStarted || mMainMenu->mDistanceCurve.GetOutVal() != 0.0 || mMainMenu->mTurnACurve.GetOutVal() != 0.0) {
				ReplyWait(30);
				return true;
			}
			PlayMenu* aPlayMenu = mMainMenu->mPlayMenu;
			if (mPlayTarget.empty() || aPlayMenu == nullptr) {
				Reply("MOVE null");
				return true;
			}
			if (aPlayMenu->IsActive()) {
				static const char* const aModes[PlayMenu::BALL_COUNT] = {"classic", "zen", "lightning", "quest", "poker", "butterflies", "icestorm", "diamondmine"};
				int aBall = 0;
				while (aBall < PlayMenu::BALL_COUNT - 1 && mPlayTarget != aModes[aBall])
					aBall++;
				if (aPlayMenu->mFadeCurve.GetOutVal() != 1.0 || aPlayMenu->mBalls[aBall]->mScaleCurve.IsDoingCurve() || aPlayMenu->mHideCurve.IsDoingCurve())
					ReplyWait(10);
				else
					ReplyMove(ClickWidget(aPlayMenu->mBalls[aBall]), 60);
				return true;
			}
			if (mMainMenu->mPlayButton->mScaleCurve.IsDoingCurve())
				ReplyWait(10);
			else
				ReplyMove(ClickWidget(mMainMenu->mPlayButton), 60);
			return true;
		}
		if (mAtMoveCount >= 0 && mBoard->IsSettled() && !mBoard->mGameOver && mSwapsGiven >= mAtMoveCount) {
			ScriptCommand aRun = {mUpdateCount, mAtMoveCommand, mAtMoveArgs, 0};
			mAtMoveCount = -1;
			gReplyPrefix = "atmove: ";
			HandleScriptCommand(aRun);
			gReplyPrefix = "";
		}
		BoardMove aMove;
		if (mBoard->mGameOver || mQuestMenu != nullptr) {
			// the game has ended (the dialog is up or about to be) or a quest's map is over the board: nothing more to play
			if (IsGameOver(this))
				Reply("MOVE null");
			else
				ReplyWait(20);
			return true;
		}
		if (!mBoard->IsSettled() || mBoard->mHyperspace != nullptr || mBoard->mLevelUpPending) {
			ReplyWait(10);
			return true;
		}
		if (mIdle || !mBoard->FindMove(&aMove, 0, false, nullptr, false)) {
			ReplyWait(10);
			return true;
		}
		mSwapsGiven++;
		const int aCell = 128;
		auto aPixel = [&](int theMeasure) { return mArtRes * theMeasure / 1200; };
		int aFromX = aPixel(mBoard->GetGridX() + aMove.mFromCol * aCell + aCell / 2);
		int aFromY = aPixel(mBoard->GetGridY() + aMove.mFromRow * aCell + aCell / 2);
		int aToX = aPixel(mBoard->GetGridX() + aMove.mToCol * aCell + aCell / 2);
		int aToY = aPixel(mBoard->GetGridY() + aMove.mToRow * aCell + aCell / 2);
		Reply("MOVE {\"swap\":[" + std::to_string(aMove.mFromCol) + "," + std::to_string(aMove.mFromRow) + "," + std::to_string(aMove.mToCol) +
			"," + std::to_string(aMove.mToRow) + "],\"input\":[{\"click\":[" + std::to_string(aFromX) + "," + std::to_string(aFromY) +
			"]},{\"click\":[" + std::to_string(aToX) + "," + std::to_string(aToY) + "]}]}");
		return true;
	}
	if (aCommand == "bot") {
		// bot [n]: from now on the board plays the bot's move each time it is at rest, n moves (no n: until the game is over)
		if (mBoard == nullptr) {
			ReplyError(theCommand, "bot needs a board");
			return true;
		}
		mBoard->mBotMoves = theCommand.mArgs.empty() ? -1 : atoi(theCommand.mArgs.c_str());
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "hint") {
		// hint: the hint button as the player presses it (draws the app's Rand() twice, as the exe's search does)
		if (mBoard == nullptr || !mBoard->HintAllowed()) {
			ReplyError(theCommand, "no hint now");
			return true;
		}
		mBoard->ShowHint(true);
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "set") {
		// set <progress|points|index> <n>: the level bar's progress, the score and the level's index, to reach the states quickly
		char aName[16] = "";
		int aValue = 0;
		if (mBoard == nullptr || sscanf(theCommand.mArgs.c_str(), "%15s %d", aName, &aValue) != 2) {
			ReplyError(theCommand, "set <progress|points|index> <n> on a board");
			return true;
		}
		std::string aWhat = aName;
		if (aWhat == "progress")
			mBoard->mLevelProgress = aValue;
		else if (aWhat == "points")
			mBoard->mScorePoints = aValue;
		else if (aWhat == "index")
			mBoard->mLevelIndex = aValue;
		else {
			ReplyError(theCommand, "set <progress|points|index> <n>");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "state") {
		std::string aLine = "STATE {\"screen\":\"";
		aLine += mBoard != nullptr ? ModeKey(mBoard) : mQuestMenu != nullptr ? "quest" : "menu";
		aLine += "\"";
		if (mBoard == nullptr)
			aLine += ",\"update\":" + std::to_string(mUpdateCount) + ",\"dialogs\":" + std::to_string(GetDialogCount());
		if (mQuestMenu != nullptr && mProfile != nullptr) {
			// the quest map is up: the section on show, the sections' done counts and states, and the quests done as a string of 40 characters
			std::string aDone;
			for (int aQuest = 0; aQuest < QuestMenu::QUEST_COUNT; aQuest++)
				aDone += mQuestMenu->IsQuestDone(aQuest) ? '1' : '0';
			aLine += ",\"questmenu\":1,\"questsection\":" + std::to_string(mQuestMenu->mSection) + ",\"quests\":\"" + aDone + "\",\"sectiondone\":[";
			for (int aSection = 0; aSection < QuestMenu::SECTION_COUNT; aSection++)
				aLine += (aSection == 0 ? "" : ",") + std::to_string(mQuestMenu->mSectionDone[aSection]);
			aLine += "],\"sectionstate\":[";
			for (int aSection = 0; aSection < QuestMenu::SECTION_COUNT; aSection++)
				aLine += (aSection == 0 ? "" : ",") + std::to_string(mQuestMenu->mSectionState[aSection]);
			aLine += "],\"questbuttons\":" + std::to_string(mQuestMenu->mQuestButtons.size());
		}
		if (mBoard != nullptr) {
			aLine += std::string(",\"mode\":\"") + ModeKey(mBoard) + "\",\"score\":" + std::to_string(mBoard->mScore) + ",\"points\":" + std::to_string(mBoard->mScorePoints) +
				",\"movematches\":" + std::to_string(mBoard->mMoveRecords.empty() ? 0 : mBoard->mMoveRecords.back().mCounters[COUNTER_MATCHES]) +
				",\"matches\":" + std::to_string(mBoard->mMatchGroups.size()) + ",\"swaps\":" + std::to_string(mBoard->mSwaps.size()) +
				",\"board\":[";
			for (int aRow = 0; aRow < Board::ROWS; aRow++) {
				aLine += aRow == 0 ? "\"" : ",\"";
				for (int aCol = 0; aCol < Board::COLS; aCol++)
					aLine += ColorLetter(mBoard->mGrid[aRow][aCol]);
				aLine += "\"";
			}
			aLine += "],\"specials\":[";
			for (int aRow = 0; aRow < Board::ROWS; aRow++) {
				aLine += aRow == 0 ? "\"" : ",\"";
				for (int aCol = 0; aCol < Board::COLS; aCol++)
					aLine += SpecialLetter(mBoard->mGrid[aRow][aCol]);
				aLine += "\"";
			}
			aLine += "],\"matchcounter\":" + std::to_string(mBoard->mMatchCounter) + ",\"pieces\":" + std::to_string(mBoard->mPieceCounter);
			aLine += ",\"level\":" + std::to_string(mBoard->mLevelIndex + 1) + ",\"levelnumber\":" + std::to_string(mBoard->mLevelNumber) +
				",\"progress\":" + std::to_string(mBoard->mLevelProgress) + ",\"levelmax\":" + std::to_string(mBoard->GetLevelMax()) +
				",\"bar\":" + std::to_string((int)(mBoard->mLevelBar * 10000.0f)) + ",\"levelchange\":" +
				std::to_string(mBoard->mHyperspace != nullptr ? mBoard->mHyperspace->mAge : (mBoard->mLevelUpPending ? -1 : 0)) +
				",\"gameover\":" + std::to_string(mBoard->mGameOver ? 1 : 0) + ",\"gameoverticks\":" + std::to_string(mBoard->mGameOverTicks) +
				",\"endscreen\":" + std::to_string(mBoard->mEndScreenShown ? 1 : 0) + ",\"hinttimer\":" + std::to_string(mBoard->mHintTimer) +
				",\"botmoves\":" + std::to_string(mBoard->mBotMoveCount) + ",\"highscores\":[";
			for (size_t aIndex = 0; aIndex < mBoard->mHighScores.size(); aIndex++)
				aLine += (aIndex == 0 ? "" : ",") + std::to_string(mBoard->mHighScores[aIndex]);
			aLine += "]";
			aLine += ",\"background\":" + std::to_string(mBoard->mBackgroundIndex);
			// how far the fall has come: the update count, the pieces not at rest, the fill's landing estimate and the
			// sums of the pieces' y and speeds (as whole thousandths, so a run can be compared with another)
			int aMoving = 0;
			long long aYSum = 0;
			long long aSpeedSum = 0;
			for (int aRow = 0; aRow < Board::ROWS; aRow++) {
				for (int aCol = 0; aCol < Board::COLS; aCol++) {
					const Piece* aPiece = mBoard->mGrid[aRow][aCol];
					if (aPiece == nullptr)
						continue;
					if (!Board::PieceIsSettled(aPiece))
						aMoving++;
					aYSum += (long long)(aPiece->mY * 1000.0f);
					aSpeedSum += (long long)(aPiece->mFallSpeed * 1000.0f);
				}
			}
			aLine += ",\"update\":" + std::to_string(mBoard->mUpdateCnt) + ",\"moving\":" + std::to_string(aMoving) +
				",\"landticks\":" + std::to_string(mBoard->mLandTicks) + ",\"ysum\":" + std::to_string(aYSum) +
				",\"speedsum\":" + std::to_string(aSpeedSum);
			if (const QuestBoard* aQuest = dynamic_cast<const QuestBoard*>(mBoard)) {
				// the quest boards' update counters (+0x3198 and +0x31a0, QuestBoard::Update)
				aLine += ",\"updatesplayed\":" + std::to_string(aQuest->mUpdatesPlayed) + ",\"updatesrun\":" + std::to_string(aQuest->mUpdatesRun);
			}
			if (const QuestBoard* aQuestBoard = dynamic_cast<const QuestBoard*>(mBoard)) {
				// a quest (a board that is not a secret mode): its index, whether it was won, the clock of the level's end (+0xd20), the game's end counter is
				// "gameoverticks" above, the goal's size and its kind (the goal's class by the setting Goal) and what the goal counts; the moves and the time for the boards that have
				// them
				if (!aQuestBoard->mIsSecretMode) {
					aLine += ",\"quest\":" + std::to_string(aQuestBoard->mQuestIndex) + ",\"questwon\":" + std::to_string(aQuestBoard->mQuestWon ? 1 : 0) +
						",\"questclock\":" + std::to_string(aQuestBoard->mQuestClock) + ",\"goal\":\"" + aQuestBoard->SettingText("Goal") +
						"\",\"goalsize\":" + std::to_string(aQuestBoard->GetLevelMax()) + ",\"goalcount\":" + std::to_string(aQuestBoard->mLevelProgress) +
						",\"questscore\":" + std::to_string(aQuestBoard->mScorePoints);
					if (aQuestBoard->mGoal != nullptr)
						aLine += aQuestBoard->mGoal->DescribeState();
					if (mProfile != nullptr) {
						std::string aDone;
						for (int aQuest = 0; aQuest < QuestMenu::QUEST_COUNT; aQuest++)
							aDone += mProfile->mQuestsDone[aQuest / QuestMenu::QUESTS_PER_SECTION][aQuest % QuestMenu::QUESTS_PER_SECTION] != 0 ? '1' : '0';
						aLine += ",\"quests\":\"" + aDone + "\",\"questmenu\":" + std::to_string(mQuestMenu != nullptr ? 1 : 0);
					}
					if (const MoveLimitBoard* aMoves = dynamic_cast<const MoveLimitBoard*>(mBoard))
						aLine += ",\"movesleft\":" + std::to_string(aMoves->mMovesLeft) + ",\"moves\":" + std::to_string(aMoves->mMoves);
					if (const TimeLimitBoard* aTimed = dynamic_cast<const TimeLimitBoard*>(mBoard))
						aLine += ",\"timeleft\":" + std::to_string(aTimed->GetTimeLeft());
					if (const FillerBoard* aFiller = dynamic_cast<const FillerBoard*>(mBoard))
						aLine += ",\"credits\":" + std::to_string(aFiller->mCredits) + ",\"droppermove\":" + std::to_string(aFiller->mDropPerMove) + ",\"gemsdropped\":" +
							std::to_string(aFiller->mGemsDropped) + ",\"gemsremoved\":" + std::to_string(aFiller->mGemsRemoved) + ",\"gemsatstart\":" + std::to_string(aFiller->mGemsAtMoveStart) +
							",\"fullwarning\":" + std::to_string((int)(aFiller->mFullWarning * 1000.0f)) + ",\"dropallowed\":" + std::to_string(aFiller->mDropAllowed ? 1 : 0);
					if (const BalanceBoard* aBalance = dynamic_cast<const BalanceBoard*>(mBoard))
						aLine += ",\"balance\":" + std::to_string((int)(aBalance->mBalance * 1000.0f)) + ",\"red\":" + std::to_string(aBalance->mRedCount) + ",\"blue\":" +
							std::to_string(aBalance->mBlueCount) + ",\"gemspercolor\":" + std::to_string(aBalance->mGemsPerColor);
					if (const BadMoveLimitBoard* aBadMoves = dynamic_cast<const BadMoveLimitBoard*>(mBoard))
						aLine += ",\"badallowed\":" + std::to_string(aBadMoves->mBadMovesAllowed) + ",\"badused\":" + std::to_string(aBadMoves->mBadMovesUsed);
					if (const TimeBombBoard* aBombs = dynamic_cast<const TimeBombBoard*>(mBoard)) {
						// the bombs: the count of each as a digit (9 for more), . where there is none; and the bombs on the board, the moves until the next drop, whether the play has begun
						aLine += ",\"bombgrid\":[";
						for (int aRow = 0; aRow < Board::ROWS; aRow++) {
							aLine += aRow == 0 ? "\"" : ",\"";
							for (int aCol = 0; aCol < Board::COLS; aCol++) {
								const Piece* aPiece = aBombs->mGrid[aRow][aCol];
								if (aPiece == nullptr || (aPiece->mFlags & TimeBombBoard::FLAG_BOMB) == 0)
									aLine += '.';
								else
									aLine += (char)('0' + (aPiece->mTimeGemSeconds < 0 ? 0 : aPiece->mTimeGemSeconds > 9 ? 9 : aPiece->mTimeGemSeconds));
							}
							aLine += "\"";
						}
						aLine += "],\"bombs\":" + std::to_string(aBombs->mBombCount) + ",\"untildrop\":" + std::to_string(aBombs->mMovesUntilDrop) + ",\"bombstarted\":" +
							std::to_string(aBombs->mStarted ? 1 : 0) + ",\"realtime\":" + std::to_string(aBombs->mRealTime ? 1 : 0);
					}
				}
			}
			if (const ButterflyBoard* aButterflies = dynamic_cast<const ButterflyBoard*>(mBoard)) {
				// Butterflies: the moves each butterfly waits before it climbs (a digit, 9 for more; . where there is none), a caught one
				// as its digit in capitals' place (a letter from A: its count), the spider's column in hundredths, where it goes, the
				// drop's countdown, the credit and the count of a drop in thousandths, the chance of a side column in thousandths, and the
				// butterflies freed and the best number freed by one move
				aLine += ",\"butterflies\":[";
				for (int aRow = 0; aRow < Board::ROWS; aRow++) {
					aLine += aRow == 0 ? "\"" : ",\"";
					for (int aCol = 0; aCol < Board::COLS; aCol++) {
						const Piece* aPiece = aButterflies->mGrid[aRow][aCol];
						if (aPiece == nullptr || (aPiece->mFlags & Piece::FLAG_BUTTERFLY) == 0)
							aLine += '.';
						else {
							int aLeft = aPiece->mTimeGemSeconds < 0 ? 0 : aPiece->mTimeGemSeconds > 9 ? 9 : aPiece->mTimeGemSeconds;
							aLine += aPiece->mSpecialState == -1 ? (char)('A' + aLeft) : (char)('0' + aLeft);
						}
					}
					aLine += "\"";
				}
				aLine += "],\"spiderx\":" + std::to_string((int)(aButterflies->mSpiderX * 100.0f)) + ",\"spiderto\":" +
					std::to_string((int)aButterflies->mSpiderTo) + ",\"dropcountdown\":" + std::to_string(aButterflies->mDropCountdown) +
					",\"credit\":" + std::to_string((int)(aButterflies->mSpawnCredit * 1000.0f)) + ",\"spawncount\":" +
					std::to_string((int)(aButterflies->mSpawnCount * 1000.0f)) + ",\"sidechance\":" +
					std::to_string((int)(aButterflies->mSideSpawnChance * 1000.0f)) + ",\"freed\":" +
					std::to_string(aButterflies->mCounters[ButterflyBoard::COUNTER_BUTTERFLIES_FREED]) + ",\"bestcombo\":" +
					std::to_string(aButterflies->mCounters[ButterflyBoard::COUNTER_BEST_COMBO]);
			}
			if (const IceStormBoard* anIce = dynamic_cast<const IceStormBoard*>(mBoard)) {
				// Ice Storm: for each of the eight columns the ice's height in thousandths of a row (-1 where no ice is made; a double's second
				// column too), how many columns wide it is, whether it is crushed (1) and its crack ticks (-1 none); then the ice taken off the bar
				// and the bar's need in thousandths of a row, the multiplier, the stage, whether the ice is paused, the ice's ticks, the columns
				// crushed, the best chain and the chain's length
				std::string aHeights;
				std::string aSpans;
				std::string aCrushed;
				std::string aCracks;
				for (int aColumn = 0; aColumn < IceStormBoard::COLUMN_COUNT; aColumn++) {
					const IceStormBoard::Column& aState = anIce->mColumns[aColumn];
					const bool anAnchor = aState.mOwner == aColumn && aState.mActive;
					const char* aSeparator = aColumn == 0 ? "" : ",";
					aHeights += aSeparator + std::to_string(anAnchor ? (int)(aState.mPosition * 1000.0f) : -1);
					aSpans += aSeparator + std::to_string(anAnchor ? aState.mSpan : 0);
					aCrushed += aSeparator + std::to_string(anAnchor && aState.mCrushed ? 1 : 0);
					aCracks += aSeparator + std::to_string(anAnchor ? aState.mCrackTicks : -1);
				}
				aLine += ",\"ice\":[" + aHeights + "],\"icespan\":[" + aSpans + "],\"icecrushed\":[" + aCrushed + "],\"icecrack\":[" + aCracks +
					"],\"iceremoved\":" + std::to_string((int)(anIce->mIceRemoved * 1000.0f)) + ",\"icebar\":" +
					std::to_string((int)(anIce->mMultiplierIceThreshold * 1000.0f)) + ",\"icemult\":" + std::to_string(anIce->mLevelNumber) +
					",\"icestage\":" + std::to_string(anIce->mStage) + ",\"icepaused\":" + std::to_string(anIce->mPaused ? 1 : 0) +
					",\"iceticks\":" + std::to_string(anIce->mTicks) + ",\"icecrushedcount\":" +
					std::to_string(anIce->mCounters[IceStormBoard::COUNTER_COLUMNS_CRUSHED]) + ",\"icebestchain\":" +
					std::to_string(anIce->mBestChain) + ",\"icechain\":" + std::to_string(anIce->mChainCount);
			}
			if (const DigBoard* aDig = dynamic_cast<const DigBoard*>(mBoard)) {
				// Diamond Mine: the grid's bricks as eight rows of characters (. none, 1 to 5 dirt with that many hits left, g gold, d diamond, a artifact,
				// h dirt that hides a hypercube), the bricks left, the depth in rows, the clock in hundredths and its limit in seconds, the clock's cap (the last
				// bonus' left), whether the time ran out, the treasure collected (the score), the gold, diamonds and artifacts, the best treasure, the
				// rows made, the scrolls and whether one runs, the dirt destroyed and the artifacts placed in the dirt
				const DigGoal* aGoal = aDig->Goal();
				std::string aMap;
				for (int aRow = 0; aRow < Board::ROWS; aRow++) {
					if (aRow > 0)
						aMap += '/';
					for (int aCol = 0; aCol < Board::COLS; aCol++) {
						const Piece* aPiece = aDig->mGrid[aRow][aCol];
						char aCharacter = '.';
						if (aPiece != nullptr && (aPiece->mFlags & DigGoal::FLAG_BRICK) != 0) {
							auto aBrick = aGoal->mBricks.find(aPiece->mId);
							aCharacter = '?';
							if (aBrick != aGoal->mBricks.end()) {
								if (aBrick->second.mKind == DigGoal::KIND_ARTIFACT)
									aCharacter = 'a';
								else if (aBrick->second.mKind == DigGoal::KIND_MONEY)
									aCharacter = aBrick->second.mHitsInitial <= DigGoal::GOLD_STRENGTH_MAX ? 'g' : 'd';
								else if (std::find(aGoal->mHypercubeBrickIds.begin(), aGoal->mHypercubeBrickIds.end(), aPiece->mId) != aGoal->mHypercubeBrickIds.end())
									aCharacter = 'h';
								else
									aCharacter = (char)('0' + aBrick->second.mHitsLeft);
							}
						}
						aMap += aCharacter;
					}
				}
				aLine += ",\"digmap\":\"" + aMap + "\",\"digbricks\":" + std::to_string(aGoal->CountBricks()) + ",\"digdepth\":" +
					std::to_string(aGoal->DepthShown()) + ",\"digtime\":" + std::to_string(aDig->GetTimeLeft()) + ",\"digtimelimit\":" +
					std::to_string(aDig->mTimeSeconds) + ",\"digtimecap\":" + std::to_string(aDig->mTimeLeftCap) + ",\"digtimeup\":" +
					std::to_string(aDig->mTimeUp ? 1 : 0) + ",\"digtreasure\":" + std::to_string(aDig->mLevelProgress) + ",\"diggold\":" +
					std::to_string(aGoal->mGoldCollected) + ",\"digdiamonds\":" + std::to_string(aGoal->mDiamondsCollected) + ",\"digartifacts\":" +
					std::to_string(aGoal->mArtifactsCollected) + ",\"digbest\":" + std::to_string(aGoal->mBestTreasureValue) + ",\"digrows\":" +
					std::to_string((int)aGoal->mRowsGenerated) + ",\"digscrolls\":" + std::to_string(aGoal->mScrollCount) + ",\"digscrolling\":" +
					std::to_string(aGoal->mScrolling ? 1 : 0) + ",\"digdestroyed\":" + std::to_string(aGoal->mBricksDestroyed) + ",\"digplaced\":" +
					std::to_string(aGoal->mPlacedArtifacts.size());
			}
			if (const PokerBoard* aPoker = dynamic_cast<const PokerBoard*>(mBoard)) {
				// Poker: the hand (each card's suit, "/" and its second suit, a letter for a wild (w), flame (f) or star (s) card; - for no card),
				// the best hand's index (-1 none), the skull's level (-1 none) and its clock and the eliminator's bar in thousandths, the hands
				// cleared, the skulls busted, the counts of each kind (the first the hands a skull took), the curves of the hand, the flip and the fly-away in thousandths, the last hand's
				// points and the coin flips
				aLine += ",\"hand\":\"" + aPoker->DescribeCards() + "\",\"besthand\":" + std::to_string(aPoker->mBestHand) + ",\"skull\":" +
					std::to_string(aPoker->mSkull) + ",\"skullclock\":" + std::to_string((int)(aPoker->mSkullClock * 1000.0f)) + ",\"buster\":" +
					std::to_string((int)(aPoker->mBuster * 1000.0f)) + ",\"hands\":" + std::to_string(aPoker->mHands) + ",\"handsleft\":" + std::to_string(aPoker->mHandsLeft) + ",\"pokergoal\":" + std::to_string(aPoker->mGoalScore) + ",\"busted\":" +
					std::to_string(aPoker->mSkullsBusted) + ",\"counts\":\"";
				for (int i = 0; i <= PokerBoard::HAND_KINDS; i++)
					aLine += (i > 0 ? "," : "") + std::to_string(aPoker->mHandCounts[i]);
				aLine += "\",\"handcurve\":" + std::to_string((int)(aPoker->mHandCurve.GetOutVal() * 1000.0)) + ",\"flipcurve\":" + std::to_string((int)(aPoker->mFlipCurve.GetOutVal() * 1000.0)) + ",\"flycurve\":" + std::to_string((int)(aPoker->mFlyCurve.GetOutVal() * 1000.0)) + ",\"handscore\":" + std::to_string(aPoker->mHandScore) +
					",\"flips\":" + std::to_string(aPoker->mCounters[PokerBoard::COUNTER_COIN_FLIPS]);
			}
			if (const LightningBoard* aLightning = dynamic_cast<const LightningBoard*>(mBoard)) {
				// Lightning: the game ticks left of the round, the clock, the seconds the matched time gems hold (this round's account and
				// the game's), the multiplier, whether the time-up is shown, the chances' counters and the time gems on the board (each
				// cell its seconds as a letter: 5 or a for 10, . for none)
				aLine += ",\"timeleft\":" + std::to_string(aLightning->GetTimeLeft()) + ",\"clock\":" + std::to_string((int)(aLightning->mClock * 100.0f)) +
					",\"limit\":" + std::to_string(aLightning->mTimeLimitTicks) + ",\"bonus\":" + std::to_string(aLightning->mBonusSeconds) +
					",\"bonustotal\":" + std::to_string(aLightning->mBonusSecondsTotal) + ",\"timeup\":" + std::to_string(aLightning->mTimeUpShown ? 1 : 0) +
					",\"gameticks\":" + std::to_string(aLightning->mGameTicks) + ",\"chance5\":" + std::to_string((int)(aLightning->mChance5.mChance * 1000000.0f)) +
					",\"chance10\":" + std::to_string((int)(aLightning->mChance10.mChance * 1000000.0f)) + ",\"timegems\":[";
				for (int aRow = 0; aRow < Board::ROWS; aRow++) {
					aLine += aRow == 0 ? "\"" : ",\"";
					for (int aCol = 0; aCol < Board::COLS; aCol++) {
						const Piece* aPiece = aLightning->mGrid[aRow][aCol];
						int aSeconds = aPiece != nullptr && (aPiece->mFlags & Piece::FLAG_TIME_GEM) != 0 ? aPiece->mTimeGemSeconds : 0;
						aLine += aSeconds == 0 ? '.' : aSeconds == 5 ? '5' : 'a';
					}
					aLine += "\"";
				}
				aLine += "]";
			}
		}
		aLine += std::string(",\"over\":") + (IsGameOver(this) ? "true" : "false") + "}";
		Reply(aLine);
		return true;
	}
	return SexyApp::HandleScriptCommand(theCommand);
}
