#ifndef __NOAH_NOAHAPP_H__
#define __NOAH_NOAHAPP_H__

// NoahApp.cpp: the application class of Noah's Ark Deluxe 1.1 and its helpers (WinNoah.exe 0x4166d0-0x41c230).
// Behaviour spec: docs/GAME_NOTES.md section 3.
//
// Original: class Sexy::NoahApp : public Sexy::SexyApp (2002 SexyApp : ButtonListener), vftable 0x4888ec (54
// slots, the same as SexyApp's), data from +0x3b0. Offsets below are the original's (code never depends on them,
// the port's base classes have other sizes).

#include "SexyApp.h"
#include "Res.h"
#include "Rect.h"
#include "Image.h"
#include "DDImage.h"
#include "DialogButton.h"
#include <list>
#include <string>
#include <vector>

namespace Sexy
{

class Board;
class TitleScreen;
class IntroWidget;
class NextLevelWidget;
class HighScoreWidget;
class Image;
class MemoryImage;
class ImageFont;
class Dialog;
class ButtonWidget;
class Widget;
class MusicInterface;
class NoahApp;

// The game's pictures by id (IMAGE_X in Res.h). mApp->mImages[IMAGE_X] decodes a picture the first time it is asked for; the original
// decodes all 82 at start (0x417450 for each id), which is most of the load and of the memory at the title, and a picture that
// nothing asks for in a run is never decoded or held. A picture that is not there is told at its first use (a popup and the end of
// the game), as the original tells it at start.
class ImageTable
{
public:
	explicit ImageTable(NoahApp* theApp);
	Image*					operator[](int theId) { return mImages[theId] != NULL ? mImages[theId] : Load(theId); }

private:
	Image*					Load(int theId);

	NoahApp*				mApp;
	Image*					mImages[NUM_IMAGES];
};

enum FontId
{
	FONT_BIG = 0,                // data/fontbig.txt        app+0x3e4
	FONT_MEDIUM = 1,             // data/fontmedium.txt     app+0x3e8
	FONT_SMALL = 2,              // data/fontsmall.txt      app+0x3ec
	FONT_COLORLESS = 3,          // data/colorlessfont.txt  app+0x3f0
	FONT_GARAMOND_MED = 4,       // data/garamondfontmed.txt   app+0x3f4
	FONT_GARAMOND_SMALL = 5,     // data/garamondfontsmall.txt app+0x3f8
	FONT_GARAMOND_TINY = 6,      // data/garamondfonttiny.txt  app+0x3fc
	FONT_SCORE = 7,              // data/scorefont.txt      app+0x400
	NUM_FONTS
};

// app+0x444: the game type chosen on the menu
enum GameMode
{
	GAMEMODE_ACTION = 1,         // "Flood" in the web version: the water rises in real time
	GAMEMODE_STRATEGY = 3,       // "Freefall": turn based, the animals fall
	GAMEMODE_PUZZLE = 4,         // swap adjacent animals to make rows of three
	GAMEMODE_STAMPEDE = 5        // the bonus round after every third level (the mode is kept in mSavedGameMode)
};

// Dialog ids (AddDialog / DoDialog / KillDialog)
enum DialogId
{
	DIALOG_NEW_GAME = 0,         // "start a new game? your saved game will be lost"
	DIALOG_QUIT = 1,             // quit to the menu / quit the game
	DIALOG_MODE_INTRO = 2,       // how to play the chosen mode
	DIALOG_OPTIONS = 3,
	DIALOG_TUTORIAL = 4,
	DIALOG_HIGH_SCORES = 10,
	DIALOG_RECORD_SCORE = 11,
	DIALOG_DIFFICULTY = 12
};

// A dialog's own buttons report to the app with the dialog's id added to one of these
enum DialogButtonBase
{
	DIALOG_YES = 2000,           // the first button: Yes, OK, or the one button of a footer
	DIALOG_NO = 3000,            // the second: No or Cancel
	DIALOG_BUTTONS_END = 4000
};

// Button ids
enum ButtonId
{
	BUTTON_ACTION = 5,
	BUTTON_STRATEGY = 6,
	BUTTON_PUZZLE = 7,
	BUTTON_CONTINUE = 8,
	BUTTON_QUIT = 9,             // the menu's quit button
	BUTTON_SIDEBAR_QUIT = 10,    // the board's quit button
	BUTTON_SIDEBAR_OPTIONS = 12,
	BUTTON_QUIT_KEY = 14
};

class NoahApp : public SexyApp
{
public:
	enum
	{
		GAME_WIDTH = 640,        // ctor: mWidth 0x280, mHeight 0x1e0
		GAME_HEIGHT = 480,
		NUM_LOADING_TASKS = 34   // one per sound
	};

	ImageTable				mImages;            // +0x3e0 new Image*[82] (NUM_IMAGES), see ImageTable
	ImageFont*				mFonts[NUM_FONTS];  // +0x3e4 .. +0x404
	Board*					mBoard;             // +0x408 the game in progress (StartGame)
	NextLevelWidget*		mNextLevelWidget;   // +0x40c shown between levels
	IntroWidget*			mIntroWidget;       // +0x410 the menu
	TitleScreen*			mTitleScreen;       // +0x414 until the first click
	HighScoreWidget*		mHighScoreWidget;   // +0x41c the game-over screen
	int						mLevel;             // +0x43c level number of the current game
	int						mDifficulty;        // +0x440 0 easy, 4 medium, 8 hard
	int						mGameMode;          // +0x444 GameMode
	int						mSavedGameMode;     // +0x448 the mode a bonus round returns to
	bool					mBonusRound;        // +0x439 a Stampede round is on
	bool					mPuzzleNoticeShown; // +0x4a9 the bonus round's how-to dialog has been shown
	int						mGameScore;         // +0x474 score of the game in progress
	int						mShownScore;        // +0x478 the score on the display, counting up to mGameScore
	int						mAnimalsSaved;      // +0x480
	int						mComboCount;        // +0x484 pairs of a kind joined in a row at least twice
	int						mBestCombo;         // +0x488
	int						mSavedThisLevel;    // +0x48c animals saved on the level just ended (NextLevelWidget takes it)
	bool					mShowHints;         // +0x490 registry ShowHints (the options dialog's check box)
	bool					mShowTutorial;      // +0x491 registry ShowTutorial
	SexyString				mLastScoreName;     // +0x498 registry LastScoreName
	bool					mSoundTried[NUM_SOUNDS];   // port: the sound's file has been asked for (see LoadSoundOnce)
	bool					mMainSongStarted;
	bool					mMainSongLooping;

	// The sidebar's two buttons (the board's left panel) and their images
	MemoryImage*			mQuitButtonImage;   // +0x464 BUTTONBACK with QUITTEXT on it
	MemoryImage*			mOptionsButtonImage;// +0x468 BUTTONBACK with OPTIONSTEXT on it
	ButtonWidget*			mQuitButton;        // +0x46c id 10
	ButtonWidget*			mOptionsButton;     // +0x470 id 12

	std::list<ButtonWidget*>	mButtons;       // +0x430 the buttons made by AddButton / AddDialogButton

public:
	NoahApp();
	virtual ~NoahApp();

	// SexyApp
	virtual void			ButtonPress(int theId) override;
	virtual void			ButtonDepress(int theId) override;
	virtual void			UpdateFrames() override;
	virtual void			PlaySample(int theSoundNum) override;
	virtual void			PlaySample(int theSoundNum, int thePan) override;
	virtual void			LoadingThreadProc() override;
	virtual void			WriteToRegistry() override;
	virtual void			ReadFromRegistry() override;
	virtual Dialog*			NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
								const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) override;
	virtual void			Init() override;
	virtual void			LostFocus() override;
	virtual void			HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;

	// Test hooks (testhooks/Autoplay.cpp): -autoplay=1 plays games by itself with a bot that has its own random
	// numbers; -autoplaymode=action|strategy|puzzle, -autoplaydiff=0|4|8, -autoplaygames=N, -autoplaycap=<updates>,
	// -autoplaylog=<file>, -autoplayshots=<prefix>
	bool					mAutoplay;
	int						mAutoplayMode;
	int						mAutoplayDiff;
	int						mAutoplayGames;
	int						mAutoplayCap;
	int						mAutoplayStop;
	bool					mAutoplayMenu;
	std::string				mAutoplayLog;
	std::string				mAutoplayShots;
	struct AutoplayState*	mAutoplayState;
	void					AutoplayUpdate();
	// -statelog=<file>: the game's state at each update that changes it (test hook, compared with origrun's --watch)
	std::string				mStateLog;
	std::string				mStateLogCells;     // -statelogcells=x,y;x,y: cells whose type is logged at every change
	struct StateLogState*	mStateLogState;
	void					StateLogUpdate();
	// Test commands (testhooks/TestCommands.cpp, docs/TEST_COMMANDS.md): state, goto, level, set, seed, win, lose, move.
	// Reached only from a -script or the live command channel; a game that is never given one is not touched.
	virtual bool			HandleScriptCommand(const ScriptCommand& theCommand) override;
	bool					HandleTestCommand(const ScriptCommand& theCommand);
	std::string				TestScreen();
	void					UpdateTestGoto();
	int						mTestGotoMode;      // "goto <mode>": the mode whose dialogs are answered as they come up (0: none)
	int						mTestGotoDiff;
	int						mTestWantMode;      // "move <mode> <difficulty>": the game the bot starts from the menu (0: none)
	int						mTestWantDiff;
	unsigned int			mTestBotSeed;       // the bot's own generator for "move" (the seed command sets it)
	// -resync=<file>: lines "n <level> <state>" and "r <level> <state>" (test hook): the C generator's state at the start of
	// each level (n) and at the end of its get-ready (r) in the original, which the port takes over there to be compared past
	// the differences of the get-ready (games/noahsark/tools/resync.py writes the file from an origrun run)
	std::string				mResync;
	struct ResyncState*		mResyncState;
	void					ResyncEvent(char theType);
	bool					ResyncHold();

	// Helpers of the module (WinNoah.exe addresses in NoahApp.cpp)
	Image*					LoadImage(int theImageId);   // 0x417450: NULL (and the game ends) when the picture is not there
	void					LoadSoundOnce(int theSoundNum);
	ButtonWidget*			AddButton(int theId, const Rect& theRect, Image* theImage, Image* theDownImage);
	DialogButton*			AddDialogButton(int theId, const Rect& theRect, Image* theImage);
	void					TitleScreenDone();
	void					ShowIntro(bool theKeepMusic);
	void					StartGame(bool theContinue);
	void					RemoveIntro();
	void					RemoveNextLevel();
	void					ShowNextLevel();
	void					ShowHighScoreWidget();
	void					RemoveHighScoreWidget();
	void					ShowDifficultyDialog();
	void					ShowOptions();
	void					ShowTutorial();
	void					ShowQuitDialog();
	void					ShowNewGameDialog(int theGameMode);
	void					ShowModeIntroDialog(int theMode);
	void					ShowHighScores();
	void					ShowCredits();
	void					ShowRecordScore(const SexyString& thePlace);
	void					PlayGameMusic();
	bool					HasSavedGame();
	void					DeleteSavedGame();
};

extern NoahApp* gApp;

}

#endif
