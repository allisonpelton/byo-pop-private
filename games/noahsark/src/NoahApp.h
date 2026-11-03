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
	FONT_REGISTER = 8,           // data/registerfont.txt   app+0x404
	NUM_FONTS
};

// app+0x444: the game type chosen on the menu
enum GameMode
{
	GAMEMODE_ACTION = 1,         // "Flood" in the web version: the water rises in real time
	GAMEMODE_STRATEGY = 3,       // "Freefall": turn based, the animals fall
	GAMEMODE_PUZZLE = 4,         // swap adjacent animals to make rows of three (a time limit in the trial)
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
	DIALOG_REGISTER = 5,
	DIALOG_REG_INVALID = 6,
	DIALOG_NO_WINDOWED = 7,
	DIALOG_HIGH_SCORE_NOTE = 9,
	DIALOG_HIGH_SCORES = 10,
	DIALOG_RECORD_SCORE = 11,
	DIALOG_DIFFICULTY = 12,
	DIALOG_PUZZLE_TIME_UP = 13
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

	Image**					mImages;            // +0x3e0 new Image*[83] (NUM_IMAGES), loaded by Init (GetImage)
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
	int						mFloodStart;        // +0x47c the water's level at the start of the level
	int						mAnimalsSaved;      // +0x480
	int						mComboCount;        // +0x484 pairs of a kind joined in a row at least twice
	int						mBestCombo;         // +0x488
	int						mSavedThisLevel;    // +0x48c animals saved on the level just ended (NextLevelWidget takes it)
	bool					mShowHints;         // +0x490 registry ShowHints (the options dialog's check box)
	bool					mShowTutorial;      // +0x491 registry ShowTutorial
	DWORD					mGameStartTick;     // +0x494
	SexyString				mLastScoreName;     // +0x498 registry LastScoreName
	int						mTimesPlayed;       // +0x3b0
	bool					mRegistered;        // +0x328 (always true in the port: see GAME_NOTES.md section 3)
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
	virtual void			LoadingThreadProc() override;
	virtual void			LoadingThreadCompleted() override;
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

	// Helpers of the module (WinNoah.exe addresses in NoahApp.cpp)
	bool					LoadImage(int theImageId);
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
	void					ShowRegisteredDialog();
	void					ShowCredits();
	void					ShowRecordScore(const SexyString& thePlace);
	void					PlayGameMusic();
	bool					HasSavedGame();
	void					DeleteSavedGame();
};

extern NoahApp* gApp;

}

#endif
