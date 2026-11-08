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
#include "WheelSpin.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace Sexy;

namespace {

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
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
std::string WinTwistApp::TestScreen()
{
	if (mCredits != nullptr)
		return "credits";
	if (mBoard != nullptr && !mBoard->mUnknown37C4)
		return mOptionsDialog != nullptr ? "options" : "game";
	if (dynamic_cast<Mandala*>(mUnknownF24) != nullptr)
		return GetDialog(10000) != nullptr ? "continue" : "modeselect";  // the question whether to go on with a save
	if (mMainMenu != nullptr && mMainMenu->mParent != nullptr) {
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
	Mandala* aSelect = dynamic_cast<Mandala*>(mUnknownF24);
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
	Board* aBoard = (mBoard != nullptr && !mBoard->mUnknown37C4) ? mBoard : nullptr;

	if (aCommand == "state") {
		std::string aScreen = TestScreen();
		std::string aLine = "STATE {\"screen\":\"" + aScreen + "\"";
		if (aBoard != nullptr) {
			aLine += StrFormat(",\"mode\":\"%s\",\"level\":%d,\"score\":%d,\"moves\":%d,\"progress\":[%d,%d],\"busy\":%s,\"over\":%s",
				ModeName(aBoard->mGameMode), aBoard->mUnknown158, aBoard->mUnknown168, aBoard->mUnknown23C,
				aBoard->mUnknown200, aBoard->mUnknown204, aBoard->IsBusy() ? "true" : "false", aBoard->mUnknown36B0 ? "true" : "false");
			if (aBoard->mGameMode == Board::MODE_BLITZ)
				aLine += StrFormat(",\"timer\":%d", aBoard->mUnknown3458 / 100);
			if (gWheelSpin != nullptr)
				aLine += ",\"wheel\":true";  // the second chance's wheel is up
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
		if (aBoard == nullptr) {
			Reply("MOVE null");
			return true;
		}
		// The second chance's wheel: -autoplay stops it as soon as its button can be pressed (WheelSpin::UpdateState)
		if (gWheelSpin != nullptr) {
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
		int aRows = aBoard->mRows;
		int aCols = aBoard->mCols;
		std::vector<int> aGrid(aRows * aCols, -1);  // a colour, or -1 for a gem that matches nothing
		std::vector<char> aFixed(aRows * aCols, 0);  // a locked gem (or none) keeps any turn from happening
		std::vector<char> aBomb(aRows * aCols, 0);
		for (int aRow = 0; aRow < aRows; aRow++) {
			for (int aCol = 0; aCol < aCols; aCol++) {
				Piece* aPiece = aBoard->GetPieceAt(aRow, aCol);
				int i = aRow * aCols + aCol;
				if (aPiece == nullptr || aPiece->IsFlagSet(Piece::FLAG_LOCKED)) {
					aFixed[i] = 1;
					continue;
				}
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
		if (aBestRow < 0) {
			Reply("MOVE null");
			return true;
		}
		// the click -autoplay makes for it: the corner the turn goes about, in the board's (and the game's) pixels
		int aX = aBoard->mX + aBoard->GetColX(aBestCol) + gGemWidth;
		int aY = aBoard->mY + aBoard->GetRowY(aBestRow) + gGemHeight;
		Reply(StrFormat("MOVE {\"kind\":\"twist\",\"row\":%d,\"col\":%d,\"bomb\":%s,\"input\":[{\"move\":[%d,%d]},"
			"{\"click\":[%d,%d]}]}", aBestRow, aBestCol, aBestBomb ? "true" : "false", aX, aY, aX, aY));
		return true;
	}

	if (aCommand == "seed") {
		ReplyError(theCommand, "the game draws from several generators (the app's, each board's and 0x85b970); "
			"use -seed=<n> at the start");
		return true;
	}

	// The rest act on a game
	if (aCommand != "level" && aCommand != "set" && aCommand != "win" && aCommand != "lose")
		return false;
	if (aBoard == nullptr) {
		ReplyError(theCommand, "no game is on");
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
			aBoard->mUnknown3458 = aValue * 100;  // in hundredths of a second
			aBoard->mGoodTube->SetTime(aBoard->mUnknown3458, true);
		} else if (aName == "progress" && aLevels) {
			aBoard->mUnknown200 = aValue;
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
	if (aBoard->IsBusy() || aBoard->mUnknown36B0) {
		ReplyError(theCommand, "the board is busy (a move, a level's end or the game's end)");
		return true;
	}

	if (aCommand == "level" || aCommand == "win") {
		if (aBoard->mGameMode == Board::MODE_BLITZ) {
			if (aCommand == "level") {
				ReplyError(theCommand, "blitz has no levels");
				return true;
			}
			aBoard->mUnknown3458 = 0;  // the clock runs out: blitz ends its own way
		} else {
			if (aCommand == "level") {
				int aLevel = std::atoi(anArgs.c_str());
				if (aLevel < 2) {
					ReplyError(theCommand, "level <n>, n from 2: the game goes up to it from the level before");
					return true;
				}
				aBoard->mUnknown158 = aLevel - 1;
			}
			// the level's goal is met: the board's own update ends the level and starts the next (CheckLevelDone)
			aBoard->mUnknown200 = aBoard->mUnknown204;
			aBoard->CheckLevelDone();
		}
		ReplyOK(theCommand);
		return true;
	}

	// lose: blitz's clock runs out; the other modes end as when no move is left
	if (aBoard->mGameMode == Board::MODE_BLITZ)
		aBoard->mUnknown3458 = 0;
	else
		aBoard->EndGame(false);
	ReplyOK(theCommand);
	return true;
}
