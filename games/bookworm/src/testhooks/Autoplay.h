#ifndef __BOOKWORM_AUTOPLAY_H__
#define __BOOKWORM_AUTOPLAY_H__

// port: test hook for -autoplay=<seed> (see BookWormApp.h). Not part of the original game.
//
// A bot that plays BookWorm through script input: after every update it looks at the game and queues its
// next input with SexyAppBase::AddScriptCommand for the next update (click, rclick, down / move / up for
// drags, char and key), so its input takes exactly the path of a -script file's commands (RunScript,
// before WidgetManager::UpdateFrame). Its choices come from its own std::mt19937, never from the game's
// generators (gGameRand, the Board's MTRand, the CRT rand), so for a given -seed and -autoplay seed the run
// is the same every time. -autoplaylog=<file> writes the input as a -script file, with comments on what
// happened (words and their scores, level-ups, game over, the Hall of Fame), which replays the same game
// on any build, also one without the bot.
//
// What it does: clicks the title screen's play link once loading is done; types a player name and presses
// Enter when the new-user dialog appears; picks the mode on the game selector (-autoplaymode=classic,
// action or both: Classic, then Action; or a comma-separated list of those, played in order); continues past the help screen. On the board, whenever it takes
// input (no dialog, no summary, play state, no tile falling or burning), it finds the words on the board
// by a DFS over neighbouring tiles (Board::IsAdjacent, the hex layout; a Q tile is "QU") with the app's
// dictionary, and plays mostly a word (the bonus word when it is there, else a long word or a short one
// that avoids fire tiles, so fire tiles pile up and Classic ends), sometimes a random path (usually not a
// word, cleared again with a right click), a selection it cancels (right click, or a drag released off the
// tiles), or a scramble (a click on Lex; more often with 5 or more fire tiles on the board). A word is
// submitted with the Submit button, by clicking its last tile again, or by a drag, chosen at random; the
// time between clicks varies. It dismisses hint and "Can't Scramble!" dialogs, continues past the level
// summary, and after game over goes through the Hall of Fame back to the game selector, then quits 100
// updates later (or plays the next mode first). It never opens the Options dialog and never clicks a link
// other than the title's play link; an unexpected dialog makes it quit.
// -autoplaycap=<n> quits at update n (default 200000). -autoplayshots=<prefix> saves
// <prefix><mode>-levelup<n>.png, <prefix><mode>-gameover.png (with the mods "greedy-bots" or "daily-board" also
// <prefix><mode>-gameover-mods.png, once their balloon is up), <prefix><mode>-fame.png and, every
// -autoplayshotwords=<n> words (default 25, 0: none), <prefix><mode>-word<n>.png (script lines, so a
// replay of the log takes them too). Under -renderscale=<n> the shots are at the full size (screenshotfull).
// With the mod "word-journal" the bot opens the journal at game over, saves <prefix><mode>-journal.png and
// closes it; with "select-definitions" it waits for the first definition shown for a selection and saves
// <prefix><mode>-preview.png, and the log notes each one shown. With "widescreen" it saves <prefix><mode>-panel.png
// once, after 8 words, with the tiles at rest, and (-autoplayhints) takes every other hint by a click on the side
// panel's first row instead of Tab.
// -autoplayhints=1 (with the mod "solver" on): every move is the word solver's first hint, asked for with Tab
// and submitted with the Submit button (a scramble when the board spells no word). The log notes each hint
// and the word's score, and "HINT MISMATCH" where the game scored the word otherwise than the solver said.
// With the solver's setting "saveBonus" on, "SAVE MISMATCH" where a best or fire hint uses bonus tiles although
// a path as safe goes without them (for the fire hint, one that takes the same lowest fire tile).
// With its setting "fewerFire" on (Classic), each hint's new fire tiles as the solver predicts them against the ones
// that came once the board rests: "FIRE MISMATCH" where fewer came than the meter's certain ones, or some came where
// none could; the totals at the end. And "FIRE ORDER MISMATCH" where the first best hint is not one of the fewest
// new fire tiles (SolverFireRater's cost) among the words as safe.
// -autoplayundo=1 as well (with the mod "undo" on): every other word is undone (the Undo button and Ctrl+Z
// in turn), the board checked against the one before the word, the same hint played again and the board
// checked against the first play's; "UNDO MISMATCH" where one differs (letters, tile types, score, fire).
// In Action the replay is not checked: the timed fire tiles depend on when the word comes.
// -autoplaymodel=1 (Classic, without -autoplayundo): the greedy bots' model of the rules (GreedyBots.h) starts
// from the game's first board and takes every move the game takes (Board::mModLastMove); at each rest after
// a move the model's board, score, fire meter, bonus word and level must be the game's, and at game over the
// model's game must be over with the same score. "MODEL MISMATCH" where one differs (the model then starts
// again from the game's board). With the mod "greedy-bots" on, the game-over note gives the bots' results.
// -autoplaycontinue=1: the continue-game question gets a yes, so a game saved by an earlier run's quit goes on.
// -autoplaytype=1 (with the mod "type-word" on): the bot's words are typed, letter by letter (mostly lower
// case), and Enter submits them; now and then a letter is taken back with Backspace and typed again, or the
// word begun, cleared with Esc and typed afresh. Before each key the selection must spell the letters typed
// so far, and before Enter it must be the best of the word's paths (TypedWord::Better, over every path the
// word solver finds for it); "TYPE MISMATCH" where not. Hints and Submit-button words are sent with Enter
// half the time.

#include <cstdio>
#include <random>
#include <string>
#include <vector>

namespace Sexy
{

class BookWormApp;
class Board;
class BotGame;
class Widget;

class AutoplayBot
{
public:
	AutoplayBot(BookWormApp* theApp, unsigned long theSeed, const std::string& theMode, int theUpdateCap,
		const std::string& theLogFileName, const std::string& theShotPrefix, int theShotWords, bool theUseHints,
		bool theTestUndo, bool theCheckModel, bool theTypeWords);
	~AutoplayBot();

	// After each update (BookWormApp::UpdateFrames): looks at the game, queues input for the next update
	void Update();

private:
	enum StepKind
	{
		STEP_CLICK_TILE,    // click the tile in mCell (mDX/mDY from its centre)
		STEP_DOWN_TILE,     // button down on the tile, starts a drag
		STEP_DRAG_TILE,     // mouse move toward the tile, mFrac percent of the way from the last drag point
		STEP_UP_TILE,       // button up on the tile (ends a drag)
		STEP_UP_OFF,        // button up off the tiles (a drag released there clears the selection)
		STEP_RCLICK_TILE,   // right click on the tile (or any tile at rest): clears the selection
		STEP_CLICK_SUBMIT,  // the Submit button
		STEP_CLICK_LEX,     // Lex: scramble
		STEP_KEY_HINT,      // Tab: the word solver's next hint (-autoplayhints)
		STEP_UNDO,          // the Undo button (mCell -1) or Ctrl+Z (mCell -2) (-autoplayundo)
		STEP_TYPE_CHAR,     // types mLetter (-autoplaytype)
		STEP_KEY_BACK,      // Backspace: the last letter typed taken back
		STEP_KEY_ESCAPE,    // Esc: the typed word cleared
		STEP_KEY_RETURN     // Enter: the typed word submitted
	};

	struct Step
	{
		int mKind;
		int mCell;          // row * BOARD_COLS + col, -1 for none
		char mLetter;       // the tile's letter when planned
		int mDX;
		int mDY;
		int mFrac;          // STEP_DRAG_TILE: percent of the way
		int mDelay;         // updates after the previous step
		bool mSubmit;       // the step submits (or scrambles): the next update logs what came of it
	};

	enum PlanKind
	{
		PLAN_WORD,          // a dictionary word
		PLAN_JUNK,          // a random path, submitted (usually not a word), then cleared
		PLAN_CANCEL,        // a selection cleared before it is submitted
		PLAN_SCRAMBLE,      // a click on Lex
		PLAN_CLEAR,         // a right click on a left-over selection
		PLAN_HINT,          // the word solver's first hint, submitted (-autoplayhints)
		PLAN_UNDO           // the last word undone (-autoplayundo)
	};

	struct FoundWord
	{
		std::string mWord;
		std::vector<int> mPath;
		int mFireTiles;
	};

	BookWormApp* mApp;
	std::mt19937 mRand;
	unsigned long mSeed;
	std::string mModeName;
	std::vector<int> mModes;        // GameMode of each game to play, in order
	int mUpdateCap;
	FILE* mLog;
	std::string mShotPrefix;
	int mShotWords;
	bool mUseHints;
	std::string mHintWord;          // the hint submitted, "" for none
	int mHintScore;                 // its score as the solver has it
	int mHintMismatches;
	// the solver's "fewerFire": the hint's new fire tiles as predicted, checked once the board rests (CheckNewFire)
	int mFireState;                 // 0 none, 1 the hint submitted, 2 scored: waiting for the board to rest
	double mFirePredicted;
	int mFireSure;
	int mFireBase;                  // the board's fire tiles less the word's: the ones that stay
	int mFireWords;
	double mFireExpected;
	int mFireCame;
	int mFireMismatches;
	bool mFireTagShot;              // "-firetag": the hint's tag with new fire to expect, shot once
	bool mTestUndo;
	int mUndoPhase;                 // 0 a word to play, 1 played: undo it, 2 undone: check, replay, 3 check
	std::string mUndoBefore;        // BoardState before the word, after it
	std::string mUndoAfter;
	int mUndoWordsScored;           // mWordsScored when the word was planned
	int mUndoChecks;
	int mUndoMismatches;
	bool mCheckModel;
	bool mTypeWords;
	std::string mTyped;             // the letters typed so far (upper case)
	std::string mTypeCells;         // the cells' letters and tile types when the typing was planned
	int mTypeChecks;
	int mTypeMismatches;
	BotGame* mModel;                // the model's game, from mModelBoard's first board
	Board* mModelBoard;
	int mModelMoves;                // the moves it took (the game's mModMoves when it agrees)
	int mModelChecks;
	int mModelMismatches;
	bool mDone;
	int mQuitAt;                    // update of the quit once the last game is done, -1 before

	// screens
	bool mTitleClicked;
	int mGamesStarted;
	int mGamesOver;
	const void* mWaitKey;           // the screen or dialog waited on (Due)
	int mWaitUntil;
	const void* mNameDialog;        // the new-user dialog a name was typed into
	bool mSummaryNoted;             // the summary on screen was logged (and shot)
	int mSummaryNotedAt;            // the update it was
	bool mModsShot;                 // the game-over summary shot again with the mods' balloon
	int mJournalStep;               // the word journal at game over: 0 to open, 1 opening, 2 shot, 3 closing, 4 done
	int mJournalStepAt;             // the update of the last step
	bool mPreviewNoted;             // the selected word's definition was showing at the last update
	int mPreviewShots;              // its showings so far, +1 once the first is shot
	bool mPanelShot;                // the widescreen side panel was shot
	bool mFameNoted;
	int mLevelUps;

	// the board
	std::vector<Step> mPlan;
	size_t mPlanPos;
	int mPlanKind;
	int mNextStepUpdate;            // update at which mPlan[mPlanPos] is due
	int mWaitedSince;               // update at which the due step started waiting for the board, -1
	bool mDragging;
	int mDragX;
	int mDragY;
	int mDragCell;                  // the tile the drag is on
	std::string mPlanWord;          // the selection's letters
	int mPlanNum;
	int mResultUpdate;              // the update that ran the plan's submit, -1 for none
	int mScoreBefore;
	int mWordsScored;
	Board* mLastBoard;
	int mLastState;
	int mLastLevel;

	int Random(int theRange);
	void Note(const char* theFormat, ...);
	void Queue(const std::string& theCommand, const std::string& theArgs);
	void QueueAt(const std::string& theCommand, int theX, int theY);
	void Shot(const std::string& theEvent);
	void Quit();
	bool Due(const void* theKey, int theMinDelay, int theMaxDelay);
	bool ClickWidget(Widget* theWidget, const char* theWhat);
	const char* ModeName(int theMode);

	void LogChanges(Board* theBoard);
	void CheckResult(Board* theBoard);
	void CheckNewFire(Board* theBoard);
	void NoteFireTotals();
	void HandleDialogs();
	void HandleFame();
	void HandleSummary(Board* theBoard);
	void HandleSelector();
	void PlayBoard(Board* theBoard);

	bool BoardTakesInput(Board* theBoard);
	bool TileClickable(Board* theBoard, int theCell, int theX, int theY);
	void FindWords(Board* theBoard, std::vector<FoundWord>* theWords);
	void PlanNext(Board* theBoard);
	void PlanSelection(Board* theBoard, const std::vector<int>& thePath, int theMethod, bool clearAfter);
	Step& AddStep(int theKind, int theCell, Board* theBoard, int theDelay);
	std::string PathWord(Board* theBoard, const std::vector<int>& thePath);
	static std::string BoardState(Board* theBoard);
	void CheckUndo(Board* theBoard, const std::string& theExpected, const char* theWhat);
	bool PlanUndoTest(Board* theBoard);
	bool ModelTakesMove(Board* theBoard);
	void CheckModel(Board* theBoard);
	void CheckModelEnd(Board* theBoard);
	void PlanTyping(Board* theBoard, const std::string& theWord);
	void CheckTyped(Board* theBoard, bool isWord);
	void NoteTyped();
	int RunStep(Board* theBoard, const Step& theStep);
	void EndDrag(const char* theWhy);
	void DropPlan(const char* theWhy);
};

}

#endif
