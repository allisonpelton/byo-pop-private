// port: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script or live command
// channel through HandleScriptCommand. A command that is never given changes nothing. Each answers on stdout with one
// line: STATE {...}, MOVE {...}, OK <command> or ERROR <command>: <reason>.
//
// The game runs its own loop (PGApp::mainLoop and each dialog's doModal), and a command arrives while that loop sleeps
// in the framework (NingPoHost::Sleep). So a command that acts on the game is answered at once and then run at the
// game's next read of its events (NingPoApp::EventRead), in the game's own loop, through the game's own code: the
// title's start, New Game without its question, the dialogs' own functions, the last pair's match. A command that acts
// is refused while a dialog is open; the dialogs are answered by real input (`move` says which).

#include "NingPoApp.h"
#include "NingPo.h"
#include "PGDialog.h"
#include "WidgetManager.h"
#include "platform/Audio.h"
#include "platform/MemReport.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <typeinfo>

using namespace Sexy;
using namespace ningpo;

namespace {

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

std::string CommandText(const ScriptCommand& theCommand)
{
	return theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);
}

void ReplyOK(const ScriptCommand& theCommand)
{
	Reply("OK " + CommandText(theCommand));
}

void ReplyError(const ScriptCommand& theCommand, const std::string& theReason)
{
	Reply("ERROR " + CommandText(theCommand) + ": " + theReason);
}

const char* ScreenName(int theState)
{
	switch (theState)
	{
	case STATE_TITLE: return "title";
	case STATE_PLAYING: return "playing";
	case STATE_WON: return "won";
	case STATE_GAME_OVER: return "gameover";
	default: return "exit";
	}
}

// The open dialog by a name a driver can use, or "" when none is open. The name entry and the yes/no questions share
// a class (ClickDialog); the name entry is the one on an end screen.
std::string DialogName(NingPo* theGame)
{
	pg::PGDialog* aDialog = pg::TopDialog();
	if (aDialog == nullptr)
		return "";
	std::string aType = typeid(*aDialog).name();
	bool aGame = aType.find("ningpo") != std::string::npos;
	if (aType.find("BestTimesDialog") != std::string::npos)
		return "besttimes";
	if (aType.find("NoMovesDialog") != std::string::npos)
		return "nomoves";
	if (aType.find("OptionsDialog") != std::string::npos)
		return "options";
	if (aType.find("Slideshow") != std::string::npos)
		return "rules";
	if (aType.find("StartupDialog") != std::string::npos)
		return aGame ? "start" : "splash";
	if (aType.find("ClickDialog") != std::string::npos)
		return (theGame->mState == STATE_WON || theGame->mState == STATE_GAME_OVER) ? "name" : "question";
	if (aType.find("MessageDialog") != std::string::npos)
		return "question";
	return "dialog";
}

// The seconds the clock shows, as NingPo::tick counts them (without the minutes' wrap), or the final time once the
// game has ended
int TimerSeconds(NingPo* theGame)
{
	if (theGame->mState == STATE_WON || theGame->mState == STATE_GAME_OVER)
		return theGame->mFinalTime;
	if (theGame->mState != STATE_PLAYING)
		return 0;
	pg::PyTime aPlayed = pg::PyTimeSub(theGame->mEventTime, theGame->mGameStart);
	return ((int)aPlayed - (int)theGame->getTime()) / 1000 + theGame->mPenalty;
}

// A point of a tile's face where a click picks that tile, as a click finds it (Board::tileAt, NingPo::onLeftClick):
// the face's middle, or else the point nearest it, since a tile on a higher layer may cover part of a free one
void TileCentre(Board& theBoard, Tile* theTile, int* theX, int* theY)
{
	PGRect aFace;
	theTile->getFaceRect(&aFace);
	*theX = (aFace.left + aFace.right) / 2;
	*theY = (aFace.top + aFace.bottom) / 2;
	int aBest = -1;
	for (int y = aFace.top + 2; y < aFace.bottom - 2; y++)
		for (int x = aFace.left + 2; x < aFace.right - 2; x++)
		{
			PGPoint aPoint = {x, y};
			int aDistance = (x - (aFace.left + aFace.right) / 2) * (x - (aFace.left + aFace.right) / 2) +
				(y - (aFace.top + aFace.bottom) / 2) * (y - (aFace.top + aFace.bottom) / 2);
			if ((aBest < 0 || aDistance < aBest) && theBoard.tileAt(&aPoint, true) == theTile)
			{
				aBest = aDistance;
				*theX = x;
				*theY = y;
			}
		}
}

std::string ClickMove(const char* theKind, int theX, int theY)
{
	char aLine[256];
	snprintf(aLine, sizeof(aLine), "MOVE {\"kind\":\"%s\",\"input\":[{\"move\":[%d,%d]},{\"click\":[%d,%d]}]}", theKind,
		theX, theY, theX, theY);
	return aLine;
}

// The middle of a dialog item's rectangle (kept in screen coordinates), where a click hits it
void ItemCentre(const pg::PGDialogItem* theItem, int* theX, int* theY)
{
	const PGRect* aRect = theItem->getRect();
	*theX = (aRect->left + aRect->right) / 2;
	*theY = (aRect->top + aRect->bottom) / 2;
}

// The open dialog's visible buttons in their order, as a JSON list of their middles ("[]" with no dialog open)
std::string DialogButtons()
{
	std::string aList = "[";
	pg::PGDialog* aDialog = pg::TopDialog();
	for (pg::PGDialogItem* anItem = aDialog != nullptr ? aDialog->mFirstItem : nullptr; anItem != nullptr;
		anItem = anItem->mNext)
	{
		if (!anItem->isVisible() || dynamic_cast<pg::PGDialogButton*>(anItem) == nullptr)
			continue;
		int x, y;
		ItemCentre(anItem, &x, &y);
		char aPoint[32];
		snprintf(aPoint, sizeof(aPoint), "%s[%d,%d]", aList.size() > 1 ? "," : "", x, y);
		aList += aPoint;
	}
	return aList + "]";
}

} // namespace

void NingPoApp::EventRead()
{
	UpdateStateLog();
	// a test command's action, at the game's read of its events (above); taken first, so that a dialog it opens, whose
	// own loop reads events too, does not run it again
	if (mTestAction && pg::TopDialog() == nullptr)
	{
		std::function<void()> anAction;
		anAction.swap(mTestAction);
		anAction();
	}
}

bool NingPoApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aCommand = theCommand.mCommand;
	const std::string& anArgs = theCommand.mArgs;
	NingPo* aGame = mGame;
	if (aGame == nullptr || !mGameRunning)
	{
		if (aCommand == "state" || aCommand == "move" || aCommand == "goto" || aCommand == "level" ||
			aCommand == "unlock" || aCommand == "set" || aCommand == "win" || aCommand == "lose" || aCommand == "seed")
		{
			ReplyError(theCommand, "the game has not started yet");
			return true;
		}
		return false;
	}
	std::string aDialog = DialogName(aGame);

	if (aCommand == "state")
	{
		char aLine[1024];
		snprintf(aLine, sizeof(aLine), "STATE {\"screen\":\"%s\",\"dialog\":%s%s%s,\"mode\":\"mahjong\","
			"\"level\":%d,\"layout\":%d,\"tileset\":%d,\"background\":%d,\"timer\":%d,\"penalty\":%d,\"moves\":%d,"
			"\"tiles\":%d,\"hints\":%d,\"undos\":%d,\"shuffles\":%d,\"paused\":%s,\"fullscreen\":%s,\"cheat\":%s,\"over\":%s,"
			"\"updates\":%d,\"buttons\":%s}",
			ScreenName(aGame->mState), aDialog.empty() ? "" : "\"", aDialog.empty() ? "null" : aDialog.c_str(),
			aDialog.empty() ? "" : "\"", aGame->mLayout, aGame->mLayout, aGame->mTileset, aGame->mBackground,
			TimerSeconds(aGame), aGame->mPenalty, aGame->mBoard.mMoves, aGame->mBoard.mTilesLeft, aGame->mHints,
			aGame->mUndos, aGame->mShuffles, aGame->mPaused ? "true" : "false", aGame->mFullscreen != 0 ? "true" : "false", aGame->mCheat ? "true" : "false",
			(aGame->mState == STATE_WON || aGame->mState == STATE_GAME_OVER) ? "true" : "false", mUpdateCount,
			DialogButtons().c_str());
		Reply(aLine);
		return true;
	}

	if (aCommand == "move")
	{
		// The bot: the first pair Board::findMove finds (the hint's first), a click on each tile; in the dialogs and on
		// the end screens, the input that goes on (End Game, the continue arrow, a name, OK). It changes nothing.
		if (aDialog == "start")
			Reply(ClickMove("start", 320, 425));
		else if (aDialog == "nomoves")
			Reply(ClickMove("endgame", 385, 368));
		else if (aDialog == "name")
		{
			// the name: the one offered cleared (it holds at most 12 letters), "bot", Enter
			std::string aLine = "MOVE {\"kind\":\"name\",\"input\":[";
			for (int i = 0; i < 16; i++)
				aLine += "{\"key\":\"Backspace\"},";
			Reply(aLine + "{\"key\":\"b\"},{\"key\":\"o\"},{\"key\":\"t\"},{\"key\":\"Enter\"}],\"updates\":5}");
		}
		else if (aDialog == "besttimes")
			Reply(ClickMove("ok", 495, 435));
		else if (!aDialog.empty())
		{
			// another dialog (the rules, a question): the button Enter would click, which closes it or says no, or
			// nothing when it has none. The options' default is Best Times, so there it is Done, their last item
			// (OptionsDialog::setup).
			pg::PGDialogItem* aDefault = pg::TopDialog()->mDefault;
			if (aDialog == "options")
				for (aDefault = pg::TopDialog()->mFirstItem; aDefault != nullptr && aDefault->mNext != nullptr;)
					aDefault = aDefault->mNext;
			int x, y;
			if (aDefault == nullptr || !aDefault->isVisible())
				Reply("MOVE null");
			else
			{
				ItemCentre(aDefault, &x, &y);
				Reply(ClickMove("default", x, y));
			}
		}
		else if (aGame->mState == STATE_WON || aGame->mState == STATE_GAME_OVER)
			Reply(ClickMove("continue", 270, 345));
		else if (aGame->mPaused)
		{
			// a click on the board ends the pause (NingPo::onLeftClick), on the title too after a focus loss
			const PGRect& aBoard = aGame->mBoardRect;
			Reply(ClickMove("resume", (aBoard.left + aBoard.right) / 2, (aBoard.top + aBoard.bottom) / 2));
		}
		else if (aGame->mState == STATE_TITLE)
			Reply(ClickMove("play", 340, 440));
		else if (aGame->mState != STATE_PLAYING || aGame->mPaused)
			Reply("MOVE null");
		else
		{
			Tile* aTile = nullptr;
			Tile* anOther = nullptr;
			if (!aGame->mBoard.findMove(&aTile, &anOther, 0))
			{
				Reply("MOVE null");
				return true;
			}
			int x0, y0, x1, y1;
			TileCentre(aGame->mBoard, aTile, &x0, &y0);
			TileCentre(aGame->mBoard, anOther, &x1, &y1);
			// a tile already selected is clicked only once more, to pick its match
			bool aSelected = aGame->mBoard.mSelected == aTile;
			char aLine[512];
			if (aSelected)
				snprintf(aLine, sizeof(aLine), "MOVE {\"kind\":\"pair\",\"tiles\":[%d,%d],\"input\":[{\"move\":[%d,%d]},"
					"{\"click\":[%d,%d]}],\"updates\":5}", (int)(aTile - aGame->mBoard.mTiles),
					(int)(anOther - aGame->mBoard.mTiles), x1, y1, x1, y1);
			else
				snprintf(aLine, sizeof(aLine), "MOVE {\"kind\":\"pair\",\"tiles\":[%d,%d],\"input\":[{\"move\":[%d,%d]},"
					"{\"click\":[%d,%d]},{\"move\":[%d,%d]},{\"click\":[%d,%d]}],\"updates\":5}",
					(int)(aTile - aGame->mBoard.mTiles), (int)(anOther - aGame->mBoard.mTiles), x0, y0, x0, y0, x1, y1,
					x1, y1);
			Reply(aLine);
		}
		return true;
	}

	if (aCommand == "seed")
	{
		char* anEnd = nullptr;
		unsigned long aSeed = strtoul(anArgs.c_str(), &anEnd, 10);
		if (anArgs.empty() || *anEnd != '\0')
		{
			ReplyError(theCommand, "needs a number");
			return true;
		}
		// as -seed=<n>: the generator that deals and shuffles (PGApp::mRandom), not seeded again by the first event
		aGame->mRandom.seed((uint32_t)aSeed);
		aGame->mRandomSeeded = true;
		ReplyOK(theCommand);
		return true;
	}

	// the game's picture drawn now, as the native screenshot command draws it: the web page's shots read the canvas,
	// which -fast draws only every few updates (tools/shots/tour.py's web driver asks for it before each shot)
	if (aCommand == "redraw")
	{
		mWidgetManager->MarkAllDirty();
		DrawDirtyStuff();
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "unlock")
	{
		ReplyError(theCommand, "nothing is locked: the port plays the registered game");
		return true;
	}

	if (aCommand != "goto" && aCommand != "level" && aCommand != "set" && aCommand != "win" && aCommand != "lose")
		return false;

	if (!aDialog.empty())
	{
		ReplyError(theCommand, "the " + aDialog + " dialog is open");
		return true;
	}
	if (mTestAction)
	{
		ReplyError(theCommand, "the command before it has not run yet");
		return true;
	}

	if (aCommand == "goto")
	{
		if (anArgs == "title" || anArgs == "menu")
		{
			if (aGame->mState != STATE_TITLE)
				mTestAction = [aGame]() { aGame->toTitle(false); };
		}
		else if (anArgs == "mahjong" || anArgs == "play" || anArgs == "game")
		{
			mTestAction = [aGame]() {
				if (aGame->mState != STATE_TITLE)
					aGame->toTitle(false);
				aGame->startGame();
			};
		}
		else if (anArgs == "options")
			mTestAction = [aGame]() { aGame->showOptions(); };
		else if (anArgs == "help" || anArgs == "rules")
			mTestAction = [aGame]() { aGame->showRules(); };
		else if (anArgs == "highscores" || anArgs == "besttimes")
			mTestAction = [aGame]() { aGame->showBestTimes(aGame->mLayout); };
		else
		{
			ReplyError(theCommand, "screens: title, mahjong, options, help, highscores");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "level")
	{
		int aLayout = atoi(anArgs.c_str());
		if (anArgs.empty() || aLayout < 0 || aLayout >= NUM_LAYOUTS)
		{
			ReplyError(theCommand, "layouts are 0 to 17");
			return true;
		}
		// the title's layout arrows, then its start
		mTestAction = [aGame, aLayout]() {
			if (aGame->mState != STATE_TITLE)
				aGame->toTitle(false);
			aGame->cycleLayout(aLayout - aGame->mLayout);
			aGame->startGame();
		};
		ReplyOK(theCommand);
		return true;
	}

	if (aCommand == "set")
	{
		size_t aSpace = anArgs.find(' ');
		std::string aName = anArgs.substr(0, aSpace);
		int aValue = aSpace == std::string::npos ? -1 : atoi(anArgs.c_str() + aSpace + 1);
		if (aGame->mState != STATE_TITLE)
		{
			ReplyError(theCommand, "the settings change on the title");
			return true;
		}
		if (aName == "layout" && aValue >= 0 && aValue < NUM_LAYOUTS)
			mTestAction = [aGame, aValue]() { aGame->cycleLayout(aValue - aGame->mLayout); };
		else if (aName == "tileset" && aValue >= 0 && aValue < NUM_TILESETS)
			mTestAction = [aGame, aValue]() { aGame->cycleTileset(aValue - aGame->mTileset); };
		else if (aName == "background" && aValue >= 0 && aValue < NUM_BACKGROUNDS)
			mTestAction = [aGame, aValue]() { aGame->cycleBackground(aValue - aGame->mBackground); };
		else
		{
			ReplyError(theCommand, "names: layout 0-17, tileset 0-8, background 0-9 (on the title)");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}

	// win and lose
	if (aGame->mState != STATE_PLAYING || aGame->mPaused)
	{
		ReplyError(theCommand, "no game is being played");
		return true;
	}
	if (aCommand == "win")
	{
		// every pair but one taken off the board as the game takes a pair off (Board::removePair), then the last one
		// matched as a click would match it (NingPo::match), which ends the game won
		mTestAction = [aGame]() {
			Board& aBoard = aGame->mBoard;
			Tile* aLast[2] = {nullptr, nullptr};
			for (int i = 0; i < Board::NUM_TILES; i++)
			{
				Tile* aTile = &aBoard.mTiles[i];
				if (!aTile->mActive)
					continue;
				for (int j = i + 1; j < Board::NUM_TILES; j++)
				{
					Tile* anOther = &aBoard.mTiles[j];
					if (anOther->mActive && anOther->mType == aTile->mType)
					{
						if (aBoard.mTilesLeft > 2)
						{
							aTile->mActive = false;
							aBoard.removePair(aTile, anOther);
						}
						else
						{
							aLast[0] = aTile;
							aLast[1] = anOther;
						}
						break;
					}
				}
			}
			if (aLast[0] != nullptr)
				aGame->match(aLast[0], aLast[1]);
			aGame->invalidateAll();
		};
	}
	else
	{
		// the game's ending with no moves left: its question (End Game, Undo, Shuffle) comes up as when the last move
		// is gone, and a driver answers it ("move" says End Game)
		mTestAction = [aGame]() {
			uint64_t aPlayed = (uint64_t)(pg::PyTimeSub(aGame->mEventTime, aGame->mGameStart) - aGame->getTime());
			int aSeconds = (int)(aPlayed / 1000);
			aGame->endGame(false, aGame->mPenalty + aSeconds, aSeconds);
		};
	}
	ReplyOK(theCommand);
	return true;
}

// docs/TEST_COMMANDS.md, "memreport": what the game holds beside the framework's own pictures. The sprites are the game's
// table (a dialog's own sprites are loaded and released as it opens and closes), 15 bits a pixel and a byte of alpha
// where a picture has one; the sounds are the 64 decoded sounds; the music is the track that is playing, which the port
// decodes whole; the back buffer is the 15-bit screen the game draws into
void NingPoApp::AddMemReportHolders(Platform::MemReport* theReport)
{
	if (mGame == nullptr)
		return;
	size_t aSpriteBytes = 0;
	int aSpriteCount = 0;
	for (int i = 0; i < NUM_SPRITES; i++)
	{
		const pg::HSurface* aSurface = mGame->mSprites[i].mSurface;
		if (aSurface == nullptr)
			continue;
		aSpriteCount++;
		aSpriteBytes += (size_t)aSurface->mHeight * aSurface->mPitch;
		if (aSurface->mAlpha != nullptr)
			aSpriteBytes += (size_t)aSurface->mWidth * aSurface->mHeight;
	}
	size_t aSoundBytes = 0;
	for (const pg::PGSoundManager::Sound& aSound : mGame->mSounds.mSounds)
		if (aSound.mData)
			aSoundBytes += aSound.mData->Bytes();
	size_t aMusicBytes = mGame->mMixer.mMusic ? mGame->mMixer.mMusic->Bytes() : 0;
	theReport->mOthers.push_back({"sprites (" + std::to_string(aSpriteCount) + ", 15 bits and alpha)", aSpriteBytes});
	theReport->mOthers.push_back({"sounds (decoded)", aSoundBytes});
	theReport->mOthers.push_back({"music (the whole track decoded)", aMusicBytes});
	theReport->mOthers.push_back({"back buffer (15 bits)", mGame->mDisplay->mScreen.capacity() * sizeof(uint16_t)});
}
