#ifndef __BIGMONEY_WINBMAPP_H__
#define __BIGMONEY_WINBMAPP_H__

// WinBMApp.cpp: the application class of Big Money Deluxe.
// Original: class Sexy::WinBMApp : public SexyApp (SexyAppFramework 1.2), sizeof 0x828, the static
// global gWinBMApp @0x52ddc8 (dynamic initializer 0x4f3a20, atexit dtor 0x4f4150); WinMain (0x41b670)
// calls Init() (non-virtually), Start() and Shutdown() on it.
// Vtables: primary 0x4fc6e4 (97 slots: 0-6 ButtonListener, 7-89 SexyAppBase, 90-95 SexyApp, 96 new),
// DialogListener 0x4fc6d8. sizeof(SexyApp) is 0x6d0, so WinBMApp's data starts at +0x6d0; offsets below
// are absolute (the port's base classes have other sizes, nothing depends on them).
// The original ctor leaves +0x6d0, +0x6d8, +0x6e0, +0x6e8 and +0x825 to the zero-initialised global;
// the port initialises them.
// Behaviour spec: re/bigmoney/analysis/app.md.

#include "SexyApp.h"
#include "Res.h"
#include <cstdint>
#include <list>
#include <vector>
#include <string>

namespace Sexy
{

class Board;
class TitleScreen;
class GameSelector;
class HelpWidget;
class MrMoney;
class Image;
class MemoryImage;
class ImageFont;
class ButtonWidget;
class Dialog;
class MusicInterface;

class AutoplayBot;
namespace Replay { struct File; }

class WinBMApp : public SexyApp
{
public:
	enum
	{
		NUM_LOADING_TASKS = 72,         // mNumLoadingThreadTasks; LoadingThreadProc completes only 64 of them
		NUM_MRMONEY_IMAGES = 6,
		NUM_BILL_IMAGES = 4
	};

	Board*			mBoard;                     // +0x6d0 current board (NewGame / RemoveBoard)
	MrMoney*		mMrMoney;                   // +0x6d4 made by TitleScreenIsFinished
	TitleScreen*	mTitleScreen;               // +0x6d8 until TitleScreenIsFinished
	GameSelector*	mGameSelector;              // +0x6dc ShowGameSelector / RemoveGameSelector
	HelpWidget*		mHelpWidget;                // +0x6e0 ShowHelp / RemoveHelp
	Image**			mImages;                    // +0x6e4 new Image*[NUM_IMAGES] (DDImages from GetImage)
	MemoryImage*	mBrightCoinsImage;          // +0x6e8 IMAGE_COINS with c' = min(255, c*280/255 + 48); never deleted
	// port(mods): the coins of the extra colours 5..7 (Piece::MAX_COIN_COLORS), one row each, from IMAGE_COINS,
	// IMAGE_HORZCOINS and mBrightCoinsImage (MakeExtraCoins)
	MemoryImage*	mExtraCoinsImage;
	MemoryImage*	mExtraHorzCoinsImage;
	MemoryImage*	mExtraBrightCoinsImage;
	ImageFont*		mFont1;                     // +0x6ec bigmoneyfont1
	ImageFont*		mFont1NoShadow;             // +0x6f0 copy + "noshadow"
	ImageFont*		mFont1OutlineNoShadow;      // +0x6f4 copy + "outline" + "noshadow"
	ImageFont*		mFont1Underlined;           // +0x6f8 copy + "underlined"
	ImageFont*		mFont1OutlineUnderlined;    // +0x6fc copy + "outline" + "underlined"
	ImageFont*		mFont3;                     // +0x700 bigmoneyfont3 (dialog button and lines font)
	ImageFont*		mFont3Outline;              // +0x704 copy + "outline"
	ImageFont*		mFont3Underlined;           // +0x708 copy + "underlined"
	ImageFont*		mFont3OutlineUnderlined;    // +0x70c copy + "outline" + "underlined"
	ImageFont*		mScoreFont;                 // +0x710 scorefont (point size 25)
	ImageFont*		mLevelFont;                 // +0x714 ImageFont(IMAGE_LEVELFONT, levelfont.txt)
	ImageFont*		mFloatFont;                 // +0x718 floatfont
	ImageFont*		mScoreFontSmall;            // +0x71c copy of mScoreFont, SetPointSize(20)
	std::list<ButtonWidget*> mButtonList;       // +0x720 side buttons (MakeButton; _Myhead +0x724, _Mysize +0x728)
	std::list<Image*> mButtonImageList;         // +0x72c side-button over/down copies (deleted by the dtor)
	Image*			mCheckboxCheckedImage;      // +0x738 CopyImage(IMAGE_CHECKBOXES, Rect(22, 0, 22, 23))
	Image*			mCheckboxUncheckedImage;    // +0x73c CopyImage(IMAGE_CHECKBOXES, Rect(0, 0, 22, 23))
	Image*			mStartArrowsMirrored;       // +0x740 CopyImage(IMAGE_STARTARROWS) + MirrorImage
	Image*			mGameTypeButtonImages[3][4];// +0x744 [type][normal, over, down, unused]
	Image*			mSkillButtonImages[3][4];   // +0x774 [skill - 1][normal, over, down, unused]
	Image*			mSelectorButtonImages[3][3];// +0x7a4 [GameSelector button 0..2][button, over, down]; only [1][0] (+0x7b0) is set
	Image*			mMrMoneyImages[NUM_MRMONEY_IMAGES]; // +0x7c8 mrmoney + mouth frame j (frame 5 also has mouth 0 on top)
	Image*			mBillImages[NUM_BILL_IMAGES];// +0x7e0 IMAGE_BILL scaled 0.9, 0.8, 0.7, 0.6 (Board reads app+0x7dc + 4n, n = 1..4)
	Image*			mCobrandImage;              // +0x7f0 images\cobrand if property Cobranded (else NULL)
	bool			mHelpShownThisSession[NUM_GAMETYPES]; // +0x7f4 help screen already shown before a game of this type
	// +0x7f7 pad
	std::string		mLastScoreName;             // +0x7f8 registry LastScoreName; prefills RecordScoreDialog
	int				mSkillLevel;                // +0x814 1..3 (GameSelector: skill button id - 10); ctor 1; registry SkillLevel
	int				mGameType;                  // +0x818 GameType; ctor 0; registry GameType
	bool			mBagCollectedOnce;          // +0x81c a bag was collected this run: suppresses Board hint 5 and the column-13 rule
	bool			mBombSeenOnce;              // +0x81d a dollar bomb detonated this run: suppresses Board hint 3
	bool			mDangerHintShown;           // +0x81e Board "coins near the top" hint shown this run (not Puzzle)
	bool			mHasClickedGroup;           // +0x81f a group was clicked this run (Board idle hint at update 600)
	bool			mNoPopLink;                 // +0x820 registry NoPopLink, read only, never used
	bool			mWelcomePlayed;             // +0x821 GameSelector "welcome" played this run
	bool			mDontShowHelp[NUM_GAMETYPES]; // +0x822 registry DontShowHelp0..2
	bool			mShowInstructor;            // +0x825 help was shown before this game: the Board shows its instructor
	// +0x826..+0x827 pad (sizeof 0x828)

public:
	WinBMApp();                                                     // 0x41c790
	virtual ~WinBMApp();                                            // 0x41c240 (scalar deleting dtor 0x41c9d0, slot 17)

	virtual void	ButtonPress(int theId) override;                         // 0x41bfa0 slot 1 (PlaySample(SOUND_BUTTONCLICK))
	virtual void	ButtonDepress(int theId) override;                       // 0x420f10 slot 2
	virtual void	LoadingThreadCompleted() override;                       // 0x421450 slot 16 (update check prompt, not ported)
	virtual MusicInterface* CreateMusicInterface() override;                 // port: SexyAppBase's default in the original (BASS); here SDLMusicInterface
	virtual void	LoadingThreadProc() override;                            // 0x41efd0 slot 22
	virtual void	WriteToRegistry() override;                              // 0x41c9f0 slot 23
	virtual void	ReadFromRegistry() override;                             // 0x41cb60 slot 24
	virtual Dialog*	NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) override; // 0x41bd90 slot 25
	virtual void	URLOpenFailed(const std::string& theURL) override;       // 0x41fe80 slot 33
	virtual void	URLOpenSucceeded(const std::string& theURL) override;    // 0x41bf80 slot 34
	virtual bool	OpenURL(const std::string& theURL, bool shutdownOnOpen = false) override; // 0x420140 slot 35
	virtual void	Init() override;                                         // 0x41d9a0 slot 45
	virtual bool	KillDialog(int theDialogId) override;                    // 0x41c1c0 slot 71
	virtual void	ModalOpen() override;                                    // 0x41bf00 slot 73 (pauses Action only)
	virtual void	ModalClose() override;                                   // 0x41bf20 slot 74
	virtual void	GotFocus() override;                                     // 0x41bf40 slot 75
	virtual void	LostFocus() override;                                    // 0x41bf60 slot 76
	virtual void	TitleScreenIsFinished();                        // 0x4215c0 slot 96 (new virtual; TitleScreen::ButtonDepress)

	bool			LoadAppImage(int theImageId);                   // 0x41d7d0
	void			RemoveBoard();                                  // 0x41b6a0
	void			ShowHelp();                                     // 0x41b6f0
	void			RemoveHelp();                                   // 0x41b7f0
	void			ShowGameSelector();                             // 0x41b840
	void			RemoveGameSelector();                           // 0x41b9c0
	void			ShowHighScoreDialog();                          // 0x41ba00 (dialog 13)
	void			ShowCreditsDialog();                            // 0x41baf0 (dialog 14)
	void			ShowOptionsDialog();                            // 0x41bbe0 (dialog 1)
	void			ShowRecordScoreDialog(const SexyString& theLines); // 0x41bd00 (dialog 3)
	void			NewGame();                                      // 0x41bfb0
	void			StartGame();                                    // 0x41c170 (help screen first, or NewGame)
	void			ShowRegisterDialog();                           // 0x41cd60 (registered: the "already registered" notice, dialog 16)
	void			ConfirmContinueGame();                          // 0x420370 (dialog 5)
	void			ConfirmQuit();                                  // 0x420560 (dialog 12)
	void			ConfirmNewGame();                               // 0x4207e0 (dialog 0)
	ButtonWidget*	MakeButton(int theId, const Rect& theRect, Image* theOverImage, Image* theDownImage); // 0x421470

	// Not ported (registration / trial / update check; the Steam build is registered and up to date):
	// ShowInvalidCodeDialog 0x41d090, ShowPuzzleTrialDialog 0x41d2e0, ShowPuzzleNextLevelLocked 0x41d4c0,
	// ShowRegisterToRecordScoreDialog 0x41d6a0, ConfirmCheckForUpdates 0x420a30, DoUpdateCheck 0x420c50.

	// port (web): the page can be closed or crash without the app being deleted, which is where the
	// original saves a game in progress (see SexyAppBase::UpdateUserDataBackup)
	virtual void	GetShutdownSaves(std::vector<ShutdownSave>* theSaves) override;

	// port(mods): the mod menu's "Restart game" (docs/MOD_MENU_DESIGN.md 6, 8.3): a game that can be saved is in
	// progress (Restart asks first); no restart while the high-score name is being entered; a restart keeps the
	// replay folder and a seed still waiting for its game
	virtual bool	HasGameInProgress() override;
	virtual bool	CanRestartNow(SexyString* theReason) override;
	virtual void	GetRestartArgs(std::vector<std::string>* theArgs) override;
	virtual void	WriteShutdownRegistry() override;
	virtual void	ShutdownHook() override;                                 // port (web): ~WinBMApp's save (the app is never deleted)

	// port(mods): a seed the player chose (-gameseed=N, set by main.cpp; ?seed=N on the web), 0 for none:
	// every new game gets it until a game on it is played (Board::UsePendingSeed, ModRules.h)
	uint32_t		mPendingSeed;

	// port(mods): replays (Replay.h)
	std::string		mReplayDir;             // where recorded games go (-replaydir=; main.cpp sets the default)
	std::string		mWatchPath;             // -replay=<file>: watched once the title has loaded
	Replay::File*	mWatchFile;             // the replay a new board plays (StartWatch .. EndWatch), else NULL
	int				mWatchSpeed;            // board ticks per update while watching: 1, 2, 4 or 8
	bool			mWatchAtStart;          // the -replay watch: the continue prompt waits for its end
	int				mWatchSavedGameType;    // the app's state before the watch, given back by EndWatch
	int				mWatchSavedSkill;
	int				mWatchSavedTimesPlayed;
	bool			mWatchSavedFlags[5];
	bool			StartWatch(const std::string& thePath);   // false (and a message) when it cannot be played
	void			EndWatch();

	// port: test hooks (not in the original; the framework's -seed, -script, -fast and -trace are in
	// SexyAppBase.h). None of them changes the game or its random streams.
	//   -autoplay=<seed>       a bot with its own RNG plays through script input (testhooks/Autoplay.h);
	//                          -autoplaymode=0..2 (Action/Strategy/Puzzle), -autoplayskill=1..3,
	//                          -autoplaygames=<n> games (default 1), -autoplaycap=<n> updates
	//                          (default 200000), -autoplaylog=<file> (the input as a -script file),
	//                          -autoplayshot=<prefix> (screenshots at level ends and game overs)
	bool			mAutoplay;
	unsigned long	mAutoplaySeed;
	int				mAutoplayMode;
	int				mAutoplaySkill;
	int				mAutoplayGames;
	int				mAutoplayCap;
	std::string		mAutoplayLogFileName;
	std::string		mAutoplayShotPrefix;
	int				mAutoplaySolver;        // -autoplaysolver[=N]: the bot clicks the Puzzle solver's plan N% of the time (100)
	bool			mAutoplayHints;         // -autoplayhints: and presses H first (PuzzleSolver.h)
	int				mAutoplayUndo;          // -autoplayundo[=N] (AutoplayBot::mPressUndo)
	bool			mAutoplayCheck;         // -autoplaycheck (AutoplayBot::mCheckClicks)
	int				mAutoplayPocket;        // -autoplaypocket[=N] (AutoplayBot::mUsePockets)
	AutoplayBot*	mAutoplayBot;
	virtual void	UpdateFrames() override;
	virtual void	HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;

	using SexyApp::ButtonPress;
	using SexyApp::KillDialog;
};

}

#endif
