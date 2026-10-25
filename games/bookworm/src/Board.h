#ifndef __BOOKWORM_BOARD_H__
#define __BOOKWORM_BOARD_H__

// Board.cpp: the 7x8 letter grid of BookWorm Deluxe - rules, per-tick update, input, scoring, fire tiles,
// levels and the saved game (players/<user>/classic.dat, action.dat). The drawing half (Draw and the
// Draw* helpers declared below) is in BoardDraw.cpp (board_draw).
// Original: class Sexy::Board : public Widget, public ButtonListener, sizeof 0x3f0 (BookWormApp::StartGame
// 0x410d10 does new(0x3f0)), vftables 0x4f9b7c (Widget, 47 slots) and 0x4f9b60 (ButtonListener at +0x58).
// Board data starts at +0x5c; offsets in the comments are absolute (the port's base classes have other
// sizes, nothing depends on them).
// Behaviour spec: re/bookworm/analysis/board_logic.md (logic) and board_draw.md (drawing); cross-module
// names: re/bookworm/analysis/NAMES.md.

#include "Widget.h"
#include "ButtonListener.h"
#include "Buffer.h"
#include "Color.h"
#include "KeyCodes.h"
#include "MTRand.h"
#include "Rect.h"
#include "Piece.h"
#include "WordScore.h"
#include "StatKeeper.h"
#include <cstdio>
#include <list>
#include <string>
#include <vector>

namespace Sexy
{

class BookWormApp;
class Graphics;
class Image;
class DialogButton;
class OverlayWidget;
class LevelUpWidget;
class Lex;
class EffectMgr;
class EmberListMgr;
class Points;
class WidgetManager;
class Solver;
class UndoStack;
class GreedyBots;
class TypedWord;
class WordReview;
class DailyBoard;

// Board::mState (+0x194)
enum BoardState
{
	BOARD_PLAYING = 0,
	BOARD_GAMEOVER_MUSIC = 1,       // one tick: high scores and the summary, then BOARD_GAMEOVER
	BOARD_GAMEOVER = 2,             // final
	BOARD_INACTIVE = 3,             // Update returns at once (Abandon, app StartGame with dialog 5 up)
	BOARD_GAMEOVER_PENDING = 4,     // a fire tile burned through the bottom; waits for Lex and the flying tiles
	BOARD_LEVEL_COMPLETE = 5,       // the level threshold was reached: the level-up announcement, then the summary
	BOARD_NEXT_LEVEL = 6,           // request from the summary's continue button: Update calls NextLevel
	BOARD_SCRAMBLING = 7            // the letters flip after a scramble
};

// Board::mSelectMode (+0x198)
enum SelectMode
{
	SELECT_NONE = 0,
	SELECT_CLICKED = 1,             // the selection was made by clicks: the next click on its end submits
	SELECT_DRAGGING = 2,
	SELECT_SUBMIT = 3               // a submit was requested (double click, click on the last tile)
};

// Board button ids (heard by Board::ButtonPress / ButtonDepress)
enum BoardButtonId
{
	BOARD_BUTTON_SUBMIT = 16,
	BOARD_BUTTON_OPTIONS = 17,
	BOARD_BUTTON_LEAVE = 18,
	BOARD_BUTTON_HINT = 19,         // port(mod): the solver's HINT, beside SUBMIT (Solver.h)
	BOARD_BUTTON_UNDO = 20          // port(mod): UNDO, beside OPTIONS (Undo.h)
};

// Board::mHintId (+0x34c), passed to BookWormApp::ShowHint; only 1, 2, 4 and 10 are ever scheduled
enum BoardHintId
{
	HINT_HOW_TO_PLAY = 0,
	HINT_WATCH_OUT = 1,
	HINT_GREEN_TILE = 2,
	HINT_GOLD_TILE = 3,
	HINT_SCRAMBLE = 4,
	HINT_SUBMIT_BUTTON = 5,
	HINT_SUBMIT_CLICK = 6,
	HINT_SUBMIT_DRAG = 7,
	HINT_SUBMIT_KEY = 8,
	HINT_BONUS_WORD = 9,
	HINT_FIRE_ALERT = 10
};

enum
{
	BOARD_COLS = 7,                 // odd columns hold rows 0..7, even columns rows 1..7 (slot [0][even] is NULL)
	BOARD_ROWS = 8,
	TILE_SIZE = 57
};

// Tables (rdata), board_logic.md 2.5
extern const int	gLevelThresholds[42];       // 0x4f97e0 cumulative score at which level n is complete
extern const int	gLetterValues[26];          // 0x4f9888 letter scores A..Z (also BoardDraw and the app's tile sets)
extern const int	gLengthBonus[9];            // 0x4f98f0 CalcScoreByLength (dead)
extern const int	gLetterWeights[26];         // 0x4f9918 letter frequencies A..Z (sum 95500)
extern const int	gFireChanceByLevel[13];     // 0x4f9980 percent, index min(level, 12)
extern const double	gFireMeterRefill[12];       // 0x4f99b8 index min(level, 11)
extern const int	gActionSpeed[10];           // 0x4f9a18 index min(level, 9) (written to a dead global)
extern const double	gFireMeterByLength[6];      // 0x4f9a40 index min(length - 3, 5)
extern const int	gBonusWordPoints[11];       // 0x4f9a70 index min(bonus words this game, 10)
extern const int	gBonusWordLength[10];       // 0x4f9a9c index min(bonus words given, 9)
extern int			gSubmitColorsInvalid[2][3]; // 0x52e280 submit label colours (normal, over), word not valid
extern int			gSubmitColorsValid[2][3];   // 0x52e298 submit label colours, valid word

class Board : public Widget, public ButtonListener
{
public:
	// +0x58 ButtonListener vfptr
	BookWormApp*		mApp;                   // +0x5c
	std::list<DialogButton*> mButtons;          // +0x60 the submit, options and leave buttons
	MTRand*				mRand;                  // +0x6c the board's RNG: drop heights, Lex chatter, particles
	EffectMgr*			mEffectMgr;             // +0x70 the board's effects (sparkles)
	EmberListMgr*		mEmberMgr;              // +0x74 particles (pool of 1000)
	Piece*				mBoard[BOARD_ROWS][BOARD_COLS]; // +0x78 [row][col]; top tile of column c: [c % 2 == 0 ? 1 : 0][c]
	OverlayWidget*		mOverlay;               // +0x158 bonus-word banner, flying tiles, Lex's balloon (AddedToManager)
	LevelUpWidget*		mLevelUpWidget;         // +0x15c level / game summary or NULL
	Buffer				mGameLog;               // +0x160 bit-packed event log (board_logic.md 7)
	unsigned long		mStartTick;             // +0x180 GetTickCount() in the ctor; never read
	unsigned long		mLevelStartTick;        // +0x184 when the level clock last (re)started
	int					mLevelSeconds;          // +0x188 seconds of the level (ShowSummary)
	int					mTotalSeconds;          // +0x18c sum of mLevelSeconds
	int					mLevelSecondsBanked;    // +0x190 seconds banked by pauses
	int					mState;                 // +0x194 BoardState
	int					mSelectMode;            // +0x198 SelectMode
	PieceList			mSelection;             // +0x19c selected tiles in order
	double				mFireMeter;             // +0x1a8 classic "fire debt": a fire tile is due at <= 0 (ctor 5.25)
	bool				mSkipRandomFire;        // +0x1b0 the next chance fire tile is skipped (ctor true)
	bool				mBurnedDown;            // +0x1b1 the board burned at game over
	unsigned char		mUnused1b2;             // +0x1b2 bool never initialised in the original (port: 0), saved only:
	                                            // a byte, so an original save's garbage value round-trips
	bool				mPiecesFlying;          // +0x1b3 cleared per Update, set by FlyingPieceList::Update
	bool				mFlameSoundPlayed;      // +0x1b4 flametile played once per refill burst
	// +0x1b5..+0x1b7 padding
	int					mRewardStreak;          // +0x1b8 consecutive words that earned a special tile
	// +0x1bc padding
	double				mScrambleProgress;      // +0x1c0 0..1 in BOARD_SCRAMBLING
	double				mLevelProgress;         // +0x1c8 target fraction of the level bar
	double				mShownProgress;         // +0x1d0 animated bar value
	WordScore			mWord;                  // +0x1d8 the current selection's word
	WordScore			mLastWord;              // +0x220 the word drawn above the board
	Color				mLastWordColor;         // +0x268 written by DrawWord
	double				mLastWordAlpha;         // +0x278 1.0 while selecting, fades after
	double				mScoreGlow;             // +0x280 score glow 0..0.85
	int					mBonusWordsGiven;       // +0x288 bonus words handed out (gBonusWordLength index)
	SexyString			mBonusWord;             // +0x28c word on the banner, "" when none
	StatKeeper*			mGameStats;             // +0x2a8 whole game
	StatKeeper*			mLevelStats;            // +0x2ac current level
	SexyString			mUnusedStr2b0;          // +0x2b0 saved only
	int					mUnused2cc;             // +0x2cc saved only
	SexyString			mUnusedStr2d0;          // +0x2d0 saved only
	int					mUnused2ec;             // +0x2ec saved only
	int					mUpSoundIdx;            // +0x2f0 rising "up" notes played so far
	int					mUpSoundCount;          // +0x2f4 notes to play (word length)
	int					mUpSoundDelay;          // +0x2f8 ticks to the next note
	Piece*				mGameOverPiece;         // +0x2fc fire tile that burned through the bottom
	int					mDeadSparkDelay;        // +0x300 never set > 0 (dead)
	int					mBoardBurnDelay;        // +0x304 ticks until the whole board burns
	double				mBoardBurnAlpha;        // +0x308 1.0 when the board burns, -0.01 per tick
	int					mBlinkDelay;            // +0x310 ticks to the next blink
	int					mBlinkRepeats;          // +0x314 quick re-blinks left
	int					mGameOverMusicDelay;    // +0x318 ticks until the game-over jingle
	int					mFireLineDelay;         // +0x31c ticks until the screen-burn sound
	Piece*				mFireLinePiece;         // +0x320 bottom fire tile under the fire column
	// +0x324 padding
	double				mFireLineAlpha;         // +0x328 0..1
	int					mFireLineScroll;        // +0x330 scroll into IMAGE_FX_FIRELINE
	int					mFireTileCount;         // +0x334 fire tiles on the board
	int					mActionFireTimer;       // +0x338 action: ticks to the next timed fire tile (ctor 1000)
	int					mActionCountdown;       // +0x33c action: seconds left while a fire tile is at the bottom
	int					mWarnBeeps;             // +0x340 warning beeps left
	int					mWarnBeepDelay;         // +0x344
	int					mHintDelay;             // +0x348 ticks until hint mHintId shows
	int					mHintId;                // +0x34c BoardHintId
	Lex*				mLex;                   // +0x350 created in AddedToManager
	bool				mLexGreetPending;       // +0x354 greet when Lex stops walking (ctor true)
	bool				mLexDangerTalkReady;    // +0x355
	bool				mLevelUpPending;        // +0x356 the level up (GT_LEVEL_UP) still to announce
	bool				mNewHighScore;          // +0x357 written by the app and LevelUpWidget
	bool				mLexPraisePending;      // +0x358 Lex comments a special tile
	// +0x359..+0x35b padding
	int					mLexPraiseType;         // +0x35c TileType of the comment
	int					mBlinkFrames;           // +0x360 blink in progress
	bool				mBurpPending;           // +0x364 the word had >= 2 fire tiles
	bool				mBurping;               // +0x365 burp smoke visible
	// +0x366..+0x367 padding
	int					mBurpDelay;             // +0x368 set to 50 by FlyingPieceList when a fire tile reaches Lex
	// +0x36c padding
	double				mBurpAlpha;             // +0x370 1.0, -0.005 per tick
	int					mHighlightKey;          // +0x378 letter key held: tiles with that letter drawn lit
	bool				mCtrlDown;              // +0x37c
	// +0x37d..+0x37f padding
	DialogButton*		mSubmitButton;          // +0x380
	int					mSubmitBlinkWords;      // +0x384 submit blinks while > 0; -1 per valid word (ctor 3)
	bool				mPlayCounted;           // +0x388 times-played counted for this board
	// +0x389..+0x38b padding
	int					mLastLandSoundTick;     // +0x38c mUpdateCnt of the last piece_hit
	int					mCols;                  // +0x390 7
	int					mRows;                  // +0x394 8
	int					mPointsAwarded;         // +0x398 sum of the score popups
	int					mShownScore;            // +0x39c rolling score display
	double				mScoreScale;            // +0x3a0 always 1.0
	std::list<Points*>	mPopups;                // +0x3a8 floating texts
	Points*				mLastMessage;           // +0x3b4 last ShowMessage popup (stacking)
	int					mMouseX;                // +0x3b8 never read
	int					mMouseY;                // +0x3bc never read
	bool				mSummaryShown;          // +0x3c0 the LevelUpWidget is up
	bool				mPiecesMoving;          // +0x3c1 a non-shaking tile fell this tick
	// +0x3c2..+0x3c3 padding
	int					mLevel;                 // +0x3c4 0-based
	int					mPauseCount;            // +0x3c8 > 0 paused
	// +0x3cc padding
	double				mPauseFade;             // +0x3d0 0..1
	int					mUnused3d8;             // +0x3d8 saved only
	bool				mPauseNoDialogs;        // +0x3dc while paused: no dialog and no help widget
	// +0x3dd..+0x3df padding
	int					mBestWordThisLevel;     // +0x3e0 highest popup value of the level
	int					mBestWordThisGame;      // +0x3e4
	int					mUnused3e8;             // +0x3e8 zeroed by LevelUp, saved only
	int					mUnused3ec;             // +0x3ec saved only

	// port(mod): the word solver (mod "solver"); NULL while no enabled mod turns it on. Then SUBMIT takes the
	// left part of its plank and HINT the right part.
	Solver*				mSolver;
	DialogButton*		mHintButton;
	// port(mod): undo (mod "undo"); NULL without it. Then OPTIONS takes the left part of its plank and UNDO
	// the right part.
	UndoStack*			mUndo;
	DialogButton*		mUndoButton;
	// port(mod): a mod's words with effects (ModWordEffects, mod "rude-words"): the fire tiles the next
	// Refill sets among its new tiles (set by Submit, 0 otherwise), Lex's danger clip once he has eaten the
	// word, and the words of the game the stats count (kept at the end of the saved game while such a mod is
	// on; the player's all-time total is in the registry, ModWordStatsValueName)
	int					mModFireDue;
	bool				mModDangerPending;
	int					mModWordsPlayed;
	// port(mod): the greedy bots (mod "greedy-bots", GreedyBots.h): two players' games from this game's start,
	// a few moves each tick; NULL without the mod, in Action, and for a game begun without them.
	// mBotsChecked: the start was looked at (the bots begin only at a fresh game's)
	GreedyBots*			mBots;
	// port(mod): typing a word to play it (mod "type-word", TypedWord.h); NULL without it
	TypedWord*			mTypedWord;
	// port(mod): the words played against the board's best (mods "missed-words", "par"; WordReview.h); NULL
	// without them
	WordReview*			mWordReview;
	// port(mod): the day's board (mod "daily-board", DailyBoard.h); NULL without it
	DailyBoard*			mDaily;
	bool				mTileSymbols;           // port(mod): colour-blind mode's tile symbols (TileSymbols.h)
	bool				mBotsChecked;
	// port(mod): the moves (words and scrambles) so far and the last word's cells (row * BOARD_COLS + col, in
	// order; empty for a scramble), for the -autoplay bot's check of the bots' model (-autoplaymodel)
	int					mModMoves;
	std::vector<int>	mModLastMove;

public:
	Board(BookWormApp* theApp);                                         // 0x40ae90
	virtual ~Board();                                                   // 0x4062e0 body, 0x406c10 slot 0

	// Widget
	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;    // 0x403730 slot 1
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override; // 0x405b00 slot 2
	virtual void		OrderInManagerChanged() override;               // 0x405240 slot 3
	virtual void		SetVisible(bool isVisible) override;            // 0x405a50 slot 4
	virtual void		SetDisabled(bool isDisabled) override;          // 0x405a20 slot 10
	virtual void		Draw(Graphics* g) override;                     // 0x40a980 slot 16 (BoardDraw.cpp)
	virtual void		Update() override;                              // 0x40bd40 slot 17
	virtual void		KeyChar(SexyChar theChar) override;             // 0x405d60 slot 20
	virtual void		KeyDown(KeyCode theKey) override;               // 0x403df0 slot 21
	virtual void		KeyUp(KeyCode theKey) override;                 // 0x403e40 slot 22
	virtual void		MouseLeave() override;                          // 0x403a70 slot 24
	virtual void		MouseMove(int x, int y) override;               // 0x403a80 slot 25
	virtual void		MouseDown(int x, int y, int theClickCount) override;     // 0x409de0 slot 27
	virtual void		MouseUp(int x, int y, int theClickCount) override;       // 0x40fc40 slot 29
	virtual void		MouseDrag(int x, int y) override;               // 0x40a0c0 slot 31
	using Widget::MouseDown;
	using Widget::MouseUp;

	// ButtonListener (+0x58)
	virtual void		ButtonPress(int theId, int theClickCount) override;      // 0x4081d0 listener slot 2
	virtual void		ButtonDepress(int theId) override;              // 0x40fdc0 listener slot 4
	using ButtonListener::ButtonPress;

	// geometry
	static bool			IsAdjacent(Piece* thePiece1, Piece* thePiece2); // 0x402f60 (cdecl)
	int					ColX(int theCol);                               // 0x403990
	int					RowY(int theCol, int theRow);                   // 0x4039b0
	Piece*				PieceAt(int x, int y);                          // 0x4039e0
	Piece*				PieceBelow(Piece* thePiece);                    // 0x403bd0

	// random letters, levels
	static int			RandomLetter();                                 // 0x403d10 (gGameRand)
	static int			LevelThreshold(unsigned int theLevel);          // 0x403880 (cdecl)
	static void			SetActionSpeed(double theSpeed);                // 0x402fd0 (writes a dead global)

	// board contents
	void				NewBoard();                                     // 0x404150
	void				Refill();                                       // 0x40b540
	void				SetModFire(const std::vector<Piece*>& theNewTiles, int theCount); // port(mod)
	void				RemoveBonusTiles();                             // port(mod)
	int					GetModWordsAllTime();                           // port(mod)
	void				AddModWordsAllTime(int theDelta);               // port(mod)
	void				UpdateBots();                                   // port(mod)
	void				ReadBots(FILE* theFile);                        // port(mod)
	void				DeleteAllPieces();                              // 0x4036e0
	void				RemovePieces(PieceList* thePieces);             // 0x4052d0
	void				GetNormalPieces(std::vector<Piece*>* thePieces); // 0x40b9e0
	void				CheckBottomFire();                              // 0x4042c0
	void				BurnTiles(bool advance);                        // 0x408700
	void				Scramble();                                     // 0x40ed70 (Lex::MouseUp)
	void				AwardSpecialTile(const WordScore& theWord);     // 0x40ba80

	// selection and words
	void				AddLetter(Piece* thePiece);                     // 0x409c60
	void				TruncateAfter(Piece* thePiece);                 // 0x405cf0
	// port(mod): selects thePath as clicks on its tiles would (the solver's hints)
	void				SelectTiles(const std::vector<Piece*>& thePath);
	void				ClearWord();                                    // 0x407bf0
	void				Submit(int x, int y);                           // 0x40f5b0
	void				BonusWordScored(int thePoints, int x, int y);   // 0x40abd0
	void				PickBonusWord();                                // 0x408030 (also the app's StartGame)
	void				AddScorePopup(int theScore, int x, int y);      // 0x409970
	void				ShowMessage(const SexyString& theText, int theY); // 0x409b70
	void				ClearPopups();                                  // 0x405a80

	// buttons, overlay, pause, levels
	DialogButton*		MakeButton(int theId, const Rect& theRect, Image* theButtonImage, Image* theOverImage,
							Image* theDownImage, const SexyString& theLabel); // 0x40ad00
	void				RemoveButtons();                                // 0x405810
	void				SetButtonsVisible(bool isVisible);              // 0x405850
	void				SetButtonsDisabled(bool isDisabled);            // 0x405890
	void				DeleteButtons();                                // 0x405de0
	void				ShowOverlay(bool show);                         // 0x403d80
	void				Pause(bool pause);                              // 0x405be0 (counted)
	void				UpdatePauseFade();                              // 0x403ae0
	void				ShowSummary();                                  // 0x404b40
	void				RemoveLevelUpWidget();                          // 0x403ea0
	void				LevelUp();                                      // 0x407a50
	void				NextLevel();                                    // 0x408000
	void				StartGameOverMusic();                           // 0x4038c0
	void				Abandon();                                      // 0x405cd0
	bool				IsInProgress();                                 // 0x403720

	// saved games (players/<user>/classic.dat, action.dat)
	bool				SaveGame(int theMode);                          // 0x406c40
	bool				LoadGame(int theMode);                          // 0x408ab0
	// port: LoadGame's reading of the record; false for a version before 2. The undo mod's restore reads
	// a record made by SaveGameSnapshot.
	bool				ReadSavedGame(FILE* theFile, bool* isHelpOpen);
	bool				ReadSavedGame(const Buffer& theRecord);
	// port(mod): the program-wide state the record leaves out, for the undo mod: RandomLetter's previous
	// letter (never drawn twice running; each rejected letter costs a gGameRand draw) and Refill's
	// previous level
	static void			GetGlobalState(int* theLastLetterIdx, int* theLastRefillLevel);
	static void			SetGlobalState(int theLastLetterIdx, int theLastRefillLevel);
	bool				DeleteSavedGame(int theMode);                   // 0x4078b0
	void				WriteSavedGame(FILE* theFile, int theMode, bool isHelpOpen); // 0x406c40 (the record, SaveGame's second half)
	// port (BookWormApp::GetShutdownSaves, web persistence): the bytes SaveGame(theMode) would write now,
	// without changing the board
	bool				SaveGameSnapshot(int theMode, Buffer* theBuffer);

	// drawing (BoardDraw.cpp, board_draw)
	void				DrawPieces(Graphics* g);                        // 0x40a160
	void				DrawPiece(Graphics* g, Piece* thePiece);        // 0x4044f0
	void				DrawPieceBlank(Graphics* g, Piece* thePiece);   // 0x404380 (action mode, paused)
	void				DrawSelection(Graphics* g);                     // 0x405340
	void				DrawLevelText(Graphics* g);                     // 0x4056e0
	void				DrawProgressBar(Graphics* g);                   // 0x403c00
	void				DrawScore(Graphics* g);                         // 0x405580
	void				DrawWord(Graphics* g);                          // 0x407c60 (writes mLastWordColor)
	// port(mod): a groove across a left-panel plank at theX, where a mod's button splits it
	void				DrawPlankDivider(Graphics* g, int theX, int theY);
};

}

#endif
