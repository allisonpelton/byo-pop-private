#ifndef __HEAVYWEAPON_HEAVYWEAPONAPP_H__
#define __HEAVYWEAPON_HEAVYWEAPONAPP_H__

// HeavyWeaponApp.cpp: the application class of Heavy Weapon Deluxe.
// Original: the static global 0x56b1a0 of the game's SexyApp subclass (constructor 0x40b750, destructor 0x40b980, the
// dynamic initializer 0x517ff0); WinMain (0x40e210) calls Init(), seeds mRand from Rand(), and calls Start() and
// Shutdown(). Its vtable 0x52a570 has 86 slots (0-6 ButtonListener, then SexyAppBase's and SexyApp's: the framework of
// early 2005, games/heavyweapon/docs/FRAMEWORK_2005.md); the game overrides 4 ButtonPress, 5 ButtonDepress,
// 8 UpdateFrames, 16 the destructor, 18 Init, 20 LoadingThreadProc, 38 Shutdown, 42 Start, 46 PlaySample(id, pan),
// 59 and 60 SwitchScreenMode, 70 GotFocus and 71 LostFocus.
// Offsets below are the original's (absolute); the game's own fields start at +0x688. The port's base classes have
// other sizes, and nothing depends on them.

#include "SexyApp.h"
#include "MTRand.h"
#include "Res.h"

#include <cstdio>
#include <string>
#include <vector>

namespace Sexy
{

class Armory;
class Board;
class Graphics;
class HelpScreen;
class Image;
class ImageFont;
class MainMenu;
class MapScreen;
class Sprite;
class TitleScreen;
class Widget;

// One kind of enemy craft, from data/craft.xml (the app's table at +0xb48, 0x68 bytes an entry). Entry 0 stays empty:
// the loader fills the table from entry 1, and the craft index of waves.xml is the entry's.
struct CraftInfo
{
	std::string		mName;			// +0x00 name: the id waves.xml names the craft by
	std::string		mDescription;	// +0x1c desc: its full name on the mission briefing
	std::string		mArms;			// +0x38 arms: its weapons on the mission briefing
	int				mPoints;		// +0x54 points for destroying it
	int				mIndex;			// +0x58 its index in the table (1..21)
	double			mArmor;			// +0x60 armor
};

// A level's name, length and briefing texts, from data/levels.xml (the app's vector at +0x1458, 0x30 bytes an entry)
struct LevelInfo
{
	std::string					mName;		// +0x00 name: the region, also the backgrounds' file names
	std::vector<std::string>	mIntel;		// +0x1c the Intel texts
	int							mLength;	// +0x2c length
};

// One wave of a level, from data/waves.xml or data/survivalN.xml (0x28 bytes)
struct WaveInfo
{
	int					mLength;		// +0x00 length
	std::vector<int>	mCraft;			// +0x08 the craft index (CraftInfo) of each Craft line
	std::vector<int>	mQuantities;	// +0x18 how many of it
};

// A background animation of a level, from images/anims/anims.xml (0x50 bytes)
struct BackgroundAnimInfo
{
	Sprite*				mSprite;		// +0x00 Images\Anims\<name>, frames columns
	int					mPlane;			// +0x10 plane - 1: 3 sky, 2 far background, 1 background, 0 ground
	double				mOffset;		// +0x18 offset from the plane image's left side
	int					mY;				// +0x20 y of its centre
	double				mMoveX;			// +0x28 mx: pixels added to x each update
	bool				mHasNukedFrame;	// +0x30 nuke: the strip's last frame shows it nuked
	bool				mLoadFailed;	// +0x31 the strip did not load (the loader stops at once then)
	std::vector<double>	mFrameDelays;	// +0x34 each frame's speed: speed, or a Delay line's
	bool				mPlaysOnce;		// +0x48 type is not "looping"
	bool				mRare;			// +0x4c rare
};

// A player's record in userdata/users.dat (0x60 bytes, written raw)
struct UserRecord
{
	// mFlags
	enum
	{
		FLAG_NO_HELP = 1,				// the help's "don't show again" box: no help before the first game
		FLAG_NO_TIPS = 2				// the options' hints box unticked: no play tips
	};

	char			mName[0x18];		// +0x00
	int				mBestScore;			// +0x18 the score when mMaxLevel last rose
	unsigned short	mLevel;				// +0x1c the level of the campaign in progress
	unsigned short	mMaxLevel;			// +0x1e the furthest level reached
	int				mBestSurvivalTime;	// +0x20 the longest survival, in updates
	int				mScore;				// +0x24 the score of the campaign in progress
	unsigned short	mPowerUps[6];		// +0x28 HeavyWeaponApp::mPowerUps
	unsigned short	mWeapons[6];		// +0x34 HeavyWeaponApp::mWeapons
	unsigned short	mMegaLaserParts;	// +0x40 the mega laser parts caught toward the next charge
	unsigned char	mFlags;				// +0x42 FLAG_
	unsigned char	mPadBeforeStats;	// +0x43 padding before mStats, never written
	int				mStats[7];			// +0x44 HeavyWeaponApp::mStats
};

// A line of the campaign's high-score list (userdata/hiscores.dat, 0x24 bytes written raw)
struct HighScoreRecord
{
	char			mName[0x18];
	int				mScore;			// +0x18
	int				mLevel;			// +0x1c the level the game ended on
	int				mMaxLevel;		// +0x20 the player's furthest level, 0x13 for a finished campaign
};

// A line of the survival high-score list (0x1c bytes written raw)
struct SurvivalScoreRecord
{
	char			mName[0x18];
	int				mTime;			// +0x18 updates survived
};

class HeavyWeaponApp : public SexyApp
{
public:
	enum
	{
		NUM_CRAFT = 22,				// the craft table (entry 0 empty)
		NUM_TARGET_IMAGES = 8,		// images\target1..8
		NUM_POWER_UPS = 6,			// mPowerUps
		NUM_WEAPONS = 6,			// mWeapons
		NUM_STATS = 7,				// mStats
		NUM_HIGH_SCORES = 10,
		NUM_LOADING_TASKS = 0xee,	// the loading bar's count: the thread's 146 images and 91 sounds, and one more
		FINAL_LEVEL = 0x12,			// the last level's index (its boss is the final one)
		NUM_LEVELS_PLUS_ONE = 0x13	// mMaxLevel of a finished campaign
	};

	// The tanks a game starts with (mLives): a campaign's every level, a survival game
	enum
	{
		CAMPAIGN_LIVES = 3,
		SURVIVAL_LIVES = 1
	};

	// The tank's power-ups, dropped by the supply craft (mPowerUps; PowerUpDrops has their most)
	enum
	{
		POWERUP_NUKES,				// nukes held, 0..3; a new game starts with 1
		POWERUP_SHIELD,				// the shield's level, 0..3
		POWERUP_SPEED,				// the tank's speed
		POWERUP_GUN_POWER,			// the gun's damage
		POWERUP_FIRE_RATE,			// the gun's rate of fire
		POWERUP_SPREAD				// the gun's spread, 0..4; also the gun image's row
	};

	// The campaign's counts (mStats; the board keeps the same for one level)
	enum
	{
		STAT_ENEMIES_DESTROYED,
		STAT_ENEMIES_SPAWNED,
		STAT_PROJECTILES_DESTROYED,
		STAT_PROJECTILES_FIRED,
		STAT_SHOTS_FIRED,
		STAT_SHOTS_MISSED,			// bullets that left the field or hit the ground (the enemies' too)
		STAT_FRIENDLIES_DESTROYED
	};

	// The play tips shown in this game (mTipsShown bits)
	enum
	{
		TIP_AUTO_FIRE = 1,			// clicking shot by shot (Board::MouseUp)
		TIP_NUKE = 2,				// the first nuke collected
		TIP_FRIENDLY = 4,			// the first white (friendly) helicopter
		TIP_MEGALASER = 8			// the first mega laser part caught
	};

	// The special weapons the armory sells (mWeapons, the armory's buttons and DrawUpgradeWeapon in this order)
	enum
	{
		WEAPON_ORBS,				// defense orbs circling the tank, 1..3
		WEAPON_HOMING,				// homing missiles
		WEAPON_LASERS,				// lasers on the gun
		WEAPON_ROCKETS,				// rocket pods, 1..3
		WEAPON_FLAK,				// flak guns on the gun
		WEAPON_STATIC				// the static strike
	};

	// GetBossIndex: 0..8 the nine bosses in turn, then these
	enum
	{
		BOSS_FINAL = 9,				// the last level's
		BOSS_SURVIVAL = 10			// a survival game's
	};

	// The dialogs (AddDialog ids) and the buttons of theirs that report to the app (ButtonDepress)
	enum
	{
		DIALOG_QUIT = 3,			// "quit game" over a game (QuitDialog)
		DIALOG_OPTIONS = 7,
		DIALOG_USER_LIST = 0x15,
		DIALOG_USER_NAME = 0x1b,
		DIALOG_MESSAGE = 0x20,
		BUTTON_QUIT_CANCEL = 4,		// back to the game
		BUTTON_QUIT_TO_MENU = 5,
		BUTTON_OPTIONS = 6,			// the main menu's and the pause screen's
		BUTTON_CLOSE_USER_LIST = 0x17,
		BUTTON_CLOSE_MESSAGE = 0x39,
		BUTTON_HELP_PLAY = 0x3f		// the help screen's, before a first survival game
	};

	Sprite*			mSprites[NUM_IMAGES];		// +0x688 the image table (Res.h)
	Image*			mTargetImages[NUM_TARGET_IMAGES];// +0x8fc images\target1..8
	bool			mLoadingCompleteShown;		// +0x91c the title screen has been told the loading is done
	Board*			mBoard;						// +0x920
	ImageFont*		mFonts[NUM_FONTS];			// +0x924 Res.h FontId
	Armory*			mArmory;					// +0x944
	TitleScreen*	mTitleScreen;				// +0x948 the first screen (the loading bar, then the intro)
	MainMenu*		mMainMenu;					// +0x94c
	MapScreen*		mMapScreen;					// +0x950 the mission map and briefing
	HelpScreen*		mHelpScreen;				// +0x958 "how to play"
	int				mScore;						// +0x95c the campaign's score
	int				mLevel;						// +0x960 the level being played (0-based)
	int				mLives;						// +0x964 tanks left, the one in play included
	int				mPowerUps[NUM_POWER_UPS];	// +0x968 the power-ups' levels (POWERUP_)
	int				mWeapons[NUM_WEAPONS];		// +0x980 the special weapons' levels (WEAPON_)
	int				mSoundDelays[NUM_SOUNDS];	// +0x998 updates until each sound may play again (PlaySample)
	int				mStats[NUM_STATS];			// +0xb08 the campaign's counts: 0 enemies destroyed, 6 friendly units
												// destroyed, the others (Board)
	double			mParticles;					// +0xb28 the options' particle amount, 0..1 (registry Particles / 10000)
	bool			mHelpPending;				// +0xb30 set by ShowMainMenu: the help comes up before the next game unless
												// the player turned it off (UserRecord::FLAG_NO_HELP); cleared when it does
	bool			mIsNewUser;					// +0xb31 the player was just made: the main menu says welcome, not welcome back
	unsigned short	mTipsShown;					// +0xb3e one bit a play tip shown in this game (NewGame sets every bit for a
												// survival game, which shows none)
	int				mCurrentUser;				// +0xb40 registry LastUser: the index in mUsers
	CraftInfo		mCraft[NUM_CRAFT];			// +0xb48
	std::vector<HighScoreRecord>		mHighScores;		// +0x1438
	std::vector<SurvivalScoreRecord>	mSurvivalScores;	// +0x1448
	std::vector<LevelInfo>				mLevels;			// +0x1458
	std::vector<std::vector<WaveInfo> >	mWaves;				// +0x1468 each level's waves (LoadWaves)
	std::vector<std::vector<BackgroundAnimInfo> >	mAnims;				// +0x1478 each level's background animations
	std::vector<UserRecord>				mUsers;				// +0x1488
	MTRand			mRand;						// +0x1498 the game's own generator, seeded from Rand() after Init
	FILE*			mTraceFile;					// port: the test command "trace" writes the state each update leaves here

public:
	HeavyWeaponApp();															// 0x40b750
	virtual ~HeavyWeaponApp();													// 0x40b980 (deleting 0x40cff0, slot 16)

	virtual void	ButtonPress(int theId) override;							// 0x401080 slot 4
	virtual void	ButtonDepress(int theId) override;							// 0x40e4f0 slot 5
	virtual void	UpdateFrames() override;									// 0x406ab0 slot 8
	virtual void	Init() override;											// 0x40d2c0 slot 18
	virtual void	LoadingThreadProc() override;								// 0x4041b0 slot 20
	virtual void	Shutdown() override;										// 0x406b20 slot 38
	virtual void	PlaySample(int theSoundNum, int thePan) override;			// 0x401150 slot 46
	virtual void	SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false) override; // 0x401740 slot 59
	virtual void	SwitchScreenMode(bool wantWindowed) override;				// 0x4016f0 slot 60
	virtual void	GotFocus() override;										// 0x401040 slot 70
	virtual void	LostFocus() override;										// 0x401020 slot 71
	virtual MusicInterface* CreateMusicInterface() override;					// port: the original's is BASS
	virtual bool	HandleScriptCommand(const ScriptCommand& theCommand) override; // port: the test commands (testhooks)
	void			TraceFrame();												// port: a line of the test trace (testhooks)

	using SexyApp::PlaySample;

	void			ShowArmory();												// 0x401090
	void			DrawUpgradeWeapon(Graphics* g, int theWeapon, int x, int y, double theTurretFrame); // 0x4011b0
	int				GetBossIndex(int theLevel);									// 0x4015f0
	void			ShowCredits();												// 0x401630
	void			ShowMainMenu();												// 0x401a80
	void			DrawTank(Graphics* g, int x, int y, int theTreadFrame, double theTurretFrame); // 0x401b80
	HelpScreen*		ShowHelp(ButtonListener* theListener);						// 0x401ec0
	void			SaveUserProgress();											// 0x402860
	void			StartLevel();												// 0x403e20
	void			SaveUsers();												// 0x404c90
	void			SaveHighScores();											// 0x404e80
	void			FatalError(const std::string& theMessage);					// 0x405080
	int				GetCraftIndex(const std::string& theName);					// 0x406d70
	int				AddHighScore(bool isSurvival, bool hasFinished);			// 0x407540
	void			LoadCraft();												// 0x4094e0
	void			LoadLevels();												// 0x40a8b0
	void			LoadWaves(bool isSurvival);									// 0x40ba90
	void			LoadAnims();												// 0x40c1d0
	void			StartSurvival();											// 0x40d020
	void			StartCampaign();											// 0x40e170
	void			NewGame(bool isSurvival);									// 0x40e250
	void			ContinueGame(int theLevel);									// 0x40e2f0

	// The record of the player playing
	UserRecord&		CurrentUser() { return mUsers[mCurrentUser]; }
};

extern HeavyWeaponApp* gApp;

// port: the kill percentage the game-over box and the results screen show, (int)(destroyed / spawned * 100) as the
// original computes it on the x87; its _ftol gives 0x80000000 for the infinity or NaN of no enemy spawned, where a
// plain conversion is undefined (and traps in wasm)
int KillPercentage(int theDestroyed, int theSpawned);

}

#endif
