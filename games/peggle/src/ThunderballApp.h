// ThunderballApp.h: Peggle's application class (the original's Sexy::ThunderballApp, a global object whose
// constructor runs before WinMain; WinMain calls Init, Start and Shutdown on it).

#ifndef __THUNDERBALLAPP_H__
#define __THUNDERBALLAPP_H__

#include "CharacterMgr.h"
#include "GameMode.h"
#include "GameStats.h"
#include "HighScoreMgr.h"
#include "ImageMgr.h"
#include "ProfileMgr.h"
#include "SexyApp.h"
#include "StageMgr.h"
#include "StatsMgr.h"
#include "TrophyMgr.h"
#include "WidgetMover.h"

#include <string>

namespace Sexy
{

class BlendedImage;
class Dialog;
class Image;
class LoadingScreen;
class MainMenu;
class Widget;

// the ids of the dialogs (what GetDialog and KillDialog take); the ones that are not named are not understood yet
enum DialogId
{
	DIALOG_CONFIRM_DELETE_USER = 2,
	DIALOG_CONFIRM_MAIN_MENU = 4,
	DIALOG_CONFIRM_QUIT = 5,
	DIALOG_CONFIRM_RESTART_LEVEL = 6,
	DIALOG_CONFIRM_NEW_CHALLENGE = 7,
	DIALOG_CONFIRM_RESTART_ADVENTURE = 8,
	DIALOG_CREATE_USER = 10,
	DIALOG_NAME_ERROR = 11,
	DIALOG_TIP = 14,
	DIALOG_HIGH_SCORE_ENTRY = 16,
	DIALOG_OPTIONS = 0x13,
	DIALOG_RENAME_USER = 0x14,
	DIALOG_NAME_ERROR_2 = 0x15,
	DIALOG_USER = 0x18,
	DIALOG_LOCKED = 0x26		 // the dialog of a mode that is not unlocked yet
};

// the music ids PlayMusic takes
enum MusicId
{
	MUSIC_ODE = 0,				 // the ode of the fever
	MUSIC_AMBIENCE_A = 1000,	 // the two orders of the level music the board's ambience uses
	MUSIC_AMBIENCE_B = 2000,
	MUSIC_MENU = 3000			 // the opening
};

// the rows of mFeverStars: the star pictures of the fever effects, each in three sizes
enum FeverStarColor
{
	FEVER_STAR_GLOW = 0,
	FEVER_STAR_RED = 1,
	FEVER_STAR_GREEN = 2,
	FEVER_STAR_BLUE = 3,
	NUM_FEVER_STAR_COLORS = 4
};
static const int NUM_FEVER_STAR_SIZES = 3;
static const int NUM_BRICK_IMAGES = 8;
static const int NUM_PEG_IMAGES = 8;

class ThunderballApp : public SexyApp
{
public:
	ThunderballApp(); // 0x426f10
	~ThunderballApp() override;

	bool HandleScriptCommand(const ScriptCommand& theCommand) override; // the test commands (TestCommands.cpp)

	void Init() override; // 0x42ca30
	void LoadingThreadProc() override; // 0x421a70: loads the rest of the resources while the loading screen is up

	// The pictures the game makes from the ones it loads: the bricks and pegs (a second set for the colorblind), and the
	// stars of the fever.
	void CopyBrickTextures(); // 0x4081d0
	void CopyPegTextures();	  // 0x41c610
	void MakeFeverStars();	  // 0x408280
	bool SetColorblind(bool theColorblind); // 0x41c810: true when it changed

	// Music. The level music is every piece of music other than the two of the menus (the original lists the music folder; the
	// game ships one), loaded when the loading screen is up.
	void LoadLevelMusic(); // 0x4a6f90 (the original's thread procedure), 0x405740

	// Music. The menus' two pieces and the module of the level music (one piece for each order of it) are on three slots.
	void PlayMusic(int theMusic, bool theLoop);		 // 0x40bdd0: 0 the ode, 1000 and 2000 level music for the board, 3000 the menu's, others the level music of that id
	void CheckPlayMusic(bool thePlay);				 // 0x40d180: starts the board's music, or fades everything out
	void UpdateMusic();								 // 0x40d200: changes the music's intensity at the end of the bar playing
	void SetMusicIntensityIncreasePending(int theIntensity); // 0x4059c0: the music changes to that intensity (1 to 6) at the next bar
	void SetMusicVolume(double theVolume) override;	 // 0x405900
	void SetFeverVolume(double theVolume);			 // 0x405940
	void SyncOdeVolume();							 // 0x4058b0: the ode of the fever plays at the fever volume
	int GetMusicRow(int theMusic);					 // 0x405970 (the Mac build names it GetMusicOrder): the row a module is at, -1 if not

	// The screens: the main menu and the others are made by these (each takes the previous screen off the way it was
	// told to scroll). One of each kind is up at most.
	void ShowLoadingScreen();					  // 0x4296c0
	void ShowMainMenu();						  // 0x42d480
	void ShowStoryScreen(bool theFromMenu, bool theScrollOn); // 0x42d670
	void ShowAdventureScreen();					  // 0x429890
	void ShowLevelScreen(bool theScrollOn);		  // 0x42d350
	void ShowTrophyScreen();					  // 0x429780
	void StartAdventureGame();					  // 0x42ff70
	void ViewReplays();							  // 0x42f9d0
	bool CheckSaveGame(bool theAskToContinue);	  // 0x4306e0
	void ResetTwoPlayerStats();					  // 0x405b50
	void PauseBoard(bool thePause);				  // 0x405c30
	void ShowHelpScreen(bool theFromMenu, bool theScrollOn); // 0x423540
	void ShowBoard(bool theNewGame, bool theScrollOn);		 // 0x42f860
	void ShowReplay(bool theKeepBoard);			  // 0x42fb00

	// Dialogs. Each is a ThunderDialog that slides in; the buttons of all of them end up in DialogButtonDepress.
	Dialog* NewDialog(int theDialogId, bool isModal, const std::string& theDialogHeader, const std::string& theDialogLines,
					  const std::string& theDialogFooter, int theButtonMode) override; // 0x40c0a0: also places it
	Dialog* DoDialogScroll(int theDialogId, bool isModal, const std::string& theDialogHeader, const std::string& theDialogLines,
						   const std::string& theDialogFooter, int theButtonMode); // 0x405f20: a dialog that slides in
	void DoScrollOff(int theDialogId);			  // 0x405f60: slides a dialog out (and it is deleted)
	void DialogButtonDepress(int theDialogId, int theButtonId) override; // 0x4302a0: a button of a dialog was pressed
	void BringDialogsToFront();					  // 0x40c140
	void DoOptionsDialog();						  // 0x40c210
	void FinishOptionsDialog(bool theApply, bool theScrollOff); // 0x41c840
	void DoUserDialog();						  // 0x40c2e0
	void DoCreateUserDialog();					  // 0x40c3b0
	void DoConfirmQuitDialog();					  // 0x41c960
	void FinishConfirmQuitDialog(bool theYes);	  // 0x4084a0
	void CleanupScreen(Widget* theScreen);		  // 0x4234f0: takes the screen off the manager and deletes it
	void CleanupScreens(bool theIncludeBoard);	  // 0x429390: all of them (the board stays when told)
	void CheckScrollOff(Widget* theScreen, WidgetScroll theTo, bool theRemoveWhenDone); // 0x423480
	void ScrollOn(Widget* theScreen);			  // 0x4234c0: brings it in from the edge the last screen left to

	// the managers; the original makes them with new in its constructor and keeps them at these offsets of the app
	CharacterMgr mCharacterMgr;		 // +0x844
	HighScoreMgr mHighScoreMgr;		 // +0x848
	ImageMgr mImageMgr;				 // +0x84c (the level pictures)
	ProfileMgr mProfileMgr;			 // +0x850
	StageMgr mStageMgr;				 // +0x858
	StatsMgr mStatsMgr;				 // +0x85c
	StatsMgr mCheatStatsMgr;		 // +0x860 (what the level editor's test games record)
	TrophyMgr mTrophyMgr;			 // +0x864

	Image* mBrickImages[NUM_BRICK_IMAGES] = {}; // +0x7dc: one for each of the brick picture's cels (a copy each; owned)
	Image* mBrickGlowImage = nullptr;	   // +0x7fc (the loaded picture; not owned)
	Image* mFeverStars[NUM_FEVER_STAR_COLORS][NUM_FEVER_STAR_SIZES] = {}; // +0x800: by FeverStarColor, then the size (20, 30, 40)

	WidgetMover mWidgetMover;		 // +0x738
	WidgetScroll mScrollOnFrom = SCROLL_NONE; // +0x750: the edge the next screen comes in from (the one the last left to)

	int mGameMode = GAME_MODE_NONE;	 // +0x760
	int mStartStage = 0;			 // +0x764: the stage a game starts at
	int mStartLevel = 0;			 // +0x768
	bool mLevelMusicPending = false; // +0x779: the level music is still to be loaded
	int mCurMusic = -1;				 // +0x77c: what PlayMusic was last told to play
	int mNextLevelMusicId = 1;		 // +0x780: the music slot the next level music gets
	int mLevelMusicParam = 9;		 // +0x784 (set with the two above; read by the music code)
	int mMusicPendingRow = -1;		 // +0x788: the row the music was at when its intensity was told to change
	bool mMusicIntensityPending = false; // +0x78c: the intensity changes when the music has come to the next bar
	double mFeverVolume = 0.85;		 // +0x790
	std::string mReplayFileName;	 // +0x79c: the replay to show (empty: none)

	// the screens (null when one is not up). The board's, the help screen's and the others' types come with their classes.
	Widget* mBoard = nullptr;		 // +0x7b8
	Widget* mHelpScreen = nullptr;	 // +0x7c0
	Widget* mLevelScreen = nullptr;	 // +0x7c4
	LoadingScreen* mLoadingScreen = nullptr; // +0x7c8
	MainMenu* mMainMenu = nullptr;	 // +0x7cc
	Widget* mTrophyScreen = nullptr; // +0x7d0
	Widget* mStoryScreen = nullptr;	 // +0x7d4
	Widget* mAdventureScreen = nullptr; // +0x7d8

	PlayerInfo* mCurrentPlayer = nullptr; // +0x878 (the profile in use; owned by mProfileMgr)

	// what a game of two players keeps (ResetTwoPlayerStats): not understood yet, so named by their places
	int mUnknown87c = 0;
	int mUnknown880 = 0;
	int mUnknown884 = 0;
	int mUnknown888 = 0;
	int mUnknown88c = 0;
	int mUnknown890 = 0;
	int mUnknown894 = -1;
	int mUnknown898 = 0;
	GameStats mTwoPlayerStats[2];	 // +0x89c, +0x908: the statistics of each player of a duel
};

extern ThunderballApp* gThunderballApp;

// Globals of the original (0x650a55, 0x65057c, 0x65059c): the setting of the colorblind pictures, and the peg pictures that
// can be drawn between pixels (one for each of the eight pegs of the picture, and the shadow's).
extern bool gColorblind;
extern BlendedImage* gPegImages[NUM_PEG_IMAGES];
extern BlendedImage* gPegShadowImage;

} // namespace Sexy

#endif
