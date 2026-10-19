// port: test hook for -autoplay=<seed> (see Autoplay.h and BookWormApp.h). Not part of the original game.

#include "Autoplay.h"
#include "BookWormApp.h"
#include "Board.h"
#include "Lex.h"
#include "TitleScreen.h"
#include "BWGameSelector.h"
#include "HelpWidget.h"
#include "FameWidget.h"
#include "LevelUpWidget.h"
#include "NewUserDialog.h"
#include "ColorizedButton.h"
#include "Res.h"

#include "WidgetManager.h"
#include "ButtonWidget.h"
#include "HyperlinkWidget.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "EditWidget.h"
#include "Point.h"

#include <SDL.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <set>

using namespace Sexy;

namespace
{

enum
{
	RUN_DONE,       // the step's input is queued
	RUN_SKIP,       // the step needs no input (done)
	RUN_WAIT,       // the board takes no input now: try again later
	RUN_DROP        // the board has changed: the plan is void
};

enum
{
	METHOD_BUTTON,      // click the tiles, then the Submit button
	METHOD_CLICK_LAST,  // click the tiles, then the last one again
	METHOD_DRAG,        // drag over the tiles, release on the last
	METHOD_CLICKS_ONLY, // click the tiles (a cancel: a right click follows)
	METHOD_DRAG_OFF     // drag over the tiles, release off them (a cancel: the release clears)
};

const int MAX_WORD_LENGTH = 12;     // the DFS stops there (a whole word, "QU" counting 2)
const int OFF_TILE_X = 260;         // in the board, above column 0 (whose row 0 is empty): no tile there
const int OFF_TILE_Y = 12;
const char* const BOT_PLAYER_NAME = "Autoplay Bot";

const char* const gStateNames[] =
{
	"playing", "game over music", "game over", "inactive", "game over pending", "level complete", "next level",
	"scrambling"
};

const char* StateName(int theState)
{
	return ((theState >= 0) && (theState < (int) (sizeof(gStateNames) / sizeof(gStateNames[0])))) ? gStateNames[theState] : "?";
}

const char* MethodName(int theMethod)
{
	switch (theMethod)
	{
	case METHOD_BUTTON: return "submit button";
	case METHOD_CLICK_LAST: return "click the last tile again";
	case METHOD_DRAG: return "drag";
	case METHOD_CLICKS_ONLY: return "clicks, then a right click";
	case METHOD_DRAG_OFF: return "drag released off the tiles";
	}
	return "?";
}

int CellCol(int theCell)
{
	return theCell % BOARD_COLS;
}

int CellRow(int theCell)
{
	return theCell / BOARD_COLS;
}

Piece* CellPiece(Board* theBoard, int theCell)
{
	return theBoard->mBoard[CellRow(theCell)][CellCol(theCell)];
}

bool SameWordIgnoringCase(const std::string& theWord1, const std::string& theWord2)
{
	if (theWord1.size() != theWord2.size())
		return false;
	for (size_t i = 0; i < theWord1.size(); i++)
	{
		if (toupper((unsigned char) theWord1[i]) != toupper((unsigned char) theWord2[i]))
			return false;
	}
	return true;
}

}

AutoplayBot::AutoplayBot(BookWormApp* theApp, unsigned long theSeed, const std::string& theMode, int theUpdateCap,
	const std::string& theLogFileName, const std::string& theShotPrefix, int theShotWords) :
	mApp(theApp),
	mRand((std::mt19937::result_type) theSeed),
	mSeed(theSeed),
	mModeName(theMode),
	mUpdateCap(theUpdateCap),
	mLog(NULL),
	mShotPrefix(theShotPrefix),
	mShotWords(theShotWords),
	mDone(false),
	mQuitAt(-1),
	mTitleClicked(false),
	mGamesStarted(0),
	mGamesOver(0),
	mWaitKey(NULL),
	mWaitUntil(0),
	mSummaryNoted(false),
	mFameNoted(false),
	mLevelUps(0),
	mPlanPos(0),
	mPlanKind(PLAN_WORD),
	mNextStepUpdate(0),
	mWaitedSince(-1),
	mDragging(false),
	mDragX(0),
	mDragY(0),
	mDragCell(-1),
	mPlanNum(0),
	mResultUpdate(-1),
	mScoreBefore(0),
	mWordsScored(0),
	mLastBoard(NULL),
	mLastState(-1),
	mLastLevel(-1)
{
	if (!theLogFileName.empty())
	{
		mLog = fopen(theLogFileName.c_str(), "wb");
		if (mLog == NULL)
			SDL_Log("autoplay: cannot write %s", theLogFileName.c_str());
	}

	if (mModeName == "action")
		mModes.push_back(GAMEMODE_ACTION);
	else if (mModeName == "both")
	{
		mModes.push_back(GAMEMODE_CLASSIC);
		mModes.push_back(GAMEMODE_ACTION);
	}
	else
	{
		if (mModeName != "classic")
			SDL_Log("autoplay: unknown -autoplaymode=%s, playing classic", mModeName.c_str());
		mModeName = "classic";
		mModes.push_back(GAMEMODE_CLASSIC);
	}

	Note("-autoplay=%lu -seed=%lu -autoplaymode=%s -autoplaycap=%d: the bot's input as a -script file", mSeed,
		(unsigned long) mApp->mRandSeed, mModeName.c_str(), mUpdateCap);
}

AutoplayBot::~AutoplayBot()
{
	if (mLog != NULL)
		fclose(mLog);
}

int AutoplayBot::Random(int theRange)
{
	return (int) (mRand() % (unsigned) theRange);
}

// A comment line in the log (the state after the current update)
void AutoplayBot::Note(const char* theFormat, ...)
{
	if (mLog == NULL)
		return;

	va_list anArgs;
	va_start(anArgs, theFormat);
	fprintf(mLog, "# update %d: ", mApp->mUpdateCount);
	vfprintf(mLog, theFormat, anArgs);
	fprintf(mLog, "\n");
	va_end(anArgs);
}

// Queues one script command for the next update, and logs it as a script line
void AutoplayBot::Queue(const std::string& theCommand, const std::string& theArgs)
{
	int anUpdate = mApp->mUpdateCount + 1;
	mApp->AddScriptCommand(anUpdate, theCommand, theArgs);

	if (mLog != NULL)
		fprintf(mLog, "%d %s%s%s\n", anUpdate, theCommand.c_str(), theArgs.empty() ? "" : " ", theArgs.c_str());
}

void AutoplayBot::QueueAt(const std::string& theCommand, int theX, int theY)
{
	Queue(theCommand, std::to_string(theX) + " " + std::to_string(theY));
}

// -autoplayshots: a screenshot at the start of the next update
void AutoplayBot::Shot(const std::string& theEvent)
{
	if (!mShotPrefix.empty())
		Queue("screenshot", mShotPrefix + theEvent + ".png");
}

void AutoplayBot::Quit()
{
	if (mDragging)
		EndDrag("quit");
	Queue("quit", "");
	mDone = true;
	if (mLog != NULL)
		fflush(mLog);
}

const char* AutoplayBot::ModeName(int theMode)
{
	return (theMode == GAMEMODE_ACTION) ? "action" : "classic";
}

// True once theKey (a screen or dialog) has been waited on for a random theMinDelay..theMaxDelay updates
bool AutoplayBot::Due(const void* theKey, int theMinDelay, int theMaxDelay)
{
	if (theKey != mWaitKey)
	{
		mWaitKey = theKey;
		mWaitUntil = mApp->mUpdateCount + theMinDelay + Random(theMaxDelay - theMinDelay + 1);
	}
	if (mApp->mUpdateCount < mWaitUntil)
		return false;
	mWaitKey = NULL;
	return true;
}

// A click near the middle of a visible, enabled widget, if the click reaches it (nothing in front of it)
bool AutoplayBot::ClickWidget(Widget* theWidget, const char* theWhat)
{
	if ((theWidget == NULL) || (!theWidget->mVisible) || (theWidget->mDisabled))
		return false;

	Point aPos = theWidget->GetAbsPos();
	int x = aPos.mX + theWidget->mWidth / 2 + Random(theWidget->mWidth / 2 + 1) - theWidget->mWidth / 4;
	int y = aPos.mY + theWidget->mHeight / 2 + Random(theWidget->mHeight / 2 + 1) - theWidget->mHeight / 4;
	if (mApp->mWidgetManager->GetWidgetAt(x, y, NULL, NULL) != theWidget)
		return false;

	Note("click %s", theWhat);
	QueueAt("click", x, y);
	return true;
}

void AutoplayBot::LogChanges(Board* theBoard)
{
	if (theBoard != mLastBoard)
	{
		if (theBoard != NULL)
		{
			Note("new board: %s", ModeName(mApp->mGameMode));
			mWordsScored = 0;
		}
		mLastBoard = theBoard;
		mLastState = -1;
		mLastLevel = -1;
		if (mPlanPos < mPlan.size())
			DropPlan("new board");
	}

	if (theBoard == NULL)
		return;

	if (theBoard->mState != mLastState)
	{
		Note("board: %s -> %s, level %d, score %d, %d fire tiles", StateName(mLastState), StateName(theBoard->mState),
			theBoard->mLevel + 1, theBoard->mGameStats->mScore, theBoard->mFireTileCount);
		if (theBoard->mState == BOARD_GAMEOVER_PENDING)
			Note("game over: a fire tile burned through the bottom");
		mLastState = theBoard->mState;
	}
	if (theBoard->mLevel != mLastLevel)
	{
		if (mLastLevel >= 0)
			Note("level up: now level %d", theBoard->mLevel + 1);
		mLastLevel = theBoard->mLevel;
	}
}

// The update after a submit (or a click on Lex): what came of it
void AutoplayBot::CheckResult(Board* theBoard)
{
	if ((mResultUpdate < 0) || (mApp->mUpdateCount < mResultUpdate))
		return;
	mResultUpdate = -1;
	if (theBoard == NULL)
		return;

	if (mPlanKind == PLAN_SCRAMBLE)
	{
		const char* aResult = (theBoard->mState == BOARD_SCRAMBLING) ? "the letters flip" :
			(mApp->GetDialog(DIALOG_HINT) != NULL) ? "refused with a dialog" : "refused";
		Note("scramble: %s, %d fire tiles, %d scrambles this game", aResult, theBoard->mFireTileCount,
			theBoard->mGameStats->mScrambles);
		return;
	}

	int aScore = theBoard->mGameStats->mScore;
	if (aScore != mScoreBefore)
	{
		mWordsScored++;
		Note("word %d: %s scored %d (the word %d), total %d, level %d, %d fire tiles", mWordsScored,
			theBoard->mLastWord.mWord.c_str(), aScore - mScoreBefore, theBoard->mLastWord.mScore, aScore, theBoard->mLevel + 1,
			theBoard->mFireTileCount);
		if ((mShotWords > 0) && (mWordsScored % mShotWords == 0))
			Shot(std::string(ModeName(mApp->mGameMode)) + "-word" + std::to_string(mWordsScored));
	}
	else
		Note("%s not taken (%s)", mPlanWord.c_str(), theBoard->mSelection.empty() ? "no selection" : "the selection stays");
}

// The first dialog (lowest id) in front: the new-user dialog gets a name, hint, message and can't-scramble
// dialogs their OK, the continue-game question a no. Anything else is unexpected (the bot opens no
// Options, Leave or Quit dialog, and must click no link): the run ends.
void AutoplayBot::HandleDialogs()
{
	DialogMap::iterator anItr = mApp->mDialogMap.begin();
	int anId = anItr->first;
	Dialog* aDialog = anItr->second;

	switch (anId)
	{
	case DIALOG_NEW_USER:
		{
			if (!Due(aDialog, 20, 50))
				return;
			NewUserDialog* aNewUser = (NewUserDialog*) aDialog;
			Note("new-user dialog: typing the name \"%s\"", BOT_PLAYER_NAME);
			for (size_t i = 0; i < aNewUser->mNameEdit->mString.size(); i++)
				Queue("key", "BACK");
			for (const char* aChar = BOT_PLAYER_NAME; *aChar != 0; aChar++)
				Queue("char", (*aChar == ' ') ? std::string("SPACE") : std::string(1, *aChar));
			Queue("key", "RETURN");
		}
		return;

	case DIALOG_HINT:
	case DIALOG_MESSAGE:
		if (Due(aDialog, 25, 70))
		{
			std::string aWhat = "the OK of dialog " + std::to_string(anId) + " (" + aDialog->mDialogHeader + ")";
			ClickWidget(aDialog->mYesButton, aWhat.c_str());
		}
		return;

	case DIALOG_CONTINUE:
		if (Due(aDialog, 25, 70))
			ClickWidget(aDialog->mNoButton, "no: a new game, not the saved one");
		return;

	default:
		Note("unexpected dialog %d (%s): quitting", anId, aDialog->mDialogHeader.c_str());
		Quit();
		return;
	}
}

// The Hall of Fame: logged and shot once its entries have pulsed a while, then continue
void AutoplayBot::HandleFame()
{
	FameWidget* aFame = mApp->mFameWidget;
	if ((!aFame->mVisible) || (aFame->mDisabled))
		return;

	if (!mFameNoted)
	{
		if (!Due(aFame, 150, 150))
			return;
		mFameNoted = true;

		int aMode = mApp->mGameMode;
		const HighScoreByScoreSet& aTable = (aMode == GAMEMODE_ACTION) ? mApp->mActionScores : mApp->mClassicScores;
		std::string aRows;
		int aRank = 1;
		for (HighScoreByScoreSet::const_iterator anItr = aTable.begin(); anItr != aTable.end(); ++anItr, aRank++)
			aRows += " " + std::to_string(aRank) + ". " + anItr->mName + " " + std::to_string(anItr->mScore) + (anItr->mIsNew ? " (new)" : "") + ";";
		Note("hall of fame, %s table:%s", ModeName(aMode), aRows.c_str());
		Shot(std::string(ModeName(aMode)) + "-fame");
		return;
	}

	if (Due(aFame, 30, 60))
		ClickWidget(aFame->mContinueButton, "the Hall of Fame's continue");
}

// The level / game-over summary: logged and shot once it has settled, then continue
void AutoplayBot::HandleSummary(Board* theBoard)
{
	LevelUpWidget* aSummary = theBoard->mLevelUpWidget;
	if ((aSummary->mFalling) || (aSummary->mSlide != 1.0))
		return;

	bool isGameOver = theBoard->mState == BOARD_GAMEOVER;
	if (!mSummaryNoted)
	{
		mSummaryNoted = true;
		StatKeeper* aStats = theBoard->mGameStats;
		std::string aMode = ModeName(mApp->mGameMode);
		if (isGameOver)
		{
			mGamesOver++;
			Note("game over summary (%s): score %d, level %d, %d words, %d letters, best word %s (%d), longest %s, %d tiles burned, "
				"%d scrambles, %d bonus words", aMode.c_str(), aStats->mScore, theBoard->mLevel + 1, aStats->mWords, aStats->mLetters,
				aStats->mBestWord.c_str(), aStats->mBestWordScore, aStats->mLongestWord.c_str(), aStats->mTilesBurned,
				aStats->mScrambles, aStats->mBonusWords);
			Shot(aMode + "-gameover");
		}
		else
		{
			mLevelUps++;
			Note("level %d complete (%s): score %d, %d words", theBoard->mLevel + 1, aMode.c_str(), aStats->mScore, aStats->mWords);
			Shot(aMode + "-levelup" + std::to_string(theBoard->mLevel + 1));
		}
		return;
	}

	if (Due(aSummary, 30, 80))
		ClickWidget(aSummary->mContinueButton, isGameOver ? "the game-over summary's continue" : "the level summary's continue");
}

// The game selector at rest: the next mode, or the end of the run
void AutoplayBot::HandleSelector()
{
	BWGameSelector* aSelector = mApp->mGameSelector;
	if ((aSelector->mOpening) || (aSelector->mClosing) || (aSelector->mDisabled))
		return;

	if (mGamesStarted >= (int) mModes.size())
	{
		Note("back on the game selector after %d game(s): quitting in 100 updates", mGamesOver);
		mQuitAt = mApp->mUpdateCount + 100;
		return;
	}

	if (!Due(aSelector, 40, 100))
		return;

	int aMode = mModes[mGamesStarted];
	ButtonWidget* aButton = (aMode == GAMEMODE_ACTION) ? aSelector->mActionButton : aSelector->mClassicButton;
	std::string aWhat = std::string("the ") + ModeName(aMode) + " button";
	if (ClickWidget(aButton, aWhat.c_str()))
		mGamesStarted++;
}

// The board takes a click now: no dialog or other screen, in play, not paused, past the debounce after an
// unpause, and every tile at rest (none falling or burning: a click on a column with a burning tile is
// ignored, and a falling tile is not where the plan put it)
bool AutoplayBot::BoardTakesInput(Board* theBoard)
{
	if ((theBoard == NULL) || (!mApp->mDialogMap.empty()) || (mApp->mHelpWidget != NULL) || (mApp->mFameWidget != NULL) ||
		(mApp->mGameSelector != NULL))
		return false;
	if ((theBoard->mState != BOARD_PLAYING) || (theBoard->mPauseCount > 0) || (theBoard->mSummaryShown) ||
		(!theBoard->mVisible) || (theBoard->mDisabled))
		return false;
	if (mApp->mUpdateCount + 1 - mApp->mResumeUpdateCnt <= 14)
		return false;

	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = theBoard->mBoard[aRow][aCol];
			if (aPiece == NULL)
				continue;
			if ((aPiece->mBurning) || (fabs(aPiece->mX - theBoard->ColX(aCol)) > 3.5) ||
				(fabs(aPiece->mY - theBoard->RowY(aCol, aRow)) > 3.5))
				return false;
		}
	}
	return true;
}

// A click at x, y reaches the board and hits the tile in theCell
bool AutoplayBot::TileClickable(Board* theBoard, int theCell, int theX, int theY)
{
	Piece* aPiece = CellPiece(theBoard, theCell);
	return (aPiece != NULL) && (theBoard->PieceAt(theX, theY) == aPiece) &&
		(mApp->mWidgetManager->GetWidgetAt(theX, theY, NULL, NULL) == theBoard);
}

// Every word on the board (one path each): a DFS from every tile over the neighbours not yet in the path,
// cut where no dictionary word starts with the letters so far
void AutoplayBot::FindWords(Board* theBoard, std::vector<FoundWord>* theWords)
{
	enum { NUM_CELLS = BOARD_ROWS * BOARD_COLS };

	struct Search
	{
		const std::set<SexyString>* mDict;
		Board* mBoard;
		std::vector<int> mNeighbours[NUM_CELLS];
		bool mInPath[NUM_CELLS];
		std::vector<int> mPath;
		std::string mWord;
		std::set<std::string> mFound;
		std::vector<FoundWord>* mWords;

		void From(int theCell)
		{
			Piece* aPiece = CellPiece(mBoard, theCell);
			size_t aLength = mWord.size();
			mWord += aPiece->mLetter;
			if (aPiece->mLetter == 'Q')
				mWord += 'U';

			std::set<SexyString>::const_iterator anItr = mDict->lower_bound(mWord);
			if ((anItr != mDict->end()) && (anItr->compare(0, mWord.size(), mWord) == 0))
			{
				mInPath[theCell] = true;
				mPath.push_back(theCell);
				if ((mWord.size() >= 3) && (*anItr == mWord) && (mFound.insert(mWord).second))
				{
					FoundWord aWord;
					aWord.mWord = mWord;
					aWord.mPath = mPath;
					aWord.mFireTiles = 0;
					for (size_t i = 0; i < mPath.size(); i++)
					{
						if (CellPiece(mBoard, mPath[i])->mType == TILE_FIRE)
							aWord.mFireTiles++;
					}
					mWords->push_back(aWord);
				}
				if ((int) mWord.size() < MAX_WORD_LENGTH)
				{
					for (size_t i = 0; i < mNeighbours[theCell].size(); i++)
					{
						if (!mInPath[mNeighbours[theCell][i]])
							From(mNeighbours[theCell][i]);
					}
				}
				mPath.pop_back();
				mInPath[theCell] = false;
			}
			mWord.resize(aLength);
		}
	};

	Search aSearch;
	aSearch.mDict = &mApp->mDictionary;
	aSearch.mBoard = theBoard;
	aSearch.mWords = theWords;

	std::vector<int> aCells;
	for (int aCell = 0; aCell < NUM_CELLS; aCell++)
	{
		aSearch.mInPath[aCell] = false;
		Piece* aPiece = CellPiece(theBoard, aCell);
		if ((aPiece != NULL) && (aPiece->mLetter >= 'A') && (aPiece->mLetter <= 'Z') &&
			(TileClickable(theBoard, aCell, theBoard->ColX(CellCol(aCell)) + 28, theBoard->RowY(CellCol(aCell), CellRow(aCell)) + 28)))
			aCells.push_back(aCell);
	}
	for (size_t i = 0; i < aCells.size(); i++)
	{
		for (size_t j = 0; j < aCells.size(); j++)
		{
			if ((i != j) && (Board::IsAdjacent(CellPiece(theBoard, aCells[i]), CellPiece(theBoard, aCells[j]))))
				aSearch.mNeighbours[aCells[i]].push_back(aCells[j]);
		}
	}

	if (aSearch.mDict->empty())
		return;
	for (size_t i = 0; i < aCells.size(); i++)
		aSearch.From(aCells[i]);
}

std::string AutoplayBot::PathWord(Board* theBoard, const std::vector<int>& thePath)
{
	std::string aWord;
	for (size_t i = 0; i < thePath.size(); i++)
	{
		char aLetter = CellPiece(theBoard, thePath[i])->mLetter;
		aWord += aLetter;
		if (aLetter == 'Q')
			aWord += 'U';
	}
	return aWord;
}

AutoplayBot::Step& AutoplayBot::AddStep(int theKind, int theCell, Board* theBoard, int theDelay)
{
	Step aStep;
	aStep.mKind = theKind;
	aStep.mCell = theCell;
	aStep.mLetter = (theCell >= 0) ? CellPiece(theBoard, theCell)->mLetter : 0;
	aStep.mDX = Random(29) - 14;
	aStep.mDY = Random(29) - 14;
	aStep.mFrac = 100;
	aStep.mDelay = theDelay;
	aStep.mSubmit = false;
	mPlan.push_back(aStep);
	return mPlan.back();
}

// The input of a selection of the tiles in thePath and its end (theMethod), and a right click after it
// (clearAfter: a submitted non-word stays selected)
void AutoplayBot::PlanSelection(Board* theBoard, const std::vector<int>& thePath, int theMethod, bool clearAfter)
{
	int aThink = 15 + Random(50);
	if ((theMethod == METHOD_DRAG) || (theMethod == METHOD_DRAG_OFF))
	{
		AddStep(STEP_DOWN_TILE, thePath[0], theBoard, aThink);
		for (size_t i = 1; i < thePath.size(); i++)
		{
			// sometimes a move part of the way first (it stays on one of the two tiles)
			if (Random(3) == 0)
				AddStep(STEP_DRAG_TILE, thePath[i], theBoard, 1 + Random(3)).mFrac = 30 + Random(41);
			AddStep(STEP_DRAG_TILE, thePath[i], theBoard, 2 + Random(5));
		}
		if (theMethod == METHOD_DRAG)
			AddStep(STEP_UP_TILE, thePath.back(), theBoard, 1 + Random(4)).mSubmit = true;
		else
			AddStep(STEP_UP_OFF, -1, theBoard, 2 + Random(4)).mSubmit = true;
	}
	else
	{
		AddStep(STEP_CLICK_TILE, thePath[0], theBoard, aThink);
		for (size_t i = 1; i < thePath.size(); i++)
			AddStep(STEP_CLICK_TILE, thePath[i], theBoard, 4 + Random(12));
		if (theMethod == METHOD_BUTTON)
			AddStep(STEP_CLICK_SUBMIT, -1, theBoard, 8 + Random(20)).mSubmit = true;
		else if (theMethod == METHOD_CLICK_LAST)
			AddStep(STEP_CLICK_TILE, thePath.back(), theBoard, 6 + Random(15)).mSubmit = true;
	}

	if (clearAfter)
		AddStep(STEP_RCLICK_TILE, thePath[0], theBoard, 20 + Random(40));
}

// The board takes input and nothing is selected: the next move
void AutoplayBot::PlanNext(Board* theBoard)
{
	std::vector<FoundWord> aWords;
	FindWords(theBoard, &aWords);

	mPlanNum++;
	int aKind = Random(100);

	// A scramble (3%; 30% with 5 or more fire tiles on the board: each scramble of a game adds one more fire
	// tile than the last, so this soon gets the can't-scramble dialog; always when there is no word)
	int aScrambleChance = (theBoard->mFireTileCount >= 5) ? 30 : 3;
	if ((aKind < aScrambleChance) || (aWords.empty()))
	{
		mPlanKind = PLAN_SCRAMBLE;
		mPlanWord = "";
		Note("plan %d: scramble (%u words on the board, %d fire tiles)", mPlanNum, (unsigned) aWords.size(), theBoard->mFireTileCount);
		AddStep(STEP_CLICK_LEX, -1, theBoard, 20 + Random(60)).mSubmit = true;
		return;
	}

	// A random path (6%): submitted and cleared, or cancelled
	if (aKind < aScrambleChance + 6)
	{
		std::vector<int> aCells;
		for (int aCell = 0; aCell < BOARD_ROWS * BOARD_COLS; aCell++)
		{
			Piece* aPiece = CellPiece(theBoard, aCell);
			if ((aPiece != NULL) && (TileClickable(theBoard, aCell, theBoard->ColX(CellCol(aCell)) + 28,
				theBoard->RowY(CellCol(aCell), CellRow(aCell)) + 28)))
				aCells.push_back(aCell);
		}
		if (aCells.empty())
			return;

		std::vector<int> aPath(1, aCells[Random((int) aCells.size())]);
		int aLength = 3 + Random(4);
		while ((int) aPath.size() < aLength)
		{
			std::vector<int> aNext;
			for (size_t i = 0; i < aCells.size(); i++)
			{
				if ((std::find(aPath.begin(), aPath.end(), aCells[i]) == aPath.end()) &&
					(Board::IsAdjacent(CellPiece(theBoard, aPath.back()), CellPiece(theBoard, aCells[i]))))
					aNext.push_back(aCells[i]);
			}
			if (aNext.empty())
				break;
			aPath.push_back(aNext[Random((int) aNext.size())]);
		}

		mPlanWord = PathWord(theBoard, aPath);
		bool isWord = mApp->mDictionary.find(mPlanWord) != mApp->mDictionary.end();
		if (Random(4) == 0)
		{
			int aMethod = Random(2) ? METHOD_CLICKS_ONLY : METHOD_DRAG_OFF;
			mPlanKind = PLAN_CANCEL;
			Note("plan %d: select %s and cancel (%s)", mPlanNum, mPlanWord.c_str(), MethodName(aMethod));
			PlanSelection(theBoard, aPath, aMethod, aMethod == METHOD_CLICKS_ONLY);
		}
		else
		{
			int aMethod = Random(3);
			mPlanKind = PLAN_JUNK;
			Note("plan %d: random path %s%s (%s)", mPlanNum, mPlanWord.c_str(), isWord ? ", a word by chance" : "", MethodName(aMethod));
			PlanSelection(theBoard, aPath, aMethod, true);
		}
		return;
	}

	// A word: the bonus word when it is there (80%), else a long one (45%) or a short one without fire tiles
	const FoundWord* aChoice = NULL;
	const char* aWhy = "";
	if (!theBoard->mBonusWord.empty())
	{
		for (size_t i = 0; i < aWords.size(); i++)
		{
			if ((SameWordIgnoringCase(aWords[i].mWord, theBoard->mBonusWord)) && (Random(10) < 8))
			{
				aChoice = &aWords[i];
				aWhy = "the bonus word";
			}
		}
	}
	if (aChoice == NULL)
	{
		size_t aMinLength = 1000;
		size_t aMaxLength = 0;
		for (size_t i = 0; i < aWords.size(); i++)
		{
			aMinLength = std::min(aMinLength, aWords[i].mWord.size());
			aMaxLength = std::max(aMaxLength, aWords[i].mWord.size());
		}

		std::vector<const FoundWord*> aCandidates;
		if (Random(100) < 45)
		{
			for (size_t i = 0; i < aWords.size(); i++)
			{
				if (aWords[i].mWord.size() + 1 >= aMaxLength)
					aCandidates.push_back(&aWords[i]);
			}
			aWhy = "long";
		}
		else
		{
			for (size_t i = 0; i < aWords.size(); i++)
			{
				if ((aWords[i].mWord.size() == aMinLength) && (aWords[i].mFireTiles == 0))
					aCandidates.push_back(&aWords[i]);
			}
			if (aCandidates.empty())
			{
				for (size_t i = 0; i < aWords.size(); i++)
				{
					if (aWords[i].mWord.size() == aMinLength)
						aCandidates.push_back(&aWords[i]);
				}
			}
			aWhy = "short";
		}
		aChoice = aCandidates[Random((int) aCandidates.size())];
	}

	int aMethod = Random(3);
	mPlanKind = PLAN_WORD;
	mPlanWord = aChoice->mWord;
	Note("plan %d: %s (%s of %u words, %d fire tiles in it) by %s", mPlanNum, aChoice->mWord.c_str(), aWhy,
		(unsigned) aWords.size(), aChoice->mFireTiles, MethodName(aMethod));
	PlanSelection(theBoard, aChoice->mPath, aMethod, false);
}

// Queues the input of a due step (RUN_DONE / RUN_SKIP), or says to wait (RUN_WAIT) or that the plan is
// void (RUN_DROP)
int AutoplayBot::RunStep(Board* theBoard, const Step& theStep)
{
	if (!BoardTakesInput(theBoard))
		return mDragging ? RUN_DROP : RUN_WAIT;

	switch (theStep.mKind)
	{
	case STEP_CLICK_TILE:
	case STEP_DOWN_TILE:
	case STEP_DRAG_TILE:
	case STEP_UP_TILE:
	case STEP_RCLICK_TILE:
		{
			int aCell = theStep.mCell;
			if (theStep.mKind == STEP_RCLICK_TILE)
			{
				// on a selected tile: the one the plan names may have gone
				if (theBoard->mSelection.empty())
					return RUN_SKIP;
				Piece* aFront = theBoard->mSelection.front();
				aCell = aFront->mRow * BOARD_COLS + aFront->mCol;
			}
			else
			{
				Piece* aPiece = CellPiece(theBoard, aCell);
				if ((aPiece == NULL) || (aPiece->mLetter != theStep.mLetter))
					return RUN_DROP;
			}

			Piece* aPiece = CellPiece(theBoard, aCell);
			int x = theBoard->ColX(CellCol(aCell)) + 28 + theStep.mDX;
			int y = theBoard->RowY(CellCol(aCell), CellRow(aCell)) + 28 + theStep.mDY;
			if ((theStep.mKind == STEP_DRAG_TILE) && (theStep.mFrac < 100))
			{
				// part of the way: only while that stays on the last tile or the next one
				x = mDragX + (x - mDragX) * theStep.mFrac / 100;
				y = mDragY + (y - mDragY) * theStep.mFrac / 100;
				Piece* aHit = theBoard->PieceAt(x, y);
				if ((aHit != aPiece) && ((mDragCell < 0) || (aHit != CellPiece(theBoard, mDragCell))))
					return RUN_SKIP;
				mDragX = x;
				mDragY = y;
				QueueAt("move", x, y);
				return RUN_DONE;
			}

			if ((theStep.mKind == STEP_DRAG_TILE) || (theStep.mKind == STEP_UP_TILE))
			{
				if (theBoard->PieceAt(x, y) != aPiece)
					return RUN_DROP;
			}
			else if (!TileClickable(theBoard, aCell, x, y))
				return RUN_DROP;

			switch (theStep.mKind)
			{
			case STEP_CLICK_TILE:
				QueueAt("click", x, y);
				break;
			case STEP_RCLICK_TILE:
				QueueAt("rclick", x, y);
				break;
			case STEP_DOWN_TILE:
				QueueAt("down", x, y);
				mDragging = true;
				break;
			case STEP_DRAG_TILE:
				QueueAt("move", x, y);
				break;
			case STEP_UP_TILE:
				QueueAt("up", x, y);
				mDragging = false;
				break;
			}
			mDragX = x;
			mDragY = y;
			mDragCell = aCell;
		}
		return RUN_DONE;

	case STEP_UP_OFF:
		if (theBoard->PieceAt(OFF_TILE_X, OFF_TILE_Y) != NULL)
			return RUN_DROP;
		QueueAt("move", OFF_TILE_X, OFF_TILE_Y);
		QueueAt("up", OFF_TILE_X, OFF_TILE_Y);
		mDragging = false;
		return RUN_DONE;

	case STEP_CLICK_SUBMIT:
		return ClickWidget(theBoard->mSubmitButton, "the Submit button") ? RUN_DONE : RUN_WAIT;

	case STEP_CLICK_LEX:
		{
			Lex* aLex = theBoard->mLex;
			if (aLex == NULL)
				return RUN_DROP;
			if (aLex->IsMoving())
				return RUN_WAIT;
			return ClickWidget(aLex, "Lex (scramble)") ? RUN_DONE : RUN_WAIT;
		}
	}

	return RUN_DROP;
}

// A drag cut short: the button goes up where the mouse is
void AutoplayBot::EndDrag(const char* theWhy)
{
	Note("drag ended early (%s)", theWhy);
	QueueAt("up", mDragX, mDragY);
	mDragging = false;
}

void AutoplayBot::DropPlan(const char* theWhy)
{
	Note("plan %d dropped (%s)", mPlanNum, theWhy);
	if (mDragging)
		EndDrag(theWhy);
	mPlan.clear();
	mPlanPos = 0;
	mWaitedSince = -1;
}

void AutoplayBot::PlayBoard(Board* theBoard)
{
	int aNext = mApp->mUpdateCount + 1;

	if (mPlanPos < mPlan.size())
	{
		if (aNext < mNextStepUpdate)
			return;

		const Step& aStep = mPlan[mPlanPos];
		bool isSubmit = aStep.mSubmit;
		int aScore = theBoard->mGameStats->mScore;
		switch (RunStep(theBoard, aStep))
		{
		case RUN_DONE:
			if (isSubmit)
			{
				mScoreBefore = aScore;
				mResultUpdate = aNext;
			}
			// fall through
		case RUN_SKIP:
			mPlanPos++;
			mWaitedSince = -1;
			if (mPlanPos < mPlan.size())
				mNextStepUpdate = aNext + mPlan[mPlanPos].mDelay;
			else
			{
				mPlan.clear();
				mPlanPos = 0;
			}
			break;

		case RUN_WAIT:
			if (mWaitedSince < 0)
				mWaitedSince = mApp->mUpdateCount;
			else if (mApp->mUpdateCount - mWaitedSince > 1000)
				DropPlan("the board took no input for 1000 updates");
			break;

		default:
			DropPlan(BoardTakesInput(theBoard) ? "the tiles changed" : "the board stopped taking input");
			break;
		}
		return;
	}

	if (!BoardTakesInput(theBoard))
		return;

	mPlan.clear();
	mPlanPos = 0;
	if (!theBoard->mSelection.empty())
	{
		Piece* aFront = theBoard->mSelection.front();
		mPlanKind = PLAN_CLEAR;
		mPlanWord = "";
		Note("a selection is left over: right click");
		AddStep(STEP_RCLICK_TILE, aFront->mRow * BOARD_COLS + aFront->mCol, theBoard, 10 + Random(20));
	}
	else
		PlanNext(theBoard);

	if (!mPlan.empty())
		mNextStepUpdate = aNext + mPlan[0].mDelay;
}

void AutoplayBot::Update()
{
	if (mDone)
		return;

	Board* aBoard = mApp->mBoard;
	CheckResult(aBoard);
	LogChanges(aBoard);

	int aNext = mApp->mUpdateCount + 1;
	if (aNext >= mUpdateCap)
	{
		Note("update cap");
		Quit();
		return;
	}

	if (mQuitAt >= 0)
	{
		if (aNext >= mQuitAt)
			Quit();
		return;
	}

	// A drag ends as soon as anything else takes the screen
	if ((mDragging) && (!BoardTakesInput(aBoard)))
	{
		DropPlan("the board stopped taking input");
		return;
	}

	if (mApp->mTitleScreen != NULL)
	{
		HyperlinkWidget* aLink = mApp->mTitleScreen->mPlayLink;
		if ((!mTitleClicked) && (mApp->mTitleScreen->mLoadDone) && (aLink->mVisible) && (Due(aLink, 10, 40)))
			mTitleClicked = ClickWidget(aLink, "the title's play link");
		return;
	}

	if (mApp->mFameWidget == NULL)
		mFameNoted = false;
	LevelUpWidget* aSummary = (aBoard != NULL) ? aBoard->mLevelUpWidget : NULL;
	bool hasSummary = (aSummary != NULL) && (aSummary->mVisible) && (!aSummary->mDisabled) && (aBoard->mSummaryShown);
	if (!hasSummary)
		mSummaryNoted = false;

	if (!mApp->mDialogMap.empty())
		HandleDialogs();
	else if (mApp->mFameWidget != NULL)
		HandleFame();
	else if (mApp->mHelpWidget != NULL)
	{
		if (Due(mApp->mHelpWidget, 30, 80))
			ClickWidget(mApp->mHelpWidget->mContinueButton, "the help screen's continue");
	}
	else if (hasSummary)
		HandleSummary(aBoard);
	else if (mApp->mGameSelector != NULL)
		HandleSelector();
	else if (aBoard != NULL)
		PlayBoard(aBoard);
}
