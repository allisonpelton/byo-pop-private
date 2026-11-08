#ifndef __WINDMAPP_H__
#define __WINDMAPP_H__

// WinDMApp.cpp: the application class of Bejeweled 2 ("DM" = Diamond Mine, the game's first name).
// Original: class Sexy::WinDMApp : public SexyApp, a global object (gApp @0x67ccd8) whose static constructor runs
// WinDMApp(); WinMain calls Init(), Start() and Shutdown() on it. Its functions are at 0x5dbb36-0x5e3675.
// The offsets in the comments are the original's (absolute, from the start of the object).
//
// Left out as the port leaves out the registration and the online services: the registration, nag, trial and
// time-out dialogs (RegisterDialog and the DoRegister, DoNoMorePlays, DoRegisterError and CheckForUpdates functions,
// the MaxPlays/MaxExecutions/MaxTime test), the announcements (DoAnnounceDialog, FinishAnnounceDialog and the dialogs
// 0x13, 0x14, 0x2723, 0x4e33), the "more games" screen (DoMoreGames), the URL dialogs (OpenURL, URLOpenSucceeded and
// URLOpenFailed show a dialog and the port opens no URL) and the update check.

#include "SexyApp.h"
#include "Res.h"
#include "Profile.h"

#include <list>
#include <string>
#include <vector>

namespace Sexy
{

class Board;
class TitleScreen;
class MainMenu;
class FancyBackdrop;
class HighScoreWidget;
class ReloadDisplay;
class GalaxyParser;
class BackdropsParser;
class Dialog;
class MemoryImage;
class ButtonWidget;
class WorkerThread;

// The ids of the dialogs (the dialog map of SexyAppBase). The yes, OK and footer button of dialog n sends 2000+n to
// ButtonDepress, the no and cancel button 3000+n.
enum DialogId
{
	DIALOG_NEW_GAME = 0,           // "NEWGAMEDIALOG1/2": yes starts a new game
	DIALOG_QUIT = 1,
	DIALOG_OPTIONS = 2,
	DIALOG_OPTION_VERIFY = 3,      // "keep this display mode?" after a change of the display
	// 4 to 9: the original's registration dialogs (6 the registration, 8 and 9 the nag)
	DIALOG_HIGH_SCORE_NAME = 10,   // the name of a new high score
	// 11: the original's URL dialog
	// 0x12: its update check; 0x13, 0x14: its announcements; 0x10: the registration again
	DIALOG_FULL_INSTRUCTIONS = 13, // also the colour depth message
	DIALOG_LOAD_GAME = 17,         // "Load Game?"
	DIALOG_UNDO = 22,
	DIALOG_HINT = 23,
	DIALOG_NEXT_PUZZLE_MOVE = 24,
	DIALOG_HINT_FIRST = 25,        // 25, 26, 38 and 39: the one-time hint explanations
	DIALOG_HINT_SECOND = 26,
	DIALOG_CREATE_USER = 27,
	DIALOG_RENAME_USER = 29,
	DIALOG_DELETE_USER = 30,
	// 31: the original's "more games" screen
	DIALOG_USER_LIST = 32,
	DIALOG_RESET_HIGH_SCORES = 33,
	DIALOG_PUZZLE_PACKS = 34,
	DIALOG_PUZZLE_RATING = 35,
	DIALOG_WELCOME = 36,           // the message that follows a new user's first visit to the menu
	DIALOG_MESSAGE = 37,           // the one-button messages (instructions, name conflict, profile error)
	DIALOG_HINT_THIRD = 38,
	DIALOG_HINT_FOURTH = 39,
	DIALOG_HELP = 40,
	DIALOG_ENTER_NAME = 41
};

// A puzzle rating the player gave (the 8-byte elements of the original's vector at +0x768; nothing sends them)
struct PuzzleRating
{
	short mPuzzleNum;
	unsigned char mRating;
	short mScore;    // the puzzle score / 100
	short mScoreHigh;
};

// The two colors of a backdrop (the 50 pairs at +0x814 that the BackdropsParser fills)
struct BackdropColors
{
	Color mColor1;
	Color mColor2;
};

class WinDMApp : public SexyApp
{
public:
	// the settings of config.xml (Init): the points of a level and the multiplier of a move in each mode
	int				mNormBonus;                 // +0x778 "NormBonus"
	double			mNormBonusMult;             // +0x780
	double			mNormPMBase;                // +0x788
	double			mNormPMMult;                // +0x790
	int				mEndlessBonus;              // +0x798
	double			mEndlessBonusMult;          // +0x7a0
	double			mEndlessPMBase;             // +0x7a8
	int				mTimedBonus;                // +0x7b0
	double			mTimedBonusMult;            // +0x7b8
	double			mTimedBonusAccel;           // +0x7c0
	double			mTimedPMBase;               // +0x7c8
	double			mTimedPMMult;               // +0x7d0
	double			mGraceSeconds;              // +0x7f8
	double			mGraceSecondsMult;          // +0x800
	double			mGraceSecondsMax;           // +0x808
	BackdropsParser* mBackdropsParser;          // +0x810
	BackdropColors	mBackdropColors[50];        // +0x814 the colors of each backdrop (BackdropsParser fills them)

	int				mScreenWidth;               // +0x728 the screen (GetSystemMetrics)
	int				mScreenHeight;              // +0x72c
	bool			mWantWindowed;              // +0x730 the windowed flag when the display was last set up (PreDisplayHook)
	int				mGameMode;                  // +0x740 registry "GameMode" 0 to 4 (2 = puzzle, 3 = the saved game opens)
	bool			mPuzzleFlag;                // +0x744 set by the ctor and DoNewGame, read by PuzzleStateOverlay (name pending)
	bool			mInitialMenuCleared;        // +0x745 ClearInitialMainMenu
	std::vector<std::string> mBackdrops;        // +0x748 the file names of images/backdrops, in the order of backdrops.lst
	std::vector<std::string> mPuzzlePacks;      // +0x758 the folders of puzzles/ with a galaxy.xml
	std::vector<PuzzleRating> mPuzzleRatings;   // +0x768 the ratings the player gave
	int				mCurrentSong;               // +0xe54 the module's song the game plays (0)
	int				mModeSeconds[4];            // +0xe58 registry "ModeSeconds0..3"
	Board*			mBoard;                     // +0xe68
	GalaxyParser*	mGalaxyParser;              // +0xe6c
	WorkerThread*	mWorker;                    // +0xe70 the original's task thread (the port runs its tasks in line)
	Profile*		mProfile;                   // +0xe74
	TitleScreen*	mTitleScreen;               // +0xe78
	FancyBackdrop*	mFancyBackdrop;             // +0xe7c
	MainMenu*		mMainMenu;                  // +0xe80
	bool			mReloadPending;             // +0xe84 DoMainMenu killed a game in progress: UpdateFrames reloads
	MemoryImage***	mShrunkenGems;              // +0xe88 [7 colors][6 frames] the gems shrinking
	HighScoreWidget* mHighScoreWidget;          // +0xe8c
	std::string		mLastScoreName;             // +0xe90 registry "LastScoreName"
	bool			mHighRes;                   // +0xeac registry "HighResolution": 1024x768 (else 640x480)
	bool			mAlwaysUpdate;              // +0xead registry "AlwaysUpdate"
	bool			m3dImagesLoaded;            // +0xeae
	std::list<ButtonWidget*> mButtonList;       // +0xeb0 buttons that Shutdown deletes
	MemoryImage*	mBombifiedStillStrip;       // +0xf20 GenerateBombifiedStillStrip
	bool			mIsDiamondMine;             // +0xf26 property "IsDiamondMine"
	bool			mWasInGame;                 // +0xf28 a game was in progress when the display reloaded
	ReloadDisplay*	mReloadDisplay;             // +0xf2c
	bool			mReloading;                 // +0xf30 the reload runs (the original's thread handle)
	int				mPendingMusicOffset;        // +0xf34 the main menu music starts at this offset, -1 for none
	int				mPendingMusicDelay;         // +0xf38 updates to wait before it starts

public:
	WinDMApp();                                                     // 0x5e1b49
	virtual ~WinDMApp();                                            // 0x5e3476

	// screen scaling for the 640x480 mode
	int				ScaleNum(int theValue, int theOffset);          // 0x5dbb94
	int				ScaleNumUp(int theValue, int theOffset);        // 0x5dbbbf
	float			ScaleNum(float theValue, float theOffset);      // 0x5dbb64
	double			ScaleNum(double theValue, double theOffset);    // 0x5dbb3f

	virtual void	Init() override;                                // 0x5e2a19
	virtual void	Shutdown() override;                            // 0x5ddf3a
	virtual void	PreDisplayHook() override;                      // 0x5e0b6f
	virtual void	LoadingThreadProc() override;                   // 0x5de207
	virtual void	UpdateFrames() override;                        // 0x5e13f3
	virtual void	WriteToRegistry() override;                     // 0x5de7bb
	virtual void	ReadFromRegistry() override;                    // 0x5de9ba
	virtual void	ButtonDepress(int theId) override;              // 0x5e1ed9
	virtual Dialog*	NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines,
						const SexyString& theDialogFooter, int theButtonMode) override; // 0x5dce51
	virtual void	ModalOpen() override;                           // 0x5dd6e2
	virtual void	ModalClose() override;                          // 0x5dc069
	virtual void	GotFocus() override;                            // 0x5dc086
	virtual void	LostFocus() override;                           // 0x5dc0a3
	virtual void	AddDialog(int theDialogId, Dialog* theDialog) override; // 0x5dc03b
	virtual void	SwitchScreenMode(bool wantWindowed) override;   // 0x5e0b03
	virtual void	Done3dTesting() override;                       // 0x5dbe1f
	virtual void	PlaySample(int theSoundNum) override;           // 0x5dbe86
	virtual void	PlaySample(int theSoundNum, int thePan) override; // 0x5dbe9a
	// port: the standard test commands (docs/TEST_COMMANDS.md; TestCommands.cpp). HandleScriptCommand offers a command to them first
	virtual bool	HandleScriptCommand(const ScriptCommand& theCommand) override;
	bool			HandleTestCommand(const ScriptCommand& theCommand);
	std::string		TestScreen();
	void			TestAdvanceGoto();      // one step towards the mode that "goto <mode>" asked for (called every update)
	int				mTestGoto;              // the mode a goto waits to start, -1 for none
	bool			mTestUnlock;            // "unlock all" was given before there was a profile
	using SexyApp::SwitchScreenMode;
	using SexyApp::AddDialog;
	using SexyApp::PlaySample;

	// the functions the original added to the vtable
	void			DoGameOver() {}                                 // 0x5dc172 (does nothing)
	virtual void	DoHighScoreWidget();                            // 0x5dc0c0 (slot 0x1a0)
	virtual void	KillHighScoreWidget();                          // 0x5dc140 (slot 0x1a4)

	void			SetupDialog(Dialog* theDialog, int theWidth);   // 0x5dbf24 fonts, margins and position

	void			KillMainMenu();                                 // 0x5dbbef also the fancy backdrop
	void			DoNewGame();                                    // 0x5dbc4e
	void			KillBoard();                                    // 0x5dbdba
	void			DoOptions();                                    // 0x5dc173
	void			DoPuzzlePackSelector();                         // 0x5dc1f6
	void			DoPuzzleRating();                               // 0x5dc25b
	void			DoUserDialog();                                 // 0x5dc2ed
	void			DoCreateUserDialog();                           // 0x5dc352
	void			DoRenameUserDialog(const std::string& theOldName); // 0x5dc3c9
	void			DoHelpScreen();                                 // 0x5dc44d
	void			FinishHelpScreen(bool theNeverShowAgain);       // 0x5dc501
	void			MakeShrunkenGems();                             // 0x5dc96d
	void			GenerateBombifiedStillStrip();                  // 0x5dcb1b
	void			DoMainMenu();                                   // 0x5dcd44
	void			DoBoardShow();                                  // 0x5de1a6
	void			ClearInitialMainMenu();                         // 0x5de14d
	void			KillTitleScreen();                              // 0x5dec6d
	void			LoadingCompleted();                             // 0x5decdc
	void			DoReloading(bool wantWindowed, bool is3d);      // 0x5e0479
	void			DoneReloading();                                // 0x5e0777
	void			ReloadThreadProc();                             // 0x5dddd9
	void			DeleteReloadResources();                        // 0x5ddc91
	void			LoadGalaxyData();                               // 0x5e0957
	void			ValidatePuzzlePack();                           // 0x5dd641
	void			NewGame();                                      // 0x5e0aa4
	void			TryConfirmLoad();                               // 0x5df168
	void			ConfirmNewGame();                               // 0x5e119e
	void			ConfirmResetHighScores();                       // 0x5df216
	void			ConfirmQuit();                                  // 0x5df2d0
	void			ShowInstructionsDialog();                       // 0x5df3d7
	void			ShowFullInstructionsDialog();                   // 0x5df4d9
	void			ShowColorDepthDialog();                         // 0x5df614
	void			FinishUserDialog(bool theOk);                   // 0x5df7bc
	void			FinishCreateUserDialog(bool theOk);             // 0x5df8cb
	void			DoConfirmDeleteUserDialog(const std::string& theName); // 0x5dfae8
	void			FinishConfirmDeleteUserDialog(bool theYes);     // 0x5dda71
	void			FinishRenameUserDialog(bool theOk);             // 0x5dfb97
	void			FinishOptionsDialog(bool theOk);                // 0x5e1287
	void			FinishPuzzlePackSelector(bool theOk);           // 0x5e15ea
	void			FinishPuzzleRatingDialog(bool theOk);           // 0x5e1a45

	// the lists of the backdrop images (0x5e2531) and of the puzzle packs (0x5e2832), made by Init
	void			BuildBackdropList();
	void			BuildPuzzlePackList();

private:
	// a dialog of the one-button message kind: a header, the text and a footer (the OK button's text)
	Dialog*			ShowMessage(int theDialogId, const std::string& theHeader, const std::string& theLines, int theButtonMode);
};

extern WinDMApp* gApp;      // 0x67ccd8

}

#endif
