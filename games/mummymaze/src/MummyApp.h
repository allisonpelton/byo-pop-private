#ifndef __MUMMYMAZE_MUMMYAPP_H__
#define __MUMMYMAZE_MUMMYAPP_H__

// MummyApp.cpp: the application class of Mummy Maze Deluxe 1.1 and its helpers (WinMM.exe 0x419bb0-0x4210xx).
// Behaviour spec: docs/GAME_NOTES.md.
//
// Original: class MMApp : public Sexy::SexyApp (the 2002 SexyApp : ButtonListener), vftable 0x482748 (47 slots), data
// from +0x10380 (the 256 x 256 light mask is at +0x380). Offsets below are the original's (code never depends on them).

#include "SexyApp.h"
#include "Res.h"
#include "Rect.h"
#include "Image.h"
#include "DDImage.h"
#include "DialogButton.h"
#include "Buffer.h"
#include "MTRand.h"
#include "ModFixes.h"
#include <list>
#include <map>
#include <string>
#include <vector>

namespace Sexy
{

class Board;
class TitleScreen;
class IntroScreen;
class Sidebar;
class GameOverWidget;
class NextLevelWidget;
class HighScoreWidget;
class BeachWidget;
class HighScoreSlide;
class MapWidget;
class Image;
class MemoryImage;
class ImageFont;
class Dialog;
class Widget;

class MMApp;

// The game's pictures by id (IMAGE_X in Res.h). mApp->mImages[IMAGE_X] decodes a picture the first time it is asked for; the
// original decodes all of them at start (0x41db90 for each id), which is most of the load, and a picture that nothing
// asks for in a run is never decoded or held.
class ImageTable
{
public:
	explicit ImageTable(MMApp* theApp);
	Image*					operator[](int theId) { return mImages[theId] != NULL ? mImages[theId] : Load(theId); }

private:
	Image*					Load(int theId);

	MMApp*				mApp;
	Image*					mImages[NUM_IMAGES];
};

// The game modes (app+0x103d8)
enum GameMode
{
	MODE_CLASSIC = 0,
	MODE_ADVENTURE = 1,
	MODE_ADVENTURE_DONE = 2,     // an adventure game that has been won (the reward board)
	MODE_CLASSIC_DONE = 3,       // a classic game that has been won
	MODE_TUTORIAL = 4            // 4 to 8: the parts of the tutorial
};

// Dialog ids (AddDialog / DoDialog / KillDialog)
enum DialogId
{
	DIALOG_STARTED_TUTORIAL = 1,
	DIALOG_QUIT = 2,
	DIALOG_QUIT_TO_MAIN = 3,
	DIALOG_OPTIONS = 5,
	DIALOG_CREDITS = 0x12,
	DIALOG_ADVENTURE_NEW = 0xa,
	DIALOG_NEW_ADVENTURE_NAME = 0xb,
	DIALOG_CONTINUE = 0xe,
	DIALOG_INTRO = 0x10,
	DIALOG_TIP = 0x1b,
	DIALOG_RECORD_SCORE = 0x15,
	DIALOG_HIGH_SCORES = 0x17,
	DIALOG_BAD_SAVE = 7,
	DIALOG_NAME_INVALID = 0xc,
	DIALOG_ANKH = 9,
	DIALOG_MAP_MESSAGE = 0xf,
	DIALOG_ADV_INFO = 0x11,
	DIALOG_DELETE_ADVENTURE = 0x18
};

class MMApp : public SexyApp
{
public:
	enum
	{
		GAME_WIDTH = 640,        // ctor: the base SexyApp's +0x228 0x280, +0x22c 0x1e0
		GAME_HEIGHT = 480,
		NUM_LOADING_TASKS = 27   // 25 sounds and two more steps
	};

	ImageTable				mImages;            // +0x10380 new Image*[NUM_IMAGES] (see ImageTable)
	ImageFont*				mFont1;             // +0x10384 data\font1.txt
	ImageFont*				mScoreFont;         // +0x10388 data\scorefont.txt (made by the loading thread)
	ImageFont*				mHeaderFont;        // +0x1038c data\headerfont.txt
	ImageFont*				mBiggestFont;       // +0x10390 data\biggestfont.txt
	ImageFont*				mPyramidFont;       // +0x10394 data\pyramidfont.txt (made by the loading thread)
	TitleScreen*			mTitleScreen;       // +0x10398
	IntroScreen*				mMainMenu;          // +0x1039c
	GameOverWidget*				mGamePanel;         // +0x103a0 the buttons of a game in progress (0x412ea0)
	NextLevelWidget*	mLevelComplete;     // +0x103a4 (0x422480)
	HighScoreWidget*		mHighScores;        // +0x103a8 (0x4143a0)
	Widget*					mHighScoreBack;     // +0x103ac (0x414290)
	MapWidget*				mWorldMap;          // +0x103b0 (0x417010)
	Board*					mBoard;             // +0x103b4 (0x403e80)
	BeachWidget*			mVictory;           // +0x103b8 (0x401ca0)
	Sidebar*				mSidePanel;         // +0x103bc (0x4291d0)
	int						mGameMode;          // +0x103d8 GameMode
	int						mCurrentSlot;       // +0x1048c the adventure slot (registry "Name")
	int						mSpeed;             // +0x10434 registry "Speed" (default 1)
	bool					mShowClock;         // +0x1042c registry "Clock"
	bool					mShowAnkh;          // +0x1042d registry "Ankh" (default true)
	bool					mTipTwoMummies;     // +0x10e8c registry TwoMummyNag
	bool					mTipGate;           // +0x10e8d GateNag
	bool					mTipTrap;           // +0x10e8e TrapNag
	bool					mTipRed;            // +0x10e8f RedNag
	bool					mTipScorpion;       // +0x10e90 ScorpNag
	bool					mTipDark;           // +0x10e91 DarkNag
	bool					mTipAnkh;           // +0x10e92 AnkhNag
	SexyString				mLastScoreName;     // +0x104b0 registry LastScoreName
	int						mScore;             // +0x10404 the classic game's score
	int						mAdvPyramid;        // +0x10410 the adventure: the pyramid (0 .. 14)
	int						mAdvChamber;        // +0x1040c the adventure: the chamber (0 .. 14) of the pyramid
	int						mClassicPyramid;    // +0x10428 the classic game: the pyramid, 0 .. 14 (14 is the last, a dark one)
	int						mClassicChamber;    // +0x10424 the classic game: the chamber, 0 .. 14 (14 holds the treasure)
	bool					mRestoring;         // +0x104a4 loading a saved game: LoadMap takes the file and maze from the save
	int						mRestoreFile;       // +0x104a0
	int						mRestoreMaze;       // +0x1049c
	bool					mFixedFile;         // +0x10498 LoadMap takes mFixedFileNumber / mFixedMaze (the tutorial)
	int						mFixedFileNumber;   // +0x10494
	int						mFixedMaze;         // +0x10490
	double					mTotalTime;         // +0x10418 the adventure's time played, in seconds
	int						mDeaths;            // +0x10420 how often the explorer was caught in the adventure
	bool					mChamberDone;       // +0x10408 a chamber of the adventure has just been won (the map takes it in)
	SexyString				mAdvName;           // the adventure's player (port only)
	Buffer					mScoreBuffer;       // +0x103ec what the Internet score list is sent: a byte and the level's seconds per level
	MTRand					mRand;              // +0x104cc the app's generator: seeded at start, saved and loaded, never drawn from
	int						mElapsedBase;        // +0x104a8 the time already played, from a saved game
	int						mMusicSlot;         // +0x10430 which of the two music slots is playing
	uint8_t					mLightMask[256][256]; // +0x380

	FixSettings				mFixes;             // port(mods): the fixes mod's toggles, read in Init

	// Test hooks (testhooks/Autoplay.cpp)
	bool					mAutoplay;
	int						mAutoplayGames;
	int						mAutoplayCap;
	std::string				mAutoplayMode;
	std::string				mAutoplayLog;
	std::string				mAutoplayShots;
	struct AutoplayState*	mAutoplayState;
	bool					mSoundTried[NUM_SOUNDS];   // port: the sound's file has been asked for (see LoadSoundOnce)
	void					AutoplayUpdate();
	void					HandleAutoplayParam(const std::string& theParamName, const std::string& theParamValue);

public:
	MMApp();
	virtual ~MMApp();

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
	virtual void			HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;
	virtual bool			HandleScriptCommand(const ScriptCommand& theCommand) override;	// the standard test commands (testhooks/TestCommands.cpp)

	// Helpers (WinMM.exe addresses in MummyApp.cpp)
	void					LoadSoundOnce(int theSoundNum);
	Image*					DoLoadImage(int theImageId);   // 0x41db90: NULL (and the game ends) when the picture is not there
	// the clock of the times that a game takes (the original's GetTickCount): the bot's runs count updates instead, 10 ms each,
	// so that their scores do not depend on how fast the machine is
	// (the fixes mod's gameClock does the same in a normal run: the time of a chamber is the updates it took)
	DWORD					GameTick() { return (mAutoplay || mFastMode || mFixes.mGameClock) ? (DWORD) mUpdateCount * 10 : GetTickCount(); }
	void					DoMusic(int theOrder, bool theFade);
	void					LoadingCompleted();                            // 0x41e970
	void					DoIntro(int theEntry, bool theFade);   // 0x41b160
	void					KillBoard();                          // 0x41c340
	void					KillGameOver();                      // 0x41b4b0
	void					KillNextLevel();                  // 0x41b7b0
	void					KillMap();                       // 0x41c300
	void					NewGame();                     // 0x41e6a0
	void					DoGameOver();                        // 0x41b2f0
	void					KillHighScoreSlide();                 // 0x41c380
	void					DoNextLevel();                      // 0x41b4f0
	void					OnContinueLink();                       // 0x4112b0 case 3
	void					TryConfirmLoad();               // 0x41c410
	void					DoTutorial();                        // 0x41c3c0
	void					ConfirmQuit();                       // 0x41cc30
	void					ConfirmQuitToMain();                 // 0x41cfb0
	void					DoOptions();                          // 0x41eab0
	void					DoHighScoreDialog();                  // 0x41f6a0
	Dialog*					NewDialogOf(Dialog* theDialog);
	void					DoRecordScoreDialog(const SexyString& theLines);   // 0x41f3e0
	void					DoWarning(int theWhich);                  // 0x41b5b0
	void					DoAdventure();                   // 0x41b800
	void					ShowNewAdventure();
	void					DoDeleteDialog();                  // 0x41bb10
	void					DoVersionError();                   // 0x41f0d0
	void					DeleteGame(int theSlot);           // 0x41bc80
	void					CreateAdventure(int theSlot, const SexyString& theName);   // 0x4201e0 case 0x7db
	bool					AdventureExists(int theSlot);
	bool					AdventureName(int theSlot, SexyString& theName);
	void					DoMap(int theSlot, bool theFlag);   // 0x41bcc0
	void					AdventureCaught();                      // 0x406f10, 0x407cc0, 0x4083e0
	void					AdventureLeave();                       // 0x413950, 0x42a3d0
	void					OnWorldMapButton();                     // 0x42a3d0 case 13
	void					DoAnkhExplanation(bool theSolvable);         // 0x41fc70
	void					DoBeach();                          // 0x41e890
	bool					LoadGame();                          // 0x41c600
	bool					HasSavedGame();
	void					DoMessage(int theId, const SexyString& theHeader, const SexyString& theLines, int theMode, int theX, int theY, int theW,
											int theH = -1);
};

extern MMApp* gApp;

}

#endif
