#ifndef __HAMMERHEADS_APPLICATION_H__
#define __HAMMERHEADS_APPLICATION_H__

// The application of Hammer Heads Deluxe (0x41e030-0x422dd0 and its construction at 0x41ffd0): the screens a game moves
// through (the loading screen, the main menu, the game, the shop, the challenge, the trophy room), the dialogs, the pause
// count, the music, and the cheat words. Original: class Sexy::Application : public SexyApp (the 2004 SexyApp), a global
// (gApp = DAT_005cea54). The offsets in the comments are the original's.
//
// Left out as the port's rule: the registration code (the demo's limits, the upsell, the DRM checks), the "More Games" and
// the other buttons that leave the game, the editor (ToolWidget) that the game screens derive from in the original, and the
// how-to-play movie (the SWF player is shared work that is not here yet: ShowHowToPlay does nothing).

#include <string>
#include <vector>

#include "Res.h"
#include "SexyApp.h"

namespace Sexy
{

class ChallengeScreen;
class Dialog;
class GameScreen;
class GameWonWidget;
class HowToPlayWidget;
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

enum GameMode
{
	GAME_MODE_CLASSIC = 0,
	GAME_MODE_TOUGH_COOKIE = 1,
	GAME_MODE_MARATHON = 2,
	NUM_GAME_MODES = 3
};

// The ids of the dialogs (the first argument of DoDialog, AddDialog and GetDialog)
enum DialogId
{
	DIALOG_GAME_MENU = 0,				// the menu of a game on screen (0x44fba0)
	DIALOG_USER = 4,					// the player list (0x462c50)
	DIALOG_CREATE_USER = 5,				// the box that makes the first player (0x44e750(false))
	DIALOG_CREATE_ANOTHER_USER = 6,		// the box that makes another player (0x44e750(true))
	DIALOG_ERROR = 8,
	DIALOG_DELETE_USER = 9,				// "Are You Sure?" (0x421480)
	DIALOG_LEAVE_GAME = 0xb,			// "Leave Game?" (0x422500)
	DIALOG_CONTINUE = 0xc,				// "Continue?" a saved game (0x421790)
	DIALOG_READY_GO = 0xd,				// "Ready?" (0x45cde0)
	DIALOG_HIGH_SCORE_TABLE = 0xe,		// the table of the best scores (0x456810)
	DIALOG_GAME_OVER = 0xf,
	DIALOG_HIGH_SCORES = 0x10,
	DIALOG_BONUS_ROUND = 0x13,			// "Bonus Round!" (0x421ac0)
	DIALOG_TOUGH_UNLOCKED = 0x16,
	DIALOG_GAME_WON = 0x17
};

// What the zoom wipe shows (0x41eab0): the picture it grows from and into
enum WipeKind
{
	WIPE_HAMMER = 0,
	WIPE_TROPHY = 1,
	WIPE_LEVEL_UP = 2,
	WIPE_SHOP = 3
};

// The tracks of the music, the first argument of StartMusic (the files are loaded in order by LoadingThreadProc)
enum MusicTrack
{
	MUSIC_MENU = 0,
	MUSIC_GAME = 1,
	MUSIC_HIGH_SCORE = 2,
	MUSIC_BONUS = 3
};

class Application : public SexyApp
{
public:
	enum
	{
		NUM_CHEATS = 9,
		CHEAT_NO_MISS_DAMAGE = 0,		// "nomissdamage"
		CHEAT_NO_AWAY_DAMAGE = 1,		// "noawaydamage"
		CHEAT_SUPER_POWER = 2,			// "superpower"
		CHEAT_JUMP_TO_5 = 3,			// 3 to 7: "jumpto5", "jumpto10", "jumpto15", "jumpto20", "jumpto25"
		CHEAT_NEVER = 8					// the table's last word is empty: no typing can reach it
	};

	typedef void (*WipeCallback)();

	ParticleDefs*	mParticleDefs;			// +0x6d8: made when the loading is done
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
	int				mStartLevel;			// +0x744: the restart point the next game starts at (0 for a new game)
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

	// The framework's hooks
	virtual void	Init() override;									// 0x4205f0 (slot 45)
	virtual void	LoadingThreadProc() override;						// 0x420950 (slot 22)
	virtual void	LoadingThreadCompleted() override;					// 0x41e660 (slot 16)
	virtual void	UpdateFrames() override;							// 0x41f040 (slot 8)
	virtual void	ShutdownHook() override;							// 0x41f420 (slot 20): the screens are taken down
	virtual Dialog*	NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines,
						const SexyString& theDialogFooter, int theButtonMode) override;	// 0x41ee10 (slot 25)
	virtual void	AddDialog(int theDialogId, Dialog* theDialog) override;	// 0x41f260 (slot 69): the dialog takes the focus
	using SexyApp::AddDialog;
	virtual void	GotFocus() override;								// 0x41fb00
	virtual void	LostFocus() override;								// 0x41fb60
	virtual void	DialogButtonDepress(int theDialogId, int theButtonId) override;	// 0x422ac0 (the DialogListener's slot 1)

	// The original's own virtuals (slots 97 and 98)
	virtual void	LoadingScreenDone();								// 0x4220d0: the loading screen's wipe is over, the menu comes
	virtual void	DialogClosed(int theDialogId, int theResult);		// 0x4222a0: a moving dialog was answered or left (the result is a button id, or 999 when it was left with the escape key)

	// State
	bool			GetCheatFlag(int theIndex) const;					// 0x41f250
	void			ToggleCheat(int theIndex);							// 0x41f210
	void			ClearCheats();										// 0x41f1f0
	bool			IsShopOpen(bool theIncludeOpening = false) const;	// 0x41f150 (false: the screen exists; true: or it is being zoomed into)
	bool			IsChallengeOpen(bool theIncludeOpening = false) const;	// 0x41f1a0
	bool			IsPaused() const;									// 0x41ebc0: the pause count is above 0
	bool			IsMenuOpen();										// 0x41ebf0: the game menu or the leave box is up
	bool			IsReadyGoOpen();									// 0x41ec20
	bool			IsBonusRoundOpen();									// 0x41f100
	bool			HasFewRestartPoints();								// 0x41f290: no player, or fewer than three restart points of the classic mode
	bool			IsToughUnlocked();									// 0x41f2d0: the current player has it open

	// The screens
	void			PauseGame(bool thePause);							// 0x41f790: counts up or down; the game screen stops or goes on
	void			ShowGameMenu();										// 0x41f890
	void			EndGameMenu();										// 0x41f8c0
	void			ProcessPending();									// 0x422770: what a game screen's removal sets off
	void			StartMusic(int theTrack, bool theSlowFade);			// 0x41cfb0
	void			FadeOutMusic();										// 0x41cf80
	void			DoShopScreen();										// 0x41fa90
	void			DoChallengeScreen();								// 0x41fac0
	void			DoGameOver();										// 0x41fa70
	void			LevelFinished();									// 0x422860
	void			GameToTrophyRoom(const std::vector<int>& theTrophies);	// 0x41f950
	void			NewGame(int theMode);								// 0x422a80
	void			Quit();												// the game is to end (mQuitting)

	// The zoom wipe: the picture grows over the screen, the callback runs, the picture shrinks away
	void			DoZoomWipe(WipeCallback theCallback, int theKind);	// 0x41eab0
	void			EndZoomWipe();										// 0x41f380

	// Making and taking down the screens
	void			CreateGameScreen(bool theContinue, bool theFromRestartPoint);	// 0x41e6e0
	void			CreateGame(bool theContinue);						// 0x420ff0: the screen, then the intro or the "Ready?" box
	void			CreateMainMenu();									// 0x420dc0
	bool			CreateLevelIntro();									// 0x420c80: true when it made the "Ready?" box instead of an intro widget
	void			CreateShopScreen();									// 0x41e810
	void			CreateChallengeScreen();							// 0x41e8b0
	void			CreateLoadingScreen();								// 0x41e9f0
	void			CreateTrophyRoom(int theMode, const std::vector<int>& theTrophies);	// 0x41f640
	void			OpenShopScreen();									// 0x41f130: the screen made and opened (also what a loaded game in the shop does)
	void			OpenChallengeScreen();								// 0x41f180
	void			ShowHighScoreTableFromMenu();					// 0x41ed40: the menu goes, the high score table comes
	void			DeleteGameScreen();									// 0x41e960
	void			DeleteChallengeScreen();							// 0x41e9a0
	void			RemoveAllScreens();									// 0x41ec90: the game screen, the shop, the challenge, the resurrect screen
	void			RemoveWidgetAndDelete(Widget*& theWidget);			// the repeated RemoveWidget, SafeDeleteWidget, null

	// The dialogs
	void			DoReadyGoDialog(bool thePlaySound, const std::string& theHeader, const std::string& theText);	// 0x41fec0
	void			DoLeaveGameDialog();								// 0x422500: the box, or at once when no game is on
	void			DoBonusRoundDialog();								// 0x421ac0
	void			DeleteUserDialog(const std::string& theName);		// 0x421480: "Are You Sure?"
	void			DeleteSelectedUser(bool theDelete);					// 0x4215f0: the answer to it
	bool			ShowContinueDialog();								// 0x421790: false when the player has no saved game
	void			CenterDialog(Dialog* theDialog);					// 0x41eda0
	void			ShowUserDialog();									// 0x41f060: the player list
	void			ReopenCreateUserDialog();							// 0x41f070
	void			CreateUserDialogDone();								// 0x41f090
	void			ShowCreateAnotherUserDialog(const std::string& theText);	// 0x41f0b0
	void			CreateAnotherUserDialogDone();						// 0x41f0d0
	void			UserDialogClosed(bool theSelect);					// 0x41f980: the player list was left; theSelect makes the chosen player the current one
	bool			ShowHighScores(bool theShow);						// 0x41fdc0: records the score; true when the table is shown

	// What the buttons and the wipes lead to
	void			LeaveConfirmed();									// 0x422260: "Yes" of the leave box: the wipe back to the menu
	void			LeaveShop();										// 0x422480
	void			LeaveChallenge();									// 0x4224a0
	void			GameOverDone();										// 0x422460
	void			RetryFromGameOver();								// 0x422bb0
	void			MainMenuFromGameScreen();							// 0x421150
	void			ShopDone();											// 0x421ca0
	void			ChallengeDone();									// 0x421d70
	void			NextLevelStarts();									// 0x421e70
	void			ResurrectBackToGame();								// 0x4212f0
	void			ResurrectDone();									// 0x422140
	void			ResurrectOrPlay(bool theFlag);						// 0x422180: the restart point choice, or the game at once
	void			GameEndToMainMenu();								// 0x4213f0
	void			HighScoresToMainMenu();								// 0x4213d0
	void			TrophyRoomToMainMenu();								// 0x4211c0
	void			ShowTrophyRoomFromMenu();							// 0x41f700
	void			GameWonToMainMenu();								// 0x421c30
	void			LevelIntroDone();									// 0x41fbf0
	void			ShowHowToPlay(bool theFlag);						// 0x41ec30 (skipped)
	void			HideHowToPlay();									// 0x41ec50 (skipped)

	// The cheat words
	void			KeyChar(char theChar);								// 0x421ec0: the typed letters of the cheat words (called by the main menu and the game screen)

	// The wipes' callbacks (the original's tiny functions at 0x422070 and after, each a call on gApp)
	static void		WipeToMainMenu();									// 0x422070 -> MainMenuFromGameScreen
	static void		WipeToShop();										// 0x422080 -> ShopDone
	static void		WipeToGame();										// 0x422090 -> ChallengeDone
	static void		WipeNextLevel();									// 0x4220a0 -> NextLevelStarts
	static void		WipeGameWon();										// 0x41fd00
	static void		WipeNewGame();										// 0x422a10: the continue box was answered "New Game"
	static void		WipeContinueGame();									// 0x422950
	static void		WipeOpenShop();										// 0x41f310
	static void		WipeOpenChallenge();								// 0x41f330

private:
	void			SetCursorImages();									// 0x41e5b0
};

extern Application* gApp;

}

#endif
