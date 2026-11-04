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
//   -autoplaylog=<file>   the log ("level N done at update U", "game N over at update U")
//   -autoplayshots=<prefix>  screenshots at each level summary and game over
//   -autoplayui           where it would press Enter, click the button Enter presses (the original reads its keys with
//                         DirectInput, which origrun's posted input does not reach), and type the high score's name
//                         before it clicks OK
//   -autoplayscript=<file>   the bot's input as a -script file, each line at the update it is run
//   -autoplaysave=N       after N blocks in the first game, end it through End game and save it, then continue it from
//                         the new game screen
//
// -statelog=<file> (any run): a line "<update> <name> <value>" at each change of the values that decide play, read
// after the update's pass; "-" while there is no board. The names are those games/tiptop/tools/identical/case.sh
// watches in the original.

#include "TipTopApp.h"
#include "App.h"
#include "Board.h"
#include "Config.h"
#include "Game.h"
#include "Music.h"
#include "Gui.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace tiptop
{
uint32_t SummaryItemProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);  // App.cpp, 0x424e66
}

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
	int mSaveStep;        // -autoplaysave: 0 to do, 1 End game clicked, 2 saved, 3 continued
	FILE* mLog;
	FILE* mScriptOut;

	AutoplayState(unsigned int theSeed) : mSeed(theSeed), mWait(0), mGames(0), mLastScreen(-1), mShots(0), mMoves(0),
		mNameTyped(false), mShot(false), mSaveStep(0), mLog(nullptr), mScriptOut(nullptr) {}

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
	if (theState->mScriptOut != nullptr)
		fclose(theState->mScriptOut);
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

// Input for the next update, written to the -autoplayscript file as well
void Add(AutoplayState* s, const std::string& theCommand, const std::string& theArgs)
{
	gApp->AddScriptCommand(gApp->mUpdateCount, theCommand, theArgs);
	if (s->mScriptOut != nullptr)
	{
		fprintf(s->mScriptOut, "%d %s %s\n", gApp->mUpdateCount + 1, theCommand.c_str(), theArgs.c_str());
		fflush(s->mScriptOut);
	}
}

// What Enter would press: a button of the top modal layer that Enter presses, or the level summary's continue item once
// it takes input; nullptr if none
tiptop::Handler* EnterTarget()
{
	uint16_t aCount = 0;
	tiptop::Handler* aTable = tiptop::HandlerTable(&aCount);
	for (int i = 0; i < aCount; i++)
	{
		tiptop::Handler* h = &aTable[i];
		if (h->mProc == nullptr || tiptop::InTopModal(h->mLayer) == 0)
			continue;
		if ((h->mProc == tiptop::ButtonProc || h->mProc == tiptop::ShapedButtonProc) &&
			(h->u.button.mCommand & 0x80000000) != 0)
			return h;
		if (h->mProc == tiptop::SummaryItemProc && h->u.mSlots[0] == 4 && (h->u.mSlots[3] & 4) != 0)
			return h;
	}
	return nullptr;
}

// Clicks the button of the top modal layer that sends theCommand; false if there is none
bool ClickCommand(AutoplayState* s, uint32_t theCommand)
{
	uint16_t aCount = 0;
	tiptop::Handler* aTable = tiptop::HandlerTable(&aCount);
	for (int i = 0; i < aCount; i++)
	{
		tiptop::Handler* h = &aTable[i];
		if (h->mProc != nullptr && tiptop::InTopModal(h->mLayer) != 0 &&
			(h->mProc == tiptop::ButtonProc || h->mProc == tiptop::ShapedButtonProc) &&
			(h->u.button.mCommand & 0x3fffffff) == theCommand)
		{
			Add(s, "click", StrFormat("%d %d", h->mRect.x + h->mRect.w / 2, h->mRect.y + h->mRect.h / 2));
			return true;
		}
	}
	return false;
}

// Enter, or with -autoplayui a click on what Enter would press; false if there is nothing to click yet
bool Enter(AutoplayState* s)
{
	if (!gApp->mAutoplayUi)
	{
		Add(s, "key", "RETURN");
		return true;
	}
	tiptop::Handler* h = EnterTarget();
	if (h == nullptr)
		return false;
	Add(s, "click", StrFormat("%d %d", h->mRect.x + h->mRect.w / 2, h->mRect.y + h->mRect.h / 2));
	return true;
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
	if (s->mScriptOut == nullptr && !mAutoplayScript.empty())
		s->mScriptOut = fopen(mAutoplayScript.c_str(), "w");
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
			Log(s, "level %u done at update %d: score %u\n", anApp.mBoard->mState.mLevel, mUpdateCount,
				anApp.mBoard->GetScore());
			s->mWait = 300;
		}
		else if (aScreen == tiptop::SCREEN_GAMEOVER && anApp.mBoard != nullptr)
		{
			s->mGames++;
			Log(s, "game %d over at update %d: level %u, score %u, %d moves\n", s->mGames, mUpdateCount,
				anApp.mBoard->mState.mLevel, anApp.mBoard->GetScore(), s->mMoves);
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
		Add(s, "click", "186 288");
		s->mWait = 100;
		break;
	case tiptop::SCREEN_MAINMENU:
		if (s->mGames >= mAutoplayGames)
		{
			Log(s, "done\n");
			Shutdown();
			return;
		}
		Add(s, "click", "165 88");  // Play
		s->mWait = 100;
		break;
	case tiptop::SCREEN_NEWGAME:
		// the new game screen's Regular, Strategy and Speed buttons, or Continue for the game just saved
		if (s->mSaveStep == 2)
		{
			if (ClickCommand(s, 0x24))
			{
				Log(s, "update %d: continue the saved game\n", mUpdateCount);
				s->mSaveStep = 3;
				s->mWait = 100;
			}
			break;
		}
		Add(s, "click", mAutoplayMode == 1 ? "145 260" : mAutoplayMode == 2 ? "240 325" :
			"232 185");
		s->mWait = 100;
		break;
	case tiptop::SCREEN_GAME:
	{
		tiptop::Board* aBoard = anApp.mBoard;
		if (tiptop::InTopModal(anApp.mLayer) == 0)
		{
			if (s->mSaveStep == 1)
			{
				// the save dialog: save (command 3), back to the main menu
				if (ClickCommand(s, 3))
				{
					Log(s, "update %d: saved after %d moves, score %u\n", mUpdateCount, s->mMoves,
						aBoard != nullptr ? aBoard->GetScore() : 0);
					s->mSaveStep = 2;
					s->mWait = 100;
				}
				break;
			}
			// the tips (or another dialog) over the game: its default button
			if (Enter(s))
				s->mWait = 100;
			break;
		}
		if (aBoard == nullptr || aBoard->mField == nullptr || aBoard->mGameOver != 0 || tiptop::gBusy != 0)
			break;
		// a press the column cursor still holds (a click that came while the game was busy) is tapped first: a click in
		// the same pass would make the original, whose scripted cursor moves in the middle of a pass, tap the held
		// press in the new click's column (phase one of the identical-play pass, the speed game)
		if (anApp.mEC != nullptr && anApp.mEC->u.mSlots[1] != 0)
			break;
		if (s->mSaveStep == 0 && mAutoplaySave > 0 && s->mMoves >= mAutoplaySave && s->mGames == 0)
		{
			// End game (command 2), which asks whether to save
			if (ClickCommand(s, 2))
			{
				s->mSaveStep = 1;
				s->mWait = 100;
			}
			break;
		}
		int aColumn = ChooseColumn(s, aBoard);
		if (aColumn < 0)
			break;
		int x = tiptop::gConfig.mBlockgridX + aColumn * tiptop::gConfig.mBlockWidth + tiptop::gConfig.mBlockWidth / 2;
		int y = tiptop::gConfig.mBlockgridY + tiptop::gConfig.mBlockHeight * tiptop::gConfig.mNumRows / 2;
		Add(s, "click", StrFormat("%d %d", x, y));
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
				Add(s, "char", std::string(1, *c));
			s->mNameTyped = true;
			if (mAutoplayUi)
			{
				// the game takes one typed character a pass: OK once they are in
				s->mWait = 50;
				break;
			}
		}
		else if (s->mGames >= mAutoplayGames && (!mAutoplayUi || tiptop::InTopModal(anApp.mLayer) != 0))
		{
			// (with -autoplayui the name's OK is clicked first)
			Log(s, "done\n");
			Shutdown();
			return;
		}
		if (Enter(s))
			s->mWait = 200;
		break;
	default:
		// the level summary, the high scores: Enter goes on
		if (Enter(s))
			s->mWait = 200;
		break;
	}
}

// -soundlog: the music's state as the original's memory holds it (playing 0x48a098, the pause count 0x489f08, the
// music volume 0x47e298), a line at each change
void TipTopApp::UpdateMusicLog()
{
	static std::string sLast;
	std::string aState = StrFormat("music %d %d %u", tiptop::MusicPlaying(), tiptop::MusicPauseCount(),
		tiptop::MusicVolume());
	if (aState != sLast)
	{
		fprintf(mSoundLog, "%d %s\n", mUpdateCount, aState.c_str());
		sLast = aState;
	}
	// a web build under Node ends without closing the log
	fflush(mSoundLog);
}

// -statelog: the values that decide play, as the original's memory holds them (the names of case.sh's watches)
void TipTopApp::UpdateStateLog()
{
	if (mStateLog == nullptr)
	{
		mStateLog = fopen(mStateLogName.c_str(), "w");
		if (mStateLog == nullptr)
		{
			mStateLogName.clear();
			return;
		}
	}
	static const char* const kNames[] = {"screen", "mode", "rng", "draws", "score", "level", "blocksleft", "meter",
		"gameover", "levelcomplete", "ending", "next0", "next1", "fnext", "lowrow", "highrow", "pending", "floaters",
		"bestchain", "bestmatch", "totaltime", "leveltime", "spawndelay", "paused", "bonus", "queueindex", "busy", "lockfall", "lockspawn",
		"shifting", "hs0", "hs1", "hs2", "hs3", "hsname"};
	const int kCount = (int)(sizeof(kNames) / sizeof(kNames[0]));
	static std::string sLast[sizeof(kNames) / sizeof(kNames[0])];
	std::string aValues[kCount];
	for (int i = 0; i < kCount; i++)
		aValues[i] = "-";
	tiptop::App& anApp = tiptop::gApp;
	aValues[0] = StrFormat("%d", anApp.mScreen);
	aValues[1] = StrFormat("%u", tiptop::gGameMode);
	aValues[2] = StrFormat("%u", tiptop::gRandState);
	aValues[3] = StrFormat("%u", tiptop::gRandDraws);
	aValues[23] = StrFormat("%u", (uint32_t)(uint8_t)tiptop::gPaused[0] | (uint32_t)(uint8_t)tiptop::gPaused[1] << 8 |
		(uint32_t)(uint8_t)tiptop::gPaused[2] << 16);
	aValues[24] = StrFormat("%u", (uint32_t)(uint8_t)tiptop::gBonusRound);
	aValues[26] = StrFormat("%u", (uint32_t)(uint8_t)tiptop::gBusy);
	// the high score table of the mode: its first four scores and the first four bytes of the top name
	if (tiptop::gGameMode < 4)
	{
		for (int i = 0; i < 4; i++)
			aValues[30 + i] = StrFormat("%d", tiptop::gHighScores[tiptop::gGameMode][i].mScore);
		uint32_t aName;
		memcpy(&aName, tiptop::gHighScores[tiptop::gGameMode][0].mName, 4);
		aValues[34] = StrFormat("%u", aName);
	}
	tiptop::Board* aBoard = anApp.mBoard;
	if (aBoard != nullptr)
	{
		uint32_t aMeter;
		memcpy(&aMeter, &aBoard->mState.mMeter, 4);
		aValues[5] = StrFormat("%u", aBoard->mState.mLevel);
		aValues[6] = StrFormat("%d", aBoard->mState.mBlocksLeft);
		aValues[7] = StrFormat("%u", aMeter);
		aValues[8] = StrFormat("%d", aBoard->mGameOver);
		aValues[9] = StrFormat("%d", aBoard->mLevelComplete);
		aValues[10] = StrFormat("%d", aBoard->mEnding);
		aValues[11] = StrFormat("%d", aBoard->mNextBlocks[0].mKind);
		aValues[12] = StrFormat("%d", aBoard->mNextBlocks[1].mKind);
		aValues[25] = StrFormat("%d", aBoard->mQueueIndex);
		aValues[29] = StrFormat("%d", aBoard->mQueueShifting);
		tiptop::Field* aField = aBoard->mField;
		if (aField != nullptr)
		{
			aValues[4] = StrFormat("%u", aField->mScore);
			aValues[13] = StrFormat("%d", aField->mNext.mKind);
			aValues[14] = StrFormat("%d", aField->mLowRow);
			aValues[15] = StrFormat("%d", aField->mHighRow);
			aValues[16] = StrFormat("%u", aField->mPendingSpawns);
			aValues[17] = StrFormat("%u", aField->mPendingFloaters);
			aValues[18] = StrFormat("%u", aField->mBestChain);
			aValues[19] = StrFormat("%u", aField->mBestMatch);
			aValues[20] = StrFormat("%u", aField->mTotalTime);
			aValues[21] = StrFormat("%u", aField->mLevelTime);
			aValues[22] = StrFormat("%d", aField->mSpawnDelay);
			aValues[27] = StrFormat("%u", aField->mLockFall);
			aValues[28] = StrFormat("%u", aField->mLockSpawn);
		}
	}
	for (int i = 0; i < kCount; i++)
	{
		if (aValues[i] != sLast[i])
		{
			fprintf(mStateLog, "%d %s %s\n", mUpdateCount, kNames[i], aValues[i].c_str());
			sLast[i] = aValues[i];
		}
	}
	fflush(mStateLog);
}

void TipTopApp::UpdateButtonLog()
{
	if (mButtonLog == nullptr)
	{
		mButtonLog = fopen(mButtonLogName.c_str(), "w");
		if (mButtonLog == nullptr)
		{
			mButtonLogName.clear();
			return;
		}
	}
	static const struct { tiptop::HandlerProc mProc; const char* mName; } kProcs[] = {{tiptop::ButtonProc, "button"},
		{tiptop::ShapedButtonProc, "shaped"}, {tiptop::CheckBoxProc, "check"}, {tiptop::SliderProc, "slider"},
		{tiptop::ScrollBarProc, "scroll"}, {tiptop::TextInputProc, "input"}, {tiptop::TextAreaProc, "text"},
		{tiptop::SummaryItemProc, "summary"}};
	std::string aList;
	uint16_t aCount = 0;
	tiptop::Handler* aTable = tiptop::HandlerTable(&aCount);
	for (int i = 0; i < aCount; i++)
	{
		tiptop::Handler* h = &aTable[i];
		if (h->mProc == nullptr || tiptop::InTopModal(h->mLayer) == 0)
			continue;
		for (const auto& k : kProcs)
		{
			if (h->mProc == k.mProc)
			{
				uint32_t aCommand = (h->mProc == tiptop::ButtonProc || h->mProc == tiptop::ShapedButtonProc) ?
					h->u.button.mCommand : 0;
				aList += StrFormat("  %s %d %d %d %d centre %d %d command 0x%x\n", k.mName, h->mRect.x, h->mRect.y,
					h->mRect.w, h->mRect.h, h->mRect.x + h->mRect.w / 2, h->mRect.y + h->mRect.h / 2, aCommand);
				break;
			}
		}
	}
	static std::string sLast;
	static int sLastScreen = -1;
	if (aList != sLast || tiptop::gApp.mScreen != sLastScreen)
	{
		fprintf(mButtonLog, "%d screen %d\n%s", mUpdateCount, tiptop::gApp.mScreen, aList.c_str());
		fflush(mButtonLog);
		sLast = aList;
		sLastScreen = tiptop::gApp.mScreen;
	}
}
