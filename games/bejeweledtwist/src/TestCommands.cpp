// port: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script or live command
// channel through HandleScriptCommand. A command that is never given changes nothing. Each answers on stdout with one
// line: STATE {...}, OK <command> or ERROR <command>: <reason>. They act through the game's own code: the menu's
// buttons, the quit question's answer, the level's own completion, the clock's own end.

#include "WinTwistApp.h"
#include "Board.h"
#include "GoodTube.h"
#include "Mandala.h"
#include "Menu.h"
#include "ModeButton.h"
#include "Piece.h"
#include "Profile.h"
#include "TwistButton.h"
#include "TwistDialog.h"
#include "Tutorial.h"
#include "AwardTelly.h"
#include "Gates.h"
#include "MenuButtonWidget.h"
#include "MenuPanels.h"
#include "RankBonusWidget.h"
#include "WheelSpin.h"
#include "WidgetManager.h"
#include <SDL.h>

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace Sexy;

namespace {

// Answers go to SDL's log, as the framework's own "OK" and "ERROR" do (natively stderr, on the web the page's log):
// on the web a game line on stdout once lost its end of line in the page's terminal buffer, which then held every later
// answer back
void Reply(const std::string& theLine)
{
	SDL_Log("%s", theLine.c_str());
	fflush(stdout);
	fflush(stderr);
}

void ReplyOK(const ScriptCommand& theCommand)
{
	Reply("OK " + theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs));
}

void ReplyError(const ScriptCommand& theCommand, const std::string& theReason)
{
	Reply("ERROR " + theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs) + ": " +
		theReason);
}

// The modes by the names a driver gives them, as Board's MODE_ numbers
int ModeByName(const std::string& theName)
{
	if (theName == "classic")
		return Board::MODE_CLASSIC;
	if (theName == "zen")
		return Board::MODE_ZEN;
	if (theName == "blitz")
		return Board::MODE_BLITZ;
	if (theName == "challenge")
		return Board::MODE_CHALLENGE;
	return -1;
}

const char* ModeName(int theMode)
{
	switch (theMode) {
	case Board::MODE_CASUAL: return "casual";
	case Board::MODE_CLASSIC: return "classic";
	case Board::MODE_CHALLENGE: return "challenge";
	case Board::MODE_BLITZ: return "blitz";
	case Board::MODE_ZEN: return "zen";
	case Board::MODE_PUZZLE: return "puzzle";
	}
	return "unknown";
}

// The gem colours by Piece::mColor, as IMAGE_GEMS' cels: red, white, green, yellow, purple, orange, blue
const char kColorLetters[] = "rwgypob";

}  // namespace

// The screen a driver sees: a game (with its options over it), the credits, the mode select, a menu panel
// The board's rank bar while it is up, or null
static RankBonusWidget* TestRankBar(Board* theBoard)
{
	RankBonusWidget* aRank = dynamic_cast<RankBonusWidget*>(theBoard->mRankBonusWidget);
	return aRank != nullptr && aRank->mWidgetManager != nullptr ? aRank : nullptr;
}

std::string WinTwistApp::TestScreen()
{
	if (mCredits != nullptr)
		return "credits";
	if (mMainMenu != nullptr && mMainMenu->mEndStats != nullptr)
		return "endstats";  // the stats after a game, with their Continue
	if (mBoard != nullptr && !mBoard->mIsReplay)
		return mOptionsDialog != nullptr ? "options" : "game";
	if (dynamic_cast<Mandala*>(mMandalaWidget) != nullptr)
		return GetDialog(10000) != nullptr ? "continue" : "modeselect";  // the question whether to go on with a save
	if (mMainMenu != nullptr && mMainMenu->mParent != nullptr) {
		if (mMainMenu->mNewUserPanel != nullptr)
			return "newplayer";  // the name of a new player is asked (the first launch)
		switch (mMainMenu->mPanel) {
		case Menu::PANEL_MAIN: return "menu";
		case Menu::PANEL_PROFILE: return "profile";
		case Menu::PANEL_SCORES: return "highscores";
		case Menu::PANEL_OPTIONS: return "options";
		case Menu::PANEL_HELP: return "help";
		}
		return "menu";
	}
	return "loading";
}

// "goto <mode>" pressed Play: once the mode select is open, its button for the mode is pressed, as a player's click
// would (a saved game of the mode then asks to be continued, as it does for a player). Anything else on the way
// (another screen) drops the request.
void WinTwistApp::UpdateTestGoto()
{
	std::string aScreen = TestScreen();
	Mandala* aSelect = dynamic_cast<Mandala*>(mMandalaWidget);
	if (aSelect == nullptr) {
		if (aScreen != "menu")
			mTestGotoMode = -1;
		return;
	}
	if (aSelect->mState != 1)
		return;
	for (size_t i = 0; i < aSelect->mButtons.size(); i++) {
		if (aSelect->mButtons[i]->mMode == mTestGotoMode) {
			mTestGotoMode = -1;
			aSelect->ButtonDepress((int)i);
			return;
		}
	}
	mTestGotoMode = -1;
}

bool WinTwistApp::HandleTestCommand(const ScriptCommand& theCommand)
{
	const std::string& aCommand = theCommand.mCommand;
	const std::string& anArgs = theCommand.mArgs;
	Board* aBoard = (mBoard != nullptr && !mBoard->mIsReplay) ? mBoard : nullptr;

	if (aCommand == "state") {
		std::string aScreen = TestScreen();
		std::string aLine = "STATE {\"screen\":\"" + aScreen + "\"";
		// the app's update count, whether the window has the focus and whether it is windowed (for drivers that
		// check focus, minimize and Alt+Enter)
		aLine += StrFormat(",\"updates\":%d,\"active\":%s,\"windowed\":%s", mUpdateCount, mActive ? "true" : "false",
			mIsWindowed ? "true" : "false");
		if (aBoard != nullptr) {
			aLine += StrFormat(",\"mode\":\"%s\",\"level\":%d,\"score\":%d,\"moves\":%d,\"progress\":[%d,%d],\"busy\":%s,\"over\":%s",
				ModeName(aBoard->mGameMode), aBoard->mLevel, aBoard->mScore, aBoard->mTwistsMade,
				aBoard->mLevelProgress, aBoard->mLevelProgressGoal, aBoard->IsBusy() ? "true" : "false", aBoard->mGameOver ? "true" : "false");
			if (aBoard->mGameMode == Board::MODE_BLITZ)
				aLine += StrFormat(",\"timer\":%d", aBoard->mBlitzStartTime / 100);
			if (gWheelSpin != nullptr)
				aLine += ",\"wheel\":true";  // the second chance's wheel is up
			if (TestRankBar(aBoard) != nullptr)
				aLine += ",\"rank\":true";
			if (dynamic_cast<Mandala*>(mMandalaWidget) != nullptr && static_cast<Mandala*>(mMandalaWidget)->mMode != 0)
				aLine += ",\"zenjump\":true";  // the zen jump's mode select over the game  // the rank bar after a level or a game (and the new rank's badge)
			// a tip or the tutorial (+0x2584): its wait (Tutorial::STATE_), whether its dialog with buttons holds
			// the board (+0x210) and the square a forced move turns (+0x2404, +0x2400)
			if (aBoard->mTutorial != nullptr) {
				Tutorial* aTip = aBoard->mTutorial;
				aLine += StrFormat(",\"tip\":{\"wait\":%d,\"dialog\":%s", aTip->mState, aTip->mWaitsForPlayer ? "true" : "false");
				if (0 <= aBoard->mTipCol && 0 <= aBoard->mTipRow)
					aLine += StrFormat(",\"forced\":[%d,%d]", aBoard->mTipRow, aBoard->mTipCol);
				aLine += "}";
			}
			// the board: a row a string, a gem by its colour's letter (. for none); the specials listed apart
			std::string aRows;
			std::string aSpecials;
			for (int aRow = 0; aRow < aBoard->mRows; aRow++) {
				aRows += aRow == 0 ? "\"" : ",\"";
				for (int aCol = 0; aCol < aBoard->mCols; aCol++) {
					Piece* aPiece = aBoard->GetPieceAt(aRow, aCol);
					if (aPiece == nullptr || aPiece->mColor < 0 || aPiece->mColor > 6) {
						aRows += '.';
						continue;
					}
					aRows += kColorLetters[aPiece->mColor];
					std::string aKind;
					if (aPiece->IsFlagSet(Piece::FLAG_BOMB))
						aKind = StrFormat("bomb\",\"count\":%d", aPiece->mBombCount);
					else if (aPiece->IsFlagSet(Piece::FLAG_COAL))
						aKind = "coal\"";
					else if (aPiece->IsFlagSet(Piece::FLAG_DEVIL))
						aKind = "doom\"";
					else if (aPiece->IsFlagSet(Piece::FLAG_POWER))
						aKind = "flame\"";
					else if (aPiece->IsFlagSet(Piece::FLAG_ELECTRO))
						aKind = "lightning\"";
					else if (aPiece->IsFlagSet(Piece::FLAG_FRUIT))
						aKind = "fruit\"";
					if (aPiece->IsFlagSet(Piece::FLAG_LOCKED))
						aKind = aKind.empty() ? StrFormat("locked\",\"lock\":%d", aPiece->mLockCount)
						                      : aKind + StrFormat(",\"lock\":%d", aPiece->mLockCount);
					if (!aKind.empty())
						aSpecials += StrFormat("%s{\"row\":%d,\"col\":%d,\"kind\":\"%s}", aSpecials.empty() ? "" : ",",
							aRow, aCol, aKind.c_str());
				}
				aRows += "\"";
			}
			aLine += ",\"board\":[" + aRows + "],\"specials\":[" + aSpecials + "]";
		}
		Reply(aLine + "}");
		return true;
	}

	if (aCommand == "goto") {
		std::string aScreen = TestScreen();
		int aMode = ModeByName(anArgs);
		if (anArgs == "menu" || anArgs == "title") {
			if (aBoard != nullptr) {
				// the quit question's Yes: the game is saved and the menu comes back
				aBoard->DialogButtonDepress(1, 1000);
			} else if (aScreen == "options" || aScreen == "highscores" || aScreen == "help" || aScreen == "profile") {
				// the gate button that opened the panel closes it again
				int aButton = aScreen == "options" ? 5 : aScreen == "highscores" ? 4 : aScreen == "help" ? 2 : 1;
				mMainMenu->ButtonDepress(aButton);
			} else if (aScreen != "menu") {
				ReplyError(theCommand, "not from the screen " + aScreen);
				return true;
			}
		} else if (anArgs == "options" && aBoard != nullptr) {
			if (aScreen != "options")
				aBoard->mGoodTube->ButtonDepress(0);  // the tube's menu button: the options over the game
		} else if (anArgs == "options" || anArgs == "highscores" || anArgs == "help" || anArgs == "credits" ||
		           aMode != -1) {
			if (aScreen != "menu") {
				ReplyError(theCommand, "only from the main menu (now " + aScreen + ")");
				return true;
			}
			if (anArgs == "options")
				mMainMenu->ButtonDepress(5);
			else if (anArgs == "highscores")
				mMainMenu->ButtonDepress(4);
			else if (anArgs == "help")
				mMainMenu->ButtonDepress(2);
			else if (anArgs == "credits")
				ShowCredits();
			else if (aMode == Board::MODE_CHALLENGE) {
				ReplyError(theCommand, "challenges start from their own menu, not ported to this command");
				return true;
			} else if (!mProfile->IsModeUnlocked(aMode)) {
				ReplyError(theCommand, "the mode is locked (unlock all first)");
				return true;
			} else {
				// Play, then the mode's button once the mode select is open (UpdateTestGoto): the path a player's
				// clicks take, which sets what a game started from the menu depends on (mPlayPressed: Blitz's clock)
				mTestGotoMode = aMode;
				mMainMenu->ButtonDepress(0);
			}
		} else {
			ReplyError(theCommand, "unknown screen");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "unlock") {
		if (anArgs != "all") {
			ReplyError(theCommand, "only \"unlock all\"");
			return true;
		}
		// the modes, as the game's own unlocks set them in the profile (0x7337c2); challenges and awards are left
		for (int aMode = Board::MODE_CLASSIC; aMode <= Board::MODE_ZEN; aMode++)
			mProfile->UnlockMode(aMode);
		mProfile->Save();
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "move") {
		// The twist -autoplay's FindMove (0x6c14fc) would choose: rows top down, columns left to right, the first
		// turn that makes a line of three, one with a bomb in the line at once. FindMove moves the cursor over every
		// cell to ask, so this reads the gems into a grid of its own and turns them there: nothing in the game moves
		// and nothing is drawn from a generator. A turn is clockwise about the corner at (row + 1, col + 1).
		// The end of a game, as a player clicks through it: the rank bar (the new rank's Continue when its badge is
		// up, otherwise the bar itself once it waits), the stats' Continue, and on the high scores the gate button that
		// opened them, which closes them and leaves the main menu. None of them leads out of the game.
		std::string aScreen = TestScreen();
		auto aClick = [&](Widget* theWidget, const char* theKind) {
			Point aPos = theWidget->GetAbsPos();
			int aX = aPos.mX + theWidget->mWidth / 2;
			int aY = aPos.mY + theWidget->mHeight / 2;
			Reply(StrFormat("MOVE {\"kind\":\"%s\",\"input\":[{\"move\":[%d,%d]},{\"click\":[%d,%d]}]}", theKind, aX, aY,
				aX, aY));
		};
		if (aBoard != nullptr && TestRankBar(aBoard) != nullptr) {
			RankBonusWidget* aRank = TestRankBar(aBoard);
			AwardTelly* aTelly = aRank->mTelly;
			if (aTelly != nullptr && aTelly->mButton != nullptr && !aTelly->mButton->mDisabled &&
				aTelly->mWidgetManager != nullptr) {
				aClick(aTelly->mButton, "continue");
			} else if (aTelly == nullptr && 1 < aRank->mState && aRank->mState < 5) {
				int aX = aRank->mCenterX;
				int aY = (int)aRank->mBarY + 20;
				Reply(StrFormat("MOVE {\"kind\":\"continue\",\"input\":[{\"move\":[%d,%d]},{\"click\":[%d,%d]}]}", aX, aY,
					aX, aY));
			} else {
				Reply("MOVE null");
			}
			return true;
		}
		if (aScreen == "endstats") {
			EndStats* aStats = mMainMenu->mEndStats;
			if (aStats->mState == 1 && aStats->mContinueButton != nullptr && !aStats->mContinueButton->mDisabled)
				aClick(aStats->mContinueButton, "continue");
			else
				Reply("MOVE null");
			return true;
		}
		if (aScreen == "highscores" && aBoard == nullptr) {
			// after a game the bar under the scores (ShowBar) holds Play Again and Main Menu: its quit button leaves for
			// the main menu (the topmost, since the bar keeps an old pair under a new one)
			HighScoresPanel* aScores = mMainMenu != nullptr ? mMainMenu->mHighScoresPanel : nullptr;
			if (aScores != nullptr && aScores->mBar != nullptr && aScores->mBar->mParent != nullptr &&
				(aScores->mBar->mWidgetFlagsMod.mRemoveFlags & WIDGETFLAGS_ALLOW_MOUSE) == 0) {
				TwistButton* aQuit = nullptr;
				for (Widget* aChild : aScores->mBar->mWidgets) {
					TwistButton* aButton = dynamic_cast<TwistButton*>(aChild);
					if (aButton != nullptr && aButton->mId == HighScoresPanel::BUTTON_QUIT)
						aQuit = aButton;
				}
				if (aScores->mState == 0 && !aScores->mBarPressed && aQuit != nullptr && !aQuit->mDisabled)
					aClick(aQuit, "back");
				else
					Reply("MOVE null");
				return true;
			}
			if (mGates != nullptr && mGates->mScoresButton != nullptr && mGates->mButtonsOn &&
				!mGates->mScoresButton->mDisabled)
				aClick(mGates->mScoresButton, "back");
			else
				Reply("MOVE null");
			return true;
		}
		if (aBoard == nullptr) {
			Reply("MOVE null");
			return true;
		}
		// The zen jump (Board::ShowZenJump, once a player): the mode select over the game asks whether to go on or move
		// to the other mode; the board's own mode's button goes on (Classic's over Classic, Zen's over Zen)
		Mandala* aJump = dynamic_cast<Mandala*>(mMandalaWidget);
		if (aJump != nullptr && aJump->mMode != 0) {
			ModeButton* aStay = aJump->mButtons.size() == 4 ? aJump->mButtons[aJump->mMode == 1 ? 0 : 3] : nullptr;
			if (aJump->mState == 1 && aStay != nullptr && aStay->mVisible && !aStay->mDisabled)
				aClick(aStay, "continue");
			else
				Reply("MOVE null");
			return true;
		}
		// The second chance's wheel: -autoplay stops it as soon as its button can be pressed (WheelSpin::UpdateState).
		// Before that its tip ("one last chance") may hold it with a dialog, whose button comes below.
		bool aTipDialog = aBoard->mTutorial != nullptr && aBoard->mTutorial->mWaitsForPlayer;
		if (gWheelSpin != nullptr && !aTipDialog) {
			TwistButton* aStop = gWheelSpin->mButton;
			if (gWheelSpin->mState != 5 || aStop == nullptr || aStop->mDisabled) {
				Reply("MOVE null");
				return true;
			}
			int aX = gWheelSpin->mX + aStop->mX + aStop->mWidth / 2;
			int aY = gWheelSpin->mY + aStop->mY + aStop->mHeight / 2;
			Reply(StrFormat("MOVE {\"kind\":\"stop\",\"input\":[{\"move\":[%d,%d]},{\"click\":[%d,%d]}]}", aX, aY, aX, aY));
			return true;
		}
		// A tip or the tutorial: its dialog's first button once the dialog has glided into place, then nothing while
		// it waits on its seconds or a forced move has no square yet, and a forced move's own square (the board
		// turns no other while +0x2400 is set)
		if (aBoard->mTutorial != nullptr) {
			Tutorial* aTip = aBoard->mTutorial;
			TwistDialog* aDialog = aTip->mDialog;
			if (aTip->mWaitsForPlayer) {
				DialogButton* aButton = aDialog != nullptr ? aDialog->mYesButton : nullptr;
				if (aButton == nullptr || aDialog->mClosing || (0 <= aTip->mDialogX &&
					(aDialog->mX != aTip->mDialogX || aDialog->mY != aTip->mDialogY))) {
					Reply("MOVE null");
					return true;
				}
				int aX = aDialog->mX + aButton->mX + aButton->mWidth / 2;
				int aY = aDialog->mY + aButton->mY + aButton->mHeight / 2;
				Reply(StrFormat("MOVE {\"kind\":\"dialog\",\"input\":[{\"move\":[%d,%d]},{\"click\":[%d,%d]}]}", aX, aY, aX,
					aY));
				return true;
			}
			if (aTip->mState == Tutorial::STATE_SECONDS) {
				Reply("MOVE null");
				return true;
			}
			if (0 <= aBoard->mTipCol && 0 <= aBoard->mTipRow) {
				if (aBoard->IsBusy()) {
					Reply("MOVE null");
					return true;
				}
				int aX = aBoard->mX + aBoard->GetColX(aBoard->mTipCol) + gGemWidth;
				int aY = aBoard->mY + aBoard->GetRowY(aBoard->mTipRow) + gGemHeight;
				Reply(StrFormat("MOVE {\"kind\":\"forced\",\"row\":%d,\"col\":%d,\"input\":[{\"move\":[%d,%d]},"
					"{\"click\":[%d,%d]}]}", aBoard->mTipRow, aBoard->mTipCol, aX, aY, aX, aY));
				return true;
			}
			if (aTip->mState == Tutorial::STATE_MOVE) {
				Reply("MOVE null");
				return true;
			}
		}
		int aRows = aBoard->mRows;
		int aCols = aBoard->mCols;
		std::vector<int> aGrid(aRows * aCols, -1);  // a colour, or -1 for a gem that matches nothing
		std::vector<char> aFixed(aRows * aCols, 0);  // a locked gem (or none) keeps any turn from happening
		std::vector<char> aBomb(aRows * aCols, 0);
		for (int aRow = 0; aRow < aRows; aRow++) {
			for (int aCol = 0; aCol < aCols; aCol++) {
				Piece* aPiece = aBoard->GetPieceAt(aRow, aCol);
				int i = aRow * aCols + aCol;
				if (aPiece == nullptr) {
					aFixed[i] = 1;
					continue;
				}
				// a locked gem cannot turn, but it still makes a line with the gems turned next to it
				if (aPiece->IsFlagSet(Piece::FLAG_LOCKED))
					aFixed[i] = 1;
				if (!aPiece->IsFlagSet(Piece::FLAG_COAL) && !aPiece->IsFlagSet(Piece::FLAG_DEVIL) &&
					0 <= aPiece->mColor && aPiece->mColor <= 6)
					aGrid[i] = aPiece->mColor;
				aBomb[i] = aPiece->IsFlagSet(Piece::FLAG_BOMB) ? 1 : 0;
			}
		}
		auto aLineAt = [&](const std::vector<int>& theGrid, const std::vector<char>& theBombs, int theRow,
		                   int theCol, bool* theHasBomb) {
			// whether the gem at theRow, theCol is in a line of three or more, and whether a bomb is in it
			int aColor = theGrid[theRow * aCols + theCol];
			if (aColor < 0)
				return false;
			bool aFound = false;
			for (int aDir = 0; aDir < 2; aDir++) {
				int aDRow = aDir == 0 ? 0 : 1;
				int aDCol = aDir == 0 ? 1 : 0;
				int aStart = 0;
				while (true) {
					int aRow = theRow - (aStart + 1) * aDRow;
					int aCol = theCol - (aStart + 1) * aDCol;
					if (aRow < 0 || aCol < 0 || theGrid[aRow * aCols + aCol] != aColor)
						break;
					aStart++;
				}
				int anEnd = 0;
				while (true) {
					int aRow = theRow + (anEnd + 1) * aDRow;
					int aCol = theCol + (anEnd + 1) * aDCol;
					if (aRow >= aRows || aCol >= aCols || theGrid[aRow * aCols + aCol] != aColor)
						break;
					anEnd++;
				}
				if (aStart + anEnd + 1 >= 3) {
					aFound = true;
					for (int k = -aStart; k <= anEnd; k++)
						if (theBombs[(theRow + k * aDRow) * aCols + theCol + k * aDCol])
							*theHasBomb = true;
				}
			}
			return aFound;
		};
		int aBestRow = -1, aBestCol = -1;
		bool aBestBomb = false;
		for (int aRow = 0; aRow < aRows - 1 && !aBestBomb; aRow++) {
			for (int aCol = 0; aCol < aCols - 1; aCol++) {
				int aTL = aRow * aCols + aCol, aTR = aTL + 1, aBL = aTL + aCols, aBR = aBL + 1;
				if (aFixed[aTL] || aFixed[aTR] || aFixed[aBL] || aFixed[aBR])
					continue;
				std::vector<int> aTurned = aGrid;
				std::vector<char> aBombs = aBomb;
				// clockwise: top left to top right, top right to bottom right, bottom right to bottom left, and on
				aTurned[aTR] = aGrid[aTL]; aTurned[aBR] = aGrid[aTR]; aTurned[aBL] = aGrid[aBR]; aTurned[aTL] = aGrid[aBL];
				aBombs[aTR] = aBomb[aTL]; aBombs[aBR] = aBomb[aTR]; aBombs[aBL] = aBomb[aBR]; aBombs[aTL] = aBomb[aBL];
				bool aHasBomb = false;
				bool aMatch = false;
				const int aCells[4][2] = {{aRow, aCol}, {aRow, aCol + 1}, {aRow + 1, aCol}, {aRow + 1, aCol + 1}};
				for (int k = 0; k < 4; k++)
					aMatch = aLineAt(aTurned, aBombs, aCells[k][0], aCells[k][1], &aHasBomb) || aMatch;
				if (aMatch && (aBestRow < 0 || aHasBomb)) {
					aBestRow = aRow;
					aBestCol = aCol;
					aBestBomb = aHasBomb;
					if (aHasBomb)
						break;
				}
			}
		}
		if (aBestRow < 0 && !aBoard->IsBusy() && 0 <= aBoard->mBestMoveRow && 0 <= aBoard->mBestMoveCol) {
			// the grid found none, but the board's own FindMove (SettleTurn keeps its answer at +0x201c, +0x2020) did:
			// its match check (MoveCursor) is the game's, so its move goes first (a web Classic board had one the grid
			// missed)
			aBestRow = aBoard->mBestMoveRow;
			aBestCol = aBoard->mBestMoveCol;
		}
		bool aMatches = 0 <= aBestRow;
		if (aBestRow < 0 && !aBoard->IsBusy()) {
			// no turn makes a line: -autoplay then clicks a random square (Rand), which would draw from the game's own
			// generator; the bot takes a square it can turn instead, which makes no line but changes the board: the
			// squares counted top down and left to right, the one the board's move count picks, so that a board with
			// no line is not turned four times about one square back to where it was
			int aCount = 0;
			for (int aPass = 0; aPass < 2 && aBestRow < 0; aPass++) {
				int aIndex = 0;
				int aPick = aPass == 0 ? 0 : aBoard->mTwistsMade % aCount;
				for (int aRow = 0; aRow < aRows - 1 && aBestRow < 0; aRow++) {
					for (int aCol = 0; aCol < aCols - 1 && aBestRow < 0; aCol++) {
						int aTL = aRow * aCols + aCol;
						if (!aFixed[aTL] && !aFixed[aTL + 1] && !aFixed[aTL + aCols] && !aFixed[aTL + aCols + 1]) {
							if (aPass == 0)
								aCount++;
							else if (aIndex++ == aPick) {
								aBestRow = aRow;
								aBestCol = aCol;
							}
						}
					}
				}
				if (aCount == 0)
					break;
			}
		}
		if (aBestRow < 0) {
			Reply("MOVE null");
			return true;
		}
		// the click -autoplay makes for it: the corner the turn goes about, in the board's (and the game's) pixels
		int aX = aBoard->mX + aBoard->GetColX(aBestCol) + gGemWidth;
		int aY = aBoard->mY + aBoard->GetRowY(aBestRow) + gGemHeight;
		Reply(StrFormat("MOVE {\"kind\":\"twist\",\"row\":%d,\"col\":%d,\"bomb\":%s,\"match\":%s,\"input\":"
			"[{\"move\":[%d,%d]},{\"click\":[%d,%d]}]}", aBestRow, aBestCol, aBestBomb ? "true" : "false",
			aMatches ? "true" : "false", aX, aY, aX, aY));
		return true;
	}

	if (aCommand == "seed") {
		ReplyError(theCommand, "the game draws from several generators (the app's, each board's and 0x85b970); "
			"use -seed=<n> at the start");
		return true;
	}

	// The rest act on a game
	if (aCommand != "level" && aCommand != "set" && aCommand != "win" && aCommand != "lose" && aCommand != "gem")
		return false;
	if (aBoard == nullptr) {
		ReplyError(theCommand, "no game is on");
		return true;
	}

	if (aCommand == "gem") {
		// gem <row> <col> flame: the gem there becomes a flame gem at once, as a match of four makes one (the piece's
		// own MakePower, without the board's stats), so a driver can set one off with a real twist
		int aRow = -1, aCol = -1;
		char aKind[16] = {};
		if (std::sscanf(anArgs.c_str(), "%d %d %15s", &aRow, &aCol, aKind) != 3 || std::string(aKind) != "flame") {
			ReplyError(theCommand, "gem <row> <col> flame");
			return true;
		}
		Piece* aPiece = aRow >= 0 && aRow < aBoard->mRows && aCol >= 0 && aCol < aBoard->mCols ?
			aBoard->GetPieceAt(aRow, aCol) : nullptr;
		if (aPiece == nullptr || aBoard->IsBusy()) {
			ReplyError(theCommand, "no gem there, or the board is busy");
			return true;
		}
		aPiece->MakePower(0.0f);
		ReplyOK(theCommand);
		return true;
	}
	bool aLevels = aBoard->mGameMode == Board::MODE_CLASSIC || aBoard->mGameMode == Board::MODE_ZEN;

	if (aCommand == "set") {
		size_t aSpace = anArgs.find(' ');
		std::string aName = anArgs.substr(0, aSpace);
		int aValue = aSpace == std::string::npos ? 0 : std::atoi(anArgs.c_str() + aSpace + 1);
		if (aSpace == std::string::npos) {
			ReplyError(theCommand, "set <name> <value>");
			return true;
		}
		if (aName == "score") {
			aBoard->SetScore(aValue, true);
		} else if (aName == "timer" && aBoard->mGameMode == Board::MODE_BLITZ) {
			aBoard->mBlitzStartTime = aValue * 100;  // in hundredths of a second
			aBoard->mGoodTube->SetTime(aBoard->mBlitzStartTime, true);
		} else if (aName == "progress" && aLevels) {
			aBoard->mLevelProgress = aValue;
		} else {
			ReplyError(theCommand, "can set score, progress (classic, zen) and timer (blitz)");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}

	if (aBoard->mGameMode == Board::MODE_CHALLENGE) {
		ReplyError(theCommand, "not in a challenge");
		return true;
	}
	if (aBoard->IsBusy() || aBoard->mGameOver) {
		ReplyError(theCommand, "the board is busy (a move, a level's end or the game's end)");
		return true;
	}

	if (aCommand == "level" || aCommand == "win") {
		if (aBoard->mGameMode == Board::MODE_BLITZ) {
			if (aCommand == "level") {
				ReplyError(theCommand, "blitz has no levels");
				return true;
			}
			aBoard->mBlitzStartTime = 0;  // the clock runs out: blitz ends its own way
		} else {
			if (aCommand == "level") {
				int aLevel = std::atoi(anArgs.c_str());
				if (aLevel < 2) {
					ReplyError(theCommand, "level <n>, n from 2: the game goes up to it from the level before");
					return true;
				}
				aBoard->mLevel = aLevel - 1;
			}
			// the level's goal is met: the board's own update ends the level and starts the next (CheckLevelDone)
			aBoard->mLevelProgress = aBoard->mLevelProgressGoal;
			aBoard->CheckLevelDone();
		}
		ReplyOK(theCommand);
		return true;
	}

	// lose: blitz's clock runs out; the other modes end as when no move is left
	if (aBoard->mGameMode == Board::MODE_BLITZ)
		aBoard->mBlitzStartTime = 0;
	else
		aBoard->EndGame(false);
	ReplyOK(theCommand);
	return true;
}
