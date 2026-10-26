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
#include <list>
#include <string>
#include <vector>

namespace Sexy
{

class Board;
class LoadingScreen;
class GameSelector;
class Image;
class ImageFont;
class Dialog;
class Widget;

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
	Board*					mBoard;                 // app+0x598 the game in progress, null in the menu
	int						mMode;                  // app+0x530 the chosen game mode (0 arcade, 1 classic, 2 strategy)
	int						mDifficulty;            // app+0x534 the chosen difficulty (1 easy, 2 medium, 3 hard)
	bool					mNoPopLink;             // app+0x548 the "NoPopLink" property
	bool					mNoHints;               // app+0x549 an option of the Options dialog (0x4130b0): no hint arrow, no talking head

public:
	RocketApp();
	virtual ~RocketApp();

	// SexyApp
	virtual void			Init() override;
	virtual void			LoadingThreadProc() override;
	virtual void			ButtonPress(int theId) override;
	virtual void			ButtonDepress(int theId) override;
	virtual bool			HandleScriptCommand(const ScriptCommand& theCommand) override;

	// Helpers of the module (RocketManiaGame addresses in RocketApp.cpp)
	bool					LoadImage(int theImageId);
	MemoryImage*			CopyImage(Image* theImage, const Rect& theRect);
	void					GenerateImages();
	void					AssembleMusic(const std::string& thePrefix, char theLastSegment);

	// the loading screen's play link was clicked (the original's slot 62)
	virtual void			LoadingScreenDone();
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
	bool					ReadSavedGame(int theMode, int* theDifficulty, int* theScore, int* theLevel);  // 0x4036f0

	// the first-time explanation of a mode (0x42d670); true when it opened a dialog, which starts the game when closed
	bool					ShowModeHint(int theHint, bool theForce);
	void					StartBoard(bool theContinuing); // 0x42ea30

	// the menu's lower buttons (the dialogs are not translated yet, the quit question is)
	void					ShowQuitDialog();       // 0x42e390
	void					ShowHighScores(bool theFromMenu);   // 0x42dcc0
	void					ShowOptions(bool theFromMenu);      // 0x42de20
};

enum DialogId
{
	DIALOG_GAME_OVER = 0,
	DIALOG_QUIT = 5,
	DIALOG_HINT = 0xb,
	DIALOG_DIFFICULTY = 0xc,
	DIALOG_CONTINUE = 0xf,
	DIALOG_REGISTER_TO_LOAD = 0x11
};

extern RocketApp* gApp;

}

#endif
