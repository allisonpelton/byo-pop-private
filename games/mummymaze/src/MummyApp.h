#ifndef __MUMMYMAZE_MUMMYAPP_H__
#define __MUMMYMAZE_MUMMYAPP_H__

// MummyApp.cpp: the application class of Mummy Maze Deluxe 1.1 and its helpers (WinMM.exe 0x419bb0-0x4210xx).
// Behaviour spec: docs/GAME_NOTES.md.
//
// Original: class MummyApp : public Sexy::SexyApp (the 2002 SexyApp : ButtonListener), vftable 0x482748 (47 slots), data
// from +0x10380 (the 256 x 256 light mask is at +0x380). Offsets below are the original's (code never depends on them).

#include "SexyApp.h"
#include "Res.h"
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
class MainMenu;
class SidePanel;
class GamePanel;
class LevelCompleteScreen;
class HighScoreScreen;
class VictoryScreen;
class WorldMap;
class Image;
class MemoryImage;
class ImageFont;
class Dialog;
class Widget;

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
	DIALOG_NEW_GAME = 0,
	DIALOG_STARTED_TUTORIAL = 1,
	DIALOG_QUIT = 2,
	DIALOG_QUIT_TO_MAIN = 3,
	DIALOG_OPTIONS = 5,
	DIALOG_REGISTER = 6,
	DIALOG_ADVENTURE_NEW = 0xa,
	DIALOG_NEW_ADVENTURE_NAME = 0xb,
	DIALOG_CONTINUE = 0xe,
	DIALOG_INTRO = 0x10,
	DIALOG_TIP = 0x1b,
	DIALOG_RESET_TIPS = 0x1c
};

class MummyApp : public SexyApp
{
public:
	enum
	{
		GAME_WIDTH = 640,        // ctor: the base SexyApp's +0x228 0x280, +0x22c 0x1e0
		GAME_HEIGHT = 480,
		NUM_LOADING_TASKS = 27   // 25 sounds and two more steps
	};

	Image**					mImages;            // +0x10380 new Image*[165], loaded by Init
	ImageFont*				mFont1;             // +0x10384 data\font1.txt
	ImageFont*				mScoreFont;         // +0x10388 data\scorefont.txt (made by the loading thread)
	ImageFont*				mHeaderFont;        // +0x1038c data\headerfont.txt
	ImageFont*				mBiggestFont;       // +0x10390 data\biggestfont.txt
	ImageFont*				mPyramidFont;       // +0x10394 data\pyramidfont.txt (made by the loading thread)
	TitleScreen*			mTitleScreen;       // +0x10398
	MainMenu*				mMainMenu;          // +0x1039c
	GamePanel*				mGamePanel;         // +0x103a0 the buttons of a game in progress (0x412ea0)
	LevelCompleteScreen*	mLevelComplete;     // +0x103a4 (0x422480)
	HighScoreScreen*		mHighScores;        // +0x103a8 (0x4143a0)
	Widget*					mHighScoreBack;     // +0x103ac (0x414290)
	WorldMap*				mWorldMap;          // +0x103b0 (0x417010)
	Board*					mBoard;             // +0x103b4 (0x403e80)
	VictoryScreen*			mVictory;           // +0x103b8 (0x401ca0)
	SidePanel*				mSidePanel;         // +0x103bc (0x4291d0)
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
	int						mMusicSlot;         // +0x10430 which of the two music slots is playing
	uint8_t					mLightMask[256][256]; // +0x380

	// Test hooks (testhooks/Autoplay.cpp)
	bool					mAutoplay;
	int						mAutoplayGames;
	int						mAutoplayCap;
	std::string				mAutoplayMode;
	std::string				mAutoplayLog;
	std::string				mAutoplayShots;
	struct AutoplayState*	mAutoplayState;
	void					AutoplayUpdate();

public:
	MummyApp();
	virtual ~MummyApp();

	// SexyApp
	virtual void			ButtonPress(int theId) override;
	virtual void			ButtonDepress(int theId) override;
	virtual void			UpdateFrames() override;
	virtual void			LoadingThreadProc() override;
	virtual void			WriteToRegistry() override;
	virtual void			ReadFromRegistry() override;
	virtual Dialog*			NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
								const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) override;
	virtual void			Init() override;
	virtual void			HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;

	// Helpers (WinMM.exe addresses in MummyApp.cpp)
	bool					LoadImage(int theImageId);
	void					SetMusicOrder(int theOrder, bool theFade);
	void					TitleDone();
	void					ShowMainMenu(int theEntry, bool theFade);
	void					ShowQuitToMainDialog();
	void					ShowTutorialDialog();
	void					ShowQuitDialog();
	void					ShowResetTipsDialog();
};

extern MummyApp* gApp;

}

#endif
