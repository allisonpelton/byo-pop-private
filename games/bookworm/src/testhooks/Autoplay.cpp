// port: test hook for -autoplay=<seed> (see Autoplay.h and BookWormApp.h). Not part of the original game.

#include "Autoplay.h"
#include "BookWormApp.h"
#include "Board.h"
#include "Lex.h"
#include "OverlayWidget.h"
#include "WordBalloon.h"
#include "TitleScreen.h"
#include "BWGameSelector.h"
#include "HelpWidget.h"
#include "FameWidget.h"
#include "LevelUpWidget.h"
#include "NewUserDialog.h"
#include "ColorizedButton.h"
#include "Res.h"
#include "Solver.h"
#include "Undo.h"
#include "GreedyBots.h"
#include "ModFeatures.h"
#include "TypedWord.h"
#include "WordReview.h"
#include "SidePanel.h"

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

bool SameWordIgnoringCase(const std::string& theFirstWord, const std::string& theSecondWord)
{
	if (theFirstWord.size() != theSecondWord.size())
		return false;
	for (size_t i = 0; i < theFirstWord.size(); i++)
	{
		if (toupper((unsigned char) theFirstWord[i]) != toupper((unsigned char) theSecondWord[i]))
			return false;
	}
	return true;
}

}

AutoplayBot::AutoplayBot(BookWormApp* theApp, unsigned long theSeed, const std::string& theMode, int theUpdateCap,
	const std::string& theLogFileName, const std::string& theShotPrefix, int theShotWords, bool theUseHints,
	bool theTestUndo, bool theCheckModel, bool theTypeWords) :
	mApp(theApp),
	mRand((std::mt19937::result_type) theSeed),
	mSeed(theSeed),
	mModeName(theMode),
	mUpdateCap(theUpdateCap),
	mLog(NULL),
	mShotPrefix(theShotPrefix),
	mShotWords(theShotWords),
	mUseHints(theUseHints),
	mHintScore(0),
	mHintMismatches(0),
	mFireState(0),
	mFirePredicted(0.0),
	mFireSure(0),
	mFireBase(0),
	mFireWords(0),
	mFireExpected(0.0),
	mFireCame(0),
	mFireMismatches(0),
	mFireTagShot(false),
	mTestUndo(theTestUndo),
	mUndoPhase(0),
	mUndoWordsScored(0),
	mUndoChecks(0),
	mUndoMismatches(0),
	mCheckModel(theCheckModel),
	mTypeWords(theTypeWords),
	mTypeChecks(0),
	mTypeMismatches(0),
	mModel(NULL),
	mModelBoard(NULL),
	mModelMoves(0),
	mModelChecks(0),
	mModelMismatches(0),
	mDone(false),
	mQuitAt(-1),
	mTitleClicked(false),
	mGamesStarted(0),
	mGamesOver(0),
	mWaitKey(NULL),
	mWaitUntil(0),
	mSummaryNoted(false),
	mSummaryNotedAt(0),
	mModsShot(false),
	mJournalStep(0),
	mJournalStepAt(0),
	mPreviewNoted(false),
	mPreviewShots(0),
	mPanelShot(false),
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

	// a comma-separated list of games in order: classic, action, or both (Classic, then Action)
	std::string aList = mModeName + ",";
	for (size_t aStart = 0, aComma; (aComma = aList.find(',', aStart)) != std::string::npos; aStart = aComma + 1)
	{
		std::string aName = aList.substr(aStart, aComma - aStart);
		if ((aName == "classic") || (aName == "both"))
			mModes.push_back(GAMEMODE_CLASSIC);
		if ((aName == "action") || (aName == "both"))
			mModes.push_back(GAMEMODE_ACTION);
		if ((aName != "classic") && (aName != "action") && (aName != "both"))
			SDL_Log("autoplay: unknown game \"%s\" in -autoplaymode=%s, skipped", aName.c_str(), mModeName.c_str());
	}
	if (mModes.empty())
	{
		mModeName = "classic";
		mModes.push_back(GAMEMODE_CLASSIC);
	}

	Note("-autoplay=%lu -seed=%lu -autoplaymode=%s -autoplaycap=%d%s%s%s%s: the bot's input as a -script file", mSeed,
		(unsigned long) mApp->mRandSeed, mModeName.c_str(), mUpdateCap, mUseHints ? " -autoplayhints=1" : "",
		mTestUndo ? " -autoplayundo=1" : "", mCheckModel ? " -autoplaymodel=1" : "", mTypeWords ? " -autoplaytype=1" : "");
	if ((mCheckModel) && (mTestUndo))
	{
		Note("-autoplaymodel: the model cannot undo; no model check with -autoplayundo");
		mCheckModel = false;
	}
}

AutoplayBot::~AutoplayBot()
{
	delete mModel;
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

// -autoplayshots: a screenshot at the start of the next update (at the full size under -renderscale, so HD
// art can be checked)
void AutoplayBot::Shot(const std::string& theEvent)
{
	if (!mShotPrefix.empty())
		Queue((mApp->mForcedRenderScale > 1) ? "screenshotfull" : "screenshot", mShotPrefix + theEvent + ".png");
}

void AutoplayBot::Quit()
{
	if (mDragging)
		EndDrag("quit");
	NoteTyped();
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

	// the selected word's definition (mod "select-definitions"): noted each time it shows, the first one shot
	// once its balloon has faded in
	bool isPreview = (theBoard->mLex != NULL) && (theBoard->mLex->mPreviewShowing);
	if ((isPreview) && (!mPreviewNoted))
	{
		Note("definition shown for the selection %s", theBoard->mSelectDefWord.c_str());
		mPreviewShots++;
	}
	mPreviewNoted = isPreview;
	if ((isPreview) && (mPreviewShots == 1) && (theBoard->mOverlay != NULL) && (theBoard->mOverlay->mBalloon->mAlpha >= 1.0))
	{
		mPreviewShots++;
		Shot(std::string(ModeName(mApp->mGameMode)) + "-preview");
	}

	// the side panel (mod "widescreen"): shot once, after 8 words, with the tiles at rest (the solver's list up)
	if ((!mPanelShot) && (mApp->mSidePanel != NULL) && (mWordsScored >= 8) && (Solver::BoardAtRest(theBoard)) &&
		((theBoard->mSolver == NULL) || (theBoard->mSolver->IsCurrent())))
	{
		mPanelShot = true;
		Shot(std::string(ModeName(mApp->mGameMode)) + "-panel");
	}

	if (theBoard->mState != mLastState)
	{
		Note("board: %s -> %s, level %d, score %d, %d fire tiles", StateName(mLastState), StateName(theBoard->mState),
			theBoard->mLevel + 1, theBoard->mGameStats->mScore, theBoard->mFireTileCount);
		if (theBoard->mState == BOARD_GAMEOVER_PENDING)
		{
			Note("game over: a fire tile burned through the bottom");
			if (mUseHints)
				Note("hints: %d words scored, %d mismatches", mWordsScored, mHintMismatches);
			NoteFireTotals();
			if (mTestUndo)
				Note("undo: %d checks, %d mismatches", mUndoChecks, mUndoMismatches);
			NoteTyped();
			CheckModelEnd(theBoard);
		}
		if ((theBoard->mState == BOARD_GAMEOVER) && (theBoard->mBots != NULL))
		{
			GreedyBots* aBots = theBoard->mBots;
			Note("greedy bots%s after %.0f ms: best word %d (level %d, %d words, end %d), fire first %d (level %d, "
				"%d words, end %d), least fire %d (level %d, %d words, end %d)", aBots->Finished() ? "" : " (still playing)",
				aBots->mElapsedMs,
				aBots->mResults[GREEDY_BEST_WORD].mScore, aBots->mResults[GREEDY_BEST_WORD].mLevel + 1,
				aBots->mResults[GREEDY_BEST_WORD].mWords, aBots->mResults[GREEDY_BEST_WORD].mEnd,
				aBots->mResults[GREEDY_FIRE_FIRST].mScore, aBots->mResults[GREEDY_FIRE_FIRST].mLevel + 1,
				aBots->mResults[GREEDY_FIRE_FIRST].mWords, aBots->mResults[GREEDY_FIRE_FIRST].mEnd,
				aBots->mResults[GREEDY_LEAST_FIRE].mScore, aBots->mResults[GREEDY_LEAST_FIRE].mLevel + 1,
				aBots->mResults[GREEDY_LEAST_FIRE].mWords, aBots->mResults[GREEDY_LEAST_FIRE].mEnd);
		}
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
		mFireState = 0;
		return;
	}

	int aScore = theBoard->mGameStats->mScore;
	if (aScore != mScoreBefore)
	{
		mWordsScored++;
		Note("word %d: %s scored %d (the word %d), total %d, level %d, %d fire tiles", mWordsScored,
			theBoard->mLastWord.mWord.c_str(), aScore - mScoreBefore, theBoard->mLastWord.mScore, aScore, theBoard->mLevel + 1,
			theBoard->mFireTileCount);
		if (theBoard->mWordReview != NULL)
		{
			// the mods "missed-words" and "par": the word against the board's best
			const ReviewTotals& aLevel = theBoard->mWordReview->LevelTotals();
			const ReviewTotals& aGame = theBoard->mWordReview->mGameTotals;
			Note("review: level %d words, %d points of par %d (%d%%), %d missed; game %d words, %d of par %d, %d missed",
				aLevel.mWords, aLevel.mScore, aLevel.mPar, aLevel.ParPercent(), aLevel.mMissed, aGame.mWords, aGame.mScore,
				aGame.mPar, aGame.mMissed);
		}
		if (mFireState == 1)
			mFireState = 2;
		if ((!mHintWord.empty()) && ((theBoard->mLastWord.mWord != mHintWord) || (theBoard->mLastWord.mScore != mHintScore)))
		{
			mHintMismatches++;
			Note("HINT MISMATCH: the solver had %s for %d", mHintWord.c_str(), mHintScore);
		}
		if ((mShotWords > 0) && (mWordsScored % mShotWords == 0))
			Shot(std::string(ModeName(mApp->mGameMode)) + "-word" + std::to_string(mWordsScored));
	}
	else
	{
		Note("%s not taken (%s)", mPlanWord.c_str(), theBoard->mSelection.empty() ? "no selection" : "the selection stays");
		mFireState = 0;
		if (!mHintWord.empty())
		{
			mHintMismatches++;
			Note("HINT MISMATCH: the solver's %s was no word", mHintWord.c_str());
		}
	}
	mHintWord = "";
}

// The solver's "fewerFire": once the board rests after the hint's word, the new fire tiles against the prediction
void AutoplayBot::CheckNewFire(Board* theBoard)
{
	if ((mFireState != 2) || (theBoard == NULL) || (!Solver::BoardAtRest(theBoard)))
		return;
	mFireState = 0;
	int aCame = theBoard->mFireTileCount - mFireBase;
	mFireWords++;
	mFireExpected += mFirePredicted;
	mFireCame += aCame;
	Note("fire: %d new fire tiles came (%.2f expected)", aCame, mFirePredicted);
	if ((aCame < mFireSure) || ((mFirePredicted == 0.0) && (aCame > 0)))
	{
		mFireMismatches++;
		Note("FIRE MISMATCH: %d new fire tiles came, %.2f expected (%d certain)", aCame, mFirePredicted, mFireSure);
	}
}

void AutoplayBot::NoteFireTotals()
{
	if (mFireWords > 0)
		Note("fire prediction: %d words, %.1f new fire tiles expected, %d came, %d mismatches", mFireWords,
			mFireExpected, mFireCame, mFireMismatches);
}

// The first dialog (lowest id) in front: the new-user dialog gets a name, hint, message and can't-scramble
// dialogs their OK, the continue-game question a no (-autoplaycontinue: a yes). Anything else is unexpected (the bot opens no
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
		if ((Due(aDialog, 25, 70)) && (mApp->mAutoplayContinue))
			ClickWidget(aDialog->mYesButton, "yes: the saved game goes on");
		else if (mWaitKey == NULL)
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
		mSummaryNotedAt = mApp->mUpdateCount;
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

	// the mods' balloon fades in after the slide; the continue comes 31 updates after the note at the earliest
	if ((isGameOver) && (!mModsShot) && (aSummary->mModsBalloon != NULL) && (mApp->mUpdateCount >= mSummaryNotedAt + 25))
	{
		mModsShot = true;
		Shot(std::string(ModeName(mApp->mGameMode)) + "-gameover-mods");
	}
	// the word journal (mod "word-journal"): at game over its page is opened once, shot and closed with a click
	// outside the book, before the continue
	if ((isGameOver) && (aSummary->mJournalButton != NULL) && (mJournalStep < 4))
	{
		int aSince = mApp->mUpdateCount - mJournalStepAt;
		if (mJournalStep == 0)
		{
			if ((mApp->mUpdateCount >= mSummaryNotedAt + 30) && (ClickWidget(aSummary->mJournalButton, "the word journal's button")))
			{
				mJournalStep = 1;
				mJournalStepAt = mApp->mUpdateCount;
			}
		}
		else if ((mJournalStep == 1) && (aSummary->mJournal != NULL) && (aSince >= 15))
		{
			Shot(std::string(ModeName(mApp->mGameMode)) + "-journal");
			mJournalStep = 2;
			mJournalStepAt = mApp->mUpdateCount;
		}
		else if ((mJournalStep == 2) && (aSince >= 10))
		{
			Note("click the word journal's page, outside the book");
			QueueAt("click", 30, 30);
			mJournalStep = 3;
			mJournalStepAt = mApp->mUpdateCount;
		}
		else if ((mJournalStep == 3) && (aSummary->mJournal == NULL))
			mJournalStep = 4;
		else if ((mJournalStep != 0) && (aSince >= 200))
		{
			Note("the word journal did not %s", (mJournalStep == 1) ? "open" : "close");
			mJournalStep = 4;
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

// Every word on the board (one path each, the first the search finds): the solver's search
// (FindBoardWords: a DFS from every tile over the neighbours not yet in the path, cut where no dictionary
// word starts with the letters so far) over the tiles a click reaches, in cell order
void AutoplayBot::FindWords(Board* theBoard, std::vector<FoundWord>* theWords)
{
	enum { NUM_CELLS = BOARD_ROWS * BOARD_COLS };

	std::vector<Piece*> aTiles;
	for (int aCell = 0; aCell < NUM_CELLS; aCell++)
	{
		Piece* aPiece = CellPiece(theBoard, aCell);
		if ((aPiece != NULL) && (aPiece->mLetter >= 'A') && (aPiece->mLetter <= 'Z') &&
			(TileClickable(theBoard, aCell, theBoard->ColX(CellCol(aCell)) + 28, theBoard->RowY(CellCol(aCell), CellRow(aCell)) + 28)))
			aTiles.push_back(aPiece);
	}

	std::set<std::string> aFound;
	FindBoardWords(aTiles, mApp->mDictionary, [&](const std::vector<Piece*>& thePath, const SexyString& theWord)
	{
		if (!aFound.insert(theWord).second)
			return;
		FoundWord aWord;
		aWord.mWord = theWord;
		aWord.mFireTiles = 0;
		for (size_t i = 0; i < thePath.size(); i++)
		{
			aWord.mPath.push_back(thePath[i]->mRow * BOARD_COLS + thePath[i]->mCol);
			if (thePath[i]->mType == TILE_FIRE)
				aWord.mFireTiles++;
		}
		theWords->push_back(aWord);
	});
}

// -autoplayundo: what an undo must put back, and a replay make again: every cell's letter and tile type,
// the score, the fire meter and the bonus word
std::string AutoplayBot::BoardState(Board* theBoard)
{
	std::string aState;
	for (int aCell = 0; aCell < BOARD_ROWS * BOARD_COLS; aCell++)
	{
		Piece* aPiece = CellPiece(theBoard, aCell);
		aState += (aPiece == NULL) ? '.' : aPiece->mLetter;
		aState += (aPiece == NULL) ? '.' : (char)('0' + aPiece->mType);
	}
	return aState + StrFormat(" score %d fire %.4f tiles %d bonus %s", theBoard->mGameStats->mScore,
		theBoard->mFireMeter, theBoard->mFireTileCount, theBoard->mBonusWord.c_str());
}

void AutoplayBot::CheckUndo(Board* theBoard, const std::string& theExpected, const char* theWhat)
{
	mUndoChecks++;
	std::string aState = BoardState(theBoard);
	if (aState == theExpected)
		Note("undo: %s as it should be", theWhat);
	else
	{
		mUndoMismatches++;
		Note("UNDO MISMATCH: %s: %s, expected %s", theWhat, aState.c_str(), theExpected.c_str());
	}
}

// -autoplayundo: the step of the undo test this move takes (true: planned or waiting), or false for a
// plain hint move. Every other word: played, undone, checked, played again, checked.
bool AutoplayBot::PlanUndoTest(Board* theBoard)
{
	if (mUndoPhase == 1)
	{
		// the word was played and the board rests: undo it (not across a level-up, nor a word not taken)
		if ((mWordsScored == mUndoWordsScored) || (theBoard->mUndo->Count() == 0))
			mUndoPhase = 0;
		else
		{
			mUndoAfter = BoardState(theBoard);
			mPlanKind = PLAN_UNDO;
			mPlanWord = "";
			Note("plan %d: undo the last word", mPlanNum);
			AddStep(STEP_UNDO, (mUndoChecks % 4 == 0) ? -1 : -2, theBoard, 15 + Random(40));
			mUndoPhase = 2;
			return true;
		}
	}
	if (mUndoPhase == 2)
	{
		if (theBoard->mUndo->mPending)
			return true;
		CheckUndo(theBoard, mUndoBefore, "the board after the undo");
		mUndoPhase = 3;
		mUndoWordsScored = mWordsScored;
		return false;
	}
	if (mUndoPhase == 3)
	{
		if ((mWordsScored != mUndoWordsScored) && (mApp->mGameMode != GAMEMODE_ACTION))
			CheckUndo(theBoard, mUndoAfter, "the board after the word played again");
		mUndoPhase = 4;
		return false;
	}
	// mUndoPhase 4: the word after a tested one was untested; this one is

	mUndoWordsScored = mWordsScored;
	mUndoPhase = 1;
	return false;
}

// -autoplaymodel: the game's last move made in the model (false: its tiles are not the model's)
bool AutoplayBot::ModelTakesMove(Board* theBoard)
{
	mModelMoves++;
	const std::vector<int>& aMove = theBoard->mModLastMove;
	if (aMove.empty())
		return mModel->Scramble();

	std::vector<Piece*> aPath;
	for (size_t i = 0; i < aMove.size(); i++)
	{
		Piece* aPiece = mModel->At(aMove[i] / BOARD_COLS, aMove[i] % BOARD_COLS);
		if (aPiece == NULL)
			return false;
		aPath.push_back(aPiece);
	}
	return mModel->Submit(aPath);
}

// -autoplaymodel: at each rest, the move the game took since the last one, made in the model; the two boards
// compared
void AutoplayBot::CheckModel(Board* theBoard)
{
	if ((!mCheckModel) || (mApp->mGameMode != GAMEMODE_CLASSIC))
		return;
	if (mModelBoard != theBoard)
	{
		delete mModel;
		mModel = NULL;
		mModelBoard = theBoard;
		if (theBoard->mModMoves != 0)
		{
			Note("model: the game had %d moves when the check began; no model check", theBoard->mModMoves);
			return;
		}
		mModel = new BotGame();
		mModel->Capture(theBoard);
		mModelMoves = 0;
		Note("model: from the game's first board");
		return;
	}
	if ((mModel == NULL) || (theBoard->mModMoves == mModelMoves))
		return;

	std::string aWhy;
	if (theBoard->mModMoves != mModelMoves + 1)
		aWhy = StrFormat("%d moves since the last rest", theBoard->mModMoves - mModelMoves);
	else if (!ModelTakesMove(theBoard))
		aWhy = "the model did not take the move";
	mModelChecks++;
	// after a bonus word the banner takes up the next one once it has slid away and back, which can be later
	// than the tiles rest; the model has it at once (BoardState ends with the bonus word)
	std::string aGame = BoardState(theBoard);
	bool isBannerLate = (theBoard->mBonusWord.empty()) && (!mModel->mBonusWord.empty()) &&
		(mModel->mBonusWord == mApp->mBonusWord);
	if (isBannerLate)
		aGame += mModel->mBonusWord;
	aGame += StrFormat(" level %d", theBoard->mLevel + 1);
	std::string aModel = mModel->State() + StrFormat(" level %d", mModel->mLevel + 1);
	if ((aWhy.empty()) && (aGame == aModel))
	{
		Note("model: as the game after move %d%s", mModelMoves, isBannerLate ? " (its bonus banner still on its way)" : "");
		return;
	}

	mModelMismatches++;
	Note("MODEL MISMATCH after move %d%s%s: the game %s, the model %s", theBoard->mModMoves, aWhy.empty() ? "" : ": ",
		aWhy.c_str(), aGame.c_str(), aModel.c_str());
	mModel->Capture(theBoard);
	mModelMoves = theBoard->mModMoves;
}

// -autoplaymodel: the move that ended the game ends the model's, with the same score
void AutoplayBot::CheckModelEnd(Board* theBoard)
{
	if ((!mCheckModel) || (mModel == NULL) || (mModelBoard != theBoard))
		return;
	if (theBoard->mModMoves == mModelMoves + 1)
	{
		bool isTaken = ModelTakesMove(theBoard);
		mModelChecks++;
		if ((isTaken) && (mModel->IsOver()) && (mModel->mScore == theBoard->mGameStats->mScore))
			Note("model: over as the game after move %d, score %d", mModelMoves, mModel->mScore);
		else
		{
			mModelMismatches++;
			Note("MODEL MISMATCH at game over after move %d: the game's score %d, the model's %d (%s)", theBoard->mModMoves,
				theBoard->mGameStats->mScore, mModel->mScore, mModel->IsOver() ? "over" : "not over");
		}
	}
	Note("model: %d checks, %d mismatches", mModelChecks, mModelMismatches);
	delete mModel;
	mModel = NULL;
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

	// -autoplayhints: the solver's first hint, asked for with Tab, submitted with the Submit button
	if ((mUseHints) && (theBoard->mSolver == NULL))
	{
		Note("-autoplayhints: the mod \"solver\" is off, so the bot picks its own words");
		mUseHints = false;
	}
	if ((mTestUndo) && ((!mUseHints) || (theBoard->mUndo == NULL)))
	{
		Note("-autoplayundo: needs -autoplayhints=1 and the mod \"undo\"; no undo test");
		mTestUndo = false;
	}
	if ((mTypeWords) && (!ModFeatureEnabled("typeWord")))
	{
		Note("-autoplaytype: the mod \"type-word\" is off; no typing");
		mTypeWords = false;
	}
	mTyped.clear();
	if ((mTestUndo) && (PlanUndoTest(theBoard)))
		return;
	CheckModel(theBoard);
	if ((mUseHints) && (!aWords.empty()))
	{
		mPlanKind = PLAN_HINT;
		mPlanWord = "";
		Note("plan %d: the first hint (%u words on the board, %d fire tiles)", mPlanNum, (unsigned) aWords.size(),
			theBoard->mFireTileCount);
		AddStep(STEP_KEY_HINT, -1, theBoard, 15 + Random(50));
		AddStep(STEP_CLICK_SUBMIT, -1, theBoard, 8 + Random(20)).mSubmit = true;
		return;
	}

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
	if (mTypeWords)
	{
		Note("plan %d: %s (%s of %u words, %d fire tiles in it) typed", mPlanNum, aChoice->mWord.c_str(), aWhy,
			(unsigned) aWords.size(), aChoice->mFireTiles);
		PlanTyping(theBoard, aChoice->mWord);
		return;
	}
	Note("plan %d: %s (%s of %u words, %d fire tiles in it) by %s", mPlanNum, aChoice->mWord.c_str(), aWhy,
		(unsigned) aWords.size(), aChoice->mFireTiles, MethodName(aMethod));
	PlanSelection(theBoard, aChoice->mPath, aMethod, false);
}

// -autoplaytype: theWord typed and submitted with Enter; one time in eight a letter is taken back and typed
// again, one in eight the word is begun, cleared with Esc and typed afresh. Letters are mostly lower case, as
// a keyboard types them.
void AutoplayBot::PlanTyping(Board* theBoard, const std::string& theWord)
{
	mTypeCells = BoardState(theBoard).substr(0, BOARD_ROWS * BOARD_COLS * 2);
	int aVariant = Random(8);
	std::string aLetters = theWord;
	for (size_t i = 0; i < aLetters.size(); i++)
	{
		if (Random(4) != 0)
			aLetters[i] = (char) tolower((unsigned char) aLetters[i]);
	}
	if ((aVariant == 0) && (aLetters.size() > 2))
	{
		AddStep(STEP_TYPE_CHAR, -1, theBoard, 15 + Random(50)).mLetter = aLetters[0];
		AddStep(STEP_TYPE_CHAR, -1, theBoard, 3 + Random(10)).mLetter = aLetters[1];
		AddStep(STEP_KEY_ESCAPE, -1, theBoard, 10 + Random(20));
	}
	for (size_t i = 0; i < aLetters.size(); i++)
		AddStep(STEP_TYPE_CHAR, -1, theBoard, (i == 0) ? 15 + Random(50) : 3 + Random(10)).mLetter = aLetters[i];
	if (aVariant == 1)
	{
		AddStep(STEP_KEY_BACK, -1, theBoard, 5 + Random(15));
		AddStep(STEP_TYPE_CHAR, -1, theBoard, 3 + Random(10)).mLetter = aLetters[aLetters.size() - 1];
	}
	AddStep(STEP_KEY_RETURN, -1, theBoard, 8 + Random(20)).mSubmit = true;
}

// -autoplaytype: the selection spells the letters typed so far (after a Q: the Qu tile's QU); isWord, before
// Enter: it is the best of the paths the word solver finds for the plan's word
void AutoplayBot::CheckTyped(Board* theBoard, bool isWord)
{
	std::vector<int> aSelected;
	for (PieceList::iterator anItr = theBoard->mSelection.begin(); anItr != theBoard->mSelection.end(); ++anItr)
		aSelected.push_back((*anItr)->mRow * BOARD_COLS + (*anItr)->mCol);
	std::string aSpelled = PathWord(theBoard, aSelected);
	std::string aTyped = mTyped;
	if ((!aTyped.empty()) && (aTyped[aTyped.size() - 1] == 'Q'))
		aTyped += 'U';
	mTypeChecks++;
	if (aSpelled != aTyped)
	{
		mTypeMismatches++;
		Note("TYPE MISMATCH: typed \"%s\", the selection spells \"%s\"", mTyped.c_str(), aSpelled.c_str());
		return;
	}
	if (!isWord)
		return;

	std::vector<Piece*> aTiles;
	for (int aCell = 0; aCell < BOARD_ROWS * BOARD_COLS; aCell++)
	{
		Piece* aPiece = CellPiece(theBoard, aCell);
		if ((aPiece != NULL) && (aPiece->mLetter >= 'A') && (aPiece->mLetter <= 'Z') && (!aPiece->mBurning))
			aTiles.push_back(aPiece);
	}
	std::vector<SolverWord> aWords;
	FindSolverWords(aTiles, mApp, &aWords);
	const SolverWord* aBest = NULL;
	int aPaths = 0;
	for (size_t i = 0; i < aWords.size(); i++)
	{
		if (!SameWordIgnoringCase(aWords[i].mWord, mPlanWord))
			continue;
		aPaths++;
		if ((aBest == NULL) || (TypedWord::Better(aWords[i], *aBest)))
			aBest = &aWords[i];
	}

	SolverWord aChosen;
	aChosen.mPath.assign(theBoard->mSelection.begin(), theBoard->mSelection.end());
	RateSolverWord(&aChosen, aTiles, mApp);
	static const char* const SAFETY_NAMES[] = { "safe", "sets fire", "fire to the bottom", "ends the game" };
	if (aBest == NULL)
	{
		mTypeMismatches++;
		Note("TYPE MISMATCH: the solver finds no path for %s", mPlanWord.c_str());
	}
	else if (TypedWord::Better(*aBest, aChosen))
	{
		mTypeMismatches++;
		Note("TYPE MISMATCH: %s typed takes %d fire tiles, drops %d, %s, %d points; a path takes %d, drops %d, %s, %d points",
			mPlanWord.c_str(), aChosen.mFireTiles, aChosen.mFireDrops, SAFETY_NAMES[aChosen.mSafety], aChosen.mScore,
			aBest->mFireTiles, aBest->mFireDrops, SAFETY_NAMES[aBest->mSafety], aBest->mScore);
	}
	else
		Note("typed %s, the best of %d paths: %d fire tiles, %d dropped, %s, %d points", mPlanWord.c_str(), aPaths,
			aChosen.mFireTiles, aChosen.mFireDrops, SAFETY_NAMES[aChosen.mSafety], aChosen.mScore);
}

// -autoplaytype: the checks so far (at each game over and the quit; a game Action's clock ends goes from play
// to game over without the note)
void AutoplayBot::NoteTyped()
{
	if (mTypeWords)
		Note("typed: %d checks, %d mismatches", mTypeChecks, mTypeMismatches);
}

// Queues the input of a due step (RUN_DONE / RUN_SKIP), or says to wait (RUN_WAIT) or that the plan is
// void (RUN_DROP)
int AutoplayBot::RunStep(Board* theBoard, const Step& theStep)
{
	if (!BoardTakesInput(theBoard))
		return mDragging ? RUN_DROP : RUN_WAIT;
	// the first definition shown for a selection (mod "select-definitions") stays until it is shot
	if ((mPreviewShots == 1) && (theBoard->mLex != NULL) && (theBoard->mLex->mPreviewShowing))
		return RUN_WAIT;

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
		if (mPlanKind == PLAN_HINT)
		{
			const SolverWord* aHint = theBoard->mSolver->ShownHint();
			if (aHint == NULL)
				return RUN_DROP;
			static const char* const KIND_NAMES[] = { "fire", "bonus", "best", "longest", "cash in" };
			static const char* const SAFETY_NAMES[] = { "safe", "sets fire", "fire to the bottom", "ends the game" };
			if ((mTestUndo) && (mUndoPhase == 1))
				mUndoBefore = BoardState(theBoard);     // the board the undo must put back
			mPlanWord = aHint->mWord;
			mHintWord = aHint->mWord;
			mHintScore = aHint->mScore;
			std::string aCells;
			for (size_t i = 0; i < aHint->mPath.size(); i++)
				aCells += StrFormat("%s%d", (i == 0) ? "" : " ", aHint->mPath[i]->mRow * BOARD_COLS + aHint->mPath[i]->mCol);
			std::vector<Piece*> aTiles;
			int aBonusOnBoard = 0;
			for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
			{
				for (int aCol = 0; aCol < BOARD_COLS; aCol++)
				{
					Piece* aPiece = theBoard->mBoard[aRow][aCol];
					if ((aPiece == NULL) || (aPiece->mLetter < 'A') || (aPiece->mLetter > 'Z'))
						continue;
					aTiles.push_back(aPiece);
					if ((aPiece->mType == TILE_GREEN) || (aPiece->mType == TILE_GOLD) || (aPiece->mType == TILE_SAPPHIRE) ||
						(aPiece->mType == TILE_DIAMOND))
						aBonusOnBoard++;
				}
			}
			Note("hint %d/%u: %s (%s, %d points, %d fire tiles, %s, %d bonus tiles, %d on the board; cells %s)",
				theBoard->mSolver->mHintIdx + 1, (unsigned) theBoard->mSolver->mHints.size(), aHint->mWord.c_str(),
				KIND_NAMES[aHint->mKind], aHint->mScore, aHint->mFireTiles, SAFETY_NAMES[aHint->mSafety],
				aHint->mBonusTiles, aBonusOnBoard, aCells.c_str());
			// the solver's "saveBonus": a best or fire hint uses bonus tiles only where no path as safe (for the fire
			// hint, one that takes the same lowest fire tile) goes without them
			if ((ModFeatureBool("solver", "saveBonus", false)) && (aHint->mBonusTiles > 0) &&
				((aHint->mKind == SOLVER_HINT_BEST) || (aHint->mKind == SOLVER_HINT_FIRE)))
			{
				std::vector<SolverWord> anAll;
				FindSolverWords(aTiles, mApp, &anAll);
				for (const SolverWord& aWord : anAll)
				{
					if ((aWord.mBonusTiles == 0) && (aWord.mSafety <= aHint->mSafety) &&
						((aHint->mKind == SOLVER_HINT_BEST) || ((aWord.mFireTiles > 0) && (aWord.mFireRow == aHint->mFireRow))))
					{
						Note("SAVE MISMATCH: the hint %s uses %d bonus tiles, %s needs none", aHint->mWord.c_str(),
							aHint->mBonusTiles, aWord.mWord.c_str());
						break;
					}
				}
			}
			// the solver's "fewerFire": the new fire tiles to check once the board rests; the first best hint has the
			// fewest (by cost) of the words as safe, the earlier hints' words left out (while the list is the board's: a
			// bonus word whose banner came back after the hints were made makes them stale)
			if ((ModFeatureBool("solver", "fewerFire", false)) && (mApp->mGameMode == GAMEMODE_CLASSIC))
			{
				mFireState = 1;
				mFirePredicted = aHint->mNewFire;
				mFireSure = aHint->mNewFireSure;
				mFireBase = theBoard->mFireTileCount - aHint->mFireTiles;
				Note("fire: %.2f new fire tiles to expect (%d certain), cost %.2f, meter %.2f", aHint->mNewFire,
					aHint->mNewFireSure, aHint->mFireCost, theBoard->mFireMeter);
				if ((!mFireTagShot) && (aHint->mNewFire > 0.0))
				{
					mFireTagShot = true;
					Shot(std::string(ModeName(mApp->mGameMode)) + "-firetag");
				}
				const std::vector<SolverWord>& aHints = theBoard->mSolver->mHints;
				int anIdx = theBoard->mSolver->mHintIdx;
				bool isFirstBest = aHint->mKind == SOLVER_HINT_BEST;
				for (int i = 0; (i < anIdx) && (isFirstBest); i++)
					isFirstBest = aHints[i].mKind != SOLVER_HINT_BEST;
				if ((isFirstBest) && (theBoard->mSolver->IsCurrent()))
				{
					std::vector<SolverWord> anAll;
					FindSolverWords(aTiles, mApp, &anAll);
					SolverFireRater aRater = SolverFireRater::ForBoard(theBoard, aTiles);
					for (SolverWord& aWord : anAll)
					{
						bool isListed = false;
						for (int i = 0; i < anIdx; i++)
							isListed = isListed || (aHints[i].mWord == aWord.mWord);
						if (isListed)
							continue;
						aRater.Rate(&aWord);
						if ((aWord.mSafety < aHint->mSafety) || ((aWord.mSafety == aHint->mSafety) &&
							(SolverFireRater::CostOrder(aWord) < SolverFireRater::CostOrder(*aHint))))
						{
							Note("FIRE ORDER MISMATCH: the hint %s costs %.2f, %s %.2f", aHint->mWord.c_str(),
								aHint->mFireCost, aWord.mWord.c_str(), aWord.mFireCost);
							mFireMismatches++;
							break;
						}
					}
				}
			}
		}
		if ((mTypeWords) && (Random(2) == 0))
		{
			Note("Enter");
			Queue("key", "RETURN");
			return RUN_DONE;
		}
		return ClickWidget(theBoard->mSubmitButton, "the Submit button") ? RUN_DONE : RUN_WAIT;

	case STEP_TYPE_CHAR:
	case STEP_KEY_BACK:
	case STEP_KEY_ESCAPE:
	case STEP_KEY_RETURN:
		// in Action the board changes by itself (a fire tile burns the tile under it, which clears the
		// selection): the word may be gone
		if (BoardState(theBoard).compare(0, mTypeCells.size(), mTypeCells) != 0)
			return RUN_DROP;
		CheckTyped(theBoard, theStep.mKind == STEP_KEY_RETURN);
		switch (theStep.mKind)
		{
		case STEP_TYPE_CHAR:
			mTyped += (char) toupper((unsigned char) theStep.mLetter);
			Queue("char", std::string(1, theStep.mLetter));
			break;
		case STEP_KEY_BACK:
			if (!mTyped.empty())
				mTyped.erase(mTyped.size() - 1);
			Queue("key", "BACK");
			break;
		case STEP_KEY_ESCAPE:
			mTyped.clear();
			Queue("key", "ESCAPE");
			break;
		case STEP_KEY_RETURN:
			mTyped.clear();
			Queue("key", "RETURN");
			break;
		}
		return RUN_DONE;

	case STEP_KEY_HINT:
		// With the mod "widescreen", every other hint is the side panel's first row, clicked (no random draw, so
		// the game goes as with Tab)
		if ((mApp->mSidePanel != NULL) && (mPlanNum % 2 == 0))
		{
			Rect aRow = SidePanel::HintRect(0);
			int x = mApp->mSidePanel->mX + aRow.mX + aRow.mWidth / 2;
			int y = mApp->mSidePanel->mY + aRow.mY + aRow.mHeight / 2;
			if (mApp->mSidePanel->HintAt(x - mApp->mSidePanel->mX, y - mApp->mSidePanel->mY) != 0)
				return RUN_WAIT;
			Note("click the side panel's first hint");
			QueueAt("click", x, y);
			return RUN_DONE;
		}
		Queue("key", "TAB");
		return RUN_DONE;

	case STEP_UNDO:
		if (theStep.mCell == -2)
		{
			Note("Ctrl+Z");
			Queue("keydown", "CONTROL");
			Queue("key", "Z");
			Queue("keyup", "CONTROL");
			return RUN_DONE;
		}
		return ClickWidget(theBoard->mUndoButton, "the Undo button") ? RUN_DONE : RUN_WAIT;

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
	mTyped.clear();
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
	CheckNewFire(aBoard);
	LogChanges(aBoard);

	int aNext = mApp->mUpdateCount + 1;
	if (aNext >= mUpdateCap)
	{
		Note("update cap");
		if (mUseHints)
			Note("hints: %d words scored, %d mismatches", mWordsScored, mHintMismatches);
		NoteFireTotals();
		if (mTestUndo)
			Note("undo: %d checks, %d mismatches", mUndoChecks, mUndoMismatches);
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
	{
		mSummaryNoted = false;
		mModsShot = false;
		mJournalStep = 0;
	}

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
