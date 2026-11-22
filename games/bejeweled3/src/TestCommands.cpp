// The standard test commands (docs/TEST_COMMANDS.md) of Bejeweled 3, handled by the app's HandleScriptCommand. A command
// that is never given changes nothing. Each answers on SDL's log with one line: STATE {...} for state, OK <command> when
// it has done its work, ERROR <command>: <reason> when it cannot.
#include "Bej3App.h"

#include "Board.h"
#include "Common.h"

#include <SDL.h>

#include <cstdlib>
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
	if (aCommand == "state") {
		std::string aLine = "STATE {\"screen\":\"";
		aLine += mBoard != nullptr ? "classic" : "menu";
		aLine += "\"";
		if (mBoard != nullptr) {
			aLine += ",\"mode\":\"classic\",\"score\":" + std::to_string(mBoard->mScore) + ",\"points\":" + std::to_string(mBoard->mScorePoints) +
				",\"movematches\":" + std::to_string(mBoard->mMoveRecords.empty() ? 0 : mBoard->mMoveRecords.back().mMatchCount) +
				",\"matches\":" + std::to_string(mBoard->mMatchGroups.size()) + ",\"swaps\":" + std::to_string(mBoard->mSwaps.size()) +
				",\"board\":[";
			for (int aRow = 0; aRow < Board::ROWS; aRow++) {
				aLine += aRow == 0 ? "\"" : ",\"";
				for (int aCol = 0; aCol < Board::COLS; aCol++)
					aLine += ColorLetter(mBoard->mGrid[aRow][aCol]);
				aLine += "\"";
			}
			aLine += "],\"background\":" + std::to_string(mBoard->mBackgroundIndex);
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
