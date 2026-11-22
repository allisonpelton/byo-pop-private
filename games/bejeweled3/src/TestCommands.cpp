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
	if (aCommand == "state") {
		std::string aLine = "STATE {\"screen\":\"";
		aLine += mBoard != nullptr ? "classic" : "menu";
		aLine += "\"";
		if (mBoard != nullptr) {
			aLine += ",\"mode\":\"classic\",\"score\":" + std::to_string(mBoard->mScore) + ",\"board\":[";
			for (int aRow = 0; aRow < Board::ROWS; aRow++) {
				aLine += aRow == 0 ? "\"" : ",\"";
				for (int aCol = 0; aCol < Board::COLS; aCol++)
					aLine += ColorLetter(mBoard->mGrid[aRow][aCol]);
				aLine += "\"";
			}
			aLine += "],\"background\":" + std::to_string(mBoard->mBackgroundIndex);
		}
		aLine += "}";
		Reply(aLine);
		return true;
	}
	return SexyApp::HandleScriptCommand(theCommand);
}
