// Sexy::Board (about 0x3940 bytes, vtable 0x7dd0d4 with 71 slots; 0x6a3d7b-0x6d6899): the game itself. Being
// ported (docs/NOTES.md, "The board"); each field's comment starts with its offset in the original's object, and the
// fields come in as the code that uses them does.
#pragma once

#include "PopAnim.h"
#include "TwistWidget.h"
#include "Tutorial.h"
#include "X86Math.h"
#include "WinTwistApp.h"

#include "Buffer.h"
#include "Challenge.h"
#include "CurvedVal.h"
#include "MTRand.h"
#include "Rect.h"
#include "SexyMatrix.h"

#include <deque>
#include <map>
#include <set>
#include <vector>

namespace Sexy {

class EffectsManager;
class GoodTube;
class Hyperspace;
class Font;
class Image;
class MemoryImage;
class Piece;
class TextImage;
class Traveller;
class PointsManager;
class ReplayWidget;
class Points;
class FlyingText;
class GemSoul;
class Shockwave;
class SoundInstance;
class WinTwistApp;

// A pair of numbers that change with the level, read from the config (0x74ec7d, "<name>FirstLevel" and the rest): from
// mFirstLevel on, a base, plus a change every mLevelsBetweenChange levels from mFirstChangeLevel and every
// mTurnsBetweenChange of the app's +0xcf8, held to a min and a max (each unless below 0). The original's record is
// 0x58 bytes.
// 0x198 bytes (0x6a3217): a geode flung out of broken coal (Board::AddGeode, 0x6ba87f). It waits mDelay updates, flies
// to the side with its sparks, and when it lands its points are added to the score. Not a Traveller: no vtable.
struct Geode {
	float mX = 0;                // +0x00
	float mY = 0;                // +0x04
	float mDX = 0;               // +0x08
	float mDY = 0;               // +0x0c
	float mLife = 0;             // +0x10: 0.45 while it flies; it lands when this is 0 or less
	float mSpin[8] = {-3.1415927f, -3.1415927f, -3.1415927f, -3.1415927f,
		-3.1415927f, -3.1415927f, -3.1415927f, -3.1415927f};  // +0x14: the angles of its eight arms
	CurvedVal mArmTurn;          // +0x38: the arms' turn (linked to +0xb8)
	int mPoints = 0;             // +0xa8
	int mDelay = 0;              // +0xac: updates until it starts
	int mLandCount = 0;          // +0xb0: set to 0 by the constructor, never read
	int mIndex = 0;              // +0xb4: the order it was made in (a pitch for its landing sound)
	CurvedVal mFlight;           // +0xb8: its flight
	CurvedVal mBrightness;       // +0x128: its brightness (linked to +0xb8)
};

// 0x10 bytes (0x74c8c8) at the board's +0x211c: a developer's stepper through the board (0x74db1b, run by UpdateGame
// while +0x210c is set, which nothing in the exe ever sets). The original sets only its first field; the rest is
// whatever the allocator left.
struct BoardStepper {
	WinTwistApp* mApp;       // +0x00
	int mBoard = 0;          // +0x04: the board, as 0x74db1b reads it
	int mStepState = 0;      // +0x08: never set by the original, a developer stepper's state
	int mStepCount = 0;      // +0x0c: never set by the original, a developer stepper's count
	explicit BoardStepper(WinTwistApp* theApp) : mApp(theApp) {}  // 0x74c8c8
};

struct LevelValue {
	int mValue = 0;                 // +0x0: GetCount (0x74eac6) at a new level
	int mFirstLevel = 0;            // +0x4
	int mFirstChangeLevel = 0;      // +0x8: mFirstLevel + mLevelsBetweenChange unless the config says
	int mBase = 0;                  // +0xc
	int mValueBase = 0;             // +0x10
	int mLevelsBetweenChange = 1;   // +0x14
	int mChange = 0;                // +0x18
	int mValueChange = 0;           // +0x1c
	int mMin = -1;                  // +0x20
	int mMax = -1;                  // +0x24
	int mValueMin = -1;             // +0x28
	int mValueMax = -1;             // +0x2c
	int mTurnsBetweenChange = 0;    // +0x30
	bool mTurnBased = false;        // +0x34
	double mChancePow = 0;          // +0x38
	double mChanceScale = 1.0;      // +0x40: multiplies the chance Roll works out (1.0 from Read)
	double mDistanceScale = 1.0;    // +0x48: multiplies the share of the count already used up in Roll (1.0 from Read)
	bool mIsDue = false;            // +0x50: set once Roll says it comes now; saved with mValue
	bool mKeepSavedValue = false;   // +0x51: Read leaves mValue alone

	void Read(const char* theName);  // 0x74ec7d
	int GetCount();                  // 0x74ea57: mBase, mChange, mMin, mMax
	int GetValue();                  // 0x74e9e8: mValueBase, mValueChange, mValueMin, mValueMax
	void Restart() { mValue = GetCount(); mIsDue = false; }  // 0x74eac6
	void RestartAtLevel();           // 0x74ead6: Restart when mFirstLevel is the board's level
	bool Roll();                     // 0x74eb38: one turn down; whether it comes now, by chance (BoardRand)
	bool ShouldAppear();             // 0x74ec45: from mFirstLevel on, whether it comes now
	bool NextTurn();                 // 0x74ec17: from mFirstLevel on, a turn-based one rolls; whether it is due
};

// A match, or the match a turn would make (0x6b97b6 makes one, 0x6afdee resets it)
struct MatchInfo {
	int mKind = 0;                  // +0x0: 4 for the square
	int mRow = -1;                  // +0x4
	int mCol = -1;                  // +0x8
	int mCount = 0;                 // +0xc: its gems
	bool mIsFresh = true;           // +0x10: true after Reset, never read
	int mColor = -1;                // +0x14
	std::vector<int> mIds;          // +0x18: its gems' ids
	int mSerial = 0;                // +0x28: from gMatchInfoCount
	int mPoints = 0;                // +0x2c: the points the match scores, shown over it
	bool mWasScored = false;        // +0x30: false after Reset, never read

	MatchInfo();
	void Reset();
};
extern int gMatchInfoCount;  // 0x84e890
extern int gSparkDelay;      // 0x85aef8: the delay FindMatches gives a gem's sparks (0x6a453b)

class Board : public TwistWidget, public TwistDialogListener, public PopAnimListener {  // the listeners at +0x14c, +0x150
public:
	// The game modes (+0x20c)
	enum { MODE_CASUAL = 0, MODE_CLASSIC = 1, MODE_CHALLENGE = 2, MODE_BLITZ = 3, MODE_ZEN = 4, MODE_PUZZLE = 5 };

	int mLevel = 0;                    // +0x158: the level (LevelValue)
	int mGameMode = 0;                 // +0x20c
	int mModeFlags = 0;                // +0x210: 8 for a replay
	WinTwistApp* mApp = nullptr;       // +0x954
	Image* mPuzzleImage = nullptr;     // +0x1320: the puzzle's picture (mode 5), cut into the gems
	bool mPuzzleSolved = false;        // +0x1324: a puzzle is solved (hints and cursor are off in a puzzle while it is set)
	int mRows = 0;                     // +0x1330
	int mCols = 0;                     // +0x1334
	Piece** mGrid = nullptr;           // +0x1338: mRows x mCols, row by row
	// +0x149c: the electro gems' strikes (the class at 0x71eb9e, not yet ported); ClearBoard deletes them
	std::vector<void*> mElectroStrikes;
	std::vector<Piece*> mBombList;     // +0x14bc: bombs about to go off, drawn over the board (Piece::DrawFront)
	struct HazeRect { float mLeft, mTop, mRight, mBottom; };
	std::vector<HazeRect> mHazeRects;  // +0x14cc: the rectangles over the bombs' cells the distortion drawing (0x6f7ab4) takes
	std::map<int, Piece*> mPieceMap;   // +0x1364: the pieces by id (0x6b8c20 adds, 0x6b677b drops)
	float mShineAmount = 1.0f;            // +0x163c
	float mPieceAlpha = 1.0f;          // +0x1640
	EffectsManager* mFXManager = nullptr;  // +0x1648
	EffectsManager* mFXManager2 = nullptr; // +0x164c
	int mBoardMouseX = 0;              // +0x15b4: the mouse, over the board (x)
	int mBoardMouseY = 0;              // +0x15b8: the mouse, over the board (y)
	// The config's level values (0x6c6469 reads them)
	LevelValue mLockGem;               // +0x1ca0
	LevelValue mSkulls;                // +0x1cf8
	LevelValue mCounter;               // +0x1d50
	LevelValue mBadGemsOnBoardMax;     // +0x1da8
	LevelValue mSavedOnlyLevelSetting;  // +0x1e00: a LevelValue between the bad-gem maximum and the level-up points; only saved and loaded (never read)
	LevelValue mLevelUpPoints;         // +0x1e58
	LevelValue mDevil;                 // +0x1eb0: a doom gem's count
	LevelValue mCoalGem;               // +0x1f08
	int mBombGemCount = 0;             // +0x1f78: the bomb gems on the board, counted each update while the state is 0; Piece uses it to time a bomb's sparks
	bool mCoalAsSnackers = false;      // +0x1bf2: coal is drawn as snackers
	Image* mShineImage = nullptr;      // +0x1c54
	int mBlastChainCount = 0;          // +0x1c5c: the blasts chained in this turn (0 when a twist starts); it adds to the heat wave's strength
	int mBurstSize = 0;                // +0x1c58: 25 when a twist starts; the size of the bursts at the gems
	float mTwistGlow = 0;              // +0x2038: 1 when a twist ends, falls by 0.02 an update
	int mLevelUpTilePhase = 0;         // +0x20dc: the level-up's tiles: 0 none, 1 turning over in rings, 2 waiting (set while the next level is made and filled)
	float mJumpFillWait = 0;           // +0x2108: the level jump's count down, 7.9 at its start, 0.01 less an update; no gems fill while above 0
	int mSparkBlock = 0;               // +0x21ac: no sparks while above 0
	float mBlueFlash = 0;           // +0x21b0: a blue flash over the board, alpha out of 255
	bool mDebugPieces = false;         // +0x21db: each piece shows its row and column
	float mMatchFlash = 0;             // +0x23ec: set to 1 by a match; the flash falls from there
	bool mTipShowing = false;          // +0x23f5: a tip's script or dialog is up (ShowNextTip sets it, the tip's end clears it)
	// +0x2584: the tutorial being run (Tutorial.h), which can point at a piece (its +0x1a4)
	Tutorial* mTutorial = nullptr;
	bool mHintsReady = false;          // +0x25a4: the hint can be shown (cleared when a match is made)
	float mBoardZoom = 1.0f;           // +0x268c
	float mShakeX = 0;                 // +0x2690
	float mShakeY = 0;                 // +0x2694
	bool mIsReplay = false;            // +0x37c4: the board plays a replay
	bool mQuietFill = false;           // +0x37f8: no sounds or marks for the gems made (loading a game)
	bool mReplayPodDue = false;        // +0x3904: the replay pod is to be shown once nothing else is up (UpdateGame shows it and clears this)
	float mNextLevelWait = 0;          // +0x380c: the wait before the next level starts, 0.01 less an update; the rank bar must be ready
	int mTurnRating = 0;               // +0x3804: the turn rating (0x6a5179)
	float mBoardFade = 1.0f;           // +0x3924: the board brightness, 0 or 1
	int mBoardX = 0;                   // +0x3474: the grid's left edge
	// The rest of the fields the constructor (0x6c4df9) sets
	int mInputLock = 0;                     // +0x15c: while above 0 the cursor does not move and no twist starts; only ever 0 once play has begun (saved)
	int mPlayState = 0;                     // +0x160: the board's state: 0 play, 1 a replay's rewind, 3 and 4 waits, 5 the level's start, 7 the level-up's blast, 8 the puzzle's fill
	int mScore = 0;                         // +0x168: the score
	int mScoreAtTurnStart = 0;              // +0x16c: the score when the turn began (the turn's points are +0x168 minus it)
	int mProgressGainAtTurnStart = 0;       // +0x170: +0x14fc as the twist began (never read)
	int mGemsBroken = 0;                    // +0x1e8: the gems broken in the game (counted by BreakPiece; saved)
	int mGemsBrokenAtTurnStart = 0;         // +0x1ec: +0x1e8 when the turn began; the difference is the turn's gems
	int mGemsCleared = 0;                   // +0x1f0: the gems cleared by matches in the game (saved)
	int mGemsClearedAtTurnStart = 0;        // +0x1f4: +0x1f0 when the turn began (never read)
	int mLevelGemCount = 0;                 // +0x1f8: the gems counted for the big count popup (0 at a new game)
	int mGameGemCount = 0;                  // +0x1fc: the gems counted in the game (saved); with +0x1f8 it grows when a gem is first broken
	int mLevelProgress = 0;                 // +0x200: the points gained toward the level-up (saved)
	int mLevelProgressGoal = 1;             // +0x204: the points that finish the level (the level-up points, 500 at most in some modes)
	int mFruitsAwarded = 0;                 // +0x218: fruits awarded (AwardFruit: their colours go round the seven)
	int mSavedOnlyValue = 0;                // +0x21c: only saved and loaded
	int mLevelStepCount = 0;                // +0x224: the steps made so far; GetLevelFor turns them into a level (saved)
	int mMostMovesInLevel = 0;              // +0x234: the most moves in a level
	int mFruitsBroken = 0;                  // +0x22c: fruits broken (nothing reads it)
	float mFruitBlast = 0;                  // +0x248: the fruit blast, 0 to 1 (0 none; UpdateFruitBlast)
	bool mFruitBrokeFree = false;           // +0x24c: a fruit broke with no lock or bomb on the board (nothing reads it)
	float mPlaySeconds = 0;                 // +0x230: up 0.01 each update while +0x15c is 0 (saved as text)
	int mFruitCol = -1;                     // +0x250: the broken fruit column
	int mFruitRow = -1;                     // +0x254: the broken fruit row
	int mFruitId = -1;                      // +0x258: the broken fruit id
	CurvedVal mBlastRingSize;               // +0x260: the blast rings size, by their progress
	CurvedVal mBlastRingAlpha;              // +0x2d0: the blast rings opacity
	CurvedVal mFruitGlowAlpha;              // +0x340: the fruit glow opacity, by the blast progress
	CurvedVal mFruitBlastScale;             // +0x3b0: the fruit and its glow scale
	int mFruitColor = 0;                    // +0x420: the broken fruit colour
	CurvedVal mBannerStripe;                // +0x428: the mode-unlock banner stripe (0x6b40b5)
	CurvedVal mBannerShow;                  // +0x498: set by 0x6b40b5; while it is not 0 the mode-unlock banner is drawn and the rank bar draws the overlays above priority 50 under it
	                                        // rank bar draws the overlays above priority 50 under it
	CurvedVal mBannerModePicture;           // +0x508: the banner mode picture
	CurvedVal mBannerGlow;                  // +0x578: the banner glow
	CurvedVal mBannerSheenColor;            // +0x5e8: the banner sheen colour
	CurvedVal mBannerSheenPos;              // +0x658: the banner sheen place along the width
	int mUnlockedMode = 0;                  // +0x6c8: the mode unlocked last (2 is the challenge)
	bool mTooltipUp = false;                // +0x6cc: the tooltip is up (UpdateToolTips, DrawOverlay)
	int mTooltipHoverCount = 0;             // +0x6d0: the updates the mouse has stayed over the same text
	SexyString mTooltipTitle;               // +0x6d4: the tooltip's title
	SexyString mTooltipText;                // +0x6f0: the tooltip's text
	int mTooltipX = 0;                      // +0x70c: the tooltip's x (the place of the gem or the tube's part)
	int mTooltipY = 0;                      // +0x710: and y
	int mTooltipWidth = 0;                  // +0x714: and width
	CurvedVal mTooltipArrowSlide;           // +0x718: the tooltip's arrow's slide
	CurvedVal mTooltipFade;                 // +0x788: the tooltip's fade
	SexyString mTooltipHoverText;           // +0x7f8: the text the mouse is over now
	int mSlowDownWait = 0;                  // +0x814: updates left of a slow-down while a tip about power and electro gems shows; the game's speed eases to 0.4 for its last 30
	int mChainLinks = 0;                    // +0x818: the chain's links so far in this level (saved); 0 when a level begins
	int mTipIndex = 0;                      // +0x82c: the tip shown, from the profile's count
	CurvedVal mTipSlide;                    // +0x830: the tip's slide
	SexyString mTipText;                    // +0x8a0: the tip's text
	float mTipX = 0;                        // +0x8c0: the tip's x
	float mTipSlideBackWait = 0;            // +0x8bc: the tip's wait before it slides back
	// +0x135c: a value per cell (AllocGrid)
	Piece** mSavedGrid = nullptr;    // +0x135c: a copy of the grid kept while a turn is tried (0x6a3b00 and 0x6a3b26)
	// +0x1360: a record per cell (0x44 bytes, 0x6a38a7), row by row
	struct Cell {
		bool mSavedState = false;
		float mFade = 0;
		int mRow = 0;              // +0x8
		int mCol = 0;              // +0xc
		bool mHoldsFixedGem = false;
		Color mColor = Color(0, 0, 0, 0);  // +0x14: Color() with its alpha then cleared
		float mLightPulse = 0;
		float mLightGlow = 0;
		bool mLevelUpStarted = false;
		float mLevelUpTurn;        // +0x30: not set by the constructor; the tile's turn, 1 down to 0 (UpdateLevelUp)
		bool mIsMarked = false;
		int mMarkTick = 0;
		float mMarkGlow = 0;
		float mHintFlash = 0;
	};
	Cell* mCells = nullptr;
	int* mColumnBreakTime = nullptr;        // +0x1370: the update count at which a gem last broke in each column
	int mStartState = 3;                    // +0x1378: 3 at first, 0 or 2 while the start's fade (+0x13b0) runs, 1 once play may begin
	bool mStatsAdded = false;               // +0x137c: the game's totals have been added to the profile (UpdateRecords does it once)
	bool mIsBlitzGame = false;  // +0x137d: set for a blitz game; its seconds go to the blitz total of the app
	bool mIsUnscoredMode = false;           // +0x137e: never set; if it were, the game would not raise the profile's best score and its seconds would go to a fourth total
	bool mDebugColorOff = false;            // +0x137f: never set; draws without the hue-cycling debug colour when set
	bool mEditorNoMatches = false;          // +0x1382: never set; FindMatches finds nothing when set, and the editor's first label (yellow) shows
	bool mEditorGreenLabel = false;         // +0x1383: never set; the editor's green label shows when set
	bool mShowEditorLabels = false;         // +0x1384: never set; the level editor's labels show when set
	SoundInstance* mRewindSound = nullptr;  // +0x138c: SOUND_REWIND, looping while a replay rewinds
	bool mBeforeFirstTwist = true;          // +0x1390: true until the game's first twist (set again by a new game); no save is kept and no level ends while it is set
	int mFallDelay = 0;                     // +0x1394: updates left before FallPieces lets the gems fall again
	bool mClearBacklogDue = false;          // +0x1398: the next draw clears the update backlog (set at the end of a level jump)
	Widget* mCornerWidget = nullptr;                  // +0x139c: a widget resized to the lower right each update; never set, so always null
	CurvedVal mZenExtraCurve;               // +0x8c8: stepped every update, twice in zen; nothing reads it (never read)
	CurvedVal mAngryBombFade;               // +0x2898: the angry bomb over a gem: its opacity (set when a puzzle is lost in the wheel spin)
	CurvedVal mCascadePopupScale;           // +0x36c0: the cascade popup's shadow scale; while it runs, +0x37a0 fades
	CurvedVal mCountPopupScale;             // +0x3730: the count popup's shadow scale; while it runs, +0x37a4 fades
	CurvedVal mGridSlide;                   // +0x3478: the grid's slide to its place
	CurvedVal mLevelSlideIn;                // +0x3810: the new level's slide in from the right (0x6d090f)
	CurvedVal mTubeSlide;                   // +0x3880: the tube's slide out and back, started by 0x6b681d
	CurvedVal mGoScale;                     // +0x34f0: the "GO" picture's scale and fade, started when a turn ends (0x6b8bca)
	CurvedVal mStartFade;                   // +0x13b0: the level start's fade, set to 0 by StartLevel; it runs while +0x1378 is 0 or 2
	// +0x27b8 and the five linked to it: the nuke at the end of a game (EndGame)
	CurvedVal mNukeProgress;                // +0x27b8: the nuke's progress; also scales the nuke's cloud (the end of a game, EndGame)
	CurvedVal mNukeCloudAlpha;              // +0x2828: the nuke cloud's opacity
	CurvedVal mNukeNovaScale;               // +0x26d8: the nova's scale, and the blast's reach
	CurvedVal mNukeNovaAlpha;               // +0x2748: the nova's opacity
	CurvedVal mNukeGemZoom;                 // +0x2908: the zoom of the gem that set the nuke off
	CurvedVal mNukeTubeShake;               // +0x2978: how far the tube is turned and moved during the nuke
	SexyTransform2D mTransform;             // +0x29e8: DrawAll's scale and offsets, pushed to the device in 3D mode (0x44f320 clears it)
	bool mGlintRunning = false;             // +0x1420: a glint is running over the board
	float mGlintPlace = 0;                  // +0x1424: the glint's run, 0 to 1
	float mDimAmount = 0;                   // +0x1428: counts down 0.06 an update to 0; nothing sets it above 0
	float mDimRecovery = 0;                 // +0x142c: up 0.06 an update to 1 while +0x15c is 0 (never read)
	std::deque<TwistWidget*> mPendingDialogs;  // +0x23dc: the dialogs waiting to come up (0x6a6e36 its size)
	std::vector<FlyingText*> mFlyingTexts;  // +0x1f98: the praise and other words over the board (0x6a6d18 its size)
	std::vector<GemSoul*> mLevelSouls;      // +0x26b0: the souls of the last level's special gems (0x6a6eaa its size)
	// +0x1650: lights over the gems (0x2c bytes each), added by UpdateGame
	struct Light {
		float mX = 0;
		float mY = 0;
		float mSize = 0;
		float mFalloff = 0;
		float mLevel = 0;
		float mStrength = 0;
		bool mRising = false;
		float mRiseRate = 0;
		float mFadeRate = 0;
		Piece* mPiece = nullptr;  // +0x24: the gem it follows, or none
		int mPieceId = -1;        // +0x28
	};
	std::vector<Light> mLights;
	std::vector<Shockwave*> mShockwaves;    // +0x1660: the waves that shake the gems (0x6af497)
	std::vector<Traveller*> mSmokeTravellers;  // +0x2170: the points' smoke on its way to the tube (0x6af0e2)
	int mPuzzleFillWait = 0;                // +0x203c: the puzzle fill's wait, counted down each update
	// +0x21e4: objects of a class with its update at 0x6f465f (not yet ported)
	std::vector<void*> mDroppers;
	std::vector<Piece*> mTwistPieces;       // +0x20f4: the gems the twist turns (SnapPiecesX moves them too)
	Piece* mCursorPiece = nullptr;          // +0x1430: the gem under the cursor (RemovePiece forgets it)
	int mCursorPieceTicks = 0;              // +0x1434: counts every 8th update while the cursor has a gem (never read)
	bool mWaitDialogUp = false;             // +0x1438: dialog 40 is up; the next match's end closes it
	float mCursorMoveGlow = 1.0f;           // +0x1450: set to 1 when the cursor moves to another cell (never read)
	float mTwistAngle = 0;                    // +0x1454: the twist's angle in degrees, 0 to 90 (saved)
	float mTwistStep = 0;                   // +0x1458: the twist's step, in degrees an update
	float mTwistSpeedScale = 1.0f;          // +0x145c: scales the twist's speed (1; the tutorial can set it)
	int mSavedOnlyFirst = 0;                // +0x1464: the first of two ints saved after the twist's step; only saved and loaded
	int mSavedOnlySecond = 0;               // +0x1468: the second of two ints saved after the twist's step; only saved and loaded
	int mUpdatesPlayed = 0;                 // +0x1470: the updates the board has run (100 to a second of play)
	// (+0x1474: the trial's limit dialog, registration code, left out)
	bool mClearSparksDue = false;           // +0x1478: the next draw clears the spark starts and the hit counts
	int mReplacedGems = 0;                  // +0x1488: the gems that state 2 puts back on the board; the constructor sets 0 and nothing else writes it
	int mPlayStartCount = 0;                // +0x14dc: the update count when play (re)started after a pause; hundredths of a second since it add to +0x14e8
	int mLastTimerCount = 0;                // +0x14e0: the update count at the last play-time check
	int mSecondProgress = 0;                // +0x14e4: the updates counted toward the next second of play (a second is 1000 / frame time); a timed challenge adds it to its clock
	int mSecondsBeforePause = 0;            // +0x14e8: the seconds played before the last pause
	int mSecondsPlayed = 0;                 // +0x14ec: the seconds played in the game so far
	int mSecondsAddedToProfile = 0;         // +0x14f0: +0x14ec as it was when the profile's time total was last added to
	int mUpdatesSinceCascade = 0;           // +0x14f4: counted each update while the state is 0; 0 at each cascade and each twist (saved)
	int mShownScore = 0;                    // +0x14f8: the score on show, rolling up to +0x168
	int mProgressGain = 0;                  // +0x14fc: the progress points gained from matches, added with +0x200 (saved)
	int mGemsBrokenAtLastProfileSave = 0;   // +0x1500: +0x1e8 at the last time the profile's gem total was added to
	int mClearedAtGameEnd = 0;              // +0x1504: set to 0 when a game ends (never read)
	int mChainLength = 0;                   // +0x1508: the cascades of this turn (0 at its start); sizes the shockwave and the points
	bool mCascadePointsOn = true;           // +0x150c: cascades after the first score (true once the tips are on: profile flag 14); false before
	int mLastLandSound = 0;                 // +0x1510: the update count of the last gem-landing sound; they are at least 10 updates apart
	int mFallLimit = 0;                     // +0x1514: set to 50 each update before the gems fall (never read)
	float mShakeSize = 0;                   // +0x1518: the shake's size
	float mShakeAmount = 0;                 // +0x151c: the shake, fading to 0
	int mShakeOffsetX = 0;                  // +0x1520: this update's shake offset in x
	int mShakeOffsetY = 0;                  // +0x1524: this update's shake offset in y
	int mBurstPowerGems = 0;                // +0x1528: the power gems a burst reached; they set the size of the shake
	bool mBurstSoundDue = false;            // +0x152c: a burst began; the next update plays SOUND_BOMB_EXPLODE and clears it
	int mAchievementOneCount = 0;           // +0x1530: a count nothing increases; the difference to +0x1534 is added to the profile's counter that gives achievement bit 1 at 10
	int mAchievementOneCounted = 0;         // +0x1534: +0x1530 as it was when it was last added to the profile
	int mAchievementTwoCount = 0;           // +0x1538: a count nothing increases; the difference to +0x153c is added to the profile's counter that gives achievement bit 2 at 5
	int mAchievementTwoCounted = 0;         // +0x153c: +0x1538 as it was when it was last added to the profile
	int mTurnPoints = 0;                    // +0x1544: the points added in this turn, 0 at its start; the progress points follow a curve of it
	bool mTwistTried = true;                // +0x1558: set when a twist is tried (never read)
	bool mFirstLevelHints = true;           // +0x1559: the first levels' hints are on (true at a new game, false after the hyperspace of level 2)
	int mPauseCount = 0;                    // +0x1570: how many times the board is paused (Pause and Unpause)
	int mPauseShownCount = 0;               // +0x1574: how many of those pauses show the pause picture
	bool mBoardHidden = false;              // +0x15b0: never set; the board's gems are not drawn while set
	int mAutoClickX = 0;                    // +0x15bc: -autoplay's click, x
	int mAutoClickY = 0;                    // +0x15c0: -autoplay's click, y
	int mAutoClickTime = 0;                 // +0x15c4: the update count of -autoplay's click, 0 with none
	int mLastClickX = 0;                    // +0x15c8: the last click on the board, x (never read)
	int mLastClickY = 0;                    // +0x15cc: and y (never read)
	bool mClickedThisUpdate = false;        // +0x15d0: a click began a twist since the last update
	bool mClickedLastUpdate = false;        // +0x15d1: +0x15d0 of the update before (never read)
	int mClickBlockUpdates = 0;             // +0x15d4: updates left in which clicks do not count after an unpause; 1 on an unpause, the tube takes the mouse again at 0
	bool mFinishingTwist = false;           // +0x15d8: true while a twist is finished or turned back (never read)
	int mBoardImage = 0;                    // +0x15dc: an image of the whole board drawn instead of it; the constructor sets 0 and nothing else writes it
	bool mOverlayRedrawDue = true;          // +0x15fc: set at the end of a jump and a game, cleared by the next draw (never read)
	float mJumpScroll = 0;                  // +0x1614: scrolls down 0.1 an update (plus 5 when it wraps) while +0x1618 is above 0 (never read)
	double mJumpActive = 0;                 // +0x1618: 1 when a hyperspace jump is taken, 0 when it or a game ends
	double mJumpDistance = 0;               // +0x1620: falls by +0x1628 each update while +0x1618 is above 0 (never read)
	double mJumpSpeed = 0;                  // +0x1628: up 0.0005 each update while +0x1618 is above 0
	bool mJumpEnded = false;                // +0x1638: set to false when a game ends (never read)
	bool mNoDialogsOpen = false;            // +0x1644: the app has no dialog open, as of the last update (never read)
	bool mRainEffect = false;               // +0x1bf0: never set; when set, half the updates start a falling effect (effect 0x15) at a random x
	int mColorCount = 7;                    // +0x1bf4: the gem colours FillInBlanks picks from; never set after the constructor
	int mCursorX = 0;                       // +0x1c00: the cursor's x (its left edge)
	int mCursorY = 0;                       // +0x1c04: the cursor's y (its top edge)
	int mCursorWidth = 0;                   // +0x1c08: the cursor's width
	int mCursorHeight = 0;                  // +0x1c0c: the cursor's height
	int mHoverCol = 0;                      // +0x1c10: the column under the cursor at the last update
	int mHoverRow = 0;                      // +0x1c14: the row under the cursor at the last update
	int mCursorRestCount = 0;               // +0x1c18: the updates the cursor has stayed on one cell; its bob follows
	int mClickCursorX = 0;                  // +0x1c1c: the cursor's x when the last twist began (never read)
	int mClickCursorY = 0;                  // +0x1c20: and y (never read)
	int mClickCursorWidth = 0;              // +0x1c24: and width (never read)
	int mClickCursorHeight = 0;             // +0x1c28: and height (never read)
	int mCursorEaseSpeed = 0;               // +0x1c34: the speed of the cursor's ease
	float mCursorEaseProgress = 1.0f;       // +0x1c38: 0 to 1, up 0.25 an update after the cursor jumps to a cell; the ease is quickest at the middle
	int mCursorMoveCount = 0;               // +0x1c3c: the update count of the last time the cursor moved onto a new cell
	bool mTwistMatches = false;             // +0x1c40: a twist at the cursor would make a match (TestTwist, after each cursor move)
	bool mTwistBlocked = true;              // +0x1c41: the cursor's twist is not allowed (CannotTwist)
	bool mTurnStartedWithMatch = false;     // +0x1c42: +0x1c40 as the turn began
	float mHintFade = 0;                      // +0x1c48: counts down 0.01 an update to 0; nothing sets it above 0
	int mModeToUnlock = -1;                 // +0x1c64: the mode the turn's end unlocks (UnlockMode), -1 for none
	float mSpeedAccum = 0;                    // +0x1c84: the updates owed by the game speed's fraction, carried between frames
	float mSpeedTarget = 1.0f;              // +0x1c88: the speed the game runs at, which +0x1c8c eases to (1 after a tip)
	float mSpeedNow = 1.0f;                 // +0x1c8c: the speed the game runs at now; below 0 the twist is made at once
	bool mFallHeld = false;                 // +0x1c90: the gems do not fall while set (set at the end of a level jump, cleared when play goes on)
	bool mIceGemsAllowed = false;           // +0x1c98: the app's IceGemsAllowed setting
	int mCoalLevel = 0;                     // +0x1f64: the most coal gems seen at once, decaying (saved); it limits the coal FillInBlanks makes
	int mCoalDecayCount = 0;                // +0x1f68: the turns toward the next decay of +0x1f64 (saved)
	int mPowerGemCount = 0;                 // +0x1f74: the power gems on the board, counted each update while the state is 0 (never read)
	int mElectroGemCount = 0;               // +0x1f7c: the electro gems on the board, counted each update while the state is 0 (never read)
	int mLastPraiseLevel = -1;              // +0x1f80: the praise shown last (-1 at each twist)
	int mPendingPraise = -1;                // +0x1f84: a praise level waiting for another voice to fade, -1 for none
	Image* mBackdrop = nullptr;             // +0x1fa8: imagesackdrops	wist-back<n>.png, which the game does not have
	float mCursorFadeIn = 0;                  // +0x1fac: the cursor's opacity, up 0.01 an update to 1
	float mCursorTurn = 0;                    // +0x1fb0: the cursor's turning angle, kept inside one circle
	int mTurnCount = 0;                     // +0x1fb8: the turns (twists) made in the game (saved)
	int mTurnSettled = 0;                   // +0x1fc4: +0x1fb8 when the board last settled (saved)
	int mMatchCenterX = 0;                  // +0x1fe8: the matched gems' mean x (FindMatches)
	int mMatchCenterY = 0;                  // +0x1fec: and y
	int mMatchGemCount = 0;                 // +0x1ff0: their count
	bool mEndsWhenIdle = false;             // +0x1ff4: never set; when set, +0x1ff5 follows once no dialog waits
	bool mGameOverShown = false;            // +0x1ff5: the game's end has been shown; the board stops updating and is not drawn
	int mSparkleStep = 0;                   // +0x1ff8: 0 to 7, up every 16 updates
	int mGameOverTicks = 0;                 // +0x1ffc: the updates of the nuke's shake in UpdateGameOver, from 0x41
	int mGameOverShakeTicks = 0;            // +0x2000: counts once +0x1ffc is above 0x46; the shake runs for 199
	bool mCounterHintDue = false;           // +0x2004: a counter gem was just made; the hint comes after 3 seconds
	int mMatchDelay = 0;                    // +0x2008: the updates left until MatchAfterMove runs after a twist (10 at its end)
	Hyperspace* mHyperspace = nullptr;      // +0x200c: the space behind the board, during a jump
	int mLowestCounter = 0;                 // +0x2010: the lowest count left on a counter gem on the board
	bool mGlowGrowing = false;              // +0x2014: never set; the update's glow step is +0.005 when set, -0.005 otherwise
	bool mTwistClockwise = false;           // +0x2015: the twist turns the gems clockwise (a quarter turn one way or the other)
	bool mMatchCheckDue = false;            // +0x2016: a twist ended and MatchAfterMove has not yet run
	int mMatchesInTurn = 0;                 // +0x2018: the matches made in this turn, 0 at its start
	int mBestMoveRow = -1;                  // +0x201c: the row of the move FindMove found at the last turn's end
	int mBestMoveCol = -1;                  // +0x2020: and its column
	bool mEditorMode = false;               // +0x2030: never set; when set the level editor's text shows and the cursor is off
	bool mHyperspaceClicked = true;         // +0x2040: set when the hyperspace is clicked away (never read)
	bool mLevelStarted = true;              // +0x2041: the level is under way (set when it starts, saved); points count and the gauge fills only then
	bool mLevelPrepared = true;             // +0x2042: set when the level's board is made (never read)
	bool mBoardShown = true;                // +0x2043: the board and its back are drawn
	bool mPuzzleBoardMade = false;          // +0x2054: set when the puzzle's board is made
	float mGlowRing[24] = {};               // +0x2058: 24 glows that run round (UpdateGlows)
	float mRingBurst = 0;                     // +0x20b8: a ring that grows and fades, 1 down to 0 by 0.01 an update
	int mGlowStep = -1;                     // +0x20bc: the glow of +0x2058 lit now, 0 to 23
	float mCursorTurnNow = 0;               // +0x20c0: the cursor's turn while twisting
	float mCursorTurnBase = 0;              // +0x20c4: the cursor's turn when the last turn began
	float mTwistTrailFade = 0;                // +0x20c8: 1 while a twist is turning, then down 0.01 an update to 0
	float mCursorAlpha = 0;                   // +0x20cc: the cursor's opacity, 0 to 1; the board's glows also scale by it
	bool mCursorShown = true;               // +0x20d0: the cursor is shown (set by the first click or move)
	float mCursorTwistFade = 0;             // +0x20d4: the twisting cursor's opacity, up 0.05 while a twist turns
	bool mCursorOnGrid = false;             // +0x20d8: the cursor is inside the grid
	int mClearedOnHyperspaceStart = 0;      // +0x20e4: set to 0 at the start of a jump, a start and a restart from the menu (never read)
	float mBoardGlowSize = 0.1f;            // +0x20e8: never set; the board's glow is drawn while above 0.001
	float mProgressHold = 0;                // +0x20ec: holds the progress bar back: 4 when a jump is taken in the first level, down 0.01 an update
	bool mInstantTwist = false;             // +0x210c: twists are made at once, 90 degrees a step
	std::vector<std::pair<int, int>> mUnturnableCells;  // +0x2120: the cursor's cells that cannot turn (row, column)
	bool mTurnEndSoundDue = false;          // +0x2130: set when a twist begins; the turn's end plays its sound once
	float mWhiteFlash = 0;                    // +0x2150: a white fill over the board, its opacity, down 0.01 an update
	int mMusicTrack = 0;                    // +0x2154: the in-game track playing, as an index counted from track 100
	int mChainFlashSet = -1;                // +0x215c: set to -1 when a chain's flash begins (never read)
	int mChainFlashKind = 1;                // +0x2160: set to 1 when a chain's flash begins (never read)
	float mChainFlash = 0;                    // +0x2164: 1 when a chain adds up, down 0.01 an update (never read)
	bool mChainAdded = false;               // +0x216c: true when a chain was added in this turn (never read)
	bool mColorGemBroke = false;            // +0x218c: a gem of a plain colour broke in this turn (never read)
	bool mBombsMayCountDown = false;        // +0x2198: the locks and counters may count down (true as a twist starts, cleared once CountDownBombs has run) (0x6aa0b7)
	bool mFallMatchDue = false;             // +0x2199: a gem was seen falling; MatchAfterMove runs when all are still
	int mLandingBlock = 0;                  // +0x219c: never set; the landing check waits while above 0
	bool mClicksIgnored = false;            // +0x21a0: never set; clicks do not start twists when set
	bool mGameLoaded = false;               // +0x21a2: set when a saved game finishes loading; cleared by a jump or a new start
	bool mLevelDone = false;                // +0x21a3: the level's goal is met and the level's end is under way
	Widget* mExtraWidget = nullptr;                   // +0x21a8: never set; a widget updated and drawn over the board when present
	SoundInstance* mGlowSounds[2] = {};     // +0x21c4: never set; UpdateGlows stops them in turn
	int mGlowSoundIndex = 0;                // +0x21cc: which of the two +0x21c4 sounds is next (0 or 1)
	int mStepLevel = 1;                     // +0x21d0: the level the steps count has reached, 1 to 10 (saved); it multiplies points
	int mModeMultiplier = 1;                // +0x21d4
	bool mHintShowing = false;              // +0x21d8: a first-level hint is showing
	bool mTwisting = false;                 // +0x21d9: a twist is turning the gems
	bool mSettling = false;                 // +0x21da: a match was made and the gems have not yet dropped and filled (saved)
	int mWheelSpinCount = 0;                // +0x21dc: how many times the wheel has been spun (saved)
	int mFirstMatchColor = -1;              // +0x21f4: the colour of the turn's first match, -1 at its start
	bool mSkipMouseClick = false;           // +0x2200: the next mouse click is skipped (the original's debug text calls it mSkipMouseClick)
	TwistWidget* mShownDialog = nullptr;    // +0x2208: the dialog over the board (taken from the queue at +0x23dc)
	float mChainShine = 0;                  // +0x2228: 0.5 with each chain, 0 when a praise is worked out
	int mPraiseUpdate = 0;                  // +0x222c: the update count at the last praise
	int mGameStats[84] = {};                // +0x2234: the game's own statistic counters (AddStat), also kept by the app
	Buffer mSaveBuffer;                     // +0x2384: the game as StartLevel saved it (0x6b0d91)
	Buffer mReplaySave;                     // +0x23b4: the game saved at the replay's start (ReplayWidget saves and restores)
	int mReplayStartFrame = -1;             // +0x23a4: the first frame of the last turn's recording
	int mReplayEndFrame = -1;               // +0x23a8: the last frame of the last turn's recording
	int mSavedRankPoints = 0;               // +0x23ac: the profile's rank points as a turn began (ReplayWidget puts them back)
	int mSavedRank = 0;                     // +0x23b0: the profile's rank as a turn began (ReplayWidget puts it back)
	int mClearedAtStart = 0;                // +0x23d4: set to 0 when a game starts (never read)
	float mMatchFlashDecay = 0.01f;         // +0x23f0: how fast +0x23ec falls in a twist; 0.015 once a match is seen
	bool mTipShowingAtJump = false;         // +0x23f4: +0x23f5 as the level jump began (the end shows read it)
	float mTipArrowFade = 0;                // +0x23f8: the tutorial arrow's fade
	float mCursorGlow = 0;                  // +0x23fc: the cursor's glow
	int mTipCol = -10000;                   // +0x2400: the column the tip points the cursor at, -1 for none
	int mTipRow = -10000;                   // +0x2404: and the row, -1 for none
	std::vector<int> mTipGemsByColumn[20];  // +0x2408: the tutorial's gems for each column, the next at the back
	int mTipBombCount = -1;                 // +0x2548: the count a new gem gets while a tip shows (the tutorial sets it, 15 at first)
	Buffer mTipScript;                      // +0x254c: the tutorial script's lines (properties/tutorial.txt, read by ShowNextTip)
	std::map<int, int> mWaitingTips;        // +0x256c: the tips waiting for their dialog, by kind, with a gem's id
	int mReturnDialogWait = 0;              // +0x2578: updates until the "returning to game" dialog after the zen jump (400)
	int mZenJumpWait = 0;                   // +0x257c: updates until the zen jump begins
	int mMatchSetCount = 1;                 // +0x2580: the sets in the match being scored (2 for a double set)
	bool mBoardSettled = false;             // +0x25a5: nothing is moving or waiting: set each update from the other states
	bool mShownBeforeStart = false;         // +0x25a6: set when the tube is shown before the level has started
	int mSettledUpdates = 0;                // +0x25a8: counts the updates the board is settled and not frozen
	int mHintState = 0;                     // +0x25c8: 0 no hint, 1 the hint is being shown
	int mHintsShown = 0;                    // +0x25cc: the hints shown in this turn
	float mHintWait = 0;                    // +0x25d0: the wait for a hint, in seconds
	float mHintSparkleTurn = 0;             // +0x25d4: the hint's sparkles' turn
	int mHintRow = -1;                      // +0x25d8: the hint's row (FindMove)
	int mHintCol = -1;                      // +0x25dc: the hint's column
	ReplayWidget* mReplayPod = nullptr;     // +0x2624: the pod with the replay button (ShowReplayPod)
	bool mReplayPodDown = false;            // +0x2628: the level's end took the pod down; the rank bar's end shows it again
	bool mReplayPlaying = false;            // +0x2629: a replay plays (StartReplay to the move's end)
	bool mFruitBrokeThisTurn = false;       // +0x262a: a fruit's blast began in this turn (0 at its start); it also marks the turn as worth a replay
	int mLowestBombCount = -1;              // +0x262c: the lowest count left on a bomb or devil gem on the board, 100 with none
	Challenge* mChallenge = nullptr;        // +0x2630: the challenge being played (mode 2), the app's ChallengeInfo owns it
	int mChallengeBest = 0;                 // +0x2634: the best of the challenge's counted stat in this game (saved)
	int mChallengeCount = 0;                // +0x2638: the stat the challenge counts (saved)
	int mBlitzSecondsShown = 0;             // +0x263c: the seconds left as last shown by the timed challenge
	int mBonusChallengesMet = 0;            // +0x2640: the bonus challenges met in the game (saved); it adds to the next one's points
	int mBonusStage = 0;                    // +0x265c: a fruit broke (1 or 2), its bonus challenge (3), met: the fruit dance (4), missed (5)
	int mBonusMatched = 0;                  // +0x2660: the bonus challenge's gems matched so far (saved)
	int mBonusMatchedMark = 0;              // +0x2664: the matched count at which the fruit's count may start again, -1 at the bonus challenge's start
	bool mChallengeMet = false;             // +0x2669: the challenge's goal is reached and waits for the board to settle
	bool mChallengeOver = false;            // +0x266a: the challenge is over; the last check keeps the stats
	int mChallengeNumber = 0;               // +0x2678: the challenge's number within its category
	PopAnim* mFruitDance = nullptr;         // +0x267c: the fruit dance (fruitdance.pam)
	int mFruitDanceUpdates = 0;             // +0x2680: its updates, to 2200
	int mFruitDanceSmallPoints = 0;         // +0x2684: its "small" points so far (PopAnimCommand)
	int mFruitDanceLargePoints = 0;         // +0x2688: its "large" points
	bool mTubeAboveMiddle = true;           // +0x2698: the progress bar's end is above the grid's middle
	int mBonusLevel = 0;                    // +0x269c: the rank bonus the level's end gives (GetBonusLevel)
	TwistWidget* mRankBonusWidget = nullptr;               // +0x26a0: the rank bar (RankBonusWidget) shown at the level's end
	float mSoulFade = 0;                    // +0x26a4: 0.01 when a soul leaves (UpdateJump), 0 at the level's start (never read)
	int mJumpStage = 0;                     // +0x26a8: the jump between levels (UpdateJump): 1 the show, 2 the rows, 4, 0
	TwistWidget* mLevelUpShow = nullptr;                   // +0x26ac: the LevelUpShow (ShowLevelUp)
	int mGameOverRow = 0;                   // +0x26c0: the row falling off next (end state 3)
	float mEndStateWait = 0;                // +0x26c4: the end states' wait
	int mEndState = 0;                      // +0x26c8: the end of the game's state (UpdateGameOver), 1 from EndGame
	Piece* mEndingPiece = nullptr;          // +0x26cc: the gem that ended the game (a counter at zero)
	TwistWidget* mGameOverShow = nullptr;                  // +0x26d0: the game-over show (GameOverShow) over the board
	int mRecordedFrames = 0;                // +0x3444: the frames recorded for the last move's replay (0 at a twist's start)
	int mRewindFrame = 0;                   // +0x3448: the frame a replay's rewind has reached, counting down
	int mOverflowLevelCount = 0;            // +0x344c: how many times the steps passed twice the last level's size
	int mGeodeBonusCount = 0;               // +0x3450: the geodes made in this turn; each adds 50 more points to the next (250 at most)
	float mGeodeFlash = 0;                    // +0x3454: 1 when geodes are made, down 0.005 an update
	float mStateWait = 0;                   // +0x345c: the wait of states 4 and 5, down 0.01 an update
	float mNukeZoom = 8.0f;                 // +0x3460: the zoom of the nuke's view, 8 at the end of the game and down by 0.1 an update
	TextImage* mFinalScoreLabel = nullptr;  // +0x3464: blitz's FINAL SCORE (SetEndState 4)
	TextImage* mFinalScoreText = nullptr;   // +0x3468: and its score
	float mFinalScoreFade = 0;              // +0x346c: their fade (end states 4 to 8)
	float mFinalScoreWait = 0;              // +0x3470: end state 4's wait
	int mTipKind = 0;                       // +0x34e8: the kind of the tip shown (its profile flag is cleared if the tutorial is left)
	int mEndPending = 0;                    // +0x34ec: 1 once the game's end waits for the dialog or show to finish
	int mGemCounts[84] = {};                // +0x3560: how many gems of each kind the board has now (CountGems)
	bool mGameOver = false;                 // +0x36b0: the game is over (set once; a challenge met also sets it)
	bool mEndedByNuke = true;               // +0x36b1: the game ended with the nuke
	int mCountPopupCount = 0;               // +0x36b4: the gem count the popup shows
	int mCascadePopupCount = 0;             // +0x36b8: the cascade length the popup shows
	float mCascadePopupShadow = 0;            // +0x37a0: the cascade popup's shadow opacity, 1 when it starts
	float mCountPopupShadow = 0;              // +0x37a4: the count popup's shadow opacity, 1 when it starts
	float mCascadePopupFade = 0;              // +0x37a8: the cascade popup's opacity, down 0.01 an update
	float mCountPopupFade = 0;                // +0x37ac: the count popup's opacity, down 0.01 an update
	bool mCountPopupOn = true;  // +0x37b0: the count popup may show (not in the challenge)
	bool mCascadePopupOn = true;  // +0x37b1: the cascade popup may show (not in the challenge)
	int mCountPopupMin = 15;                // +0x37b4: the count popup shows from this many gems (15)
	int mReplaySlideWait = 0;               // +0x37c8: updates a replay waits before the board slides away (100 when it begins)
	float mBoardOffsetX = 0;                // +0x37ec: the board's drawing offset in x
	float mBoardOffsetY = 0;                // +0x37f0: and in y
	float mSettleWait = 0;                    // +0x37f4: the wait after the board settles before the turn ends (0.15 on a level-up, 1.5 for the next level)
	float mTileRingWait = 0;                  // +0x1630: the wait to the next ring of tiles
	int mTileRingDistance = 0;                // +0x1634: the ring's distance from the middle
	int mLastSoulLandUpdate = 0;            // +0x37fc: the update count of the last gem-soul landing sound
	TwistWidget* mHeldDialog = nullptr;                    // +0x3808: never set; a dialog that holds the level jump while the board is busy
	TextImage* mMinuteWarningFront = nullptr;  // +0x38f0: blitz's minute mark, the front of its warning (drawn black)
	TextImage* mMinuteWarningGlow = nullptr;  // +0x38f4: the same, the glow behind it
	float mMinuteWarningAlpha = 0;            // +0x38f8: the opacity of blitz's minute warning
	float mMinuteWarningRed = 0;              // +0x38fc: the warning's red level
	float mMinuteWarningWait = 0;             // +0x3900: the warning's wait, 5 when it starts, down 0.01 an update
	bool mGeodesPending = false;            // +0x3905: geodes are flying; the replay pod waits until they are gone
	int mBestMoveChain = 0;                 // +0x3908: the chain of the best move so far (saved)
	int mBestMoveGems = 0;                  // +0x390c: the gems of the best move so far (saved)
	int mBestMovePoints = 0;                // +0x3910: the points of the best move so far (saved)
	int mSeenFlags = 0;                     // +0x3914: the profile's flags +0x1e4 (tips and hints seen) as kept for the game (saved)
	int mUnlockFlags = 0;                   // +0x3918: the profile's flags +0x1e0 as kept for the game (saved)
	float mReplayStaticFade = 0;              // +0x391c: the opacity of the static-space picture behind a replay, up to 1 and back to 0
	bool mReplayMoveReady = false;          // +0x3920: the turn made a move worth a replay (a long chain, many gems or a high score)
	bool mReplayWaiting = false;            // +0x3921: +0x3920 and the board is settled, with the score shown; the replay may start
	int mMusicPositionLow = 0;              // +0x3928: the low half of the music's saved position (saved; PlayMusic is given it)
	int mMusicPositionHigh = 0;             // +0x392c: the high half of the music's saved position (saved)
	bool mEndedByDevil = false;             // +0x3930: the gem that ended the game was a devil gem; it picks the game-over sound
	// The fields the constructor sets from other values
	int mTwistSpeedGrade = 0;               // +0x154: 1 in the challenge, else 0 (saved in the header); it adds to the twist's speed
	bool mFruitDue = false;                 // +0x214: a fruit is due once the board settles (AwardFruit)
	float mSavedOnlyFraction = 1.0f;        // +0x220: 1 at first, saved as ten-thousandths; nothing reads it
	int mSavedBoolsSet = 0;                   // +0x1328: how many of the profile's mSavedBools are true; never read
	float mColumnLift[8] = {};                // +0x133c: a lift for each column's gems, set by a break and eased back to 0 each update
	Font* mScoreFont = nullptr;             // +0x1578: FONT_SCORE, set by the constructor (never read)
	Font* mScoreFontCopy = nullptr;         // +0x157c: FONT_SCORE again, set by the constructor (never read)
	MemoryImage* mSquareImage = nullptr;    // +0x15f0: a 42x42 memory image the constructor makes (never drawn into or read)
	// +0x1674: a 7x7 table over a cell, 14 apart from 0 to 84
	struct CellPoint {
		float mX, mY;      // +0x0, +0x4
		float mU, mV;      // +0x8: mX / 84 / 40, +0xc: mY / 84
		float mAngle;      // +0x10: from the middle (42, 42), plus 6.28318
		float mDist;       // +0x14: from the middle
		float mScale;      // +0x18: the point's pulse scale
	};
	CellPoint mCellPoints[49] = {};
	float mEasedCursorX = 0;                // +0x1c2c: the cursor drawn, x, easing to +0x1c00
	float mEasedCursorY = 0;                // +0x1c30: the cursor drawn, y, easing to +0x1c04
	Color mFillColor;                       // +0x1f88: filled over the board while its alpha (+0x1f94) counts down
	int mTurnCountedForStats = 0;           // +0x1fbc: +0x1fb8 when the turn's stats were counted
	int mTurnCountedForSpeed = 0;           // +0x1fc0: +0x1fb8 when the turn's speed bonus was last decided
	float mTubeOffsetX = 0;                 // +0x20e0: the tube's offset in x: half the app's width less an image's offset (menus set it to 0)
	BoardStepper* mStepper = nullptr;       // +0x211c: the BoardStepper (0x74c8c8; freed by the destructor at 0x6bf0bb)
	int mTurnOfLastSound = 0;               // +0x2190: +0x1fb8 when the turn's sound was last played
	GoodTube* mGoodTube = nullptr;          // +0x2194: the tube at the side
	EffectsManager* mFXManager3 = nullptr;  // +0x21b8
	float mGlowSoundWait = 0;               // +0x21bc: random wait before the next glow sound, down 0.01 an update
	float mGlowSoundRepeat = 0;             // +0x21c0: the wait of the glow's repeat, 4 when it runs out
	int mIntroRoll = 0;                     // +0x21e0: 1 to 5, mostly 2 (3 in the first game); a roll below 3 sends a first classic game to the zen jump
	PointsManager* mPointsManager = nullptr;  // +0x21fc: the points that float up
	float mJumpLoadFade = 0;                // +0x2220: only LoadGame's end of a hyperspace sets it (to 0); never read
	float mAlertGlow = 0;                   // +0x2224: 0.2 when a low count shows, fading 0.015 an update
	float mMoveBlockWait = 0;               // +0x23d8: while above 0 (down 0.01 an update) the board is not ready for moves
	Image* mGridBarTop = nullptr;           // +0x2588
	Image* mGridBarBot = nullptr;           // +0x258c
	int mQuickMoveGap = 0;                  // +0x25ac: the gap before the last quick move
	int mQuickMoveTime = 0;                 // +0x25b0: +0x25a8 at the last quick move
	int mQuickMoveCount = 0;                // +0x25b4: the quick moves made one after another (0 resets); every 6 raise the speed bonus
	std::vector<int> mHintGems;             // +0x25b8: the gems of the move FindMove found
	MatchInfo mMoveMatchInfo;               // +0x25e0: FindMove's match for each cell
	std::vector<MatchInfo> mTurnMatches;    // +0x2614: the matches made
	int mBonusGemCount = 0;                 // +0x2644: the gems the bonus challenge asks for (saved)
	int mBonusChallengeMinGems = 0;         // +0x266c
	int mChallengeCategory = 0;             // +0x2674: the challenge's category (mode 2), -1 with none; part of the save's name
	int mBlitzStartTime = 0;                // +0x3458: blitz's 30000 (hundredths of a second), the time the tube is set to
	std::map<int, int> mSpecialKindBySerial;        // +0x37b8: the kind of special gem (-0x68 to -0x65) each new serial stands for
	MTRand mRand;                           // +0x958: seeded from Rand()
	MTRand mReplayRand;                     // +0x2a80: the random generator for the replay's rewind lines, seeded from the update count while +0x2629 is set
	// Read by ReadConfig (0x6c6469)
	int mGemColorCount = 0;                 // +0x1bfc: 7, set by ReadConfig (never read)
	bool mBadTwistsAllowed = false;         // +0x1bf8: true; a twist that makes no match does not set the move back at once
	int mMatchLength = 0;                   // +0x1c68: the gems in a row that make a match (3; ReadConfig)
	bool mBombsCanMove = false;             // +0x1c6c: bomb gems may be twisted (true; ReadConfig)
	bool mMatchAtOnce = false;              // +0x1c6d: false; when set MatchAfterMove runs at once after a twist
	bool mCountersOn = false;               // +0x1c6e: counters may be added to gems (true; ReadConfig)
	bool mDelayMatchAfterMove = false;      // +0x1c6f: false; when set MatchAfterMove waits 10 updates after a twist
	bool mTwistOverEmpty = false;           // +0x1c71: false; with +0x2054, twists may reach empty cells
	bool mDropperGemsMove = false;            // +0x1c78: gems with FLAG_DROPPER may be twisted (true; ReadConfig)
	bool mTurnSoundsOn = false;             // +0x1c79: true; the turn's end plays its sound once per turn
	bool mLockedGemsBusy = false;           // +0x1c7b: false; when set FLAG_DROPPER and locked gems are made busy after a twist
	int mLaserGemCascades = 0;              // +0x1c80
	int mFruitFirstLevel = 0;               // +0x1c94
	int mAmnestyGemCount = 0;               // +0x1c9c
	int mCoalMax = 0;                       // +0x1f60
	int mBonusRoundLevelGap = 0;            // +0x2104
	bool mFruitsBlast = false;              // +0x2138: false; when set fruit gems are blasted with the breaking gems
	std::vector<int> mMatchIds;             // +0x2140: cleared by FindMatches
	int mTimeFreezeMatchCount = 0;          // +0x21b4
	int mBonusChallengeMaxGems = 0;         // +0x2670
	int mMaxLocksOnScreen = 0;              // +0x3800
	std::vector<Geode*> mGeodes;            // +0x14ac: the geodes in flight (the original frees them with free(), skipping their destructors)
	int mFrozenGemCount = 0;                // +0x2180: the gems that cannot move (counted at a turn's end)
	int mFrozenSpecialCount = 0;            // +0x2184: the bombs, devils and FLAG_DROPPER gems among them
	int mLevelsFinished = 0;                // +0x2204: counts the levels finished while +0x2201 is set (never read)
	// Set by NewLevel (0x6bf659) and the level's progress
	bool mClearedOnNewLevel = false;        // +0x164: false at each new level, saved; never read
	int mLevelStartScore = 0;               // +0x208: the score when the level began (saved)
	int mLevelRecords[2] = {};              // +0x228: [0] the level's longest chain, [1] zeroed with it; both saved
	int mLongestChain = 0;                  // +0x238: the longest chain (+0x1508) of the game (saved)
	int mTwistsMade = 0;                    // +0x23c: the twists made in the level
	int mBadTwists = 0;                     // +0x240: the twists that made no match
	int mChainsMade = 0;                    // +0x244: the chains made
	int mCounterFlash = 0;                  // +0x1c44: 255 when the counters count down, 2 less each update
	bool mDevilChecked = false;             // +0x1f6c: the level's devil gem has been made (saved); false at a new level
	int mLevelColor = 0;                    // +0x1f70: the level modulo 7 (saved)
	float mScoreRollGlow = 0;                 // +0x20f0: 1 when the shown score rolls, down 0.02 an update
	bool mCountersAllowed = false;          // +0x21a1: counter gems may be added (CanAddCounter): past the first level and within the amnesty count
	int mLevelStartMark = 0;                // +0x21f8: 100 at a level's start (0x6a3ff4); saved and never read
	bool mLevelCounts = false;              // +0x2201: true at a new level; cleared when the steps run on
	int mBarWidth = 0;                      // +0x2590: the grid's width and a quarter of a cell (0x6a49a3); the width of the bars over and under it
	float mBarEndY = 1.0f;                  // +0x2594: the progress bar's end in y; it follows the grid's middle
	float mBarGlow = 0;                     // +0x25a0: the progress bar's glow
	float mBarTopAlpha = 1.0f;              // +0x2598: the opacity of the bar over the grid
	float mBarBottomAlpha = 1.0f;           // +0x259c: the opacity of the bar under the grid
	std::vector<int> mBonusColors;          // +0x2648: the bonus challenge's colours (0x6bbfe4)
	int mBonusPoints = 0;                   // +0x2658: the bonus challenge's points (saved)
	bool mGoalCheckDue = false;             // +0x2668: true as a twist starts; the turn's end checks the goal once
	CurvedVal mProgressCurve;               // +0x178: classic's progress curve (UpdateProgress)
	std::vector<SexyString> mTips;          // +0x81c: properties\tips.txt, a line each

	Board(WinTwistApp* theApp, int theMode, int theFlags, int theChallengeCategory);  // 0x6c4df9
	~Board() override;

	void LoadConfig();                           // 0x6b2b77
	// 0x6c6469: the board's size, the level values and the rest of the config's numbers; theResize remakes the
	// grid when its size changed
	void ReadConfig(bool theResize);
	void ResizeBoard(int theRows, int theCols);  // 0x6c427e
	void ClearBoard();                           // 0x6afe3a
	void AllocGrid(int theRows, int theCols);    // 0x6b0886
	void MakeCells();                            // 0x6a3ea2
	void RemovePiece(Piece* thePiece, bool theDelete);  // 0x6aa076
	void NewLevel();                             // 0x6bf659
	void SetStat(int theStat, int theValue);     // 0x6a44e5: the game's stat and the app's copy (unless +0x37c4)
	void ResetLevelValues();                     // 0x6a3f77: the level values that start at this level
	void UpdateTubeProgress(bool theAtOnce);     // 0x6a434d
	// 0x6bd78e: the tube, whether the level is done and classic's curve; theNoCheck skips 0x6bbbc2
	void UpdateProgress(bool theNoCheck);
	bool CheckLevelDone();                       // 0x6ab195
	int GetBonusLevel(int theLevel) { return mGameMode == 4 ? mLevelProgressGoal / 2000 : theLevel; }  // 0x6a6618
	void StartBonusChallenge(int theExcludeColor);  // 0x6bbfe4: the colours of the next bonus challenge
	// 0x6ba5b4: a fruit for the gem at theRow, theCol, or (theRow below 0) a gem picked for it: the award screen
	// (FruitAwardScreen) joins the dialog queue
	void AwardFruit(int theRow, int theCol);
	// 0x6c3cf5 (PopAnimListener slot 1): the fruit dance's "small" and "large" points
	void PopAnimCommand(int theId, const std::string& theCommand, const std::string& theParam) override;

	// 0x6ca727: the game in theBuffer again, with the cursor at theRow, theCol, and its move made again (+0x2629):
	// thePlayback first rewinds the recorded frames of the move (state 1), else the move starts at once
	void StartReplay(Buffer& theBuffer, int theRow, int theCol, bool thePlayback);
	// 0x6c9d0b: a new game, or the saved one unless the menu's play button resumes the game on the screen
	void StartGame();
	// 0x6c8e99: the game saved in theBuffer; false for none (port: the reading of a save is not ported yet)
	bool LoadGame(Buffer& theBuffer);
	// 0x6a6529: a save's version, and from version 60 its mode, flags, +0x154 and level; false for an older or empty one
	static bool ReadSaveHeader(Buffer& theBuffer, int* theVersion, int* theMode, int* theFlags, int* theTwistSpeedGrade,
		int* theLevel);
	void SetScore(int theScore, bool theAtOnce);  // 0x6bd840
	// 0x6a5fa3: the board takes theHyperspace (a new one if null) and sets up the jump into the game
	void TakeHyperspace(Hyperspace* theHyperspace);
	// 0x6c77b1: after StartGame: the config again, the basics tutorial or the puzzle's board, the save and the tube
	void StartLevel();
	void UpdateChallengeTube(bool theAtOnce);    // 0x6af390: the tube shows how much of the challenge is left
	void UpdateChallengeTimer();                 // 0x6bedd8: a time limit's clock on the tube, with blitz's marks
	// 0x6c7c7b: the challenge to play (null for none): its config, the level values that start at 1, the tube and the
	// picture behind the board
	void SetChallenge(Challenge* theChallenge);
	void SetBlitzColor();                        // 0x6bd1f0: the tube's colour by blitz's time left
	void ShowBlitzTime(int theSeconds);          // 0x6bea4b: blitz's marks, 30 seconds, 9 to 1 and each minute left
	// 0x6ba0c2: theText grows in the middle of the board; theClear first ends the words already there
	void ShowWords(const SexyString& theText, bool theQueued, bool theClear);

	int GetSlideX() { return Ftol((double)mWidth * (double)0.635f - (double)((mCols * gGemWidth) / 2)); }  // 0x6a6afa
	void SnapPiecesX();                          // 0x6ab551: every piece to its column's x
	int GetRowAt(int theY);                      // 0x6a4888: -1 off the grid
	int GetColAt(int theX);                      // 0x6a4849: -1 off the grid
	void SetCursor(int theRow, int theCol);      // 0x6a9d2d: the selection's rectangle, two gems square
	// 0x6bd9ac: the cursor to theRow, theCol, with its sound, trail and the moves it shows
	void MoveCursor(int theRow, int theCol, bool theQuiet, MatchInfo* theInfo);
	// The twist (BoardTwist.cpp)
	void MouseMove(int x, int y) override;                       // 0x6a47ba
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;  // 0x6c7232
	void MouseUp(int x, int y, int theClickCount) override;      // 0x6a52c1
	void MouseDrag(int x, int y) override;                       // 0x6a52cb
	void MouseWheel(int theDelta) override;                      // 0x6a52d7
	void KeyDown(KeyCode theKey) override;                       // 0x6ca439: pause, the wheel, the cursor's keys
	void WarpToCursor();  // 0x6a45da: the mouse pointer onto the cursor's gem while +0x1430 holds a piece
	void UpdateCursor(int x, int y);                             // 0x6bdbd9 (slot 70, the board's own)
	void StartTwist(bool theInstant, int theX, int theY);        // 0x6c6e56
	void SetupTwist(int theX, int theY);                         // 0x6bb171: the four gems at +0x20f4
	bool StartPieceMove(Piece* thePiece, int theDCol, int theDRow);  // 0x6a9d6f
	void FinishTwist(bool theStop);                              // 0x6bba82: the turned gems into their cells
	void CountDownBombs(bool theInstant);                        // 0x6aa0b7
	void LockPiece(Piece* thePiece);                             // 0x6a4e78
	void UnturnPieces();                                         // 0x6a5424
	void ClearTurning();                                         // 0x6a54cb
	void UpdateTwist();                                          // 0x6cfd85
	void EvaluateCursor(MatchInfo* theInfo);     // 0x6bd872: +0x1c40 a match, +0x1c41 the turn blocked
	void ClearCellHints();                       // 0x6a4764
	bool TestTwist(bool theTest, MatchInfo* theInfo);  // 0x6bb619: whether turning the cursor's 2x2 matches
	bool CannotTwist(bool theCheck);             // 0x6bb4b5: the cursor's 2x2 cannot turn (its cells in +0x2120)
	bool MatchAt(int theCol, int theRow, MatchInfo* theInfo);  // 0x6ba1b2
	bool MatchAny(MatchInfo* theInfo);           // 0x6ba4ab
	void SaveGrid();                             // 0x6a3b00
	void RestoreGrid();                          // 0x6a3b26
	bool FindMove(int* theRow, int* theCol, bool theAny);  // 0x6c14fc
	int FillPieces(bool theAtOnce);              // 0x6c25a4: new gems for the gaps; returns how many
	bool CanAddCounter();                        // 0x6a63c5
	// 0x6bde87: after a twist (and after a constant-doom challenge's fill), a gem may turn into a lock gem (mLockGem,
	// up to mMaxLocksOnScreen) and a doom gem may appear (mDevil, or always in a constant-doom challenge)
	void NextTurnGems();
	void PreLockPiece(Piece* thePiece);          // 0x6ace4d: two turns to its lock, with SOUND_GEM_PRELOCK
	float RandUnit();                            // 0x6a3477: -1 to 1 in steps of 1/5000, from BoardRand
	bool HasChallengeFlag(int theFlag);          // the challenge's flags (+0x2630's +0x80) in a challenge
	void CountGems();                            // 0x6a56a5: the gems on the board by kind, into mGemCounts
	void MakeCounter(Piece* thePiece);           // 0x6a50a9: a bomb with the level's counter
	void MakeCoal(Piece* thePiece);              // 0x6a5074
	void SaveGame(Buffer* theBuffer, bool theReplays);  // 0x6b0d91: theReplays adds the app's three at +0xfa0
	int FindMatches(unsigned int theFlags);      // 0x6cd468: bit 1 for tips, bit 2 for real; returns the matches
	int FinishMatches(std::vector<MatchInfo>& theMatches, std::set<int>& theTipGems);
	void MakeMatchSpecial(MatchInfo& theMatch, std::vector<Piece*>& theReset, bool thePower, bool theElectro);
	void MakePowerGem(Piece* thePiece, float theDelay);    // 0x6a5019
	void MakeElectroGem(Piece* thePiece, float theDelay);  // 0x6a4ef8
	void MakeIceGem(Piece* thePiece, float theDelay);        // 0x6a4f72
	void MakeFruitGem(Piece* thePiece);          // 0x6c23bd
	void ClearHitCounts();                       // 0x6a4560
	void ClearSparkStarts();                     // 0x6a5f12
	void StartSparks(Piece* thePiece);           // 0x6a453b
	// 0x6bf983: the points of a match, popped up at theX, theY in theColor; returns their floater (or null)
	Points* AddPoints(int theX, int theY, int thePoints, const Color& theColor, int theSerial, bool theAddsProgress,
		bool theMultiplyBySets, bool theFirst, bool theByLevel);
	void ShowCountPopup();                       // 0x6b3ffc
	void UnlockMode(int theMode);                // 0x6b40b5: the mode opens, with a banner
	bool CheckGoal(bool theFlag);                // 0x6b3432: the challenge's goal, or the tube's bonus challenge being met
	void ResetFruitCount();                      // 0x6b3615
	void MatchAfterMove();                       // 0x6cf3fd: the matches once the gems have moved, and the chain
	void PlayChainSound(int theChain);           // 0x6a674d
	// 0x6c3456: a move's match in quick succession (+0x25b4 of them, each within 100 ticks of the last and two gaps
	// in a row 150 at most); each sixth earns 500, 1000 or 2000 points and a banner
	void SpeedBonus();
	void ShowSpeedText(int theLevel);            // 0x6bc3e3: the banner and its sound
	void Refill();                               // 0x6c755e: once the matched gems have gone
	void StartTubeSlide();                     // 0x6b681d
	void StartGo();                     // 0x6b8bca: a turn ends
	// 0x6b66d2: a challenge's music: a timed one's (300, or 308 for three minutes) at once, else after a fade the
	// turn-based track 305 (a Moves goal) or 306
	void PlayChallengeMusic();
	// 0x6b2fda: track 305's channels come in one by one as the moves add up, and its tempo rises with them
	void UpdateTurnBasedMusic();
	void RestartChallenge();                     // 0x6c9987: the in-game options' restart, in a challenge
	void RestartBlitz();                         // 0x6c7a75: and in Blitz
	void ShowRankBonus(int theBonus);            // 0x6ab269: the rank bar (RankBonusWidget) with the bonus
	void AddStat(int theStat, int theDelta);     // 0x6a450f: the game's stat and the app's copy (unless +0x37c4)
	void EaseCursor();                           // 0x6a4674: the cursor drawn eases to the cursor
	// 0x6bd338: the bombs' and devil gems' counters show their warnings; true when the game ends on one at zero
	void UpdateGlows();                           // 0x6b094e
	void UpdateCellPoints();                      // 0x6a5809: the cells' wobble
	void UpdateLevelUp();                         // 0x6c6905
	int GetGridRight();                          // 0x6a49a3
	float GetGridMidY();                         // 0x6a49cf
	void UpdateFruitBlast();                     // 0x6c4466: the fruit's blast: its rings break the gems of its colour
	void UpdateHint();                           // 0x6aff84
	void ShowHint();                             // 0x6c32cf
	void UpdateToolTips();                       // part of 0x6d090f
	void ShowReplayPod();                        // 0x6ab48d: the pod with the replay button rises (not in blitz)
	void SetBoardFade(float theValue);         // 0x6a6a07: the board's brightness; rising from 0 it shows the gems
	void ApplyBoardFade();                     // 0x6a69a8: every gem's alpha to it
	void RetargetTravellers();                   // 0x6a9f1d
	void FinishLevelUp();                         // 0x6bab49: the change between levels is over
	void SettleTurn();                            // 0x6c2f69: the board at rest: the next move and the turn's end
	void QueueTip(int theKind, Piece* thePiece);  // 0x6b98f8: a tip waits for its dialog (+0x256c)
	bool CheckTips();                             // 0x6bc129: tips for the special gems on the board
	void ShowNextTip();                           // 0x6bbc58
	// 0x6c986a: the tube beside the board shown (the grid slides over to make room) or hidden (the grid in the middle)
	void ShowTube(bool theShow, bool theInstant);
	void CloseTip();                              // 0x6c78a5: the tip's dialog is done (marked seen; the basics end)
	void ShowMenu();                              // 0x6a6983: the options come up over the game
	// 0x6a615b: the zen jump (the mode select's theMode 1 over classic, 2 over zen) as the board's dialog
	void ShowZenJump(int theMode);
	void AskQuit();                               // 0x6b38f3: dialog 1, by the game mode, heard by this board
	void DialogButtonDepress(int theDialogId, int theButtonId) override;  // 0x6c8632: dialogs 1 and 0x28
	bool DropFlaggedGems();                      // 0x6bb7e3: the lowest FLAG_DROPPER gem of each column sinks a row
	void EndTurn();                               // 0x6be49c
	void UpdateRecords();                         // 0x6bcd96: the game's best move, cascade and score
	bool IsGameOverPending();                     // 0x6a5626
	void CheckCombos();                           // 0x6bbbc2: the turn's praise
	int GetTurnRating();                          // 0x6a5179
	void ShowPraise(int theLevel);                // 0x6ba73a: 0 to 5, from "Good" up
	void PlayGameMusic();                         // 0x6a6a8b
	void LevelComplete();                         // 0x6b3634
	void ShowLevelUp();                           // 0x6a66d5: the LevelUpShow, with its voice
	void UpdateJump();                            // UpdateGame's part (0x6d090f) while +0x2108 runs: the level's end
	void ClearShockwaves();                       // 0x6b02ea
	void EndJump();                               // 0x6baaa7: the board after the jump between levels
	bool UpdateShockwaves();                     // 0x6af497
	bool UpdateSmokeAndGrowth();                     // 0x6af0e2
	void CheckSettled();                          // 0x6c7659
	bool SettlePieces();                          // 0x6c812f: true once every gem is at rest
	void UpdateStrikes();                         // 0x6c87d2: the electro gems' strikes (+0x149c)
	void FallPieces();                            // 0x6cf84a
	static void UpdateCell(Cell& theCell);        // 0x6a3904
	bool DropPieces();                            // 0x6a5512: true if a column has a gap
	int FillInBlanks(bool theAtOnce);             // 0x6c42e3: returns how many gems are new
	bool CanMove(Piece* thePiece);                // 0x6a50e8
	void LayoutTube();                            // 0x6b3da8
	bool CountDownCounters();
	void NextLevelMusic();                       // 0x6a496b: the next of the game's tunes
	void AddedToManager(WidgetManager* theWidgetManager) override;  // 0x6b8aa2
	// 0x6ca084: saves the game (back at the level's start if a move is under way) unless it is over
	void RemovedFromManager(WidgetManager* theWidgetManager) override;
	void DrawAll(ModalFlags* theFlags, Graphics* g) override;  // 0x6aac5e
	void Draw(Graphics* g) override;                 // 0x6cc7ed
	void DrawBoardBack(Graphics* g);                 // 0x6cc5a3: the 2D board and the space behind it
	void DrawBoardBack3D(Graphics* g);               // 0x6cc762: the board with Direct3D
	void DrawStrikes(Graphics* g);                   // 0x6aafc1
	void DrawOverlay(Graphics* g) override;          // 0x6b4a39
	void DrawCounters(Graphics* g);                  // 0x6b42e1
	// 0x6caa13 (0x6cc547 skips it while +0x1ff5): theToImage leaves out the grid, theSkipPieces all but the grid
	void DrawBoard(Graphics* g, bool theToImage, bool theSkipPieces);
	void DrawGrid(Graphics* g, float theArtScale);  // 0x6caa13's grid
	void DrawPieces(Graphics* g, Graphics& theLightG, bool is3D);  // 0x6caa13's gems
	void DrawBombHaze(Graphics* g, float theHazeFactor);  // 0x6caa13's mask pass of the bombs' heat haze (0x6cb2f4)
	void GatherHazeRects();                               // 0x6caa13's rectangles over the bombs' cells into mHazeRects
	// 0x6aa3bc: in a tip, the forced move's "click here" and the arrows around the cursor
	void DrawTutorialCursor(Graphics* g);
	void DrawSelector(Graphics* g, bool theRing);  // 0x6aa6d1: the twist cursor, its ring or its spiral
	void AddLight(float theX, float theY, float theSize, float theFalloff, float theStrength, Piece* theSkip,
		bool theSkipPower);                         // 0x6a5c60
	// 0x6a6ad0: blitz, or a challenge with the timed flag
	bool IsTimed() { return mGameMode == MODE_BLITZ || HasChallengeFlag(2); }
	void Update() override;                      // 0x6d6899: the game's updates for this frame, at its speed
	void UpdateGame();                           // 0x6d090f: one update of the game
	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x6a5b53: the pieces move with the grid
	using Widget::Resize;
	int GetBoardY() { return (mHeight - mRows * gGemHeight) / 2; }    // 0x6a4801
	int GetRowY(int theRow) { return GetBoardY() + gGemHeight * theRow; }  // 0x6a4831
	static int GetBoardWidth();                  // 0x6a47ee: the grid's width (gBoard's)

	Piece* GetPieceAt(int theRow, int theCol);   // 0x6a40c7
	Piece* GetPieceById(int theId);              // 0x6aff38
	bool AnyCellMarked();                     // 0x6a478b
	Piece* FindHintPiece();                      // 0x6a42b4
	void AddPieceId(int theId, Piece* thePiece); // 0x6b8c20
	void RemovePieceId(int theId);               // 0x6b677b
	bool IsSparkBlocked() { return mSparkBlock > 0; }  // 0x6a62d0
	// 0x6a4065: not a challenge, nor at the game's end (+0x160 6 and 7), nor with +0x1390
	// port (web): the save RemovedFromManager would write if the board went now (into theSave, with the mode whose file
	// it is), without changing the board; false where it would write none. A page that goes away never removes it.
	bool GetQuitSave(Buffer* theSave, int* theMode);
	// port (web): where SaveGame put each save without the replays in mSaveBuffer and mReplaySave, in bytes, so that
	// GetQuitSave can give what LoadGame of one followed by SaveGame with the replays writes
	struct SaveRange {
		const Buffer* mBuffer;
		int mStart;
		int mEnd;
	};
	std::vector<SaveRange> mSaveRanges;
	void ResaveAsLoaded(const uchar* theSave, Buffer* theOut);
	void WriteReplays(Buffer* theBuffer);  // SaveGame's flag and the app's three replays
	bool CanSaveOnQuit() { return !mBeforeFirstTwist && mPlayState != 6 && mPlayState != 7 && mGameMode != MODE_CHALLENGE; }
	// 0x6a32ca
	bool IsBusy() { return 0.0f < mJumpFillWait || 0 < mLevelUpTilePhase || 0.0f < mNextLevelWait; }
	// port: the piece the dialog at +0x2584 points at, once it is ported
	int GetPointedPieceId() { return mTutorial != nullptr ? mTutorial->mPointedPieceId : -1; }
	Piece* GetPieceAtXY(int theX, int theY);     // 0x6a4121
	static int GetLevelSteps(int theLevel);      // 0x70205f: the chain steps a multiplier level takes
	int GetLevelFor(int* theLeft);               // 0x6a4383: the level +0x224's steps reach, and the steps over
	void SetLevelSteps(int theSteps);            // 0x6a43e8: the chain's steps (+0x224), its level and the tube's
	void AddChainStep();                         // 0x6a62e1
	void ShowTubeLevel();                        // 0x6a3e81
	void BreakPiece(Piece* thePiece);            // 0x6c0ccf: a gem breaks up, in a match or blown up
	void BlowUpPiece(Piece* thePiece, float theX, float theY, bool theBreak, bool theBlast);  // 0x6c1857
	// 0x6a4a1b: the gems above thePiece jump, harder the nearer they are
	void LiftColumn(Piece* thePiece, float theStrength);
	void SetOffPowerGem(Piece* thePiece);         // 0x6c6d68
	void BurstAt(int theX, int theY, int theRange);    // 0x6baed1
	// 0x6b2c50: the game is over; theNuke when a counter reached zero (the board blows up), else GAME OVER shows
	void EndGame(bool theNuke);
	// 0x6af1d0: one more pause (+0x1570); the first stops the game's clock (+0x14e8), the tube's mouse and the danger
	// music. theShow counts it in +0x1574 too, the pauses the player sees
	void Pause(bool theShow);
	// 0x6bd0d9: thePiece's counter reached zero: the second chance's wheel (WheelSpin) waits for its turn
	void SpinWheel(Piece* thePiece);
	// 0x6af2be: one pause less; the last restarts the clock and the music
	void Unpause(bool theShow);
	void GetCenter(int* theX, int* theY);  // 0x6a48c9: the grid's middle (the board's, while +0x2041 is off)
	void SetEndState(int theState);        // 0x6b2708
	// 0x6d045b: the board after the game, state by state (+0x26c8): the rows fall off (3), blitz's final score
	// (4 to 8), then back to the menu (9)
	void UpdateGameOver();
	// 0x6d090f's state 7 with the nuke: the blast from +0x26cc blows the gems apart, the tube steams, then space.
	// True when a night challenge ends there (0x6b330d), which ends UpdateGame too
	bool UpdateNuke();
	// 0x6b330d: the challenge is met: "challenge complete" (an AwardTelly the challenge menu takes over, its +0x9dc),
	// space comes back over the board and the menu goes to state 5. theFlag is set for a night challenge
	void ShowChallengeComplete(bool theFlag);
	// 0x6a3b4c: the game's time, scores and counts into the profile and the app, once (+0x137c)
	void UpdateStats();
	void DrawNukeGem(Graphics* g, Piece* thePiece);  // 0x6a6885: the gem that set off the nuke, while hidden
	void DrawToolTip(Graphics* g);                   // part of 0x6b4a39 (DrawOverlay): the tooltip UpdateToolTips brings up
	// 0x6c03a4: a doom gem, coal, a gem lock or a bomb breaking; theType is 1 when the match breaks it, 2 when electro
	// does
	void BreakSpecial(Piece* thePiece, int theType);
	Geode* AddGeode(int theX, int theY, float theAngle);  // 0x6ba87f
	void UpdateGeodes();                           // 0x6d090f: the geodes in flight
	void BurstGems(int theX, int theY, int theRange);  // 0x6bac72
	// 0x6a4b7c: the gems above and around thePiece jump away from it
	void LiftAround(Piece* thePiece, float theStrength);
	// 0x6a6962: one more gem set off in a chain of blasts, each worth twice the last (to 100)
	void AddBlastChain() { mBlastChainCount++; mBurstSize = mBurstSize * 2 <= 100 ? mBurstSize * 2 : 100; }
	bool IsTipShowing() { return mTipShowing; }     // 0x6a639a
	int GetBoardX() { return mBoardX; }                // 0x6a47d9
	int GetColX(int theCol) { return GetBoardX() + gGemWidth * theCol; }  // 0x6a4819
	int GetPan(int theX);                              // 0x6a9ccd: -800 to 800 across the grid
	// 0x6ab14b: a sound panned to where thePiece is, unless the board is quiet (+0x37f8)
	void PlayPieceSound(Piece* thePiece, int theSoundId, float thePitch, float theVolume, bool theTrack);
};

// DAT_0085aef4: the board while there is one
extern Board* gBoard;
extern int gSettleCount;  // 0x84e88c: how often the board has come to rest
extern MTRand gBoardRand;  // 0x85b970: a generator of its own, seeded from the time by each new board
int BoardRand();  // 0x6a3447
// 0x85af08: each colour's tint, copied by the board's constructor from the table at 0x85c378 (seven of them; the last
// three stay Color(), opaque black)
extern Color gGemColors[10];

}
