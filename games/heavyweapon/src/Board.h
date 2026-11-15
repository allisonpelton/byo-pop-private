#ifndef __HEAVYWEAPON_BOARD_H__
#define __HEAVYWEAPON_BOARD_H__

// Board.cpp: the playing field of a level or a survival game: the tank and its weapons, every enemy, shot and effect,
// the scrolling backgrounds, and the level's course from "get ready" to the results.
// Original: constructor 0x4191d0 (0x368 bytes), destructor 0x4198e0; vtable 0x52c888 (a Widget, 48 slots: 0 the
// destructor 0x41aca0, 1 AddedToManager 0x4184b0, 2 RemovedFromManager 0x40fdb0, 16 Draw 0x415530, 17 Update 0x41b690,
// 18 UpdateF 0x419e30, 21 KeyChar 0x4175a0, 22 KeyDown 0x410660, 23 KeyUp 0x413a30, 24 MouseEnter 0x4138c0,
// 25 MouseLeave 0x413850, 26 MouseMove and 32 MouseDrag 0x40feb0, 27 MouseDown 0x41acd0, 29 MouseUp 0x417090) and
// 0x52c868 at +0x58 (its ButtonListener: 4 ButtonPress 0x410670, 5 ButtonDepress 0x410690, for the game-over
// dialog's buttons). Offsets in the comments are the original's (absolute); the Widget's own fields end at +0x58.
// The original keeps one ObjectList of each kind of thing on the field; the walks over them are written out at every
// use, here as there.

#include "Widget.h"
#include "ButtonListener.h"
#include "ObjectList.h"
#include "Particles.h"
#include "HeavyWeaponApp.h"

#include <string>
#include <vector>

namespace Sexy
{

class BossDeath;
class Banner;
class Enemy;
class TankShot;
class FloatingText;
class Graphics;
class HeavyWeaponApp;
class BossWarning;
class NukeBlast;
class MegaLaserPart;
class Projectile;
class ResultsScreen;
class PowerUpDrops;
class SoundInstance;
class Sprite;
class StatusBar;

// A shot of the tank's gun or an enemy's (0x40 bytes, the list at +0x1e4)
struct Bullet
{
	double			mX;				// +0x00
	double			mY;				// +0x08
	double			mVelX;			// +0x10
	double			mVelY;			// +0x18
	double			mGravity;		// +0x20 added to mVelY each update (0 for AddEnemyBullet's; 0.05 for the enemies' falling ones)
	double			mDamage;		// +0x28 the tank's: its gun power + 1
	bool			mIsEnemy;		// +0x30
	Sprite*			mSprite;		// +0x34
	int				mColumn;		// +0x38 the cel: the tank's, its direction (0..19)
	int				mRow;			// +0x3c the tank's: its gun power, 0..4
};

// An explosion (0x38 bytes, the list at +0x204)
struct Explosion
{
	double			mX;				// +0x00 its centre
	double			mY;				// +0x08
	double			mVelX;			// +0x10
	double			mVelY;			// +0x18
	int				mWidth;			// +0x20
	int				mHeight;		// +0x24
	double			mFrame;			// +0x28 0..19
	double			mFrameSpeed;	// +0x30
};

// A crater where something hit the ground (0x10 bytes, the list at +0x224)
struct Crater
{
	double			mX;				// +0x00 moves with the ground
	int				mLife;			// +0x08 updates left, from 1000
	int				mFrame;			// +0x0c which of the five craters
};

// A stretch of the tank's tread marks on the ground (0x18 bytes, the list at +0x254)
struct TreadMark
{
	double			mLeft;			// +0x00
	double			mRight;			// +0x08
	bool			mIsUnderTank;	// +0x10 the stretch the tank is laying now
};

// A spent shell thrown out of the gun (0x20 bytes, the list at +0x284)
struct ShellCasing
{
	double			mX;				// +0x00
	double			mY;				// +0x08
	double			mVelX;			// +0x10
	double			mVelY;			// +0x18
};

// A muzzle flash at the gun's end (0x28 bytes, the list at +0x294)
struct MuzzleFlash
{
	double			mX;				// +0x00
	double			mY;				// +0x08
	double			mVelX;			// +0x10
	double			mVelY;			// +0x18
	double			mLife;			// +0x20 updates left, from 5
};

// A spark (the list at +0x2a4)
struct Spark
{
	double			mX;				// +0x00
	double			mY;				// +0x08
	double			mVelX;			// +0x10
	double			mVelY;			// +0x18
	int				mBrightness;	// +0x20 drawn Rand() % 32 + 32 each update
	int				mFrame;			// +0x24 drawn Rand() % 5 each update
	int				mLife;			// +0x28 updates left
};

// A white flash over the ground (0x10 bytes, the list at +0x2d4)
struct GroundFlash
{
	double			mX;				// +0x00 moves with the ground
	int				mAlpha;			// +0x08 from 255
	int				mType;			// +0x0c 0 fades 3 an update, else 5
};

// A background animation playing (0x50 bytes, the four slots at +0x314: one a plane). The board copies it from the
// level's BackgroundAnimInfo (HeavyWeaponApp::mAnims) when it starts one.
struct BackgroundAnim
{
	Sprite*				mSprite;		// +0x00
	double				mFrame;			// +0x08
	int					mPlane;			// +0x10 3 sky, 2 far background, 1 background, 0 ground
	double				mX;				// +0x18
	int					mY;				// +0x20
	double				mMoveX;			// +0x28
	bool				mHasNukedFrame;	// +0x30
	bool				mIsNuked;		// +0x31 a nuke went off while it was on screen: it shows its last frame
	std::vector<double>	mFrameDelays;	// +0x34
	int					mDirection;		// +0x44 1 forwards, -1 backwards
	int					mPlaysOnce;		// +0x48 1 plays to its end and back, else loops
	bool				mRare;			// +0x4c
};

// A wave being sent in (0x28 bytes, the list at +0x2b4): a copy of a WaveInfo, its quantities scaled up for the
// levels past the last list of waves
struct WaveSpawner
{
	int					mTimeLeft;		// +0x00 updates left, from mLength down to 0
	int					mLength;		// +0x04
	std::vector<int>	mCraft;			// +0x08 the craft index (CraftInfo) of each line
	std::vector<int>	mQuantities;	// +0x18 how many of it, sent in evenly over mLength
};

// One of the defense orbs circling the tank (0x18 bytes, three at +0xc8)
struct DefenseOrb
{
	double			mX;				// +0x00
	double			mY;				// +0x08
	double			mFlash;			// +0x10 1 when it is hit, fading
};

class Board : public Widget, public ButtonListener
{
public:
	// The dialog the board puts up when the game is over, and its buttons (ButtonDepress)
	enum
	{
		DIALOG_GAME_OVER = 0x28,
		BUTTON_GAME_OVER_CONTINUE = 0x29,
		BUTTON_GAME_OVER_SCORES = 0x2a
	};

	enum
	{
		NUM_ORBS = 3,
		NUM_ANIM_PLANES = 4,
		NUM_WEAPON_DELAYS = 6,
		NO_RANK = 10,				// AddHighScore's answer when the score made no list
		NUM_SIGNS = 4,				// the roadside signs a level's ground shows after a boss (mSignFrame)
		BOSS_WARNING_TIME = 600		// the boss's warning comes this long before the level's end
	};

	// The background animations' planes (BackgroundAnim::mPlane, mAnims' slots)
	enum
	{
		PLANE_GROUND,
		PLANE_BACKGROUND,
		PLANE_FAR_BACKGROUND,
		PLANE_SKY
	};

	HeavyWeaponApp*	mApp;					// +0x5c
	int				mMouseX;				// +0x60
	int				mMouseY;				// +0x64
	float			mSkyScroll;				// +0x6c the four planes' scroll, 0.1 of the ground's speed
	float			mGroundScroll;			// +0x70 1
	float			mBackgroundScroll;		// +0x74 0.3
	float			mFarBackgroundScroll;	// +0x78 0.2
	int				mLevelEndBlocks;		// +0x7c things that hold the level's end back (an enemy coming in)
	int				mTankDeadTime;			// +0x80 updates until the next tank (300 at the start, 400 after a death);
											// 0 while the tank plays
	int				mShake;					// +0x84 the screen's shaking, down 2 an update
	double			mNukeFlash;			// +0x88 the flash of a nuke or a boss's last blast (128 for a FatBoy's), down 0.75 an update:
											// the backgrounds tinted red and an orange fill added; the enemies hold their fire meanwhile
	double			mTankX;					// +0x90 from the screen's centre
	int				mTankY;					// +0x98
	double			mTankVelX;				// +0xa0 nothing reads it
	int				mGunDelay;				// +0xa8 updates until the gun fires again
	int				mWeaponDelays[NUM_WEAPON_DELAYS]; // +0xac updates until each special weapon fires again
											// (WEAPON_ order, the orbs' never set)
	DefenseOrb		mOrbs[NUM_ORBS];		// +0xc8
	double			mOrbAngle;				// +0x110
	double			mTreadFrame;			// +0x118 0..9
	double			mTurretFrame;			// +0x120 the gun's direction, 0..20
	bool			mMouseDown;				// +0x128 the left button is down: the gun fires
	int				mMegaLaserTime;			// +0x12c the mega laser's charge, in updates of firing
	int				mMegaLaserFiring;		// +0x130 updates the mega laser keeps firing
	int				mTankWidth;				// +0x134 the tank sprite's cel
	int				mTankHeight;			// +0x138
	double			mShieldFrame;			// +0x140 the shield's animation, 0..10
	double			mShieldHitGlow;			// +0x148 9 when a hit costs a shield level, down 0.3 an update; while it
											// glows, more hits cost nothing
	double			mLaserAngle;			// +0x150 the laser's and mega laser's beam
	double			mLaserLength;			// +0x158
	int				mShieldFlash;			// +0x160 the tank's hit flash, down 4 an update
	int				mSupplyCopterDelay;		// +0x178 updates until the next supply copter may come (1000 at the start;
											// 1000 after one, 600 in survival)
	int				mLevelTime;				// +0x17c updates played (a survival game counts 2 an update); it stops
											// at mLevelLength while the boss lives
	int				mTimePenalty;			// +0x180 survival: time taken off the clock (6000, 30 seconds, for each
											// friendly unit shot down), at most mLevelTime
	int				mLevelLength;			// +0x184 the level's length (LevelInfo); the boss comes at its end
	double			mScrollSpeed;			// +0x188 the ground's, 0..1
	double			mScrollDistance;		// +0x190 below 1000 the ground scrolls this far more and stops
	int				mSignFrame;				// +0x198 which roadside sign the ground shows; Rand() % 4 after a boss
	double			mDifficulty;			// +0x1a0 n * n * (100 / (n + 15)), n the level (survival: its minutes) + 1, at
											// most 8
	int				mMegaLaserPartsCaught;	// +0x1a8 the mega laser's parts caught (the player's record's at the start)
	int				mMegaLaserPartDelay;	// +0x1ac updates until an enemy may drop another mega laser part
	int				mNukeDropDelay;			// +0x1b0 updates until a supply copter may bring a nuke
	int				mShieldDropDelay;		// +0x1b4 updates until a supply copter may bring a shield
	bool			mBlockedLeft;			// +0x1b8 something holds the tank off the left side (no trucks or tanks come)
	bool			mBlockedRight;			// +0x1b9
	bool			mDozerOut;				// +0x1ba a dozer is on the field (no second one comes)
	int				mFriendlyCount;			// +0x1bc the player's units on the field
	int				mBannerTime;			// +0x1c4 below 0 a banner is showing, counting down to -705
	int				mDeathTime;				// +0x1c8 updates since the tank died, from 1 to 400, then 0
	int				mDeathX;				// +0x1cc where the tank died
	int				mWaveDelay;				// +0x1d0 updates until the next wave once none is being sent in
	int				mClickCount;			// +0x1d4 clicks in a row that fired one shot each (the auto-fire tip)
	int				mShotsThisPress;		// +0x1d8 shots fired since the left button went down
	int				mNoNukesTime;			// +0x1dc updates the "no nukes" line shows, 250 after a right click without one
	bool			mBossUntouchable;		// +0x1e0 the boss cannot be hurt now: no nuke, no health bar (set by the boss,
											// cleared each update while it lives)

	ObjectList<Bullet>			mBullets;			// +0x1e4
	ObjectList<Enemy>			mEnemies;			// +0x1f4
	ObjectList<Explosion>		mExplosions;		// +0x204
	ObjectList<Projectile>		mProjectiles;		// +0x214
	ObjectList<Crater>			mCraters;			// +0x224
	ObjectList<TankShot>		mTankShots;		// +0x234 the special weapons' shots (homing missiles, rockets, flak, static)
	ObjectList<FloatingText>	mFloatingTexts;		// +0x244
	ObjectList<TreadMark>		mTreadMarks;		// +0x254
	ObjectList<NukeBlast>		mNukeBlasts;		// +0x264
	ObjectList<MegaLaserPart>	mMegaLaserParts;	// +0x274
	ObjectList<ShellCasing>		mShellCasings;		// +0x284
	ObjectList<MuzzleFlash>		mMuzzleFlashes;		// +0x294
	ObjectList<Spark>			mSparks;			// +0x2a4
	ObjectList<WaveSpawner>		mWaveSpawners;		// +0x2b4
	ObjectList<Banner>			mBanners;			// +0x2c4
	ObjectList<GroundFlash>		mGroundFlashes;		// +0x2d4

	Enemy*			mBoss;					// +0x2e4
	Particles		mParticles;				// +0x2e8
	PowerUpDrops*	mPowerUpDrops;			// +0x2fc the power-ups and weapons a supply copter dropped
	BossDeath*		mBossDeath;				// +0x300 the boss's last explosions, while it dies
	StatusBar*		mStatusBar;				// +0x304 the bar across the top (a widget the board adds and removes)
	ResultsScreen*	mResultsScreen;			// +0x308 the level's results, after the boss
	BossWarning*	mBossWarning;			// +0x30c the warning before the boss
	SoundInstance*	mMegaLaserSound;		// +0x310
	BackgroundAnim*	mAnims[NUM_ANIM_PLANES];// +0x314
	bool			mPaused;				// +0x324 the pause screen is up
	bool			mSuspended;				// +0x325 nothing moves: paused, or the mouse left the field
	bool			mHasStarted;			// +0x326 the first tank came in
	bool			mIsSurvival;			// +0x327
	int				mEnemiesDestroyed;		// +0x328
	int				mEnemiesSpawned;		// +0x32c
	int				mProjectilesDestroyed;	// +0x330
	int				mProjectilesFired;		// +0x334
	int				mShotsFired;			// +0x338
	int				mShotsMissed;			// +0x33c
	int				mFriendliesDestroyed;	// +0x340
	int				mRank;					// +0x344 the game's place on the high scores, NO_RANK if none
	std::string		mTypedKeys;				// +0x34c the keys typed, while they spell the start of a code

public:
	Board(HeavyWeaponApp* theApp, bool isSurvival);								// 0x4191d0
	virtual ~Board();															// 0x4198e0 (deleting 0x41aca0, slot 0)

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;	// 0x4184b0 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override; // 0x40fdb0 slot 2
	virtual void	Draw(Graphics* g) override;									// 0x415530 slot 16
	virtual void	Update() override;											// 0x41b690 slot 17
	virtual void	UpdateF(float theFrac) override;							// 0x419e30 slot 18
	virtual void	KeyChar(SexyChar theChar) override;							// 0x4175a0 slot 21
	virtual void	KeyDown(KeyCode theKey) override;							// 0x410660 slot 22
	virtual void	KeyUp(KeyCode theKey) override;								// 0x413a30 slot 23
	virtual void	MouseEnter() override;										// 0x4138c0 slot 24
	virtual void	MouseLeave() override;										// 0x413850 slot 25
	virtual void	MouseMove(int x, int y) override;							// 0x40feb0 slot 26
	virtual void	MouseDrag(int x, int y) override;							// 0x40feb0 slot 32
	virtual void	MouseDown(int x, int y, int theBtnNum, int theClickCount) override; // 0x41acd0 slot 27
	virtual void	MouseUp(int x, int y, int theBtnNum, int theClickCount) override;	// 0x417090 slot 29
	virtual void	ButtonPress(int theId) override;							// 0x410670 (ButtonListener slot 4)
	virtual void	ButtonDepress(int theId) override;							// 0x410690 (ButtonListener slot 5)

	using Widget::MouseDown;
	using Widget::MouseUp;
	using ButtonListener::ButtonPress;

	void			UpdateShake();												// 0x40fe00
	// The craft of craft.xml's index theCraft comes in (a wave's line, SpawnCraft's switch)
	void			SpawnCraft(int theCraft);									// 0x40fed0
	// The enemies' bombs, missiles and rockets: none while the tank is dead or the boss dies
	void			AddDumbBomb(int x, int y, double theVelX, double theVelY, bool isMirrored); // 0x4104b0
	void			AddIronBomb(int x, int y, double theVelX, double theVelY, bool isMirrored); // 0x410540
	void			AddHellfire(int x, int y, double theVelX, double theVelY);	// 0x4105d0
	void			UseMegaLaser(int theAmount);								// 0x410860
	void			UpdateBanners();											// 0x410bd0
	void			DrawAnim(Graphics* g, int thePlane);						// 0x410c20
	void			UpdateTankShots();											// 0x410cf0
	void			AddGunShot(double theGunPower, int theSideOffset, int theAngleOffset); // 0x410d40
	void			UpdateEnemies();											// 0x410ec0
	// theFrameSpeed is overwritten: every explosion plays at 0.4 frames an update
	void			AddExplosion(double x, double y, int theWidth, int theHeight, double theVelX, double theVelY,
						double theFrameSpeed);									// 0x410f40
	void			UpdateExplosions();											// 0x4111a0
	void			UpdateProjectiles();										// 0x4112a0
	void			UpdateScroll(float theFrac);								// 0x4112f0
	// 2 * theSpread + 1 craters 20 apart, centred on x, each throwing up smoke and rocks
	void			AddCrater(int x, int theSpread);							// 0x411540
	void			UpdateTreadMarks();											// 0x4116b0
	void			UpdateNukeBlasts();											// 0x411800
	void			AddEnemyBullet(int x, int y, double theAngle, double theSpeed); // 0x411850
	void			UpdateMegaLaserParts();										// 0x411930
	void			UpdateShellCasings();										// 0x411980
	void			AddMuzzleFlash(double x, double y, double theVelX, double theVelY); // 0x411ab0
	void			UpdateMuzzleFlashes();										// 0x411b40
	void			UpdateSparks();												// 0x411c30
	// The pause screen comes up (unless it is up, or the help or the options are)
	void			Pause();													// 0x411d40
	// Nothing moves, and the crafts' looping sounds are muted, until Resume
	void			Suspend();													// 0x411de0
	void			Resume();													// 0x411e40
	void			UpdateGroundFlashes(float theFrac);							// 0x411eb0
	void			AddGroundFlash(int x, int theType);							// 0x411fa0
	void			UpdateTank();												// 0x412dc0
	void			UpdateBossDeath();											// 0x4138e0
	void			UpdateFloatingTexts();										// 0x414a70
	// A campaign's score gains thePoints (never below 0), shown floating up from (x, y); survival scores nothing
	void			AddScore(int thePoints, int x, int y);						// 0x417230
	// A projectile theSprite's cel touches at (x, y) takes theDamage; true if one was hit
	bool			HitProjectiles(Sprite* theSprite, int x, int y, int theColumn, int theRow, double theDamage); // 0x417390
	void			UpdateOrbs();												// 0x418770
	void			UpdateLasers();												// 0x418a50
	void			UpdateMegaLaser();											// 0x418c90
	void			UpdateAnims(float theFrac);									// 0x418f50
	void			StartAnims();												// 0x419eb0
	void			FireNuke();													// 0x41a160
	// Sends in theWave of theLevel's list (the last list, scaled up, past it)
	void			AddWaveSpawner(int theLevel, int theWave);					// 0x41a5b0
	void			KillTank();													// 0x41a880
	// What a supply copter brings: a random one of the power-ups (HeavyWeaponApp::POWERUP_) the player may get now,
	// and in survival a special weapon (WEAPON_ + 6) while they are few; -1 for none
	int				ChoosePowerUp();											// 0x41aa90
	void			UpdateCollisions();											// 0x41add0
	void			UpdateWaves();												// 0x41b340
};

}

#endif
