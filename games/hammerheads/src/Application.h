#ifndef __HAMMERHEADS_APPLICATION_H__
#define __HAMMERHEADS_APPLICATION_H__

// The application of Hammer Heads Deluxe (0x41e030-0x422dd0 and its construction at 0x41ffd0): the screens a game moves
// through (the loading screen, the main menu, the game, the shop, the challenge, the trophy room), the dialogs, the pause
// count, the music, and the cheat words. Original: class Sexy::Application : public SexyApp (the 2004 SexyApp), a global
// (gApp = DAT_005cea54). The offsets in the comments are the original's.
//
// Left out as the port's rule: the registration code (the demo's limits, the upsell, the DRM checks), the "More Games" and
// the other buttons that leave the game, and the editor (ToolWidget) that the game screens derive from in the original.

#include <string>
#include <vector>

#include "Res.h"
#include "SexyApp.h"

namespace Sexy
{

class ChallengeScreen;
class Dialog;
class GameScreen;
class LevelIntroWidget;
class LoadingScreen;
class MainMenu;
class ParticleDefs;
class ParticleRand;
class ResurrectScreenWidget;
class ShopScreen;
class TrophyRoomWidget;
class UserManager;
class Widget;
class ZoomWipeWidget;
class GameWonWidget;
class HowToPlayWidget;

enum GameMode
{
	GAME_MODE_CLASSIC = 0,
	GAME_MODE_TOUGH_COOKIE = 1,
	GAME_MODE_MARATHON = 2,
	NUM_GAME_MODES = 3
};

// The ids of the dialogs (the first argument of DoDialog and GetDialog)
enum DialogId
{
	DIALOG_NEW_GAME = 0,				// "GameMenu" opened in the main menu: the game menu of a game on screen
	DIALOG_OPTIONS = 4,
	DIALOG_USER = 5,
	DIALOG_CREATE_USER = 6,
	DIALOG_ERROR = 8,
	DIALOG_DELETE_USER = 9,
	DIALOG_LEAVE_GAME = 0xb,
	DIALOG_CONTINUE = 0xc,
	DIALOG_GAME_OVER = 0xf,
	DIALOG_HIGH_SCORES = 0x10,
	DIALOG_BONUS_ROUND = 0x13,
	DIALOG_TOUGH_UNLOCKED = 0x16
};

// What the zoom wipe shows (0x41eab0): the picture it grows from and into
enum WipeKind
{
	WIPE_HAMMER = 0,
	WIPE_TROPHY = 1,
	WIPE_LEVEL_UP = 2,
	WIPE_SHOP = 3
};

class Application : public SexyApp
{
public:
	enum
	{
		NUM_CHEATS = 9,
		CHEAT_NO_MISS_DAMAGE = 0,
		CHEAT_NO_AWAY_DAMAGE = 1,
		CHEAT_SUPER_POWER = 2,
		CHEAT_JUMP_TO_5 = 3,			// 3 to 7: jumpto5, 10, 15, 20, 25
		CHEAT_NEVER = 8					// the table's last word is empty: no typing can reach it
	};

	ParticleDefs*	mParticleDefs;			// +0x6d8
	ParticleRand*	mParticleRand;			// +0x6dc
	bool			mCheatFlags[NUM_CHEATS];	// +0x6e0
	std::string		mTyped;					// +0x6ec: the letters typed so far, for the cheat words
	int				mScreenWidth;			// +0x70c: the desktop's size
	int				mScreenHeight;			// +0x710
	bool			mQuietSpawn;			// the gnomes the shop sends up do not drop hearts (set while the shop is filled)
	bool			mLevelUpPending;		// +0x71f: the level-up wipe is running, the next level is to start
	bool			mGameWonPending;		// +0x721: the last level was won, the game-won screen comes
	GameScreen*		mGameScreen;			// +0x728
	ShopScreen*		mShopScreen;			// +0x72c
	bool			mShopOpening;			// +0x730: the zoom into the shop runs (or the shop is open)
	ChallengeScreen* mChallengeScreen;		// +0x734
	bool			mChallengeOpening;		// +0x738
	bool			mChallengeLeaving;		// +0x739
	LoadingScreen*	mLoadingScreen;			// +0x73c
	ResurrectScreenWidget* mResurrectScreen;	// +0x740
	int				mStartLevel;			// +0x744: the level the next game starts at (a saved game's), 0 for a new one
	GameWonWidget*	mGameWon;				// +0x748
	HowToPlayWidget* mHowToPlay;			// +0x74c
	int				mPauseCount;			// +0x750
	MainMenu*		mMainMenu;				// +0x754
	TrophyRoomWidget* mTrophyRoom;			// +0x758
	bool			mQuitting;				// +0x75c: the game is to end when the update is done
	LevelIntroWidget* mLevelIntro;			// +0x760
	double			mFade;					// +0x768: how far the screen is dimmed behind a dialog, 0 to 1
	bool			mZooming;				// +0x770: a zoom wipe runs
	ZoomWipeWidget*	mZoomWipe;				// +0x774
	int				mGameMode;				// +0x778
	int				mFocusedUpdate;			// +0x780: the update count of the game screen when it was last unpaused (a click in the same update is eaten)
	UserManager*	mUserManager;			// +0x784

public:
	Application();										// 0x41ffd0
	virtual ~Application();								// 0x420400

	virtual void	Init() override;									// 0x4205f0
	virtual void	LoadingThreadProc() override;						// 0x420950
	virtual void	LoadingThreadCompleted() override;					// 0x41e660
	virtual Dialog*	NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines,
						const SexyString& theDialogFooter, int theButtonMode) override;	// 0x41ee10
	virtual void	GotFocus() override;								// 0x41fb00
	virtual void	LostFocus() override;								// 0x41fb60
	virtual void	ButtonPress(int theId) override;					// 0x422ac0 (the dialogs' buttons)
	virtual void	ButtonDepress(int theId) override;					// 0x4222a0
	virtual void	ShutdownHook() override;							// 0x41f040

	bool			GetCheatFlag(int theIndex) const;					// 0x41f250
	bool			IsShopOpen() const;									// 0x41f150 (false: the screen exists)
	bool			IsChallengeOpen() const;							// 0x41f1a0
	bool			IsPaused() const;									// 0x41ebc0: the pause count is above 0

	void			PauseGame(bool thePause);							// 0x41f790: counts up or down; the game screen stops or goes on
	void			StartMusic(int theTrack, bool theFadeIn);			// 0x41cfb0
	void			FadeOutMusic();										// 0x41d0f0
	void			DoShopScreen();										// 0x41fa90
	void			DoChallengeScreen();								// 0x41fac0
	void			DoGameOver();										// 0x41fa70
	void			LevelFinished();									// 0x422860
	void			GameToTrophyRoom(const std::vector<int>& theTrophies);	// 0x41f950
};

extern Application* gApp;

}

#endif
