#ifndef __SEVENSEAS_SEVENSEASAPP_H__
#define __SEVENSEAS_SEVENSEASAPP_H__

// SevenSeasApp.cpp: the application class of Seven Seas Deluxe 1.1 and its helpers (WinSS.exe 0x41bc00-0x421a10).
// Behaviour spec: docs/GAME_NOTES.md section 2.
//
// Original: class SevenSeasApp : public Sexy::SexyApp (the 2003 SexyApp : ButtonListener), vftable 0x494080 (55
// slots), data from +0x3b0. Offsets below are the original's (code never depends on them).

#include "SexyApp.h"
#include "Res.h"
#include "Anims.h"
#include "Rect.h"
#include "Image.h"
#include "DDImage.h"
#include "DialogButton.h"
#include <list>
#include <map>
#include <string>
#include <vector>

namespace Sexy
{

class Board;
class TitleScreen;
class SidePanel;
class IntroWidget;
class MapWidget;
class TalkWidget;
class HighScoreWidget;
class DemoWidget;
class HelpWidget;
class Image;
class MemoryImage;
class ImageFont;
class Dialog;
class ButtonWidget;
class Widget;

// app+0x404 .. 0x420
enum FontId
{
	FONT_FONT1 = 0,              // data/font1.txt (gold letters and numbers)       app+0x404
	FONT_MT11 = 1,               // data/ItalianOldStyleMT11Bold.txt                 app+0x408
	FONT_MT22 = 2,               // data/ItalianOldStyleMT22Bold.txt                 app+0x40c
	FONT_YESNO = 3,              // data/yesno.txt                                   app+0x410
	FONT_SCORE = 4,              // data/scorefont.txt                               app+0x414
	FONT_UNUSED = 5,             // app+0x418 (data/font2.txt is never loaded)
	FONT_MEDIUM = 6,             // data/mediumfont.txt                              app+0x41c
	FONT_TUT = 7,                // data/tutfont.txt                                 app+0x420
	NUM_FONTS
};

// Dialog ids (AddDialog / DoDialog / KillDialog)
enum DialogId
{
	DIALOG_NEW_GAME = 0,         // "are you sure you want to end the current game?"
	DIALOG_START_NEW = 1,
	DIALOG_QUIT = 2,
	DIALOG_OPTIONS = 4,
	DIALOG_NEXT_LEVEL = 5,       // level complete
	DIALOG_REGISTER = 6,
	DIALOG_REG_INVALID = 7,
	DIALOG_HIGH_SCORE_NOTE = 8,
	DIALOG_RECORD_SCORE = 10,
	DIALOG_SUBMIT_SCORE = 11,
	DIALOG_HIGH_SCORES = 12,
	DIALOG_BONUS_NAG = 13,
	DIALOG_WARNING = 14,
	DIALOG_CONTINUE = 15,        // "do you want to continue your last game?"
	DIALOG_NO_WINDOWED = 16
};

// Button ids (the sidebar's, the screens')
enum ButtonId
{
	BUTTON_UNDO = 10,
	BUTTON_OPTIONS = 12,
	BUTTON_NEW_GAME = 14,
	BUTTON_QUIT = 16
};

class SevenSeasApp : public SexyApp
{
public:
	enum
	{
		GAME_WIDTH = 640,        // ctor: +0x228 0x280, +0x22c 0x1e0
		GAME_HEIGHT = 480,
		NUM_LOADING_TASKS = 23   // one per sound
	};

	Image**					mImages;            // +0x400 new Image*[88] (NUM_IMAGES), loaded by Init
	ImageFont*				mFonts[NUM_FONTS];  // +0x404 ..
	// the widgets that tell the side panel where the mouse is (its pupils follow it)
	void					FollowMouse(int x, int y);
	void					RemoveDemo();

	SidePanel*				mSidePanel;         // +0x440
	TitleScreen*			mTitleScreen;       // +0x424
	IntroWidget*			mIntroWidget;       // +0x428 the difficulty menu
	HighScoreWidget*		mHighScoreWidget;   // +0x42c the game over high-score screen
	DemoWidget*				mDemoWidget;        // +0x430 the demo before a first easy game
	Board*					mBoard;             // +0x434
	Widget*					mMapWidget;         // +0x438 the map between the chapters
	HelpWidget*				mHelpWidget;        // +0x43c
	std::list<ButtonWidget*>	mButtons;       // +0x448 the buttons made by AddButton / AddDialogButton
	ButtonWidget*			mUndoButton;        // +0x45c id 10
	ButtonWidget*			mOptionsButton;     // +0x460 id 12
	ButtonWidget*			mNewGameButton;     // +0x464 id 14
	ButtonWidget*			mQuitButton;        // +0x468 id 16
	MemoryImage*			mBigPanel;          // +0x46c the 0xc0 x 0xc0 copy of the dialog image
	MemoryImage*			mArrows[9];         // +0x470 .. +0x490: the arrows around the ship (3 x 3, [4] unused)
	int						mMusicSlot;         // +0x4c8 which of the two music slots is playing
	bool					mTutorial;          // +0x494 the tutorial game is on
	int						mDifficulty;        // +0x498 1 beginner, 2 normal, 3 expert
	int						mGamesPlayed;       // +0x3cc
	bool					mFlag4c4;           // +0x4c4 the pirate's welcome (the heading and the spoken line at the first menu) until a game starts
	bool					mTalking;           // +0x4f0 the pirate is talking
	TalkWidget*				mTalkWidget;        // +0x4ec the pirate head at the left
	bool					mBonusNag;          // +0x4f1 registry BonusNag
	bool					mWarnBlue;          // +0x4f2 WarnBlueNag
	bool					mWarnGhost;         // +0x4f3 WarnGhostNag
	bool					mWarnSerpent;       // +0x4f4 WarnSerpNag
	bool					mWarnWhirl;         // +0x4f5 WarnWhirlNag
	SexyString				mLastScoreName;     // +0x4cc registry LastScoreName
	std::map<std::string, AnimDef*>	mAnimDefs;  // +0x4a4 the animations of data\anims.txt
	std::map<std::string, Image*>	mImageMap;  // +0x4b4 the images by name
	bool					mMenuDifficultySaved;

public:
	SevenSeasApp();
	virtual ~SevenSeasApp();

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

	// Test hooks (testhooks/Autoplay.cpp)
	bool					mAutoplay;
	int						mAutoplayLevel;
	int						mAutoplayDiff;
	int						mAutoplayGames;
	int						mAutoplayCap;
	std::string				mAutoplayLog;
	std::string				mAutoplayShots;
	struct AutoplayState*	mAutoplayState;
	void					AutoplayUpdate();
	// the standard test commands (testhooks/Commands.cpp, docs/TEST_COMMANDS.md)
	virtual bool			HandleScriptCommand(const ScriptCommand& theCommand) override;

	// Helpers of the module (WinSS.exe addresses in SevenSeasApp.cpp)
	bool					LoadImage(int theImageId);
	void					LoadAnims();
	AnimDef*				GetAnimDef(const std::string& theName);
	ButtonWidget*			AddButton(int theId, const Rect& theRect, Image* theImage, Image* theOverImage, Image* theDownImage);
	DialogButton*			AddDialogButton(int theId, const Rect& theRect, Image* theImage);
	void					TitleScreenDone();
	void					ShowIntro();
	void					StartGame();
	void					ContinueGame();
	void					OnPlay(int theDifficulty);
	void					EndTutorial();
	void					RemoveIntro();
	void					LevelComplete();
	void					ShowHighScoreWidget();
	void					ShowOptions();
	void					ShowQuitDialog();
	void					ShowNewGameDialog();
	void					ShowContinueDialog();
	void					ShowHighScores();
	void					ShowRecordScore(const SexyString& thePlace);
	void					ShowBonusNag();
	void					ShowWarning(int theWhich);
	void					ShowMap();
	void					ShowHelp();
	void					CloseHelp();
	void					SetMusicOrder(int theOrder, bool theFade);
	void					StartTalk();
	void					StopTalk();
	void					UpdateCursor();
	bool					HasSavedGame();
	void					DeleteSavedGame();
	void					BuildArrows();
};

extern SevenSeasApp* gApp;

}

#endif
