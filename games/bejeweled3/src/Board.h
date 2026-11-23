// Sexy::Board (vtable 0x864cb4 (bejeweled3), made by 0x74ae30) and Sexy::ClassicBoard (RTTI vftable 0x86511c, 0x3088
// bytes, made by 0x73b390): the game screen of Classic mode, and Sexy::Piece (0x630 bytes, made by 0x7415c0): one gem.
// The board is an 8 by 8 grid of pieces; the exe measures it in 1200ths of the art height (a cell is 128), draws it at
// (0x20b, 0x4b) of that and scales every measure by the app's art size over 1200.
// port: only what Classic's start, first fill and the fall of a new board read is translated (docs/NOTES.md, "The board"):
// the swap, the matches, the rest of the update (0x759170) and the other modes' boards come with their steps. The exe's board has about 200 virtual
// functions that the modes override; Classic's overrides are constants (0x73b5a0 and 0x73bc00 to 0x73bc30), which this
// port reads in place (each use says which slot it stands for).
#pragma once

#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "DialogListener.h"
#include "CurvedVal.h"
#include "Effects.h"
#include "MTRand.h"
#include "Widget.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Sexy {

class Board;
class Background;
class PIEffect;
class ClassicEndLevelDialog;

// One gem. Fields are named by what the code that reads them does with them; the ones that nothing translated reads
// are left out (the fill also sets +0x182 to 1 and +0x244 to 0 on each piece it makes: nothing translated reads them).
class Piece {
public:
	enum { NO_COLOR = -1, COLOR_COUNT = 7 };
	// +0x228: what the gem is. Named by what the exe does with the bit (0x745ca0, 0x749570, 0x7494d0 set them): a 4-run makes
	// FLAG_FLAME, a run of 5 FLAG_HYPERCUBE, a crossing FLAG_STAR, a run of 6 or more both FLAG_STAR and FLAG_FLAME. The other bits
	// (0x10, 0x80, 0x400, 0x800, 0x1000, 0x2000, 0x8000, 0x20000, 0x80000) belong to other modes: nothing in Classic sets them
	enum {
		FLAG_FLAME = 1,        // goes off as a 3 by 3 explosion (Explode, 0x73f8e0), which sets neighbours with this bit off in turn
		FLAG_HYPERCUBE = 2,    // swapped with a neighbour it clears every gem of that one's colour (0x740f20, 0x74db70, 0x7870f0)
		FLAG_STAR = 4,        // when matched it makes a match group whose rings run along its row and column (0x789a90, 0x75dee0)
		FLAG_BUTTERFLY = 0x80,         // Butterflies' gem (0x7492c0 sets it): matched it is freed; it climbs a row when its mTimeGemSeconds (+0x244) counts out
		FLAG_RANDOM_PICK = 0x60,
		FLAG_TIME_GEM = 0x20000,        // Lightning's time gem (0x716c60 makes it, 0x715360 collects it): mTimeGemSeconds is its worth
		FLAG_TIME_GEM_AGING = 0x40000,  // set on a gem 0x7173f0 counts down (+0x244) every 150 game ticks and clears with the time gem's bits when it is out; nothing translated sets it
		FLAGS_SPECIAL_MADE = 0x2e0   // the bits 0x745ca0, 0x749570 and 0x7494d0 clear from the flags (the mask 0xfffffd1f)
	};

	Board* mBoard;          // +0
	int mId;                // +4: the board's piece counter (+0xf4) when it was made
	int mCol;               // +8
	int mRow;               // +0xc
	float mX;               // +0x10: where it is drawn, in 1200ths of the art height from the board's corner (a cell is 128)
	float mY;               // +0x14: new pieces start above the board and fall to row * 128
	int mCreatedUpdate;     // +0x18: the board's update count when it was made
	float mFallSpeed;       // +0x1c: how far it moves down each update, in 1200ths: 0 at rest and until it first falls; the fill
	                        // gives a new piece a negative start (it rises a little first, which staggers a column), gravity adds
	int mColumnTag;         // +0x188: what the column's tag (board +0x218) held when it was made
	CurvedVal mScaleCurve;  // +0x20: the piece's size, the constant 1 from the constructor (0x720120); 0x75af60 starts a pulse on it when
	                        // the piece is matched, UpdatePiecePulses a shrink after it and then the piece's removal
	bool mCanFall;          // +0x17e: set by every pass of the fill over it
	bool mCanSwap;          // +0x17f: 1 at the start
	CurvedVal mSlideCurve;  // +0x198: the piece's slide towards mSlideCol and mSpecialState (the cluster of a special that is made: 0x75cd40 to 0x75d851;
	                        // idle at the start); while it is not at 0 the piece is drawn that far along (0x769189) and is not removed
	int mSlideCol = 0;          // +0x208: the column a slide goes to
	int mSpecialState;      // +0x20c: the row a slide goes to; -1 keeps it from falling
	int mMovedUpdate;       // +0x218: the board's update count when it was made and when it last moved (set while its speed is not 0)
	int mColor;             // +0x220: -1 for none, else 0 to 6 (red, white, green, yellow, purple, orange, blue)
	int mFlags;             // +0x228: 0 for a plain gem
	int mBusyCount;         // +0x230: above 0 while something is done to it: the fill waits for it; a piece whose count
	                        // reaches 0 from 1 is destroyed (UpdateGame)
	bool mSelected = false; // +0x23c: the player has picked it up (a click on it, waiting for the next gem)
	bool mIsGroupOrigin = false;   // +0x174: a match group was made for it (0x789a90), or a hypercube's group has picked it (0x7870f0)
	bool mDestroyed = false;    // +0x17d: set by 0x74c770 (DestroyPiece) just before the piece leaves the grid and by 0x73f8e0 (Explode) for a piece the explosion takes: it was taken by a special's
	                        // destruction and not by a plain match's shrink (a plain gem's match goes through UpdatePiecePulses, which never sets it). Wall Blast and Gold Lines read it
	float mMatchTimer = 0.0f;   // +0x178: how far its clear has come. A match group (0x789a90) puts its first piece at 0.9 and a piece
	                        // that its ring reaches at 0.01; 0x75dee0 adds 0.015 each update and lets the piece go at 1. A plain
	                        // gem's match makes no group, so a plain gem never uses it (its scale curve, mScaleCurve, clears it)
	bool mClearHeld = false;    // +0x17c: set while another clear holds the piece (a flame gem found by 0x74c570); a match group
	                        // skips it and its ring does not reach it
	bool mClearable = true;     // +0x182: 1 from the constructor (0x720120): a match group's timer only runs for such pieces
	float mRingMark = 0.0f;     // +0x264: the highest timer (up to 1) a ring of a match group has seen on the piece (drawn only)
	int mDestroyerId = -1;      // +0x234: the id of the piece that cleared it (0x75dee0, 0x74c570); nothing translated reads it
	int mDestroyerFlags = 0;    // +0x238: the flags of what cleared it; nothing translated reads it
	float mSpinPhase = 0.0f;    // +0x170: how far through its turn (0 to 1) the gem is while it is picked up or already turning; 0.02 more each
	                        // update (UpdatePieceSpins) and 0 again at 1; the picture is cel (20 x the phase) of the gem's sheet (0x757df0)
	int mSwapUpdate = -1;   // +0x214: the update it was last swapped in
	int mMatchedUpdate = -1;   // +0x210: the update it was matched in
	int mGroupId = -1;      // +0x190: the id of the run that matched it (the board's counter, +0xe1c)
	int mRememberedColor = -1;   // +0x21c: its colour when it was matched (a hypercube has none, +0x220 is -1, so a run that holds it still
	                        // counts it for the colour it had); the matcher reads it for a piece matched in this update
	int mTimeGemSeconds = 0;     // +0x244: the seconds a Lightning time gem is worth (5 or 10); 0 for a plain gem (the fill sets it to 0 on every piece it makes);
	                        // Butterflies' gem: the moves left before it climbs a row (0x7492c0's maker, 0x6d7c70 and 0x6e82a0 set it, 0x73c710 counts it down)
	float mMoveCountdownPerLevel = 0.0f;   // +0x254: a butterfly gem's copy of the board's MoveCountdownPerLevel (0x6d7c70), which 0x6e82a0 reads again
	int mSpecialCountdown = 0;   // +0x184: 25 when a special was made on it (0x745ca0, 0x749570, 0x7494d0), counts down by one each update
	                        // (UpdateGame); the 3 by 3 explosion and the busy-count branch leave a piece alone while it is above 0

	// Time Bomb's bombs (flags 0x200 with 0x20 for a move bomb or 0x40 for a real-time one; the counter is mTimeGemSeconds): the shake that counts a move down (set by
	// 0x73c710, stepped by 0x767e20) and the real-time clock (0x767e20 steps it every update; a tick comes when it comes round to 0)
	float mShakePhase = 0.0f;   // +0x26c
	float mShakeSpeed = 0.0f;   // +0x270: 0 when no shake runs
	int mTickCounter = 0;       // +0x24c
	int mTickPeriod = 100;      // +0x250

	std::vector<Effect*> mEffects;   // +0x620: the effects that hold the piece (their users count, +0x1b4, counts it)

	explicit Piece(Board* theBoard);   // 0x7203b0 (bejeweled3), with 0x720120 (bejeweled3)
	~Piece();                          // 0x720040 (bejeweled3): the effects let go (0x71ffd0)
};

// The exe's counters (0x73d170): the board keeps one set for the game (+0xc84, 38 ints) and each move record one for the move
// (+0x40, the same 38); the profile's statistics (Profile::mStats, +0xc0) are the same list. port: only the counters that
// the translated code adds to are written (COUNTER_POINTS and COUNTER_MATCHES); the other callers of 0x73d170 (the match
// search, the special gems, the swaps: 0x75bb00, 0x75c941 to 0x75cc4f, 0x75d2c8, 0x75dbf4, 0x759fcf, 0x74c6b5, 0x74dbb5 and
// others) are left out
enum { COUNTER_COUNT = 38, COUNTER_SECONDS = 0, COUNTER_POINTS = 1, COUNTER_ACTIVITY = 15, COUNTER_MATCHES = 0x1a };

// The exe's 0xd8-byte record of one move (made by 0x744660, kept in the list at +0xc48): the id its pieces carry as their
// tag (+0x188) and the counters of what the move has made (counter COUNTER_MATCHES is the number of matches the move and
// its cascades have made: 0x75af60 adds one for each match and reads it for the score; COUNTER_POINTS is the points it has
// made, which the level bar's gain reads: 0x74a210)
struct MoveRecord {
	int mId;                            // +0x3c
	int mCounters[COUNTER_COUNT];       // +0x40
};

// A move of two neighbouring gems, as the move search (0x75a8e0) gives it: the cell of the gem to move and the cell it goes to
struct BoardMove {
	int mFromCol;    // out[0]
	int mFromRow;    // out[1]
	int mToCol;      // out[2]
	int mToRow;      // out[3]
};

// The level change's overlay (HyperspaceFallback, vtable 0x86616c, 0x1b8 bytes, made by 0x723320 and held at the board's
// +0x2890; the 3D one is HyperspaceUltra, 0x726c40). Only what its update (0x725ef0) does to the board is kept; it is drawn
// by 0x724110, which is left out
struct Hyperspace {
	int mAge = 0;              // +0x38: its update count (WidgetContainer::Update, before anything else)
	CurvedVal mCurve;          // +0xc8: 0 to 1 at 0.0025 an update; when it runs out the level change ends
};

// One swap of two neighbours (the exe's 0x100-byte record in the list at +0xc30, made by 0x75ee20, run by 0x759ba0)
struct SwapRecord {
	Piece* mFirst;        // +0: the piece the player moved
	Piece* mSecond;       // +4
	int mDeltaCol;        // +8: where the first one goes, from its own column
	int mDeltaRow;        // +0xc
	CurvedVal mCurve;     // the swap's curve
	bool mIsSwapBack;     // the swap made no match and the pieces are going back
	bool mDone;           // +0xf8 (set while it is checked for matches: the pieces are no longer held)
	bool mForced = false; // +0xf9: the 5th argument of slot 195 (0x75ee20): a swap that stays whether or not it makes a match and is not a move of the player
	                      // (Butterflies' climb gives 1: 0x759ba0 does not send it back, 0x74c070 and 0x6d7f40 do not count it as a move)
};

// One cell of a match group's ring (the exe's 12-byte entry in the group's list at +0xa4, pushed by 0x75dee0)
struct RingCell {
	int mCol;         // +0
	int mRow;         // +4
	float mTimer;     // +8: 0.01 when pushed, 0.015 more each update; at 1 the entry goes
};

// A match group (the exe's effect of 0x2e8 bytes made by 0x789a90 for a gem with bit 4, a flame gem, or by 0x74c570, kept in
// the list at +0x125c): rings that go out from its first piece and hit the cells around it, and the pieces that they find.
// port: its child effects (the streaks of 0x789860, which draw the app's Rand() when they run), its curves and its sounds
// are not translated; the fields that only they and the hypercube's update (0x7870f0, type 7) read are left out
struct MatchGroup {
	int mPixelX;               // +4: where the piece is, in 1200ths from the grid's corner, at its middle
	int mPixelY;               // +8
	int mAge;                  // +0xc: how many updates 0x78a390 has run for it
	int mMaxDistance;          // +0x14: how far a ring goes: 3 for type 3, else 7
	int mFlags;                // +0x1c: the first piece's flags (+0x228), which the pieces it clears take over
	int mType;                 // +0x28: 2 or 3 (a match; 3, or 2 when the board's +0xe8 is set), 6 when the piece has bit 1, 7 a hypercube's
	int mColor;                // +0x10: for type 7, the colour it clears (-1: every colour)
	std::vector<float> mBoltLives;   // +0x74: the bolts of lightning (0x786be0, 0x8c bytes each; 0 at the start); only each one's life (+0x80) is kept
	float mLife;               // +0x30: 1, and 0.25 less for each ring after the last distance has been reached; at 0 the group ends
	int mOriginId;             // +0x34: the id of the first piece
	int mTag;                  // +0x38: the first piece's tag (+0x188), the lowest tag its pieces take
	int mRunId;                // +0x3c: the first piece's run id (+0x190)
	int mRing;                 // +0x48: the rings made, which is also the distance of the next one from the first piece
	float mRingTimer;          // +0x4c: counts down by 0.01 each update, a ring at 0 or below
	float mFade;               // +0x2e0: 1, 0.01 less each update down to 0 (drawn only)
	int mLifeCountdown;        // +0x2e4: updates left before it ends; 0 is off (nothing translated sets it)
	std::vector<int> mPieceIds;    // +0x8c: the first piece's id, then the ids of the pieces its rings found
	std::vector<RingCell> mRingCells;    // +0xa4
};

// A swap the player asked for (the exe's 0x14-byte entry in the list at +0xc60, run by UpdateTick when its update comes)
struct PendingSwap {
	int mUpdate;    // +0
	int mPieceId;   // +4
	int mRow;       // +8
	int mCol;       // +0xc
	bool mForced = false;   // +0x10: TrySwap's fourth argument (slot 194), which UpdateTick gives StartSwap as its fifth
};

class Board : public Widget, public ButtonListener, public DialogListener {
public:
	enum { COLS = 8, ROWS = 8, CELL = 128 };
	// 0x73bd80 and 0x73bd70 (bejeweled3): where the grid is, in 1200ths of the art height
	enum { GRID_X = 0x20b, GRID_Y = 0x4b };

	Piece* mGrid[ROWS][COLS];        // +0xf8: row after row; null where there is no gem
	int mPieceCounter = 0;           // +0xf4: the number of pieces made, each one's id
	float mColumnSpeed[COLS];        // +0x1f8: the speed the fill takes from a column's last piece (the grid's ninth row); each update
	                                 // it rises by 0.275 towards 0 (UpdateGame)
	int mColumnTag[COLS];            // +0x218: -1 after a fill (the grid's tenth row)
	MTRand mRand;                    // +0x258: the board's own random numbers, seeded from the app's Rand() when it is made
	int mLastRow = ROWS - 1;         // +0x22e4: the last row
	int mFillHeld = 0;               // +0x2ef8: while it is not 0 the fill does nothing
	int mBackgroundIndex = -1;       // +0x2864: which of the game's backgrounds, in the order of 0x8d2170
	Background* mBackground = nullptr;   // +0x2868: the animated picture behind the screen, a widget of the app's manager (z-order -1)
	bool mLeaving = false;           // +0x2978: the Main Menu button was pressed: the game stands still while the screen fades, then goes
	int mLevelIndex = 0;             // +0xe04: the level, from 0 (0 after a reset; the score's multiplier and the hint read it)
	int mLevelNumber = 1;            // +0xe0c: 1 after a reset (0x74a880); the points a match is worth are multiplied by it
	std::vector<int> mColorList;     // +0x105c: the colours a new piece may get: each of the seven twice
	bool mNoSavedGame = true;        // port: the saved game of this mode (0x74e1f0) is not read: a new game starts
	int mScore = 0;                  // +0xd64: the score as shown, which follows mScorePoints (UpdateShownScore)
	int mScorePoints = 0;            // +0xd24: the score
	int mMatchCounter = 0;           // +0xe1c: the next id of a move or a match
	std::vector<MoveRecord> mMoveRecords;    // +0xc48
	std::vector<SwapRecord> mSwaps;          // +0xc30
	std::vector<std::unique_ptr<MatchGroup>> mMatchGroups;    // +0x125c (the exe's list holds pointers too)
	int mGroupTypeSwitch = 0;        // +0xe8: not 0 makes the groups of matches type 2 instead of 3 (nothing translated sets it)
	float mCheckerFlash = 0.0f;      // +0x1188: 0 after a reset (0x74aa8d); 0x74a590 (run by 0x74c291 and 0x762986, in the special gems' going off: not
	                                 // translated, so it stays 0) makes it 1; 0.00125 less each update while no match group lives (0x769e01):
	                                 // the checkered squares of the panel are lit by it (0x751d50)
	EffectsManager* mEffectsUnder = nullptr;   // +0x12bc: the effects under the gems (drawn in the gem passes, 0x7568a0; updated by 0x767090)
	EffectsManager* mEffectsOver = nullptr;    // +0x12c0: the effects over them (a child of the board)
	PIEffect* mLevelBarEffect = nullptr;   // a copy of PIEFFECT_LEVELBAR (the exe draws the resource itself, 0x7540c0), drawn over the bar's back
	ButtonWidget* mHintButton = nullptr;   // +0x286c: made by the layout 0x741840 (id 0, IMAGE_INGAMEUI_HINT_BUTTON_CLASSIC)
	ButtonWidget* mMenuButton = nullptr;   // +0x2870: (id 1, IMAGE_INGAMEUI_MENU_BUTTON_CLASSIC)
	ButtonWidget* mResetButton = nullptr;  // +0x2878: (id 2, IMAGE_INGAMEUI_RESET_BUTTON_LIGHTNING) in the Lightning modes (0x745240 makes it, 0x741840 places it)
	float mGroupGlow = 0.0f;         // +0x27b8: 0.02 less each update, 0.05 more while a group lives, up to 1 (drawn only)
	float mRingFlash = 0.0f;         // +0x1240: 1 when a group has made its last ring (drawn only)
	std::vector<PendingSwap> mPendingSwaps;  // +0xc60
	bool mMouseDown = false;         // +0x27d4: the button is held down since the last MouseDown
	int mMouseDownX = 0;             // +0x27d8: where it went down, in pixels
	int mMouseDownY = 0;             // +0x27dc
	Piece* mSelectedAtMouseUp = nullptr;   // +0x27e0: the piece that was picked up when the button last came up
	float mTickFraction = 0.0f;      // +0xf0: what is left of the update's steps (Update runs the whole steps, keeps the rest)
	int mGameTicks = 0;              // +0xe38: the game's own ticks, which stop while a dialog or a hold stops the game
	int mLandTicks = 0;              // +0xe40: how many more updates the fill thinks the new pieces need to land (counts down)
	int mGravityHold = 0;            // +0xe24: while above 0 the pieces stand still (counts down)
	int mLastLandSoundUpdate = 0;    // +0xc20: the update a piece's landing was last heard in
	bool mSettledOnce = false;       // +0x1190: set the first time the board stands settled (IsSettled)
	int mCounters[COUNTER_COUNT] = {};   // +0xc84: the game's counters (0x73d170); +0xcc0 (COUNTER_ACTIVITY) scales how often the idle shine starts
	int mLevelProgress = 0;          // +0xe00: how far the level bar has come in this level (AddPoints adds to it); the level ends when it is the level's size
	float mLevelBar = 0.0f;          // +0xd68: the level bar as drawn: it follows mLevelProgress over the level's size, and the level ends when it is 1
	float mSecondBar = 0.0f;         // +0xd6c: a second bar (the timer's, 0x755620: 0 where there is no timer, as in Classic); nothing reads it
	CurvedVal mBarRateCurve;         // +0x2258: 1 after a reset (0x74a880): the bar follows its target at 0.025 of the way (a curve at 0 makes it 0.0255)
	bool mLevelUpPending = false;    // +0x288c: the level bar is full (LevelUp, 0x74e190): the change starts when the board is at rest
	std::unique_ptr<Hyperspace> mHyperspace;    // +0x2890: the overlay of a level change that is running
	int mLevelCounter = 0;           // +0xe34: put to 0 when a level ends (0x760ac0, event 2); nothing translated reads it
	bool mGameOver = false;          // +0xd8c: no move is left (0x7602c0)
	int mGameOverTicks = 0;          // +0xd1c: 0 while the game is played, 1 at the game's end and one more each update (the end screen comes at 400, and the leaderboard's wait after that)
	int mEndDialogWait = 0;          // +0x307c: ClassicBoard's own counter (its leaderboard listener's): the end screen waits for it to run out (0x73b2d0, 0x73b100)
	bool mEndScreenShown = false;    // the end-of-game dialog (id 0x26, ClassicEndLevelDialog, made by 0x73b100) has been made
	int mHintTimer = 0;              // +0x26a4: updates the board has stood at rest with nothing to do; at 1500 the hint is given
	int mHintFlash = 0;              // +0x26a0: counts down from 250 after a hint was asked for (the hint button's glow: drawn only)
	bool mAutoHints = true;          // +0x1370: 1 from the constructor (0x74b49a): a hint comes by itself after the wait
	int mBotMoves = 0;               // port: test only (the script's bot command): how many more moves the bot plays itself; -1 without a limit
	int mBotMoveCount = 0;           // port: test only: the moves the bot has made
	bool mShineOn = false;           // +0x27b0: the idle shine has started and the board has not been unsettled since
	int mShineUpdate = 0;            // +0x27b4: the update the idle shine last started in
	CurvedVal mShineCurve;           // +0x2740: the idle shine's curve, started by UpdateGame
	CurvedVal mFallFactorCurve;      // +0xe58: 1 after a reset; the fall speed's factor is made of it (GetFallFactor)
	CurvedVal mCoverCurve;           // +0x2898: 1 from the constructor (0x74b76c): while it is 1 the main menu behind is hidden (0x6a3530)

	Board();                                   // 0x74ae30 (bejeweled3)
	~Board() override;
	// 0x74bc40 (bejeweled3), the listener at +0xbc: the Hint button (id 0) asks for a hint (slot 202); the Menu button (id 1) is not
	// translated yet (the in-game menu dialog, 0x681370)
	void ButtonDepress(int theId) override;
	enum { BUTTON_HINT = 0, BUTTON_MENU = 1, BUTTON_RESET = 2 };   // the ids 0x741840 gives the buttons (3 is the replay button, not made)
	enum { DIALOG_RESET = 0x16 };   // the dialog that asks whether to abandon the game (0x74bc40, case 2)
	// 0x7563c0 (bejeweled3), the board's dialog listener (the subobject at +0xc0): the reset dialog (id 0x16) answered Yes (1000)
	// abandons the game (the saved game's file, which 0x743120 deletes, is not kept in the port; slots 85 and 135 start the new one);
	// either answer closes the dialog (0x76a670). The listener's case for the id 0x12 (the tutorial dialogs) is not translated
	void DialogButtonDepress(int theDialogId, int theButtonId) override;

	// slot 85, 0x74a880 (bejeweled3): the board is emptied for a new game
	virtual void ResetGame();
	// slot 117, 0x756390 (bejeweled3): how strongly gravity pulls: 1 at the curve's 1, 0.65 less for each step of it below
	float GetFallFactor() { return (float)((mFallFactorCurve.GetOutVal() - 1.0) * 0.64999997615814209 + 1.0); }
	// slot 22, 0x759170 (bejeweled3): the board's own update: the game speed gives the number of steps this update is worth
	// (one at the normal speed) and each runs UpdateTick
	void Update() override;
	// slot 234, 0x767090 (bejeweled3): one step of the game (virtual since Ice Storm's 0x6e5850 runs after it)
	virtual void UpdateTick();
	// slot 231, 0x767e20 (bejeweled3): the game's part of a step: the fill, the fall and the idle shine (ButterflyBoard's 0x6e82a0 runs it first)
	virtual void UpdateGame();
	// slot 214, 0x73dd70 (bejeweled3): gravity: each column's pieces move by their speeds and land on the row or the piece below
	virtual void UpdateFalling();   // virtual: Diamond Mine's slot 214 (0x736d60) runs it and then stops its bricks
	// 0x75fca0 (bejeweled3): every piece is at rest in its cell, nothing is left to land, and there is no match
	bool IsSettled();
	// 0x75fc10 (bejeweled3) without the first-settled call: the board is at rest (the tests of IsSettled)
	bool IsAtRest();
	// slot 140 (0x230): the first time IsSettled finds the board at rest (0x75fd15); Quest-derived boards start their play here (slot 256)
	virtual void OnFirstSettled() {}
	// slot 192 (0x300): a swap of the player's found no match and goes back (0x759ba0, after the counter 0x73d170): Bad Move Limit counts it
	virtual void OnSwapFailed(const SwapRecord& theSwap) { (void)theSwap; }
	// slot 204 (0x330), the last call of 0x763e90: the fill has been accepted (Time Bomb's 0x6c53d0 counts the bomb it made)
	virtual void OnFillDone(const std::vector<Piece*>& theNewPieces) { (void)theNewPieces; }
	// slot 212 (0x350, base 0x73c710), run by CountDownMoves for each piece and (with theTick) by the bombs' real-time clock: a piece's counter counts down
	virtual void CountDownPiece(Piece* thePiece, bool theTick);
	// slot 181 (0x2d4), run once in each UpdateGame after the pieces' flag loops and before the busy counts (Filler's warning, 0x6dc770, and Time Bomb's
	// flag loop, 0x767e20 at about line 190)
	virtual void UpdateWarnings() {}
	// slot 191 (0x2fc), 0x75a58a: a swap has found matches (the swap record is given); with the pieces' move counters of 0x73c7e0 first
	virtual void OnSwapMatched(const SwapRecord& theSwap) { (void)theSwap; }
	// slot 211 (0x34c, 0x73ba10 returns at once; Ice Storm's 0x6e2250 reads it): FindMatches has kept one run of pieces
	virtual void OnRunFound(const std::vector<Piece*>& thePieces) { (void)thePieces; }
	// slot 106 (0x1a8, 0x73c240 is 15): the seconds a board at rest waits before it shows a hint (Ice Storm's 0x6c5cc0 gives 5 in a secret mode)
	virtual int GetHintWaitSeconds() const { return 15; }
	// 0x73c7e0 (bejeweled3) with slot 212 (0x73c710) for each piece without the bits 0x40 and 0x100: a butterfly gem (FLAG_BUTTERFLY) with
	// moves left counts one down (the other branches of 0x73c710, for the bit 0x200, are the Quest boards' goals': not translated). Run
	// by 0x75f85c, 0x75f8be (a hypercube's swap) and by 0x74c070 (a swap that found a match), before slots 179 and 191
	void CountDownMoves();
	// 0x759840 (bejeweled3), run in each step by 0x767e20 (at 0x76818e, after the fill and before the swaps): a move's record goes once no match
	// group lives and the pieces that carry its id have all come to rest (their tag, +0x188, is put back to -1), and no column's tag holds it. The
	// exe's sounds and the hint's flags at its end are left out
	void UpdateMoveRecords();
	// 0x7492c0 (bejeweled3), slot 168's sibling: the piece becomes a butterfly gem (FLAG_BUTTERFLY and nothing else); 0x748ad0 makes its looks
	void MakeButterflyGem(Piece* thePiece);
	void MakeButterflyEffect(Piece* thePiece);
	// the level bar (0x755940) and the level change exist: false on a board with no levels (the secret modes: slot 217 of QuestBoard, 0x6c5460, is false)
	virtual bool HasLevels() const { return true; }
	// 0x755ad0 (bejeweled3): the piece is still, in its cell and free to be played
	static bool PieceIsSettled(const Piece* thePiece);
	// slot 164, 0x749c20 (bejeweled3) with a step of 1 (the only one the translated code gives it): the next of the 19 backgrounds is
	// made and put behind the screen, the one before it goes
	void SetBackground(int theStep);
	// slot 232 (0x756730): the dialog's MAIN MENU button: the first screen comes back and this one fades out and goes
	virtual void LeaveToMenu();
	// slot 135, 0x74e410 (bejeweled3): a new game: its background is chosen and the first fill is made
	virtual void NewGame();
	// 0x74e410 (bejeweled3): the background of a new game: Classic's slot 100 (0x73afa0) says to start from the first of the game's order; the Quest-derived
	// boards (slot 100 of Board) read the setting BackgroundIdx and keep it
	virtual void ChooseBackground();
	// slot 203 (0x6dd4d0, QuestBoard): the fill does nothing once the game's end has begun (+0xd1c or +0xd20 is not 0)
	virtual bool IsFillStopped() const { return false; }
	// slot 203, 0x763e90 (bejeweled3): makes the pieces that the empty cells need above the grid, lets them fall into
	// place one row at a time and gives them colours that make no match and leave a move. theNoMatchCheck is the
	// exe's second argument (set when it gave up). Classic's first fill is this with 0 for it.
	virtual void FillInBlanks(bool theNoMatchCheck);   // virtual: Filler's (0x6db550) and Balance's (0x6dd720) take slot 203

	// 0x73ba40 (bejeweled3): the grid has three or more of a colour in a row or column
	bool HasMatches() const;
	// 0x75a8e0 (bejeweled3) with no piece to include: some swap of two neighbours makes a run of three. theNeedBonus is
	// its last argument: only a run of four or more, or a crossing of two, counts
	bool HasMove(bool theNeedBonus) { return FindMove(nullptr, 0, false, nullptr, theNeedBonus); }
	// 0x75a8e0 (bejeweled3): the moves of two neighbours that make a run, in the exe's order (rows from the top, or from the bottom
	// when theReverse; in a row from the left; for a gem the right, left, lower and upper neighbour). The theSkip'th is given in
	// theOut (when it is not null) and true is returned; theMustInclude is a piece the run has to hold (null: any), theNeedBonus as
	// in HasMove
	bool FindMove(BoardMove* theOut, int theSkip, bool theReverse, const Piece* theMustInclude, bool theNeedBonus);
	// 0x75ae10 (bejeweled3): a move for the hint: two draws of the app's Rand() (2 and 10) say from which end of the grid and how
	// many moves to skip; it is tried down to 0 skips, then from the other end
	bool FindHint(BoardMove* theOut);
	// slot 202, 0x75aeb0 (bejeweled3): the hint is asked for (by the player with theUser, else by the wait); the timer starts again.
	// port: the help tip, the glow of the gem (0x748480) and its sound are left out
	void ShowHint(bool theUser);
	// slot 98, 0x73d4f0 (bejeweled3): a hint may come now
	virtual bool HintAllowed() const;

	// slot 53, 0x73f140 (bejeweled3): picks a gem up, or swaps the one picked up with the one clicked
	using Widget::MouseDown;
	using Widget::MouseUp;
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;
	// slot 57, 0x73d990 (bejeweled3): the button comes up
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;
	// slot 58, 0x73c510 (bejeweled3): dragging a picked-up gem a fifth of a cell sideways or up and down swaps it
	void MouseDrag(int x, int y) override;
	// 0x73bf00, 0x73bc50 (bejeweled3): the piece that is picked up, and the piece at a place (in 1200ths from the grid's corner)
	Piece* GetSelectedPiece() const;
	Piece* GetPieceAt(float theX, float theY) const;
	// slot 194, 0x740e80 (bejeweled3): asks for a swap of the piece with the one at a cell. It starts in the next update
	bool TrySwap(Piece* thePiece, int theRow, int theCol, bool theForced = false);
	// slot 193, 0x75fae0 (bejeweled3): the swap is allowed
	bool CanSwap(Piece* thePiece, unsigned theRow, unsigned theCol);
	// slot 195, 0x75ee20 (bejeweled3): the swap begins
	bool StartSwap(Piece* thePiece, int theRow, int theCol, bool theForced = false);
	// 0x759ba0 (bejeweled3): the swaps move on, and are checked for matches when they come to an end
	void UpdateSwaps();
	// 0x73caa0 (bejeweled3): a swap holds the piece
	bool IsHeldBySwap(const Piece* thePiece) const;
	// slot 201, 0x75af60 (bejeweled3): looks for runs of three or more and makes a match of each; returns how many
	virtual int FindMatches();
	// 0x75dee0 (bejeweled3): the match groups run: their pieces' timers, their rings, their end
	void UpdateMatchGroups();
	// 0x767e20, 0x7690d0 to 0x769149: the gems that are picked up or turning turn further
	void UpdatePieceSpins();
	void UpdatePiecePulses();   // the loop of 0x767e20 (0x768f7b to 0x7690b8) over the pieces' scale curves
	// 0x789a90 (bejeweled3), for the types other than 7: a group is made for a piece and put at the end of the list
	MatchGroup* MakeMatchGroup(Piece* thePiece, int theType);
	// 0x73c9d0 (bejeweled3): the index of the group made for the piece (the group's +0x34 is its id), or -1
	int FindMatchGroup(const Piece* thePiece) const;
	// slot 200, 0x74c570 (bejeweled3): the piece is to be cleared by another piece; true when it took it
	bool ClearPiece(Piece* thePiece, Piece* theBy);
	// slots 166, 167 and 168 (0x745ca0, 0x749570, 0x7494d0, bejeweled3): the piece becomes a flame gem (FLAG_FLAME), a hypercube
	// (FLAG_HYPERCUBE; theWithEffect is the exe's second argument) or a star gem (FLAG_STAR)
	virtual void MakeFlameGem(Piece* thePiece);
	void MakeHypercube(Piece* thePiece, bool theWithEffect);
	virtual void MakeStarGem(Piece* thePiece);
	// BoardEffects.cpp: the persistent looks of the special gems, the flame gem's creation and the hint's flash
	// 0x749220 (bejeweled3): makes the effect a gem's flags ask for (the Star Gem's and the Hypercube's here)
	void MakeSpecialEffect(Piece* thePiece);
	void MakeStarEffects(Piece* thePiece);        // 0x749050, with 0x748dc0 and 0x748f30: a Star Gem's two effects, one in each layer
	void MakeHypercubeEffect(Piece* thePiece);    // 0x748a00: PIEFFECT_HYPERCUBE under the gems
	void MakeFlameCreationEffects(Piece* thePiece);   // 0x745ca0: the two PopAnimEffects of POPANIM_FLAMEGEMCREATION
	void MakeHintEffect(Piece* thePiece, bool theUser);   // 0x748480: PIEFFECT_HINTFLASH over the gem the hint is for
	void ClearEffects();                          // 0x729ca0 for both layers: every effect goes (pieces let go first)
	void DrawOverlay(Graphics* g) override;       // slot 44: the over layer's overlay pass (0x72e3c0)
	// slot 198, 0x74d940 with 0x73f8e0 (bejeweled3): a flame gem at a place (in pixels, the middle of its cell) goes off
	void Explode(int theX, int theY);
	// 0x73f8e0 (bejeweled3): the draws of the app's Rand() that the particles of the explosion make for one neighbour of the flame gem
	void DrawExplosionRandoms();
	// slot 179, 0x740f20 (bejeweled3): a hypercube is swapped with a piece: it starts its group at once, and no swap is made
	virtual void StartHypercube(Piece* theCube, Piece* thePartner);
	// slot 180, 0x74db70 (bejeweled3): the hypercube makes its group (type 7) for the colour (-1: every colour)
	virtual void MakeHypercubeGroup(Piece* theCube, int theColor);
	// 0x7870f0 (bejeweled3), run by 0x78a390 for a group of type 7: picks the pieces of the colour one by one, makes a bolt to each,
	// and clears the ones whose timers have come to 1
	void UpdateHypercubeGroup(MatchGroup* theGroup);
	// 0x767e20 (bejeweled3), from 0x7682b0 to 0x768b3f: the countdowns of the specials (+0x184) and the busy counts (+0x230): a piece
	// whose count comes to 0 goes off, is cleared by another or is destroyed
	void UpdateBusyPieces();
	// 0x767e20 (bejeweled3), 0x767ad0 and 0x769155 to 0x769281: the pieces' slide curves step and put the pieces along their slides
	void StepSlideCurves();
	void UpdateSlidePositions();
	// 0x75dee0 and 0x74c770 (bejeweled3): the draws of the app's Rand() that the 20 particles of a ring make for one piece
	void DrawRingParticleRandoms();
	// true when any match group lives (the fill waits for them)
	bool HasLiveMatches() const { return !mMatchGroups.empty(); }
	// slot 200 (0x74c570) and 0x74c770 (bejeweled3): the piece is destroyed; slot 176, 0x755b80 (bejeweled3): it leaves the grid
	void DestroyPiece(Piece* thePiece);
	void DrawSparkRandoms();
	void RemovePiece(Piece* thePiece);
	// slot 152, 0x74df80 with 0x74a210 (bejeweled3): points are added to the score
	virtual void AddPoints(int thePoints, int theMoveId = -1, bool theCountsForProgress = true, bool theUsesMultiplier = true);
	// 0x73d170 and 0x73cf10 (bejeweled3): a counter goes up (the game's, the move's), and a move's counter
	void AddCounter(int theKey, int theValue, int theMoveId);
	int GetMoveCounter(int theMoveId, int theKey);
	// slot 154 (0x73bf70): how much the level bar needs in this level. ZenBoard 0x70e450 and SpeedBoard 0x7140d0 override it
	virtual int GetLevelMax() const;
	// 0x755940 and slot 217, 0x73b810 (bejeweled3): the level bar follows its target, and when it and the progress are full the level
	// change is asked for (slot 156, 0x74e190)
	void UpdateLevelBar();
	virtual bool IsLevelFull() const;   // slot 217 (0x73b810; QuestBoard's 0x6c5460 hands it to the goal's slot 26): see UpdateLevelBar
	virtual void OnLevelFull();   // slot 156: the level is full
	// 0x767e20 (bejeweled3), after the match groups: the level change starts when the board is at rest (the overlay is made)
	void StartLevelChange();
	// 0x725ef0 (bejeweled3): the overlay's update, at the start of every step while it runs
	void UpdateHyperspace();
	// slot 157, 0x760ac0 (bejeweled3): the level change's events (2: the level goes up; 5: the overlay goes)
	void LevelEvent(int theEvent);
	// 0x7604d0 (bejeweled3): the gems are put in a new order that has a move (the level change's middle)
	void ReshuffleBoard();
	// slot 161 (0x73b2d0 for Classic) and 0x7602c0 (bejeweled3): no move is left: the score is handed in and the game ends
	virtual void NoMoreMoves(bool theAnnounce);
	// slot 233 (0x73b100, Lightning's 0x716ab0): the end-of-game dialog made for the board
	virtual ClassicEndLevelDialog* MakeEndLevelDialog();
	void EndGame();   // 0x7602c0 (bejeweled3): the game is over (the end of NoMoreMoves)
	// 0x767e20 (bejeweled3): the game's end goes on (the counter at +0xd1c) and the end screen comes (slot 233, 0x73b100)
	void UpdateGameOver();
	// port: the top scores of the mode (the exe's LocalLeaderboard, 0x6728b0), in the order of the leaderboard, kept for the game
	std::vector<int> mHighScores;
	std::vector<std::string> mHighScoreNames;   // port: the name (UTF-8) each score was made under, parallel to mHighScores
	int mLastScoreRank = -1;                    // port: where the last SubmitScore put its score (index), -1 when it got no place (the exe's end dialog +0x380)
	void SubmitScore(int theScore);
	// port: test only: the bot's move when the board is at rest (the exe has no bot; this is the move search's first move)
	void UpdateBot();
	// port: test only: false holds the bot back (Diamond Mine's scroll starts from a board at rest, which a bot's swap in the same update would hide)
	virtual bool BotMayMove() const { return true; }
	// slot 230, 0x73b6d0 (bejeweled3): the shown score follows the score, every fourth update
	void UpdateShownScore();
	// 0x744660 (bejeweled3): a new move record, and 0x73cf10 and 0x73d170: its counter
	MoveRecord* NewMoveRecord();
	MoveRecord* FindMoveRecord(int theId);
	// 0x73e370 (bejeweled3): the piece with an id
	Piece* FindPieceById(int theId) const;

	// 0x7415c0 (bejeweled3): a new piece at a cell
	Piece* MakePiece(int theCol, int theRow);
	// slots 171 and 172 (0x73bd80 and 0x73bd70 on Board and ClassicBoard; SpeedBoard's 172 is 0x714100: 0x82 below the board's own +0x2c50, which is 0)
	virtual int GetGridX() const { return GRID_X; }
	virtual int GetGridY() const { return GRID_Y; }

	// slot 89, 0x7413e0 (bejeweled3): the board's "Title" setting (the music's song, the log's mode). SpeedBoard's own 0x7144a0 says "Speed"
	virtual const char* GetModeName() const { return "Classic"; }

	// The slots that the modes' boards change (docs/NOTES.md, "Zen and Lightning"). Each is the base board's value here, which is Classic's
	// slot 113 (0x1c4, 0x73c1f0 on Board and ClassicBoard, 0x70e3a0 on ZenBoard): true on the Zen board, where the match timers (0.01 for a
	// piece the ring reaches, instead of 0.015), the explosion's busy counts and the sparks' draws take the slow branch
	virtual bool IsSlowMode() const { return false; }
	// slot 122 (0x1e8): the factor of the level in a point's progress and score. Board's own 0x73b600 is 1; ClassicBoard 0x73af50 is the level index + 1
	// (what this base returns, since the port's Board stands for Classic's), ZenBoard 0x70e330 is 1 and SpeedBoard 0x714090 is 5
	virtual float GetLevelFactor() const { return (float)mLevelIndex + 1.0f; }
	// slot 143 (0x23c), called with the piece by slot 142 (0x73d2b0), which RemovePiece (slot 176, 0x755b80) calls first: a piece is cleared. The
	// base 0x74b9e0 only makes effects for the bits 0x400 and 0x200, which Classic's pieces never have
	virtual void OnPieceCleared(Piece* thePiece) { (void)thePiece; }
	// slot 142 (0x238, 0x73d2b0 on Board), run by RemovePiece (0x755b80) for the piece with the flag 1 before slot 143: QuestBoard's 0x6c63c0 hands it to the goal's slot 39 first
	virtual void OnPieceClearing(Piece* thePiece, bool theFlag) { (void)thePiece; (void)theFlag; }
	// slot 197 (0x314, 0x73bb40 on Board returns at once): a special's reach touches a cell (an explosion's nine cells, 0x73f8e0; a match group's ring cells and its first piece's cell,
	// 0x75dee0), whether or not a gem is there. QuestBoard's 0x6c4b80 hands it to the goal's slot 16
	virtual void OnCellHit(int theCol, int theRow) { (void)theCol; (void)theRow; }
	// slot 176 (0x2c0, 0x755b80) with slot 142 (0x73d2b0) for a piece an explosion or a clear takes out: true keeps the piece in the grid (nothing of
	// Classic's boards does: Diamond Mine's 0x6bb6a0 and 0x6b6a80 keep a brick that the hit has not broken, and turn the dirt that hides a hypercube into it)
	virtual bool OnPieceRemoving(Piece* thePiece) { (void)thePiece; return false; }
	// slot 185 (0x2e4): the fill has a colouring for its new pieces that is accepted (0x73bc00 on Board and ClassicBoard is true); SpeedBoard's
	// 0x716c60 lifts the pieces and makes the time gems, and says true as well
	virtual bool OnFillAccepted(std::vector<Piece*>& theNewPieces) { (void)theNewPieces; return true; }
	// slot 104 (0x1a0): the seconds a round lasts (0 where no clock runs: 0x73c250 on Board; SpeedBoard 0x7140f0 is 60)
	virtual int GetTimeLimit() const { return 0; }
	// slot 188 (0x2f0): where the level bar is going (0x744b10 on Board: the progress over the level's size, at most 1; SpeedBoard 0x716f70
	// also ends the round when the time is out)
	virtual float GetBarTarget();
	// slot 155 (0x26c, 0x73bf60 on Board): how far the level has come
	virtual int GetLevelProgress() const { return mLevelProgress; }

	// the settings of the board's quest (+0xc8, a string map filled by the start of the mode: speed.cfg's "Lightning" for SpeedBoard). The
	// board's "Title" is among them
	std::map<std::string, std::string> mSettings;
	// 0x73b250 (bejeweled3): the panel (0x751d50), then the gems and the rest of the screen (0x750320)
	void Draw(Graphics* g) override;
	void DrawPanel(Graphics* g);                           // 0x751d50 (bejeweled3): the checkered squares behind the gems
	// the three pieces of the left panel DrawScreenFrame draws (the exe's slots 249, 255 and 251), which Zen replaces
	virtual void DrawTopWidget(Graphics* g);
	virtual bool ShowsLevelNumber() const { return true; }   // the top widget (slot 249) writes the level over its picture; a board with no levels does not
	virtual void DrawScoreText(Graphics* g);
	virtual void DrawBottomWidget(Graphics* g);
	// slot 237, 0x757df0 (bejeweled3): theScaleFactor is its third argument (1; the swap loop of 0x7568a0 gives 1 plus or minus the swap's lift)
	void DrawPiece(Graphics* g, const Piece* thePiece, float theScaleFactor = 1.0f);
	void DrawHypercube(Graphics* g, const Piece* thePiece);       // slot 235, 0x73f630 (bejeweled3)
	void DrawPieceShadow(Graphics* g, const Piece* thePiece);   // slot 236, 0x754670 (bejeweled3): the shadow sheet's cel under a gem that is drawn from its sheet
	void DrawPieces(Graphics* g);                         // slot 246, 0x7568a0 (bejeweled3) with its second argument 0: the passes over the gems
	virtual void DrawScreenFrame(Graphics* g);             // the pictures round the grid (slots 0x3bc, 0x3c0, 0x3c4, 0x3e4) and the left panel
	// the board's own pass over the screen after everything else (the exe's slot 43, whose Lightning version 0x716ef0 draws the mode's
	// HUD); nothing in the modes that have none
	virtual void DrawModeOverlay(Graphics* g) { (void)g; }
	// port: drawn by Board::Draw before the gems, for what lies behind them (the wall, the gold squares, the sand); the exe's goals draw theirs by slot 56 (0xe0) around the pieces' draw
	virtual void DrawBehindPieces(Graphics* g) { (void)g; }
	// 0x741840 (bejeweled3): the Hint and Menu buttons with the pictures of the mode (Classic's by default)
	// theResetImage (the Lightning modes') adds the Reset button (id 2)
	void MakeButtons(Image* theHintImage, int theHintImageId, Image* theMenuImage, int theMenuImageId,
		Image* theResetImage = nullptr, int theResetImageId = 0);
	void MakeClassicButtons();
};

class ClassicBoard : public Board {
public:
	ClassicBoard();   // 0x73b390 (bejeweled3)
};

}  // namespace Sexy
