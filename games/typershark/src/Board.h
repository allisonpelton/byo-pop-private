#ifndef __TYPERSHARK_BOARD_H__
#define __TYPERSHARK_BOARD_H__

// Board.h: the game screen of Typer Shark: the sea, the diver, the sharks and piranhas with their words, the clams, the
// bonus creatures, the bosses, the score and the keyboard. It is the biggest class of the game (53 member functions, about
// 56 KB of code); games/typershark/docs/BOARD.md has the map of the methods, the split into translation jobs, the fields
// whose role is unknown, the registration field and the event codes the event loops handle.
// Original: class Sexy::Board : public Widget (the only base; the 2004 Widget is 0x58 bytes), vftable 0x5309a4 (47 slots),
// sizeof 0xf98 (WinWSApp makes it with operator new(0xf98) and the constructor 0x4544b0). Every field comment gives the
// offset in the original's object, which the port's layout does not follow (its Widget, std::string, std::vector and
// std::map are other sizes); the field order is the original's.
// The lists of creatures hold the Enemy part of each creature (the original keeps the Enemy part at offset 0 of Shark,
// Piranha, Clam and BonusCreature; the Board casts a Shark or Piranha with dynamic_cast, see ENTITIES.md). The embedded
// helper objects (Smoother, BgLayer, RatingPopup, DepthMarker) are the original's anonymous structs, named by their role.
// Registration code is not translated: the field at +0x150 and the test of it in Update are left out (BOARD.md, part 4).

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
		double				mField38;				// +0x38 constructor 0, never used
		double				mPrev;					// +0x40 the value at the previous draw
		double				mSmoothed;				// +0x48 the eased value
	};

	// 0x48 bytes: the same ring for one value whose eased result lives in the RatingPopup (+0x08), so there is no mSmoothed.
	struct FrameSmoother
	{
		double				mSamples[6];			// +0x00 ring of the last six changes
		int					mIndex;					// +0x30 next slot of mSamples (0 to 5)
		double				mField38;				// +0x38 constructor 0, never used
		double				mPrev;					// +0x40 the value at the previous draw
	};

	// 0x20 bytes: one of the two background strips (the sea behind the board); Draw moves and draws them.
	struct BgLayer
	{
		double				mX;						// +0x00 constructor 0; Draw draws the picture at this x
		double				mY;						// +0x08 constructor: the height of the strip's picture times its index (0, then 1)
		int					mUnused10[4];			// +0x10 constructor 0, never used
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
		int					mField34;				// +0x34 never written
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

	WSTimer*				mTimer58;				// +0x58 constructor makes it; AddedToManager adds it to the manager, RemovedFromManager takes it off; nothing else uses it
	int						mDrawCount;				// +0x5c ++ in every Draw
	int						mField060;				// +0x60 constructor 0, never used
	int						mUpdateCount;			// +0x64 ++ at the start of every Update, before the paused return
	Smoother				mSmoothers[2];			// +0x68 one for each BgLayer (0x50 bytes each, to +0x108)
	FrameSmoother			mFrameSmoother;			// +0x108 for the RatingPopup's mY (0x48 bytes, to +0x150)
	int						mMaxUnregAbyssDepth;	// +0x150 REGISTRATION: registry value "MaxUnregAbyssDepth", default 600; Update stops an unregistered abyss game at this depth (left out of the port)
	int						mWavePhase1;				// +0x154 ++ every other tick (constructor 0)
	int						mWavePhase2;				// +0x158 ++ every fourth tick (constructor 0)
	double					mWaveAmp;				// +0x160 the strength of the waves (4.0 to 5.0, moves by 0.004 every other tick; 440330 and 441ac0 use it twice over)
	bool					mWaveGrowing;			// +0x168 the direction mWaveAmp moves in (constructor true)
	GameOverDialog*			mGameOverDialog;		// +0x16c made by PlayerEaten when the last life is gone
	BgLayer					mBgLayers[2];			// +0x170 (0x20 bytes each, to +0x1b0)
	std::map<int, EnemyRecord*>	mRecordsByType;		// +0x1b0 (12 bytes) the wave's enemy record for each enemy type that SpawnWave made a creature of; cleared at every wave
	bool					mFastCPU;				// +0x1bc SetFastCPU (the constructor copies the app's flag): Draw uses the plain picture, not the wavy one, when it is off
	int						mField1c0;				// +0x1c0 constructor 0; SetDepth sets it
	int						mScore;					// +0x1c4 the score; in an abyss game it is the depth in feet (SetDepth sets it)
	int						mEmpInterval;			// +0x1c8 milliseconds of mEmpTimer for each 0.01 of the diver's EMP charge: 2500, or 1250 in an abyss game
	double					mSinTable[360];			// +0x1d0 sin(i * 1/pi) for i = 0..359 (to +0xd10)
	double					mField0d10;				// +0xd10 y of a scrolling picture of the sea (constructor: the Widget's +0x14); SetLessonManager: mField0e10 + 270; Update moves it with the dive
	double					mField0d18;				// +0xd18 the same for the next picture; SetLessonManager: mField0e10 + 120
	double					mField0d20;				// +0xd20 the same for the last picture; SetLessonManager: mField0e10 - 30
	bool					mFlag0d28;				// +0xd28 constructor false; SetDepth sets it; SpawnWave clears it and in an abyss game builds a boss when it was set
	int						mField0d2c;				// +0xd2c constructor -1; the first Update sets it from the profile; SpawnWave takes it mod 4 to pick the abyss boss (0 TorpedoBoss, 1 MechaShark...)
	int						mFadeAlpha;				// +0xd30 constructor 255; Update takes 2 off every other tick; the black fade-in at the start of a level
	WSTimer*				mGameTimer;				// +0xd34 starts disabled; SetVisible(true) starts it; its Elapsed is the game's time (GetStats gives it out)
	bool					mFlag0d38;				// +0xd38 constructor false; KeyChar sets it
	double					mDiverSpeed;			// +0xd40 SetLessonManager: the <DiverSpeed> of the lesson; Update adds it to mDivePos every tick
	double					mDivePos;				// +0xd48 constructor 1.0; the depth reached in pixels (Update += mDiverSpeed)
	double					mDivePosMarked;			// +0xd50 constructor 0; mDivePos at the last time Update moved the score on (every 8.0)
	WinWSApp*				mApp;					// +0xd58 constructor argument
	LessonManager*			mLessonManager;			// +0xd5c SetLessonManager
	bool					mFlag0d60;				// +0xd60 constructor false, never used
	int						mField0d64;				// +0xd64 constructor 0; SpawnWave zeroes it; HandleBonusCreatureEvents counts in it
	Player*					mPlayer;				// +0xd68 the diver (Player.h), made by the constructor (operator new(0x148))
	Enemy*					mOpenClam;				// +0xd6c the first clam of a pair that was opened (the memory game of the clams); 0 when none
	int						mPairsFound;			// +0xd70 pairs of clams matched so far (the third match shows the treasure)
	std::vector<Enemy*>		mSharks;				// +0xd74 (pad +0xd74, begin +0xd78, end +0xd7c, capacity +0xd80) the Shark creatures
	std::vector<Enemy*>		mPiranhas;				// +0xd84 the Piranha creatures
	std::vector<Enemy*>		mClams;					// +0xd94 the Clam creatures
	std::vector<Bubble*>	mBubbles;				// +0xda4 AddBubble
	std::vector<BonusText*>	mBonusTexts;			// +0xdb4 AddBonusText
	std::vector<Enemy*>		mBonusCreatures;		// +0xdc4 the BonusCreature creatures (enemy record type 11)
	std::vector<BackgroundFish*>	mBackgroundFish;	// +0xdd4 the fish that swim behind the sharks
	std::string				mTyped;					// +0xde4 (0x1c bytes) the letters typed so far on mTarget
	Enemy*					mTarget;				// +0xe00 the creature the typed letters belong to (0 when none)
	HudBar*					mHudBar;				// +0xe04 made by the constructor (operator new(0x1c), ctor 0x441730 with the player and the app)
	WSTimer*				mRatingTimer;			// +0xe08 the event loops stop or reset it when a group of words ends (ShowRating); Update stops and resets it as the RatingPopup fades
	double					mField0e10;				// +0xe10 constructor: the Widget's +0x14; SetLessonManager: the y of the ruler's mOceanDepth mark; Draw compares it with the height
	WSTimer*				mTypeTimer;				// +0xe18 reset by KeyChar and the event loops; its Elapsed is added to mTypeTime when a word is finished
	int						mTypeTime;				// +0xe1c milliseconds spent typing (the app adds it to its own total after the game)
	AppStats				mStats;					// +0xe20 (0x30 bytes) the block GetStats hands out; GetStats fills it from the diver, mRating, mField0e8c, mPausedTime, mGameTimer, mField0f68, mField0f6c
	WSTimer*				mTimer0e50;				// +0xe50 starts disabled; the fade of mField0e54 waits 2000 ms of it
	int						mField0e54;				// +0xe54 PlayerEaten sets 255; Update takes 3 off a tick once mTimer0e50 has passed 2 s (diver in state 1 or 9)
	int						mPausedTime;			// +0xe58 milliseconds the game spent paused (SetPaused adds the pause timer to it; GetStats gives it out)
	WSTimer*				mPauseTimer;			// +0xe5c SetPaused(true) resets it
	bool					mPaused;				// +0xe60 the game is paused (every creature's Draw and Update read it)
	int						mPauseCount;			// +0xe64 SetPaused(true) adds one
	WSTimer*				mBannerTimer;			// +0xe68 shows mBannerText; 2000 ms of it and mBannerLife counts down
	std::string				mBannerText;			// +0xe6c (0x1c bytes) a message printed on the screen (SetBannerText)
	int						mBannerLife;			// +0xe88 constructor 0; counts down by 2 each tick once mBannerTimer passes 2 s
	int						mField0e8c;				// +0xe8c constructor 0; GetStats gives it out (AppStats word 5) and GetFinishBonus reads it
	int						mField0e90;				// +0xe90 the diver's score at the start of the game (constructor, StartGame)
	int						mField0e94;				// +0xe94 KeyChar counts in it and compares it with a lesson value; ResetPiranhaSpeed zeroes it
	double					mPiranhaSpeed;			// +0xe98 1.0 after ResetPiranhaSpeed; AddPiranha multiplies it by 1.7 after the sixth piranha
	int						mPiranhaCount;			// +0xea0 piranhas AddPiranha made
	int						mField0ea4;				// +0xea4 constructor 255; Draw uses it as the alpha of the opening hint text ("Type the words on the sharks...")
	bool					mFlag0ea8;				// +0xea8 constructor false; KeyChar sets it
	WSTimer*				mEmpTimer;				// +0xeac every mEmpInterval ms the diver's EMP charge grows by 0.01
	int						mFlashAlpha;			// +0xeb0 a white flash over the screen: KeyChar sets 255, Update takes 8 off a tick, Draw draws it
	bool					mFlag0eb4;				// +0xeb4 the EMP charge is draining (Update takes 0.01 off a tick)
	bool					mFlag0eb5;				// +0xeb5 constructor false; the event loops add mTypeTimer's time to mTypeTime only while it is false
	Enemy*					mBoss;					// +0xeb8 the boss of the level (TorpedoBoss, GhostShip, MechaShark or MechaSquid), made by SpawnWave and the level start; 0 when none
	bool					mFlag0ebc;				// +0xebc constructor false; HandleBossEvents sets it when a boss is beaten outside an abyss game
	int						mField0ec0[9];			// +0xec0 constructor: zeros, except [5] = random 4..7 and [8] = random 8..11; only Draw uses them (three groups of three)
	int						mField0ee4;				// +0xee4 constructor 0; Update counts it; IsHudActive tests it against 0x10
	Image*					mImage0ee8;				// +0xee8 constructor: the app's mImages at byte offset 0x324 (index 0xc9)
	Image*					mImage0eec;				// +0xeec constructor: the app's mImages at byte offset 0x338 (index 0xce)
	Image*					mImage0ef0;				// +0xef0 constructor: the first entry of the app's mSonarSweepImages (+0x7cc); Update takes entries 1 to 4 of it in the sonar animation of diver state 7 (the table holds pictures)
	Image*					mImage0ef4;				// +0xef4 constructor: the first entry of the app's mSonarSweep2Images (+0x7dc); Update takes entries 1 to 4 of it in the sonar animation of diver state 7
	int						mField0ef8;				// +0xef8 constructor 255
	bool					mFlag0efc;				// +0xefc constructor false
	WSTimer*				mTimer0f00;				// +0xf00 starts disabled; stopped and reset when the level ends
	int						mField0f04;				// +0xf04 constructor 255; only Draw reads it
	bool					mFlag0f08;				// +0xf08 constructor false; Draw reads it
	int						mField0f0c;				// +0xf0c KeyChar adds GetStreakBonus to it; SaveProgress / Update keep the best in the profile (+0x64)
	std::string				mString0f10;			// +0xf10 (0x1c bytes) KeyChar clears it when mField0f2c reaches its length; Draw reads it
	int						mField0f2c;				// +0xf2c KeyChar counts it up to the length of mString0f10
	int						mField0f30;				// +0xf30 GetStreakBonus uses it; SaveProgress / Update keep the best in the profile (+0x68)
	double					mField0f38;				// +0xf38 constructor 0; KeyChar takes 5.0 off, Draw reads it
	bool					mFlag0f40;				// +0xf40 constructor false
	int						mField0f44;				// +0xf44 constructor 255
	int						mField0f48;				// +0xf48 constructor 0
	std::vector<DepthMarker*>	mDepthMarkers;		// +0xf4c the labels of the depth ruler (SetLessonManager builds them, Update adds more)
	RatingPopup*			mRating;				// +0xf5c made by the constructor (operator new(0x38))
	int						mField0f60;				// +0xf60 KeyChar counts it; SpawnWave zeroes it
	int						mField0f64;				// +0xf64 KeyChar counts it; SpawnWave zeroes it
	int						mField0f68;				// +0xf68 KeyChar counts it; GetStats gives it out (AppStats word 8)
	int						mField0f6c;				// +0xf6c constructor 0; GetStats gives it out (AppStats word 9)
	int						mField0f70;				// +0xf70 KeyChar counts it
	int						mField0f74;				// +0xf74 KeyChar counts it
	bool					mFirstUpdate;			// +0xf78 constructor true; the first Update does its start-of-game work and clears it
	Image*					mTreasureImage;			// +0xf7c ShowTreasure picks one of the app's mImages at byte offset 0x310 + 4 * (0..3), indexes 0xc4 to 0xc7; Draw draws it
	bool					mFlag0f80;				// +0xf80 constructor true; AddPiranha clears it; the event loops and Update set it again
	int						mField0f84;				// +0xf84 constructor: the +8 field of the app's mImages at byte offset 0x7c (index 0x1f)
	int						mSurfaceHeight;				// +0xf88 constructor: the +8 field of the app's mImages at byte offset 0x5c (index 0x17); IsBelowSurface compares mDivePos with it
	int						mOceanDepth;			// +0xf8c SetLessonManager: the <OceanDepth> of the lesson (-1 in an endless game)
	int						mFloorY;				// +0xf90 constructor: the app's +0x64 minus the +8 field of mImages[8] (the status bar picture): the bottom of the play field; the lanes of creatures are counted up from it
	HyperlinkWidget*		mContinueLink;			// +0xf94 (role guessed) a link widget (id 300, listener the app) that AddedToManager makes and adds; SetVisible and SetDisabled forward to it while the diver is in state 7; WinWSApp hides it while the options dialog is up in the diver states 6, 7 and 0xc; ShowTreasure brings it to the front

public:
	// ---- Board.cpp (job B2): the constructor, the Widget overrides, two small queries ----
	Board(WinWSApp* theApp);														// 0x4544b0
	virtual ~Board() override;														// 0x454e10 deleting destructor, body 0x451700

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x445eb0 slot 1: makes mField0f94, adds the nine timers
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x43fbf0 slot 2: takes them off again
	virtual void			SetVisible(bool isVisible) override;							// 0x4401b0 slot 4
	virtual void			SetDisabled(bool isDisabled) override;							// 0x440250 slot 10
	virtual void			KeyDown(KeyCode theKey) override;								// 0x442520 slot 21 (empty)
	virtual void			MouseMove(int x, int y) override;								// 0x440c20 slot 25
	bool					IsHudActive();													// 0x440b70 (the StatusBar asks) false while the diver is in state 7 and mField0ee4 > 0x10
	bool					IsEmpTimerPaused();												// 0x440cb0 (the StatusBar asks) mEmpTimer's paused flag

	// ---- BoardDraw.cpp (job B2): Draw and what only Draw calls ----
	virtual void			Draw(Graphics* g) override;										// 0x44d200 slot 16
	void					BuildWaterImage(Graphics* g);									// 0x440330 (role guessed) the wave effect when the board cannot draw wavy pictures (the original takes the Graphics and ignores it)
	void					DrawWavyImage(Graphics* g, Image* theImage, int theX, int theY);	// 0x441ac0 draws a picture row by row, shifted along mSinTable
	int						GetNextGoal();													// 0x440010 the next score goal above the diver's score (multiples of 150000, then fixed steps)
	std::string				GetRankTitle();													// 0x446950 the title for the score ("Sea Titan", "Leviathan"...)
	int						GetLevelBonus();												// 0x43fca0 (difficulty + 1) * the diver's mField114 + 1 (11 once mField114 is 12 or more) * 1000, at most 60000
	int						GetBossBonus();													// 0x43fce0 (role guessed) 10000, 30000, 50000, 70000 for the difficulties 0 to 3, 100000 otherwise
	int						GetFinishBonus();												// 0x440290 (role guessed) the end-of-level bonus from mField0e8c and the lives, at most 100000
	int						GetStreakBonus();												// 0x4423f0 points for a bonus round by difficulty and mField0f30
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
	void					ResetPiranhaSpeed();											// 0x4402f0 mPiranhaSpeed 1.0, mField0e94 0

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
	void					StartGame(const AppStats& theStats);							// 0x440130 the original takes the block by value; restores the diver, mRating, mPausedTime, mField0e8c
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
