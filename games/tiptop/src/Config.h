#ifndef __TIPTOP_CONFIG_H__
#define __TIPTOP_CONFIG_H__

// The game's configuration (0x412ca6-0x418091): the text resource 0x11 of group 3 in TipTop.dat holds the rules,
// the layout of every screen, the tables of scores and effects and the levels of the four kinds of game, as
// "KEY = value;" lines and four lists of levels after '@' marks. LoadConfig reads it into gConfig (0x4888a8); the
// levels go to gLevels, the initial high scores to gHighScores.
//
// The fields are named after their keys (read from the exe by GameText, like the other strings) and keep the
// original's offsets in comments, which is how the game code refers to them (DAT_<0x4888a8 + offset>). The layout is
// the original's (sizeof 0x12a4, checked below): the mUnknown arrays fill the offsets that no key sets, which hold a
// level's values in the board's copy (Board.h) and the bonus round's in gConfig.

#include <cstdint>

namespace tiptop
{

// A *_MOTION entry: three numbers and an integer
struct Motion
{
	float mParam[3];        // +0x00
	int32_t mTime;          // +0x0c ms a step
};

// A PARTICLES record (0x20 bytes, 0x417dd2)
struct Particle
{
	int32_t mValue0;        // +0x00
	float mValue1;          // +0x04
	float mValue2;          // +0x08
	uint32_t mColor1;       // +0x0c r << 16 | g << 8 | b
	uint32_t mColor2;       // +0x10
	int32_t mValue5;        // +0x14
	int32_t mValue6;        // +0x18
	char mValue7;           // +0x1c
	char mValue8;           // +0x1d
};

// A COLUMNNUKE_EFFECTS / COLORBOMB_EFFECTS record (0x24 bytes, 0x418091)
struct Effect
{
	int32_t mValue0;        // +0x00
	int32_t mValue1;        // +0x04
	int32_t mValue2;        // +0x08
	int32_t mValue3;        // +0x0c
	char mValue4;           // +0x10
	char mValue5;           // +0x11
	int32_t mValue6;        // +0x14
	int32_t mValue7;        // +0x18
	int32_t mValue8;        // +0x1c
	int32_t mValue9;        // +0x20
};

struct EffectList
{
	char mFlag;             // +0x00
	int32_t mValue;         // +0x04
	uint32_t mCount;        // +0x08
	Effect* mEffects;       // +0x0c
};

// A level of one kind of game (0x70 bytes, 0x412ca6). '#' in the list repeats the value of the level before.
struct Level
{
	int32_t m00;            // +0x00
	int32_t m04;            // +0x04
	int32_t m08;            // +0x08
	int32_t m0c;            // +0x0c
	float m10;              // +0x10
	float m14;              // +0x14
	int32_t m18[16];        // +0x18
	int32_t m58;            // +0x58
	int32_t m5c;            // +0x5c
	int32_t m60;            // +0x60
	int32_t m64;            // +0x64 the third list only
	float m68;              // +0x68 the third list only
	int32_t m6c;            // +0x6c the third list only
};

// A high score (0x1c bytes); four tables of five at 0x4884b0
struct HighScore
{
	char mName[0x14];       // +0x00
	int32_t mScore;         // +0x14
	int32_t mLevel;         // +0x18
};

struct Config
{
	char mPrimaryStopCenter;                    // +0x0
	char mSecondaryStopCenter;                  // +0x1
	uint8_t mUnknown2[0xa];                    // +0x2
	char mLockFloaters;                         // +0xc
	char mLockFallers;                          // +0xd
	int32_t mMaxMatchOrdinal;                   // +0x10
	int32_t mSecondaryDistance;                 // +0x14
	int32_t mSecondaryCarry;                    // +0x18
	int32_t mNumColumns;                        // +0x1c
	int32_t mNumRows;                           // +0x20
	int32_t mFallerTapRows;                     // +0x24
	int32_t mFloaterTapRows;                    // +0x28
	int32_t mRocketTapRows;                     // +0x2c
	int32_t mHammerTapRows;                     // +0x30
	int32_t mExploderTapRows;                   // +0x34
	int32_t mMagnetTapRows;                     // +0x38
	int32_t mRainbowTapRows;                    // +0x3c
	int32_t mColorbombTapRows;                  // +0x40
	int32_t mColumnnukeTapRows;                 // +0x44
	Motion mPrimaryfallerMotion;                // +0x48
	Motion mSecondaryfallerMotion;              // +0x58
	Motion mFloaterMotion;                      // +0x68
	Motion mRocketMotion;                       // +0x78
	Motion mExfaMotion;                         // +0x88
	Motion mExflMotion;                         // +0x98
	Motion mHammerMotion;                       // +0xa8
	Motion mMagnetMotion;                       // +0xb8
	Motion mRainbowMotion;                      // +0xc8
	Motion mColorbombMotion;                    // +0xd8
	Motion mColumnnukeMotion;                   // +0xe8
	int32_t mTapSpeed;                          // +0xf8
	int32_t mTapDelay;                          // +0xfc
	int32_t mSecondaryfallInitdelay;            // +0x100
	int32_t mSecondaryfallAdddelay;             // +0x104
	int32_t mSecondaryfallRowdelay;             // +0x108
	int32_t mRemoveDelay;                       // +0x10c
	uint8_t mUnknown110[0x8];                  // +0x110 (a level's values in the board's copy: Board.h)
	int32_t mBonusTime;                        // +0x118 the bonus round's seconds (a level's goal in the board's copy)
	int32_t mGroupSize;                         // +0x11c
	uint8_t mUnknown120[0x4];                  // +0x120
	int32_t mDequeueFloatDelay;                 // +0x124
	char mWinningPrecedence;                    // +0x128
	int32_t mStartLevel;                        // +0x12c
	uint8_t mUnknown130[0x48];                 // +0x130
	int32_t mExploderRadius;                    // +0x178
	char mRandomBlocks;                         // +0x17c
	char mFloatersMatch;                        // +0x17d
	char mFloaterMatchesCreateFloats;           // +0x17e
	char mFloaterMatchesAwardPoints;            // +0x17f
	char mFallingExploderFall;                  // +0x180
	char mFloatingExploderFall;                 // +0x181
	char mColorbombFall;                        // +0x182
	char mFallingExploderCreateFloats;          // +0x183
	char mFloatingExploderCreateFloats;         // +0x184
	char mColorbombCreateFloats;                // +0x185
	char mColumnnukeCreateFloats;               // +0x186
	char mExploderPoints;                       // +0x187
	char mColumnnukePoints;                     // +0x188
	char mColorbombPoints;                      // +0x189
	uint8_t mUnknown18a[0x6];                  // +0x18a
	int32_t mGamewindowX;                       // +0x190
	int32_t mGamewindowY;                       // +0x194
	int32_t mBlockWidth;                        // +0x198
	int32_t mBlockHeight;                       // +0x19c
	int32_t mIngameInstructionsx;               // +0x1a0
	int32_t mIngameInstructionsy;               // +0x1a4
	int32_t mIngameOptionsx;                    // +0x1a8
	int32_t mIngameOptionsy;                    // +0x1ac
	int32_t mIngameEndgamex;                    // +0x1b0
	int32_t mIngameEndgamey;                    // +0x1b4
	int32_t mIngamePausex;                      // +0x1b8
	int32_t mIngamePausey;                      // +0x1bc
	int32_t mIngameNextblocksx;                 // +0x1c0
	int32_t mIngameNextblocksy;                 // +0x1c4
	int32_t mOptionsContinuex;                  // +0x1c8
	int32_t mOptionsContinuey;                  // +0x1cc
	uint8_t mUnknown1d0[0x10];                 // +0x1d0
	int32_t mOptionsTipsx;                      // +0x1e0
	int32_t mOptionsTipsy;                      // +0x1e4
	int32_t mOptionsFullscreenx;                // +0x1e8
	int32_t mOptionsFullscreeny;                // +0x1ec
	int32_t mOptionsVolumex;                    // +0x1f0
	int32_t mOptionsVolumewidth;                // +0x1f4
	int32_t mOptionsSoundy;                     // +0x1f8
	int32_t mOptionsMusicy;                     // +0x1fc
	// The members named mRegistrationUnused are the settings of the original's registration and trial and of its links out of
	// the game, which the port took out (GAME_NOTES.md, "Registration out"). They stay as padding of the same size: Board
	// reaches Config by offset.
	char mRegistrationUnused200[16];  // +0x200 16 bytes: mOptionsRegisterx..mOptionsVisitpopcapy
	int32_t mBkghix;                            // +0x210
	int32_t mBkghiy;                            // +0x214
	int32_t mBkgcolyoffset;                     // +0x218
	int32_t mBkgcolShrink;                      // +0x21c
	int32_t mBkgcolGlowradius;                  // +0x220
	int32_t mBkgcolLengthNormal;                // +0x224
	int32_t mBkgcolLengthTimed;                 // +0x228
	int32_t mBkgcolLengthSpeed;                 // +0x22c
	int32_t mBkgcolLengthStrategy;              // +0x230
	int32_t mIngameScorex;                      // +0x234
	int32_t mIngameScorey;                      // +0x238
	int32_t mIngameLevelx;                      // +0x23c
	int32_t mIngameLevely;                      // +0x240
	int32_t mBlockgridX;                        // +0x244
	int32_t mBlockgridY;                        // +0x248
	int32_t mTipsBackgroundx;                   // +0x24c
	int32_t mTipsBackgroundy;                   // +0x250
	int32_t mTipsTogglex;                       // +0x254
	int32_t mTipsToggley;                       // +0x258
	int32_t mTipsPrevx;                         // +0x25c
	int32_t mTipsPrevy;                         // +0x260
	int32_t mTipsNextx;                         // +0x264
	int32_t mTipsNexty;                         // +0x268
	int32_t mTipsOkx;                           // +0x26c
	int32_t mTipsOky;                           // +0x270
	int32_t mTipsTextx;                         // +0x274
	int32_t mTipsTexty;                         // +0x278
	int32_t mTipsTextwidth;                     // +0x27c
	int32_t mTipsTextheight;                    // +0x280
	int32_t mHighscoresBgx;                     // +0x284
	int32_t mHighscoresBgy;                     // +0x288
	int32_t mHighscoresTextx;                   // +0x28c
	int32_t mHighscoresTexty;                   // +0x290
	int32_t mHighscoresOkx;                     // +0x294
	int32_t mHighscoresOky;                     // +0x298
	int32_t mInstructionsNextx;                 // +0x29c
	int32_t mInstructionsNexty;                 // +0x2a0
	int32_t mInstructionsPrevx;                 // +0x2a4
	int32_t mInstructionsPrevy;                 // +0x2a8
	int32_t mInstructionsOkx;                   // +0x2ac
	int32_t mInstructionsOky;                   // +0x2b0
	int32_t mCreditsOkx;                        // +0x2b4
	int32_t mCreditsOky;                        // +0x2b8
	int32_t mMainmenuHighscorex;                // +0x2bc
	int32_t mMainmenuHighscorey;                // +0x2c0
	int32_t mMainmenuNewgamex;                  // +0x2c4
	int32_t mMainmenuNewgamey;                  // +0x2c8
	int32_t mMainmenuOptionsx;                  // +0x2cc
	int32_t mMainmenuOptionsy;                  // +0x2d0
	int32_t mMainmenuInstructionsx;             // +0x2d4
	int32_t mMainmenuInstructionsy;             // +0x2d8
	int32_t mMainmenuCreditsx;                  // +0x2dc
	int32_t mMainmenuCreditsy;                  // +0x2e0
	int32_t mMainmenuQuitx;                     // +0x2e4
	int32_t mMainmenuQuity;                     // +0x2e8
	int32_t mMainmenuHsRegulary;                // +0x2ec
	int32_t mMainmenuHsTimedy;                  // +0x2f0
	int32_t mMainmenuHsSpeedy;                  // +0x2f4
	int32_t mMainmenuHsStrategyy;               // +0x2f8
	int32_t mMainmenuHsNamex;                   // +0x2fc
	int32_t mMainmenuHsScorex;                  // +0x300
	int32_t mMainmenuHsLevelx;                  // +0x304
	int32_t mBlockgridCenterX;                  // +0x308 BLOCKGRID_X + the grid's half width (set after loading)
	int32_t mBlockgridCenterY;                  // +0x30c
	int32_t mAreyousureX;                       // +0x310
	int32_t mAreyousureY;                       // +0x314
	int32_t mAreyousureOkx;                     // +0x318
	int32_t mAreyousureOky;                     // +0x31c
	int32_t mAreyousureCancelx;                 // +0x320
	int32_t mAreyousureCancely;                 // +0x324
	char mRegistrationUnused328[64];  // +0x328 64 bytes: mAreyousureregX..mAreyousureregsaveRegistery
	int32_t mSavegameX;                         // +0x368
	int32_t mSavegameY;                         // +0x36c
	int32_t mSavegameOkx;                       // +0x370
	int32_t mSavegameOky;                       // +0x374
	int32_t mSavegameCancelx;                   // +0x378
	int32_t mSavegameCancely;                   // +0x37c
	int32_t mDeletegameX;                       // +0x380
	int32_t mDeletegameY;                       // +0x384
	int32_t mDeletegameOkx;                     // +0x388
	int32_t mDeletegameOky;                     // +0x38c
	int32_t mDeletegameCancelx;                 // +0x390
	int32_t mDeletegameCancely;                 // +0x394
	char mFallfloatAdjust;                      // +0x398
	char mDisableStops;                         // +0x399
	char mSecondaryUnlimited;                   // +0x39a
	int32_t mMagnetEffect;                      // +0x39c
	int32_t mScoreEmptyrowsInitial;             // +0x3a0
	int32_t mScoreEmptyrowsCoefficient;         // +0x3a4
	int32_t mScoreRemovedbyspecial;             // +0x3a8
	uint8_t mUnknown3ac[0x8];                  // +0x3ac
	int32_t mProgressbarX;                      // +0x3b4
	int32_t mProgressbarY;                      // +0x3b8
	int32_t mProgressbarTopclip;                // +0x3bc
	int32_t mNewgameNormalx;                    // +0x3c0
	int32_t mNewgameNormaly;                    // +0x3c4
	int32_t mNewgameTimedx;                     // +0x3c8
	int32_t mNewgameTimedy;                     // +0x3cc
	int32_t mNewgameSpeedx;                     // +0x3d0
	int32_t mNewgameSpeedy;                     // +0x3d4
	int32_t mNewgameStrategyx;                  // +0x3d8
	int32_t mNewgameStrategyy;                  // +0x3dc
	int32_t mNewgameMainx;                      // +0x3e0
	int32_t mNewgameMainy;                      // +0x3e4
	int32_t mNewgameContinuex;                  // +0x3e8
	int32_t mNewgameContinuey;                  // +0x3ec
	int32_t mNewgameSummaryboxx;                // +0x3f0
	int32_t mNewgameSummaryboxy;                // +0x3f4
	int32_t mNewgameSummarydatex;               // +0x3f8
	int32_t mNewgameSummarydatey;               // +0x3fc
	int32_t mNewgameSummarytypex;               // +0x400
	int32_t mNewgameSummarytypey;               // +0x404
	int32_t mNewgameSummarylevelx;              // +0x408
	int32_t mNewgameSummarylevely;              // +0x40c
	int32_t mNewgameSummarycombox;              // +0x410
	int32_t mNewgameSummarycomboy;              // +0x414
	int32_t mNewgameSummaryscorex;              // +0x418
	int32_t mNewgameSummaryscorey;              // +0x41c
	int32_t mHighscoresmenuClearhighscoresx;    // +0x420
	int32_t mHighscoresmenuClearhighscoresy;    // +0x424
	int32_t mHighscoresmenuMainx;               // +0x428
	int32_t mHighscoresmenuMainy;               // +0x42c
	int32_t mHighscoresmenuNormaly;             // +0x430
	int32_t mHighscoresmenuTimedy;              // +0x434
	int32_t mHighscoresmenuSpeedy;              // +0x438
	int32_t mHighscoresmenuStrategyy;           // +0x43c
	int32_t mHighscoresmenuSkipy;               // +0x440
	int32_t mHighscoresmenuNamex;               // +0x444
	int32_t mHighscoresmenuScorex;              // +0x448
	int32_t mHighscoresmenuLevelx;              // +0x44c
	int32_t mPlayagainX;                        // +0x450
	int32_t mPlayagainY;                        // +0x454
	int32_t mPlayagainYesx;                     // +0x458
	int32_t mPlayagainYesy;                     // +0x45c
	int32_t mPlayagainNox;                      // +0x460
	int32_t mPlayagainNoy;                      // +0x464
	uint8_t mUnknown468[0xc];                  // +0x468
	float mNormalRocketRamp;                    // +0x474
	float mNormalExfaRamp;                      // +0x478
	float mNormalExflRamp;                      // +0x47c
	float mNormalHammerRamp;                    // +0x480
	float mNormalMagnetRamp;                    // +0x484
	float mNormalRainbowRamp;                   // +0x488
	float mNormalColorbombRamp;                 // +0x48c
	float mNormalColumnnukeRamp;                // +0x490
	float mNormalRndfltRamp;                    // +0x494
	int32_t mNormalRndfltMin;                   // +0x498
	float mTimedRocketRamp;                     // +0x49c
	float mTimedExfaRamp;                       // +0x4a0
	float mTimedExflRamp;                       // +0x4a4
	float mTimedHammerRamp;                     // +0x4a8
	float mTimedMagnetRamp;                     // +0x4ac
	float mTimedRainbowRamp;                    // +0x4b0
	float mTimedColorbombRamp;                  // +0x4b4
	float mTimedColumnnukeRamp;                 // +0x4b8
	float mTimedRndfltRamp;                     // +0x4bc
	int32_t mTimedRndfltMin;                    // +0x4c0
	float mTimedPlpsecRamp;                     // +0x4c4
	float mSpeedRocketRamp;                     // +0x4c8
	float mSpeedExfaRamp;                       // +0x4cc
	float mSpeedExflRamp;                       // +0x4d0
	float mSpeedHammerRamp;                     // +0x4d4
	float mSpeedMagnetRamp;                     // +0x4d8
	float mSpeedRainbowRamp;                    // +0x4dc
	float mSpeedColorbombRamp;                  // +0x4e0
	float mSpeedColumnnukeRamp;                 // +0x4e4
	float mSpeedRndfltRamp;                     // +0x4e8
	int32_t mSpeedRndfltMin;                    // +0x4ec
	float mStrategyRocketRamp;                  // +0x4f0
	float mStrategyExfaRamp;                    // +0x4f4
	float mStrategyExflRamp;                    // +0x4f8
	float mStrategyHammerRamp;                  // +0x4fc
	float mStrategyMagnetRamp;                  // +0x500
	float mStrategyRainbowRamp;                 // +0x504
	float mStrategyColorbombRamp;               // +0x508
	float mStrategyColumnnukeRamp;              // +0x50c
	float mStrategyRndfltRamp;                  // +0x510
	int32_t mStrategyRndfltMin;                 // +0x514
	int32_t mTopcoverX;                         // +0x518
	int32_t mTopcoverY;                         // +0x51c
	int32_t mBottomcoverX;                      // +0x520
	int32_t mBottomcoverY;                      // +0x524
	int32_t mWarningshake0;                     // +0x528
	int32_t mWarningshake1;                     // +0x52c
	int32_t mWarningshake2;                     // +0x530
	int32_t mWarningshake3;                     // +0x534
	int32_t mWarningshake4;                     // +0x538
	int32_t mRemovingshake;                     // +0x53c
	int32_t mBkgcolx[8];                        // +0x540
	int32_t mBkgcoldx[8];                       // +0x560
	int32_t mFloaterprobability[8];             // +0x580
	uint32_t mMainmenuanimationsCount;          // +0x5a0 records of 6
	int32_t* mMainmenuanimations;               // +0x5a4
	uint32_t mNewgamemenuanimationsCount;       // +0x5a8
	int32_t* mNewgamemenuanimations;            // +0x5ac
	uint32_t mHighscoresmenuanimationsCount;    // +0x5b0
	int32_t* mHighscoresmenuanimations;         // +0x5b4
	uint32_t mOptionsmenuanimationsCount;       // +0x5b8
	int32_t* mOptionsmenuanimations;            // +0x5bc
	uint32_t mInstructionsmenuanimationsCount;  // +0x5c0
	int32_t* mInstructionsmenuanimations;       // +0x5c4
	uint32_t mCreditsmenuanimationsCount;       // +0x5c8
	int32_t* mCreditsmenuanimations;            // +0x5cc
	int32_t mMusicDelay;                        // +0x5d0
	int32_t mSplashDelay;                       // +0x5d4
	int32_t mSplashDisplacement;                // +0x5d8
	float mSplashCoefficient;                   // +0x5dc
	float mSplashSpringtime;                    // +0x5e0
	int32_t mSplashMovetime;                    // +0x5e4
	int32_t mFloatershake;                      // +0x5e8
	int32_t mScoremessageDelay;                 // +0x5ec
	float mScoremessageMoveexponent;            // +0x5f0
	float mScoremessageFadeexponent;            // +0x5f4
	int32_t mScoremessageDistance;              // +0x5f8
	int32_t mSplashLoadbarx;                    // +0x5fc
	int32_t mSplashLoadbary;                    // +0x600
	char mRegistrationUnused604[8];  // +0x604 8 bytes: mSplashRegisterx..mSplashRegistery
	int32_t mSplashPlayx;                       // +0x60c
	int32_t mSplashPlayy;                       // +0x610
	char mRegistrationUnused614[24];  // +0x614 24 bytes: mSplashTimesplayedx..mSplashUnregistered2y
	int32_t mSplashRegisteredx;                 // +0x62c
	int32_t mSplashRegisteredy;                 // +0x630
	char mRegistrationUnused634[72];  // +0x634 72 bytes: mSplashExecutionsleftx..mRegisterPastey
	int32_t mValidcodeBgx;                      // +0x67c
	int32_t mValidcodeBgy;                      // +0x680
	int32_t mValidcodeOkx;                      // +0x684
	int32_t mValidcodeOky;                      // +0x688
	int32_t mInvalidcodeBgx;                    // +0x68c
	int32_t mInvalidcodeBgy;                    // +0x690
	int32_t mInvalidcodeOkx;                    // +0x694
	int32_t mInvalidcodeOky;                    // +0x698
	int32_t mSummaryX;                          // +0x69c
	int32_t mSummaryY;                          // +0x6a0
	int32_t mSummaryPlayagainx;                 // +0x6a4
	int32_t mSummaryPlayagainy;                 // +0x6a8
	int32_t mSummaryTimex;                      // +0x6ac
	int32_t mSummaryLevelreachedx;              // +0x6b0
	int32_t mSummaryBestcombox;                 // +0x6b4
	int32_t mSummaryScorex;                     // +0x6b8
	int32_t mSummaryBestmatchx;                 // +0x6bc
	int32_t mSummaryTimey;                      // +0x6c0
	int32_t mSummaryLevelreachedy;              // +0x6c4
	int32_t mSummaryBestcomboy;                 // +0x6c8
	int32_t mSummaryScorey;                     // +0x6cc
	int32_t mSummaryBestmatchy;                 // +0x6d0
	char mRegistrationUnused6d4[28];  // +0x6d4 28 bytes: mInitialWait..mRisHiddenfilename
	int32_t mSummaryHsnamex;                    // +0x6f0
	int32_t mSummaryHsscorex;                   // +0x6f4
	int32_t mSummaryHslevelx;                   // +0x6f8
	int32_t mSummaryHsy;                        // +0x6fc
	int32_t mSummaryHsskipy;                    // +0x700
	char mRegistrationUnused704[24];  // +0x704 24 bytes: mSplashNomoreexecutionsx..mSplashNmeregistery
	int32_t mScoretable[100];                   // +0x71c
	int32_t mBlreducetable[100];                // +0x8ac
	int32_t mFlashDivisor;                      // +0xa3c
	int32_t mFlashDuration;                     // +0xa40
	int32_t mSummaryDelay;                      // +0xa44
	int32_t mSummaryFademovetime;               // +0xa48
	int32_t mSummaryMovedistance;               // +0xa4c
	int32_t mSummaryDuration;                   // +0xa50
	float mSummaryMotionexponent;               // +0xa54
	uint8_t mBonusQueue[0x44];                 // +0xa58 the bonus round's preview blocks (0x11 ends it)
	uint8_t mBonusLayout[0x80];                // +0xa9c the bonus round's blocks, row 0 first (0x11 empty)
	int32_t mNormalBonusmodulus;                // +0xb1c
	int32_t mTimedBonusmodulus;                 // +0xb20
	int32_t mSpeedBonusmodulus;                 // +0xb24
	int32_t mStrategyBonusmodulus;              // +0xb28
	int32_t mBonusTimecountx;                   // +0xb2c
	int32_t mBonusTimecounty;                   // +0xb30
	int32_t mBonusLargetimecountx;              // +0xb34
	int32_t mBonusLargetimecounty;              // +0xb38
	int32_t mBonustimerInitialmagnification;    // +0xb3c
	int32_t mBonustimerCriticaltime1;           // +0xb40
	int32_t mBonustimerCriticaltime2;           // +0xb44
	float mBonustimerMotionexponent;            // +0xb48
	int32_t mBonusScore;                       // +0xb4c the points of a block in a bonus round
	uint32_t mComboanimParamsCount;             // +0xb50 pairs
	int32_t* mComboanimParams;                  // +0xb54
	uint32_t mCombostarParamsCount;             // +0xb58 groups of 4
	int32_t* mCombostarParams;                  // +0xb5c
	uint32_t mStarburstParamsCount;             // +0xb60 groups of 4
	int32_t* mStarburstParams;                  // +0xb64
	int32_t mComboanimTable[100];               // +0xb68
	int32_t mStarburstTable[100];               // +0xcf8
	int32_t mScorefontTable[100];               // +0xe88
	int32_t mScorewordTable[100];               // +0x1018
	int32_t mNextfloaterBlinkperiod;            // +0x11a8
	int32_t mLevelcompleteDelay;                // +0x11ac
	int32_t mInitialSfxvolume;                  // +0x11b0
	int32_t mInitialMusicvolume;                // +0x11b4
	char mTurnbased;                            // +0x11b8
	uint32_t mParticleCount;                    // +0x11bc
	int32_t mLevelsummaryScorey;                // +0x11c0
	int32_t mLevelsummaryTimey;                 // +0x11c4
	int32_t mLevelsummaryComboy;                // +0x11c8
	int32_t mLevelsummaryMatchy;                // +0x11cc
	int32_t mLevelsummaryItemx;                 // +0x11d0
	int32_t mLevelsummaryDelay;                 // +0x11d4
	int32_t mLevelsummaryButtonx;               // +0x11d8
	int32_t mLevelsummaryButtony;               // +0x11dc
	char mLevelcompleteDisable;                 // +0x11e0
	Particle* mParticles;                       // +0x11e4
	EffectList mColumnnukeEffects;              // +0x11e8
	EffectList mColorbombEffects;               // +0x11f8
	int32_t mFloaterdeterminationNormal;        // +0x1208
	int32_t mFloaterdeterminationTimed;         // +0x120c
	int32_t mFloaterdeterminationSpeed;         // +0x1210
	int32_t mFloaterdeterminationStrategy;      // +0x1214
	char mRegistrationUnused1218[6];  // +0x1218 6 bytes: mExecutionlimit..mExecutionlimitAllowcontinue
	int32_t mLevelsummaryStartevent;            // +0x1220
	int32_t mLevelsummaryEndevent;              // +0x1224
	int32_t mLevelsummaryEntertime;             // +0x1228
	int32_t mLevelsummaryBouncetime;            // +0x122c
	int32_t mLevelsummaryFadetime;              // +0x1230
	float mLevelsummaryMaxcompression;          // +0x1234
	char mDeletegamewarningDisable;             // +0x1238
	int32_t mNormalShakesounds[5];              // +0x123c
	int32_t mTimedShakesounds[5];               // +0x1250
	int32_t mSpeedShakesounds[5];               // +0x1264
	int32_t mStrategyShakesounds[5];            // +0x1278
	int32_t mBkgcolMinbrightness;               // +0x128c
	int32_t mBkgcolMaxbrightness;               // +0x1290
	int32_t mBkgcolCycletime;                   // +0x1294
	int32_t mBkgcolResolutiontime;              // +0x1298
	int32_t mBkgcolAddbrightness;               // +0x129c
	char mRegistrationUnused12a0[1];  // +0x12a0 1 bytes: mDisableUnregistered
};

static_assert(sizeof(Config) == 0x12a4, "Config has the original's layout");

extern Config gConfig;                // 0x4888a8
extern HighScore gHighScores[4][5];   // 0x4884b0
extern Level* gLevels[4];             // 0x48848c, 0x4884a4, 0x4886e8, 0x488384
extern uint32_t gLevelCount[4];       // 0x48849c, 0x4886e4, 0x488490, 0x488488

// Reads the configuration; negative on failure
int LoadConfig(Config* theConfig);    // 0x413830

} // namespace tiptop

#endif
