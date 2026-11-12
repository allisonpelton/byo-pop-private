#ifndef __ROCKETAPP_H__
#define __ROCKETAPP_H__

// RocketApp.cpp: the application class of Rocket Mania Deluxe 1.0 (the original's WinTubeTricksApp, RTTI vtable
// 0x4c7c24: SexyApp's 62 slots plus one; code 0x42a0d0-0x42f730). Behaviour spec: docs/GAME_NOTES.md.
//
// The original's field offsets are given as comments (app+0x...); the port's base classes have other sizes, so code
// never depends on them.

#include "SexyApp.h"
#include "Res.h"
#include "Rect.h"
#include "Color.h"
#include "Image.h"
#include "MemoryImage.h"
#include "testhooks/UiBot.h"
#include <list>
#include <string>
#include <vector>

namespace Sexy
{

class Board;
class TalkingHead;
class LoadingScreen;
class StateLog;
class UiBot;
class GameSelector;
class Image;
class ImageFont;
class Dialog;
class Widget;
class ButtonWidget;

// the fonts, in the order of the original's table at app+0x5b0 (slot 0, app+0x5b0, is never loaded)
enum FontId
{
	FONT_UNUSED = 0,
	FONT_ARIAL8_BOLD = 1,          // app+0x5b4 data\Arial8Bold.txt
	FONT_ARIAL8 = 2,               // app+0x5b8 data\Arial8.txt
	FONT_ARIAL10 = 3,              // app+0x5bc data\Arial10.txt
	FONT_VERDANA12_BOLD = 4,       // app+0x5c0 data\Verdana12Bold.txt
	FONT_VERDANA18_BOLD = 5,       // app+0x5c4 data\Verdana18Bold.txt
	FONT_SHOWCARD22 = 6,           // app+0x5c8 data\ShowcardGothic22.txt (the field at +4, the ascent, is set to 30 after loading)
	FONT_ARIAL10_BOLD = 7,         // app+0x5cc data\Arial10Bold.txt
	FONT_DOMO36_BOLD = 8,          // app+0x5d0 data\DomoAregato36Bold.txt
	FONT_DOMO22_BOLD = 9,          // app+0x5d4 data\DomoAregato22Bold.txt
	FONT_DOMO14_BOLD = 10,         // app+0x5d8 data\DomoAregato14Bold.txt
	FONT_DOMO11_BOLD = 11,         // app+0x5dc data\DomoAregato11Bold.txt
	FONT_MYRIAD11 = 12,            // app+0x5e0 data\MyriadCondensedWeb11.txt
	NUM_FONTS
};

// the music: the two assembled pieces and the two loops (the original's table entries 179 to 182)
enum Song
{
	SONG_MENU = 0,
	SONG_GAME = 1,
	SONG_GAMEOVER = 2,
	SONG_LEVELUP = 3
};

// the 2003 framework's global generator (the original's at 0x52c658, seeded by SexyAppBase::Init with the tick count): 2003
// initialisation (SeedMTRand2004), which the port's framework Rand() does not have. The game's effects, the head and the
// seed of each board draw from it.
int GlobalRand();
void SeedGlobalRand(unsigned long theSeed);
void GetGlobalRandState(uint32_t* theState, int* theIndex);       // test hook (the state log)

class RocketApp : public SexyApp
{
public:
	enum
	{
		GAME_WIDTH = 640,           // properties/partner.xml: Width 640, Height 480
		GAME_HEIGHT = 480,
		NUM_PIECE_ROWS = 5,
		NUM_PIECE_COLUMNS = 4,
		NUM_PIECE_VARIANTS = 7
	};

	Image**					mImages;                // app+0x5a4 new Image*[114], filled by Init and the loading proc
	ImageFont*				mFonts[NUM_FONTS];      // app+0x5b0
	MemoryImage*			mBubbleFlipH;           // app+0x5a8 the pop-up bubble mirrored left to right
	MemoryImage*			mBubbleFlipV;           // app+0x5ac the pop-up bubble mirrored top to bottom
	Color					mDialogColors[3];       // app+0x5e4, 5e8, 5ec colours 0, 1 and 3 of the dialogs (0x1a5e49, 0x397968, 0x498c7a)
	MemoryImage*			mCheckChecked;          // app+0x5f0 the checkbox picture's second 24x24 half
	MemoryImage*			mCheckUnchecked;        // app+0x5f4 its first half
	MemoryImage*			mPieces[NUM_PIECE_ROWS][NUM_PIECE_COLUMNS][NUM_PIECE_VARIANTS];  // app+0x5f8 the 42x42 tiles of the pipe pieces
	MemoryImage*			mHeadFrames[7];         // app+0x828 the talking head with each mouth frame
	Image*					mHeadRotate1;           // app+0x844 (headrotate1)
	Image*					mHeadRotate2;           // app+0x848 (headrotate2)
	MemoryImage*			mRocketHues[4];         // app+0x84c the magic rocket's picture turned to four hues

	LoadingScreen*			mLoadingScreen;         // app+0x870
	GameSelector*			mGameSelector;          // app+0x890
	Dialog*					mHintDialog;            // app+0x874 the hint dialog opened last (nothing reads it)
	Board*					mBoard;                 // app+0x598 the game in progress, null in the menu
	TalkingHead*			mHead;                  // app+0x59c the announcer, made when the loading screen is done (0x42d360)
	bool					mWelcomed;              // app+0x85c the head has welcomed the player in the menu (once per run)
	bool					mFirstGameHint;         // app+0x85d the first game of the run has been started with the head's words
	int						mHeadCountdown;         // app+0x860 updates until the head says its first lines in a game (0 none)
	int						mMode;                  // app+0x530 the chosen game mode (0 arcade, 1 classic, 2 strategy)
	int						mDifficulty;            // app+0x534 the chosen difficulty (1 easy, 2 medium, 3 hard)
	UiBotParams				mUiParams;              // port: the test hooks (testhooks/UiBot.h)
	StateLog*				mStateLog;
	UiBot*					mUiBot;
	bool					mNoPopLink;             // app+0x548 the "NoPopLink" property
	bool					mNoHints;               // app+0x549 an option of the Options dialog (0x4130b0): no hint arrow, no talking head
	bool					mHintFlags[8];          // app+0x54a to +0x551 the hints that were shown (kept as HintsDisabledFlag%d)
	SexyString				mLastScoreName;         // app+0x538 the name typed for the last high score (kept as LastScoreName)

public:
	RocketApp();
	virtual ~RocketApp();

	// SexyApp
	virtual void			Init() override;
	virtual void			LoadingThreadProc() override;
	virtual Dialog*			NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
								const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) override;  // 0x42d4e0
	virtual bool			KillDialog(int theDialogId) override;       // 0x42d620
	using SexyApp::KillDialog;
	virtual void			ModalOpen() override;                       // 0x42f270 the board is paused while a modal dialog is up
	virtual void			ModalClose() override;                      // 0x42f290
	virtual void			GotFocus() override;                        // 0x42f290
	virtual void			LostFocus() override;                       // 0x42f270
	virtual void			ButtonPress(int theId) override;
	virtual void			ButtonDepress(int theId) override;
	virtual bool			HandleScriptCommand(const ScriptCommand& theCommand) override;
	virtual void			HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;
	virtual void			UpdateFrames() override;
	virtual void			WriteToRegistry() override;                 // 0x42f2b0
	virtual void			ReadFromRegistry() override;                // 0x42f4b0

	// Helpers of the module (RocketManiaGame addresses in RocketApp.cpp)
	bool					LoadImage(int theImageId);
	MemoryImage*			CopyImage(Image* theImage, const Rect& theRect);
	void					GenerateImages();
	void					AssembleMusic(const std::string& thePrefix, char theLastSegment);

	// the loading screen's play link was clicked (the original's slot 62)
	virtual void			LoadingScreenDone();
	void					CreateHead();           // 0x42d360
	void					ShowGameSelector();     // 0x42e010

	// the game modes (0 arcade, 1 classic, 2 strategy) and difficulties (1 easy to 3 hard, 0 none) and their texts,
	// kept in the original at app+0x554 to +0x594
	static const SexyString&	ModeName(int theMode);          // "Arcade"
	static const SexyString&	ModeTitle(int theMode);         // the game's title in that mode
	static const SexyString&	HighScoreTitle(int theMode);    // the high score list's title
	static const SexyString&	DifficultyName(int theDifficulty);
	static const SexyString&	DifficultyNameUpper(int theDifficulty);

	// the saved game of a mode (the file's first four bytes are 0xFADA0002; classic saves to savea, arcade to saveb,
	// strategy to savec)
	static std::string		SavePath(int theMode);          // 0x42f700 (path relative to the user folder)
	bool					HasSavedGame(int theMode);      // 0x4062b0
	bool					DeleteSavedGame(int theMode);   // 0x41d380
	virtual void			ShutdownHook() override;        // 0x42f730
	bool					ReadSavedGame(int theMode, int* theDifficulty, int* theScore, int* theLevel);  // 0x4036f0

	// the first-time explanation of a mode (0x42d670); true when it opened a dialog, which starts the game when closed
	bool					ShowModeHint(int theHint, bool theContinuing);
	void					StartBoard(bool theContinuing); // 0x42ea30

	// the menu's lower buttons
	void					ShowQuitDialog();       // 0x42e390
	void					AddBoardButtons();      // 0x42d1a0 the Pause, Options and Quit buttons beside the board
	void					RemoveBoardButtons();
	void					DisableBoardButtons(bool theDisabled);  // 0x42d270
	void					ShowPauseDialog();      // 0x42d820
	void					QuitGame();             // 0x42e530
	std::list<ButtonWidget*>	mBoardButtons;      // app+0x87c
	void					ShowHighScores(int theOpenedFrom);  // 0x42dcc0 (0 after a game, 1 from the menu, 2 in a game, 3 from the options)
	void					ShowRecordScore(const SexyString& theMessage);  // 0x42df40
	void					ShowOptions(bool theFromMenu);      // 0x42de20
	void					CenterDialog(Dialog* theDialog);    // 0x42e330
	void					ShowCredits();                      // 0x42d760
};

enum DialogId
{
	DIALOG_GAME_OVER = 0,
	DIALOG_OPTIONS = 1,
	DIALOG_RECORD_SCORE = 3,
	DIALOG_QUIT = 5,
	DIALOG_CREDITS = 7,
	DIALOG_HINT = 0xb,
	DIALOG_DIFFICULTY = 0xc,
	DIALOG_LEVEL_UP = 0xd,
	DIALOG_HIGH_SCORES = 0xe,
	DIALOG_CONTINUE = 0xf,
	DIALOG_MESSAGE = 0x10
};

extern RocketApp* gApp;

}

#endif
