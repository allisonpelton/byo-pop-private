#ifndef __TYPERSHARK_BOARD_H__
#define __TYPERSHARK_BOARD_H__

// Board.h: the game screen of Typer Shark: the sea, the diver, the sharks and piranhas with their words, the clams, the
// bonus creatures, the bosses, the score and the keyboard. It is the biggest class of the game (53 member functions, about
// 56 KB of code); games/typershark/docs/BOARD.md has the map of the methods, the split into translation jobs, the fields
// with what each holds and the event codes the event loops handle.
// Original: class Sexy::Board : public Widget (the only base; the 2004 Widget is 0x58 bytes), vftable 0x5309a4 (47 slots),
// sizeof 0xf98 (WinWSApp makes it with operator new(0xf98) and the constructor 0x4544b0). Every field comment gives the
// offset in the original's object, which the port's layout does not follow (its Widget, std::string, std::vector and
// std::map are other sizes); the field order is the original's.
// The lists of creatures hold the Enemy part of each creature (the original keeps the Enemy part at offset 0 of Shark,
// Piranha, Clam and BonusCreature; the Board casts a Shark or Piranha with dynamic_cast, see ENTITIES.md). The embedded
// helper objects (Smoother, BgLayer, RatingPopup, DepthMarker) are the original's anonymous structs, named by their role.

#include "Widget.h"
#include "WinWSApp.h"		// AppStats
#include "Player.h"
#include "HyperlinkWidget.h"
#include <map>
#include <string>
#include <vector>

namespace Sexy
{

class Bubble;
class BackgroundFish;
class BonusText;
class Enemy;
class Font;
class GameOverDialog;		// the dialog of the game's end (ctor 0x43adb0, job D1)
class Graphics;
class HudBar;				// the strip at the top of the screen (ctor 0x441730, Update 0x441760, Draw 0x449910; not a Board method)
class Image;
class LessonManager;
class WinWSApp;
class WSTimer;
struct EnemyRecord;			// LessonManager.h

// ---- added for B2 ----
// 0x41d350 (a free function; Board::Draw and TypingTutor::Draw call it; BoardDraw.cpp defines it): draws theText1 and then
// theText2 one character at a time from (theX, theY), each in its own font and colour, a character starting where the
// previous one's StringWidth ended. A NULL theFont1 means theFont2. When theCenter is set, theX is moved so that both
// strings together (measured with theFont1) are centred in theWidth. The original takes the strings and colours by value.
void DrawTwoColorString(Graphics* g, int theX, int theY, const std::string& theText1, const Color& theColor1, Font* theFont1,
	const std::string& theText2, const Color& theColor2, Font* theFont2, int theWidth, bool theCenter);

class Board : public Widget
{
public:
	// ---- the embedded objects (the original has no names for them) ----

	// 0x50 bytes: the last six changes of a value and their average, to ease its movement on the screen (Draw 0x44d200
	// feeds each BgLayer's mY through one, and the rating popup's mY through the FrameSmoother below).
	struct Smoother
	{
		double				mSamples[6];			// +0x00 ring of the last six changes
		int					mIndex;					// +0x30 next slot of mSamples (0 to 5)
		double				mAlwaysZero;				// +0x38 constructor 0; never read
		double				mPrev;					// +0x40 the value at the previous draw
		double				mSmoothed;				// +0x48 the eased value
	};

	// 0x48 bytes: the same ring for one value whose eased result lives in the RatingPopup (+0x08), so there is no mSmoothed.
	struct FrameSmoother
	{
		double				mSamples[6];			// +0x00 ring of the last six changes
		int					mIndex;					// +0x30 next slot of mSamples (0 to 5)
		double				mAlwaysZero;				// +0x38 constructor 0; never read
		double				mPrev;					// +0x40 the value at the previous draw
	};

	// 0x20 bytes: one of the two background strips (the sea behind the board); Draw moves and draws them.
	struct BgLayer
	{
		double				mX;						// +0x00 constructor 0; Draw draws the picture at this x
		double				mY;						// +0x08 constructor: the height of the strip's picture times its index (0, then 1)
		int					mAlwaysZeroInts[4];			// +0x10 four ints, constructor 0; never read
	};

	// 0x38 bytes: the "Great!" / "Excellent!" picture that rises after a group of words (ShowRating 0x43fd90 starts it,
	// Update ages it, Draw draws it). It also keeps the streak of good ratings.
	struct RatingPopup
	{
		double				mY;						// +0x00 the y it floats from (ctor: the Widget's +0x14; ShowRating: mFloorY - 50); Update takes 1.0 off per tick
		double				mSmoothY;				// +0x08 constructor 0; the eased mY (the FrameSmoother's result)
		double				mX;						// +0x10 constructor: the Widget's +0x10 divided by 2
		int					mAlpha;					// +0x18 constructor 0; ShowRating 255; Update takes 1 off per tick, the popup ends at 0
		Image*				mImage;					// +0x1c the picture (the app's mImages at byte offset 0x178, 0x174 or 0x170, i.e. indexes 0x5e, 0x5d, 0x5c, by the percent)
		int					mStreak;				// +0x20 words in a row rated 95% or more (GetStats saves it)
		int					mPercent;				// +0x24 the percent ShowRating was called with
		int					mPoints;				// +0x28 constructor 0; the points of the words typed since the last rating
		int					mBonus;					// +0x2c the bonus the popup gave (at most 20000)
		bool				mActive;				// +0x30 the popup is showing
		int					mNeverWritten;				// +0x34 never written or read (the original leaves it unset; the port zero-fills the popup)
	};

	// 0x30 bytes: a label of the depth ruler at the left edge ("100 ft"...), one for every 20 units of the ocean depth.
	struct DepthMarker
	{
		std::string			mText;					// +0x00 (0x1c bytes)
		int					mDepth;					// +0x1c the depth the label stands for
		double				mX;						// +0x20 constructor 0
		double				mY;						// +0x28 the y on the screen (Draw stops at the first one below the screen)
	};

	// ---- fields, in the original's order ----

	WSTimer*				mUnreadTimer;				// +0x58 a timer widget that nothing starts, stops or reads: the constructor makes it, AddedToManager adds it to the manager, RemovedFromManager takes it off
	int						mDrawCount;				// +0x5c ++ in every Draw
	int						mAlwaysZero;				// +0x60 constructor 0; never read
	int						mUpdateCount;			// +0x64 ++ at the start of every Update, before the paused return
	Smoother				mSmoothers[2];			// +0x68 one for each BgLayer (0x50 bytes each, to +0x108)
	FrameSmoother			mFrameSmoother;			// +0x108 for the RatingPopup's mY (0x48 bytes, to +0x150)
	// +0x150 MaxUnregAbyssDepth, the abyss depth limit of an unregistered copy: removed (registration)
	int						mWavePhase1;				// +0x154 ++ every other tick (constructor 0)
	int						mWavePhase2;				// +0x158 ++ every fourth tick (constructor 0)
	double					mWaveAmp;				// +0x160 the strength of the waves (4.0 to 5.0, moves by 0.004 every other tick; 440330 and 441ac0 use it twice over)
	bool					mWaveGrowing;			// +0x168 the direction mWaveAmp moves in (constructor true)
	GameOverDialog*			mGameOverDialog;		// +0x16c made by PlayerEaten when the last life is gone
	BgLayer					mBgLayers[2];			// +0x170 (0x20 bytes each, to +0x1b0)
	std::map<int, EnemyRecord*>	mRecordsByType;		// +0x1b0 (12 bytes) the wave's enemy record for each enemy type that SpawnWave made a creature of; cleared at every wave
	bool					mFastCPU;				// +0x1bc SetFastCPU (the constructor copies the app's flag): Draw uses the plain picture, not the wavy one, when it is off
	int						mLessonStepDepth;				// +0x1c0 constructor 0; SetDepth sets it to the start depth; in a game with no ocean floor (abyss, endless) Update sets it to the depth (mScore) each time the lesson goes on to its next level, which happens every 200 feet
	int						mScore;					// +0x1c4 the score; in an abyss game it is the depth in feet (SetDepth sets it)
	int						mEmpInterval;			// +0x1c8 milliseconds of mEmpTimer for each 0.01 of the diver's EMP charge: 2500, or 1250 in an abyss game
	double					mSinTable[360];			// +0x1d0 sin(i * 1/pi) for i = 0..359 (to +0xd10)
	double					mNearMountainsY;				// +0xd10 the y of the nearest mountain picture (the last one before the floor): constructor like the floor; SetLessonManager: mMidMountainsY + 150; Update scrolls it up with the dive, twice the diver's speed once the far range is on the screen
	double					mMidMountainsY;				// +0xd18 the y of the middle mountain picture: constructor like the floor; SetLessonManager: mFarMountainsY + 150; Update scrolls it up with the dive, 1.8 times the diver's speed once the far range is on the screen
	double					mFarMountainsY;				// +0xd20 the y of the farthest mountain picture (the first one drawn): constructor like the floor; SetLessonManager: mOceanFloorY - 30; Update scrolls it up with the dive, 1.6 times the diver's speed once it is on the screen
	bool					mAbyssBossDue;				// +0xd28 an abyss boss is due: constructor false; SetDepth sets it when the start depth is a multiple of 1000, Update sets it when the diver reaches a 1000-foot mark with no boss out (abyss game); SpawnWave clears it and in an abyss game builds the boss when it was set
	int						mAbyssBossIndex;				// +0xd2c which abyss boss is called for: constructor -1; the first Update takes it from the profile's saved AbyssBoss value, SetDepth adds the thousands of the start depth, Update adds one at each 1000-foot mark that calls a boss, and both save it back to the profile; SpawnWave takes it mod 4 to pick the boss (0 TorpedoBoss, 1 MechaShark, 2 MechaSquid, 3 GhostShip)
	int						mFadeAlpha;				// +0xd30 constructor 255; Update takes 2 off every other tick; the black fade-in at the start of a level
	WSTimer*				mGameTimer;				// +0xd34 starts disabled; SetVisible(true) starts it; its Elapsed is the game's time (GetStats gives it out)
	bool					mLetterTyped;				// +0xd38 constructor false; KeyChar sets it each time a letter is taken by a creature, a boss shot or the bonus word; never read
	double					mDiverSpeed;			// +0xd40 SetLessonManager: the <DiverSpeed> of the lesson; Update adds it to mDivePos every tick
	double					mDivePos;				// +0xd48 constructor 1.0; the depth reached in pixels (Update += mDiverSpeed)
	double					mDivePosMarked;			// +0xd50 constructor 0; mDivePos at the last time Update moved the score on (every 8.0)
	WinWSApp*				mApp;					// +0xd58 constructor argument
	unsigned int			mBoardSerial;			// port: the number of this board (one for each level), counted from 1 over the run, which the autosplit observer reads (LiveSplitObserver.cpp); nothing in the game reads it
	LessonManager*			mLessonManager;			// +0xd5c SetLessonManager
	bool					mAlwaysFalse;				// +0xd60 constructor false; never read
	int						mBonusCreaturesKilled;				// +0xd64 constructor 0; SpawnWave zeroes it for each wave; HandleBonusCreatureEvents adds one for every bonus creature whose word is finished and pays 200 points times the count (the rising tone follows the count too)
	Player*					mPlayer;				// +0xd68 the diver (Player.h), made by the constructor (operator new(0x148))
	Enemy*					mOpenClam;				// +0xd6c the first clam of a pair that was opened (the memory game of the clams); 0 when none
	int						mPairsFound;			// +0xd70 pairs of clams matched so far (the third match shows the treasure)
	std::vector<Enemy*>		mSharks;				// +0xd74 (pad +0xd74, begin +0xd78, end +0xd7c, capacity +0xd80) the Shark creatures
	std::vector<Enemy*>		mPiranhas;				// +0xd84 the Piranha creatures
	std::vector<Enemy*>		mClams;					// +0xd94 the Clam creatures
	std::vector<Bubble*>	mBubbles;				// +0xda4 AddBubble
	std::vector<BonusText*>	mBonusTexts;			// +0xdb4 AddBonusText
	std::vector<Enemy*>		mBonusCreatures;		// +0xdc4 the BonusCreature creatures (enemy record type 11)
	std::vector<Enemy*>		mRetired;				// port (leak fix, docs/LEAKS.md): the creatures the original takes off a list without deleting (a bonus creature that left through the top, a boss shot that is over); owned, freed with the board, since mTarget may still point to one
	std::vector<BackgroundFish*>	mBackgroundFish;	// +0xdd4 the fish that swim behind the sharks
	std::string				mTyped;					// +0xde4 (0x1c bytes) the letters typed so far on mTarget
	Enemy*					mTarget;				// +0xe00 the creature the typed letters belong to (0 when none)
	HudBar*					mHudBar;				// +0xe04 made by the constructor (operator new(0x1c), ctor 0x441730 with the player and the app)
	WSTimer*				mRatingTimer;			// +0xe08 the event loops stop or reset it when a group of words ends (ShowRating); Update stops and resets it as the RatingPopup fades
	double					mOceanFloorY;				// +0xe10 the y of the ocean floor picture, the end of the dive (-1 in an endless game): constructor the Widget's height; SetLessonManager works it out from mOceanDepth; Update takes mDiverSpeed off it each tick of the dive; Draw draws the floor at it and Update stops the dive when the screen's bottom reaches it
	WSTimer*				mTypeTimer;				// +0xe18 reset by KeyChar and the event loops; its Elapsed is added to mTypeTime when a word is finished
	int						mTypeTime;				// +0xe1c milliseconds spent typing (the app adds it to its own total after the game)
	AppStats				mStats;					// +0xe20 (0x30 bytes) the block GetStats hands out; GetStats fills it from the diver, mRating, mAccurateLevels, mPausedTime, mGameTimer, mLettersRight, mLettersWrong
	WSTimer*				mLevelTitleTimer;				// +0xe50 times the "Level N" title: starts disabled; the fade-in at the level's start and the diver's restart from the top (state 1) start it, and mLevelTitleAlpha fades once it has run 2000 ms (it is stopped when the title is gone); SetPaused adds the pause to it
	int						mLevelTitleAlpha;				// +0xe54 alpha of the "Level N" title (the secret level's name in a bonus level) that fades out after a level starts: constructor 0; the first fade-in sets 255 (0 for a player who has not seen the tutorial), PlayerEaten sets 255; Update takes 3 off a tick once the timer at +0xe50 has passed 2 s (diver in state 1 or 9, or diving); Draw draws the title while it is above 0
	int						mPausedTime;			// +0xe58 milliseconds the game spent paused (SetPaused adds the pause timer to it; GetStats gives it out)
	WSTimer*				mPauseTimer;			// +0xe5c SetPaused(true) resets it
	bool					mPaused;				// +0xe60 the game is paused (every creature's Draw and Update read it)
	int						mPauseCount;			// +0xe64 SetPaused(true) adds one
	WSTimer*				mBannerTimer;			// +0xe68 shows mBannerText; 2000 ms of it and mBannerLife counts down
	std::string				mBannerText;			// +0xe6c (0x1c bytes) a message printed on the screen (SetBannerText)
	int						mBannerLife;			// +0xe88 constructor 0; counts down by 2 each tick once mBannerTimer passes 2 s
	int						mAccurateLevels;				// +0xe8c levels of this game finished with 95% accuracy or more: constructor 0; Update adds one (and pays GetFinishBonus) when the level's end is reached; StartGame restores it and GetStats gives it out (AppStats word 5); GetFinishBonus and Draw read it
	int						mStartScore;				// +0xe90 the diver's score at the start of the game (constructor, StartGame); never read
	int						mPenaltyTypoCount;				// +0xe94 wrong letters counted toward a penalty piranha: KeyChar adds one for a typo in a piranha wave and, when it reaches the lesson's mNumForPenalty, zeroes it and calls AddPiranha; ResetPiranhaSpeed zeroes it; the state log writes it
	double					mPiranhaSpeed;			// +0xe98 1.0 after ResetPiranhaSpeed; AddPiranha multiplies it by 1.7 after the sixth piranha
	int						mPiranhaCount;			// +0xea0 piranhas AddPiranha made
	int						mHintAlpha;				// +0xea4 alpha of the opening hint text ("Type the words on the sharks...", shown while the player has not seen the tutorial and the diver is in state 0): constructor 255; Update takes 2 off a tick while mHintFading is set; Draw uses it
	bool					mHintFading;				// +0xea8 the opening hint is fading out: constructor false; KeyChar sets it when a shark or bonus creature takes a letter and the tutorial is not seen yet; Update clears it when mHintAlpha reaches 0
	WSTimer*				mEmpTimer;				// +0xeac every mEmpInterval ms the diver's EMP charge grows by 0.01
	int						mFlashAlpha;			// +0xeb0 a white flash over the screen: KeyChar sets 255, Update takes 8 off a tick, Draw draws it
	bool					mEmpDraining;				// +0xeb4 the EMP charge is draining: KeyChar sets it when the EMP is fired; Update takes 0.01 off the diver's charge a tick and clears it at 0
	bool					mEmpUsedInWave;				// +0xeb5 the EMP was fired in the current wave: constructor false; KeyChar sets it, SpawnWave clears it; while it is set the event loops add no typing time to mTypeTime and show no rating for the group
	Enemy*					mBoss;					// +0xeb8 the boss of the level (TorpedoBoss, GhostShip, MechaShark or MechaSquid), made by SpawnWave and the level start; 0 when none
	bool					mBossBeaten;				// +0xebc constructor false; HandleBossEvents sets it when a boss is beaten outside an abyss game; Update tests it before it stops the dive for the ocean floor and before it sends the next wave
	int						mSparkles[9];			// +0xec0 the three sparkles drawn over the treasure picture, three ints each (x offset, y offset, frame 0 to 13): constructor zeros, except [5] = random 4..7 and [8] = random 8..11 (the first frames); Update takes each offset pair from GetSpotOffset when the treasure shows and steps the frames every tenth tick; only Draw reads them
	int						mSonarStep;				// +0xee4 the step of the sonar sweep of the level-end screen: constructor 0; Update counts it up every sixth tick while the diver is in state 7 (each step changes the sweep pictures; at 1 the ping plays); IsHudActive tests it against 0x10
	Image*					mSonarTopRightImage;				// +0xee8 the sonar sweep picture drawn at the top right of the level-end screen's sonar: constructor the app's mImages at index 0xc9 (IMAGE_SONAR_SWEEP_0); Update takes the next picture at steps 1 to 4 of mSonarStep and clears it from step 5; Draw shows it unless it is NULL
	Image*					mSonarBottomRightImage;				// +0xeec the sonar sweep picture drawn at the bottom right of the sonar: constructor the app's mImages at index 0xce (IMAGE_SONAR_SWEEP2_0); Update takes the next picture at steps 5 to 8 of mSonarStep and clears it from step 9; Draw shows it unless it is NULL
	Image*					mSonarBottomLeftImage;				// +0xef0 the sonar sweep picture drawn at the bottom left of the sonar: constructor the first entry of the app's mSonarSweepImages (+0x7cc, a table of pictures); Update takes entries 1 to 4 of it at steps 9 to 12 of mSonarStep and clears it from step 13; Draw shows it unless it is NULL
	Image*					mSonarTopLeftImage;				// +0xef4 the sonar sweep picture drawn at the top left of the sonar: constructor the first entry of the app's mSonarSweep2Images (+0x7dc); Update takes entries 1 to 4 of it at steps 13 to 16 of mSonarStep and clears it from step 17; Draw shows it unless it is NULL
	int						mSonarAlpha;				// +0xef8 alpha of the sonar pictures of the level-end screen: constructor 255, nothing else writes it; Draw fades the pictures in with it when it is below 255 and shows the text at 255; Update runs the sweep only at 255
	bool					mCreaturePassDone;				// +0xefc constructor false; Update clears it at the start of each tick's pass over the creatures; nothing sets it to true and nothing reads it
	WSTimer*				mTreasureTimer;				// +0xf00 times the treasure screen: starts disabled; started and reset when the air tank runs out outside a bonus level (state 6) and when the diver is back at the bottom with the treasure (state 0xb to 0xc); after 3000 ms Update stops it and sets mTreasureFading; SetPaused adds the pause to it
	int						mTreasureAlpha;				// +0xf04 alpha of the treasure screen's picture and texts: constructor 255; Update takes 1 off a tick once mTreasureFading is set and at 0 puts the diver in state 7; Draw uses it
	bool					mTreasureFading;				// +0xf08 the treasure screen is fading out: constructor false; Update sets it when the timer at +0xf00 has run 3 s in state 6 or 0xc; Draw then draws the treasure picture with mTreasureAlpha
	int						mBonusWordPoints;				// +0xf0c points won by the bonus words: constructor 0; KeyChar adds GetStreakBonus for each word; Draw shows it on the treasure screen; Update keeps the best in the profile (+0x64)
	std::string				mBonusWord;			// +0xf10 (0x1c bytes) the word of the air-tank round (state 5 outside a bonus level): Update picks it with GetLongWord when it is empty, KeyChar clears it when mBonusWordPos reaches its length; Draw shows it split at mBonusWordPos
	int						mBonusWordPos;				// +0xf2c constructor 0; how many letters of the bonus word (the string at +0xf10) are typed so far: KeyChar counts it up to the word's length, Update zeroes it when a new word is chosen, Draw splits the word at it
	int						mBonusWordCount;				// +0xf30 bonus words typed whole in the air-tank round (diver state 5): constructor 0; KeyChar counts it (the tone rises with it); GetStreakBonus pays by it; Draw shows it as "Bonus words"; SaveProgress records it; Update keeps the best in the profile (+0x68)
	double					mAirGauge;				// +0xf38 the air tank's fill in pixels (0 to 240): constructor 0; Update adds 0.96 a tick in state 4 (the tank fills) and takes 0.1 off in state 5 (0.12 in a bonus level) until it is 0; KeyChar takes 5.0 off for a typo in the bonus word; Draw uses it as the width of the fill; the state log writes it
	bool					mAirFillRising;				// +0xf40 the glow of the air gauge's green fill is rising (true) or falling (false): constructor false; Update flips it at the ends of the pulse (alpha 255 and 170)
	int						mAirFillAlpha;				// +0xf44 alpha of the air gauge's green fill: constructor 255; Update pulses it between 170 and 255 in the last third of the countdown; Draw uses it
	int						mTreasureFrame;				// +0xf48 constructor 0; Update steps it from 0 to 9 and round again, one step every 11th tick while the diver is in state 6 or 0xc; never read
	std::vector<DepthMarker*>	mDepthMarkers;		// +0xf4c the labels of the depth ruler (SetLessonManager builds them, Update adds more)
	RatingPopup*			mRating;				// +0xf5c made by the constructor (operator new(0x38))
	int						mWaveLettersRight;				// +0xf60 letters typed right in the current wave: KeyChar counts it; SpawnWave zeroes it; ShowRating works the percent out of it
	int						mWaveLettersWrong;				// +0xf64 letters typed wrong in the current wave: KeyChar counts it; SpawnWave zeroes it; ShowRating works the percent out of it
	int						mLettersRight;				// +0xf68 letters typed right that count for the level's accuracy and typing speed (not in a piranha wave): KeyChar counts it; GetStats gives it out (AppStats word 8, which the app adds to its total); Draw, Update and GetTypingSpeed read it
	int						mLettersWrong;				// +0xf6c letters typed wrong that count against the level's accuracy (not in a piranha wave): constructor 0; KeyChar counts it; GetStats gives it out (AppStats word 9, which the app adds to its total); Draw, Update and GetTypingSpeed read it
	int						mBonusLettersRight;				// +0xf70 constructor 0; KeyChar counts the letters of the bonus word typed right; never read
	int						mBonusLettersWrong;				// +0xf74 constructor 0; KeyChar counts the letters of the bonus word typed wrong; never read
	bool					mFirstUpdate;			// +0xf78 constructor true; the first Update does its start-of-game work and clears it
	Image*					mTreasureImage;			// +0xf7c ShowTreasure picks one of the app's mImages at byte offset 0x310 + 4 * (0..3), indexes 0xc4 to 0xc7; Draw draws it
	bool					mScreenClear;				// +0xf80 no wave is on the screen (KeyChar ignores letters, Update may send the next wave): constructor true; AddPiranha and SpawnWave clear it; the event loops and Update set it again when the wave's creatures are gone
	int						mWaterImageHeight;				// +0xf84 constructor: the height of the app's water picture (mImages at byte offset 0x7c, index 0x1f); Update wraps the two background strips by it
	int						mSurfaceHeight;				// +0xf88 constructor: the +8 field of the app's mImages at byte offset 0x5c (index 0x17); IsBelowSurface compares mDivePos with it
	int						mOceanDepth;			// +0xf8c SetLessonManager: the <OceanDepth> of the lesson (-1 in an endless game)
	int						mFloorY;				// +0xf90 constructor: the app's +0x64 minus the +8 field of mImages[8] (the status bar picture): the bottom of the play field; the lanes of creatures are counted up from it
	HyperlinkWidget*		mContinueLink;			// +0xf94 (role guessed) a link widget (id 300, listener the app) that AddedToManager makes and adds; SetVisible and SetDisabled forward to it while the diver is in state 7; WinWSApp hides it while the options dialog is up in the diver states 6, 7 and 0xc; ShowTreasure brings it to the front

public:
	// ---- Board.cpp (job B2): the constructor, the Widget overrides, two small queries ----
	Board(WinWSApp* theApp);														// 0x4544b0
	virtual ~Board() override;														// 0x454e10 deleting destructor, body 0x451700

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x445eb0 slot 1: makes mContinueLink, adds the nine timers
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x43fbf0 slot 2: takes them off again
	virtual void			SetVisible(bool isVisible) override;							// 0x4401b0 slot 4
	virtual void			SetDisabled(bool isDisabled) override;							// 0x440250 slot 10
	virtual void			KeyDown(KeyCode theKey) override;								// 0x442520 slot 21 (empty)
	virtual void			MouseMove(int x, int y) override;								// 0x440c20 slot 25
	bool					IsHudActive();													// 0x440b70 (the StatusBar asks) false while the diver is in state 7 and mSonarStep > 0x10
	bool					IsEmpTimerPaused();												// 0x440cb0 (the StatusBar asks) mEmpTimer's paused flag

	// ---- BoardDraw.cpp (job B2): Draw and what only Draw calls ----
	virtual void			Draw(Graphics* g) override;										// 0x44d200 slot 16
	void					BuildWaterImage(Graphics* g);									// 0x440330 (role guessed) the wave effect when the board cannot draw wavy pictures (the original takes the Graphics and ignores it)
	void					DrawWavyImage(Graphics* g, Image* theImage, int theX, int theY);	// 0x441ac0 draws a picture row by row, shifted along mSinTable
	int						GetNextGoal();													// 0x440010 the next score goal above the diver's score (multiples of 150000, then fixed steps)
	std::string				GetRankTitle();													// 0x446950 the title for the score ("Sea Titan", "Leviathan"...)
	int						GetLevelBonus();												// 0x43fca0 (difficulty + 1) * the diver's mLevelNumber + 1 (11 once mLevelNumber is 12 or more) * 1000, at most 60000
	int						GetBossBonus();													// 0x43fce0 (role guessed) 10000, 30000, 50000, 70000 for the difficulties 0 to 3, 100000 otherwise
	int						GetFinishBonus();												// 0x440290 (role guessed) the end-of-level bonus from mAccurateLevels and the lives, at most 100000
	int						GetStreakBonus();												// 0x4423f0 points for a bonus round by difficulty and mBonusWordCount
	int						GetTypingSpeed();												// 0x43ff90 (role guessed) two double-to-int conversions of the typing time and counts

	// ---- BoardUpdate.cpp (job B1): Update and its helpers ----
	virtual void			Update() override;												// 0x45c140 slot 17
	bool					IsBelowSurface();												// 0x43fd30 mDivePos has passed mSurfaceHeight, the diver is hurt-free and in state 0
	void					ShowRating();													// 0x43fd90 starts the rating popup for a group of words (the percent is worked out inside)
	void					GetSpotOffset(int* theX, int* theY, int theIndex);				// 0x4408d0 (role guessed) one of four offset pairs by index; two tables, for app level 11 and the others
	void					UpdateDangerMusic(Enemy* theEnemy, bool theFlag);				// 0x440b90 switches the music to track 0x25 / 0x2b when the creature gets near the diver
	int						CountActiveLists();												// 0x441bb0 how many of mPiranhas, mSharks, mBonusCreatures are not empty
	void					ShowTreasure();													// 0x442760 the third pair of clams: diver state 6, the link widget to the front, the treasure picture
	bool					AvoidWordOverlap(Enemy* theEnemy);								// 0x446110 (role guessed) moves a creature so its word does not cover another's
	std::string				PickClamWord();													// 0x446720 a word for a new clam that no other clam has
	void					PlayerEaten();													// 0x44fe30 a life is lost: next try or the game-over dialog
	void					ResetPiranhaSpeed();											// 0x4402f0 mPiranhaSpeed 1.0, mPenaltyTypoCount 0

	// ---- BoardEvents.cpp (job B1): the event loops (BOARD.md, part 5); each returns true when it removed the creature from its list ----
	bool					HandleSharkEvents(Enemy* theEnemy);								// 0x457480
	bool					HandlePiranhaEvents(Enemy* theEnemy);							// 0x457ad0
	bool					HandleClamEvents(Enemy* theEnemy);								// 0x458030
	bool					HandleBonusCreatureEvents(Enemy* theEnemy);						// 0x4582c0
	bool					HandleBossEvents(Enemy* theEnemy);								// 0x4586b0

	// ---- BoardWaves.cpp (job B3): the wave spawner ----
	void					SpawnWave();													// 0x455bf0 fills the lists from the lesson's next wave (and builds the boss by type)
	void					PlaceEnemies(int theFirstNew);									// 0x452050 spreads the sharks and piranhas the spawner just added over the lanes
	void					PlaceBonusCreatures(int theFirstNew);							// 0x442540 the same for the bonus creatures
	void					AddPiranha();													// 0x452a60 a penalty piranha with a letter, when a piranha that is not dying (state 3) is on the screen (KeyChar calls it)

	// ---- BoardKeys.cpp (job B3) ----
	virtual void			KeyChar(SexyChar theChar) override;								// 0x454e40 slot 20
	bool					TryLetter(char theChar, Enemy* theEnemy);						// 0x443ae0 the creature's next letter is theChar: lock on it (mTarget), sound, append to mTyped
	void					SetBannerText(const std::string& theText);						// 0x446aa0 sets mBannerText and restarts mBannerTimer

	// ---- BoardGame.cpp (job B3): what WinWSApp calls ----
	AppStats				GetStats();														// 0x440080 fills mStats from the diver and the counters and returns the 12 dwords
	void					StartGame(const AppStats& theStats);							// 0x440130 the original takes the block by value; restores the diver, mRating, mPausedTime, mAccurateLevels
	void					SetFastCPU(bool theFlag);										// 0x440570 mFastCPU = theFlag
	void					SaveProgress(bool theFlag, bool theWithDouble);					// 0x440580 (the arguments go on to WinWSApp::SaveGameProgress) saves the game into the profile
	void					SetPaused(bool thePaused);										// 0x442100 pauses or resumes the game and its timers
	void					SetLessonManager(LessonManager* theLessonManager);				// 0x451cb0 takes the lesson: speed, ocean depth, the depth ruler, the water lines
	void					SetDepth(int theDepth);											// 0x452cf0 starts an abyss game at a depth: mScore, the water lines, the ruler
	void					AddBubble(int theX, int theY);									// 0x452f30
	void					AddBonusText(const std::string& theText, int theX, int theY, Font* theFont = NULL);	// 0x452fd0 a NULL font is the app's mFontUnsteadyOversteer22
};

}

#endif
