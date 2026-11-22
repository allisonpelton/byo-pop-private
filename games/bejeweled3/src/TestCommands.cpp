// The standard test commands (docs/TEST_COMMANDS.md) of Bejeweled 3, handled by the app's HandleScriptCommand. A command
// that is never given changes nothing. Each answers on SDL's log with one line: STATE {...} for state, OK <command> when
// it has done its work, ERROR <command>: <reason> when it cannot.
#include "Bej3App.h"

#include "Board.h"
#include "Common.h"
#include "MainMenu.h"
#include "PlayMenu.h"

#include <SDL.h>

#include <cstdlib>
#include <cstring>
#include <string>

using namespace Sexy;

namespace {

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
	if (aCommand == "goto") {
		if (theCommand.mArgs == "classic") {
			// the game is reached as from the screens: PLAY is pressed (the play menu opens) and then the Classic ball
			if (mMainMenu != nullptr && mMainMenu->mPlayMenu != nullptr && !mMainMenu->mPlayMenu->IsActive())
				mMainMenu->OpenPlayMenu();
			StartClassic();
			ReplyOK(theCommand);
		}
		else
			ReplyError(theCommand, "only classic is made yet");
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
		// through the same makers a run uses, then lets it go off at once (its countdown, +0x184, is put to 0)
		int aCol = -1;
		int aRow = -1;
		char aLetter[8] = "";
		char aKind[16] = "";
		int aRead = mBoard == nullptr ? 0 : sscanf(theCommand.mArgs.c_str(), "%d %d %7s %15s", &aCol, &aRow, aLetter, aKind);
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
		aPiece->mSpecialCountdown = 0;
		ReplyOK(theCommand);
		return true;
	}
	if (aCommand == "move") {
		// move: the bot's move, the first the exe's move search (0x75a8e0, rows from the top) finds, as the two clicks of the player
		// (the gem is picked up, then its neighbour is clicked); it changes nothing. MOVE null when no move is left or the board is not at rest
		BoardMove aMove;
		if (mBoard == nullptr || mBoard->mGameOver || !mBoard->IsSettled() || !mBoard->FindMove(&aMove, 0, false, nullptr, false)) {
			Reply("MOVE null");
			return true;
		}
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
		aLine += mBoard != nullptr ? "classic" : "menu";
		aLine += "\"";
		if (mBoard != nullptr) {
			aLine += ",\"mode\":\"classic\",\"score\":" + std::to_string(mBoard->mScore) + ",\"points\":" + std::to_string(mBoard->mScorePoints) +
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
		}
		aLine += "}";
		Reply(aLine);
		return true;
	}
	return SexyApp::HandleScriptCommand(theCommand);
}
