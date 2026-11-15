// port: test hooks: the standard script commands (docs/TEST_COMMANDS.md), reachable only from -script and the live channel
// (-commands natively, byopop_test_command on the web). A command that is never given changes nothing: no draw, no
// state. A command that is given draws no random number of the game's, except `seed`, which seeds them. Every command
// answers with one line on stdout: OK <command>, ERROR <command>: <reason>, STATE {...} or MOVE {...}.
//
//   state                 STATE {json}: screen, mode, level, score, over, busy, bonus, dialog, paused, inactive (paused because
//                         the window lost the focus: a driver gives it back), updates (the game's
//                         update count, for a driver that waits n updates), blocksleft and a
//                         board summary (columns, rows, the four next blocks, the grid)
//   goto <screen>         menu, options, help, highscores, credits, or a mode to start: regular, strategy, speed, timed
//                         (the timed mode's button is off the screen of TipTop Deluxe, so this is the one way in)
//   level <n>             starts level n of the game in play, as the game starts the next level after a summary
//   set <name> <value>    score, blocksleft, mode (the mode `move` picks on the new game screen), botseed
//   win, lose             the level is complete now; the game is over now (the game's own ending follows as in play)
//   seed <n>              the game's random numbers (the C library generator) are seeded with n
//   move                  MOVE {json}: what the bot would do next on this screen (Bot.h) and the real input that does it
//   unlock all            ERROR: the game has nothing locked
//
// `move` plays the whole way: the title's button, the main menu's Play, the new game screen's mode button, the tips
// dialog, a column for every block, the level summaries' button, the high score's name and OK. It answers a "wait" move
// (no input, `updates` to wait) while the game cannot take input, so a loop never meets MOVE null before the game is
// over; MOVE null is for the finished game (the game over screen's Play again button is up) and for the screens the bot
// never plays.

#include "TipTopApp.h"
#include "App.h"
#include "Board.h"
#include "Bot.h"
#include "Config.h"
#include "Game.h"
#include "Gui.h"
#include "Res.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

using namespace Sexy;

namespace
{

// the mode `move` chooses on the new game screen (-autoplaymode's numbers: 0 regular, 1 strategy, 2 speed) and the
// bot's own generator for the moves asked for
int gBotMode = 0;
unsigned int gBotSeed = 7;

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

void Error(const std::string& theCommand, const std::string& theReason)
{
	Reply("ERROR " + theCommand + ": " + theReason);
}

std::string Num(long long theValue)
{
	char aBuf[32];
	snprintf(aBuf, sizeof(aBuf), "%lld", theValue);
	return aBuf;
}

const char* ScreenName(int theScreen)
{
	switch (theScreen)
	{
	case tiptop::SCREEN_SPLASH: return "title";
	case tiptop::SCREEN_MAINMENU: return "menu";
	case tiptop::SCREEN_OPTIONS: return "options";
	case tiptop::SCREEN_INSTRUCTIONS: return "help";
	case tiptop::SCREEN_CREDITS: return "credits";
	case tiptop::SCREEN_GAME: return tiptop::gBonusRound != 0 ? "bonus" : "board";
	case tiptop::SCREEN_LEVELSUMMARY: return "summary";
	case tiptop::SCREEN_NEWGAME: return "newgame";
	case tiptop::SCREEN_HIGHSCORES: return "highscores";
	case tiptop::SCREEN_GAMEOVER: return "gameover";
	}
	return "none";
}

// The mode's name by the game's number (0 regular, 1 timed, 2 speed, 3 strategy)
const char* ModeName(uint32_t theMode)
{
	static const char* const kNames[] = {"regular", "timed", "speed", "strategy"};
	return theMode < 4 ? kNames[theMode] : "none";
}

bool OnBoardScreen(int theScreen)
{
	return theScreen == tiptop::SCREEN_GAME || theScreen == tiptop::SCREEN_LEVELSUMMARY ||
		theScreen == tiptop::SCREEN_GAMEOVER;
}

// A dialog is open over the screen (the tips, a question, the high score's name)
bool DialogOpen()
{
	return tiptop::gApp.mLayer != nullptr && tiptop::InTopModal(tiptop::gApp.mLayer) == 0;
}

// The game is over and nothing more is asked: the game over screen with its Play again button on top
bool GameIsOver()
{
	return tiptop::gApp.mScreen == tiptop::SCREEN_GAMEOVER && !DialogOpen() && BotButton(0x2d) != nullptr;
}

char KindChar(int theKind)
{
	if (theKind >= 0 && theKind <= 9)
		return (char)('0' + theKind);
	if (theKind >= 10 && theKind <= 16)
		return (char)('a' + theKind - 10);
	return '.';
}

std::string StateJson(int theUpdates)
{
	tiptop::App& anApp = tiptop::gApp;
	tiptop::Board* aBoard = anApp.mBoard;
	std::string aJson = std::string("{\"screen\":\"") + ScreenName(anApp.mScreen) + "\"";
	bool aBoardScreen = OnBoardScreen(anApp.mScreen) && aBoard != nullptr;
	aJson += std::string(",\"mode\":\"") + (aBoardScreen ? ModeName(tiptop::gGameMode) : "none") + "\"";
	aJson += std::string(",\"over\":") + (GameIsOver() ? "true" : "false");
	aJson += std::string(",\"dialog\":") + (DialogOpen() ? "true" : "false");
	aJson += std::string(",\"busy\":") + (tiptop::gBusy != 0 ? "true" : "false");
	aJson += std::string(",\"paused\":") + (tiptop::gPauseFlags != 0 ? "true" : "false");
	aJson += std::string(",\"inactive\":") + ((tiptop::gPauseFlags & 2) != 0 ? "true" : "false");
	aJson += ",\"updates\":" + Num(theUpdates);
	if (aBoardScreen)
	{
		aJson += ",\"level\":" + Num(aBoard->mState.mLevel) + ",\"score\":" + Num(aBoard->GetScore());
		aJson += std::string(",\"bonus\":") + (tiptop::gBonusRound != 0 ? "true" : "false");
		aJson += ",\"blocksleft\":" + Num(aBoard->mState.mBlocksLeft);
		aJson += std::string(",\"levelComplete\":") + (aBoard->mLevelComplete != 0 ? "true" : "false");
		aJson += std::string(",\"gameOver\":") + (aBoard->mGameOver != 0 ? "true" : "false");
		if (aBoard->mField != nullptr)
		{
			tiptop::Field* aField = aBoard->mField;
			int aColumns = (int)aField->mColumns;
			int aRows = (int)aField->mRows;
			aJson += ",\"board\":{\"columns\":" + Num(aColumns) + ",\"rows\":" + Num(aRows) + ",\"next\":[";
			for (int i = 0; i < 4; i++)
				aJson += (i == 0 ? "" : ",") + Num(aBoard->mNextBlocks[i].mKind);
			aJson += "],\"grid\":[";
			for (int r = 0; r < aRows; r++)
			{
				std::string aRow;
				for (int c = 0; c < aColumns; c++)
				{
					tiptop::Piece* p = aField->PieceAt((uint32_t)(r * aColumns + c));
					aRow += p != nullptr ? KindChar(p->mKind) : '.';
				}
				aJson += (r == 0 ? "\"" : ",\"") + aRow + "\"";
			}
			aJson += "]}";
		}
	}
	return aJson + "}";
}

// A click move in the bot's JSON: the pointer goes there first (a hover, which the game's buttons and its column cursor
// read), then the click; the steps are docs/TEST_COMMANDS.md's ordered `input` list, in game coordinates
std::string ClickMove(const char* theKind, int x, int y, int theUpdates, const std::string& theExtra = "")
{
	std::string aPoint = "[" + Num(x) + "," + Num(y) + "]";
	return std::string("{\"kind\":\"") + theKind + "\"" + theExtra + ",\"input\":[{\"move\":" + aPoint + "},{\"click\":" +
		aPoint + "}],\"updates\":" + Num(theUpdates) + "}";
}

std::string WaitMove(int theUpdates)
{
	return "{\"kind\":\"wait\",\"updates\":" + Num(theUpdates) + "}";
}

// The title screen's play button exists once its load bar is full (App.cpp's LoadBarProc): no entry the bar loads (the
// table's types 4, 2 and 8, as IsPreloaded has them) is left after the ones loaded so far
bool TitleLoaded()
{
	for (const res::Entry* e = &res::GameTable()[tiptop::gPreloaded]; e->mFlags != 0; e++)
		if (e->mType == 4 || e->mType == 2 || e->mType == 8)
			return false;
	return true;
}

std::string ButtonMove(const char* theKind, tiptop::Handler* h, int theUpdates)
{
	return ClickMove(theKind, h->mRect.x + h->mRect.w / 2, h->mRect.y + h->mRect.h / 2, theUpdates);
}

// The move on a paused game or level summary: a click on the pause image (the player's pause, flag 1; the image is a modal
// layer, so a dialog is open) takes it away; the window being inactive (flag 2) is the driver's to end, by giving the
// focus back, so the bot only waits for it
std::string PausedMove()
{
	if ((tiptop::gPauseFlags & 1) != 0 && DialogOpen())
		return ClickMove("unpause", tiptop::gConfig.mBlockgridCenterX, tiptop::gConfig.mBlockgridCenterY, 20);
	return WaitMove(10);
}

// What the bot does next, as the move's JSON, or an empty string for none. It reads the game and changes nothing.
std::string BotMove()
{
	tiptop::App& anApp = tiptop::gApp;
	switch (anApp.mScreen)
	{
	case tiptop::SCREEN_SPLASH:
		// "click here to play", once the load bar is full (a click before it is ignored): until then the move is to wait
		return TitleLoaded() ? ClickMove("start", 186, 288, 100) : WaitMove(10);
	case tiptop::SCREEN_MAINMENU:
		return ClickMove("play", 165, 88, 100);
	case tiptop::SCREEN_NEWGAME:
		// the Regular, Strategy and Speed buttons
		return gBotMode == 1 ? ClickMove("mode", 145, 260, 100) : gBotMode == 2 ? ClickMove("mode", 240, 325, 100) :
			ClickMove("mode", 232, 185, 100);
	case tiptop::SCREEN_GAME:
	{
		tiptop::Board* aBoard = anApp.mBoard;
		if (tiptop::gPauseFlags != 0)
			return PausedMove();
		if (DialogOpen())
		{
			// the tips (or another dialog) over the game: its default button
			tiptop::Handler* h = BotEnterTarget();
			return h != nullptr ? ButtonMove("dialog", h, 100) : WaitMove(20);
		}
		// not yet: the game is busy, a press is held by the column cursor (it taps as soon as the game is free), the
		// level or the game is ending
		if (aBoard == nullptr || aBoard->mField == nullptr || aBoard->mGameOver != 0 || aBoard->mLevelComplete != 0 ||
			aBoard->mEnding != 0 || tiptop::gBusy != 0 || (anApp.mColumnCursor != nullptr && anApp.mColumnCursor->u.mSlots[1] != 0))
			return WaitMove(10);
		// the bot's dice are its own, a function of the update, so asking twice gives the same move
		unsigned int aSeed = gBotSeed + (unsigned int)gApp->mUpdateCount * 2654435761u;
		int aColumn = BotChooseColumn(aBoard, aSeed);
		if (aColumn < 0)
			return WaitMove(10);
		int x, y;
		BotColumnToGame(aColumn, &x, &y);
		return ClickMove("drop", x, y, tiptop::gGameMode == 2 ? 40 : 60, ",\"column\":" + Num(aColumn));
	}
	case tiptop::SCREEN_LEVELSUMMARY:
	{
		if (tiptop::gPauseFlags != 0)
			return PausedMove();
		tiptop::Handler* h = BotEnterTarget();
		return h != nullptr ? ButtonMove("continue", h, 200) : WaitMove(20);
	}
	case tiptop::SCREEN_GAMEOVER:
	{
		if (GameIsOver())
			return "";
		if (DialogOpen())
		{
			// a new high score: its name, typed, and OK
			tiptop::Handler* h = BotButton(0xd);
			if (h == nullptr)
				return WaitMove(20);
			std::string aPoint = "[" + Num(h->mRect.x + h->mRect.w / 2) + "," + Num(h->mRect.y + h->mRect.h / 2) + "]";
			return "{\"kind\":\"name\",\"text\":\"Bot\",\"input\":[{\"key\":\"B\"},{\"key\":\"o\"},{\"key\":\"t\"},{\"move\":" +
				aPoint + "},{\"click\":" + aPoint + "}],\"updates\":200}";
		}
		return WaitMove(20);
	}
	default:
		break;
	}
	return "";
}

// Starts a game of the mode (the game's command 0x20-0x23: the level list, the new game and the game
// screen)
void StartMode(uint32_t theCommand)
{
	if (tiptop::gApp.mScreen != tiptop::SCREEN_MAINMENU)
		tiptop::gApp.SetScreen(tiptop::SCREEN_MAINMENU);
	tiptop::Command(nullptr, theCommand);
}

} // namespace

bool TipTopApp::HandleTestCommand(const ScriptCommand& theCommand)
{
	const std::string& aName = theCommand.mCommand;
	std::istringstream anArgs(theCommand.mArgs);
	std::string anArg;
	anArgs >> anArg;
	std::string aLine = aName + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);
	tiptop::App& anApp = tiptop::gApp;

	if (aName == "state")
	{
		Reply("STATE " + StateJson(mUpdateCount));
		return true;
	}
	if (aName == "move")
	{
		std::string aMove = BotMove();
		Reply(aMove.empty() ? "MOVE null" : "MOVE " + aMove);
		return true;
	}
	if (aName == "seed")
	{
		if (anArg.empty())
			Error(aLine, "seed needs <n>");
		else
		{
			tiptop::SRand((uint32_t)strtoul(anArg.c_str(), nullptr, 0));
			Reply("OK " + aLine);
		}
		return true;
	}
	if (aName == "unlock")
	{
		Error(aLine, "the game has nothing locked (the timed mode is reached with goto timed)");
		return true;
	}
	if (aName == "goto")
	{
		if (anApp.mScreen == tiptop::SCREEN_SPLASH)
		{
			// the title's button leads to the main menu, which the goto then leaves as it would from there
			if (!TitleLoaded())
			{
				Error(aLine, "the title screen is still loading: wait for its button (move waits too)");
				return true;
			}
			anApp.SetScreen(tiptop::SCREEN_MAINMENU);
		}
		if (DialogOpen())
		{
			Error(aLine, "a dialog is open: answer it first");
			return true;
		}
		if (anArg == "menu")
			anApp.SetScreen(tiptop::SCREEN_MAINMENU);
		else if (anArg == "options")
			anApp.SetScreen(tiptop::SCREEN_OPTIONS);
		else if (anArg == "help")
			anApp.SetScreen(tiptop::SCREEN_INSTRUCTIONS);
		else if (anArg == "highscores")
			anApp.SetScreen(tiptop::SCREEN_HIGHSCORES);
		else if (anArg == "credits")
			anApp.SetScreen(tiptop::SCREEN_CREDITS);
		else if (anArg == "regular")
			StartMode(0x20);
		else if (anArg == "speed")
			StartMode(0x21);
		else if (anArg == "timed")
			StartMode(0x22);
		else if (anArg == "strategy")
			StartMode(0x23);
		else
		{
			Error(aLine, "unknown screen (menu, options, help, highscores, credits, regular, strategy, speed, timed)");
			return true;
		}
		Reply("OK " + aLine);
		return true;
	}
	if (aName == "level")
	{
		int aLevel = anArg.empty() ? 0 : atoi(anArg.c_str());
		if (aLevel < 1)
			Error(aLine, "level needs <n>, 1 or more");
		else if (anApp.mScreen != tiptop::SCREEN_GAME || anApp.mBoard == nullptr)
			Error(aLine, "no game is on its board");
		else if (DialogOpen() || tiptop::gBusy != 0 || tiptop::gBonusRound != 0 || anApp.mBoard->mGameOver != 0 ||
			anApp.mBoard->mLevelComplete != 0)
			Error(aLine, "the game is not between moves (a dialog, an animation, a bonus round or the end of the level)");
		else
		{
			// as the game goes on after a bonus round (App::Frame): the next level, the game screen built again
			anApp.mBoard->mState.mLevel = (uint32_t)(aLevel - 1);
			anApp.NextLevel();
			anApp.SetScreen(tiptop::SCREEN_NONE);
			anApp.SetScreen(tiptop::SCREEN_GAME);
			Reply("OK " + aLine);
		}
		return true;
	}
	if (aName == "set")
	{
		std::string aValue;
		anArgs >> aValue;
		if (anArg == "mode")
		{
			if (aValue == "regular" || aValue == "strategy" || aValue == "speed")
			{
				gBotMode = aValue == "strategy" ? 1 : aValue == "speed" ? 2 : 0;
				Reply("OK " + aLine);
			}
			else
				Error(aLine, "mode is regular, strategy or speed (the one `move` picks on the new game screen)");
		}
		else if (anArg == "botseed")
		{
			gBotSeed = (unsigned int)strtoul(aValue.c_str(), nullptr, 0);
			Reply("OK " + aLine);
		}
		else if ((anArg == "score" || anArg == "blocksleft") && !aValue.empty())
		{
			if (anApp.mBoard == nullptr || anApp.mBoard->mField == nullptr || !OnBoardScreen(anApp.mScreen))
				Error(aLine, "no game is in play");
			else
			{
				if (anArg == "score")
					anApp.mBoard->mField->mScore = (uint32_t)strtoul(aValue.c_str(), nullptr, 0);
				else
					anApp.mBoard->mState.mBlocksLeft = atoi(aValue.c_str());
				Reply("OK " + aLine);
			}
		}
		else
			Error(aLine, "set needs score, blocksleft, mode or botseed and a value");
		return true;
	}
	if (aName == "win" || aName == "lose")
	{
		tiptop::Board* aBoard = anApp.mBoard;
		if (anApp.mScreen != tiptop::SCREEN_GAME || aBoard == nullptr || aBoard->mField == nullptr)
			Error(aLine, "no game is on its board");
		else if (DialogOpen() || tiptop::gBusy != 0 || aBoard->mGameOver != 0 || aBoard->mLevelComplete != 0 || aBoard->mEnding != 0)
			Error(aLine, "the game is not between moves (a dialog, an animation or the end of the level)");
		else if (tiptop::gBonusRound != 0)
			Error(aLine, "the bonus round has no win or lose");
		else
		{
			// the flags the board sets itself when the level is won or the game lost (Board::Draw); the game screen
			// takes it from there
			if (aName == "win")
				aBoard->mLevelComplete = 1;
			else
			{
				aBoard->mGameOver = 1;
				if (tiptop::gGameMode == 1)
					anApp.SetCrossed();
			}
			Reply("OK " + aLine);
		}
		return true;
	}
	return false;
}
