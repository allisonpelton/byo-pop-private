// Test hooks: the autoplay bot (-autoplay=<seed>). It plays whole games through the framework's scripted input
// (SexyAppBase::AddScriptCommand), so everything the game does for a player runs: the splash and the menus, the
// tips, blocks dropped into columns, matches, levels and their summaries, the game over screen and the high score
// name. In a game it drops the next block where it joins the biggest group of its kind; when no column makes a
// match it picks one of the emptiest columns at random. It has its own random numbers (the game's use the C
// library generator, seeded by -seed), so the same -seed and -autoplay always make the same game. It clicks only
// in the board and on the menus' play buttons, and otherwise presses Enter: never a link button.
//
//   -autoplay=<seed>      play with the bot (its own seed)
//   -autoplaymode=regular|strategy|speed   the game type (default regular)
//   -autoplaygames=N      play N games, then quit
//   -autoplaycap=N        quit after N updates in any case
//   -autoplaylog=<file>   the log ("level N done", "game N over")
//   -autoplayshots=<prefix>  screenshots at each level summary and game over

#include "TipTopApp.h"
#include "App.h"
#include "Board.h"
#include "Config.h"
#include "Game.h"
#include "Gui.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace Sexy;

namespace Sexy
{

struct AutoplayState
{
	unsigned int mSeed;
	int mWait;            // updates to wait before the next action
	int mGames;           // games over
	int mLastScreen;
	int mShots;
	int mMoves;           // blocks dropped in this game
	bool mNameTyped;
	bool mShot;           // the screenshot of this screen is taken
	FILE* mLog;

	AutoplayState(unsigned int theSeed) : mSeed(theSeed), mWait(0), mGames(0), mLastScreen(-1), mShots(0), mMoves(0),
		mNameTyped(false), mShot(false), mLog(nullptr) {}

	int Rand(int theRange)
	{
		mSeed = mSeed * 1103515245u + 12345u;
		return (int)((mSeed >> 16) % (unsigned int)theRange);
	}
};

AutoplayState* NewAutoplayState(unsigned int theSeed)
{
	return new AutoplayState(theSeed);
}

void DeleteAutoplayState(AutoplayState* theState)
{
	if (theState->mLog != nullptr)
		fclose(theState->mLog);
	delete theState;
}

} // namespace Sexy

namespace
{

void Log(AutoplayState* s, const char* theFormat, ...)
{
	if (s->mLog == nullptr)
		return;
	va_list args;
	va_start(args, theFormat);
	vfprintf(s->mLog, theFormat, args);
	va_end(args);
	fflush(s->mLog);
}

// The cells of the same kind joined to theIndex (theKind placed there), counted once each
int GroupSize(tiptop::Field* theField, int theIndex, int theKind, std::string& theSeen)
{
	int aColumns = (int)theField->mColumns;
	int aCount = (int)theField->mColumns * (int)theField->mRows;
	if (theIndex < 0 || theIndex >= aCount || theSeen[theIndex] != 0)
		return 0;
	theSeen[theIndex] = 1;
	int aSize = 1;
	int aColumn = theIndex % aColumns;
	const int aNeighbors[4] = {theIndex - aColumns, theIndex + aColumns, aColumn > 0 ? theIndex - 1 : -1,
		aColumn < aColumns - 1 ? theIndex + 1 : -1};
	for (int i = 0; i < 4; i++)
	{
		int aNext = aNeighbors[i];
		if (aNext < 0 || aNext >= aCount)
			continue;
		tiptop::Piece* aPiece = theField->PieceAt((uint32_t)aNext);
		if (aPiece != nullptr && aPiece->mKind == theKind)
			aSize += GroupSize(theField, aNext, theKind, theSeen);
	}
	return aSize;
}

// The column to drop the next block into: the biggest group it joins, else one of the emptiest; -1 if none is free
int ChooseColumn(AutoplayState* s, tiptop::Board* theBoard)
{
	tiptop::Field* aField = theBoard->mField;
	int aColumns = (int)aField->mColumns;
	int aRows = (int)aField->mRows;
	int aKind = theBoard->mNextBlocks[0].mKind;
	int aBest = -1;
	int aBestSize = 0;
	int aBestCount = 0;
	int aMostRoom = -1;
	int aRoomCount = 0;
	int aRoomColumn = -1;
	for (int c = 0; c < aColumns; c++)
	{
		if (aField->CanPlace(c) == 0)
			continue;
		// the block rises from the bottom row until it meets a block
		int aRow = 0;
		while (aRow + 1 < aRows && aField->PieceAt((uint32_t)((aRow + 1) * aColumns + c)) == nullptr)
			aRow++;
		std::string aSeen((size_t)(aColumns * aRows), '\0');
		int aSize = (aKind < 10) ? GroupSize(aField, aRow * aColumns + c, aKind, aSeen) : 0;
		if (aSize > aBestSize)
		{
			aBest = c;
			aBestSize = aSize;
			aBestCount = 1;
		}
		else if (aSize == aBestSize && aBest >= 0 && s->Rand(++aBestCount) == 0)
		{
			aBest = c;
		}
		if (aRow > aMostRoom)
		{
			aMostRoom = aRow;
			aRoomColumn = c;
			aRoomCount = 1;
		}
		else if (aRow == aMostRoom && s->Rand(++aRoomCount) == 0)
		{
			aRoomColumn = c;
		}
	}
	if (aBestSize >= 3)
		return aBest;
	return aRoomColumn;
}

} // namespace

void TipTopApp::Autoplay()
{
	AutoplayState* s = mAutoplayState;
	if (s->mLog == nullptr && !mAutoplayLog.empty())
		s->mLog = fopen(mAutoplayLog.c_str(), "w");
	if (mUpdateCount >= mAutoplayCap)
	{
		Log(s, "update %d: cap reached (game %d, %d moves)\n", mUpdateCount, s->mGames + 1, s->mMoves);
		if (!mAutoplayShots.empty())
			SaveScreenshot(mAutoplayShots + "-cap.png");
		Shutdown();
		return;
	}
	tiptop::App& anApp = tiptop::gApp;
	int aScreen = anApp.mScreen;
	if (aScreen != s->mLastScreen)
	{
		s->mLastScreen = aScreen;
		s->mWait = 100;
		s->mShot = false;
		if (aScreen == tiptop::SCREEN_LEVELSUMMARY && anApp.mBoard != nullptr)
		{
			Log(s, "level %u done: score %u\n", anApp.mBoard->mState.mLevel, anApp.mBoard->GetScore());
			s->mWait = 300;
		}
		else if (aScreen == tiptop::SCREEN_GAMEOVER && anApp.mBoard != nullptr)
		{
			s->mGames++;
			Log(s, "game %d over: level %u, score %u, %d moves\n", s->mGames, anApp.mBoard->mState.mLevel,
				anApp.mBoard->GetScore(), s->mMoves);
			s->mWait = 600;
			s->mNameTyped = false;
			s->mMoves = 0;
		}
	}
	if (s->mWait > 0)
	{
		s->mWait--;
		if (s->mWait == 0 && !s->mShot && !mAutoplayShots.empty() &&
			(aScreen == tiptop::SCREEN_LEVELSUMMARY || aScreen == tiptop::SCREEN_GAMEOVER))
		{
			SaveScreenshot(mAutoplayShots + StrFormat("-%02d-screen%d.png", s->mShots++, aScreen));
			s->mShot = true;
		}
		return;
	}
	switch (aScreen)
	{
	case tiptop::SCREEN_SPLASH:
		// "click here to play", once the load bar is full
		AddScriptCommand(mUpdateCount, "click", "186 288");
		s->mWait = 100;
		break;
	case tiptop::SCREEN_MAINMENU:
		if (s->mGames >= mAutoplayGames)
		{
			Log(s, "done\n");
			Shutdown();
			return;
		}
		AddScriptCommand(mUpdateCount, "click", "165 88");  // Play
		s->mWait = 100;
		break;
	case tiptop::SCREEN_NEWGAME:
		// the new game screen's Regular, Strategy and Speed buttons
		AddScriptCommand(mUpdateCount, "click", mAutoplayMode == 1 ? "145 260" : mAutoplayMode == 2 ? "240 325" :
			"232 185");
		s->mWait = 100;
		break;
	case tiptop::SCREEN_GAME:
	{
		tiptop::Board* aBoard = anApp.mBoard;
		if (tiptop::InTopModal(anApp.mLayer) == 0)
		{
			// the tips (or another dialog) over the game: its default button
			AddScriptCommand(mUpdateCount, "key", "RETURN");
			s->mWait = 100;
			break;
		}
		if (aBoard == nullptr || aBoard->mField == nullptr || aBoard->mGameOver != 0 || tiptop::gBusy != 0)
			break;
		int aColumn = ChooseColumn(s, aBoard);
		if (aColumn < 0)
			break;
		int x = tiptop::gConfig.mBlockgridX + aColumn * tiptop::gConfig.mBlockWidth + tiptop::gConfig.mBlockWidth / 2;
		int y = tiptop::gConfig.mBlockgridY + tiptop::gConfig.mBlockHeight * tiptop::gConfig.mNumRows / 2;
		AddScriptCommand(mUpdateCount, "click", StrFormat("%d %d", x, y));
		s->mMoves++;
		s->mWait = (mAutoplayMode == 2) ? 40 : 60;
		break;
	}
	case tiptop::SCREEN_GAMEOVER:
		// a new high score's name in its dialog (over the screen) and its OK; then Enter presses the play again
		// button, or the bot stops after its last game
		if (tiptop::InTopModal(anApp.mLayer) == 0 && !s->mNameTyped)
		{
			for (const char* c = "Bot"; *c != 0; c++)
				AddScriptCommand(mUpdateCount, "char", std::string(1, *c));
			s->mNameTyped = true;
		}
		else if (s->mGames >= mAutoplayGames)
		{
			Log(s, "done\n");
			Shutdown();
			return;
		}
		AddScriptCommand(mUpdateCount, "key", "RETURN");
		s->mWait = 200;
		break;
	default:
		// the level summary, the high scores: Enter goes on
		AddScriptCommand(mUpdateCount, "key", "RETURN");
		s->mWait = 200;
		break;
	}
}
