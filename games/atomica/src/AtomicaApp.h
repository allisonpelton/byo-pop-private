#ifndef __ATOMICA_ATOMICAAPP_H__
#define __ATOMICA_ATOMICAAPP_H__

// AtomicaApp.cpp: the application class of Atomica Deluxe 2.52 (Atomica.exe 0x406260-0x40b1e0 and the helpers in
// 0x405ad0-0x406240). Behaviour spec: docs/GAME_NOTES.md section 3.
//
// Original: a class derived from the 2003 SexyApp (SexyApp : ButtonListener, 55 virtual slots) with its vftable
// at 0x4ae638; the object is 0x4478 bytes. It overrides slots 5, 11 (LoadingThreadProc, 0x406f90), 13 (the
// destructor), 24 (WriteToRegistry, 0x406ec0), 25 (ReadFromRegistry, 0x406dc0), 28 and 30 (Init, 0x4067b0).
// Offsets in the comments are the original's. The pictures are held in sprites (Support.h); the sprite arrays
// keep the original's layout (an atom's pictures are indexed [type][frame]).

#include "SexyApp.h"
#include "Support.h"
#include "HighScores.h"

#include <vector>

namespace Sexy
{

class Board;
class HintBox;
class TitleScreen;
class ImageFont;
class SoundInstance;
class AtomicaApp;

// 0x429470: a sound the app has loaded; Play (0x429250) is PlaySample on its id
class Sound
{
public:
	AtomicaApp*		mApp;		// +4
	int				mId;		// +8
	Sound(AtomicaApp* theApp, const char* theName);	// "sounds\<name>"
	void			Play();
};

class AtomicaApp : public SexyApp
{
public:
	enum
	{
		GAME_WIDTH = 640,			// ctor: mWidth 0x280, mHeight 0x1e0
		GAME_HEIGHT = 480,
		NUM_ATOM_TYPES = 27,		// 0 to 10 the plain atoms, 11 the chromium atom, 12 to 26 the elements' bonus atoms
		NUM_SPIN_TYPES = 12,		// the types with rotating pictures
		NUM_ATOM_FRAMES = 25,		// the shrunk copies of an atom (0 full size, 24 the smallest)
		NUM_SPIN_FRAMES = 20		// the frames of the rotation (every 18 degrees)
	};

	// The fonts (data\*.txt)
	ImageFont*		mAtomicFont;			// +0x468 atomicfont
	ImageFont*		mLevelFont;				// +0x46c levelfont
	ImageFont*		mScoreFont;				// +0x470 scorefont
	ImageFont*		mStencilFont;			// +0x474 stencilfont
	ImageFont*		mMonitorFont;			// +0x478 monitorfont
	ImageFont*		mMonitorFontMedium;		// +0x47c monitorfont(medium)
	ImageFont*		mMonitorFontSmall;		// +0x480 monitorfont(small)
	ImageFont*		mFloatyFont;			// +0x484 floatyfont
	ImageFont*		mAnnounceFont;			// +0x488 announcefont
	ImageFont*		mCommentFont;			// +0x48c commentfont
	ImageFont*		mMouseoverFont;			// +0x490 mouseoverfont

	Board*			mBoard;					// +0x494 the game in progress
	Widget*			mMenu;					// +0x498 the main menu
	TitleScreen*	mTitleScreen;			// +0x49c until the first click
	SpriteMgr*		mSprites;				// +0x4a0

	std::string		mLoadingText;			// the line the title screen shows under its bar (title screen +0xc4)

	int				mMode;					// +0x3ea4 0 the strategy mode, 1 the action mode
	bool			mB3ea0;					// +0x3ea0 the player gave up the game
	int				mExpertMode;			// +0x3ea8 1: expert (double scores)
	bool			mB3eac;					// +0x3eac the main menu is to be closed when the game updates
	int				mPauseCount;			// +0x4068 >0: the game is paused
	int				mI440;					// +0x440 a counter of the base app that the grid raises
	bool			mShowSymbols;			// the atoms show their chemical symbols
	bool			mShowTips;				// the tutorial boxes (the option 'disable hints' turns them off)
	bool			mHintsShown[0xff];		// +0x406c the hints that were shown in earlier games
	Widget*			mLevelUpDialog;			// +0x3e98
	std::vector<HintBox*>	mHintBoxes;		// port: the tutorial boxes that are up (the test bot closes them)
	Widget*			mScoreDialog;			// the high score entry
	Widget*			mHighScoresBox;			// the tables of both games (from the options)
	ScoreTable		mScoreTables[2];		// +0x3eb0 the strategy mode's and +0x3f8c the action mode's high scores
	Widget*			mQuitDialog;			// +0x3e8c the quit box
	Widget*			mConfirmDialog;			// +0x3e94
	Widget*			mOptionsDialog;			// +0x3e90 the game options

	GameRand		mRand;					// +0x3da4 the game's random numbers
	float			mPi;					// +0x3da0 (the original sets it from the C library: 3.14159...)

	// ---- the pictures
	Sprite*			mDialogMonitor;			// +0x293c
	Sprite*			mBigExplosionSheet;		// +0x25bc
	Sprite*			mBigExplosion[20];		// +0x25c0 its cells (110 x 110)
	Sprite*			mExplosion[3][10];		// +0x2610 every other cell at 150, 300 and 450 pixels
	Sprite*			mAtomsSheet;			// +0x4b4
	Sprite*			mAtomA[NUM_ATOM_TYPES];	// +0x4b8 cell 3k of the sheet, from (2,1), 50 x 50
	Sprite*			mAtomC[NUM_ATOM_TYPES];	// +0x530 cell 3k+2
	Sprite*			mAtomShrink[NUM_ATOM_TYPES][NUM_ATOM_FRAMES]; // +0x5a8 cell 3k+1, then 24 shrunk copies
	Sprite*			mAtomMask[NUM_SPIN_TYPES][NUM_SPIN_FRAMES]; // +0x11c4 the mask turned by 18 degrees a step
	Sprite*			mAtomSpin[NUM_SPIN_TYPES][NUM_SPIN_FRAMES]; // +0x1b24 the atom blended with the turned mask
	Sprite*			mChromium;				// +0x1160 chromiumatom
	Sprite*			mChromiumShrink[24];	// +0x1164 shrunk copies of it
	Sprite*			mPreAtom;				// +0x2688 preatom
	Sprite*			mPreAtomShrink[24];		// +0x268c shrunk copies
	Sprite*			mElectron;				// +0x2488 electron
	Sprite*			mGridBug;				// +0x248c gridbug
	Sprite*			mElectronDot;			// +0x2490 a 5 x 5 piece of the electron
	Sprite*			mBiohazardSheet;		// +0x2494
	Sprite*			mBiohazard[6];			// +0x2498 cells of 55 x 55
	Sprite*			mBadRingSheet;			// +0x24b0
	Sprite*			mBadRing[2];			// +0x24b4
	Sprite*			mBioCountdownSheet;		// +0x24bc
	Sprite*			mBioCountdown[6];		// +0x24c0 cells of 15 x 20
	Sprite*			mBonusFrame;			// +0x25b8
	Sprite*			mAdjacentSheet;			// +0x24e8 adjacentreflections
	Sprite*			mAdjacent[4];			// +0x24ec cells of 55 x 55
	Sprite*			mStarBurst;				// +0x2484
	Sprite*			mMachineScore;			// +0x24fc atomicmachine(score)
	Sprite*			mMachineLevelMeter;		// +0x2500 atomicmachine(levelmeter)
	Sprite*			mMachineRim;			// +0x2504 atomicrim
	Sprite*			mMachineMonitor;		// +0x2508 atomicmachine(atomicusmonitor)
	Sprite*			mMachineBomber;			// +0x250c atomicmachine(bomber)
	Sprite*			mMachineBomberMeter;	// +0x2510 atomicmachine(bombermeter)
	Sprite*			mMachineButtonPressed;	// +0x2514 atomicmachine(buttonpressed)
	Sprite*			mMachineButtonSeqSheet;	// +0x2518 atomicmachine(buttonsequence)
	Sprite*			mMachineButtonSeq[11];	// +0x251c cells of 68 x 43
	Sprite*			mBomberChargingSheet;	// +0x2548 atomicmachine(bombercharging)
	Sprite*			mBomberCharging[2];		// +0x254c cells of 130 x 20
	Sprite*			mBomberCapSheet;		// +0x2554 atomicmachine(bombercap)
	Sprite*			mBomberCap[24];			// +0x2558 cells of 94 x 93
	Sprite*			mTopLightsSheet;		// +0x26ec
	Sprite*			mTopLights[2];			// +0x26f0 cells of 17 x 34
	Sprite*			mSliderBar;				// +0x26f8
	Sprite*			mSlider;				// +0x26fc
	Sprite*			mSideButtonsSheet;		// +0x2700
	Sprite*			mSideButtons[6];		// +0x2704 cells of 100 x 32
	Sprite*			mMonitorMask;			// +0x271c atomicusmonitormask
	Sprite*			mMonitorBuffer;			// +0x2720 a clear image of the mask's size
	Sprite*			mMonitorBlank;			// +0x2724 the monitor picture's face (0,48 150 x 150)
	Sprite*			mSmallHeadSheet;		// +0x2728 smallatomicushead
	Sprite*			mSmallHead[11];			// +0x272c cells of 112 x 123
	Sprite*			mLeftNodSheet;			// +0x2758 atomicusleftnod
	Sprite*			mRightNodSheet;			// +0x275c atomicusrightnod
	Sprite*			mLeftNod[8];			// +0x2760 cells of 110 x 123, in the order 0 1 2 3 4 3 2 1
	Sprite*			mRightNod[8];			// +0x2780 cells of 112 x 123
	Sprite*			mCoat;					// +0x27a0
	Sprite*			mMouthSheet;			// +0x27a4 atomicmouth
	Sprite*			mMouthLeftSheet;		// +0x27a8 atomicmouthleft
	Sprite*			mMouth[50];				// +0x27ac cells of 50 x 38
	Sprite*			mMouthLeft[50];			// +0x2874
	Sprite*			mNukeFlash;				// +0x2940
	Sprite*			mSpark;					// +0x2944
	Sprite*			mMiniSpark;				// +0x2948
	Sprite*			mFreeze;				// +0x294c
	Sprite*			mElementalSheet;		// +0x2950
	Sprite*			mElemental[5][16];		// +0x2954 cells of 55 x 55
	Sprite*			mBigWordsSheet;			// +0x24d8
	Sprite*			mBigWords[3];			// +0x24dc parts of it
	Sprite*			mBonusLevelsSheet;		// +0x2a94
	Sprite*			mBonusLevels[160];		// +0x2a98 cells of 61 x 61
	Sprite*			mBonusNucleusSheet;		// +0x2d18
	Sprite*			mBonusNucleus[160];		// +0x2d1c
	Sprite*			mBonusWordsSheet;		// +0x2f9c
	Sprite*			mBonusWords[5];			// +0x2fa0 cells of 278 x 50 (0, 1, 4, 2, 3)
	Sprite*			mAtomicExplosionSheet;	// +0x2fb4
	Sprite*			mAtomicExplosion[16];	// +0x2fb8 cells of 100 x 100
	Sprite*			mClouds;				// +0x2ffc
	Sprite*			mCloud[6];				// +0x3000
	Sprite*			mGeigerGlow;			// +0x2ff8
	Sprite*			mLevelAndScore;			// +0x3018
	Sprite*			mGameTypeSheet;			// +0x302c gametypeimages
	Sprite*			mGameType[3];			// +0x3030
	Sprite*			mBombSymbol;			// +0x3050 AtomicBombSymbol
	Sprite*			mRankSheet;				// +0x3054 RankStrip
	Sprite*			mRank[22];				// +0x3058 cells of 110 x 110
	Sprite*			mGreenAtomicus;			// +0x30b0
	Sprite*			mScoreArrowsSheet;		// +0x30b4 HighScoreArrows
	Sprite*			mScoreArrows[2];		// +0x30b8
	Sprite*			mTextEdit;				// +0x30c0 HSTextEdit
	Sprite*			mCancelTip;				// +0x30c4
	Sprite*			mTipGraphicsSheet;		// +0x30c8
	Sprite*			mTipGraphics[5];		// +0x30cc cells of 64 x 64 (stored as 0, 2, 4, 3, 1 from +0x30cc)
	Sprite*			mMakeGroups;			// +0x30e0
	Sprite*			mMouseOver;				// +0x30e4
	Sprite*			mMagnetSheet;			// +0x30e8
	Sprite*			mMagnet[8];				// +0x30ec cells of 77 x 77
	Sprite*			mExpertSheet;			// +0x310c
	Sprite*			mExpert[2];				// +0x3110
	Sprite*			mLevelNumbersSheet;		// +0x3118
	Sprite*			mLevelNumber[10][2];	// +0x311c cells of 12 x 19 (the digits, two sets)
	Sprite*			mLevelWord;				// +0x316c a piece of the same sheet

	// ---- the sounds
	Sound*			mSndAtomAppear;			// +0x3170
	Sound*			mSndCreateMolecule;		// +0x3174
	Sound*			mSndSelectAtom;			// +0x3178
	Sound*			mSndWhoosh;				// +0x317c
	Sound*			mSndLowWhoosh;			// +0x3180
	Sound*			mSndLevelUp;			// +0x3184 voices\levelup
	Sound*			mSndCapClick;			// +0x3188
	Sound*			mSndMoveAtom;			// +0x318c
	Sound*			mSndMoveBioAtom;		// +0x3190
	Sound*			mSndCombo[7];			// +0x3194 voices: combo, super, mega, ultra, hyper, giga, max
	Sound*			mSndAtomicBomb;			// +0x31b0 voices\atomicbomb
	Sound*			mSndNukeBlast;			// +0x31b4
	Sound*			mSndGameOver;			// +0x31b8 voices\gameover
	Sound*			mSndElectric[3];		// +0x31bc
	Sound*			mSndGo;					// +0x31c8 voices\go
	Sound*			mSndGeiger1;			// +0x31cc
	Sound*			mSndGeiger2;			// +0x31d0
	Sound*			mSndHeartbeat;			// +0x31d4
	Sound*			mSndSwoop;				// +0x31d8
	Sound*			mSndDoorSlam;			// +0x31dc
	Sound*			mSndDoorOpen;			// +0x31e0
	Sound*			mSndFreeze;				// +0x31e4
	Sound*			mSndBombReady;			// +0x31e8 voices\atomicbombready
	Sound*			mSndBonusMolecule;		// +0x31ec voices\bonusmolecule
	Sound*			mSndBiohazard;			// +0x31f0 voices\biohazard
	Sound*			mSndBonusReady;			// +0x31f4 voices\bonusmoleculeready
	Sound*			mSndBonusUpgraded;		// +0x31f8 voices\bonusmoleculeupgraded
	Sound*			mSndGetReady;			// +0x31fc voices\getready
	Sound*			mSndFire;				// +0x3200 voices\fire
	Sound*			mSndIce;				// +0x3204 voices\ice
	Sound*			mSndLightning;			// +0x3208 voices\lightning
	Sound*			mSndWind;				// +0x320c voices\wind
	Sound*			mSndEther;				// +0x3210 voices\ether
	Sound*			mSndNewHighScore;		// +0x3214 voices\anewhighscore
	Sound*			mSndCool;				// +0x3218 voices\cool
	Sound*			mSndExcellent;			// +0x321c
	Sound*			mSndGreat;				// +0x3220
	Sound*			mSndNifty;				// +0x3224
	Sound*			mSndTerrific;			// +0x3228
	Sound*			mSndWicked;				// +0x322c
	Sound*			mSndMagnet;				// +0x3230
	SoundInstance*	mGeiger1Instance;		// +0x3234 the looping geiger counters
	SoundInstance*	mGeiger2Instance;		// +0x3238

public:
	AtomicaApp();
	virtual ~AtomicaApp();

	virtual void		Init() override;
	virtual void		LoadingThreadProc() override;
	virtual void		LoadingThreadCompleted() override;
	virtual void		WriteToRegistry() override;
	virtual void		ReadFromRegistry() override;
	// a click as a script gives it (the test hooks)
	void				ScriptClick(int theX, int theY);
	virtual void		HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;
	virtual void		UpdateFrames() override;

	// Test hooks (testhooks/Autoplay.cpp): -autoplay=<seed> plays games by itself with a bot that has its own random numbers
	bool				mAutoplay;
	int					mAutoplayMode;
	int					mAutoplayGames;
	int					mAutoplayCap;
	int					mAutoplayFinish;
	bool				mAutoplayBonus;
	std::string			mAutoplayLog;
	std::string			mAutoplayShots;
	std::string			mAutoplayScript;
	struct AutoplayState*	mAutoplayState;
	void				AutoplayUpdate();
	// testhooks/Trace.cpp: -statetrace=<file> writes every change of the state the identical-play pass compares with the original
	std::string			mTraceFile;
	struct TraceState*	mTraceState;
	void				TraceUpdate();

	void				TitleScreenDone();					// the title screen's play link was clicked
	void				StartGame();						// 0x405b10
	bool				HasSavedGame();
	void				SaveGame();							// 0x405fc0
	void				ResumeGame();						// 0x406060
	void				DeleteSavedGame();
	void				ShowQuit();							// 0x405bd0
	void				ShowOptions();						// 0x405d00
	void				ResetHints();
	void				ShowHighScores();					// 0x422d40
	void				ShowHighScoreEntry(int theScore);	// 0x41e440
	void				ReadHighScores();					// 0x405ed0
	void				WriteHighScores();					// 0x405f30
	void				ShowConfirm();						// 0x405c40
	void				ShowMainMenu(bool theGameOver);		// 0x405d70
	void				ShowGameOver();						// 0x406200
	void				ShowLevelUp();						// 0x432240
	void				CloseMenu();						// 0x405e90
	void				CloseBoard();						// 0x405e20
	void				EndGame();							// 0x405cb0

private:
	void				LoadAtoms();						// the atoms' pictures (0x407?-0x408ca0)
	Sprite*				BlendSprites(Sprite* theA, Sprite* theB, double theWeight); // 0x437c50
	void				SubtractMask(Sprite* theSprite, Sprite* theMask);		// 0x406110
	void				ShrinkInto(Sprite* theSource, Sprite* theDest, int theStep); // the shrinking loops of 0x406f90
	Sound*				LoadSound(const char* theName);
	void				PumpLoading();						// port: the loading runs in the main thread, so it draws the title
};

extern AtomicaApp* gApp;

// the autoplay bot's state (testhooks/Autoplay.cpp)
struct AutoplayState* NewAutoplayState(unsigned int theSeed);

}

#endif
