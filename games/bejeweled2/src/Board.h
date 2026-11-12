#ifndef __BOARD_H__
#define __BOARD_H__

// Board.h: the game board (Sexy::Board, 0x59483b-0x5ae34e; the object is 0x165c4 bytes). One class for every mode: the
// mode flags below say which one is played (they are set from WinDMApp::mGameMode by the constructor). Offsets in the
// comments are the original's.
//
// The board is a grid of 8 by 8 Piece pointers, mBoard[row][col], null for an empty square. A piece sits at the pixel
// position mX/mY (640x480 board space; WinDMApp::ScaleNum scales it when it is drawn or hit) and falls with mVY.

#include "Widget.h"
#include "ButtonListener.h"
#include "CurvedVal.h"
#include "MTRand.h"
#include "Buffer.h"
#include "Piece.h"
#include "GalaxyParser.h"
#include <vector>
#include <list>
#include <string>

namespace Sexy
{

class WinDMApp;
class MemoryImage;
class PodButton;
class HyperlinkWidget;
class PodDialogButton;
class PuzzleStateOverlay;
class GalaxyMap;
class EffectOverlay;
class Hyperspace;
class StarField;
class Points;
class SoundInstance;
class Dialog;
class Graphics;
class Image;
class Font;

// a point of the 7 by 7 mesh that the hyperspace jump draws the board on (0x1c bytes each, at +0x16034)
struct WaterPoint
{
	float			mX;             // where the point is drawn in the 84 by 84 picture
	float			mY;
	float			mU;             // where it is in the texture
	float			mV;
	float			mAngle;         // the angle and the distance from the middle of the picture
	float			mRadius;
	float			mScale;         // how far the ripples stretch the distance
};

// a point of the 48 by 48 mesh that the whirlpool turns (0x24 bytes each, at +0x199c): a vertex of the picture of the
// board with its angle and distance from the middle of the 1024 by 768 picture
struct WhirlPoint
{
	float			mX;
	float			mY;
	float			mZ;
	float			mRHW;
	unsigned int	mColor;
	float			mU;
	float			mV;
	float			mAngle;
	float			mRadius;
};

// a node of a puzzle's solution graph (BoardPuzzle.cpp)
struct SolutionNode
{
	unsigned char	mCount;         // the number of moves that are valid in the node
	int				mStart;         // the index of the first one in mSolutionMoveList
};

// 0x5946f6: a random number from -1 to 1
inline float GetRandFloat()
{
	return (float)((Rand() % 1000 - 500) / 500.0);
}

// the rounding of a float or double to int that the original's code makes everywhere (truncation)
inline int FTol(double theValue)
{
	return (int)theValue;
}

class Board : public Widget, public ButtonListener
{
public:
	// the game modes of WinDMApp::mGameMode
	enum
	{
		MODE_CLASSIC = 0,
		MODE_ACTION = 1,
		MODE_PUZZLE = 2,
		MODE_ENDLESS = 3,
		MODE_TWILIGHT = 4,      // the classic game on an upside down board
		MODE_HYPER = 5,         // the action game with the power gems
		MODE_COGNITO = 6,       // the puzzles of the secret galaxy
		MODE_FINITY = 7,
		MODE_HIDDEN = 8         // the hidden mode of the main menu: classic with the secret rules
	};

	// mState (+0x170c)
	enum
	{
		STATE_NEW = 0,                  // gems fall into the board
		STATE_IDLE = 1,                 // waiting for a move
		STATE_SWAPPING = 2,
		STATE_SWAP_BACK = 3,
		STATE_COMMIT_SWAP = 4,
		STATE_CLEARING = 5,
		STATE_GAME_OVER_FALL = 6,
		STATE_7 = 7,
		STATE_LEVEL_COMPLETE_WAIT = 8,
		STATE_9 = 9,
		STATE_WHIRLPOOL = 10,
		STATE_LEVEL_TRANSITION = 11,
		STATE_BOARD_DROPPING = 12,
		STATE_POST_WARP = 13,
		STATE_HYPER_SWAP_DONE = 14,
		STATE_POD_CHANGE = 15,
		STATE_PUZZLE_SOLVED = 16,
		STATE_PUZZLE_STUCK = 17,
		STATE_GALAXY = 18,              // the galaxy map or the end screen
		STATE_FADE = 19,
		STATE_SECRET_FINISHED = 20,
		STATE_GALAXY_SLIDE_1 = 21,
		STATE_GALAXY_SLIDE_2 = 22,
		STATE_GALAXY_SLIDE_3 = 23
	};

	// mScreenState (+0x1714)
	enum
	{
		SCREEN_SLIDING_IN = 0,
		SCREEN_SHOWN = 1,
		SCREEN_SLIDING_OUT = 2,
		SCREEN_HIDDEN = 3
	};

public:
	WinDMApp*		mApp;               // +0x8c (the ButtonListener subobject is at +0x88)
	PuzzleStateOverlay* mPuzzleOverlay; // +0x90 the pods of the puzzle packs (puzzle mode, not Cognito)
	GalaxyMap*		mGalaxyMap;         // +0x94 the map of the puzzle galaxies (puzzle modes)
	EffectOverlay*	mEffectOverlay;     // +0x98 draws over everything: the big texts and the hyperspace
	double			mBombChance;        // +0xc00 properties "BombChance", "BombCountdown", "RockChance", "PowerGemChance"
	int				mBombCountdown;     // +0xc08
	double			mRockChance;        // +0xc10
	double			mPowerGemChance;    // +0xc18
	MTRand			mRand;              // +0xc20 draws the colours of new gems; seeded from Rand() by the constructor

	// the grid and the pieces
	Piece*			mBoard[8][8];       // +0x15ec [row][col]
	int				mColumnExplodeTick[8]; // +0x16ec the update count at which a piece of the column exploded
	std::vector<Points*>	mPoints;    // +0x15ec8 the score texts
	std::vector<Shard*>		mShards;    // the sparks and flames
	std::vector<Effect*>	mEffects;   // the explosion animations
	std::vector<ShockWave>	mShockWaves;
	std::vector<Lightning*>	mLightning; // +0x15fe0
	Piece*			mSwapPiece[2];      // +0x197c, +0x1980
	Piece*			mSelectedPiece;     // +0x1958 the gem that the mouse is dragging
	Piece*			mHintPiece;         // +0x1950 the gem that a tip dialog is about

	// the state of the play
	int				mState;             // +0x170c
	bool			mCoversScreenFlag;  // +0x1710
	int				mScreenState;       // +0x1714
	bool			mEndInfoProcessed;  // +0x1718 ProcessEndGameInfo ran
	bool			mIsClassic;         // +0x1719 modes 0 and 8
	bool			mIsAction;          // +0x171a modes 1, 5 and 7: the timed games
	bool			mIsPuzzle;          // +0x171b modes 2 and 6
	bool			mIsEndless;         // +0x171c mode 3
	bool			mIsSecret;          // +0x171d modes 5, 6, 7 and 8
	bool			mIsTwilight;        // +0x171e mode 4
	bool			mIsFinity;          // +0x171f mode 7
	bool			mIsHyper;           // +0x1720 mode 5
	bool			mFlipped;           // +0x1721 mode 4: the board is upside down (rows count from the bottom)
	bool			mInUndoPreview;     // +0x1722 (selects the undo list)
	bool			mFreeMode;          // +0x1723 saved: a swap that makes no match is not swapped back (the original never sets it)
	bool			mSecretModeUnlocked; // +0x1725 the profile has unlocked the secret version of this mode (so says no more)
	SoundInstance*	mFadingSound;       // +0x1728 the sound that fades with the level change (the original never sets it)
	bool			mNoMoveYet;         // +0x172c
	int				mDropDelay;         // +0x1730 updates to wait before gems fall into the board
	int				mLevel;
	int				mNextLevel;
	int				mScore;             // +0x15e20
	double			mLevelValue;        // +0x15e28 the points of a cleared gem
	int				mBackdropNum;       // +0x15f74
	bool			mCheated;           // +0x165b0
	int				mPauseCount;        // +0x15ec0
	int				mInterfacePauseCount; // +0x15ec4
	int				mPlayStartTick;     // +0x15e0c the update count at which the running stretch started
	int				mPlayedSeconds;     // +0x15e10 seconds of the finished stretches
	int				mUnpauseDelay;      // +0x15f3c

	// the buttons and the tutorial
	PodButton*		mMenuButton;        // +0x1758 id 0
	PodButton*		mHintButton;        // +0x175c id 1
	PodButton*		mThirdButton;       // +0x1760 id 2: undo (puzzle modes), id 3: pause (action modes), else null
	HyperlinkWidget* mBackLink;
	PodDialogButton* mTutorialButton1;
	PodDialogButton* mTutorialButton2;
	Widget*			mTutorialWidget;
	PodDialogButton* mPlayAgainButton;
	PodDialogButton* mMainMenuButton;
	std::vector<TutorialMove>	mTutorialMoves;
	std::vector<std::string>	mTutorialText;
	int				mTutorialStep;
	int				mHoverPod;          // +0x1904 the puzzle pod under the mouse, -1
	int				mPendingPod;
	int				mMouseX;            // +0x15f28
	int				mMouseY;            // +0x15f2c

	// the sweep of the level change
	int				mSweep0;            // +0x17b0
	int				mSweep1;            // +0x17b4
	int				mSweep2;            // +0x17b8
	int				mSweepA;            // +0x17bc
	int				mSweepB;            // +0x17c0
	int				mSweepTimer;        // +0x17c4

	// the pictures
	MemoryImage*	mBackdropImage;     // +0x15f58 the picture behind the board
	MemoryImage*	mNextBackdropImage; // +0x15f5c the next one, made by the worker thread
	MemoryImage*	mFontGlowImage;     // +0x15f54 the level text for the glow
	Hyperspace*		mHyperspace;        // +0x15e00 the jump through hyperspace (a widget over the board)

	// the slides and the sways
	CurvedVal		mGalaxySlide;       // +0x17e8 the board slides away when the galaxy map shows
	CurvedVal		mSlide;             // +0x1818 0 shown, 2000 off screen
	float			mSwapFade;
	float			mShakeSize;         // +0x15e5c
	float			mShakeTime;         // +0x15e60
	int				mShakeX;            // +0x15e64
	int				mShakeY;            // +0x15e68
	WaterPoint		mWaterMesh[7][7];   // +0x16034
	WhirlPoint		mWhirlMesh[48][48]; // +0x199c

	// the swap of two gems
	bool			mHyperSwap;
	int				mSwapTime;
	int				mSwapCenterX;
	int				mSwapCenterY;
	bool			mCrazy;             // +0x18e8 MakePuzzleCrazy mirrored the board

	// the undo lists and the solution of a puzzle
	Buffer			mUndoBuffer;        // +0x1868
	std::list<Buffer>	mUndoList[2];   // +0x1884
	int				mUndoIndex[2];      // +0x18a0
	int				mSolutionHash;      // +0x18a8
	int				mSolutionNode;      // +0x18ec -1: no solution known
	bool			mSolutionValid;     // +0x18f4
	int				mSolutionMoves;     // +0x18f8
	int				mSolutionHintIndex; // +0x1900
	std::string		mSolutionFile;
	std::string		mPuzzleFile;
	std::vector<SolutionNode>	mSolutionNodes;
	std::vector<unsigned int>	mSolutionMoveList;
	std::vector<std::string>	mSecretPuzzles;

	// the cheats
	std::string		mCheatBuffer;       // +0x165a0 the last 24 letters typed
	StarField*		mCheatStars;        // +0x165bc the star field or the dots of a cheat
	bool			mOldSkool;          // +0x165ac OLDSKOOL: draws the board in 2D
	bool			mXmas;              // +0x165ad XMAS
	bool			mImagesChanged;     // +0x165ae COLORSWAP, BLACKANDWHITE, AMBERSCREEN, GREENSCREEN: the images have been changed
	bool			mSlowMotion;        // +0x165af SLOMO
	bool			mNausea;            // +0x165b8 NAUSEA
	bool			mNoFrame;           // +0x165c0 NOFRAME

	// the rest, in the order of the original's object
	bool			mClearBacklog;      // +0x1751 the next Draw clears the update backlog (set when the whirlpool ends)
	int				mStartCountdown;    // +0x1764 updates until the level starts: the GO sound
	int				mLevelTextCountdown; // +0x1768 updates until the LEVEL text shows
	bool			mStartGameMusic;    // +0x1770 the board's second update fades the menu song out and starts the game's (the Play Again button clears it)
	int				mScoreSnapshot;     // +0x1774 the score when the puzzle began or the game was saved
	int				mSecretPuzzleCount; // +0x17a4 the puzzles queued for the secret galaxy (Cognito)
	int				mSecretSolved;      // +0x17a8 the puzzles solved without a hint
	int				mSecretRun;         // +0x17ac the puzzles solved without a hint in this run of four (the bar)
	int				mHyperGlowDelay;    // +0x17d4 updates until the glow starts (15)
	int				mHyperGlowX;        // +0x17d8 where the glow is
	int				mHyperGlowY;        // +0x17dc
	bool			mHyperGlow;         // +0x17d0 the glow of the swapped hyper cube is on
	float			mHyperGlowPhase;    // +0x17e0 0 to 1
	bool			mShootingStar;      // +0x1848 a shooting star runs behind the board
	float			mShootingStarPos;   // +0x184c 0 to 1: how far it has run
	bool			mPodArrowShown;     // +0x1850 the arrow over the next pod of the puzzle bobs (set when the board is quiet)
	double			mPodArrowPhase;     // +0x1858 its phase (EffectOverlay::Draw takes its cosine)
	double			mHintButtonArrowPhase; // +0x1860 the phase of the arrow that points at the hint button
	int				mSolutionFirstNode; // +0x18f0 the first node of the solution file (-1 without one)
	unsigned char	mSolutionMove;      // +0x18fc the move just made, as the solution file writes one
	int				mShowPlanetCountAtStart; // +0x190c the start of the level shows the planet's puzzle count, not GO
	int				mMoreHintsDelay;    // +0x1910 updates until the "More Hints?" box (25 after a puzzle was set back)
	int				mHintsUsed;         // +0x1918 the hints that the puzzle has asked for (the secret puzzles count only those solved without one)
	bool			mFinale;            // +0x191c the jump to the last planet's finale
	bool			mIsPlanetIntro;     // +0x191d the planet's intro puzzle is on the board (SelectPuzzlePod with -1)
	float			mTutorialMoveFade;  // +0x1944 the tutorial pointer over the move that was made: 1 fading to 0
	float			mTutorialNextFade;  // +0x1948 the pointer over the next move: 0 rising to 1
	bool			mFlameFromCross;    // +0x194c the last power gem came from two sets crossing, not from four gems in a line
	bool			mPowerGemTipShown;  // +0x194d saved: the power gem tip has been shown in this game
	bool			mHyperCubeTipShown; // +0x194e saved: the hyper cube tip has been shown in this game
	float			mHintArrowPhase;    // +0x1954 the phase of the arrow over the hint gem (EffectOverlay::Draw takes its cosine)
	int				mBarShown;          // +0x1994 saved: the length of the level bar that is drawn (it runs after the target)
	int				mPlayUpdates;       // +0x1998 the updates played in this game (the end of game info counts them as hundredths of a second)
	float			mWhirlStrength;     // +0x15d9c how hard the whirlpool pulls the mesh in
	int				mWhirlStrengthDelay; // +0x15da0 updates before the pull starts (90)
	int				mInterfaceFlyDelay; // +0x15da4 updates before the interface starts to fly away (80)
	float			mWhirlRadiusScale;  // +0x15da8 the scale of the whirlpool's spiral, 1 and rising
	CurvedVal		mInterfaceFlyRamp;  // +0x15db0 speeds the interface up as it flies away
	double			mInterfaceFly;      // +0x15de0 0 to 1: how far the interface has flown away
	bool			mPlayWhirlSound;    // +0x15de8 the next Draw plays the whirlpool sound
	int				mHyperspaceDelay;   // +0x15df0 updates until the whirlpool jumps (100)
	float			mInterfaceRestore;  // +0x15e04 0 to 1: how far the interface has come back after the jump
	int				mLightningColor;    // +0x15e08 the colour of the gems that the hyper cube's lightning hits
	int				mGameSeconds;       // +0x15e14 the seconds played in this game
	int				mSecondsCredited;   // +0x15e18 the seconds that the profile's statistics already hold
	int				mHintCountdown;     // +0x15e1c
	double			mTimedGoal;         // +0x15e30 saved: the points the next level needs, in level values (the timed games)
	double			mTimedGoalStep;     // +0x15e38 saved: what each level adds to it (it grows by TimedBonusAccel of the application)
	int				mShownScore;        // +0x15e40 saved: the score that is shown (it runs after the score)
	int				mGemsCleared;       // +0x15e44 saved: the gems collected
	int				mGemsCredited;      // +0x15e48 the gems that the profile's statistics already hold
	int				mSetsInMove;        // +0x15e4c saved: the sets cleared since the move began (the second one scores more)
	int				mCascadeCount;      // +0x15e50 the clears that followed the move so far (DoneClearing counts, a swap clears it): the combo sound's step
	int				mLastLandSoundTick; // +0x15e54 the update at which a landing gem last sounded
	int				mGameOverDelay;     // +0x15e58 updates until the board blows up at the end of the game
	int				mBiggestCascade;    // +0x15e6c saved: the longest cascade of the game
	int				mBiggestCombo;      // +0x15e70 saved: the best score of one move
	int				mChainCount;        // +0x15e74 the flame and bomb gems that exploded in this chain (reset by CheckMatches)
	bool			mExplodeHappened;   // +0x15e78 a gem exploded in this update: Update plays the sound once
	int				mPowerGemsMade;     // +0x15e7c saved
	int				mPowerGemsCredited; // +0x15e80 what the profile's statistics already hold
	int				mHyperCubesMade;    // +0x15e84 saved
	int				mHyperCubesCredited; // +0x15e88 what the profile's statistics already hold
	int				mLastExplodeSoundTick; // +0x15e8c the update at which a hyper cube in a blast last sounded
	int				mMoveScore;         // +0x15e94 the points of the move so far
	int				mBigMoveTimer;      // +0x15e98 updates left of the glow of a big move (300 to 1200)
	double			mBigMoveGlow;       // +0x15ea0 its brightness (nothing draws it; the update keeps the screen dirty meanwhile)
	bool			mMovedSinceHint;    // +0x15ea8 the player has moved since the last hint
	bool			mHintAccepted;      // +0x15ea9 the puzzle's hints are accepted: the player has taken the hinted moves
	int				mHintFromCol;       // +0x15eac the move that the puzzle's hint showed: -1 when none
	int				mHintFromRow;       // +0x15eb0
	int				mHintToCol;         // +0x15eb4
	int				mHintToRow;         // +0x15eb8
	Font*			mScoreFont;         // +0x15edc
	bool			mSkipBestRecords;   // +0x15ee0 saved: the cascade and combo records are not kept (the original never sets it)
	int				mNeverSetScoreMark; // +0x15ee4 saved: the original saves it before the level's start score and never sets or reads it
	int				mLevelStartScore;   // +0x15ee8 saved: the score at the start of the level
	int				mLevelGoalScore;    // +0x15eec saved: the score that finishes the level
	int				mLevelGoalPoints;   // +0x15ef0 saved: the points the level needs
	double			mDrain;             // +0x15ef8 saved: how far the timer bar has drained (timed games)
	double			mDrainSpeed;        // +0x15f00 saved: its speed (TimedPV)
	double			mDrainAccel;        // +0x15f08 saved: its acceleration (TimedPA)
	double			mDrainJerk;         // +0x15f10 saved: its jerk (TimedPJ)
	bool			mBarBlink;          // +0x15f18 the bar blinks (it is low)
	int				mTimerFrame;        // +0x15f1c saved: the frame of the timer (0 to 24)
	int				mLastWarningTick;   // +0x15f20 the update at which the timer last beeped
	bool			mBoardInOverlay;    // +0x15f24 the effect overlay draws the board (the game over)
	int				mMouseDownX;        // +0x15f30 where the mouse button went down
	int				mMouseDownY;        // +0x15f34
	bool			mMouseHeld;         // +0x15f38 the mouse button is down
	MemoryImage*	mStaticImage;       // +0x15f60 the static half of the screen (a DDImage that DrawNormal2d makes)
	MemoryImage*	mGemImage;          // +0x15f64 a picture that DrawBoard draws the gems of the 2D game into
	bool			mCountBombs;        // +0x15f68 saved: a move was made, so the bomb gems count down once the board is quiet
	bool			mRedrawBackground;  // +0x15f69 the picture of the static half of the screen must be drawn again
	int				mPlanetNum;         // +0x15f6c the planet the puzzle is on
	int				mSweepY;            // +0x15f7c saved: how far the sweep of the level change has come down
	float			mBlackHoleFrame;    // +0x15f84 the frame of the black hole (0 to 5) that the whirlpool turns through
	int				mDrainGrace;        // +0x15f88 updates that the timer does not drain after a move
	double			mWarpFade;          // +0x15f90 0 to 1: how far the warp's picture and sound are in
	double			mBlackHoleAngle;    // +0x15f98 the angle of the black hole's picture
	double			mBlackHoleSpin;     // +0x15fa0 how fast it turns
	bool			mSavedNeverSet;     // +0x15fa8 saved: the original writes it into the save and never sets it
	float			mPuzzleFade;        // +0x15fac 0 to 1: the puzzle's buttons and board fade while a pod is picked
	float			mFadeIn;            // +0x15fb0 how far the board's lights are in, 0 to 1 (down during the whirlpool, up after it)
	float			mPauseFade;         // +0x15fb4 1 while the game runs, 0 while only the interface is paused
	bool			mPauseTextOn;       // +0x15fb8 the PAUSED text shows while the interface is paused
	double			mStarRot;           // +0x15fc0 the turn of the first star on the selected gem
	double			mStarDrift;         // +0x15fc8 the turn of the second star, the other way
	double			mStarPhase;         // +0x15fd0 0 to 1: the flicker between the two stars and of the flames
	int				mCheerGiven;        // +0x16024 the cheer that this move has had (0 to 3)
	int				mCheerTier;         // +0x16028 the cheer that the move's score has earned: 1 GOOD, 2 EXCELLENT, 3 INCREDIBLE
	int				mCheerDelay;        // +0x1602c updates until the next cheer
	int				mLastGoodTick;      // +0x16030 the update at which GOOD last sounded

public:
	// port: the board's memory is zeroed first. The original's constructor leaves about sixty members unset (counters that
	// the first game reads as zero from fresh memory), and the port does not read what happens to be there
	static void*	operator new(size_t theSize);
	static void		operator delete(void* thePointer);
	Board(WinDMApp* theApp);                                        // 0x5ad581
	virtual ~Board();                                               // 0x5ad7d1

	// the Widget hooks
	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x594a01
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x594a71
	virtual void	SetVisible(bool isVisible) override;            // 0x594b3b
	virtual void	Update() override;                              // 0x5ad294
	virtual void	Draw(Graphics* g) override;                     // 0x5a2d15
	virtual void	MouseDown(int theX, int theY, int theClickCount) override;  // 0x5aa4e8
	virtual void	MouseUp(int theX, int theY, int theLastDownButtonId) override;  // 0x59596f
	virtual void	MouseDrag(int theX, int theY) override;        // 0x5aaac3
	virtual void	MouseMove(int x, int y) override;               // 0x5956c9
	virtual void	MouseLeave() override;                          // 0x59578c
	virtual void	KeyChar(SexyChar theChar) override;             // 0x59d5df
	virtual void	ButtonPress(int theId) override;                // 0x596bc2: the click sound
	virtual void	ButtonDepress(int theId) override;              // 0x5ad35c

	// geometry
	int				GetColX(int theCol);                            // 0x594ba4
	int				GetRowY(int theRow);                            // 0x594bd5
	void			GetBoardCenter(int* theX, int* theY);           // 0x5957f3
	int				GetPanPosition(int theX);                       // 0x5953ef the stereo pan of a sound at x, -1600 to 1600
	Piece*			GetPieceAt(int theX, int theY);                 // 0x59522a

	// the board and its pieces (BoardBase.cpp)
	void			ProcessEndGameInfo();                           // 0x59483b
	void			ProcessLevelImages();                           // 0x594d55
	void			AdvanceSweep();                                 // 0x594d97
	bool			CanPlay();                                      // 0x594eff
	unsigned int	GetBoardHash();                                 // 0x594f20
	void			MakePuzzleCrazy();                              // 0x594fd2
	void			ClearBoard();                                   // 0x5950f5
	bool			IsBoardEmpty();                                 // 0x59515c
	void			Pause(bool theInterface);                       // 0x595188
	void			Unpause(bool theInterface);                     // 0x5951db
	bool			HasSet();                                       // 0x595346
	bool			FindMove(int* theMove, int theSkip, bool theRows, bool theCols); // 0x59543c
	void			KillTutorialButtons();                          // 0x59562e
	void			ExplodingFinished();                            // 0x595857
	void			CommitSwap();                                   // 0x595c53
	bool			WillBeValidSwap();                              // 0x595da8
	unsigned char	TranslateMoveByte(unsigned char theMove);       // 0x5961a2
	void			ClearPieceHints();                              // 0x5961ec
	void			ShowHint();                                     // 0x59621c
	void			Hide();                                         // 0x5962b2
	void			Show();                                         // 0x598206
	void			ShowNow();                                      // 0x59828a
	void			HideNow();                                      // 0x59834e
	void			ShowGalaxyMap();                                // 0x596302
	void			HideGalaxyMap();                                // 0x59837e
	void			StabilizeState();                               // 0x598095
	void			ResizeButtons();                                // 0x596466
	void			UpdateStarRots();                               // 0x596747
	void			UpdateBoardDropping();                          // 0x59814e
	void			NewBoard();                                     // 0x597efd

	// effects (BoardEffects.cpp)
	Effect*			AllocEffect();                                  // 0x594b7e
	void			DoPoints(int thePoints, int theX, int theY, const Color& theColor); // 0x5a6236
	void			ExplodeAtHelper(int theCol, int theRow);        // 0x5a63a3
	void			ExplodeAt(int theCol, int theRow);              // 0x5a68b0
	void			SmallExplodeAt(int theCol, int theRow);         // 0x5a6e5f
	void			DoneClearing();                                 // 0x5a71f0
	void			UpdateEffects();                                // 0x5a7712
	void			AddAlphaSparkle(int theX, int theY, MemoryImage* theImage, int theWidth, int theHeight); // 0x5a7bfb
	void			AddLightning(int theX1, int theY1, int theX2, int theY2); // 0x5a7d39
	void			UpdateGameOverAnim();                           // 0x595ea2
	void			UpdateHyperWarpPoints();                        // 0x5967d0
	static int		ComparePieceZ(Piece* const* theA, Piece* const* theB); // 0x5968f8
	void			AddPointLight(float theX, float theY, float theScale, float theRadius, float theGain, Piece* theSkipPiece, bool theSkipFlames); // 0x596931
	void			ClearPoints();

	// the play (BoardInput.cpp, BoardMatch.cpp, BoardLogic.cpp, BoardUpdate.cpp, BoardWhirl.cpp)
	bool			MovePiece(Piece* thePiece, int theDX, int theDY); // 0x5aa2f1
	void			DoNoticeDialog(const char* theHeader, const char* theLines);
	PodDialogButton* AddBoardButton(int theId, const char* theLabel);
	void			AdvanceTutorial();                              // 0x595958
	void			SelectPuzzlePod(int thePod);                    // 0x5a92ed
	void			DeferredSelectPuzzlePod(int thePod);            // 0x5a9485
	std::list<Buffer>& GetUndoList();
	int&			GetUndoIndex();
	void			MarkForUndo();                                  // 0x5a9508
	void			Undo();                                         // 0x5a95cf
	bool			CanUndo();                                      // 0x597525
	void			UndoUntilSolvable();                            // 0x5a96f9
	void			LoadNextSecretPuzzle();                         // 0x5a9729
	void			PuzzleCompleted();                              // 0x5a9979
	void			UpdateFalling();                                // 0x5a9ced
	bool			FindSets();                                     // 0x5a7ec1
	bool			UpdateExplodeDelays();                          // 0x5a8551
	void			SwapPieces(Piece* theFirst, Piece* theSecond);  // 0x5a85bc
	void			UpdateSwapping();                               // 0x5a888f
	void			UpdateLightning();                              // 0x5a8c66
	void			DoUpdate();                                     // 0x5ab4d7
	void			UpdateClearing();                               // 0x5aab8a
	void			EndWhirlpool();                                 // 0x5aac20
	void			PickNextBackdrop();
	void			UpdateWhirlpool();                              // 0x5aafae
	void			TryShowHint();                                  // 0x5ab385

	// levels (BoardLevel.cpp)
	void			PrepareLevelImages();                           // 0x59becc
	static void		PrepareLevelImagesStub(void* theBoard);         // 0x5a5024
	void			StartPrepareLevelImages();                      // 0x5a571e
	void			InitialBackdropProcessing();                    // 0x5a573d
	void			SetupLevelData();                               // 0x59c11b
	void			GameOver(int theDelay);                         // 0x5a5030
	void			UpdateLevelTransition();                        // 0x5a510a
	void			CheckForHints();                                // 0x59cb4b
	void			DoHyperspace();                                 // 0x59d20d
	void			DoWhirlpool();                                  // 0x59d37a
	void			UpdateSecretUnlocked();                         // 0x59da14
	void			EndInterfaceRestore();                          // 0x59dae4
	void			ShowPuzzleHint();                               // 0x59db5a
	void			ShowPlanetCount();                              // 0x59de98
	void			CreateFontGlowImages();                         // 0x5a0c93

	// puzzles (BoardPuzzle.cpp)
	int				FindPuzzleValidMoves(unsigned int* theMoves, int theMax); // 0x599d97
	void			UpdateSolvability();                            // 0x599dfe
	void			UpdateSolutionValidity();                       // 0x599e3c
	void			SolutionCommitMove(unsigned char theMove);      // 0x599e5b
	void			ShowNextPuzzleMove();                           // 0x599f3c
	bool			LoadSolution(const std::string& theFileName);   // 0x5a5d6e
	bool			LoadPuzzle(const std::string& theFileName);     // 0x5a5f29
	static bool		LoadPuzzleThumb(const std::string& theFileName, int (*theThumb)[8]); // 0x59af4d
	void			UpdateBonusPenalty();                           // 0x59b0bb

	// saving (BoardSave.cpp)
	void			SaveState(Buffer& theBuffer);                   // 0x594e35
	void			LoadState(const Buffer& theBuffer);             // 0x59b957
	void			PopUndo();                                      // 0x59bad4
	void			UndoAll();                                      // 0x59bb11
	void			ClearUndoBuffers();                             // 0x59b531
	void			ClearLightning();                               // 0x59b66d
	static std::string GetSavedGameFileName();                      // 0x59c4f0
	bool			DeleteSavedGame();                              // 0x59caff
	static bool		HasSavedGame();                                 // 0x5a50b3
	bool			SaveGame();                                     // 0x59c5ab
	bool			LoadGame();                                     // 0x5ae34e

	// cheats (BoardCheats.cpp)
	void			TransformAllImages(void (*theFunction)(ulong*, int)); // 0x59b592
	void			ToggleStarField(bool theNetwork);

	// drawing
	void			DrawWidgetTo(Graphics* g, Widget* theWidget);   // 0x596a95
	void			DrawUI(Graphics* g, bool theSkipStatic, bool theSkipDynamic); // 0x596acd
	void			DrawWavyImage(Graphics* g, Image* theImage, int theX, int theY, float theLevel); // 0x5983e3
	void			DrawEffects(Graphics* g, bool thePass);         // 0x598525
	void			DrawLightning(Graphics* g);                     // 0x5989c7
	void			DrawBoard(Graphics* g, bool theSkipStatic, bool theSkipDynamic); // 0x59df5c
	void			DrawLevelText(Graphics* g);                     // 0x5a0a52
	void			DrawScore(Graphics* g, bool theSkipStatic, bool theSkipDynamic); // 0x5a0d5e
	void			Draw3DWhirlpoolState(Graphics* g);              // 0x5a1787
	void			Draw3DInterfaceRestoreState(Graphics* g);       // 0x5a238c
	void			DrawAllToBackgroundCache();                     // 0x5a29bc
	void			DrawNormal2d(Graphics* g);                      // 0x5a2a51
	void			DrawNormal(Graphics* g);                        // 0x5a2c1b
};

}

#endif
